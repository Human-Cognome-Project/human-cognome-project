// Storage investigation for #156. Symbolic definitions only: no positions,
// physical bodies, placement conventions, or production admission interface.
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "engine/runtime.h"
#include "engine/transfer.h"
#include "taichi/jit/jit_module.h"
#include "taichi/rhi/common/host_memory_pool.h"
#include "taichi/rhi/llvm/device_memory_pool.h"
#include "taichi/runtime/program_impls/llvm/llvm_program.h"
#include "taichi/util/bit.h"

namespace {
using namespace taichi::lang;
using Clock = std::chrono::steady_clock;
constexpr int block = 32;

void require(bool ok, const char *message) {
  if (!ok) throw std::runtime_error(message);
}

void cell_op(IRBuilder &b, SNode *node, Stmt *index, SNodeOpType op) {
  auto *ptr = b.create_global_ptr(node, {index});
  ptr->is_cell_access = true;
  b.insert(Stmt::make_typed<SNodeOpStmt>(op, node, ptr));
}

// Same exported runtime queries and backend-aware result fetch as the native
// profiler. These query metadata; they do not allocate a competing pool.
struct Metrics {
  Program &program;
  LlvmProgramImpl &impl;
  explicit Metrics(Program &p)
      : program(p), impl(*static_cast<LlvmProgramImpl *>(p.get_program_impl())) {}

  template <typename T, typename... Args>
  T query(const std::string &key, Args... args) {
    auto *exec = impl.get_runtime_executor();
    exec->get_runtime_jit_module()->call<void *>(
        "runtime_" + key, exec->get_llvm_runtime(), args...);
    return impl.fetch_result<T>(taichi_result_buffer_runtime_query_id,
                               program.result_buffer);
  }
  void *list(SNode *node) {
    return query<void *>("LLVMRuntime_get_element_lists",
                         impl.get_llvm_runtime(), node->id);
  }
  int size(void *list) {
    return query<int>("ListManager_get_num_elements", list);
  }
  std::uint64_t list_bytes(void *list) {
    return query<int>("ListManager_get_num_active_chunks", list) *
           query<std::uint64_t>("ListManager_get_element_size", list) *
           query<std::uint64_t>("ListManager_get_max_num_elements_per_chunk", list);
  }
};

struct Sample {
  MemoryPoolStatistics pool{};
  std::size_t host_cursor_bytes{};
  std::uint64_t requested{}, list_backing{};
  int pointer_elements{}, free_elements{}, free_used{}, recycled_elements{};
  int parent_list{}, leaf_list{}, trees{}, snodes{};
  double compilation{};
};

int main_probe(int argc, char **argv) {
  std::string layout = "dense-budgeted";
  int span = 1048576;
  int repeats = 20;
  bool cuda = false;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--cuda") cuda = true;
    else if (arg == "--layout" && i + 1 < argc) layout = argv[++i];
    else if (arg == "--span" && i + 1 < argc) span = std::stoi(argv[++i]);
    else if (arg == "--repeats" && i + 1 < argc) repeats = std::stoi(argv[++i]);
    else throw std::runtime_error("expected --layout NAME --span N --repeats N [--cuda]");
  }
  require(layout == "dense-budgeted" || layout == "flat-bitmasked" || layout == "pointer-cold" ||
              layout == "pointer-retained" || layout == "pointer-recycled",
          "unknown layout");
  require(span >= 70 && span <= (1 << 26) && repeats >= 1 && repeats <= 10000,
          "span must be 70..67108864; repeats must be 1..10000");
  const bool budgeted = layout == "dense-budgeted";
  const bool sparse = layout != "flat-bitmasked" && !budgeted;
  const bool prepared = layout != "pointer-cold";
  const int capacity = int(taichi::bit::least_pot_bound(span));
  engine::InstanceSettings settings;
  settings.read_environment = false;
  settings.cuda = cuda;
  settings.cpu_threads = 2;
  // Explicit bounded GPU reservation, separate from measured logical requests.
  if (cuda) settings.device_memory_gb = 0.5;
  engine::Runtime runtime(settings);
  require(!cuda || runtime.on_cuda(), "requested CUDA but actual backend is CPU");
  auto &program = runtime.program();
  Metrics metrics(program);
  const auto before_host = HostMemoryPool::get_instance().statistics();
  auto root = std::make_unique<SNode>(0, SNodeType::root);
  auto *root_node = root.get();
  SNode *parent = nullptr;
  if (sparse) parent = &root->pointer(Axis(0), capacity / block);
  SNode *declarations = nullptr;
  if (budgeted) declarations = &root->dense(Axis(0), capacity);
  auto *mask = &(sparse ? parent : root.get())->bitmasked(
      Axis(0), budgeted ? 4 : sparse ? block : capacity);
  auto *value = &(budgeted ? declarations : mask)->insert_children(SNodeType::place);
  value->dt = PrimitiveType::i8;
  SNode *reference = nullptr;
  if (budgeted) {
    reference = &mask->insert_children(SNodeType::place);
    reference->dt = PrimitiveType::i32;
  }
  const auto before_tree = metrics.query<std::uint64_t>("LLVMRuntime_get_total_requested_memory",
                                                       metrics.impl.get_llvm_runtime());
  auto *tree = program.add_snode_tree(std::move(root), false);
  const auto root_bytes = metrics.impl.get_cached_field(tree->id()).root_size;
  const auto rounded_root = ((root_bytes + taichi_page_size - 1) / taichi_page_size) * taichi_page_size;

