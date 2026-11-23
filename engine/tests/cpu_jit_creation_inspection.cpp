#define HCP_ALLOCATION_LIFETIME
#include "allocation_observation.h"
#include "engine/runtime.h"
#include "engine/transfer.h"
#include "taichi/codegen/llvm/compiled_kernel_data.h"
#include "taichi/runtime/program_impls/llvm/llvm_program.h"
#include "llvm/ExecutionEngine/Orc/Core.h"

#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace engine::testing {
struct CompiledKernelAccess {
  static const taichi::lang::CompiledKernelData &data(const CompiledKernel &k) { return k.compiled_data(); }
};
}  // namespace engine::testing

namespace {
using namespace taichi::lang;
using namespace llvm::orc;
using Access = engine::testing::CompiledKernelAccess;
constexpr auto unlimited = std::numeric_limits<std::size_t>::max();
constexpr int cells = 64;
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
enum class Window { Complete, Generator, Returned };
const char *name(Window w) {
  switch (w) {
    case Window::Complete: return "complete";
    case Window::Generator: return "before_generator";
    case Window::Returned: return "after_wrapper_return";
  }
  return "invalid";
}
struct GuardedRetry {};
struct ReturnedModuleFault {};
struct Lib {
  JITDylib *dylib{};
  ResourceTracker *tracker{};
  std::string name;
  void *block{};
  std::size_t bytes{};
  bool ir{}, released{};
};
struct Trace {
  Window window{};
  JITSession *session{};
  ExecutionSession *execution{};
  JITModule *latest_module{};
  std::array<Lib, 8> libs{};
  std::size_t lib_count{}, pool_calls{};
  bool injecting{}, guarded_retry{}, retry_intercepted{};
  bool probing{}, pool_mismatch{}, finalizing{}, finalized{}, ended{};
  const void *pool{};
  std::array<JITDylib *, 8> search{};
  std::size_t search_count{};
};
thread_local Trace *active{};
// Fixed observers use public native/ORC boundaries. They neither remove resources
// nor create a second execution authority. Pointers are queried only while live.
void released(void *p, std::size_t) noexcept {
  if (!active) return;
  for (auto &lib : active->libs) if (lib.block == p) lib.released = true;
}
void capture_block(Lib &lib) {
  const auto address = reinterpret_cast<std::uintptr_t>(lib.dylib);
  for (const auto &e : allocation_lifetime::table) if (e.pointer) {
    const auto begin = reinterpret_cast<std::uintptr_t>(e.pointer);
    if (address >= begin && address - begin < e.bytes) { lib.block = e.pointer; lib.bytes = e.bytes; return; }
  }
  throw std::runtime_error("dylib allocation was not observed");
}
std::unique_ptr<engine::CompiledKernel> kernel(engine::Runtime &runtime, const char *key) {
  IRBuilder b;
  auto *array = b.create_ndarray_arg_load({0}, PrimitiveType::i32, 1, 0);
  auto *value = b.create_arg_load({1}, PrimitiveType::i32, false, 0);
  auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(cells), false, runtime.cpu_threads());
  {
    auto guard = b.get_loop_guard(loop); auto *i = b.get_loop_index(loop);
    b.create_global_store(b.create_external_ptr(array, {i}), b.create_add(value, i));
  }
  return std::make_unique<engine::CompiledKernel>(runtime, key, b.extract_ir(),
      std::vector<engine::Param>{engine::Param::array(PrimitiveType::i32), engine::Param::scalar(PrimitiveType::i32)});
}
const LLVM::CompiledKernelData &data(const engine::CompiledKernel &k) {
  return dynamic_cast<const LLVM::CompiledKernelData &>(Access::data(k));
}
void output_is(Ndarray &array, int value, bool written = true) {
  std::array<int, cells> result{}; engine::readback(array, result.data(), sizeof(result));
  for (int i = 0; i < cells; ++i) check(result[i] == (written ? value + i : 0), "dispatch output changed");
}
void launch(engine::Runtime &runtime, engine::CompiledKernel &k, Ndarray &array, int value, bool prepared) {
  auto invocation = k.invocation(prepared); auto &ctx = invocation.get();
  if (!ctx.prepared) ctx.set_arg_ndarray({0}, array);
  engine::argument_int(ctx, 1, value); k.launch(ctx); runtime.synchronize();
  check(bool(ctx.prepared) == prepared, "dispatch mode changed"); output_is(array, value);
}
void fail(engine::Runtime &runtime, engine::CompiledKernel &k, Ndarray &array, bool prepared, bool retry) {
  auto invocation = k.invocation(prepared); auto &ctx = invocation.get();
  ctx.set_arg_ndarray({0}, array); ctx.set_arg_int({1}, 99);
  bool caught = false; active->guarded_retry = retry;
  try { k.launch(ctx); }
  catch (const std::bad_alloc &) { caught = !retry; }
  catch (const GuardedRetry &) { caught = retry; }
  catch (const ReturnedModuleFault &) { caught = !retry && active->window == Window::Returned; }
  allocation_lifetime::fail_at = 0;
  active->injecting = false; active->guarded_retry = false;
  check(caught && !ctx.prepared && !data(k).get_handle(), "failure published a handle or prepared state");
  runtime.synchronize(); output_is(array, 0, false);
}
void libraries_live() {
  for (std::size_t i = 0; i < active->lib_count; ++i) {
    const auto &lib = active->libs[i];
    check(!lib.released && active->execution->getJITDylibByName(lib.name) == lib.dylib,
          "session lost a retained dylib");
  }
}
void run_window(int workers, bool prepared, Window window) {
  Trace trace; trace.window = window; active = &trace;
  {
    engine::InstanceSettings settings; settings.read_environment = false; settings.cpu_threads = workers;
    engine::Runtime runtime(settings); auto &program = runtime.program();
    auto &impl = *program.get_program_impl();
    auto &llvm_impl = dynamic_cast<LlvmProgramImpl &>(impl);
    auto *executor = llvm_impl.get_runtime_executor();
    trace.pool = executor->runtime_bookkeeping().thread_pool.owner;
    check(runtime.arch() == taichi::Arch::x64 && trace.session && trace.execution && trace.pool,
          "native CPU session/service capture failed");
    {
      Ndarray array(&program, PrimitiveType::i32, {cells});
      auto first = kernel(runtime, "jit_first"), sibling = kernel(runtime, "jit_sibling"), third = kernel(runtime, "jit_third");
      launch(runtime, *first, array, 7, prepared); auto *first_module = trace.latest_module;
      launch(runtime, *sibling, array, 11, prepared); auto *sibling_module = trace.latest_module;
      launch(runtime, *third, array, 13, prepared);
      check(trace.lib_count == 4 && trace.pool_calls >= 3 && !trace.pool_mismatch,
            "warm modules did not share the existing thread service");
      const auto task = data(*first).get_internal_data().compiled_data.tasks.front().name;
      const auto sibling_task = data(*sibling).get_internal_data().compiled_data.tasks.front().name;
      auto *first_address = first_module->lookup_function(task);
      auto *sibling_address = sibling_module->lookup_function(sibling_task);
      auto candidate = kernel(runtime, "jit_candidate");
      const auto record = data(*candidate).borrow_observation().record;
      const auto candidate_task = data(*candidate).get_internal_data().compiled_data.tasks.front().name;
      std::array<int, cells> zero{}; engine::upload(array, zero.data(), sizeof(zero));
      const auto calls_before = trace.pool_calls;
      trace.injecting = window != Window::Complete;
      if (trace.injecting) fail(runtime, *candidate, array, prepared, false);
      else launch(runtime, *candidate, array, 17, prepared);
      check(trace.lib_count == 5, "candidate did not create exactly one dylib");
      auto &partial = trace.libs[4];
      const bool complete = window == Window::Complete;
      const bool exposed = window != Window::Generator;
      check(partial.name == "4" && (complete || trace.pool_calls == calls_before), "failure crossed the wrong JIT boundary or dispatched");
      auto flags = trace.execution->lookupFlags(LookupKind::Static,
          {{partial.dylib, JITDylibLookupFlags::MatchAllSymbols}},
          SymbolLookupSet(trace.execution->intern(candidate_task)));
      partial.ir = bool(flags);
      if (!flags) llvm::consumeError(flags.takeError());
      else check(flags->size() == 1, "accepted IR symbol flags were not retained");
      check(partial.ir == exposed, "candidate symbol coverage differs from expected IR acceptance");
      if (partial.ir) {
        partial.tracker = partial.dylib->getDefaultResourceTracker().get();
        check(!partial.tracker->isDefunct(), "IR default tracker became defunct");
      }
      trace.probing = true; auto *global_address = trace.session->lookup(task); trace.probing = false;
      check(global_address == first_address && trace.search_count == (exposed ? 5u : 4u) &&
                trace.search[0] == trace.libs[0].dylib &&
                (trace.search[trace.search_count - 1] == partial.dylib) == exposed,
            "actual native session lookup exposure differs from the publication boundary");
      const auto counts = llvm_impl.jit_ownership_counts(unlimited);
      check(counts.modules == (exposed ? 5u : 4u) && counts.runtime_modules == 1,
            "wrapper count included a partial backend-only module");
      if (!complete) {
        const auto entry = impl.cpu_registration_attempt(3);
        check(entry.status == OwnershipCountStatus::Incomplete && !entry.complete &&
                  entry.failed && entry.stage == CpuRegistrationStage::CreatingModule &&
                  entry.module_status == OwnershipCountStatus::Incomplete && !entry.module &&
                  entry.record.observation().record == record,
              "native attempt hid unreturned JIT side effects");
        if (window == Window::Generator) {
          // Stop before violating LLVM's unique-name precondition. This is
          // a guarded observation, not qualification of a recovered inner retry.
          fail(runtime, *candidate, array, prepared, true);
          check(trace.retry_intercepted && trace.lib_count == 5 &&
                    impl.cpu_registration_attempt(4).failed, "guarded retry changed the native name");
        } else {
          launch(runtime, *candidate, array, 19, prepared);
          check(trace.lib_count == 6 && trace.libs[5].name == "5" &&
                    data(*candidate).get_handle()->get_launch_id() == 4 &&
                    data(*candidate).borrow_observation().record == record &&
                    llvm_impl.jit_ownership_counts(unlimited).modules == 6,
                "returned-wrapper retry changed the record or reused backend identity");
        }
      }
      libraries_live();
      check(first_module->lookup_function(task) == first_address &&
                sibling_module->lookup_function(sibling_task) == sibling_address,
            "partial creation changed a surviving task address");
      launch(runtime, *first, array, 23, prepared); launch(runtime, *sibling, array, 29, prepared);
      launch(runtime, *third, array, 31, prepared);
      check(data(*first).get_handle()->get_launch_id() == 0 &&
                data(*sibling).get_handle()->get_launch_id() == 1 && !trace.pool_mismatch,
            "partial creation changed native survivor handles/services");
      std::cout << "cpu_jit_creation,window=" << name(window) << ",snapshot_dylibs=5,accepted_ir=" << partial.ir
                << ",lookup_exposed=" << exposed << ",stored_wrappers=" << counts.modules
                << ",final_dylibs=" << trace.lib_count
                << ",same_name_guarded_retry=" << (window == Window::Generator)
                << ",successful_outer_retry=" << (window == Window::Returned) << ",default_tracker=" << bool(partial.tracker)
                << ",shared_thread_service=1"
                   ",surviving_handles_tasks_dispatch=1,complete_resources=0 PASS\n";
    }
    trace.finalizing = true; program.finalize(); trace.finalizing = false;
    trace.finalized = true; libraries_live();
    const auto identity = impl.cpu_registration_attempt(3).record.observation();
    check(identity.status == OwnershipCountStatus::Absent && !identity.owner && !identity.tracked_borrows,
          "finalization retained live compiled data identity");
  }
  check(trace.ended, "native ExecutionSession teardown was not observed");
  std::size_t dylib_bytes = 0;
  for (std::size_t i = 0; i < trace.lib_count; ++i) {
    check(trace.libs[i].released, "selected dylib request survived Runtime"); dylib_bytes += trace.libs[i].bytes;
  }
  std::cout << "cpu_jit_shutdown,window=" << name(window)
            << ",dylibs_retained_through_finalize=" << trace.lib_count
            << ",dylib_requests_released_at_shutdown=" << trace.lib_count
            << ",selected_dylib_request_bytes=" << dylib_bytes
            << ",registered_retirement=0 PASS\n";
  active = nullptr;
}
}  // namespace

