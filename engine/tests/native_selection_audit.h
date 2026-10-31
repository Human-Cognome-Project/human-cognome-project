#pragma once

#include <array>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>
#include "field/native_set.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/Module.h"

namespace selection_audit {
// Pinned LLVMRuntime ABI: roots is field 7; ambient_elements is field 15 in
// engine/taichi/taichi/runtime/llvm/runtime_module/runtime.cpp. These checks
// apply to the three scalar-only native mask kernels, not arbitrary IR. Runtime
// list/allocator bookkeeping is distinct from activity words. Runtime numerical
// packet safety is covered by the existing C3 semantic audit and state tests.
enum Kind { Roots, Ambient, Tree, Node };
using Origin = std::pair<Kind, int>;
using Origins = std::set<Origin>;
inline void require(bool value, const char *why) {
  if (!value)
    throw std::runtime_error(why);
}
struct Report {
  std::vector<const llvm::AtomicRMWInst *> masks;
  std::size_t clears{}, activations{};
};
class Analyzer {
 public:
  explicit Analyzer(const std::array<field::detail::NativeSet, 2> &allowed) {
    for (auto set : allowed) {
      trees_.insert(set.cells->get_snode_tree_id());
      nodes_.insert(set.cells->id);
      nodes_.insert(set.cells->parent->id);
    }
  }
  Origins origins(const llvm::Value *value) {
    if (auto found = cache_.find(value); found != cache_.end())
      return found->second;
    if (!processing_.insert(value).second)
      return {};
    Origins out;
    const auto add = [&](const llvm::Value *v) {
      const auto x = origins(v);
      out.insert(x.begin(), x.end());
    };
    if (auto *gep = llvm::dyn_cast<llvm::GEPOperator>(value)) {
      auto *type =
          llvm::dyn_cast<llvm::StructType>(gep->getSourceElementType());
      if (type && type->hasName() &&
          type->getName().startswith("struct.LLVMRuntime")) {
        require(gep->getNumIndices() >= 2,
                "unrecognized native runtime access");
        auto *field = llvm::dyn_cast<llvm::ConstantInt>(gep->getOperand(2));
        require(field, "dynamic native runtime field");
        if (field->getZExtValue() == 7 || field->getZExtValue() == 15) {
          require(gep->getNumIndices() == 2,
                  "unrecognized native root/ambient table access");
          out.insert({field->getZExtValue() == 7 ? Roots : Ambient, -1});
        }
      } else {
        const auto base = origins(gep->getPointerOperand());
        for (const auto &o : base) {
          if (o.first == Roots || o.first == Ambient) {
            auto *index = llvm::dyn_cast<llvm::ConstantInt>(
                gep->getOperand(gep->getNumOperands() - 1));
            require(index && gep->getNumIndices() == 1,
                    "dynamic native root/ambient lookup");
            out.insert(
                {o.first == Roots ? Tree : Node, int(index->getSExtValue())});
          } else
            out.insert(o);
        }
      }
    } else if (auto *load = llvm::dyn_cast<llvm::LoadInst>(value))
      add(load->getPointerOperand());
    else if (auto *cast = llvm::dyn_cast<llvm::CastInst>(value))
      add(cast->getOperand(0));
    else if (auto *phi = llvm::dyn_cast<llvm::PHINode>(value))
      for (const auto &input : phi->incoming_values())
        add(input.get());
    else if (auto *select = llvm::dyn_cast<llvm::SelectInst>(value)) {
      add(select->getTrueValue());
      add(select->getFalseValue());
    } else if (auto *binary = llvm::dyn_cast<llvm::BinaryOperator>(value)) {
      add(binary->getOperand(0));
      add(binary->getOperand(1));
    }
    processing_.erase(value);
    cache_[value] = out;
    return out;
  }
  Report run(const llvm::Module &module, int mode) {
    Report out;
    for (const auto &fn : module)
      for (const auto &bb : fn)
        for (const auto &i : bb) {
          if (auto *atomic = llvm::dyn_cast<llvm::AtomicRMWInst>(&i)) {
            const auto source = origins(atomic->getPointerOperand());
            check_target(source);
            if (atomic->getType()->isIntegerTy(32) &&
                (atomic->getOperation() == llvm::AtomicRMWInst::And ||
                 atomic->getOperation() == llvm::AtomicRMWInst::Or)) {
              require(!source.empty(),
                      "native mask write has unknown root provenance");
              require(atomic->getOrdering() ==
                          llvm::AtomicOrdering::SequentiallyConsistent,
                      "native mask write lost sequential consistency");
              out.masks.push_back(atomic);
              if (atomic->getOperation() == llvm::AtomicRMWInst::And)
                ++out.clears;
              else
                ++out.activations;
            }
          } else if (auto *store = llvm::dyn_cast<llvm::StoreInst>(&i)) {
            const auto source = origins(store->getPointerOperand());
            check_target(source);
            require(
                source.empty() ||
                    !store->getValueOperand()->getType()->isIntegerTy(32),
                "plain native root-word store bypasses atomic mask operations");
          } else if (auto *cas = llvm::dyn_cast<llvm::AtomicCmpXchgInst>(&i))
            check_target(origins(cas->getPointerOperand()));
        }
    require(out.clears > 0, "native selection has no mask consume/clear");
    if (mode != 1)
      require(out.activations > 0, "native selection has no mask delivery");
    return out;
  }
  llvm::GetElementPtrInst *lookup(llvm::Value *pointer, Kind kind) {
    std::set<const llvm::Value *> seen;
    return lookup_impl(pointer, kind, seen);
  }

 private:
  std::set<int> trees_, nodes_;
  std::map<const llvm::Value *, Origins> cache_;
  std::set<const llvm::Value *> processing_;
  void check_target(const Origins &source) const {
    for (auto o : source) {
      require(o.first != Tree || trees_.count(o.second),
              "native selection writes the wrong tree");
      require(o.first != Node || nodes_.count(o.second),
              "native selection writes the wrong ambient node");
      require(o.first != Roots && o.first != Ambient,
              "native selection writes runtime root/ambient tables");
    }
  }
  llvm::GetElementPtrInst *lookup_impl(llvm::Value *v,
                                       Kind kind,
                                       std::set<const llvm::Value *> &seen) {
    if (!seen.insert(v).second)
      return nullptr;
    if (auto *g = llvm::dyn_cast<llvm::GetElementPtrInst>(v)) {
      if (origins(g->getPointerOperand()).count({kind, -1}))
        return g;
    }
    if (auto *instruction = llvm::dyn_cast<llvm::Instruction>(v))
      for (auto &input : instruction->operands())
        if (auto *found = lookup_impl(input.get(), kind, seen))
          return found;
    return nullptr;
  }
};
}  // namespace selection_audit
