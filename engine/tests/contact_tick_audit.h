#pragma once

#include <set>
#include <string>
#include "live_hierarchy_audit.h"
#include "native_selection_audit.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/IntrinsicInst.h"

// Bounded verification of actual emitted x64 argument/root writes and native
// copy rows. This is not a general pointer-alias or equivalence proof. Runtime
// list/allocator bookkeeping is distinct from numerical arrays and model bits.
namespace tick_audit {
using taichi::lang::Kernel;
using taichi::lang::TypeFactory;
inline void need(bool yes, const char *why) {
  if (!yes)
    throw std::runtime_error(why);
}
struct Report {
  std::set<int> writers;
  std::size_t native_writes{}, stores{};
};
inline bool scalar(const llvm::Value *v,
                   const Kernel &kernel,
                   const llvm::Module &module,
                   int id) {
  if (const auto *cast = llvm::dyn_cast<llvm::CastInst>(v))
    return scalar(cast->getOperand(0), kernel, module, id);
  const auto *load = llvm::dyn_cast<llvm::LoadInst>(v);
  if (!load)
    return false;
  std::int64_t offset = 0;
  const auto *buffer =
      llvm::dyn_cast<llvm::LoadInst>(llvm::GetPointerBaseWithConstantOffset(
          load->getPointerOperand(), offset, module.getDataLayout()));
  if (!buffer || offset != kernel.args_type->get_element_offset({id}))
    return false;
  std::int64_t context_offset = 0;
  const auto *ctx =
      llvm::dyn_cast<llvm::Argument>(llvm::GetPointerBaseWithConstantOffset(
          buffer->getPointerOperand(), context_offset, module.getDataLayout()));
  return ctx && ctx->getArgNo() == 0 && context_offset == 0;
}
inline bool implies_enabled(const llvm::Value *condition,
                            bool truth,
                            const Kernel &kernel,
                            const llvm::Module &module,
                            int enabled) {
  if (const auto *cmp = llvm::dyn_cast<llvm::ICmpInst>(condition)) {
    for (int side = 0; side < 2; ++side) {
      const auto *zero =
          llvm::dyn_cast<llvm::ConstantInt>(cmp->getOperand(side));
      if (!zero || !zero->isZero() ||
          !scalar(cmp->getOperand(!side), kernel, module, enabled))
        continue;
      return cmp->getPredicate() ==
             (truth ? llvm::ICmpInst::ICMP_NE : llvm::ICmpInst::ICMP_EQ);
    }
  }
  if (const auto *binary = llvm::dyn_cast<llvm::BinaryOperator>(condition))
    if ((truth && binary->getOpcode() == llvm::Instruction::And) ||
        (!truth && binary->getOpcode() == llvm::Instruction::Or))
      return implies_enabled(binary->getOperand(0), truth, kernel, module,
                             enabled) ||
             implies_enabled(binary->getOperand(1), truth, kernel, module,
                             enabled);
  return false;
}
inline bool guarded(const llvm::Instruction &store,
                    const Kernel &kernel,
                    const llvm::Module &module,
                    int enabled) {
  llvm::DominatorTree dom(*const_cast<llvm::Function *>(store.getFunction()));
  for (const auto &block : *store.getFunction())
    if (const auto *branch =
            llvm::dyn_cast<llvm::BranchInst>(block.getTerminator()))
      if (branch->isConditional())
        for (int side = 0; side < 2; ++side)
          if (dom.dominates(
                  llvm::BasicBlockEdge(&block, branch->getSuccessor(side)),
                  store.getParent()) &&
              implies_enabled(branch->getCondition(), side == 0, kernel, module,
                              enabled))
            return true;
  return false;
}
inline Report writes(const Kernel &kernel,
                     const llvm::Module &module,
                     const std::set<int> &allowed,
                     const std::array<field::detail::NativeSet, 2> &sets) {
  hierarchy_audit::Analyzer arguments(kernel, module, 100);
  selection_audit::Analyzer roots(sets);
  std::set<int> trees, nodes;
  for (auto set : sets) {
    trees.insert(set.cells->get_snode_tree_id());
    nodes.insert(set.cells->id);
    nodes.insert(set.cells->parent->id);
  }
  Report report;
  const auto check = [&](const llvm::Value *pointer,
                         const llvm::Instruction &instruction,
                         const llvm::AtomicRMWInst *atomic = nullptr) {
    const int arg = arguments.data_arg(pointer);
    if (arg >= 0) {
      const auto *zero =
          atomic ? llvm::dyn_cast<llvm::ConstantInt>(atomic->getValOperand())
                 : nullptr;
      const bool acquire = atomic &&
                           atomic->getOperation() == llvm::AtomicRMWInst::Or &&
                           zero && zero->isZero();
      need(allowed.count(arg) || acquire,
           "candidate kernel writes a read-only input argument");
      if (!acquire)
        report.writers.insert(arg);
    }
    const auto origins = roots.origins(pointer);
    for (auto origin : origins) {
      need(origin.first != selection_audit::Roots &&
               origin.first != selection_audit::Ambient,
           "candidate kernel overwrites runtime root/ambient tables");
      need(origin.first != selection_audit::Tree || trees.count(origin.second),
           "candidate kernel writes committed/foreign native selection");
      need(origin.first != selection_audit::Node || nodes.count(origin.second),
           "candidate kernel writes foreign native ambient storage");
    }
    if (!origins.empty()) {
      ++report.native_writes;
      if (atomic && atomic->getType()->isIntegerTy(32) &&
          (atomic->getOperation() == llvm::AtomicRMWInst::And ||
           atomic->getOperation() == llvm::AtomicRMWInst::Or))
        need(atomic->getOrdering() ==
                 llvm::AtomicOrdering::SequentiallyConsistent,
             "candidate native mask operation lost SC ordering");
      if (const auto *store = llvm::dyn_cast<llvm::StoreInst>(&instruction))
        need(!store->getValueOperand()->getType()->isIntegerTy(32),
             "candidate native mask uses a plain word store");
    }
  };
  for (const auto &fn : module)
    for (const auto &bb : fn)
      for (const auto &instruction : bb) {
        if (const auto *store = llvm::dyn_cast<llvm::StoreInst>(&instruction)) {
          check(store->getPointerOperand(), instruction);
          ++report.stores;
        } else if (const auto *atomic =
                       llvm::dyn_cast<llvm::AtomicRMWInst>(&instruction))
          check(atomic->getPointerOperand(), instruction, atomic);
        else if (const auto *cas =
                     llvm::dyn_cast<llvm::AtomicCmpXchgInst>(&instruction))
          check(cas->getPointerOperand(), instruction);
        else if (const auto *copy =
                     llvm::dyn_cast<llvm::MemTransferInst>(&instruction))
          check(copy->getRawDest(), instruction);
        else if (const auto *clear =
                     llvm::dyn_cast<llvm::MemSetInst>(&instruction))
          check(clear->getRawDest(), instruction);
      }
  return report;
}
struct CopyRows {
  int rows{}, stride{}, rank{}, bytes{};
  bool staged{};
};
inline void copy(const Kernel &kernel,
                 const llvm::Module &module,
                 const std::vector<CopyRows> &layout,
                 bool publication) {
  hierarchy_audit::Analyzer a(kernel, module, 100);
  std::map<int, std::set<int>> seen;
  const int enabled = 2 * int(layout.size());
  for (const auto &fn : module)
    for (const auto &bb : fn)
      for (const auto &instruction : bb) {
        const auto *store = llvm::dyn_cast<llvm::StoreInst>(&instruction);
        if (!store)
          continue;
        const int dest = a.data_arg(store->getPointerOperand());
        if (dest < 0)
          continue;
        need(dest % 2 == 1 && dest / 2 < int(layout.size()),
             "native copy writes its source argument");
        const auto &shape = layout[dest / 2];
        need(publication || shape.staged,
             "native stage overwrites unrelated contact workspace");
        const auto *load =
            llvm::dyn_cast<llvm::LoadInst>(store->getValueOperand());
        need(load && a.data_arg(load->getPointerOperand()) == dest - 1,
             "native copy does not deliver its paired source");
        auto from =
            a.data_index(load->getPointerOperand(), dest - 1, shape.bytes);
        auto to = a.data_index(store->getPointerOperand(), dest, shape.bytes);
        int row_from = 0, row_to = 0;
        if (shape.rank == 2) {
          const hierarchy_audit::Monomial src_shape{std::uint64_t(dest)},
              dst_shape{std::uint64_t(dest + 1)};
          if (auto i = from.terms.find(src_shape); i != from.terms.end()) {
            row_from = int(i->second);
            from.terms.erase(i);
          }
          if (auto i = to.terms.find(dst_shape); i != to.terms.end()) {
            row_to = int(i->second);
            to.terms.erase(i);
          }
        }
        need(row_from == row_to && row_to >= 0 && row_to < shape.rows &&
                 from == to,
             "native copy mixes source/destination rows or columns");
        need(guarded(instruction, kernel, module, enabled),
             "native copy writes without its enable guard");
        seen[dest].insert(row_to);
      }
  for (std::size_t slot = 0; slot < layout.size(); ++slot) {
    if (!publication && !layout[slot].staged)
      continue;
    std::set<int> expected;
    for (int row = 0; row < layout[slot].rows; ++row)
      expected.insert(row);
    need(seen[2 * slot + 1] == expected,
         "native copy omits a complete numerical/contact surface");
  }
}
}  // namespace tick_audit
