// A LLVM JIT compiler for CPU archs wrapper

#include <memory>
#include <charconv>
#include <limits>

#ifdef TI_WITH_LLVM
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/CompileOnDemandLayer.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/IRTransformLayer.h"
// From https://github.com/JuliaLang/julia/pull/43664
#if defined(__APPLE__) && defined(__aarch64__)
#include "llvm/ExecutionEngine/Orc/ObjectLinkingLayer.h"
#else
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#endif
#include "llvm/ExecutionEngine/RTDyldMemoryManager.h"
#include "llvm/ExecutionEngine/RuntimeDyld.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Error.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/IPO.h"

#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Host.h"

#endif

#include "taichi/jit/jit_module.h"
#include "taichi/util/lang_util.h"
#include "taichi/program/program.h"
#include "taichi/jit/jit_session.h"
#include "taichi/runtime/cpu/jit_module_id.h"
#include "taichi/util/file_sequence_writer.h"
#include "taichi/runtime/llvm/llvm_context.h"

namespace taichi::lang {

#ifdef TI_WITH_LLVM
using namespace llvm;
using namespace llvm::orc;
#if defined(__APPLE__) && defined(__aarch64__)
typedef orc::ObjectLinkingLayer ObjLayerT;
#else
typedef orc::RTDyldObjectLinkingLayer ObjLayerT;
#endif
#endif

std::pair<JITTargetMachineBuilder, llvm::DataLayout> get_host_target_info() {
  auto expected_jtmb = JITTargetMachineBuilder::detectHost();
  if (!expected_jtmb)
    TI_ERROR("LLVM TargetMachineBuilder has failed.");
  auto jtmb = *expected_jtmb;
  auto expected_data_layout = jtmb.getDefaultDataLayoutForTarget();
  if (!expected_data_layout) {
    TI_ERROR("LLVM TargetMachineBuilder has failed when getting data layout.");
  }
  auto data_layout = *expected_data_layout;
  return std::make_pair(jtmb, data_layout);
}

class JITSessionCPU;

class JITModuleCPU : public JITModule {
 private:
  JITSessionCPU *session_;
  JITDylib *dylib_;

 public:
  JITModuleCPU(JITSessionCPU *session, JITDylib *dylib)
      : session_(session), dylib_(dylib) {
  }

  void *lookup_function(const std::string &name) override;

  bool direct_dispatch() const override {
    return true;
  }
};

class JITSessionCPU : public JITSession {
 private:
  mutable ExecutionSession es_; // LLVM's read-only name lookup locks internally.
  ObjLayerT object_layer_;
  IRCompileLayer compile_layer_;
  DataLayout dl_;
  MangleAndInterner mangle_;
  mutable std::mutex mut_;
  std::vector<llvm::orc::JITDylib *> all_libs_;
  // Next unique dylib ID, including IDs consumed by failed creation.
  int module_counter_;
  SectionMemoryManager *memory_manager_;

 public:
  JITSessionCPU(TaichiLLVMContext *tlctx,
                std::unique_ptr<ExecutorProcessControl> EPC,
                const CompileConfig &config,
                JITTargetMachineBuilder JTMB,
                DataLayout DL)
      : JITSession(tlctx, config),
        es_(std::move(EPC)),
#if defined(__APPLE__) && defined(__aarch64__)
        object_layer_(es_),
#else
        object_layer_(es_,
                      [&]() {
                        auto smgr = std::make_unique<SectionMemoryManager>();
                        memory_manager_ = smgr.get();
                        return smgr;
                      }),
#endif
        compile_layer_(es_,
                       object_layer_,
                       std::make_unique<ConcurrentIRCompiler>(JTMB)),
        dl_(DL),
        mangle_(es_, this->dl_),
        module_counter_(0),
        memory_manager_(nullptr) {
    if (JTMB.getTargetTriple().isOSBinFormatCOFF()) {
      object_layer_.setOverrideObjectFlagsWithResponsibilityFlags(true);
      object_layer_.setAutoClaimResponsibilityForObjectSymbols(true);
    }
  }

  ~JITSessionCPU() override {
    std::lock_guard<std::mutex> _(mut_);
    if (memory_manager_)
      memory_manager_->deregisterEHFrames();
    if (auto Err = es_.endSession())
      es_.reportError(std::move(Err));
  }

  DataLayout get_data_layout() override {
    return dl_;
  }

  JITModule *add_module(std::unique_ptr<llvm::Module> M, int max_reg) override {
    return add_module_impl(std::move(M), max_reg, nullptr);
  }

