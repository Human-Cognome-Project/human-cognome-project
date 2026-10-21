// Numerical counters here are labelled storage fixtures, not model mechanics
// or production physical initialization. Existing field kernels are unchanged.
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

#include "engine/record_map.h"
#include "engine/transfer.h"
#include "taichi/jit/jit_module.h"
#include "taichi/rhi/common/host_memory_pool.h"
#include "taichi/rhi/llvm/device_memory_pool.h"
#include "taichi/runtime/program_impls/llvm/llvm_program.h"

// Counts C++ new/new[] in the linked executable, including worker threads.
// It does not observe arbitrary malloc, physical page faults or CUDA driver
// APIs.
namespace allocation_observation {
std::atomic<bool> enabled{false};
std::atomic<std::uint64_t> calls{0}, bytes{0};
void record(std::size_t n) {
  if (enabled.load(std::memory_order_relaxed)) {
    calls.fetch_add(1, std::memory_order_relaxed);
    bytes.fetch_add(n, std::memory_order_relaxed);
  }
}
void *allocate(std::size_t n) {
  record(n);
  if (void *p = std::malloc(std::max<std::size_t>(n, 1)))
    return p;
  throw std::bad_alloc();
}
}  // namespace allocation_observation
void *operator new(std::size_t n) {
  return allocation_observation::allocate(n);
}
void *operator new[](std::size_t n) {
  return allocation_observation::allocate(n);
}
void operator delete(void *p) noexcept {
  std::free(p);
}
void operator delete[](void *p) noexcept {
  std::free(p);
}
void operator delete(void *p, std::size_t) noexcept {
  std::free(p);
}
void operator delete[](void *p, std::size_t) noexcept {
  std::free(p);
}
void *operator new(std::size_t n, std::align_val_t a) {
  allocation_observation::record(n);
  void *p = nullptr;
  if (posix_memalign(&p, static_cast<std::size_t>(a),
                     std::max<std::size_t>(n, 1)) == 0)
    return p;
  throw std::bad_alloc();
}
void *operator new[](std::size_t n, std::align_val_t a) {
  return ::operator new(n, a);
}
void operator delete(void *p, std::align_val_t) noexcept {
  std::free(p);
}
void operator delete[](void *p, std::align_val_t) noexcept {
  std::free(p);
}
void operator delete(void *p, std::size_t, std::align_val_t) noexcept {
  std::free(p);
}
void operator delete[](void *p, std::size_t, std::align_val_t) noexcept {
  std::free(p);
}

