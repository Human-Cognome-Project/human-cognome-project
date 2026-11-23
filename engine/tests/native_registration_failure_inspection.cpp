#define HCP_ALLOCATION_LIFETIME
#include "allocation_observation.h"
#include "engine/runtime.h"
#include "engine/transfer.h"
#include "taichi/codegen/llvm/compiled_kernel_data.h"
#include "taichi/runtime/program_impls/llvm/llvm_program.h"

#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace engine::testing {
struct CompiledKernelAccess {
  static const taichi::lang::Kernel &definition(const CompiledKernel &k) { return *k.kernel_; }
  static const taichi::lang::CompiledKernelData &data(const CompiledKernel &k) { return k.compiled_data(); }
};
}  // namespace engine::testing

namespace {
using namespace taichi::lang;
using Access = engine::testing::CompiledKernelAccess;
constexpr auto unlimited = std::numeric_limits<std::size_t>::max();
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
enum class Kind { Definition, Data, Module };
enum class Fault { None, BeforeModule, AfterModule };
struct Injected { Fault stage; };
struct Object { const void *address{}; Kind kind{}; bool live{true}; };
struct Attempt {
  const CompiledKernelData *record{};
  std::size_t row{};
  JITModule *module{};
  Fault fault{};
  const void *record_identity{};
};
struct Control { void *allocation{}; std::size_t bytes{}; bool released{}; };
// Fixed observer storage, never an execution or retirement authority. Copied
// identities are inspected only while live; delete observers never dereference.
struct Trace {
  std::array<Object, 64> objects{};
  std::size_t object_count{}, attempt_count{}, serial{}, last_data_release{}, first_module_release{};
  std::array<Attempt, 16> attempts{};
  std::array<Control, 8> controls{};
  std::size_t control_count{};
  Program *program{};
  const CompiledKernelData *current_record{};
  JITModule *runtime_module{}, *latest_module{};
  Fault next_fault{Fault::None};
  bool finalizing{}, qualifying_shutdown{};
  std::size_t data_during_finalize{}, modules_during_finalize{}, finalize_calls{}, snapshots{}, cache_returns{};
};
thread_local Trace *active{};
void control_released(void *pointer, std::size_t) noexcept {
  if (active) for (auto &c : active->controls)
    if (c.allocation == pointer) c.released = true;
}
std::size_t control_bytes() {
  std::size_t bytes = 0;
  for (const auto &c : active->controls) if (c.allocation && !c.released) bytes += c.bytes;
  return bytes;
}
void capture_control(const CompiledKernelData &data) {
  const auto record = reinterpret_cast<std::uintptr_t>(data.borrow_observation().record);
  check(record, "fixture did not acquire native borrow metadata");
  for (const auto &e : allocation_lifetime::table) if (e.pointer) {
    const auto begin = reinterpret_cast<std::uintptr_t>(e.pointer);
    if (record < begin || record - begin >= e.bytes) continue;
    for (const auto &c : active->controls) if (c.allocation == e.pointer) return;
    check(active->control_count < active->controls.size(), "control observer full");
    active->controls[active->control_count++] = {e.pointer, e.bytes, false}; return;
  }
  throw std::runtime_error("native control request not captured");
}
void capture(const void *address, Kind kind) {
  for (std::size_t i = 0; i < active->object_count; ++i) {
    const auto &o = active->objects[i];
    if (o.live && o.address == address) { check(o.kind == kind && kind == Kind::Data, "unexpected live identity alias"); return; }
  }
  check(active->object_count < active->objects.size(), "inspection object storage exhausted");
  active->objects[active->object_count++] = {address, kind, true};
}
std::size_t live(Kind kind) {
  std::size_t count = 0;
  for (std::size_t i = 0; i < active->object_count; ++i)
    count += active->objects[i].live && active->objects[i].kind == kind;
  return count;
}
void released(void *address) noexcept {
  if (!active) return;
  for (std::size_t i = 0; i < active->object_count; ++i) {
    auto &o = active->objects[i];
    if (!o.live || o.address != address) continue;
    o.live = false; ++active->serial;
    if (o.kind == Kind::Data) {
      active->last_data_release = active->serial;
      active->data_during_finalize += active->finalizing;
    } else if (o.kind == Kind::Module) {
      if (!active->first_module_release) active->first_module_release = active->serial;
      active->modules_during_finalize += active->finalizing;
    }
    return;
  }
}
const LlvmProgramImpl &implementation(Program &program) {
  return dynamic_cast<const LlvmProgramImpl &>(*program.get_program_impl());
}
void snapshot(Program &program, const char *phase, std::size_t data_count,
              std::size_t registered, std::size_t rows, std::size_t kernel_modules,
              bool inspect_data = true) {
  const auto *impl = program.get_program_impl();
  const auto cache = impl->compiler_ownership_counts(unlimited);
  const auto jit = implementation(program).jit_ownership_counts(unlimited);
  const auto registry = impl->launch_registry_storage();
  const auto attempts = impl->cpu_registration_attempt_counts(unlimited);
  std::size_t failed_attempts = 0;
  for (std::size_t i = 0; i < active->attempt_count; ++i)
    failed_attempts += active->attempts[i].fault != Fault::None;
  check(!attempts.complete && attempts.issued_attempts == rows &&
            attempts.recorded_attempts == rows && attempts.visited_attempts == rows &&
            !attempts.missing_metadata && !attempts.unknown_records &&
            attempts.failed_attempts == failed_attempts && attempts.uncertain_modules == failed_attempts &&
            attempts.published_attempts == rows - failed_attempts && attempts.known_modules == rows - failed_attempts &&
            (attempts.status == OwnershipCountStatus::Present ||
             (!rows && attempts.status == OwnershipCountStatus::Absent) ||
             (failed_attempts && attempts.status == OwnershipCountStatus::Incomplete)),
        "native attempt coverage/uncertainty differs from observed registration");
  for (std::size_t i = 0; i < active->attempt_count; ++i) {
    const auto entry = impl->cpu_registration_attempt(i);
    const auto identity = entry.record.observation();
    const auto &observed = active->attempts[i];
    const bool failed = observed.fault != Fault::None;
    check(entry.owner == registry.owner && entry.id == i && entry.failed == failed &&
              entry.stage == (failed ? CpuRegistrationStage::CreatingModule : CpuRegistrationStage::Published) &&
              entry.module_status == (failed ? OwnershipCountStatus::Incomplete : OwnershipCountStatus::Present) &&
              entry.module == (failed ? nullptr : observed.module) &&
              identity.record == observed.record_identity && !identity.untracked_clients &&
              identity.status == (inspect_data ? OwnershipCountStatus::Present : OwnershipCountStatus::Absent) &&
              (inspect_data ? identity.owner == cache.owner : !identity.owner && !identity.tracked_borrows),
          "native attempt association or expired identity differs from its observed record");
  }
  std::size_t observed_registered = 0;
  if (inspect_data) for (std::size_t i = 0; i < active->object_count; ++i) {
    const auto &o = active->objects[i];
    if (o.kind == Kind::Data && o.live)
      observed_registered += bool(static_cast<const CompiledKernelData *>(o.address)->get_handle());
  }
  check(!cache.complete && !jit.complete && !registry.complete, "inspection became a complete resource census");
  check(live(Kind::Data) == data_count && (!inspect_data || observed_registered == registered) &&
            (cache.status == OwnershipCountStatus::Present || (!active->cache_returns && cache.status == OwnershipCountStatus::Absent)) &&
            cache.pending_objects == data_count && !cache.loaded_objects &&
            jit.status == OwnershipCountStatus::Present && jit.runtime_modules == 1 &&
            jit.kernel_modules == kernel_modules && jit.modules == kernel_modules + 1 &&
            live(Kind::Module) == jit.modules && registry.contexts == rows,
        "registration boundary differs from native owner/deletion observations");
  check(cache.visited_entries == cache.pending_entries + cache.disk_metadata_entries,
        "compiler map coverage changed");
  ++active->snapshots;
  std::cout << "registration_storage,phase=" << phase
            << ",launcher_bytes=" << registry.object_bytes
            << ",slot_capacity_bytes=" << registry.context_capacity_bytes
            << ",selected_control_bytes=" << control_bytes() << " PASS\n";
  std::cout << "registration_sample,phase=" << phase << ",data_inspected=" << inspect_data
            << ",definitions_live=" << live(Kind::Definition) << ",pending_objects=" << data_count
            << ",registered_records=" << registered << ",launch_slots=" << rows
            << ",kernel_modules=" << kernel_modules << ",runtime_modules=1"
            << ",all_counts_complete=0 PASS\n";
}
std::unique_ptr<engine::CompiledKernel> kernel(engine::Runtime &runtime, const char *key) {
  IRBuilder body;
  auto *array = body.create_ndarray_arg_load({0}, PrimitiveType::i32, 1, 0);
  auto *value = body.create_arg_load({1}, PrimitiveType::i32, false, 0);
  body.create_global_store(body.create_external_ptr(array, {body.get_int32(0)}), value);
  auto result = std::make_unique<engine::CompiledKernel>(runtime, key, body.extract_ir(),
      std::vector<engine::Param>{engine::Param::array(PrimitiveType::i32), engine::Param::scalar(PrimitiveType::i32)});
  const auto &data = Access::data(*result);
  check(runtime.program().get_program_impl()->owns_compiled_kernel(Access::definition(*result), data),
        "definition did not resolve to the native owner");
  capture(&Access::definition(*result), Kind::Definition); capture(&data, Kind::Data);
  capture_control(data);
  ++active->cache_returns; return result;
}
struct CurrentRecord {
  explicit CurrentRecord(const CompiledKernelData &data) { check(!active->current_record, "nested registration observation"); active->current_record = &data; }
  ~CurrentRecord() { active->current_record = nullptr; }
};
void unchanged(Ndarray &array, int expected) {
  int actual = 0; engine::readback(array, &actual, sizeof(actual));
  check(actual == expected, "failed registration changed a native output");
}
void launch(engine::Runtime &runtime, engine::CompiledKernel &k, Ndarray &array, int value, bool prepared) {
  auto invocation = k.invocation(prepared); auto &ctx = invocation.get();
  if (!ctx.prepared) ctx.set_arg_ndarray({0}, array);
  engine::argument_int(ctx, 1, value);
  CurrentRecord record(Access::data(k)); k.launch(ctx); runtime.synchronize();
  check(bool(ctx.prepared) == prepared, "requested dispatch mode changed");
  unchanged(array, value);
}
int handle(const engine::CompiledKernel &k) {
  const auto &value = Access::data(k).get_handle(); check(bool(value), "native handle missing");
  return value->get_launch_id();
}
std::vector<void *> tasks(JITModule &module, const engine::CompiledKernel &k) {
  std::vector<void *> result;
  const auto &data = dynamic_cast<const LLVM::CompiledKernelData &>(Access::data(k));
  for (const auto &task : data.get_internal_data().compiled_data.tasks) {
    auto *address = module.lookup_function(task.name); check(address, "surviving task symbol missing"); result.push_back(address);
  }
  check(!result.empty(), "store fixture has no task"); return result;
}
void failed_registration(engine::Runtime &runtime, engine::CompiledKernel &k,
                         Ndarray &output, Fault fault, bool prepared) {
  auto invocation = k.invocation(prepared); auto &ctx = invocation.get();
  ctx.set_arg_ndarray({0}, output); ctx.set_arg_int({1}, 99);
  active->next_fault = fault; bool failed = false;
  {
    CurrentRecord record(Access::data(k));
    try { k.launch(ctx); }
    catch (const Injected &error) { failed = error.stage == fault; }
  }
  check(failed && active->next_fault == Fault::None && !ctx.prepared &&
            !Access::data(k).get_handle(), "fault did not stop before handle/prepared publication");
  runtime.synchronize(); unchanged(output, 0);
}
void empty_slot(Program &program, std::size_t row) {
  const auto slot = program.get_program_impl()->launch_context_storage(row, 0);
  check(slot.status == LaunchMetadataStatus::Present && !slot.complete &&
            !slot.required_parameters && !slot.visited_parameters &&
            !slot.task_capacity_bytes && !slot.parameter_capacity_bytes,
        "failed registration populated its row");
}
void failure_and_retry(engine::Runtime &runtime, Ndarray &output, const char *key,
                       Fault fault, bool prepared, std::size_t base_rows,
                       std::size_t base_modules, std::size_t data_count) {
  auto &program = runtime.program();
  const auto base_attempts = active->attempt_count;
  const CompiledKernelData *record = nullptr;
  JITModule *abandoned_module = nullptr;
  const bool after_module = fault == Fault::AfterModule;
  {
    auto candidate = kernel(runtime, key); record = &Access::data(*candidate);
    snapshot(program, after_module ? "after_module_compiled" : "before_module_compiled",
             data_count, data_count - 1, base_rows, base_modules);
    failed_registration(runtime, *candidate, output, fault, prepared);
    check(active->attempt_count == base_attempts + 1, "fault had an unexpected JIT attempt count");
    const auto &attempt = active->attempts[base_attempts];
    abandoned_module = attempt.module;
    check(attempt.record == record && attempt.row == base_rows && attempt.fault == fault &&
              bool(attempt.module) == after_module, "fault association did not match its native record/row");
    empty_slot(program, base_rows);
    snapshot(program, after_module ? "after_module_failed" : "before_module_failed",
             data_count, data_count - 1, base_rows + 1, base_modules + after_module);
  }
  snapshot(program, after_module ? "after_module_definition_gone" : "before_module_definition_gone",
           data_count, data_count - 1, base_rows + 1, base_modules + after_module);
  {
    auto retry = kernel(runtime, key);
    check(&Access::data(*retry) == record && !Access::data(*retry).get_handle(),
          "retry did not share the retained compiled record");
    launch(runtime, *retry, output, after_module ? 23 : 17, prepared);
    check(handle(*retry) == int(base_rows + 1) && active->attempt_count == base_attempts + 2,
          "retry reused a failed row or skipped its new attempt");
    const auto &attempt = active->attempts[base_attempts + 1];
    check(attempt.record == record && attempt.row == base_rows + 1 && attempt.module &&
              attempt.module != abandoned_module && attempt.fault == Fault::None,
          "retry did not get a separate native module/row");
    empty_slot(program, base_rows);
    snapshot(program, after_module ? "after_module_retry" : "before_module_retry",
             data_count, data_count, base_rows + 2, base_modules + after_module + 1);
  }
  {
    const auto attempts = active->attempt_count;
    auto revisit = kernel(runtime, key);
    check(&Access::data(*revisit) == record && handle(*revisit) == int(base_rows + 1),
          "successful same-record revisit lost its handle");
    launch(runtime, *revisit, output, after_module ? 29 : 19, prepared);
    check(active->attempt_count == attempts, "same-record revisit created another module");
  }
  snapshot(program, after_module ? "after_module_revisit_gone" : "before_module_revisit_gone",
           data_count, data_count, base_rows + 2, base_modules + after_module + 1);
  // The observer only copies opaque module identity. It never looks up symbols
  // in the failed module, executes a copied function pointer or removes resources.
  std::cout << "registration_failure,stage=" << (after_module ? "after_module" : "before_module")
            << ",failed_row=" << base_rows << ",retry_row=" << base_rows + 1
            << ",retained_failed_modules=" << after_module
            << ",same_compiled_record=1,retry_appends=1,empty_failed_slot=1"
               ",same_record_revisit_no_registration=1,failed_dispatch=0 PASS\n";
}
struct Scope {
  explicit Scope(Trace &trace) { check(!active, "nested registration trace"); active = &trace; }
  ~Scope() { active = nullptr; }
};
void run(int workers, bool prepared) {
  Trace trace; Scope observe(trace);
  {
    engine::InstanceSettings settings; settings.read_environment = false; settings.cpu_threads = workers;
    engine::Runtime runtime(settings); auto &program = runtime.program(); trace.program = &program;
    check(runtime.arch() == taichi::Arch::x64, "registration inspection requires actual x64 CPU");
    auto &impl = dynamic_cast<LlvmProgramImpl &>(*program.get_program_impl());
    trace.runtime_module = impl.get_runtime_executor()->get_runtime_jit_module();
    // Its internal executor call may not cross the link-wrapper boundary.
    // Capture the initial module from the existing owner's non-lazy getter.
    if (!trace.object_count) capture(trace.runtime_module, Kind::Module);
    check(trace.runtime_module && trace.object_count == 1 && trace.objects[0].address == trace.runtime_module,
          "initial runtime module identity was not independently captured");
    snapshot(program, "fresh", 0, 0, 0, 0);
    Ndarray first_output(&program, PrimitiveType::i32, {1}), sibling_output(&program, PrimitiveType::i32, {1});
    Ndarray candidate_output(&program, PrimitiveType::i32, {1});
    const int zero = 0; engine::upload(candidate_output, &zero, sizeof(zero));
    allocation_lifetime::begin(); allocation_lifetime::on_released = control_released;
    {
      auto first = kernel(runtime, "registration_first");
      launch(runtime, *first, first_output, 7, prepared); auto *first_module = trace.latest_module;
      auto sibling = kernel(runtime, "registration_sibling");
      launch(runtime, *sibling, sibling_output, 11, prepared); auto *sibling_module = trace.latest_module;
      const auto first_tasks = tasks(*first_module, *first), sibling_tasks = tasks(*sibling_module, *sibling);
      check(handle(*first) == 0 && handle(*sibling) == 1, "initial launch IDs changed");
      snapshot(program, "two_survivors", 2, 2, 2, 2);
      failure_and_retry(runtime, candidate_output, "registration_before", Fault::BeforeModule, prepared, 2, 2, 3);
      engine::upload(candidate_output, &zero, sizeof(zero));
      failure_and_retry(runtime, candidate_output, "registration_after", Fault::AfterModule, prepared, 4, 3, 4);
      check(handle(*first) == 0 && handle(*sibling) == 1 &&
                tasks(*first_module, *first) == first_tasks && tasks(*sibling_module, *sibling) == sibling_tasks,
            "partial registration/retry changed surviving native handles/tasks");
      launch(runtime, *first, first_output, 31, prepared); launch(runtime, *sibling, sibling_output, 37, prepared);
      snapshot(program, "surviving_dispatch", 4, 4, 6, 5);
    }
    snapshot(program, "all_definitions_gone", 4, 4, 6, 5);
    check(trace.cache_returns == 8 && trace.attempt_count == 6, "final definition/attempt totals changed");
    trace.qualifying_shutdown = true;
  }
  std::size_t definitions = 0, data = 0, modules = 0;
  for (std::size_t i = 0; i < trace.object_count; ++i) {
    const auto &o = trace.objects[i]; check(!o.live, "observed native object survived Runtime destruction");
    definitions += o.kind == Kind::Definition; data += o.kind == Kind::Data; modules += o.kind == Kind::Module;
  }
  check(definitions == 8 && data == 4 && modules == 6 && trace.finalize_calls == 1 &&
            trace.data_during_finalize == 4 && !trace.modules_during_finalize &&
            trace.first_module_release > trace.last_data_release,
        "pending-data / native-session shutdown boundary changed");
  check(trace.control_count == 4 && !control_bytes(), "selected controls survived Runtime destruction");
  allocation_lifetime::on_released = nullptr; allocation_lifetime::end();
  std::cout << "registration_inspection,workers=" << workers << ",path=" << (prepared ? "prepared" : "dynamic")
            << ",definitions=8,compiled_records=4,registered_records=4,launch_slots=6"
               ",kernel_modules=5,runtime_modules=1,registration_attempts=6,empty_failed_slots=2"
               ",retained_failed_modules=1,surviving_handles_tasks=1,sibling_dispatch=1"
               ",data_released_at_finalize=1,modules_released_after_finalize=1"
               ",registered_retirement=0 PASS\n";
}
}  // namespace