std::unique_ptr<taichi::lang::JITSession> jit_real_session(taichi::lang::TaichiLLVMContext *, const taichi::lang::CompileConfig &, taichi::Arch)
    __asm__("__real__ZN6taichi4lang10JITSession6createEPNS0_17TaichiLLVMContextERKNS0_13CompileConfigENS_4ArchE");
std::unique_ptr<taichi::lang::JITSession> jit_wrap_session(taichi::lang::TaichiLLVMContext *, const taichi::lang::CompileConfig &, taichi::Arch)
    __asm__("__wrap__ZN6taichi4lang10JITSession6createEPNS0_17TaichiLLVMContextERKNS0_13CompileConfigENS_4ArchE");
std::unique_ptr<taichi::lang::JITSession> jit_wrap_session(taichi::lang::TaichiLLVMContext *context, const taichi::lang::CompileConfig &config, taichi::Arch arch) {
  auto result = jit_real_session(context, config, arch); if (active) active->session = result.get(); return result;
}
taichi::lang::JITModule *jit_real_factory(taichi::lang::LlvmRuntimeExecutor *, std::unique_ptr<llvm::Module>)
    __asm__("__real__ZN6taichi4lang19LlvmRuntimeExecutor17create_jit_moduleESt10unique_ptrIN4llvm6ModuleESt14default_deleteIS4_EE");
