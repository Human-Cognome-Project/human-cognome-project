// Is the engine usable from this workspace?
//
// Builds a field, writes it from a kernel, copies it out through an ndarray
// and checks the values, on every architecture available. It computes
// nothing meaningful on purpose: this checks the link, the runtime and the
// launch path, not a model.
#include <cstdlib>

#include "engine/runtime.h"
#include "engine/transfer.h"
#include "harness.h"
#include "taichi/platform/cuda/detect_cuda.h"

namespace {

using namespace taichi::lang;
using engine::CompiledKernel;
using engine::InstanceSettings;
using engine::Param;
using engine::Runtime;

constexpr int kCells = 32;

// One dense array of i32 under its own tree.
SNode *make_field(Runtime &runtime, int cells) {
  auto root = std::make_unique<SNode>(/*depth=*/0, SNodeType::root);
  SNode &cell = root->dense(Axis(0), cells);
  SNode &leaf = cell.insert_children(SNodeType::place);
  leaf.dt = PrimitiveType::i32;
  SNode *place = &leaf;
  runtime.program().add_snode_tree(std::move(root), /*compile_only=*/false);
  return place;
}

// field[i] = i * scale, with scale passed at launch rather than compiled in.
CompiledKernel make_writer(Runtime &runtime,
                           SNode *field,
                           int cells,
                           const std::string &name) {
  IRBuilder b;
  auto *scale = b.create_arg_load({0}, PrimitiveType::i32, /*is_ptr=*/false,
                                  /*arg_depth=*/0);
  // A top-level loop is the offloaded task and must carry a thread count.
  auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(cells),
                                  /*is_bit_vectorized=*/false,
                                  runtime.cpu_threads());
  {
    auto guard = b.get_loop_guard(loop);
    auto *i = b.get_loop_index(loop);
    b.create_global_store(b.create_global_ptr(field, {i}),
                          b.create_mul(i, scale));
  }
  return CompiledKernel(runtime, name, b.extract_ir(),
                        {Param::scalar(PrimitiveType::i32)});
}

// out[i] = field[i], so the host can read the field back in one transfer.
CompiledKernel make_reader(Runtime &runtime,
                           SNode *field,
                           int cells,
                           const std::string &name) {
  IRBuilder b;
  auto *out = b.create_ndarray_arg_load({0}, PrimitiveType::i32,
                                        /*total_dim=*/1, /*arg_depth=*/0);
  auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(cells),
                                  /*is_bit_vectorized=*/false,
                                  runtime.cpu_threads());
  {
    auto guard = b.get_loop_guard(loop);
    auto *i = b.get_loop_index(loop);
    b.create_global_store(b.create_external_ptr(out, {i}),
                          b.create_global_load(b.create_global_ptr(field, {i})));
  }
  return CompiledKernel(runtime, name, b.extract_ir(),
                        {Param::array(PrimitiveType::i32)});
}

void check_one_arch(bool cuda) {
  const std::string arch = cuda ? "cuda" : "cpu";
  harness::note("arch " + arch);

  InstanceSettings settings;
  settings.cuda = cuda;
  Runtime runtime(settings);
  CHECK_TRUE(runtime.cpu_threads() > 0, arch + ": launch thread count set");

  const int trees_before = runtime.program().get_snode_tree_size();
  const int ids_before = runtime.issued_snode_ids();

  SNode *field = make_field(runtime, kCells);
  CHECK_EQ_INT(runtime.program().get_snode_tree_size(), trees_before + 1,
               arch + ": tree added");
  CHECK_TRUE(runtime.issued_snode_ids() > ids_before,
             arch + ": snode identifiers issued");

  // A second tree on the same live runtime, alongside the first.
  SNode *other = make_field(runtime, kCells);
  CHECK_EQ_INT(runtime.program().get_snode_tree_size(), trees_before + 2,
               arch + ": trees coexist");

  // Kernel names must differ: the engine keys compiled kernels by name, so
  // two kernels sharing one name share one compilation.
  CompiledKernel writer = make_writer(runtime, field, kCells, "write_first");
  CompiledKernel reader = make_reader(runtime, field, kCells, "read_first");
  CompiledKernel other_writer =
      make_writer(runtime, other, kCells, "write_second");
  CompiledKernel other_reader =
      make_reader(runtime, other, kCells, "read_second");

  Ndarray out(&runtime.program(), PrimitiveType::i32,
              std::vector<int>{kCells});
  std::vector<int> host(kCells, -1);

  // Two launches of one compiled kernel with different arguments: the scale
  // is not baked into the kernel.
  for (int scale : {1, 7}) {
    auto write = writer.make_context();
    write.set_arg_int({0}, scale);
    writer.launch(write);
    auto read = reader.make_context();
    read.set_arg_ndarray({0}, out);
    reader.launch(read);
    runtime.synchronize();
    engine::readback(out, host.data(), host.size() * sizeof(int));

    bool correct = true;
    for (int i = 0; i < kCells; ++i) {
      correct = correct && host[i] == i * scale;
    }
    CHECK_TRUE(correct, arch + ": field written and read back, scale " +
                            std::to_string(scale));
  }

  // The second tree is independent: writing it leaves the first alone.
  auto write_other = other_writer.make_context();
  write_other.set_arg_int({0}, 3);
  other_writer.launch(write_other);
  auto read_other = other_reader.make_context();
  read_other.set_arg_ndarray({0}, out);
  other_reader.launch(read_other);
  runtime.synchronize();
  engine::readback(out, host.data(), host.size() * sizeof(int));
  bool second_correct = true;
  for (int i = 0; i < kCells; ++i) {
    second_correct = second_correct && host[i] == i * 3;
  }
  CHECK_TRUE(second_correct, arch + ": second tree holds its own values");

  auto read_first = reader.make_context();
  read_first.set_arg_ndarray({0}, out);
  reader.launch(read_first);
  runtime.synchronize();
  engine::readback(out, host.data(), host.size() * sizeof(int));
  bool first_intact = true;
  for (int i = 0; i < kCells; ++i) {
    first_intact = first_intact && host[i] == i * 7;
  }
  CHECK_TRUE(first_intact, arch + ": first tree unchanged");
}

