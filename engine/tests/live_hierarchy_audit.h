#pragma once

#include <map>
#include <set>
#include <vector>
#include <limits>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <cstdint>
#include "taichi/program/kernel.h"
#include "taichi/ir/type_factory.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Operator.h"

// Test-only verification of the actual retained LLVM, not a runtime scheduler.
// Counts describe the compiler shape. Coverage is checked at each emitted
// semantic iteration (native work-counter anchor), descriptor and packet word.
namespace hierarchy_audit {
inline void need(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
using Monomial = std::vector<std::uint64_t>;
struct Expr {
  std::map<Monomial, std::int64_t> terms;
  explicit Expr(std::int64_t c = 0) {
    if (c)
      terms[{}] = c;
  }
  static Expr atom(std::uint64_t id) {
    Expr e;
    e.terms[{id}] = 1;
    return e;
  }
  bool operator==(const Expr &o) const {
    return terms == o.terms;
  }
};
inline std::int64_t checked(__int128 v) {
  need(v >= std::numeric_limits<std::int64_t>::min() &&
           v <= std::numeric_limits<std::int64_t>::max(),
       "audit index expression overflow");
  return std::int64_t(v);
}
inline Expr add(Expr a, const Expr &b, int sign = 1) {
  for (const auto &t : b.terms) {
    auto &v = a.terms[t.first];
    v = checked(__int128(v) + __int128(sign) * t.second);
    if (!v)
      a.terms.erase(t.first);
  }
  return a;
}
inline Expr mul(const Expr &a, const Expr &b) {
  Expr out;
  for (const auto &x : a.terms)
    for (const auto &y : b.terms) {
      auto key = x.first;
      key.insert(key.end(), y.first.begin(), y.first.end());
      std::sort(key.begin(), key.end());
      need(key.size() <= 3, "unsupported packet index degree");
      auto &v = out.terms[key];
      v = checked(__int128(v) + __int128(x.second) * y.second);
      if (!v)
        out.terms.erase(key);
    }
  return out;
}
inline Expr divide(Expr e, std::int64_t n) {
  for (auto &t : e.terms) {
    need(t.second % n == 0, "non-integral native array index");
    t.second /= n;
  }
  return e;
}
enum class Role { Own, Child, ResponseChild, ResponseParent };
inline const char *name(Role r) {
  switch (r) {
    case Role::Own:
      return "own";
    case Role::Child:
      return "child";
    case Role::ResponseChild:
      return "response_child";
    case Role::ResponseParent:
      return "response_parent";
  }
  throw std::runtime_error("invalid audit role");
}
struct Packet {
  const llvm::AtomicRMWInst *acquire{};
  Role role{};
  Expr node, row;
  const llvm::LoadInst *descriptor{};
  std::map<int, std::vector<const llvm::LoadInst *>> words;
};
struct Publication {
  const llvm::AtomicRMWInst *exchange{};
  Expr node;
};
struct Result {
  std::vector<Packet> packets;
  std::vector<Publication> publications;
  int force_iterations{}, child_iterations{}, wake_iterations{},
      owner_entries{};
};
class Analyzer {
  const taichi::lang::Kernel &kernel_;
  const llvm::Module &module_;
  int mode_;
  std::map<const llvm::Value *, std::uint64_t> atoms_;
  std::map<const llvm::Value *, Expr> memo_;
  std::uint64_t next_atom_{10000};
  static std::uint64_t shape_id(int arg) {
    return 1 + arg;
  }
  const llvm::LoadInst *argument_buffer(const llvm::Value *address,
                                        std::int64_t &offset) const {
    const auto *base = llvm::GetPointerBaseWithConstantOffset(
        address, offset, module_.getDataLayout());
    const auto *buffer = llvm::dyn_cast<llvm::LoadInst>(base);
    // The candidate family also contains error branches which reload the
    // context's argument buffer. LLVM joins these equivalent loads with a PHI.
    // Accept that join only when every incoming value has the same proven ABI
    // origin; an arbitrary pointer PHI is not an argument buffer.
    if (!buffer && mode_ >= 5) {
      std::set<const llvm::Value *> visiting;
      const auto resolve =
          [&](const auto &self,
              const llvm::Value *value) -> const llvm::LoadInst * {
        if (!visiting.insert(value).second)
          return nullptr;
        const llvm::LoadInst *result = nullptr;
        if (const auto *load = llvm::dyn_cast<llvm::LoadInst>(value)) {
          std::int64_t at = 0;
          const auto *ctx = llvm::dyn_cast<llvm::Argument>(
              llvm::GetPointerBaseWithConstantOffset(
                  load->getPointerOperand(), at, module_.getDataLayout()));
          if (ctx && ctx->getArgNo() == 0 && at == 0 &&
              ctx->getParent() == load->getFunction())
            result = load;
        } else if (const auto *phi = llvm::dyn_cast<llvm::PHINode>(value)) {
          for (const auto &incoming : phi->incoming_values()) {
            const auto *load = self(self, incoming.get());
            if (!load ||
                (result && load->getFunction() != result->getFunction())) {
              result = nullptr;
              break;
            }
            result = load;
          }
        }
        visiting.erase(value);
        return result;
      };
      buffer = resolve(resolve, base);
    }
    if (!buffer)
      return nullptr;
    std::int64_t context_offset = 0;
    const auto *context =
        llvm::dyn_cast<llvm::Argument>(llvm::GetPointerBaseWithConstantOffset(
            buffer->getPointerOperand(), context_offset,
            module_.getDataLayout()));
    return context && context->getArgNo() == 0 && context_offset == 0 &&
                   context->getParent() == buffer->getFunction()
               ? buffer
               : nullptr;
  }
  Expr unknown(const llvm::Value *v) {
    auto [it, inserted] = atoms_.emplace(v, next_atom_);
    if (inserted)
      ++next_atom_;
    return Expr::atom(it->second);
  }
  Expr expression(const llvm::Value *v) {
    if (auto it = memo_.find(v); it != memo_.end())
      return it->second;
    Expr out;
    if (const auto *c = llvm::dyn_cast<llvm::ConstantInt>(v)) {
      need(c->getBitWidth() <= 64, "wide audit index constant");
      out = Expr(c->getSExtValue());
    } else if (const auto *cast = llvm::dyn_cast<llvm::CastInst>(v)) {
      // Native record/index domains are validated nonnegative i32 before use.
      if (cast->getSrcTy()->isIntegerTy() && cast->getDestTy()->isIntegerTy())
        out = expression(cast->getOperand(0));
      else
        out = unknown(v);
    } else if (const auto *load = llvm::dyn_cast<llvm::LoadInst>(v)) {
      std::int64_t offset = 0;
      bool shape = false;
      if (argument_buffer(load->getPointerOperand(), offset)) {
        const auto match_shape = [&](int arg) {
          if (offset !=
              kernel_.args_type->get_element_offset(
                  {arg, taichi::lang::TypeFactory::SHAPE_POS_IN_NDARRAY, 1}))
            return false;
          out = Expr::atom(shape_id(arg));
          shape = true;
          return true;
        };
        if (mode_ >= 5) {
          for (std::size_t arg = 0; arg < kernel_.parameter_list.size(); ++arg)
            if (kernel_.parameter_list[arg].is_array &&
                kernel_.parameter_list[arg].total_dim == 2 &&
                match_shape(int(arg)))
              break;
        } else {
          for (int arg : (mode_ >= 3 ? std::initializer_list<int>{0, 2, 3, 5}
                                     : std::initializer_list<int>{0, 10}))
            if (match_shape(arg))
              break;
        }
      }
      if (!shape)
        out = unknown(v);
    } else if (const auto *op = llvm::dyn_cast<llvm::BinaryOperator>(v)) {
      const auto a = expression(op->getOperand(0)),
                 b = expression(op->getOperand(1));
      switch (op->getOpcode()) {
        case llvm::Instruction::Add:
          out = add(a, b);
          break;
        case llvm::Instruction::Sub:
          out = add(a, b, -1);
          break;
        case llvm::Instruction::Mul:
          out = mul(a, b);
          break;
        case llvm::Instruction::Shl: {
          const auto *n = llvm::dyn_cast<llvm::ConstantInt>(op->getOperand(1));
          need(n && n->getZExtValue() < 62, "unknown native index shift");
          out = mul(a, Expr(std::int64_t{1} << n->getZExtValue()));
          break;
        }
        case llvm::Instruction::Or:
          // LLVM folds 4*selector+k into (selector<<2)|k. Only normalize
          // disjoint bits; an arbitrary OR must not be treated as addition.
          if (llvm::haveNoCommonBitsSet(op->getOperand(0), op->getOperand(1),
                                        module_.getDataLayout()))
            out = add(a, b);
          else
            out = unknown(v);
          break;
        default:
          out = unknown(v);
      }
    } else
      out = unknown(
          v);  // PHIs are per-iteration symbols, never traversed cyclically.
    return memo_[v] = out;
  }
  Expr byte_offset(const llvm::Value *ptr, int arg) {
    if (const auto *g = llvm::dyn_cast<llvm::GEPOperator>(ptr)) {
      need(g->getNumIndices() == 1, "unsupported aggregate native data index");
      return add(byte_offset(g->getPointerOperand(), arg),
                 mul(expression(g->idx_begin()->get()),
                     Expr(module_.getDataLayout()
                              .getTypeAllocSize(g->getSourceElementType())
                              .getFixedValue())));
    }
    if (const auto *cast = llvm::dyn_cast<llvm::BitCastOperator>(ptr))
      return byte_offset(cast->getOperand(0), arg);
    need(llvm::isa<llvm::LoadInst>(ptr) && data_arg(ptr) == arg,
         "unattributed native data pointer expression");
    return Expr{};
  }
  std::set<int> roots(const llvm::Value *value) const {
    std::set<const llvm::Value *> seen;
    std::set<int> out;
    const auto visit = [&](const auto &self, const llvm::Value *v) -> void {
      if (!seen.insert(v).second)
        return;
      if (const auto *load = llvm::dyn_cast<llvm::LoadInst>(v)) {
        const int arg = data_arg(load->getPointerOperand());
        if (arg == 3 || arg == 10 || arg == 18)
          out.insert(arg);
      }
      if (const auto *u = llvm::dyn_cast<llvm::User>(v))
        for (const auto &op : u->operands())
          self(self, op.get());
    };
    visit(visit, value);
    return out;
  }
  const llvm::LoadInst *descriptor(const Expr &node,
                                   const llvm::Value *ptr,
                                   int arg) {
    std::set<const llvm::Value *> seen;
    const llvm::LoadInst *out = nullptr;
    const auto visit = [&](const auto &self, const llvm::Value *v) -> void {
      if (!seen.insert(v).second)
        return;
      if (const auto *load = llvm::dyn_cast<llvm::LoadInst>(v))
        if (data_arg(load->getPointerOperand()) == arg &&
            node == expression(load)) {
          need(!out || out == load, "ambiguous native node descriptor");
          out = load;
          return;
        }
      if (const auto *u = llvm::dyn_cast<llvm::User>(v))
        for (const auto &op : u->operands())
          self(self, op.get());
    };
    visit(visit, ptr);
    need(out, "selector index is not the declared descriptor value");
    return out;
  }
  std::pair<int, Expr> response_row(const llvm::LoadInst *load) {
    auto at = divide(byte_offset(load->getPointerOperand(), 10), 4);
    const Monomial shape{shape_id(10)};
    const auto found = at.terms.find(shape);
    const auto column = found == at.terms.end() ? 0 : found->second;
    need(column == 1 || column == 2,
         "wrong response child/parent index column");
    at = add(at, mul(Expr::atom(shape_id(10)), Expr(column)), -1);
    return {int(column), at};
  }
  std::set<const llvm::AtomicRMWInst *> selectors(
      const llvm::Value *value) const {
    std::set<const llvm::Value *> seen;
    std::set<const llvm::AtomicRMWInst *> out;
    const auto visit = [&](const auto &self, const llvm::Value *v) -> void {
      if (!seen.insert(v).second)
        return;
      if (const auto *a = llvm::dyn_cast<llvm::AtomicRMWInst>(v))
        if (data_arg(a->getPointerOperand()) == 1) {
          out.insert(a);
          return;
        }
      if (const auto *u = llvm::dyn_cast<llvm::User>(v))
        for (const auto &op : u->operands())
          self(self, op.get());
    };
    visit(visit, value);
    return out;
  }
  int counter(const llvm::AtomicRMWInst &a) {
    if (data_arg(a.getPointerOperand()) != 14)
      return -1;
    const auto at = divide(byte_offset(a.getPointerOperand(), 14), 8);
    if (at.terms.size() != 1 || !at.terms.begin()->first.empty())
      return -1;
    const auto *one = llvm::dyn_cast<llvm::ConstantInt>(a.getValOperand());
    need(a.getOperation() == llvm::AtomicRMWInst::Add && one &&
             one->equalsInt(1),
         "unsupported semantic work-counter operation");
    return int(at.terms.begin()->second);
  }

 public:
  Analyzer(const taichi::lang::Kernel &kernel,
           const llvm::Module &module,
           int mode)
      : kernel_(kernel), module_(module), mode_(mode) {
  }
  bool shape_value(const llvm::Value *v, int arg) {
    return expression(v) == Expr::atom(shape_id(arg));
  }
  Expr data_index(const llvm::Value *ptr, int arg, int bytes) {
    return divide(byte_offset(ptr, arg), bytes);
  }
  int data_arg(const llvm::Value *ptr) const {
    const auto *base =
        llvm::dyn_cast<llvm::LoadInst>(llvm::getUnderlyingObject(ptr));
    if (!base || !base->getType()->isPointerTy())
      return -1;
    std::int64_t offset = 0;
    if (!argument_buffer(base->getPointerOperand(), offset))
      return -1;
    const int count = mode_ >= 5   ? int(kernel_.parameter_list.size())
                      : mode_ >= 3 ? 22
                                   : 30;
    for (int id = 0; id < count; ++id)
      if ((mode_ >= 5 ? kernel_.parameter_list[id].is_array
                      : mode_ >= 3 || id < 24 || id >= 28) &&
          offset ==
              kernel_.args_type->get_element_offset(
                  {id, taichi::lang::TypeFactory::DATA_PTR_POS_IN_NDARRAY}))
        return id;
    throw std::runtime_error(
        "unattributed ndarray access in production kernel");
  }
  Result run(bool verbose = false) {
    Result result;
    std::map<const llvm::AtomicRMWInst *, std::size_t> packet_ids;
    std::map<const llvm::Instruction *, int> counters;
    for (const auto &fn : module_)
      for (const auto &bb : fn)
        for (const auto &inst : bb) {
          if (const auto *load = llvm::dyn_cast<llvm::LoadInst>(&inst))
            need(data_arg(load->getPointerOperand()) != 1,
                 "plain production selector load");
          if (const auto *store = llvm::dyn_cast<llvm::StoreInst>(&inst))
            need(data_arg(store->getPointerOperand()) != 1,
                 "plain production selector store");
          if (const auto *cas = llvm::dyn_cast<llvm::AtomicCmpXchgInst>(&inst))
            need(data_arg(cas->getPointerOperand()) != 1,
                 "unattributed production selector compare-exchange");
          const auto *a = llvm::dyn_cast<llvm::AtomicRMWInst>(&inst);
          if (!a)
            continue;
          if (const int c = counter(*a); c >= 0)
            counters[a] = c;
          if (data_arg(a->getPointerOperand()) != 1)
            continue;
          need(a->getOrdering() == llvm::AtomicOrdering::SequentiallyConsistent,
               "production selector lost SC");
          const auto *value =
              llvm::dyn_cast<llvm::ConstantInt>(a->getValOperand());
          need(value && value->getType()->isIntegerTy(32),
               "invalid production selector operand");
          const auto node = divide(byte_offset(a->getPointerOperand(), 1), 4);
          const auto ids = roots(a->getPointerOperand());
          if (a->getOperation() == llvm::AtomicRMWInst::Xor &&
              value->equalsInt(1)) {
            need(ids == std::set<int>{18}, "publication owner path");
            descriptor(node, a->getPointerOperand(), 18);
            result.publications.push_back({a, node});
            if (verbose) {
              std::string text;
              llvm::raw_string_ostream os(text);
              a->print(os);
              std::cout << "production_selector_ir,kernel=" << mode_
                        << ",role=publish," << os.str() << '\n';
            }
            continue;
          }
          need(a->getOperation() == llvm::AtomicRMWInst::Or && value->isZero(),
               "unattributed production selector operation");
          Packet packet;
          packet.acquire = a;
          packet.node = node;
          if (ids.count(10)) {
            packet.descriptor = descriptor(node, a->getPointerOperand(), 10);
            auto row = response_row(packet.descriptor);
            packet.row = row.second;
            packet.role =
                row.first == 1 ? Role::ResponseChild : Role::ResponseParent;
          } else if (ids.count(3)) {
            packet.role = Role::Child;
            packet.descriptor = descriptor(node, a->getPointerOperand(), 3);
            packet.row = divide(
                byte_offset(packet.descriptor->getPointerOperand(), 3), 4);
          } else if (ids == std::set<int>{18}) {
            packet.role = Role::Own;
            packet.descriptor = descriptor(node, a->getPointerOperand(), 18);
            packet.row = divide(
                byte_offset(packet.descriptor->getPointerOperand(), 18), 4);
          } else
            throw std::runtime_error("unknown production acquire index path");
          packet_ids[a] = result.packets.size();
          result.packets.push_back(packet);
          if (verbose) {
            std::string text;
            llvm::raw_string_ostream os(text);
            a->print(os);
            std::cout << "production_selector_ir,kernel=" << mode_
                      << ",role=" << name(packet.role) << ',' << os.str()
                      << '\n';
          }
        }
    if (mode_ != 2)
      for (const auto &fn : module_)
        for (const auto &bb : fn)
          for (const auto &inst : bb)
            if (const auto *load = llvm::dyn_cast<llvm::LoadInst>(&inst))
              if (data_arg(load->getPointerOperand()) == 0) {
                const auto acquired = selectors(load->getPointerOperand());
                need(acquired.size() == 1,
                     "publication word has no unique acquired selector");
                const auto it = packet_ids.find(*acquired.begin());
                need(it != packet_ids.end(),
                     "publication word uses a non-acquire selector");
                auto &packet = result.packets[it->second];
                const auto at =
                    divide(byte_offset(load->getPointerOperand(), 0), 8);
                const auto slot = mul(Expr::atom(shape_id(0)),
                                      mul(expression(packet.acquire), Expr(4)));
                int word = -1;
                for (int k = 0; k < 4; ++k)
                  if (at ==
                      add(add(slot, mul(Expr::atom(shape_id(0)), Expr(k))),
                          packet.node))
                    word = k;
                need(word >= 0,
                     "packet word has wrong node, slot, or XYZ/mass row");
                packet.words[word].push_back(load);
              }
    std::map<const llvm::AtomicRMWInst *, int> used;
    for (const auto &fn : module_) {
      if (fn.isDeclaration())
        continue;
      llvm::DominatorTree dom(const_cast<llvm::Function &>(fn));
      llvm::PostDominatorTree post(const_cast<llvm::Function &>(fn));
      for (const auto &p : result.packets)
        if (p.acquire->getFunction() == &fn) {
          need(p.words.size() == 4,
               "acquired packet lacks complete XYZ/mass coverage");
          for (const auto &word : p.words)
            for (const auto *load : word.second)
              need(dom.dominates(p.acquire, load),
                   "packet load precedes its acquire");
        }
      const auto closest = [&](Role role, const llvm::Instruction *anchor,
                               bool after) -> const Packet * {
        const Packet *best = nullptr;
        for (const auto &p : result.packets)
          if (p.role == role && p.acquire->getFunction() == &fn) {
            const bool eligible = after ? post.dominates(p.acquire, anchor)
                                        : dom.dominates(p.acquire, anchor);
            if (!eligible)
              continue;
            if (!best || (after ? dom.dominates(p.acquire, best->acquire)
                                : dom.dominates(best->acquire, p.acquire)))
              best = &p;
          }
        need(best, "semantic iteration lacks a required acquired packet");
        return best;
      };
      for (const auto &[anchor, id] : counters)
        if (anchor->getFunction() == &fn) {
          if ((mode_ == 0 && id == 5) || (mode_ == 1 && id == 8)) {
            const auto *child =
                closest(Role::ResponseChild, anchor, mode_ == 1);
            const auto *parent =
                closest(Role::ResponseParent, anchor, mode_ == 1);
            need(child->row == parent->row,
                 "semantic response pair reads different influence rows");
            need(++used[child->acquire] == 1 && ++used[parent->acquire] == 1,
                 "a semantic response iteration reuses another iteration's "
                 "packet");
            if (mode_ == 0)
              ++result.force_iterations;
            else
              ++result.wake_iterations;
          } else if (mode_ == 1 && id == 4) {
            const auto *child = closest(Role::Child, anchor, false);
            need(++used[child->acquire] == 1,
                 "child reduction iteration reuses another child's packet");
            ++result.child_iterations;
          } else if (mode_ == 1 && id == 1) {
            const auto *own = closest(Role::Own, anchor, true);
            need(++used[own->acquire] == 1,
                 "owner entry reuses another node's packet");
            ++result.owner_entries;
          } else if (mode_ == 1 && id == 2) {
            int writers = 0;
            for (const auto &pub : result.publications)
              if (pub.exchange->getFunction() == &fn &&
                  dom.dominates(pub.exchange, anchor))
                ++writers;
            need(writers == 1,
                 "publication diagnostic has no unique owning exchange");
          }
        }
      for (const auto &pub : result.publications)
        if (pub.exchange->getFunction() == &fn) {
          const Packet *owner = nullptr;
          for (const auto &p : result.packets)
            if (p.role == Role::Own && p.acquire->getFunction() == &fn &&
                p.node == pub.node && dom.dominates(p.acquire, pub.exchange)) {
              need(!owner, "publication has ambiguous owner acquires");
              owner = &p;
            }
          need(owner,
               "publication does not exchange its own acquired selector");
        }
      std::map<const llvm::AtomicRMWInst *, std::set<int>> written_words;
      for (const auto &bb : fn)
        for (const auto &inst : bb)
          if (const auto *store = llvm::dyn_cast<llvm::StoreInst>(&inst))
            if (data_arg(store->getPointerOperand()) == 0) {
              need(mode_ == 1, "unexpected publication-array writer");
              const auto acquired = selectors(store->getPointerOperand());
              need(acquired.size() == 1 && packet_ids.count(*acquired.begin()),
                   "publication write has no unique acquired owner");
              const auto &owner =
                  result.packets[packet_ids.at(*acquired.begin())];
              need(owner.role == Role::Own &&
                       dom.dominates(owner.acquire, store),
                   "publication write belongs to another node");
              const Publication *writer = nullptr;
              for (const auto &pub : result.publications)
                if (pub.exchange->getFunction() == &fn &&
                    pub.node == owner.node &&
                    post.dominates(pub.exchange, store)) {
                  need(!writer, "ambiguous publication completion path");
                  writer = &pub;
                }
              need(writer,
                   "packet write has no completing publication exchange");
              // Match the inactive slot, allowing LLVM to move the XOR across
              // its scale: 4*(selector^1), 2*((2*selector)^2), or
              // (4*selector)^4.
              std::set<const llvm::Value *> seen;
              std::vector<Expr> inactive_rows;
              const auto visit = [&](const auto &self,
                                     const llvm::Value *v) -> void {
                if (!seen.insert(v).second)
                  return;
                if (const auto *op = llvm::dyn_cast<llvm::BinaryOperator>(v))
                  if (op->getOpcode() == llvm::Instruction::Xor)
                    for (int side = 0; side < 2; ++side)
                      if (const auto *c = llvm::dyn_cast<llvm::ConstantInt>(
                              op->getOperand(side))) {
                        const auto scale = c->getSExtValue();
                        if ((scale == 1 || scale == 2 || scale == 4) &&
                            expression(op->getOperand(1 - side)) ==
                                mul(expression(owner.acquire), Expr(scale)))
                          inactive_rows.push_back(
                              mul(expression(op), Expr(4 / scale)));
                      }
                if (const auto *u = llvm::dyn_cast<llvm::User>(v))
                  for (const auto &operand : u->operands())
                    self(self, operand.get());
              };
              visit(visit, store->getPointerOperand());
              const auto at =
                  divide(byte_offset(store->getPointerOperand(), 0), 8);
              int component = -1;
              for (const auto &row : inactive_rows)
                for (int k = 0; k < 4; ++k)
                  if (at == add(mul(Expr::atom(shape_id(0)), add(row, Expr(k))),
                                owner.node))
                    component = k;
              need(component >= 0,
                   "packet write has wrong inactive slot/node/word");
              written_words[writer->exchange].insert(component);
            }
      for (const auto &pub : result.publications)
        if (pub.exchange->getFunction() == &fn)
          need(written_words[pub.exchange] == std::set<int>({0, 1, 2, 3}),
               "publication exchange lacks complete inactive XYZ/mass writes");
      // A descriptor used by production response/reduction must have its own
      // matching packet. Safe readers in other clones cannot mask an omission.
      if (mode_ != 2)
        for (const auto &bb : fn)
          for (const auto &inst : bb)
            if (const auto *load = llvm::dyn_cast<llvm::LoadInst>(&inst)) {
              const int arg = data_arg(load->getPointerOperand());
              if (arg != 3 && arg != 10 && arg != 18)
                continue;
              if (arg == 10) {
                auto at = divide(byte_offset(load->getPointerOperand(), 10), 4);
                const auto it = at.terms.find({shape_id(10)});
                const auto col = it == at.terms.end() ? 0 : it->second;
                if (col != 1 && col != 2)
                  continue;
              }
              bool covered = false;
              for (const auto &p : result.packets)
                if (p.descriptor == load)
                  covered = true;
              need(covered,
                   "production node descriptor has no acquired packet path");
            }
    }
    for (const auto &p : result.packets)
      need(used[p.acquire] == 1,
           "acquired packet has no unique semantic iteration");
    if (mode_ == 0)
      need(result.force_iterations && !result.child_iterations &&
               !result.wake_iterations && !result.owner_entries &&
               result.publications.empty(),
           "missing force semantic loop coverage");
    if (mode_ == 1)
      need(result.owner_entries && result.child_iterations &&
               result.wake_iterations && !result.publications.empty() &&
               !result.force_iterations,
           "missing poll semantic loop coverage");
    if (mode_ == 2)
      need(result.packets.empty() && result.publications.empty(),
           "control unexpectedly uses selectors");
    if (verbose) {
      std::map<Role, int> counts;
      for (const auto &p : result.packets) {
        ++counts[p.role];
        std::size_t loads = 0;
        for (const auto &w : p.words)
          loads += w.second.size();
        std::cout << "production_packet_audit,kernel=" << mode_
                  << ",words=4,loads=" << loads << " PASS\n";
      }
      std::cout << "production_selector_audit,kernel=" << mode_
                << ",own=" << counts[Role::Own]
                << ",child=" << counts[Role::Child] << ",response="
                << counts[Role::ResponseChild] + counts[Role::ResponseParent]
                << ",publish=" << result.publications.size()
                << ",plain=0 PASS\n";
      std::cout << "production_semantic_audit,kernel=" << mode_
                << ",force_iterations=" << result.force_iterations
                << ",child_iterations=" << result.child_iterations
                << ",wake_iterations=" << result.wake_iterations
                << ",owner_entries=" << result.owner_entries
                << ",publication_paths=" << result.publications.size()
                << " PASS\n";
    }
    return result;
  }
};
}  // namespace hierarchy_audit