taichi::lang::JITModule *registration_real_jit(taichi::lang::LlvmRuntimeExecutor *, std::unique_ptr<llvm::Module>, int &)
    __asm__("__real__ZN6taichi4lang19LlvmRuntimeExecutor17create_jit_moduleESt10unique_ptrIN4llvm6ModuleESt14default_deleteIS4_EERi");
taichi::lang::JITModule *registration_wrap_jit(taichi::lang::LlvmRuntimeExecutor *, std::unique_ptr<llvm::Module>, int &)
    __asm__("__wrap__ZN6taichi4lang19LlvmRuntimeExecutor17create_jit_moduleESt10unique_ptrIN4llvm6ModuleESt14default_deleteIS4_EERi");
taichi::lang::JITModule *registration_wrap_jit(taichi::lang::LlvmRuntimeExecutor *executor,
                                             std::unique_ptr<llvm::Module> input, int &cpu_id) {
  Attempt *attempt = nullptr;
  if (active && active->current_record) {
    check(active->attempt_count < active->attempts.size(), "attempt observer storage exhausted");
    const auto &compiled = dynamic_cast<const taichi::lang::LLVM::CompiledKernelData &>(*active->current_record);
    const auto registry = active->program->get_program_impl()->launch_registry_storage();
    check(input && input.get() != compiled.get_internal_data().compiled_data.module.get() &&
              !compiled.get_handle() && registry.status == taichi::lang::LaunchMetadataStatus::Present &&
              registry.contexts, "JIT attempt was not a cloned, handle-free registration with an existing row");
    attempt = &active->attempts[active->attempt_count++];
    *attempt = {active->current_record, registry.contexts - 1, nullptr, active->next_fault,
                active->current_record->borrow_observation().record};
    active->next_fault = Fault::None;
    if (attempt->fault == Fault::BeforeModule) throw Injected{Fault::BeforeModule};
  }
  auto *module = registration_real_jit(executor, std::move(input), cpu_id);
  if (active) { capture(module, Kind::Module); active->latest_module = module; }
  if (attempt) {
    attempt->module = module;
    if (attempt->fault == Fault::AfterModule) throw Injected{Fault::AfterModule};
  }
  return module;
}

