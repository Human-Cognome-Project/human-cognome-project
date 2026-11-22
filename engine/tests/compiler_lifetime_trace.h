#pragma once

#include "taichi/codegen/llvm/compiled_kernel_data.h"
#include "taichi/program/program.h"
#include "taichi/runtime/llvm/llvm_runtime_executor.h"
#include "taichi/runtime/program_impls/llvm/llvm_program.h"
#include "taichi/jit/jit_session.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <unordered_map>
#include <vector>

// Test-only reconciliation of native owners and their read-only count APIs.
// Delete wrappers forward to the existing allocator; no eviction/private access.
namespace compiler_trace {
using namespace taichi::lang;
enum class Kind { Definition, Data, Module };
struct Object {
  const void *address;
  Kind kind;
  bool live{true};
  std::size_t released{};
};
struct Trace {
  std::vector<Object> objects;
  std::unordered_map<const void *, std::size_t> index;
  std::size_t serial{}, finalize_calls{}, data_during_finalize{};
  std::size_t compile_calls{}, distinct_jit_ir_inputs{};
  std::size_t modules_during_finalize{}, first_module_release{}, last_data_release{};
  bool mutating{}, finalizing{};
  const JITSession *jit_owner{};
  std::size_t fresh_jit_probes{}, native_count_samples{};
};
thread_local Trace *active{};
inline void check(bool ok, const char *message) {
  if (!ok) throw std::runtime_error(message);
}
inline void fresh_compiler_queries() {
  char directory[] = "/tmp/hcp-compiler-counts-XXXXXX";
  check(mkdtemp(directory), "fresh compiler fixture directory failed");
  const std::filesystem::path path(directory);
  const auto config = [&] {
    KernelCompilationManager::Config value;
    value.offline_cache_path = path.string();
    return value; // No compiler is installed in this partially initialized owner.
  };
  {
    KernelCompilationManager owner(config());
    const auto zero = owner.ownership_counts(0);
    const auto full = owner.ownership_counts(std::numeric_limits<std::size_t>::max());
    check(zero.status == OwnershipCountStatus::Present && full.owner == &owner &&
              !full.complete && !full.pending_entries && !full.disk_metadata_entries &&
              !full.pending_objects && !full.loaded_objects && !full.visited_entries &&
              std::filesystem::is_empty(path), "fresh compiler observation created cache state");
  }
  CacheData metadata;
  metadata.version[0] = TI_VERSION_MAJOR;
  metadata.version[1] = TI_VERSION_MINOR;
  metadata.version[2] = TI_VERSION_PATCH;
  metadata.kernels["count-fixture-a"].kernel_key = "count-fixture-a";
  metadata.kernels["count-fixture-b"].kernel_key = "count-fixture-b";
  const auto file = path / KernelCompilationManager::kMetadataFilename;
  taichi::write_to_binary_file(metadata, file.string());
  {
    KernelCompilationManager owner(config());
    const auto short_work = owner.ownership_counts(1);
    const auto exact = owner.ownership_counts(2);
    const auto repeat = owner.ownership_counts(2);
    check(short_work.status == OwnershipCountStatus::Incomplete && !short_work.visited_entries &&
              short_work.disk_metadata_entries == 2 && !short_work.loaded_objects &&
              exact.status == OwnershipCountStatus::Present && !exact.complete &&
              exact.disk_metadata_entries == 2 && exact.visited_entries == 2 &&
              !exact.pending_entries && !exact.loaded_objects && repeat.loaded_objects == 0 &&
              repeat.disk_metadata_entries == 2 && repeat.owner == exact.owner,
          "metadata-only compiler observation loaded or changed cache state");
  }
  check(std::filesystem::remove(file) && std::filesystem::remove(path),
        "owned compiler fixture cleanup failed");
  std::cout << "native_count_fresh_compiler,empty_present=1,metadata_entries=2,loaded_objects=0"
               ",short_work_incomplete=1,no_query_creation=1 PASS\n";
}
struct Scope {
  explicit Scope(Trace &t) {
    check(!active, "nested compiler trace"); fresh_compiler_queries(); active = &t;
  }
  ~Scope() { active = nullptr; }
};
inline void capture(const void *address, Kind kind) {
  if (!active) return;
  auto &t = *active;
  t.mutating = true;
  const auto found = t.index.find(address);
  if (found == t.index.end() || !t.objects[found->second].live) {
    const auto index = t.objects.size();
    t.objects.push_back({address, kind});
    t.index[address] = index;
  } else check(t.objects[found->second].kind == kind, "live compiler address aliased");
  t.mutating = false;
}
inline void released(void *address) noexcept {
  auto *t = active;
  if (!t || t->mutating) return;
  const auto found = t->index.find(address);
  if (found == t->index.end()) return;
  auto &object = t->objects[found->second];
  if (!object.live) return;
  object.live = false; object.released = ++t->serial;
  if (object.kind == Kind::Data) {
    t->last_data_release = t->serial;
    t->data_during_finalize += t->finalizing;
  } else if (object.kind == Kind::Module) {
    if (!t->first_module_release) t->first_module_release = t->serial;
    t->modules_during_finalize += t->finalizing;
  }
}
inline void compiled(Program &p, const Kernel &definition, const CompiledKernelData &data) {
  check(!definition.ir_is_ast() && definition.get_cached_kernel_key() == definition.get_name() &&
            p.get_program_impl()->owns_compiled_kernel(definition, data),
        "HCP definition did not resolve to its native name-keyed cache owner");
  capture(&data, Kind::Data);
  if (active) ++active->compile_calls;
}
inline void distinct_jit_ir(const llvm::Module *input) {
  if (!active) return;
  for (const auto &object : active->objects)
    if (object.kind == Kind::Data && object.live) {
      const auto &data = *static_cast<const CompiledKernelData *>(object.address);
      const auto &compiled = dynamic_cast<const LLVM::CompiledKernelData &>(data);
      check(compiled.get_internal_data().compiled_data.module.get() != input,
            "JIT input aliases the compilation manager's retained IR module");
    }
  ++active->distinct_jit_ir_inputs;
}
struct Sample {
  std::size_t definitions{}, definitions_live{}, data{}, data_live{}, registered{};
  std::size_t modules{}, modules_live{}, launch_contexts{}, tasks{};
};
inline void native_counts(Program &p, const Sample &observed, const char *phase) {
  constexpr auto unlimited = std::numeric_limits<std::size_t>::max();
  const auto *impl = p.get_program_impl();
  const auto cache = impl->compiler_ownership_counts(unlimited);
  check(!cache.complete, "compiler counts promoted to complete ownership");
  if (!active->compile_calls) {
    const auto repeat = impl->compiler_ownership_counts(0);
    check(cache.status == OwnershipCountStatus::Absent && !cache.owner &&
              repeat.status == OwnershipCountStatus::Absent && !repeat.owner,
          "fresh Program query instantiated a compiler cache");
  } else {
    check(cache.status == OwnershipCountStatus::Present && cache.owner &&
              cache.pending_objects == observed.data_live && !cache.loaded_objects &&
              cache.visited_entries == cache.pending_entries + cache.disk_metadata_entries,
          "native compiler counts differ from observed memory-only HCP objects");
    const auto exact = impl->compiler_ownership_counts(cache.visited_entries);
    check(exact.status == OwnershipCountStatus::Present && exact.owner == cache.owner &&
              exact.pending_objects == cache.pending_objects && exact.loaded_objects == cache.loaded_objects,
          "exact compiler work query changed native state");
    if (cache.visited_entries) {
      const auto short_work = impl->compiler_ownership_counts(cache.visited_entries - 1);
      check(short_work.status == OwnershipCountStatus::Incomplete && !short_work.visited_entries &&
                !short_work.pending_objects && !short_work.loaded_objects &&
                short_work.pending_entries == cache.pending_entries &&
                short_work.disk_metadata_entries == cache.disk_metadata_entries,
            "short compiler query scanned or reported partial objects");
    }
  }
  const auto &llvm_impl = dynamic_cast<const LlvmProgramImpl &>(*impl);
  const auto jit = llvm_impl.jit_ownership_counts(unlimited);
  check(jit.status == OwnershipCountStatus::Present && jit.modules,
        "live Runtime JIT owner or initial module absent");
  const auto exact = llvm_impl.jit_ownership_counts(jit.modules);
  const auto short_work = llvm_impl.jit_ownership_counts(jit.modules - 1);
  check(jit.status == OwnershipCountStatus::Present && !jit.complete &&
            jit.owner == active->jit_owner && jit.runtime_modules == 1 &&
            jit.kernel_modules == observed.modules_live && jit.modules == jit.kernel_modules + 1 &&
            jit.visited_modules == jit.modules && exact.status == OwnershipCountStatus::Present &&
            exact.owner == jit.owner && exact.modules == jit.modules &&
            short_work.status == OwnershipCountStatus::Incomplete && !short_work.visited_modules,
        "native JIT counts/role/budget mismatch");
  const auto unknown_role = active->jit_owner->ownership_counts(nullptr, unlimited);
  check(unknown_role.status == OwnershipCountStatus::Incomplete &&
            unknown_role.modules == jit.modules && !unknown_role.runtime_modules &&
            !unknown_role.kernel_modules && !unknown_role.visited_modules,
        "unidentified runtime module misclassified as a kernel");
  const auto repeat = llvm_impl.jit_ownership_counts(unlimited);
  check(repeat.modules == jit.modules && repeat.kernel_modules == jit.kernel_modules &&
            repeat.owner == jit.owner, "count observation materialized native modules");
  ++active->native_count_samples;
  std::cout << "native_owner_counts,phase=" << phase
            << ",compiler_status=" << static_cast<int>(cache.status)
            << ",pending_entries=" << cache.pending_entries << ",pending_objects=" << cache.pending_objects
            << ",disk_metadata_entries=" << cache.disk_metadata_entries
            << ",loaded_objects=" << cache.loaded_objects << ",visited_entries=" << cache.visited_entries
            << ",jit_modules=" << jit.modules << ",runtime_modules=" << jit.runtime_modules
            << ",kernel_modules=" << jit.kernel_modules << ",complete=0,no_query_creation=1 PASS\n";
}
inline Sample sample(Program &p, const char *phase, bool inspect_data = true) {
  check(active, "missing compiler trace");
  Sample out;
  std::size_t borrows{}, shared_records{}, raw_records{};
  for (const auto &object : active->objects) {
    if (object.kind == Kind::Definition) {
      ++out.definitions; out.definitions_live += object.live;
    } else if (object.kind == Kind::Data) {
      ++out.data; out.data_live += object.live;
      if (!object.live || !inspect_data) continue;
      const auto &data = *static_cast<const CompiledKernelData *>(object.address);
      const auto tracking = data.borrow_observation();
      check(tracking.status == OwnershipCountStatus::Present && tracking.owner &&
                !tracking.reclamation_supported, "native borrow metadata unavailable");
      borrows += tracking.tracked_borrows;
      shared_records += tracking.tracked_borrows > 1;
      raw_records += tracking.untracked_clients;
      check(data.arch() == taichi::Arch::x64, "compiler witness requires LLVM/x64");
      out.registered += bool(data.get_handle());
      const auto &llvm_data = dynamic_cast<const LLVM::CompiledKernelData &>(data);
      check(bool(llvm_data.get_internal_data().compiled_data.module), "compiled IR module absent");
      out.tasks += llvm_data.get_internal_data().compiled_data.tasks.size();
    } else { ++out.modules; out.modules_live += object.live; }
  }
  const auto registry = p.get_program_impl()->launch_registry_storage();
  check(registry.status == LaunchMetadataStatus::Absent ||
            registry.status == LaunchMetadataStatus::Present, "launch registry unavailable");
  out.launch_contexts = registry.contexts;
  check(out.launch_contexts == out.modules && out.registered <= out.launch_contexts,
        "CPU registrations / captured kernel JIT modules differ");
  native_counts(p, out, phase);
  if (inspect_data) {
    check(borrows == out.definitions_live && !raw_records,
          "tracked HCP borrows differ from live definitions");
    std::cout << "native_borrow_sample,phase=" << phase
              << ",tracked_borrows=" << borrows << ",shared_records=" << shared_records
              << ",raw_records=" << raw_records << ",reclamation_supported=0 PASS\n";
  }
  std::cout << "compiler_lifetime_sample,phase=" << phase
            << ",data_inspected=" << inspect_data
            << ",compile_calls=" << active->compile_calls
            << ",definitions=" << out.definitions << ",definitions_live=" << out.definitions_live
            << ",compiled_data=" << out.data << ",compiled_data_live=" << out.data_live
            << ",registered_data=" << out.registered << ",kernel_jit_modules=" << out.modules
            << ",kernel_jit_modules_live=" << out.modules_live
            << ",launch_contexts=" << out.launch_contexts << ",compiled_tasks=" << out.tasks << " PASS\n";
  return out;
}
inline void finish(const Trace &t) {
  std::size_t definitions{}, data{}, modules{};
  for (const auto &object : t.objects) {
    check(!object.live, "observed compiler/JIT object survived Runtime destruction");
    if (object.kind == Kind::Definition) ++definitions;
    else if (object.kind == Kind::Data) ++data;
    else ++modules;
  }
  check(t.finalize_calls == 1 && t.data_during_finalize == data &&
            !t.modules_during_finalize && t.first_module_release > t.last_data_release,
        "compiled-data / JIT-module release boundary changed");
  check(t.distinct_jit_ir_inputs == modules, "kernel JIT input observations missing");
  check(t.fresh_jit_probes == 1 && t.native_count_samples > 2, "native count probes missing");
  std::cout << "compiler_lifetime_summary,definitions=" << definitions << ",compiled_data=" << data
            << ",kernel_jit_modules=" << modules
            << ",definitions_released=1,data_released_during_finalize=1"
               ",jit_wrappers_released_after_finalize=1,distinct_jit_ir_inputs=1"
               ",name_keyed_native_provenance=1,native_counts_match=1"
               ",fresh_partial_owners_observed=1,all_released_after_runtime=1 PASS\n";
}
}  // namespace compiler_trace

