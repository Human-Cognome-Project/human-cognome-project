#include "engine/runtime.h"
#include "engine/transfer.h"
#include "compiler_lifetime_trace.h"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// Existing test friend: inspect actual HCP borrowers without changing ownership.
namespace engine::testing {
struct CompiledKernelAccess {
  static const taichi::lang::Kernel &definition(const CompiledKernel &k) {
    return *k.kernel_;
  }
  static const taichi::lang::CompiledKernelData &data(const CompiledKernel &k) {
    return *k.compiled_;
  }
};
}  // namespace engine::testing

namespace {
using namespace taichi::lang;
using Access = engine::testing::CompiledKernelAccess;
using compiler_trace::check;
struct Refused {};
template <class F> void refuses(F action) {
  try { action(); } catch (const Refused &) { return; }
  throw std::runtime_error("inspection candidate did not refuse");
}
std::unique_ptr<engine::CompiledKernel> kernel(engine::Runtime &runtime,
                                               const std::string &key) {
  IRBuilder body;
  auto *array = body.create_ndarray_arg_load({0}, PrimitiveType::i32, 1, 0);
  auto *value = body.create_arg_load({1}, PrimitiveType::i32, false, 0);
  body.create_global_store(body.create_external_ptr(array, {body.get_int32(0)}), value);
  auto result = std::make_unique<engine::CompiledKernel>(runtime, key, body.extract_ir(),
      std::vector<engine::Param>{engine::Param::array(PrimitiveType::i32),
                                 engine::Param::scalar(PrimitiveType::i32)});
  const auto &definition = Access::definition(*result);
  compiler_trace::capture(&definition, compiler_trace::Kind::Definition);
  compiler_trace::compiled(runtime.program(), definition, Access::data(*result));
  return result;
}
int handle(const engine::CompiledKernel &kernel) {
  const auto &value = Access::data(kernel).get_handle();
  check(bool(value), "expected registered native data");
  return value->get_launch_id();
}
void launch(engine::Runtime &runtime, engine::CompiledKernel &kernel, Ndarray &array,
            int value, bool prepared) {
  auto invocation = kernel.invocation(prepared);
  auto &ctx = invocation.get();
  if (!ctx.prepared) ctx.set_arg_ndarray({0}, array);
  engine::argument_int(ctx, 1, value);
  kernel.launch(ctx);
  runtime.synchronize();
  int actual = 0;
  engine::readback(array, &actual, sizeof(actual));
  check(actual == value, "surviving native task dispatched the wrong binding/value");
}
void unchanged(Ndarray &array, int expected) {
  int actual = 0;
  engine::readback(array, &actual, sizeof(actual));
  check(actual == expected, "refused candidate mutated a surviving output");
}
JITModule *latest_module(const compiler_trace::Trace &trace) {
  for (auto it = trace.objects.rbegin(); it != trace.objects.rend(); ++it)
    if (it->kind == compiler_trace::Kind::Module && it->live)
      return const_cast<JITModule *>(static_cast<const JITModule *>(it->address));
  throw std::runtime_error("registered kernel module not captured");
}
std::vector<void *> task_addresses(JITModule &module, const CompiledKernelData &data) {
  std::vector<void *> result;
  const auto &compiled = dynamic_cast<const LLVM::CompiledKernelData &>(data);
  for (const auto &task : compiled.get_internal_data().compiled_data.tasks) {
    auto *address = module.lookup_function(task.name);
    check(address, "live native task symbol missing");
    result.push_back(address);
  }
  check(!result.empty(), "inspection kernel has no native task");
  return result;
}
compiler_trace::Sample sample(engine::Runtime &runtime, const char *phase,
                              std::size_t data, std::size_t rows) {
  const auto observed = compiler_trace::sample(runtime.program(), phase);
  check(observed.data_live == data && observed.registered == rows &&
            observed.launch_contexts == rows, "sharing/registration boundary changed");
  return observed;
}
void run(int workers, bool prepared) {
  compiler_trace::Trace trace;
  compiler_trace::Scope observe(trace);
  {
    engine::InstanceSettings settings;
    settings.read_environment = false; settings.cpu_threads = workers;
    engine::Runtime runtime(settings);
    check(runtime.arch() == taichi::Arch::x64, "reclamation inspection requires CPU");
    Ndarray first(&runtime.program(), PrimitiveType::i32, {1});
    Ndarray second(&runtime.program(), PrimitiveType::i32, {1});
    Ndarray candidate_output(&runtime.program(), PrimitiveType::i32, {1});
    sample(runtime, "empty", 0, 0);
    {
      auto warm = kernel(runtime, "reclamation_shared");
      launch(runtime, *warm, first, 7, prepared);
      auto *warm_module = latest_module(trace);
      const auto warm_tasks = task_addresses(*warm_module, Access::data(*warm));
      const int warm_id = handle(*warm);
      auto sibling = kernel(runtime, "reclamation_sibling");
      launch(runtime, *sibling, second, 11, prepared);
      auto *sibling_module = latest_module(trace);
      const auto sibling_tasks = task_addresses(*sibling_module, Access::data(*sibling));
      const int sibling_id = handle(*sibling);
      check(warm_id == 0 && sibling_id == 1, "initial CPU rows are not append-only");
      sample(runtime, "two_survivors", 2, 2);
      refuses([&] {
        auto candidate = kernel(runtime, "reclamation_shared");
        check(&Access::definition(*candidate) != &Access::definition(*warm) &&
                  &Access::data(*candidate) == &Access::data(*warm),
              "identical native cache key did not share actual compiled data");
        auto invocation = candidate->invocation(true);
        auto &ctx = invocation.get();
        ctx.set_arg_ndarray({0}, candidate_output); ctx.set_arg_int({1}, 99);
        candidate->prepare(ctx);
        check(handle(*candidate) == warm_id && ctx.prepared->launch_id == warm_id,
              "shared compiled data acquired a different launch row");
        sample(runtime, "shared_candidate_prepared", 2, 2);
        throw Refused{};  // Test-only refusal after preparation, before dispatch.
      });
      unchanged(first, 7); unchanged(second, 11);
      sample(runtime, "shared_candidate_gone", 2, 2);

      const CompiledKernelData *unregistered = nullptr;
      refuses([&] {
        auto candidate = kernel(runtime, "reclamation_retry");
        unregistered = &Access::data(*candidate);
        check(!unregistered->get_handle(), "unlaunched candidate unexpectedly registered");
        sample(runtime, "unregistered_candidate", 3, 2);
        throw Refused{};  // Test-only refusal after compile, before preparation.
      });
      sample(runtime, "unregistered_candidate_gone", 3, 2);
      {
        auto retry = kernel(runtime, "reclamation_retry");
        check(&Access::data(*retry) == unregistered && !unregistered->get_handle(),
              "retry did not borrow the retained unregistered cache object");
        launch(runtime, *retry, candidate_output, 17, prepared);
        check(handle(*retry) == 2, "first registration did not append its own row");
        sample(runtime, "retry_registered", 3, 3);
      }
      sample(runtime, "registered_definition_gone", 3, 3);
      {
        auto revisit = kernel(runtime, "reclamation_retry");
        check(&Access::data(*revisit) == unregistered && handle(*revisit) == 2,
              "cached retry lost its retained native handle");
        launch(runtime, *revisit, candidate_output, 19, prepared);
        sample(runtime, "same_record_reuses_handle", 3, 3);
      }
      {
        auto different = kernel(runtime, "reclamation_distinct");
        launch(runtime, *different, candidate_output, 23, prepared);
        check(handle(*different) == 3, "distinct record reused a retired definition's row");
        sample(runtime, "distinct_record_appends", 4, 4);
      }
      check(handle(*warm) == warm_id && handle(*sibling) == sibling_id &&
                task_addresses(*warm_module, Access::data(*warm)) == warm_tasks &&
                task_addresses(*sibling_module, Access::data(*sibling)) == sibling_tasks,
            "candidate lifecycle changed surviving handles/task symbols");
      launch(runtime, *warm, first, 29, prepared);
      launch(runtime, *sibling, second, 31, prepared);
      sample(runtime, "surviving_dispatch", 4, 4);
    }
    const auto final = sample(runtime, "all_definitions_gone", 4, 4);
    check(final.definitions == 7 && !final.definitions_live && final.data == 4 &&
              trace.compile_calls == 7 && final.modules == 4,
          "final sharing fixture totals changed");
  }
  compiler_trace::finish(trace);
  std::cout << "native_reclamation_inspection,workers=" << workers
            << ",path=" << (prepared ? "prepared" : "dynamic")
            << ",definitions=7,compiled_records=4,launch_rows=4,runtime_modules=1"
               ",shared_data_and_handle=1,unregistered_retry=1,same_record_handle_reuse=1"
               ",distinct_record_appends=1,surviving_task_addresses=1,sibling_dispatch=1"
               ",eviction_executed=0 PASS\n";
}
}  // namespace
int main(int argc, char **argv) {
  try {
    if (argc != 3) throw std::invalid_argument("expected workers and prepared|dynamic");
    const int workers = std::stoi(argv[1]);
    const std::string path(argv[2]);
    if ((workers != 1 && workers != 2) || (path != "prepared" && path != "dynamic"))
      throw std::invalid_argument("unsupported inspection mode");
    run(workers, path == "prepared"); return 0;
  } catch (const std::exception &e) {
    std::cerr << "native reclamation inspection: " << e.what() << '\n'; return 1;
  }
}
