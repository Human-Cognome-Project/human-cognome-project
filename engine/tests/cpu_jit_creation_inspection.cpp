#define HCP_ALLOCATION_LIFETIME
#include "allocation_observation.h"
#include "engine/runtime.h"
#include "engine/transfer.h"
#include "taichi/codegen/llvm/compiled_kernel_data.h"
#include "taichi/runtime/cpu/jit_module_id.h"
#include "taichi/runtime/program_impls/llvm/llvm_program.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "cpu_jit_attribution_probe.h"

#include <array>
#include <charconv>
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
namespace attribution = cpu_jit_attribution_inspection;
constexpr auto unlimited = std::numeric_limits<std::size_t>::max();
constexpr int cells = 64;
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
enum class Window { Complete, NoId, Creation, Generator, Returned };
const char *name(Window w) {
  switch (w) {
    case Window::Complete: return "complete";
    case Window::NoId: return "no_issued_id";
    case Window::Creation: return "before_dylib";
    case Window::Generator: return "before_generator";
    case Window::Returned: return "after_wrapper_return";
  }
  return "invalid";
}
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
  bool injecting{}, creation_refused{};
  bool probing{}, pool_mismatch{}, finalizing{}, finalized{}, ended{};
  const void *pool{};
  std::array<JITDylib *, 8> search{};
  std::size_t search_count{};
  int last_native_id{-1};
  bool probing_presence{};
  std::size_t presence_lookups{};
  attribution::Token candidate_token, retry_token;
  Program *program{};
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
void fail(engine::Runtime &runtime, engine::CompiledKernel &k, Ndarray &array, bool prepared) {
  auto invocation = k.invocation(prepared); auto &ctx = invocation.get();
  ctx.set_arg_ndarray({0}, array); ctx.set_arg_int({1}, 99);
  bool caught = false;
  try { k.launch(ctx); }
  catch (const std::bad_alloc &) { caught = active->window != Window::Returned; }
  catch (const ReturnedModuleFault &) { caught = active->window == Window::Returned; }
  allocation_lifetime::fail_at = 0;
  active->injecting = false;
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
CpuJitPresence association(Trace &trace, ProgramImpl &impl,
                                const attribution::Token &token,
                                const char *phase, bool expected_dylib) {
  const auto bound = std::size_t(trace.last_native_id) + 1;
  check(trace.lib_count <= bound && !trace.execution->getPlatform(),
        "witness does not support the proposed issuance work bound");
  const auto borrows = token.record.observation().tracked_borrows;
  trace.presence_lookups = 0; trace.probing_presence = true;
  allocation_observation::calls = 0; allocation_observation::enabled = true;
  const auto short_work = impl.cpu_registration_jit_presence(token.cpu_slot, bound - 1);
  const auto unissued = trace.session->cpu_jit_presence(int(bound), bound);
  const auto no_owner = impl.cpu_registration_jit_presence(unlimited, bound);
  const auto skipped = trace.presence_lookups;
  const auto found = impl.cpu_registration_jit_presence(token.cpu_slot, bound);
  allocation_observation::enabled = false; trace.probing_presence = false;
  const bool has_id = token.native_id >= 0;
  check(short_work.status == (has_id ? OwnershipCountStatus::Incomplete : OwnershipCountStatus::Present) &&
            short_work.id_status == (has_id ? OwnershipCountStatus::Present : OwnershipCountStatus::Absent) && !short_work.charged_entries &&
            !short_work.dylib && short_work.dylib_status == OwnershipCountStatus::Unavailable &&
            unissued.status == OwnershipCountStatus::Absent && no_owner.status == OwnershipCountStatus::Absent &&
            !skipped && trace.presence_lookups == (has_id ? 1u : 0u) && !allocation_observation::calls &&
            found.status == OwnershipCountStatus::Present && !found.complete && found.owner == trace.session &&
            found.issued_ids == bound && found.native_id == token.native_id &&
            found.id_status == (has_id ? OwnershipCountStatus::Present : OwnershipCountStatus::Absent) &&
            found.charged_entries == (has_id ? bound : 0) &&
            found.dylib_status == (has_id ? (expected_dylib ? OwnershipCountStatus::Present : OwnershipCountStatus::Absent)
                                         : OwnershipCountStatus::Unavailable) &&
            bool(found.dylib) == expected_dylib && token.record.observation().tracked_borrows == borrows,
        "attribution proposal model lost presence, bounded work, owner or allocation controls");
  const auto entry = impl.cpu_registration_attempt(token.cpu_slot);
  check(entry.record.observation().record == token.record.observation().record &&
            entry.native_id == token.native_id, "CPU slot or compiled-record attribution changed");
  std::cout << "cpu_jit_native_presence,window=" << name(trace.window) << ",phase=" << phase
            << ",cpu_slot=" << token.cpu_slot << ",native_id=" << token.native_id
            << ",issued_bound=" << bound << ",retained_dylib=" << expected_dylib
            << ",id_issued=" << has_id << ",short_native_lookups=0,full_native_lookups=" << trace.presence_lookups
            << ",new_calls=0,client_borrows_added=0,complete=0 PASS\n";
  return found;
}
void run_window(int workers, bool prepared, Window window) {
  Trace trace; trace.window = window; active = &trace;
  {
    engine::InstanceSettings settings; settings.read_environment = false; settings.cpu_threads = workers;
    engine::Runtime runtime(settings); auto &program = runtime.program();
    trace.program = &program;
    auto &impl = *program.get_program_impl();
    auto &llvm_impl = dynamic_cast<LlvmProgramImpl &>(impl);
    auto *executor = llvm_impl.get_runtime_executor();
    check(impl.cpu_registration_jit_presence(0, 0).status == OwnershipCountStatus::Absent &&
              impl.launch_registry_storage().status == LaunchMetadataStatus::Absent,
          "fresh presence query created a launcher");
    trace.pool = executor->runtime_bookkeeping().thread_pool.owner;
    check(runtime.arch() == taichi::Arch::x64 && trace.session && trace.execution && trace.pool,
          "native CPU session/service capture failed");
    {
      Ndarray array(&program, PrimitiveType::i32, {cells});
      auto first = kernel(runtime, "jit_first"), sibling = kernel(runtime, "jit_sibling"), third = kernel(runtime, "jit_third");
      launch(runtime, *first, array, 7, prepared); auto *first_module = trace.latest_module;
      check(impl.launch_registry_storage().contexts == 1 &&
                impl.launch_registry_storage().context_capacity_bytes == sizeof(attribution::IdSlot),
            "layout model differs from actual native one-slot storage");
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
      if (trace.injecting) fail(runtime, *candidate, array, prepared);
      else launch(runtime, *candidate, array, 17, prepared);
      const bool complete = window == Window::Complete;
      const bool created = window != Window::Creation && window != Window::NoId;
      const bool exposed = complete || window == Window::Returned;
      const auto snapshot_dylibs = created ? 5u : 4u;
      check(trace.lib_count == snapshot_dylibs && (complete || trace.pool_calls == calls_before),
            "failure crossed the wrong JIT boundary or dispatched");
      Lib *partial = created ? &trace.libs[4] : nullptr;
      if (partial) {
        check(partial->name == "4", "initial naming order changed");
        auto flags = trace.execution->lookupFlags(LookupKind::Static,
            {{partial->dylib, JITDylibLookupFlags::MatchAllSymbols}},
            SymbolLookupSet(trace.execution->intern(candidate_task)));
        partial->ir = bool(flags);
        if (!flags) llvm::consumeError(flags.takeError());
        else check(flags->size() == 1, "accepted IR symbol flags were not retained");
        check(partial->ir == exposed, "candidate symbol coverage differs from expected IR acceptance");
        if (partial->ir) {
          partial->tracker = partial->dylib->getDefaultResourceTracker().get();
          check(!partial->tracker->isDefunct(), "IR default tracker became defunct");
        }
      } else {
        check((trace.creation_refused || window == Window::NoId) && !trace.execution->getJITDylibByName("4"),
              "before-creation failure retained an unexpected dylib");
      }
      trace.probing = true; auto *global_address = trace.session->lookup(task); trace.probing = false;
      check(global_address == first_address && trace.search_count == (exposed ? 5u : 4u) &&
                trace.search[0] == trace.libs[0].dylib &&
                (!partial || (trace.search[trace.search_count - 1] == partial->dylib) == exposed),
            "actual native session lookup exposure differs from the publication boundary");
      const auto counts = llvm_impl.jit_ownership_counts(unlimited);
      check(counts.modules == (exposed ? 5u : 4u) && counts.runtime_modules == 1,
            "wrapper count included a partial backend-only module");
      trace.candidate_token = {trace.session, 3, impl.cpu_registration_attempt(3).native_id, data(*candidate).record_identity()};
      check(trace.candidate_token.native_id == (window == Window::NoId ? -1 : 4) &&
                counts.owner == trace.session, "issued-ID owner scope changed");
      const auto initial = association(trace, impl, trace.candidate_token, "before_retry", created);
      check(initial.dylib == (partial ? partial->dylib : nullptr), "ID lookup returned another dylib");
      // A null native name lookup alone cannot prove the name was issued.
      check(!trace.execution->getJITDylibByName("5"), "future name already exists");
      if (!complete) {
        const auto entry = impl.cpu_registration_attempt(3);
        check(entry.status == OwnershipCountStatus::Incomplete && !entry.complete &&
                  entry.failed && entry.stage == CpuRegistrationStage::CreatingModule &&
                  entry.module_status == OwnershipCountStatus::Incomplete && !entry.module &&
                  entry.record.observation().record == record,
              "native attempt hid unreturned JIT side effects");
        launch(runtime, *candidate, array, 19, prepared);
        const auto retry_index = snapshot_dylibs;
        const auto wrappers = window == Window::Returned ? 6u : 5u;
        const auto retry = impl.cpu_registration_attempt(4);
        const auto expected_id = window == Window::NoId ? 4 : 5;
        check(trace.lib_count == snapshot_dylibs + 1 && trace.libs[retry_index].name == std::to_string(expected_id) &&
                  data(*candidate).get_handle()->get_launch_id() == 4 &&
                  data(*candidate).borrow_observation().record == record &&
                  llvm_impl.jit_ownership_counts(unlimited).modules == wrappers &&
                  retry.stage == CpuRegistrationStage::Published && !retry.failed &&
                  retry.module_status == OwnershipCountStatus::Present && retry.module == trace.latest_module &&
                  retry.record.observation().record == record && impl.cpu_registration_attempt(3).failed,
              "real retry reused a name, changed the record or lost attempt association");
        trace.probing = true; trace.session->lookup(task); trace.probing = false;
        check(trace.search_count == wrappers, "retry did not publish its lookup exposure");
        if (window == Window::Generator)
          for (std::size_t i = 0; i < trace.search_count; ++i)
            check(trace.search[i] != partial->dylib, "empty failed dylib became globally exposed");
        trace.retry_token = {trace.session, 4, impl.cpu_registration_attempt(4).native_id, data(*candidate).record_identity()};
        check(trace.retry_token.native_id == expected_id, "retry ID was not published to its row");
        const auto retained = association(trace, impl, trace.candidate_token, "after_retry", created);
        const auto retried = association(trace, impl, trace.retry_token, "retry", true);
        check(retained.dylib == initial.dylib && retried.dylib == trace.libs[retry_index].dylib &&
                  retried.dylib != retained.dylib, "retry replaced the failed attempt association");
      }
      if (window == Window::Generator) {
        const auto storage = impl.launch_registry_storage();
        check(storage.context_capacity_bytes % sizeof(attribution::IdSlot) == 0,
              "native slot capacity is not a whole layout extent");
        const auto capacity = storage.context_capacity_bytes / sizeof(attribution::IdSlot);
        std::cout << "cpu_jit_actual_id_storage,native_slot=" << sizeof(attribution::IdSlot)
                  << ",baseline_slot=" << sizeof(attribution::CurrentSlot)
                  << ",id_slot=" << sizeof(attribution::IdSlot)
                  << ",appended_link_slot=" << sizeof(attribution::AppendedLinkSlot)
                  << ",packed_link_slot=" << sizeof(attribution::PackedLinkSlot)
                  << ",capacity=" << capacity << ",native_capacity_bytes=" << storage.context_capacity_bytes
                  << ",id_capacity_bytes=" << capacity * sizeof(attribution::IdSlot)
                  << ",appended_link_capacity_bytes=" << capacity * sizeof(attribution::AppendedLinkSlot)
                  << ",packed_link_capacity_bytes=" << capacity * sizeof(attribution::PackedLinkSlot)
                  << ",observer_token_bytes=" << sizeof(attribution::Token)
                  << ",presence_snapshot_bytes=" << sizeof(CpuJitPresence)
                  << ",production_association=1 PASS\n";
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
      std::cout << "cpu_jit_creation,window=" << name(window) << ",snapshot_dylibs=" << snapshot_dylibs << ",accepted_ir=" << (partial && partial->ir)
                << ",lookup_exposed=" << exposed << ",stored_wrappers=" << counts.modules
                << ",final_dylibs=" << trace.lib_count
                << ",same_name_guarded_retry=0,successful_inner_retry=" << (window == Window::Creation || window == Window::Generator)
                << ",successful_outer_retry=" << (window == Window::Returned) << ",default_tracker=" << bool(partial && partial->tracker)
                << ",shared_thread_service=1"
                   ",surviving_handles_tasks_dispatch=1,complete_resources=0 PASS\n";
    }
    trace.finalizing = true; program.finalize(); trace.finalizing = false;
    trace.finalized = true; libraries_live();
    const auto identity = impl.cpu_registration_attempt(3).record.observation();
    check(identity.status == OwnershipCountStatus::Absent && !identity.owner && !identity.tracked_borrows,
          "finalization retained live compiled data identity");
    association(trace, impl, trace.candidate_token, "after_finalize", window != Window::Creation && window != Window::NoId);
    if (window != Window::Complete) association(trace, impl, trace.retry_token, "retry_after_finalize", true);
  }
  check(trace.ended, "native ExecutionSession teardown was not observed");
  check(trace.candidate_token.record.observation().status == OwnershipCountStatus::Absent &&
            !trace.candidate_token.record.observation().owner,
        "escaped attribution record metadata is not safely expired");
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
taichi::lang::JITModule *jit_real_factory(taichi::lang::LlvmRuntimeExecutor *, std::unique_ptr<llvm::Module>, int &)
    __asm__("__real__ZN6taichi4lang19LlvmRuntimeExecutor17create_jit_moduleESt10unique_ptrIN4llvm6ModuleESt14default_deleteIS4_EERi");
taichi::lang::JITModule *jit_wrap_factory(taichi::lang::LlvmRuntimeExecutor *, std::unique_ptr<llvm::Module>, int &)
    __asm__("__wrap__ZN6taichi4lang19LlvmRuntimeExecutor17create_jit_moduleESt10unique_ptrIN4llvm6ModuleESt14default_deleteIS4_EERi");
taichi::lang::JITModule *jit_wrap_factory(taichi::lang::LlvmRuntimeExecutor *executor, std::unique_ptr<llvm::Module> module, int &cpu_id) {
  if (active && active->injecting && active->window == Window::NoId) {
    check(cpu_id == -1, "factory refusal already has an issued ID");
    throw std::bad_alloc();
  }
  auto *result = jit_real_factory(executor, std::move(module), cpu_id);
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
  if (active) {
    int id = -1;
    const auto parsed = std::from_chars(dylib_name.data(), dylib_name.data() + dylib_name.size(), id);
    check(parsed.ec == std::errc{} && parsed.ptr == dylib_name.data() + dylib_name.size() &&
              id > active->last_native_id, "native issuance witness is not unique and monotonic");
    active->last_native_id = id;
    if (active->injecting) {
      const auto entry = active->program->get_program_impl()->cpu_registration_attempt(3);
      check(entry.native_id == id, "native ID was not published before creation");
    }
  }
  if (active && active->injecting && active->window == Window::Creation) {
    check(dylib_name == "4", "before-creation failure did not receive its issued ID");
    active->creation_refused = true; throw std::bad_alloc();
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
JITDylib *jit_real_presence(ExecutionSession *, llvm::StringRef)
    __asm__("__real__ZN4llvm3orc16ExecutionSession17getJITDylibByNameENS_9StringRefE");
JITDylib *jit_wrap_presence(ExecutionSession *, llvm::StringRef)
    __asm__("__wrap__ZN4llvm3orc16ExecutionSession17getJITDylibByNameENS_9StringRefE");
JITDylib *jit_wrap_presence(ExecutionSession *session, llvm::StringRef name) {
  if (active && active->probing_presence) ++active->presence_lookups;
  return jit_real_presence(session, name);
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
    int next = 0;
    int published = -1;
    try { (void)cpu::detail::issue_jit_module_id(next, &published); throw std::bad_alloc(); }
    catch (const std::bad_alloc &) {}
    check(published == 0 && next == 1, "issued ID did not survive a following formatting failure");
    next = 0;
    check(cpu::detail::issue_jit_module_id(next) == 0 && next == 1, "initial JIT ID issuance changed");
    next = std::numeric_limits<int>::max() - 1;
    check(cpu::detail::issue_jit_module_id(next) == std::numeric_limits<int>::max() - 1,
          "last supported JIT ID was refused");
    bool exhausted = false;
    published = -1;
    try { (void)cpu::detail::issue_jit_module_id(next, &published); }
    catch (const std::overflow_error &) { exhausted = true; }
    check(exhausted && next == std::numeric_limits<int>::max() && published == -1,
          "exhausted JIT counter or unissued output mutated");
    std::cout << "cpu_jit_id_bounds,first=0,last_supported=" << std::numeric_limits<int>::max() - 1
              << ",exhausted_counter_unchanged=1 PASS\n";
    allocation_lifetime::begin(); allocation_lifetime::on_released = released;
    for (const auto window : {Window::Complete, Window::NoId, Window::Creation, Window::Generator, Window::Returned})
      run_window(workers, path == "prepared", window);
    allocation_lifetime::on_released = nullptr; allocation_lifetime::end();
    std::cout << "cpu_jit_inspection,workers=" << workers << ",path=" << path
              << ",windows=5,recoverable_allocation_failures=3,successful_inner_retries=2,successful_no_id_retries=1,guarded_inner_retries=0,successful_outer_retries=1,cleanup_added=0 PASS\n";
    return 0;
  } catch (const std::exception &error) { std::cerr << "CPU JIT inspection: " << error.what() << '\n'; return 1; }
}