  // Independent finite numerical budget: four 16-float records, regardless of
  // the declaration span. No payload mapping or object allocator is proposed.
  Ndarray payload(&program, PrimitiveType::f32, {4 * 16});
  Ndarray output(&program, PrimitiveType::i32, {3});
  std::vector<float> retained(4 * 16, 7.25f);
  engine::upload(payload, retained.data(), retained.size() * sizeof(float));

  const auto snapshot = [&] {
    runtime.synchronize();
    Sample s;
    const auto host = HostMemoryPool::get_instance().statistics();
    s.pool = runtime.on_cuda() ? DeviceMemoryPool::get_instance().statistics() :
                               static_cast<MemoryPoolStatistics>(host);
    s.host_cursor_bytes = host.cursor_bytes;
    s.requested = metrics.query<std::uint64_t>("LLVMRuntime_get_total_requested_memory",
                                              metrics.impl.get_llvm_runtime());
    // Scalar places have no traversal list in this fork.
    for (auto *node : {root_node, parent, mask, declarations}) {
      if (!node) continue;
      s.list_backing += metrics.list_bytes(metrics.list(node));
    }
    s.parent_list = parent ? metrics.size(metrics.list(parent)) : 0;
    s.leaf_list = metrics.size(metrics.list(mask));
    if (parent) {
      auto *allocator = metrics.query<void *>("LLVMRuntime_get_node_allocators",
                                              metrics.impl.get_llvm_runtime(), parent->id);
      auto *data = metrics.query<void *>("NodeManager_get_data_list", allocator);
      s.pointer_elements = metrics.size(data);
      s.free_used = metrics.query<int>("NodeManager_get_free_list_used", allocator);
      s.list_backing += metrics.list_bytes(data);
      for (const auto &entry : {std::make_pair("NodeManager_get_free_list", &s.free_elements),
                               std::make_pair("NodeManager_get_recycled_list", &s.recycled_elements)}) {
        auto *list = metrics.query<void *>(entry.first, allocator);
        *entry.second = metrics.size(list);
        s.list_backing += metrics.list_bytes(list);
      }
    }
    s.trees = program.get_snode_tree_size();
    s.snodes = runtime.issued_snode_ids();
    s.compilation = program.get_total_compilation_time();
    return s;
  };
  std::cout << "# storage-only, actual_backend=" << (runtime.on_cuda() ? "cuda" : "cpu")
            << ",layout=" << layout << ",logical_span=" << span
            << ",padded_span=" << capacity << ",root_bytes=" << root_bytes
            << ",root_page_bytes=" << rounded_root
            << ",payload_bytes=" << retained.size() * sizeof(float)
            << ",output_bytes=12,cuda_reservation_gb=" << (cuda ? 0.5 : 0.0) << '\n';
  std::cout << "phase,elapsed_us,definitions,sum_codes,sum_indices,native_requested_bytes,"
               "native_chunk_bytes,pointer_elements,free_elements,free_used,recycled_elements,"
               "parent_list_elements,leaf_list_elements,trees,snode_ids,compilation_seconds,"
               "pool_calls,pool_requested_bytes,pool_raw_calls,pool_reserved_bytes,host_cursor_bytes\n";
  const auto row = [&](const char *phase, double us, const std::vector<int> &counts) {
    const auto s = snapshot();
    std::cout << std::fixed << std::setprecision(3) << phase << ',' << us << ','
              << counts[0] << ',' << counts[1] << ',' << counts[2] << ','
              << s.requested << ',' << s.list_backing << ',' << s.pointer_elements << ','
              << s.free_elements << ',' << s.free_used << ',' << s.recycled_elements << ','
              << s.parent_list << ',' << s.leaf_list << ',' << s.trees << ','
              << s.snodes << ',' << std::setprecision(9) << s.compilation << ','
              << s.pool.allocation_calls << ',' << s.pool.requested_bytes << ','
              << s.pool.raw_allocation_calls << ',' << s.pool.reserved_bytes << ','
              << s.host_cursor_bytes << '\n';
    return s;
  };
  row("materialized", 0, {0, 0, 0});

