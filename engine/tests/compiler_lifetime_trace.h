#pragma once

#include "taichi/codegen/llvm/compiled_kernel_data.h"
#include "taichi/program/program.h"
#include "taichi/runtime/llvm/llvm_runtime_executor.h"

#include <iostream>
#include <unordered_map>
#include <vector>

// Test-only observations of real native owners. Delete wrappers forward to the
// existing allocator; no eviction, private access or production API is added.
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
};
thread_local Trace *active{};
inline void check(bool ok, const char *message) {
  if (!ok) throw std::runtime_error(message);
}
struct Scope {
  explicit Scope(Trace &t) { check(!active, "nested compiler trace"); active = &t; }
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
inline Sample sample(Program &p, const char *phase, bool inspect_data = true) {
  check(active, "missing compiler trace");
  Sample out;
  for (const auto &object : active->objects) {
    if (object.kind == Kind::Definition) {
      ++out.definitions; out.definitions_live += object.live;
    } else if (object.kind == Kind::Data) {
      ++out.data; out.data_live += object.live;
      if (!object.live || !inspect_data) continue;
      const auto &data = *static_cast<const CompiledKernelData *>(object.address);
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
  std::cout << "compiler_lifetime_summary,definitions=" << definitions << ",compiled_data=" << data
            << ",kernel_jit_modules=" << modules
            << ",definitions_released=1,data_released_during_finalize=1"
               ",jit_wrappers_released_after_finalize=1,distinct_jit_ir_inputs=1"
               ",name_keyed_native_provenance=1,all_released_after_runtime=1 PASS\n";
}
}  // namespace compiler_trace

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
