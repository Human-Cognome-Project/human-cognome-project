#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include "allocation_observation.h"
#include "engine/runtime.h"
#include "engine/transfer.h"
#include "engine/record_map.h"
#include "taichi/runtime/llvm/kernel_launcher.h"
#include "taichi/runtime/program_impls/llvm/llvm_program.h"

namespace {
using namespace taichi::lang;
void require(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
template <class F>
void rejects(F action) {
  try {
    action();
  } catch (const std::exception &) {
    return;
  }
  throw std::runtime_error("invalid prepared operation accepted");
}
engine::CompiledKernel kernel(engine::Runtime &r,
                              const std::string &name,
                              DataType output = PrimitiveType::f32,
                              DataType integer = PrimitiveType::i32) {
  IRBuilder b;
  auto *a = b.create_ndarray_arg_load({0}, output, 1, 0);
  auto *n = b.create_arg_load({1}, integer, false, 0);
  auto *factor = b.create_arg_load({2}, PrimitiveType::f32, false, 0);
  b.create_global_store(
      b.create_external_ptr(a, {b.get_int32(0)}),
      b.create_mul(b.create_cast(n, output), b.create_cast(factor, output)));
  return engine::CompiledKernel(
      r, name, b.extract_ir(),
      {engine::Param::array(output), engine::Param::scalar(integer),
       engine::Param::scalar(PrimitiveType::f32)});
}
// Host ownership/failure policy, without provoking a hardware fault. This is
// not GPU fault-recovery evidence.
class FailingLauncher final : public taichi::lang::LLVM::KernelLauncher {
 public:
  bool fail_setup{false};
  int destroyed{0}, registered{0}, acquired{0};
  explicit FailingLauncher(LlvmRuntimeExecutor *e) : LLVM::KernelLauncher({e}) {
  }
  struct Resource final : LaunchContextBuilder::Prepared {
    int *destroyed;
    LaunchContextBuilder *observe{};
    bool *ordered{};
    explicit Resource(int *p) : destroyed(p) {
    }
    ~Resource() override {
      if (observe)
        *ordered = observe->get_arg<int>({1}) == 17;
      ++*destroyed;
    }
  };
  Handle register_llvm_kernel(const LLVM::CompiledKernelData &) override {
    ++registered;
    Handle h;
    h.set_launch_id(0);
    return h;
  }
  std::unique_ptr<LaunchContextBuilder::Prepared> prepare_llvm_kernel(
      Handle,
      LaunchContextBuilder &ctx) override {
    ++acquired;
    auto resource = std::make_unique<Resource>(&destroyed);
    ctx.prepare_scalars(*resource);
    if (fail_setup)
      throw std::runtime_error("injected setup failure");
    return resource;
  }
  void launch_llvm_kernel(Handle, LaunchContextBuilder &) override {
    throw std::runtime_error("injected dispatch failure");
  }
};

template <class F>
void rejects_unchanged(LaunchContextBuilder &ctx, F action) {
  const auto *buffer = ctx.get_context().arg_buffer;
  const std::vector<char> arguments(buffer, buffer + ctx.arg_buffer_size);
  const auto pointers = ctx.array_ptrs;
  rejects(action);
  require(!ctx.prepared && ctx.get_context().arg_buffer == buffer &&
              std::memcmp(buffer, arguments.data(), arguments.size()) == 0 &&
              ctx.array_ptrs == pointers,
          "rejected preparation changed the original context");
}

std::unique_ptr<Kernel> native_kernel(engine::Runtime &runtime,
                                      const std::string &name,
                                      DataType output = PrimitiveType::f32,
                                      DataType scalar = PrimitiveType::i32) {
  IRBuilder body;
  auto result =
      std::make_unique<Kernel>(runtime.program(), body.extract_ir(), name);
  result->insert_ndarray_param(output, 1);
  result->insert_scalar_param(scalar);
  result->finalize_params();
  result->finalize_rets();
  return result;
}

void originating_contexts(engine::Runtime &runtime) {
  auto first = kernel(runtime, "prepared_origin_first");
  auto same_abi = kernel(runtime, "prepared_origin_same_abi");
  auto array_abi =
      kernel(runtime, "prepared_origin_array_abi", PrimitiveType::f64);
  auto scalar_abi = kernel(runtime, "prepared_origin_scalar_abi",
                           PrimitiveType::f32, PrimitiveType::i64);
  Ndarray a(&runtime.program(), PrimitiveType::f32, {1});
  auto ctx = first.make_context();
  ctx.set_arg_ndarray({0}, a);
  ctx.set_arg_int({1}, 3);
  ctx.set_arg_float({2}, 2);
  rejects_unchanged(ctx, [&] { same_abi.prepare(ctx); });
  rejects_unchanged(ctx, [&] { array_abi.prepare(ctx); });
  rejects_unchanged(ctx, [&] { scalar_abi.prepare(ctx); });
  first.prepare(ctx);
  first.launch(ctx);
  runtime.synchronize();
  float value = 0;
  engine::readback(a, &value, sizeof(value));
  require(value == 6,
          "rejected wrapper preparation poisoned rightful execution");

  auto &program = runtime.program();
  auto *impl = static_cast<LlvmProgramImpl *>(program.get_program_impl());
  auto &launcher =
      dynamic_cast<LLVM::KernelLauncher &>(impl->get_kernel_launcher());
  const auto compile =
      [&](Kernel &k) -> const taichi::lang::CompiledKernelData & {
    return program.compile_kernel(program.compile_config(),
                                  program.get_device_caps(), k);
  };
  const auto bind = [&](Kernel &k) {
    auto context = k.make_launch_context();
    context.set_arg_ndarray({0}, a);
    context.set_arg_int({1}, 3);
    return context;
  };
  auto source = native_kernel(runtime, "prepared_origin_native");
  auto other = native_kernel(runtime, "prepared_origin_native_other");
  const auto &rightful = compile(*source), &wrong = compile(*other);
  FailingLauncher probe(impl->get_runtime_executor());
  auto pending = bind(*source);
  rejects_unchanged(pending, [&] { probe.prepare_kernel(wrong, pending); });
  require(probe.registered == 0 && probe.acquired == 0,
          "native origin rejection occurred after registration/acquisition");

  // The fork permits one Program. A second real native executor exercises a
  // foreign-runtime boundary without creating an unsupported second Program.
  auto foreign_config = program.compile_config();
  foreign_config.arch = taichi::host_arch();
  foreign_config.kernel_profiler = false;
  LlvmRuntimeExecutor foreign_executor(foreign_config, nullptr);
  FailingLauncher foreign(&foreign_executor);
  rejects_unchanged(pending,
                    [&] { foreign.prepare_kernel(rightful, pending); });
  require(foreign.registered == 0 && foreign.acquired == 0,
          "foreign runtime acquired launch resources");
  launcher.prepare_kernel(rightful, pending);
  program.launch_kernel(rightful, pending);
  runtime.synchronize();

  // Force a cache-key alias: provenance by key/pointer alone can now pass, but
  // the actual compiled ndarray/scalar ABI must still reject it. No mismatched
  // kernel is dispatched. Restore the source's key and reuse the same context.
  for (int variant = 0; variant < 2; ++variant) {
    const auto suffix = std::to_string(variant);
    auto target =
        native_kernel(runtime, "prepared_origin_target_" + suffix,
                      variant == 0 ? PrimitiveType::f64 : PrimitiveType::f32,
                      variant == 1 ? PrimitiveType::i64 : PrimitiveType::i32);
    auto original =
        native_kernel(runtime, "prepared_origin_original_" + suffix);
    const auto &target_data = compile(*target);
    original->set_kernel_key_for_cache(target->get_cached_kernel_key());
    require(&compile(*original) == &target_data,
            "cache-alias fixture did not alias");
    auto aliased = bind(*original);
    rejects_unchanged(aliased,
                      [&] { probe.prepare_kernel(target_data, aliased); });
    require(probe.registered == 0 && probe.acquired == 0,
            "target ABI rejection occurred after registration/acquisition");
    original->set_kernel_key_for_cache(original->get_name());
    const auto &correct = compile(*original);
    launcher.prepare_kernel(correct, aliased);
    program.launch_kernel(correct, aliased);
    runtime.synchronize();
  }
  std::cout << "prepared origin: wrapper/native kernel, ndarray/scalar ABI, "
               "foreign runtime, unchanged rejection and rightful reuse PASS\n";
}

void batches(engine::Runtime &runtime) {
  using engine::RecordStatus;
  engine::PreparedRecordMap map(runtime, 70, 7, 4);
  std::array<int, 4> keys{69, 1, 34, 8}, kinds{255, 2, 3, 4};
  std::array<engine::RecordResult, 4> results{};
  std::array<engine::RecordHandle, 7> handles{};
  require(map.define_batch(keys.data(), kinds.data(), 4, results.data()) ==
              RecordStatus::Ok,
          "batch definition failed");
  for (int i = 0; i < 4; ++i)
    handles[i] = results[i].handle;
  const auto generation = map.generation();
  keys = {2, 69, 5, 6};
  require(map.define_batch(keys.data(), kinds.data(), 2, results.data()) ==
                  RecordStatus::AlreadyDefined &&
              map.lookup(2).status == RecordStatus::NotFound &&
              map.generation() == generation && map.size() == 4,
          "native batch validation partially committed");
  keys = {2, 2, 5, 6};
  require(map.define_batch(keys.data(), kinds.data(), 2, results.data()) ==
              RecordStatus::AlreadyDefined,
          "batch duplicate accepted");
  auto stale = handles;
  ++stale[1].incarnation;
  require(map.release_batch(stale.data(), 2, results.data()) ==
                  RecordStatus::Stale &&
              map.inspect(handles[0]).status == RecordStatus::Ok &&
              map.generation() == generation,
          "stale batch retired valid peer");
  auto foreign = handles;
  ++foreign[0].context;
  require(map.release_batch(foreign.data(), 2, results.data()) ==
              RecordStatus::WrongContext,
          "foreign batch accepted");
  keys = {2, 5, 6, 9};
  require(map.define_batch(keys.data(), kinds.data(), 3, results.data()) ==
              RecordStatus::Ok,
          "remaining slots unavailable");
  for (int i = 0; i < 3; ++i)
    handles[i + 4] = results[i].handle;
  for (const auto &h : handles)
    require(h.record >= 0 && h.record < 7, "padded batch row admitted");
  require(map.define_batch(keys.data(), kinds.data(), 1, results.data()) ==
              RecordStatus::CapacityExhausted,
          "full batch bank grew");
  require(map.release_batch(handles.data(), 5, results.data()) ==
              RecordStatus::CapacityExhausted,
          "batch budget ignored");
  require(map.release_batch(handles.data(), 4, results.data()) ==
                  RecordStatus::Ok &&
              map.release_batch(handles.data() + 4, 3, results.data()) ==
                  RecordStatus::Ok,
          "batch retirement failed");
  allocation_observation::calls = 0;
  allocation_observation::bytes = 0;
  allocation_observation::enabled = true;
  for (int cycle = 0; cycle < 32; ++cycle) {
    require(map.define_batch(keys.data(), kinds.data(), 4, results.data()) ==
                RecordStatus::Ok,
            "batch reuse failed");
    for (int i = 0; i < 4; ++i)
      handles[i] = results[i].handle;
    require(map.release_batch(handles.data(), 4, results.data()) ==
                RecordStatus::Ok,
            "batch recycle failed");
  }
  allocation_observation::enabled = false;
  require(allocation_observation::calls == 0 && map.size() == 0,
          "batch path allocated C++ control metadata");
  const auto empty_generation = map.generation();
  require(map.define_batch(nullptr, nullptr, 0, nullptr) == RecordStatus::Ok &&
              map.release_batch(nullptr, 0, nullptr) == RecordStatus::Ok &&
              map.generation() == empty_generation,
          "empty batch committed generation");
}
}  // namespace
int main(int argc, char **argv) {
  try {
    const bool cuda = argc == 2 && std::string(argv[1]) == "--cuda";
    engine::InstanceSettings settings;
    settings.read_environment = false;
    settings.cuda = cuda;
    settings.cpu_threads = 2;
    if (cuda)
      settings.device_memory_gb = 0.5;
    engine::Runtime runtime(settings);
    require(!cuda || runtime.on_cuda(),
            "requested CUDA, actual backend is CPU");
    originating_contexts(runtime);
    auto first = kernel(runtime, "prepared_launch_first"),
         other = kernel(runtime, "prepared_launch_other");
    Ndarray a(&runtime.program(), PrimitiveType::f32, {1});
    auto ctx = first.make_context();
    ctx.set_arg_ndarray({0}, a);
    ctx.set_arg_int({1}, 0);
    ctx.set_arg_float({2}, 0);
    first.prepare(ctx);
    first.launch(ctx);
    runtime.synchronize();  // Prepare the actual backend dispatch before
                            // observing reuse.
    float value = -1;
    allocation_observation::calls = 0;
    allocation_observation::bytes = 0;
    allocation_observation::enabled = true;
    for (int i = 0; i < 64; ++i) {
      ctx.set_prepared_int(1, i);
      ctx.set_prepared_float(2, 0.125);
      first.launch(ctx);
      runtime.synchronize();
      engine::readback(a, &value, sizeof(value));
      require(value == float(i) * 0.125f, "prepared scalar/result incorrect");
    }
    allocation_observation::enabled = false;
    require(allocation_observation::calls == 0,
            "prepared native dispatch/scalar update allocated C++ metadata");
    rejects([&] { ctx.set_arg_ndarray({0}, a); });
    rejects([&] { first.prepare(ctx); });
    rejects([&] { ctx.set_prepared_float(1, 1); });
    rejects([&] { ctx.set_prepared_int(0, 1); });
    rejects([&] { other.launch(ctx); });
    require(ctx.prepared->usable, "wrong kernel poisoned correct binding");
    auto ordinary = first.make_context();
    ordinary.set_arg_ndarray({0}, a);
    ordinary.set_arg_int({1}, 3);
    ordinary.set_arg_float({2}, 2);
    first.launch(ordinary);
    runtime.synchronize();
    engine::readback(a, &value, sizeof(value));
    require(value == 6, "ordinary launch regressed");
    IRBuilder empty;
    engine::CompiledKernel unsupported(
        runtime, "prepared_launch_unsupported", empty.extract_ir(),
        {engine::Param::scalar(PrimitiveType::u8)});
    auto invalid = unsupported.make_context();
    invalid.set_arg_int({0}, 1);
    rejects([&] { unsupported.prepare(invalid); });
    require(!invalid.prepared, "failed setup published prepared binding");
    unsupported.launch(invalid);
    runtime.synchronize();  // Ordinary unsupported signature remains supported.
    Ndarray wrong_type(&runtime.program(), PrimitiveType::i32, {1});
    auto bad_type = first.make_context();
    bad_type.set_arg_ndarray({0}, wrong_type);
    bad_type.set_arg_int({1}, 0);
    bad_type.set_arg_float({2}, 0);
    rejects([&] { first.prepare(bad_type); });
    require(!bad_type.prepared, "wrong array type prepared");
    IRBuilder matrix_body;
    engine::CompiledKernel matrix(
        runtime, "prepared_launch_rank", matrix_body.extract_ir(),
        {engine::Param::array(PrimitiveType::f32, 2)});
    auto bad_rank = matrix.make_context();
    bad_rank.set_arg_ndarray({0}, a);
    rejects([&] { matrix.prepare(bad_rank); });
    require(!bad_rank.prepared, "wrong array rank prepared");
    IRBuilder body;
    Kernel raw(runtime.program(), body.extract_ir(),
               "prepared_launch_failure_policy");
    raw.finalize_params();
    raw.finalize_rets();
    const auto &compiled = runtime.program().compile_kernel(
        runtime.program().compile_config(), runtime.program().get_device_caps(),
        raw);
    auto *impl =
        static_cast<LlvmProgramImpl *>(runtime.program().get_program_impl());
    FailingLauncher failing(impl->get_runtime_executor());
    {
      auto failure = raw.make_launch_context();
      failing.fail_setup = true;
      rejects([&] { failing.prepare_kernel(compiled, failure); });
      require(!failure.prepared && failing.destroyed == 1,
              "partial setup resource leaked");
      failing.fail_setup = false;
      failing.prepare_kernel(compiled, failure);
      rejects([&] { failing.launch_kernel(compiled, failure); });
      require(!failure.prepared->usable,
              "dispatch failure retained ready binding");
      rejects([&] { failing.launch_kernel(compiled, failure); });
    }
    require(failing.destroyed == 2,
            "failed dispatch resource not released on destruction");
    {
      auto displaced = first.make_context();
      displaced.set_arg_int({1}, 17);
      auto replacement = first.make_context();
      replacement.set_arg_int({1}, 29);
      bool ordered = false;
      auto resource =
          std::make_unique<FailingLauncher::Resource>(&failing.destroyed);
      resource->observe = &displaced;
      resource->ordered = &ordered;
      displaced.prepared = std::move(resource);
      displaced = std::move(replacement);
      require(ordered && failing.destroyed == 3 &&
                  displaced.get_arg<int>({1}) == 29,
              "move assignment released borrowed buffers before completion");
    }
    batches(runtime);
    std::cout << "prepared_launch: scalars, ordinary path, signature "
                 "rejection, ownership/failure cleanup PASS; new=0/0\n";
  } catch (const std::exception &e) {
    allocation_observation::enabled = false;
    std::cerr << e.what() << '\n';
    return 1;
  }
}