  IRBuilder edit;
  auto *index = edit.create_arg_load({0}, PrimitiveType::i32, false, 0);
  auto *code = edit.create_arg_load({1}, PrimitiveType::i32, false, 0);
  auto *record = edit.create_arg_load({2}, PrimitiveType::i32, false, 0);
  auto *mask_index = budgeted ? record : index;
  {
    auto define = edit.get_if_guard(edit.create_if(edit.create_cmp_ne(code, edit.get_int32(0))), true);
    if (parent) cell_op(edit, parent, edit.create_div(index, edit.get_int32(block)), SNodeOpType::activate);
    cell_op(edit, mask, mask_index, SNodeOpType::activate);
    if (reference)
      edit.create_global_store(edit.create_global_ptr(reference, {record}), index);
    edit.create_global_store(edit.create_global_ptr(value, {index}),
                             edit.create_cast(code, PrimitiveType::i8));
  }
  {
    auto release = edit.get_if_guard(edit.create_if(edit.create_cmp_eq(code, edit.get_int32(0))), true);
    edit.create_global_store(edit.create_global_ptr(value, {index}),
                             edit.create_cast(code, PrimitiveType::i8));
    cell_op(edit, mask, mask_index, SNodeOpType::deactivate);
  }
  engine::CompiledKernel change(runtime, "prepared_space_change", edit.extract_ir(),
                               {engine::Param::scalar(PrimitiveType::i32),
                                engine::Param::scalar(PrimitiveType::i32),
                                engine::Param::scalar(PrimitiveType::i32)});
  IRBuilder count;
  auto *out = count.create_ndarray_arg_load({0}, PrimitiveType::i32, 1, 0);
  for (int i = 0; i < 3; ++i)
    count.create_global_store(count.create_external_ptr(out, {count.get_int32(i)}), count.get_int32(0));
  auto *loop = count.create_struct_for(mask, false, runtime.cpu_threads());
  {
    auto guard = count.get_loop_guard(loop);
    Stmt *i = count.get_loop_index(loop);
    if (reference) i = count.create_global_load(count.create_global_ptr(reference, {i}));
    count.create_atomic_add(count.create_external_ptr(out, {count.get_int32(0)}), count.get_int32(1));
    count.create_atomic_add(count.create_external_ptr(out, {count.get_int32(1)}),
                            count.create_cast(count.create_global_load(count.create_global_ptr(value, {i})), PrimitiveType::i32));
    count.create_atomic_add(count.create_external_ptr(out, {count.get_int32(2)}), i);
  }
  engine::CompiledKernel enumerate(runtime, "prepared_space_enumerate", count.extract_ir(),
                                  {engine::Param::array(PrimitiveType::i32)});
  IRBuilder read;
  auto *read_index = read.create_arg_load({0}, PrimitiveType::i32, false, 0);
  auto *read_out = read.create_ndarray_arg_load({1}, PrimitiveType::i32, 1, 0);
  read.create_global_store(read.create_external_ptr(read_out, {read.get_int32(0)}),
                           read.create_cast(read.create_global_load(read.create_global_ptr(value, {read_index})), PrimitiveType::i32));
  engine::CompiledKernel read_code(runtime, "prepared_space_read", read.extract_ir(),
                                  {engine::Param::scalar(PrimitiveType::i32),
                                   engine::Param::array(PrimitiveType::i32)});
  const auto at = [&](int i) {
    auto ctx = read_code.make_context();
    ctx.set_arg_int({0}, i);
    ctx.set_arg_ndarray({1}, output);
    read_code.launch(ctx);
    runtime.synchronize();
    int code = -1;
    engine::readback(output, &code, sizeof(code));
    return code;
  };
  const auto visit = [&] {
    auto ctx = enumerate.make_context();
    ctx.set_arg_ndarray({0}, output);
    enumerate.launch(ctx);
    runtime.synchronize();
    std::vector<int> counts(3);
    engine::readback(output, counts.data(), counts.size() * sizeof(int));
    return counts;
  };
  const auto apply = [&](int i, int code) {
    // A bounded fixture guard, not the future identity/budget admission API.
    if (i < 0 || i >= span || code < 0 || code > 127) return false;
    // Explicit fixture bindings to the four prepared numerical records. This
    // tests the split layout, without introducing a production slot allocator.
    const int record = i == 0 ? 0 : i == 32 ? 1 : i == 64 ? 2 : i == span - 1 ? 3 : -1;
    if (budgeted && record < 0) return false;
    auto ctx = change.make_context();
    ctx.set_arg_int({0}, i);
    ctx.set_arg_int({1}, code);
    ctx.set_arg_int({2}, std::max(record, 0));
    change.launch(ctx);
    return true;
  };
  engine::CompiledKernel provision, recycle;
  if (parent) {
    const auto parent_kernel = [&](bool activate) {
      IRBuilder b;
      auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(capacity / block), false, runtime.cpu_threads());
      {
        auto guard = b.get_loop_guard(loop);
        cell_op(b, parent, b.get_loop_index(loop), activate ? SNodeOpType::activate : SNodeOpType::deactivate);
      }
      return engine::CompiledKernel(runtime, activate ? "prepared_space_provision" : "prepared_space_recycle", b.extract_ir(), {});
    };
    provision = parent_kernel(true);
    recycle = parent_kernel(false);  // Compiler inserts native NodeManager GC.
  }
  row("compiled", 0, {0, 0, 0});

  auto start = Clock::now();
  if (parent && prepared) {
    auto ctx = provision.make_context();
    provision.launch(ctx);
  }
  auto counts = visit();  // Prepares traversal lists without defining bodies.
  require(counts == std::vector<int>({0, 0, 0}), "provisioning defined an object");
  if (layout == "pointer-recycled") {
    auto ctx = recycle.make_context();
    recycle.launch(ctx);
    counts = visit();
  }
  // Warm the edit kernel's actual dispatch, then leave no symbolic definition.
  // Cold layout intentionally leaves the first edit dispatch/first touch cold.
  if (prepared) {
    apply(0, 1);
    apply(0, 0);
    require(at(0) == 0, "released definition is readable as present");
    if (layout == "pointer-recycled") {
      auto ctx = recycle.make_context();
      recycle.launch(ctx);
    }
    counts = visit();
  }
  require(counts[0] == 0, "setup must end empty");
  const auto micros = [&] { return std::chrono::duration<double, std::micro>(Clock::now() - start).count(); };
  const auto ready = row("ready_empty", micros(), counts);
  const std::vector<int> ids{0, 32, 64, span - 1};
  const std::vector<int> codes{2, 3, 5, 7};
  const int sum_indices = 96 + span - 1;
  const auto no_growth = [&](const Sample &s) {
    require(s.requested == ready.requested && s.list_backing == ready.list_backing &&
                s.pointer_elements == ready.pointer_elements,
            "prepared operation obtained new native backing");
    require(s.pool.allocation_calls == ready.pool.allocation_calls &&
                s.pool.raw_allocation_calls == ready.pool.raw_allocation_calls &&
                s.pool.requested_bytes == ready.pool.requested_bytes &&
                s.pool.reserved_bytes == ready.pool.reserved_bytes &&
                s.host_cursor_bytes == ready.host_cursor_bytes,
            "prepared operation allocated from native pool or advanced host cursor");
    require(s.trees == ready.trees && s.snodes == ready.snodes && s.compilation == ready.compilation,
            "prepared operation rebuilt layout or compiled code");
  };
  double later_define_us = 0, later_release_us = 0;
  for (int cycle = 0; cycle < repeats; ++cycle) {
    start = Clock::now();
    for (std::size_t i = 0; i < ids.size(); ++i) require(apply(ids[i], codes[i]), "valid definition rejected");
    runtime.synchronize();
    const double define_us = micros();
    if (cycle > 0) later_define_us += define_us;
    counts = visit();
    require(counts == std::vector<int>({4, 17, sum_indices}), "definition values/indices did not round-trip");
    if (cycle == 0) {
      const auto touched = row("first_define", define_us, counts);
      if (prepared) no_growth(touched);
      else require(touched.requested > ready.requested, "cold pointer first-touch control did not allocate");
      for (std::size_t i = 0; i < ids.size(); ++i)
        require(at(ids[i]) == codes[i], "definition code stored at wrong index");
      require(at(1) == 0, "empty potential acquired a definition");
      // Tail padding is outside the logical span; all rejections precede dispatch.
      require(!apply(-1, 1) && !apply(span, 1) && !apply(capacity, 1) && !apply(1, 128), "invalid edit accepted");
      require(visit() == counts, "rejected edit changed existing definitions");
      if (prepared) no_growth(snapshot());
    }
    require(apply(32, 11), "redefinition rejected");
    require(apply(64, 0), "release rejected");
    require(visit() == std::vector<int>({3, 20, sum_indices - 64}), "release/redefinition corrupted survivors");
    start = Clock::now();
    for (int i : ids) require(apply(i, 0), "release rejected");
    if (layout == "pointer-recycled") {
      auto ctx = recycle.make_context();
      recycle.launch(ctx);  // Native recycling/zero-fill cost is in release_us.
    }
    runtime.synchronize();
    const double release_us = micros();
    if (cycle > 0) later_release_us += release_us;
    counts = visit();
    require(counts == std::vector<int>({0, 0, 0}), "release left a live definition");
    if (prepared) no_growth(snapshot());
    if (cycle == 0) row("first_release", release_us, counts);
  }
  if (repeats > 1) {
    std::cout << "# later_define_mean_us=" << later_define_us / (repeats - 1)
              << ",later_release_mean_us=" << later_release_us / (repeats - 1) << '\n';
  }
  // Report native empty-enumeration wall time separately from body count. The
  // native traversal still tests empty container slots/masks; zero bodies is
  // not evidence of zero metadata work or asymptotically constant traversal.
  start = Clock::now();
  for (int i = 0; i < repeats; ++i) counts = visit();
  row("empty_enumeration_mean", micros() / repeats, counts);
  const auto final = row("final_empty", 0, counts);
  if (prepared) no_growth(final);
  std::vector<float> after(retained.size());
  engine::readback(payload, after.data(), after.size() * sizeof(float));
  require(after == retained, "declaration edits modified independent numerical reserve");
  const auto requested_delta = final.requested - before_tree;
  std::cout << "# measured_tree_dynamic_delta_bytes=" << requested_delta
            << ",accounted_tree_bytes_excluding_dynamic_alignment=" << requested_delta + rounded_root
            << ",accounted_bytes_per_logical_entry=" << std::setprecision(6)
            << double(requested_delta + rounded_root) / span
            << ",physical_objects=0,cycles=" << repeats << ",result=PASS\n";
  if (!runtime.on_cuda()) {
    const auto tree_and_buffers = final.host_cursor_bytes - before_host.cursor_bytes;
    std::cout << "# host_cursor_delta_including_alignment_and_buffers=" << tree_and_buffers
              << ",host_cursor_bytes_per_entry_including_268_buffer_bytes="
              << double(tree_and_buffers) / span << '\n';
  }
  return 0;
}
}  // namespace

int main(int argc, char **argv) {
  try { return main_probe(argc, argv); }
  catch (const std::exception &e) {
    std::cerr << "prepared_space_probe: " << e.what() << '\n';
    return 1;
  }
}
