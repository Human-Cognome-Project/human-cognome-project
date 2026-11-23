#pragma once

#include <memory>
#include <functional>

#include "taichi/runtime/llvm/llvm_fwd.h"
#include "taichi/util/lang_util.h"
#include "taichi/jit/jit_module.h"
#include "taichi/program/compiler_jit_counts.h"
#include "taichi/program/cpu_jit_presence.h"

namespace taichi::lang {

// Backend JIT compiler for all archs

class TaichiLLVMContext;
struct CompileConfig;

class JITSession {
 protected:
  TaichiLLVMContext *tlctx_;
  const CompileConfig &config_;

  std::vector<std::unique_ptr<JITModule>> modules;

 public:
  JITSession(TaichiLLVMContext *tlctx, const CompileConfig &config);

  // The executor supplies its existing runtime-module identity. A missing or
  // unmatched identity with nonempty storage leaves role counts incomplete.
  JitOwnershipCounts ownership_counts(const JITModule *runtime_module,
                                     std::size_t module_budget) const;

  virtual JITModule *add_module(std::unique_ptr<llvm::Module> M,
                                int max_reg = 0) = 0;

#ifdef TI_WITH_LLVM
  // Synchronous output only; never retained by the session. Non-CPU backends
  // keep the unavailable sentinel and their existing add_module behavior.
  virtual JITModule *add_module_with_cpu_id(std::unique_ptr<llvm::Module> M,
                                           int &issued_id, int max_reg = 0);
#endif
  virtual CpuJitPresence cpu_jit_presence(int native_id,
                                         std::size_t entry_budget) const { return {}; }

  // virtual void remove_module(JITModule *module) = 0;

  virtual void *lookup(const std::string Name) {
    TI_NOT_IMPLEMENTED
  }

  virtual llvm::DataLayout get_data_layout() = 0;

  static std::unique_ptr<JITSession> create(TaichiLLVMContext *tlctx,
                                            const CompileConfig &config,
                                            Arch arch);

  virtual ~JITSession() = default;
};

}  // namespace taichi::lang