taichi::lang::JITModule *jit_wrap_factory(taichi::lang::LlvmRuntimeExecutor *, std::unique_ptr<llvm::Module>)
    __asm__("__wrap__ZN6taichi4lang19LlvmRuntimeExecutor17create_jit_moduleESt10unique_ptrIN4llvm6ModuleESt14default_deleteIS4_EE");
taichi::lang::JITModule *jit_wrap_factory(taichi::lang::LlvmRuntimeExecutor *executor, std::unique_ptr<llvm::Module> module) {
  auto *result = jit_real_factory(executor, std::move(module));
  if (active) {
    active->latest_module = result;
    if (active->injecting && active->window == Window::Returned) throw ReturnedModuleFault{};
  }
  return result;
}
llvm::Expected<JITDylib &> jit_real_dylib(ExecutionSession *, std::string)
    __asm__("__real__ZN4llvm3orc16ExecutionSession14createJITDylibENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE");
llvm::Expected<JITDylib &> jit_wrap_dylib(ExecutionSession *, std::string)
    __asm__("__wrap__ZN4llvm3orc16ExecutionSession14createJITDylibENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE");
llvm::Expected<JITDylib &> jit_wrap_dylib(ExecutionSession *session, std::string dylib_name) {
  if (active && active->guarded_retry) {
    check(dylib_name == active->libs[4].name && session == active->execution,
          "inner failure unexpectedly advanced the native naming counter");
    active->retry_intercepted = true; throw GuardedRetry{};
  }
  auto result = jit_real_dylib(session, std::move(dylib_name));
  if (active && result) {
    check(active->lib_count < active->libs.size(), "dylib observer full");
    auto &lib = active->libs[active->lib_count++]; lib.dylib = &*result; lib.name = lib.dylib->getName();
    active->execution = session; capture_block(lib);
    if (active->injecting && active->window == Window::Generator)
      allocation_lifetime::fail_at = allocation_lifetime::counts.attempts + 1;
  }
  return result;
}
llvm::Expected<llvm::JITEvaluatedSymbol> jit_real_lookup(ExecutionSession *, llvm::ArrayRef<JITDylib *>, SymbolStringPtr, SymbolState)
    __asm__("__real__ZN4llvm3orc16ExecutionSession6lookupENS_8ArrayRefIPNS0_8JITDylibEEENS0_15SymbolStringPtrENS0_11SymbolStateE");
