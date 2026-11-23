#define HCP_ALLOCATION_LIFETIME
#include "allocation_observation.h"
#include "engine/runtime.h"
#include "engine/transfer.h"
#include "taichi/runtime/cpu/kernel_launcher.h"
#include "taichi/runtime/program_impls/llvm/llvm_program.h"

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
using Access = engine::testing::CompiledKernelAccess;
constexpr auto unlimited = std::numeric_limits<std::size_t>::max();
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
enum class Fault { None, BeforeModule, AfterModule, AfterReturnAllocation };
struct Injected {};
thread_local Fault fault{};
thread_local JITModule *last_module{};
thread_local std::size_t factory_calls{};
thread_local void *selected_control{};
thread_local bool selected_released{};
void released(void *pointer, std::size_t) noexcept {
  if (pointer == selected_control) selected_released = true;
}
void select_control(const void *record) {
  const auto address = reinterpret_cast<std::uintptr_t>(record);
  for (const auto &e : allocation_lifetime::table) if (e.pointer) {
    const auto begin = reinterpret_cast<std::uintptr_t>(e.pointer);
    if (address >= begin && address - begin < e.bytes) {
      check(e.bytes == 48, "selected control request extent changed");
      selected_control = e.pointer; return;
    }
  }
  throw std::runtime_error("selected record control was not observed");
}
class UnsupportedLauncher final : public KernelLauncher {
 public:
  void launch_kernel(const CompiledKernelData &, LaunchContextBuilder &) override {}
};
std::unique_ptr<engine::CompiledKernel> kernel(engine::Runtime &runtime, const char *key) {
  IRBuilder body;
  auto *array = body.create_ndarray_arg_load({0}, PrimitiveType::i32, 1, 0);
  auto *value = body.create_arg_load({1}, PrimitiveType::i32, false, 0);
  body.create_global_store(body.create_external_ptr(array, {body.get_int32(0)}), value);
  return std::make_unique<engine::CompiledKernel>(runtime, key, body.extract_ir(),
      std::vector<engine::Param>{engine::Param::array(PrimitiveType::i32), engine::Param::scalar(PrimitiveType::i32)});
}
void output_is(Ndarray &array, int value) {
  int actual = 0; engine::readback(array, &actual, sizeof(actual));
  check(actual == value, "attempt test output mismatch");
}
void launch(engine::Runtime &runtime, engine::CompiledKernel &k, Ndarray &array, int value, bool prepared) {
  auto invocation = k.invocation(prepared); auto &ctx = invocation.get();
  if (!ctx.prepared) ctx.set_arg_ndarray({0}, array);
  engine::argument_int(ctx, 1, value); k.launch(ctx); runtime.synchronize();
  check(bool(ctx.prepared) == prepared, "dispatch path changed"); output_is(array, value);
}
const LLVM::CompiledKernelData &data(const engine::CompiledKernel &k) {
  return dynamic_cast<const LLVM::CompiledKernelData &>(Access::data(k));
}
void fail(engine::Runtime &runtime, engine::CompiledKernel &k, Ndarray &array, Fault stage, bool prepared) {
  auto invocation = k.invocation(prepared); auto &ctx = invocation.get();
  ctx.set_arg_ndarray({0}, array); ctx.set_arg_int({1}, 99);
  fault = stage; bool caught = false;
  try { k.launch(ctx); }
  catch (const Injected &) { caught = stage != Fault::AfterReturnAllocation; }
  catch (const std::bad_alloc &) { caught = stage == Fault::AfterReturnAllocation; }
  allocation_lifetime::fail_at = 0;
  check(caught && fault == Fault::None && !ctx.prepared && !data(k).get_handle(),
        "injection did not precede publication");
  runtime.synchronize(); output_is(array, 0);
}
void association(ProgramImpl &impl, std::size_t id, const void *record, CpuRegistrationStage stage,
                 bool failed, OwnershipCountStatus module_status, const void *module) {
  const auto entry = impl.cpu_registration_attempt(id);
  const auto identity = entry.record.observation();
  check(entry.owner == impl.launch_registry_storage().owner && entry.id == id && !entry.complete &&
            entry.stage == stage && entry.failed == failed && entry.module_status == module_status &&
            entry.module == module && identity.record == record,
        "attempt association mismatch");
}
void totals(ProgramImpl &impl, const char *phase) {
  const auto c = impl.cpu_registration_attempt_counts(unlimited);
  check(c.status == OwnershipCountStatus::Incomplete && !c.complete &&
            c.issued_attempts == 8 && c.stored_slots == 8 && c.visited_attempts == 8 &&
            c.recorded_attempts == 7 && c.missing_metadata == 1 && c.unknown_records == 1 &&
            c.failed_attempts == 4 && c.published_attempts == 4 && c.known_modules == 5 && c.uncertain_modules == 2,
        "attempt census hid a gap, unknown record, or uncertain side effect");
  std::cout << "native_attempt_counts,phase=" << phase
            << ",issued=8,recorded=7,missing=1,unknown_records=1,failed=4,published=4"
               ",known_modules=5,uncertain_modules=2,complete=0 PASS\n";
}
void run(int workers, bool prepared) {
  CompiledKernelIdentity escaped; const void *first_identity = nullptr;
  {
    engine::InstanceSettings settings; settings.read_environment = false; settings.cpu_threads = workers;
    engine::Runtime runtime(settings); auto &program = runtime.program(); auto &impl = *program.get_program_impl();
    check(runtime.arch() == taichi::Arch::x64, "attempt test requires actual x64 CPU");
    UnsupportedLauncher unsupported;
    allocation_observation::calls = 0; allocation_observation::enabled = true;
    const auto fresh = impl.cpu_registration_attempt_counts(0);
    const auto absent = impl.cpu_registration_attempt(0);
    const auto unavailable = unsupported.cpu_registration_attempt_counts(0);
    const auto unavailable_entry = unsupported.cpu_registration_attempt(0);
    const auto fresh_presence = impl.cpu_registration_jit_presence(0, 0);
    const auto unsupported_presence = unsupported.cpu_registration_jit_presence(0, 0);
    allocation_observation::enabled = false;
    check(fresh.status == OwnershipCountStatus::Absent && absent.status == OwnershipCountStatus::Absent &&
              unavailable.status == OwnershipCountStatus::Unavailable &&
              unavailable_entry.status == OwnershipCountStatus::Unavailable &&
              fresh_presence.status == OwnershipCountStatus::Absent &&
              unsupported_presence.status == OwnershipCountStatus::Unavailable &&
              impl.launch_registry_storage().status == LaunchMetadataStatus::Absent &&
              !allocation_observation::calls, "fresh query allocated or created a launcher");
    auto &llvm_impl = dynamic_cast<LlvmProgramImpl &>(impl);
    auto &launcher = dynamic_cast<cpu::KernelLauncher &>(impl.get_kernel_launcher());
    allocation_lifetime::begin(); allocation_lifetime::on_released = released;
    {
      Ndarray array(&program, PrimitiveType::i32, {1}); const int zero = 0;
      auto first = kernel(runtime, "attempt_first");
      first_identity = data(*first).borrow_observation().record; select_control(first_identity);
      const auto calls_before = factory_calls;
      allocation_lifetime::fail_at = allocation_lifetime::counts.attempts + 1;
      bool caught = false;
      try { launcher.register_llvm_kernel(data(*first)); } catch (const std::bad_alloc &) { caught = true; }
      allocation_lifetime::fail_at = 0;
      const auto gap = impl.cpu_registration_attempt_counts(unlimited);
      const auto gap_presence = impl.cpu_registration_jit_presence(0, 0);
      check(gap_presence.status == OwnershipCountStatus::Present &&
                gap_presence.id_status == OwnershipCountStatus::Absent && gap_presence.native_id == -1 &&
                gap_presence.dylib_status == OwnershipCountStatus::Unavailable && !gap_presence.charged_entries,
            "failed row storage manufactured a native issued ID");
      check(caught && factory_calls == calls_before && !data(*first).get_handle() &&
                gap.issued_attempts == 1 && !gap.stored_slots && !gap.recorded_attempts && gap.missing_metadata == 1,
            "row allocation failure did not retain an explicit issued-ID gap");
      association(impl, 0, nullptr, CpuRegistrationStage::Unobserved, true, OwnershipCountStatus::Absent, nullptr);
      launch(runtime, *first, array, 7, prepared);
      association(impl, 1, first_identity, CpuRegistrationStage::Published, false, OwnershipCountStatus::Present, last_module);
      allocation_observation::calls = 0; allocation_observation::enabled = true;
      const auto short_scan = impl.cpu_registration_attempt_counts(1);
      const auto snapshot = impl.cpu_registration_attempt(1);
      auto copy = snapshot.record; escaped = std::move(copy);
      const auto borrowed = escaped.observation();
      allocation_observation::enabled = false;
      check(short_scan.status == OwnershipCountStatus::Incomplete && short_scan.issued_attempts == 2 &&
                short_scan.stored_slots == 2 && !short_scan.visited_attempts && !short_scan.recorded_attempts &&
                !short_scan.failed_attempts && !short_scan.known_modules && !allocation_observation::calls &&
                borrowed.tracked_borrows == 1 && !borrowed.untracked_clients,
            "bounded query or identity copy allocated/acquired a client borrow");
      std::cout << "native_attempt_gap,issued=1,stored=0,retry_id=1,short_query_roles=0,query_new_calls=0,identity_borrows=0 PASS\n";

      const void *known_identity = nullptr, *known_module = nullptr;
      {
        auto candidate = kernel(runtime, "attempt_known_failure");
        known_identity = data(*candidate).borrow_observation().record;
        engine::upload(array, &zero, sizeof(zero)); fail(runtime, *candidate, array, Fault::AfterReturnAllocation, prepared);
        known_module = last_module;
        association(impl, 2, known_identity, CpuRegistrationStage::ModuleReturned, true, OwnershipCountStatus::Present, known_module);
      }
      check(impl.cpu_registration_attempt(2).record.observation().tracked_borrows == 0,
            "attempt identity became a borrower");
      auto known_retry = kernel(runtime, "attempt_known_failure");
      check(data(*known_retry).borrow_observation().record == known_identity, "retry changed known record identity");
      launch(runtime, *known_retry, array, 11, prepared);
      check(last_module != known_module && data(*known_retry).get_handle()->get_launch_id() == 3,
            "known-module retry reused its failed slot/module");
      association(impl, 3, known_identity, CpuRegistrationStage::Published, false, OwnershipCountStatus::Present, last_module);
      auto before = kernel(runtime, "attempt_before");
      const auto before_identity = data(*before).borrow_observation().record;
      const auto modules_before = llvm_impl.jit_ownership_counts(unlimited).modules;
      engine::upload(array, &zero, sizeof(zero)); fail(runtime, *before, array, Fault::BeforeModule, prepared);
      check(llvm_impl.jit_ownership_counts(unlimited).modules == modules_before, "before-call injection created a wrapper");
      association(impl, 4, before_identity, CpuRegistrationStage::CreatingModule, true, OwnershipCountStatus::Incomplete, nullptr);
      const void *after_identity = nullptr, *unreturned_module = nullptr;
      {
        auto after = kernel(runtime, "attempt_after"); after_identity = data(*after).borrow_observation().record;
        fail(runtime, *after, array, Fault::AfterModule, prepared); unreturned_module = last_module;
      }
      association(impl, 5, after_identity, CpuRegistrationStage::CreatingModule, true, OwnershipCountStatus::Incomplete, nullptr);
      auto after_retry = kernel(runtime, "attempt_after");
      check(data(*after_retry).borrow_observation().record == after_identity, "retry changed uncertain record identity");
      launch(runtime, *after_retry, array, 17, prepared);
      check(last_module != unreturned_module && data(*after_retry).get_handle()->get_launch_id() == 6,
            "uncertain retry reused failed resources");
      association(impl, 6, after_identity, CpuRegistrationStage::Published, false, OwnershipCountStatus::Present, last_module);
      auto clone = data(*first).clone();
      check(clone->borrow_observation().status == OwnershipCountStatus::Unavailable, "clone inherited provenance");
      launcher.register_llvm_kernel(dynamic_cast<const LLVM::CompiledKernelData &>(*clone));
      association(impl, 7, nullptr, CpuRegistrationStage::Published, false, OwnershipCountStatus::Present, last_module);
      check(impl.cpu_registration_attempt(7).status == OwnershipCountStatus::Incomplete &&
                clone->borrow_observation().status == OwnershipCountStatus::Unavailable,
            "observation manufactured unmanaged identity");
      clone.reset();
      launch(runtime, *first, array, 23, prepared); launch(runtime, *known_retry, array, 29, prepared);
      launch(runtime, *after_retry, array, 31, prepared);
      check(data(*first).get_handle()->get_launch_id() == 1 && data(*known_retry).get_handle()->get_launch_id() == 3,
            "surviving handles changed");
      check(llvm_impl.jit_ownership_counts(unlimited).modules == 7, "native module census mismatch");
      totals(impl, "before_finalize");
      std::cout << "native_attempt_failure,known_after_return=1,earlier_side_effects_incomplete=1,retry_appends=1"
                   ",unmanaged_identity_unavailable=1,surviving_dispatch=1 PASS\n";
    }
    program.finalize();
    totals(impl, "after_finalize");
    for (const auto id : {1u, 2u, 3u, 4u, 5u, 6u}) {
      const auto identity = impl.cpu_registration_attempt(id).record.observation();
      check(identity.status == OwnershipCountStatus::Absent && identity.record && !identity.owner &&
                !identity.tracked_borrows && !identity.untracked_clients, "finalization left a live record identity");
    }
    check(escaped.observation().record == first_identity && !selected_released,
          "finalization lost the selected identity");
  }
  const auto expired = escaped.observation();
  check(expired.status == OwnershipCountStatus::Absent && expired.record == first_identity &&
            !expired.owner && !expired.tracked_borrows && !selected_released,
        "identity did not safely outlive Runtime");
  escaped = {};
  check(selected_released, "last metadata-only identity did not release its control");
  allocation_lifetime::on_released = nullptr; allocation_lifetime::end();
  std::cout << "native_attempt_abi,compiled_base=" << sizeof(CompiledKernelData)
            << ",compiled_llvm=" << sizeof(LLVM::CompiledKernelData)
            << ",hcp_wrapper=" << sizeof(engine::CompiledKernel)
            << ",guard=" << sizeof(CompiledKernelBorrow)
            << ",identity=" << sizeof(CompiledKernelIdentity)
            << ",attempt_snapshot=" << sizeof(CpuRegistrationAttempt)
            << ",count_snapshot=" << sizeof(CpuRegistrationAttemptCounts) << " PASS\n";
  std::cout << "native_attempt_shutdown,workers=" << workers << ",path=" << (prepared ? "prepared" : "dynamic")
            << ",expired_identity_safe=1,last_identity_releases_control=1,registered_retirement=0 PASS\n";
}
}  // namespace