std::unique_ptr<taichi::lang::JITSession> counts_real_session(taichi::lang::TaichiLLVMContext *,
    const taichi::lang::CompileConfig &, taichi::Arch)
    __asm__("__real__ZN6taichi4lang10JITSession6createEPNS0_17TaichiLLVMContextERKNS0_13CompileConfigENS_4ArchE");
std::unique_ptr<taichi::lang::JITSession> counts_wrap_session(taichi::lang::TaichiLLVMContext *,
    const taichi::lang::CompileConfig &, taichi::Arch)
    __asm__("__wrap__ZN6taichi4lang10JITSession6createEPNS0_17TaichiLLVMContextERKNS0_13CompileConfigENS_4ArchE");
std::unique_ptr<taichi::lang::JITSession> counts_wrap_session(taichi::lang::TaichiLLVMContext *context,
    const taichi::lang::CompileConfig &config, taichi::Arch arch) {
  auto result = counts_real_session(context, config, arch);
  if (auto *t = compiler_trace::active) {
    const auto zero = result->ownership_counts(nullptr, 0);
    const auto repeat = result->ownership_counts(nullptr, 0);
    compiler_trace::check(zero.status == taichi::lang::OwnershipCountStatus::Present &&
        !zero.complete && zero.owner == result.get() && !zero.modules &&
        !zero.runtime_modules && !zero.kernel_modules && !zero.visited_modules &&
        repeat.status == zero.status && repeat.modules == zero.modules && repeat.owner == zero.owner,
        "partially initialized JIT query materialized a module");
    t->jit_owner = result.get(); ++t->fresh_jit_probes;
    std::cout << "native_count_fresh_jit,modules=0,runtime_modules=0,kernel_modules=0"
                 ",no_query_creation=1 PASS\n";
  }
  return result;
}