llvm::Expected<llvm::JITEvaluatedSymbol> jit_wrap_lookup(ExecutionSession *, llvm::ArrayRef<JITDylib *>, SymbolStringPtr, SymbolState)
    __asm__("__wrap__ZN4llvm3orc16ExecutionSession6lookupENS_8ArrayRefIPNS0_8JITDylibEEENS0_15SymbolStringPtrENS0_11SymbolStateE");
llvm::Expected<llvm::JITEvaluatedSymbol> jit_wrap_lookup(ExecutionSession *session, llvm::ArrayRef<JITDylib *> order, SymbolStringPtr symbol, SymbolState state) {
  if (active && active->probing) {
    check(order.size() <= active->search.size(), "lookup observer full"); active->search_count = order.size();
    std::copy(order.begin(), order.end(), active->search.begin());
  }
  return jit_real_lookup(session, order, std::move(symbol), state);
}
void jit_real_pool(taichi::ThreadPool *, int, int, void *, taichi::RangeForTaskFunc *)
    __asm__("__real__ZN6taichi10ThreadPool3runEiiPvPFvS1_iiE");
void jit_wrap_pool(taichi::ThreadPool *, int, int, void *, taichi::RangeForTaskFunc *)
    __asm__("__wrap__ZN6taichi10ThreadPool3runEiiPvPFvS1_iiE");
