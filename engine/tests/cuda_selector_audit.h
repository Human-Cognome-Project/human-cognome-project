#pragma once

#include <iostream>
#include <stdexcept>
#include <vector>
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicsNVPTX.h"
#include "llvm/IR/Module.h"
#include "llvm/Transforms/Utils/Cloning.h"

// Test-only guard on native selector RMW ordering. The CPU semantic/packet
// audits remain separate; this checks the additional CUDA lowering contract.
namespace cuda_selector_audit {
inline bool selector(const llvm::Instruction &i) {
  const auto *a = llvm::dyn_cast<llvm::AtomicRMWInst>(&i);
  if (!a) return false;
  const auto *v = llvm::dyn_cast<llvm::ConstantInt>(a->getValOperand());
  return v && v->getType()->isIntegerTy(32) &&
         ((a->getOperation() == llvm::AtomicRMWInst::Or && v->isZero()) ||
          (a->getOperation() == llvm::AtomicRMWInst::Xor && v->equalsInt(1)));
}
inline const llvm::Instruction *barrier(const llvm::Instruction &a, bool before) {
  for (auto *i = before ? a.getPrevNode() : a.getNextNode(); i;
       i = before ? i->getPrevNode() : i->getNextNode()) {
    if (const auto *call = llvm::dyn_cast<llvm::CallInst>(i))
      if (const auto *fn = call->getCalledFunction())
        if (fn->getIntrinsicID() == llvm::Intrinsic::nvvm_membar_gl)
          return i;
    // No packet access, other atomic, call or control-flow cut may separate
    // the RMW from its barrier. Pure address/value calculations are harmless.
    if (i->mayReadOrWriteMemory() || i->isTerminator()) return nullptr;
  }
  return nullptr;
}
inline int check(const llvm::Module &m) {
  int sites = 0;
  for (const auto &fn : m)
    for (const auto &bb : fn)
      for (const auto &i : bb)
        if (selector(i)) {
          const auto &a = llvm::cast<llvm::AtomicRMWInst>(i);
          if (a.getOrdering() != llvm::AtomicOrdering::SequentiallyConsistent ||
              !barrier(i, true) || !barrier(i, false))
            throw std::runtime_error("CUDA selector missing native device-wide ordering");
          ++sites;
        }
  if (!sites) throw std::runtime_error("CUDA selector audit found no RMW sites");
  return sites;
}
inline void qualify(const llvm::Module &m) {
  const int sites = check(m);
  int rejected = 0;
  for (bool before : {true, false}) {
    auto copy = llvm::CloneModule(m);
    llvm::Instruction *victim = nullptr;
    for (auto &fn : *copy)
      for (auto &bb : fn)
        for (auto &i : bb)
          if (!victim && selector(i))
            victim = const_cast<llvm::Instruction *>(barrier(i, before));
    if (!victim) throw std::runtime_error("missing barrier negative control");
    victim->eraseFromParent();
    try { check(*copy); }
    catch (const std::runtime_error &) { ++rejected; }
  }
  if (rejected != 2) throw std::runtime_error("missing CUDA barrier was not rejected");
  std::cout << "cuda_selector_ordering,sites=" << sites
            << ",scope=device,missing_barrier_rejections=" << rejected << " PASS\n";
}
}  // namespace cuda_selector_audit