namespace {
using namespace taichi::lang;
using engine::RecordStatus;
void require(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
struct Metrics {
  Program &program;
  LlvmProgramImpl &impl;
  explicit Metrics(Program &p)
      : program(p),
        impl(*static_cast<LlvmProgramImpl *>(p.get_program_impl())) {
  }
  template <typename T, typename... Args>
  T query(const std::string &key, Args... args) {
    auto *exec = impl.get_runtime_executor();
    exec->get_runtime_jit_module()->call<void *>(
        "runtime_" + key, exec->get_llvm_runtime(), args...);
    return impl.fetch_result<T>(taichi_result_buffer_runtime_query_id,
                                program.result_buffer);
  }
  std::uint64_t bytes(void *list) {
    return query<int>("ListManager_get_num_active_chunks", list) *
           query<std::uint64_t>("ListManager_get_element_size", list) *
           query<std::uint64_t>("ListManager_get_max_num_elements_per_chunk",
                                list);
  }
};
struct Sample {
  MemoryPoolStatistics pool{};
  std::uint64_t requested{}, chunks{};
  std::size_t cursor{};
  int elements{}, free_used{}, trees{}, snodes{};
  double compilation{};
};
bool same_backing(const Sample &a, const Sample &b) {
  return a.requested == b.requested && a.chunks == b.chunks &&
         a.elements == b.elements &&
         a.pool.allocation_calls == b.pool.allocation_calls &&
         a.pool.requested_bytes == b.pool.requested_bytes &&
         a.pool.raw_allocation_calls == b.pool.raw_allocation_calls &&
         a.pool.reserved_bytes == b.pool.reserved_bytes &&
         a.cursor == b.cursor && a.trees == b.trees && a.snodes == b.snodes &&
         a.compilation == b.compilation;
}
bool same_handle(const engine::RecordHandle &a, const engine::RecordHandle &b) {
  return a.context == b.context && a.incarnation == b.incarnation &&
         a.address == b.address && a.record == b.record;
}
int run(int argc, char **argv) {
  int records = 70, span = 1048576, cycles = 32;
  bool cuda = false, direct = false;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--records" && i + 1 < argc)
      records = std::stoi(argv[++i]);
    else if (arg == "--span" && i + 1 < argc)
      span = std::stoi(argv[++i]);
    else if (arg == "--cycles" && i + 1 < argc)
      cycles = std::stoi(argv[++i]);
    else if (arg == "--cuda")
      cuda = true;
    else if (arg == "--direct")
      direct = true;
    else
      throw std::runtime_error(
          "expected --records N --span N --cycles N [--cuda|--direct]");
  }
  require(records >= 2 && records <= 4096 && span >= 1048576 &&
              span <= (1 << 24) && !(span & (span - 1)) && cycles >= 1 &&
              cycles <= 32 && !(cuda && direct),
          "invalid fixture configuration");
  engine::InstanceSettings settings;
  settings.read_environment = false;
  settings.cuda = cuda;
  settings.cpu_threads = 2;
  if (cuda)
    settings.device_memory_gb = 0.5;
  engine::Runtime runtime(settings);
  require(!cuda || runtime.on_cuda(), "requested CUDA, actual backend is CPU");
  auto &program = runtime.program();
  Metrics metrics(program);
  const auto setup_requested =
      metrics.query<std::uint64_t>("LLVMRuntime_get_total_requested_memory",
                                   metrics.impl.get_llvm_runtime());
  const auto setup_cursor =
      HostMemoryPool::get_instance().statistics().cursor_bytes;
  engine::PreparedRecordMap map(runtime, span, records);
  const auto &binding = map.bindings();
  const auto sample = [&] {
    runtime.synchronize();
    Sample s;
    const auto host = HostMemoryPool::get_instance().statistics();
    s.pool = runtime.on_cuda() ? DeviceMemoryPool::get_instance().statistics()
                               : static_cast<MemoryPoolStatistics>(host);
    s.cursor = host.cursor_bytes;
    auto *rt = metrics.impl.get_llvm_runtime();
    s.requested = metrics.query<std::uint64_t>(
        "LLVMRuntime_get_total_requested_memory", rt);
    for (auto *node : {binding.presence->parent, binding.presence,
                       binding.declaration->parent, binding.allocator})
      s.chunks += metrics.bytes(
          metrics.query<void *>("LLVMRuntime_get_element_lists", rt, node->id));
    auto *allocator = metrics.query<void *>("LLVMRuntime_get_node_allocators",
                                            rt, binding.allocator->id);
    auto *data = metrics.query<void *>("NodeManager_get_data_list", allocator);
    s.elements = metrics.query<int>("ListManager_get_num_elements", data);
    for (const auto &name :
         {"NodeManager_get_data_list", "NodeManager_get_free_list",
          "NodeManager_get_recycled_list"})
      s.chunks += metrics.bytes(metrics.query<void *>(name, allocator));
    s.free_used =
        metrics.query<int>("NodeManager_get_free_list_used", allocator);
    s.trees = program.get_snode_tree_size();
    s.snodes = runtime.issued_snode_ids();
    s.compilation = program.get_total_compilation_time();
    return s;
  };
  const auto first_ready = sample();
  require(map.ready() && map.size() == 0 && map.generation() == 0 &&
              first_ready.elements == records,
          "preparation created definitions or padded allocator rows");
  const auto observed = [&](auto action) {
    allocation_observation::calls = 0;
    allocation_observation::bytes = 0;
    allocation_observation::enabled = true;
    auto result = action();
    allocation_observation::enabled = false;
    return std::make_pair(result, std::array<std::uint64_t, 2>{
                                      allocation_observation::calls.load(),
                                      allocation_observation::bytes.load()});
  };
  const auto first_definition =
      observed([&] { return map.define(span - 1, 255); });
  const auto first = first_definition.first;
  require(
      first.status == RecordStatus::Ok && map.inspect(first.handle).kind == 255,
      "first definition or unsigned byte failed");
  require(same_backing(first_ready, sample()),
          "first definition acquired backing or compiled");
  const auto first_release =
      observed([&] { return map.release(first.handle); });
  require(first_release.first.status == RecordStatus::Ok,
          "first release failed");
  require(same_backing(first_ready, sample()),
          "first release acquired backing or compiled");

  // Fixed numerical capacity, independently bound to the native reference row.
  Ndarray payload(&program, PrimitiveType::f32, {2, records});
  Ndarray tags(&program, PrimitiveType::i64, {records});
  Ndarray count(&program, PrimitiveType::i32, {1});
  std::vector<float> values(2 * records, -1.0f);
  std::vector<std::int64_t> initial_tags(records, 0);
  engine::upload(payload, values.data(), values.size() * sizeof(float));
  engine::upload(tags, initial_tags.data(),
                 initial_tags.size() * sizeof(std::int64_t));
  IRBuilder b;
  auto *v = b.create_ndarray_arg_load({0}, PrimitiveType::f32, 2, 0);
  auto *t = b.create_ndarray_arg_load({1}, PrimitiveType::i64, 1, 0);
  auto *out = b.create_ndarray_arg_load({2}, PrimitiveType::i32, 1, 0);
  b.create_global_store(b.create_external_ptr(out, {b.get_int32(0)}),
                        b.get_int32(0));
  auto *loop =
      b.create_struct_for(binding.presence, false, runtime.cpu_threads());
  {
    auto guard = b.get_loop_guard(loop);
    auto *row = b.get_loop_index(loop);
    auto *version =
        b.create_global_load(b.create_global_ptr(binding.incarnation, {row}));
    auto *tag = b.create_external_ptr(t, {row});
    {
      auto fresh = b.get_if_guard(
          b.create_if(b.create_cmp_ne(b.create_global_load(tag), version)),
          true);
      auto *key =
          b.create_global_load(b.create_global_ptr(binding.address, {row}));
      b.create_global_store(b.create_external_ptr(v, {b.get_int32(0), row}),
                            b.create_cast(key, PrimitiveType::f32));
      b.create_global_store(b.create_external_ptr(v, {b.get_int32(1), row}),
                            b.get_float32(0));
      b.create_global_store(tag, version);
    }
    b.create_atomic_add(b.create_external_ptr(v, {b.get_int32(1), row}),
                        b.get_float32(1));
    b.create_atomic_add(b.create_external_ptr(out, {b.get_int32(0)}),
                        b.get_int32(1));
  }
  engine::CompiledKernel advance(
      runtime, "record_fixture_" + std::to_string(binding.presence->id),
      b.extract_ir(),
      {engine::Param::array(PrimitiveType::f32, 2),
       engine::Param::array(PrimitiveType::i64),
       engine::Param::array(PrimitiveType::i32)});
  auto ctx = advance.make_context();
  if (direct)
    ctx.set_arg_external_array_with_shape(
        {0}, reinterpret_cast<std::uint64_t>(values.data()),
        values.size() * sizeof(float), {2, records});
  else
    ctx.set_arg_ndarray({0}, payload);
  ctx.set_arg_ndarray({1}, tags);
  ctx.set_arg_ndarray({2}, count);
  const auto tick = [&] {
    advance.launch(ctx);
    runtime.synchronize();
    int visited = -1;
    engine::readback(count, &visited, sizeof(visited));
    require(visited == int(map.size()),
            "native traversal included holes or omitted presence");
    if (!direct)
      engine::readback(payload, values.data(), values.size() * sizeof(float));
  };
  tick();  // warm actual empty dispatch with all bindings retained
  const auto ready = sample();
  std::vector<engine::RecordHandle> handles(records);
  std::vector<int> expected(records, 0);
  std::uint64_t token = 0, defined = 1, released = 1;
  const auto address = [&] {
    return int((token++ * 104729 + 17) % std::uint64_t(span));
  };
  const auto define = [&](int key, int kind) {
    const auto r = map.define(key, kind);
    require(r.status == RecordStatus::Ok && r.handle.record >= 0 &&
                r.handle.record < records,
            "definition returned duplicate, missing or padded row");
    ++defined;
    return r.handle;
  };
  const auto verify = [&] {
    std::vector<bool> rows(records, false);
    for (int i = 0; i < records; ++i) {
      if (handles[i].record < 0)
        continue;
      const int row = handles[i].record;
      require(!rows[row], "two live addresses share numerical record");
      rows[row] = true;
      require(values[row] == float(handles[i].address) &&
                  values[records + row] == float(expected[i]),
              "survivor reset, stale payload or incorrect capacity stride");
    }
  };
  for (int i = 0; i < records; ++i)
    handles[i] = define(address(), i % 255 + 1);
  tick();
  std::fill(expected.begin(), expected.end(), 1);
  verify();
  const auto full = sample();
  const auto observed_lookup =
      observed([&] { return map.lookup(handles[0].address); });
  require(observed_lookup.first.status == RecordStatus::Ok,
          "observed lookup failed");
  const auto full_generation = map.generation();
  require(map.define(address(), 1).status == RecordStatus::CapacityExhausted &&
              map.generation() == full_generation &&
              sample().free_used == full.free_used,
          "exhaustion advanced cursor or committed a generation");
  require(
      map.define(handles[0].address, 1).status == RecordStatus::AlreadyDefined,
      "duplicate lost precedence at capacity");
  require(map.define(-1, 1).status == RecordStatus::InvalidInput &&
              map.define(span, 1).status == RecordStatus::InvalidInput &&
              map.define(0, 256).status == RecordStatus::InvalidInput &&
              map.define(0, 0).status == RecordStatus::InvalidInput,
          "invalid address or byte admitted");
  auto foreign = handles[0];
  ++foreign.context;
  require(map.inspect(foreign).status == RecordStatus::WrongContext &&
              map.generation() == full_generation,
          "foreign reference changed state");
  for (int cycle = 0; cycle < cycles; ++cycle) {
    std::vector<engine::RecordHandle> stale;
    for (int i = cycle % 2; i < records; i += 2) {
      stale.push_back(handles[i]);
      require(map.release(handles[i]).status == RecordStatus::Ok,
              "release failed");
      ++released;
      const auto generation = map.generation();
      require(map.release(handles[i]).status == RecordStatus::Stale &&
                  map.inspect(handles[i]).status == RecordStatus::Stale &&
                  map.generation() == generation,
              "released reference remained valid or changed generation");
      handles[i] = {};
      expected[i] = 0;
    }
    tick();
    for (int i = 0; i < records; ++i)
      if (handles[i].record >= 0)
        ++expected[i];
    verify();
    const auto missing = map.lookup(stale[0].address);
    require(missing.status == RecordStatus::NotFound &&
                missing.lookup_entries == map.size(),
            "lookup scanned potential space or returned released key");
    bool reused_address = false;
    for (int i = 0; i < records; ++i)
      if (handles[i].record < 0) {
        handles[i] = define(reused_address ? address() : stale[0].address,
                            (i + cycle) % 255 + 1);
        reused_address = true;
      }
    for (const auto &old : stale)
      require(map.inspect(old).status == RecordStatus::Stale,
              "reuse resurrected stale reference");
    tick();
    for (auto &value : expected)
      ++value;
    verify();
    auto changed = map.set_kind(handles[0], 255);
    require(changed.status == RecordStatus::Ok && changed.kind == 255 &&
                same_handle(changed.handle, handles[0]),
            "kind update replaced identity");
    const auto found = map.lookup(handles[cycle % records].address);
    require(found.status == RecordStatus::Ok &&
                found.lookup_entries == std::uint64_t(records) &&
                same_handle(found.handle, handles[cycle % records]),
            "lookup did not resolve exact native binding");
    require(same_backing(ready, sample()),
            "update/reuse acquired native backing or compiled");
  }
  for (const auto &handle : handles) {
    require(map.release(handle).status == RecordStatus::Ok,
            "final release failed");
    ++released;
  }
  tick();
  require(map.size() == 0 && map.lookup(0).lookup_entries == 0 &&
              same_backing(ready, sample()),
          "final empty map retained bodies or grew backing");
  const auto final = sample();
  const auto root_bytes =
      metrics.impl.get_cached_field(binding.presence->get_snode_tree_id())
          .root_size;
  std::cout << "backend=" << (runtime.on_cuda() ? "cuda" : "cpu")
            << ",path=" << (direct ? "direct" : "bulk") << ",potential=" << span
            << ",records=" << records << ",cycles=" << cycles
            << ",defined=" << defined << ",released=" << released << '\n';
  std::cout << "# map_setup: root_bytes=" << root_bytes
            << ",dynamic_requested_delta="
            << first_ready.requested - setup_requested
            << ",host_cursor_delta_including_alignment_and_80_command_bytes="
            << first_ready.cursor - setup_cursor << '\n';
  std::cout << "phase,native_requested_bytes,native_chunk_bytes,native_"
               "elements,pool_calls,pool_requested_bytes,"
               "pool_raw_calls,pool_reserved_bytes,host_cursor_bytes,trees,"
               "snode_ids,compilation_seconds\n";
  const auto row = [&](const char *name, const Sample &s) {
    std::cout << name << ',' << s.requested << ',' << s.chunks << ','
              << s.elements << ',' << s.pool.allocation_calls << ','
              << s.pool.requested_bytes << ',' << s.pool.raw_allocation_calls
              << ',' << s.pool.reserved_bytes << ',' << s.cursor << ','
              << s.trees << ',' << s.snodes << ',' << s.compilation << '\n';
  };
  row("ready", ready);
  row("final", final);
  std::cout
      << "# C++ new calls/bytes (not all host/driver allocation): first_define="
      << first_definition.second[0] << '/' << first_definition.second[1]
      << ",first_release=" << first_release.second[0] << '/'
      << first_release.second[1] << ",lookup=" << observed_lookup.second[0]
      << '/' << observed_lookup.second[1] << '\n';
  // Independent contexts and logical zero capacity, outside the growth
  // interval.
  {
    engine::PreparedRecordMap tiny(runtime, 3, 1), empty(runtime, 3, 0);
    const auto a = tiny.define(1, 1);
    require(a.status == RecordStatus::Ok &&
                tiny.release(a.handle).status == RecordStatus::Ok,
            "one-record map failed");
    const auto again = tiny.define(1, 2);
    require(again.status == RecordStatus::Ok &&
                again.handle.record == a.handle.record &&
                again.handle.incarnation != a.handle.incarnation &&
                tiny.inspect(a.handle).status == RecordStatus::Stale,
            "same-address reuse did not invalidate prior incarnation");
    const auto tiny_generation = tiny.generation();
    require(tiny.define(3, 1).status == RecordStatus::InvalidInput &&
                tiny.generation() == tiny_generation,
            "potential padding admitted an address");
    require(map.inspect(again.handle).status == RecordStatus::WrongContext &&
                empty.define(0, 1).status == RecordStatus::CapacityExhausted &&
                empty.size() == 0 && empty.generation() == 0,
            "cross-context or zero-budget isolation failed");
  }
  std::cout
      << "record_map: PASS (storage fixtures only; no GPU qualification)\n";
  return 0;
}
}  // namespace
int main(int argc, char **argv) {
  try {
    return run(argc, argv);
  } catch (const std::exception &e) {
    std::cerr << "record_map: " << e.what() << '\n';
    return 1;
  }
}