void jit_wrap_pool(taichi::ThreadPool *pool, int splits, int workers, void *context, taichi::RangeForTaskFunc *task) {
  if (active) { ++active->pool_calls; active->pool_mismatch |= active->pool && active->pool != pool; }
  jit_real_pool(pool, splits, workers, context, task);
}
llvm::Error jit_real_end(ExecutionSession *) __asm__("__real__ZN4llvm3orc16ExecutionSession10endSessionEv");
llvm::Error jit_wrap_end(ExecutionSession *) __asm__("__wrap__ZN4llvm3orc16ExecutionSession10endSessionEv");
llvm::Error jit_wrap_end(ExecutionSession *session) {
  if (std::uncaught_exceptions()) return jit_real_end(session);
  if (active) { check(active->finalized && !active->finalizing, "session ended before completed finalization"); libraries_live(); }
  auto result = jit_real_end(session);
  if (active) {
    for (std::size_t i = 0; i < active->lib_count; ++i)
      check(!session->getJITDylibByName(active->libs[i].name), "ended session retained named dylib");
    active->ended = true;
  }
  return result;
}

int main(int argc, char **argv) {
  try {
    check(argc == 3, "expected workers and prepared|dynamic");
    const int workers = std::stoi(argv[1]); const std::string path(argv[2]);
    check((workers == 1 || workers == 2) && (path == "prepared" || path == "dynamic"), "unsupported mode");
    allocation_lifetime::begin(); allocation_lifetime::on_released = released;
    for (const auto window : {Window::Complete, Window::Generator, Window::Returned})
      run_window(workers, path == "prepared", window);
    allocation_lifetime::on_released = nullptr; allocation_lifetime::end();
    std::cout << "cpu_jit_inspection,workers=" << workers << ",path=" << path
              << ",windows=3,recoverable_allocation_failures=1,guarded_inner_retries=1,successful_outer_retries=1,cleanup_added=0 PASS\n";
    return 0;
  } catch (const std::exception &error) { std::cerr << "CPU JIT inspection: " << error.what() << '\n'; return 1; }
}