extern "C" void compiler_real_delete(void *) noexcept __asm__("__real__ZdlPv");
extern "C" void compiler_wrap_delete(void *) noexcept __asm__("__wrap__ZdlPv");
extern "C" void compiler_wrap_delete(void *p) noexcept {
  compiler_trace::released(p); compiler_real_delete(p);
}
extern "C" void compiler_real_sized_delete(void *, std::size_t) noexcept __asm__("__real__ZdlPvm");
extern "C" void compiler_wrap_sized_delete(void *, std::size_t) noexcept __asm__("__wrap__ZdlPvm");
extern "C" void compiler_wrap_sized_delete(void *p, std::size_t n) noexcept {
  compiler_trace::released(p); compiler_real_sized_delete(p, n);
}
taichi::lang::JITModule *compiler_real_jit(taichi::lang::LlvmRuntimeExecutor *, std::unique_ptr<llvm::Module>)
    __asm__("__real__ZN6taichi4lang19LlvmRuntimeExecutor17create_jit_moduleESt10unique_ptrIN4llvm6ModuleESt14default_deleteIS4_EE");
taichi::lang::JITModule *compiler_wrap_jit(taichi::lang::LlvmRuntimeExecutor *, std::unique_ptr<llvm::Module>)
    __asm__("__wrap__ZN6taichi4lang19LlvmRuntimeExecutor17create_jit_moduleESt10unique_ptrIN4llvm6ModuleESt14default_deleteIS4_EE");