  JITModule *add_module_with_cpu_id(std::unique_ptr<llvm::Module> M,
                                   int &issued_id, int max_reg) override {
    issued_id = -1;
    return add_module_impl(std::move(M), max_reg, &issued_id);
  }

 private:
  JITModule *add_module_impl(std::unique_ptr<llvm::Module> M, int max_reg,
                             int *issued_id) {
    TI_ASSERT(max_reg == 0);  // No need to specify max_reg on CPUs
    TI_ASSERT(M);
    std::lock_guard<std::mutex> _(mut_);
    const auto module_id = cpu::detail::issue_jit_module_id(module_counter_, issued_id);
    auto dylib_expect = es_.createJITDylib(fmt::format("{}", module_id));
    TI_ASSERT(dylib_expect);
    auto &dylib = dylib_expect.get();
    dylib.addGenerator(
        cantFail(llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
            dl_.getGlobalPrefix())));
    auto *thread_safe_context =
        this->tlctx_->get_this_thread_thread_safe_context();
    cantFail(compile_layer_.add(
        dylib,
        llvm::orc::ThreadSafeModule(std::move(M), *thread_safe_context)));
    all_libs_.push_back(&dylib);
    auto new_module = std::make_unique<JITModuleCPU>(this, &dylib);
    auto new_module_raw_ptr = new_module.get();
    modules.push_back(std::move(new_module));
    return new_module_raw_ptr;
  }

 public:
  CpuJitPresence cpu_jit_presence(int native_id, std::size_t entry_budget) const override {
    std::lock_guard<std::mutex> _(mut_);
    CpuJitPresence result;
    result.owner = this; result.native_id = native_id;
    result.issued_ids = std::size_t(module_counter_);
    // The private CPU path creates at most one dylib per issued ID. A platform
    // could add dylibs independently, invalidating that authoritative bound.
    if (es_.getPlatform()) return result;
    result.id_status = OwnershipCountStatus::Absent;
    if (native_id < 0) {
      result.status = OwnershipCountStatus::Present;
      return result;
    }
    if (std::size_t(native_id) >= result.issued_ids) {
      result.status = OwnershipCountStatus::Absent;
      return result;
    }
    result.id_status = OwnershipCountStatus::Present;
    result.status = OwnershipCountStatus::Incomplete;
    if (entry_budget < result.issued_ids) return result;
    char name[std::numeric_limits<int>::digits10 + 3];
    const auto formatted = std::to_chars(name, name + sizeof(name), native_id);
    if (formatted.ec != std::errc{}) return result;
    result.charged_entries = result.issued_ids;
    result.dylib = es_.getJITDylibByName(StringRef(name, formatted.ptr - name));
    result.dylib_status = result.dylib ? OwnershipCountStatus::Present : OwnershipCountStatus::Absent;
    result.status = OwnershipCountStatus::Present;
    return result;
  }

  void *lookup(const std::string Name) override {
    std::lock_guard<std::mutex> _(mut_);
#ifdef __APPLE__
    auto symbol = es_.lookup(all_libs_, mangle_(Name));
#else
    auto symbol = es_.lookup(all_libs_, es_.intern(Name));
#endif
    if (!symbol)
      TI_ERROR("Function \"{}\" not found", Name);
    return (void *)(symbol->getAddress());
  }

  void *lookup_in_module(JITDylib *lib, const std::string Name) {
    std::lock_guard<std::mutex> _(mut_);
#ifdef __APPLE__
    auto symbol = es_.lookup({lib}, mangle_(Name));
#else
    auto symbol = es_.lookup({lib}, es_.intern(Name));
#endif
    if (!symbol)
      TI_ERROR("Function \"{}\" not found", Name);
    return (void *)(symbol->getAddress());
  }
};

void *JITModuleCPU::lookup_function(const std::string &name) {
  return session_->lookup_in_module(dylib_, name);
}

std::unique_ptr<JITSession> create_llvm_jit_session_cpu(
    TaichiLLVMContext *tlctx,
    const CompileConfig &config,
    Arch arch) {
  TI_ASSERT(arch_is_cpu(arch));
  auto target_info = get_host_target_info();
  auto EPC = SelfExecutorProcessControl::Create();
  TI_ASSERT(EPC);
  return std::make_unique<JITSessionCPU>(tlctx, std::move(*EPC), config,
                                         target_info.first, target_info.second);
}

}  // namespace taichi::lang
