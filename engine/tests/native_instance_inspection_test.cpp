// #343: native storage/compilation witness, not an HCP composition or force law.
// One layout, two indexed instances, two repeated subassemblies per instance,
// two integer parts per subassembly. Attachment and revision are explicit data.
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include "engine/runtime.h"
#include "engine/transfer.h"
#include "harness.h"
#include "prepared_metrics.h"
#include "taichi/ir/transforms.h"

namespace {
using namespace taichi::lang;
using engine::CompiledKernel;
using engine::Param;

void require(bool ok, const char *message) {
  if (!ok) throw std::runtime_error(message);
}

struct Layout {
  SNode *instances{}, *parts{}, *value{}, *revision{}, *attachment{};
};

SNode *place(SNode &parent) {
  auto &leaf = parent.insert_children(SNodeType::place);
  leaf.dt = PrimitiveType::i32;
  return &leaf;
}

Layout nested_layout(SNode &root) {
  auto &instances = root.pointer(Axis(0), 2);
  auto &subassemblies = instances.dense(Axis(1), 2);
  auto &parts = subassemblies.dense(Axis(2), 2);
  return {&instances, &parts, place(parts), place(instances), place(instances)};
}

CompiledKernel compile(engine::Runtime &runtime, IRBuilder &b,
                       const std::string &name,
                       const std::vector<Param> &params, bool emit) {
  auto body = b.extract_ir();
  if (emit) {
    std::string ir;
    irpass::print(body.get(), &ir);
    std::ofstream out(name + ".tir");
    out << ir;
    require(bool(out), "could not write input IR");
  }
  return CompiledKernel(runtime, name, std::move(body), params);
}

// The selected coordinate, base and revision are launch arguments, not new
// SNodes or new kernel definitions. All values below are synthetic integers.
CompiledKernel writer(engine::Runtime &runtime, const Layout &l, bool emit) {
  IRBuilder b;
  auto *instance = b.create_arg_load({0}, PrimitiveType::i32, false, 0);
  auto *base = b.create_arg_load({1}, PrimitiveType::i32, false, 0);
  auto *revision = b.create_arg_load({2}, PrimitiveType::i32, false, 0);
  b.create_global_store(b.create_global_ptr(l.revision, {instance}), revision);
  auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(4), false,
                                  runtime.cpu_threads());
  {
    auto guard = b.get_loop_guard(loop);
    auto *p = b.get_loop_index(loop);
    auto *j = b.create_div(p, b.get_int32(2));
    auto *k = b.create_mod(p, b.get_int32(2));
    b.create_global_store(b.create_global_ptr(l.value, {instance, j, k}),
                          b.create_add(base, p));
  }
  return compile(runtime, b, "instance_write",
                 {Param::scalar(PrimitiveType::i32),
                  Param::scalar(PrimitiveType::i32),
                  Param::scalar(PrimitiveType::i32)}, emit);
}

CompiledKernel attachment(engine::Runtime &runtime, const Layout &l, bool emit) {
  IRBuilder b;
  auto *instance = b.create_arg_load({0}, PrimitiveType::i32, false, 0);
  auto *value = b.create_arg_load({1}, PrimitiveType::i32, false, 0);
  b.create_global_store(b.create_global_ptr(l.attachment, {instance}), value);
  return compile(runtime, b, "instance_attachment",
                 {Param::scalar(PrimitiveType::i32),
                  Param::scalar(PrimitiveType::i32)}, emit);
}

// Read all parts through native sparse traversal. Scalar instance metadata is
// copied separately; reading inactive coordinates does not activate them.
CompiledKernel reader(engine::Runtime &runtime, const Layout &l, bool emit) {
  IRBuilder b;
  auto *out = b.create_ndarray_arg_load({0}, PrimitiveType::i32, 1, 0);
  auto *clear = b.create_range_for(b.get_int32(0), b.get_int32(12), false,
                                   runtime.cpu_threads());
  {
    auto guard = b.get_loop_guard(clear);
    b.create_global_store(b.create_external_ptr(out, {b.get_loop_index(clear)}),
                          b.get_int32(0));
  }
  auto *loop = b.create_struct_for(l.parts, false, runtime.cpu_threads());
  {
    auto guard = b.get_loop_guard(loop);
    auto *i = b.get_loop_index(loop, 0);
    auto *j = b.get_loop_index(loop, 1);
    auto *k = b.get_loop_index(loop, 2);
    auto *flat = b.create_add(b.create_mul(i, b.get_int32(4)),
                              b.create_add(b.create_mul(j, b.get_int32(2)), k));
    b.create_global_store(b.create_external_ptr(out, {flat}),
                          b.create_global_load(b.create_global_ptr(l.value, {i, j, k})));
  }
  auto *metadata = b.create_range_for(b.get_int32(0), b.get_int32(2), false,
                                      runtime.cpu_threads());
  {
    auto guard = b.get_loop_guard(metadata);
    auto *i = b.get_loop_index(metadata);
    b.create_global_store(b.create_external_ptr(out, {b.create_add(i, b.get_int32(8))}),
                          b.create_global_load(b.create_global_ptr(l.revision, {i})));
    b.create_global_store(b.create_external_ptr(out, {b.create_add(i, b.get_int32(10))}),
                          b.create_global_load(b.create_global_ptr(l.attachment, {i})));
  }
  return compile(runtime, b, "instance_read", {Param::array(PrimitiveType::i32)}, emit);
}