taichi::lang::JITModule *compiler_wrap_jit(taichi::lang::LlvmRuntimeExecutor *executor,
                                        std::unique_ptr<llvm::Module> module) {
  compiler_trace::distinct_jit_ir(module.get());
  auto *result = compiler_real_jit(executor, std::move(module));
  compiler_trace::capture(result, compiler_trace::Kind::Module);
  return result;
}
void compiler_real_finalize(taichi::lang::Program *) __asm__("__real__ZN6taichi4lang7Program8finalizeEv");
void compiler_wrap_finalize(taichi::lang::Program *) __asm__("__wrap__ZN6taichi4lang7Program8finalizeEv");
void compiler_wrap_finalize(taichi::lang::Program *p) {
  auto *t = compiler_trace::active;
  if (!t) { compiler_real_finalize(p); return; }
  const auto before = compiler_trace::sample(*p, "before_program_finalize");
  compiler_trace::check(!before.definitions_live && before.data_live == before.data &&
                         before.modules_live == before.modules, "execution/compiler lifetime boundary mismatch");
  t->finalizing = true;
  compiler_real_finalize(p);
  t->finalizing = false; ++t->finalize_calls;
  // Cache entries have been destroyed: inspect copied release flags and the
  // independently live launcher, never dereference a data pointer here.
  const auto after = compiler_trace::sample(*p, "after_program_finalize", false);
  compiler_trace::check(!after.data_live && after.modules_live == before.modules,
                       "finalization confused compiled data with JIT session lifetime");
}