// Startup capacities: where they come from, and that one instance choice
// does not leak into the next. The expected fallback is the engine own
// default, read from the engine rather than repeated here.
// Sparse containers, so the run touches the list-manager path that dense
// fields never reach: a pointer container with a dense block under it,
// scattered activation, then a struct-for over what is active.
void check_sparse(bool cuda) {
  const std::string arch = cuda ? "cuda" : "cpu";
  harness::note("sparse " + arch);

  InstanceSettings settings;
  settings.cuda = cuda;
  settings.snode_capacity = 4096;
  settings.snode_tree_capacity = 1024;
  Runtime runtime(settings);

  constexpr int kBlocks = 64;
  constexpr int kBlockCells = 16;
  constexpr int kStride = 5;  // activates every fifth block

  SNode *sparse = nullptr;
  SNode *sparse_cell = nullptr;
  {
    auto root = std::make_unique<SNode>(/*depth=*/0, SNodeType::root);
    SNode &block = root->pointer(Axis(0), kBlocks);
    SNode &cell = block.dense(Axis(0), kBlockCells);
    SNode &leaf = cell.insert_children(SNodeType::place);
    leaf.dt = PrimitiveType::i32;
    sparse = &leaf;
    // A struct-for iterates a container, not the place leaf under it.
    sparse_cell = &cell;
    runtime.program().add_snode_tree(std::move(root), /*compile_only=*/false);
  }
  SNode *counter = make_field(runtime, 1);

  const int active_blocks = (kBlocks + kStride - 1) / kStride;

  // Writing into a pointer container activates the block it lands in.
  CompiledKernel activate = [&] {
    IRBuilder b;
    auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(active_blocks),
                                    false, runtime.cpu_threads());
    {
      auto guard = b.get_loop_guard(loop);
      auto *i = b.get_loop_index(loop);
      auto *index = b.create_mul(i, b.get_int32(kStride * kBlockCells));
      b.create_global_store(b.create_global_ptr(sparse, {index}),
                            b.create_add(i, b.get_int32(1)));
    }
    return CompiledKernel(runtime, "activate_sparse", b.extract_ir(), {});
  }();

  // A struct-for visits only what is active, which is the list the engine
  // builds through its list managers.
  CompiledKernel count = [&] {
    IRBuilder b;
    auto *zero = b.get_int32(0);
    b.create_global_store(b.create_global_ptr(counter, {zero}), zero);
    auto *loop = b.create_struct_for(sparse_cell, false, runtime.cpu_threads());
    {
      auto guard = b.get_loop_guard(loop);
      b.create_atomic_add(b.create_global_ptr(counter, {b.get_int32(0)}),
                          b.get_int32(1));
    }
    return CompiledKernel(runtime, "count_sparse", b.extract_ir(), {});
  }();

  CompiledKernel read = make_reader(runtime, counter, 1, "read_counter");

  auto activate_ctx = activate.make_context();
  activate.launch(activate_ctx);
  auto count_ctx = count.make_context();
  count.launch(count_ctx);
  runtime.synchronize();

  Ndarray out(&runtime.program(), PrimitiveType::i32, std::vector<int>{1});
  auto read_ctx = read.make_context();
  read_ctx.set_arg_ndarray({0}, out);
  read.launch(read_ctx);
  runtime.synchronize();
  int visited = 0;
  engine::readback(out, &visited, sizeof(int));

  CHECK_EQ_INT(visited, active_blocks * kBlockCells,
               arch + ": struct-for visited every active cell");
}

void check_configuration() {
  harness::note("configuration");
  const int engine_default = CompileConfig{}.llvm_snode_capacity;

  setenv("TI_LLVM_SNODE_CAPACITY", "2048", 1);
  {
    InstanceSettings settings;
    Runtime runtime(settings);
    CHECK_EQ_INT(runtime.program().compile_config().llvm_snode_capacity, 2048,
                 "environment capacity is read");
  }
  {
    InstanceSettings settings;
    settings.snode_capacity = 4096;
    Runtime runtime(settings);
    CHECK_EQ_INT(runtime.program().compile_config().llvm_snode_capacity, 4096,
                 "explicit capacity overrides the environment");
  }
  {
    InstanceSettings settings;
    settings.read_environment = false;
    Runtime runtime(settings);
    CHECK_EQ_INT(runtime.program().compile_config().llvm_snode_capacity,
                 engine_default,
                 "environment can be ignored, and nothing leaked");
  }
  unsetenv("TI_LLVM_SNODE_CAPACITY");
}

}  // namespace

int main() {
  check_configuration();
  check_one_arch(/*cuda=*/false);
  check_sparse(/*cuda=*/false);
  if (taichi::is_cuda_api_available()) {
    check_one_arch(/*cuda=*/true);
    check_sparse(/*cuda=*/true);
  } else {
    harness::note("cuda unavailable on this host: GPU checks skipped");
  }
  return harness::report("engine_smoke_test");
}