CompiledKernel release(engine::Runtime &runtime, const Layout &l, bool emit) {
  IRBuilder b;
  auto *i = b.create_arg_load({0}, PrimitiveType::i32, false, 0);
  auto *ptr = b.create_global_ptr(l.instances, {i});
  ptr->is_cell_access = true;
  b.insert(Stmt::make_typed<SNodeOpStmt>(SNodeOpType::deactivate, l.instances, ptr));
  return compile(runtime, b, "instance_release", {Param::scalar(PrimitiveType::i32)}, emit);
}

int run(int argc, char **argv) {
  int threads = 1;
  bool emit = false;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--threads" && i + 1 < argc) threads = std::stoi(argv[++i]);
    else if (arg == "--emit-ir") emit = true;
    else throw std::runtime_error("expected --threads 1|2 [--emit-ir]");
  }
  require(threads == 1 || threads == 2, "expected one or two workers");
  engine::InstanceSettings settings;
  settings.read_environment = false;
  settings.cpu_threads = threads;
  // Opt-in artifacts use the fork's existing compiler writers in the current
  // directory. Run --emit-ir from a fresh evidence directory, outside source.
  struct RestoreConfig {
    CompileConfig saved{default_compile_config};
    ~RestoreConfig() { default_compile_config = saved; }
  } restore;
  default_compile_config.print_struct_llvm_ir = emit;
  default_compile_config.print_kernel_llvm_ir = emit;
  default_compile_config.print_kernel_llvm_ir_optimized = emit;
  engine::Runtime runtime(settings);
  auto &program = runtime.program();
  auto root = std::make_unique<SNode>(0, SNodeType::root);
  const auto l = nested_layout(*root);
  auto *tree = program.add_snode_tree(std::move(root), false);
  const int layout_ids = runtime.issued_snode_ids();
  auto write = writer(runtime, l, emit);
  auto attach = attachment(runtime, l, emit);
  auto read = reader(runtime, l, emit);
  auto retire = release(runtime, l, emit);
  Ndarray output(&program, PrimitiveType::i32, {12});
  prepared_test::Metrics metrics(runtime, {l.instances, l.parts});
  const auto original_backing = metrics.impl.snode_root_backing_record(tree->id());
  CHECK_TRUE(original_backing.status == AllocationRecordStatus::Live, "original root backing is live");
  const auto change = [&](int i, int base, int revision) {
    auto ctx = write.make_context();
    ctx.set_arg_int({0}, i);
    ctx.set_arg_int({1}, base);
    ctx.set_arg_int({2}, revision);
    write.launch(ctx);
  };
  const auto observe = [&] {
    auto ctx = read.make_context();
    ctx.set_arg_ndarray({0}, output);
    read.launch(ctx);
    runtime.synchronize();
    std::array<int, 12> host{};
    engine::readback(output, host.data(), sizeof(host));
    return host;
  };
  const auto set_attachment = [&](int i, int value) {
    auto ctx = attach.make_context();
    ctx.set_arg_int({0}, i);
    ctx.set_arg_int({1}, value);
    attach.launch(ctx);
  };
  const auto check_parts = [&](const std::array<int, 12> &v, int i, int base) {
    for (int p = 0; p < 4; ++p)
      CHECK_EQ_INT(v[i * 4 + p], base + p,
                   "instance " + std::to_string(i) + " nested part " + std::to_string(p));
  };
  change(0, 10, 1);
  change(1, 10, 1);
  set_attachment(0, 0);  // Exercise this kernel before reuse measurements.
  auto values = observe();
  check_parts(values, 0, 10);
  check_parts(values, 1, 10);
  CHECK_EQ_INT(values[8], 1, "first revision pin");
  CHECK_EQ_INT(values[9], 1, "second revision pin");
  const auto initial = metrics.sample();
  initial.print("two-instances");
  CHECK_EQ_INT(runtime.issued_snode_ids(), layout_ids, "instances did not clone SNodes");
  CHECK_EQ_INT(initial.elements, 2, "two pointer cells allocated by native NodeManager");

  // Explicit value-compatible adoption for only instance zero. This does not
  // claim that the production structural recipe already supports live adoption.
  change(0, 20, 2);
  values = observe();
  check_parts(values, 0, 20);
  check_parts(values, 1, 10);
  CHECK_EQ_INT(values[8], 2, "selected instance adopted revision");
  CHECK_EQ_INT(values[9], 1, "other instance retains revision");
  CHECK_TRUE(initial.same_backing(metrics.sample()), "value update reused layout, compilation and sampled backing");

  set_attachment(0, 7);
  values = observe();
  check_parts(values, 0, 20);
  check_parts(values, 1, 10);
  CHECK_EQ_INT(values[10], 7, "context attached only to first instance");
  CHECK_EQ_INT(values[11], 0, "second instance has no attachment");
  CHECK_TRUE(initial.same_backing(metrics.sample()), "attachment reused existing typed storage and kernels");

  // A detached integer rollup is an observation, not an additional participant.
  // Both part arrays remain nested and retain the same storage coordinates.
  CHECK_EQ_INT(values[0] + values[1] + values[2] + values[3], 86, "first aggregate observation");
  CHECK_EQ_INT(values[4] + values[5] + values[6] + values[7], 46, "second aggregate observation");
  CHECK_TRUE(values == observe(), "aggregate observation preserved parts and identity coordinates");

  // Even materializing an unrelated layout leaves these existing kernels usable.
  // This is stronger storage disruption than an unused catalogue data addition;
  // it deliberately does compile/allocate its own tree and is measured separately.
  auto other_root = std::make_unique<SNode>(0, SNodeType::root);
  place(other_root->dense(Axis(0), 2));
  auto *other_tree = program.add_snode_tree(std::move(other_root), false);
  const auto other_backing = metrics.impl.snode_root_backing_record(other_tree->id());
  CHECK_TRUE(other_backing.status == AllocationRecordStatus::Live &&
                 !(original_backing.token == other_backing.token),
             "unrelated tree owns a distinct native root allocation record");
  const double compilation_after_add = program.get_total_compilation_time();
  CHECK_TRUE(values == observe(), "unrelated compiled tree preserves original instances");
  CHECK_TRUE(compilation_after_add == program.get_total_compilation_time(), "old reader launch did not recompile after tree addition");
  program.destroy_snode_tree(other_tree);
  auto *device = metrics.impl.get_runtime_executor()->get_compute_device();
  CHECK_TRUE(device->allocation_record(other_backing.token).status == AllocationRecordStatus::Retired,
             "destroy retires unrelated root record");
  CHECK_TRUE(device->allocation_record(original_backing.token).status == AllocationRecordStatus::Live,
             "destroy preserves original root record");
  CHECK_TRUE(values == observe(), "unrelated tree retirement preserves original kernels and state");

  auto ctx = retire.make_context();
  ctx.set_arg_int({0}, 0);
  retire.launch(ctx);
  values = observe();
  for (int p = 0; p < 4; ++p) CHECK_EQ_INT(values[p], 0, "retired instance absent from traversal");
  CHECK_EQ_INT(values[8], 0, "retired revision unavailable through inactive cell");
  CHECK_EQ_INT(values[10], 0, "retired attachment unavailable through inactive cell");
  check_parts(values, 1, 10);
  const auto retired = metrics.sample();
  retired.print("retired-one");
  CHECK_TRUE(device->allocation_record(original_backing.token).status == AllocationRecordStatus::Live,
             "instance retirement retains tree root backing");
  CHECK_EQ_INT(retired.elements, 2, "cell retirement retains native data-list capacity");
  change(0, 30, 3);
  values = observe();
  check_parts(values, 0, 30);
  check_parts(values, 1, 10);
  CHECK_EQ_INT(values[8], 3, "same coordinate now holds explicit new revision");
  CHECK_EQ_INT(values[10], 0, "recycled storage does not inherit attachment");
  const auto reused = metrics.sample();
  reused.print("reused-one");
  CHECK_EQ_INT(reused.elements, 2, "native free list reused cell without growing data list");
  CHECK_TRUE(retired.same_backing(reused), "reactivation reused sampled backing and compiled kernels");
  return harness::report("native_instance_inspection_test");
}
}  // namespace

int main(int argc, char **argv) {
  try { return run(argc, argv); }
  catch (const std::exception &e) {
    std::cerr << "native_instance_inspection_test: " << e.what() << '\n';
    return 1;
  }
}