extern "C" void registration_real_delete(void *) noexcept __asm__("__real__ZdlPv");
extern "C" void registration_wrap_delete(void *) noexcept __asm__("__wrap__ZdlPv");
extern "C" void registration_wrap_delete(void *p) noexcept { released(p); registration_real_delete(p); }
extern "C" void registration_real_sized_delete(void *, std::size_t) noexcept __asm__("__real__ZdlPvm");
extern "C" void registration_wrap_sized_delete(void *, std::size_t) noexcept __asm__("__wrap__ZdlPvm");
extern "C" void registration_wrap_sized_delete(void *p, std::size_t n) noexcept {
  released(p); registration_real_sized_delete(p, n);
}
void registration_real_finalize(taichi::lang::Program *) __asm__("__real__ZN6taichi4lang7Program8finalizeEv");
void registration_wrap_finalize(taichi::lang::Program *) __asm__("__wrap__ZN6taichi4lang7Program8finalizeEv");
void registration_wrap_finalize(taichi::lang::Program *program) {
  if (!active || !active->qualifying_shutdown) { registration_real_finalize(program); return; }
  check(!live(Kind::Definition), "definition survived into finalization");
  snapshot(*program, "before_finalize", 4, 4, 6, 5);
  active->finalizing = true; registration_real_finalize(program); active->finalizing = false;
  ++active->finalize_calls;
  snapshot(*program, "after_finalize", 0, 0, 6, 5, false);
}

int main(int argc, char **argv) {
  try {
    check(argc == 3, "expected workers and prepared|dynamic");
    const int workers = std::stoi(argv[1]); const std::string path(argv[2]);
    check((workers == 1 || workers == 2) && (path == "prepared" || path == "dynamic"), "unsupported mode");
    run(workers, path == "prepared"); return 0;
  } catch (const std::exception &error) { std::cerr << "registration inspection: " << error.what() << '\n'; return 1; }
}