taichi::lang::JITModule *attempt_real_jit(taichi::lang::LlvmRuntimeExecutor *, std::unique_ptr<llvm::Module>, int &)
    __asm__("__real__ZN6taichi4lang19LlvmRuntimeExecutor17create_jit_moduleESt10unique_ptrIN4llvm6ModuleESt14default_deleteIS4_EERi");
taichi::lang::JITModule *attempt_wrap_jit(taichi::lang::LlvmRuntimeExecutor *, std::unique_ptr<llvm::Module>, int &)
    __asm__("__wrap__ZN6taichi4lang19LlvmRuntimeExecutor17create_jit_moduleESt10unique_ptrIN4llvm6ModuleESt14default_deleteIS4_EERi");
taichi::lang::JITModule *attempt_wrap_jit(taichi::lang::LlvmRuntimeExecutor *executor, std::unique_ptr<llvm::Module> input, int &cpu_id) {
  const auto stage = fault; fault = Fault::None; ++factory_calls;
  if (stage == Fault::BeforeModule) throw Injected{};
  last_module = attempt_real_jit(executor, std::move(input), cpu_id);
  if (stage == Fault::AfterModule) throw Injected{};
  if (stage == Fault::AfterReturnAllocation) allocation_lifetime::fail_at = allocation_lifetime::counts.attempts + 1;
  return last_module;
}

int main(int argc, char **argv) {
  try {
    check(argc == 3, "expected workers and prepared|dynamic");
    const int workers = std::stoi(argv[1]); const std::string path(argv[2]);
    check((workers == 1 || workers == 2) && (path == "prepared" || path == "dynamic"), "unsupported mode");
    run(workers, path == "prepared"); return 0;
  } catch (const std::exception &error) { std::cerr << "registration attempts: " << error.what() << '\n'; return 1; }
}
