#include "scoped_hierarchy_audit.h"

#include <cstdlib>

#include "field/field.h"
#include "field/live_hierarchy.h"
#include "taichi/codegen/llvm/compiled_kernel_data.h"
#include "live_hierarchy_audit.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Transforms/Utils/Cloning.h"

namespace engine::testing {
struct CompiledKernelAccess {
  static const taichi::lang::Kernel &kernel(const CompiledKernel &k) {
    return *k.kernel_;
  }
  static const llvm::Module &module(const CompiledKernel &k) {
    return *dynamic_cast<const taichi::lang::LLVM::CompiledKernelData &>(
                *k.compiled_).get_internal_data().compiled_data.module;
  }
};
}  // namespace engine::testing

namespace field::testing {
struct LiveHierarchyAccess {
  static auto kernels(Harness &h) { return h.live_hierarchy_->kernels(); }
  static auto refresh_kernels(Harness &h) {
    return h.live_hierarchy_->refresh_kernels();
  }
};
}  // namespace field::testing

namespace {
using namespace llvm;
using hierarchy_audit::Analyzer;
using hierarchy_audit::Expr;
using hierarchy_audit::Packet;
using hierarchy_audit::Role;
using hierarchy_audit::need;

const LoadInst *load(const Value *v) { return dyn_cast_or_null<LoadInst>(v); }

bool cell(Analyzer &a, const Value *v, int arg, int word, const Expr &row,
          int bytes) {
  const auto *l = load(v);
  return l && a.data_arg(l->getPointerOperand()) == arg &&
         a.data_index(l->getPointerOperand(), arg, bytes) ==
             a.cell_index(arg, word, row);
}

bool number(const Value *v, int n) {
  const auto *c = dyn_cast<ConstantInt>(v);
  return c && c->getSExtValue() == n;
}

const Value *integer_origin(const Value *v) {
  while (const auto *cast = dyn_cast<CastInst>(v)) {
    if (!cast->isIntegerCast())
      break;
    v = cast->getOperand(0);
  }
  return v;
}

const Value *comparison(const Value *v, ICmpInst::Predicate pred, int rhs) {
  const auto *cmp = dyn_cast<ICmpInst>(v);
  if (cmp && cmp->getPredicate() == pred && number(cmp->getOperand(1), rhs))
    return cmp->getOperand(0);
  return nullptr;
}

const BinaryOperator *binary(const Value *v, unsigned opcode) {
  const auto *op = dyn_cast<BinaryOperator>(v);
  return op && op->getOpcode() == opcode ? op : nullptr;
}

const Value *unsigned_count(const Value *v) {
  const auto *cast = dyn_cast<UIToFPInst>(v);
  return cast ? cast->getOperand(0) : nullptr;
}

const Value *bits(const Value *v) {
  const auto *cast = dyn_cast_or_null<BitCastInst>(v);
  return cast ? cast->getOperand(0) : v;
}

bool exact_ratio(Analyzer &a, const Value *v, int arg, const Expr &row) {
  const auto *div = binary(v, Instruction::FDiv);
  return div && cell(a, unsigned_count(div->getOperand(0)), arg, 0, row, 8) &&
         cell(a, unsigned_count(div->getOperand(1)), arg, 1, row, 8);
}

bool depends_on(const Value *start, const Value *target) {
  SmallPtrSet<const Value *, 32> seen;
  SmallVector<const Value *, 16> pending{start};
  while (!pending.empty()) {
    const auto *v = pending.pop_back_val();
    if (v == target)
      return true;
    if (!seen.insert(v).second || isa<LoadInst>(v))
      continue;
    if (const auto *u = dyn_cast<User>(v))
      for (const auto &operand : u->operands())
        pending.push_back(operand.get());
  }
  return false;
}

// LLVM can sink the three coordinate loads across the signed split. Expose
// their edge-specific addresses to the ordinary packet verifier. The loads
// cross only the predecessor terminator and the join's PHIs/other plain loads.
void unfold_poll_coordinates(Module &module, Analyzer &a) {
  for (auto &f : module)
    for (auto &block : f) {
      auto at = block.getFirstNonPHI()->getIterator();
      while (auto *read = dyn_cast<LoadInst>(&*at)) {
        ++at;
        const auto *ptr = dyn_cast<PHINode>(read->getPointerOperand());
        if (!ptr || ptr->getParent() != &block || ptr->getNumIncomingValues() != 2 ||
            !read->isSimple() || !read->getType()->isDoubleTy())
          continue;
        const int left = a.data_arg(ptr->getIncomingValue(0));
        const int right = a.data_arg(ptr->getIncomingValue(1));
        if (left != 0 && right != 0)
          continue;
        need((left == 0 && right == 7) || (left == 7 && right == 0),
             "scoped coordinate join must select packet or body");
        auto *joined = PHINode::Create(read->getType(), 2, "", &block.front());
        for (unsigned i = 0; i < 2; ++i) {
          auto *pred = ptr->getIncomingBlock(i);
          need(pred->getSingleSuccessor() == &block,
               "scoped coordinate join has a bypass edge");
          auto *incoming = new LoadInst(read->getType(), ptr->getIncomingValue(i),
                                        "", false, read->getAlign(),
                                        pred->getTerminator());
          joined->addIncoming(incoming, pred);
        }
        read->replaceAllUsesWith(joined);
        read->eraseFromParent();
      }
    }
}

struct ScopedRow {
  const BranchInst *split;
  const LoadInst *child;
  const LoadInst *body;
  const BinaryOperator *weighted_mass;
};

std::vector<ScopedRow> scoped_rows(const Module &module, Analyzer &a, bool seed) {
  const int children = seed ? 7 : 3, sources = seed ? 8 : 4;
  const int ratios = seed ? 9 : 5;
  std::vector<ScopedRow> rows;
  for (const auto &f : module) {
    if (f.isDeclaration())
      continue;
    DominatorTree dom(const_cast<Function &>(f));
    for (const auto &block : f) {
      const auto *branch = dyn_cast<BranchInst>(block.getTerminator());
      if (!branch || !branch->isConditional())
        continue;
      const auto *child = load(comparison(branch->getCondition(), ICmpInst::ICMP_SGT, -1));
      if (!child || a.data_arg(child->getPointerOperand()) != children)
        continue;
      const auto *scoped = branch->getSuccessor(1);
      const auto source = hierarchy_audit::add(Expr(-1), a.index(child), -1);
      const LoadInst *body = nullptr;
      const BinaryOperator *mass = nullptr;
      for (const auto &inst : instructions(f)) {
        if (!dom.dominates(scoped, inst.getParent()))
          continue;
        if (cell(a, &inst, sources, 0, source, 4)) {
          need(!body, "ambiguous scoped body descriptor");
          body = load(&inst);
        }
        if (const auto *mul = binary(&inst, Instruction::FMul))
          if (exact_ratio(a, mul->getOperand(1), ratios, source)) {
            need(!mass, "ambiguous scoped mass term");
            mass = mul;
          }
      }
      need(body && mass, "scoped child must decode its own source row and exact ratio");
      rows.push_back({branch, child, body, mass});
    }
  }
  need(!rows.empty(), "missing signed scoped child split");
  return rows;
}

void poll_rows(Analyzer &a, const hierarchy_audit::Result &report,
               const std::vector<ScopedRow> &rows) {
  need(rows.size() == std::size_t(report.child_iterations),
       "scoped splits and acquired child packets differ in number");
  for (const auto &row : rows) {
    const Packet *packet = nullptr;
    for (const auto &p : report.packets)
      if (p.role == Role::Child && p.descriptor == row.child) {
        need(!packet, "signed split has multiple acquired child packets");
        packet = &p;
      }
    const auto *whole = row.split->getSuccessor(0);
    const auto *scoped = row.split->getSuccessor(1);
    const auto *join = whole->getSingleSuccessor();
    need(packet && packet->acquire->getParent() == whole && join &&
             scoped->getSingleSuccessor() == join,
         "scoped branch must join its paired whole-packet branch");
    need(cell(a, row.weighted_mass->getOperand(0), 7, field::kMass,
              a.index(row.body), 8), "scoped mass reads the selected body");
    for (int word = 0; word < 4; ++word) {
      const PHINode *joined = nullptr;
      for (const auto &phi : join->phis()) {
        const auto &words = packet->words.at(word);
        const auto *packet_value = phi.getIncomingValueForBlock(whole);
        if (std::find(words.begin(), words.end(), packet_value) != words.end()) {
          need(!joined, "ambiguous scoped reduction operand");
          joined = &phi;
        }
      }
      need(joined, "reduction must join all four packet words");
      const auto *value = joined->getIncomingValueForBlock(scoped);
      need(word == 3 ? value == row.weighted_mass
                     : cell(a, value, 7, field::kPosX + word, a.index(row.body), 8),
           "scoped reduction joins a wrong XYZ/mass operand");
      bool published = false;
      for (const auto &[exchange, stores] : report.writes)
        if (exchange->getFunction() == joined->getFunction())
          published |= depends_on(stores.at(word)->getValueOperand(), joined);
      need(published, "scoped joined operand has no published reduction");
    }
  }
}

void check_seed(const taichi::lang::Kernel &kernel, const Module &module) {
  Analyzer abi(kernel, module, 5);
  const auto rows = scoped_rows(module, abi, true);
  for (const auto &row : rows) {
    const auto &f = *row.split->getFunction();
    DominatorTree dom(const_cast<Function &>(f));
    bool whole = false, validated = false, accumulates = false;
    for (const auto &inst : instructions(f)) {
      whole |= dom.dominates(row.split->getSuccessor(0), inst.getParent()) &&
               cell(abi, &inst, 4, 0, abi.index(row.child), 4);
      if (const auto *branch = dyn_cast<BranchInst>(&inst))
        validated |= branch->isConditional() &&
                     dom.dominates(row.split->getSuccessor(1), inst.getParent()) &&
                     cell(abi, comparison(branch->getCondition(), ICmpInst::ICMP_SGT, 1),
                          14, 0, abi.index(row.body), 4);
      if (const auto *sum = binary(&inst, Instruction::FAdd))
        accumulates |= isa<PHINode>(sum->getOperand(0)) &&
                       depends_on(sum->getOperand(1), row.weighted_mass);
    }
    need(whole && validated && accumulates,
         "seed must use the whole selector or validate and accumulate the scoped row");
  }
  std::cerr << "scoped_compiler_audit,seed,child_splits=" << rows.size() << " PASS\n";
}
bool packet_word(const Packet &packet, int word, const Value *v) {
  const auto &loads = packet.words.at(word);
  return std::find(loads.begin(), loads.end(), v) != loads.end();
}

// For wake comparisons a publication contributes either its pre-exchange word
// or the corresponding inactive-slot store. A different node uses the
// acquired branch packet unchanged.
const Value *replacement(const Value *v, const Packet &parent,
                         const Packet &owner, int word) {
  const auto *s = dyn_cast<SelectInst>(v);
  const auto *eq = s ? dyn_cast<ICmpInst>(s->getCondition()) : nullptr;
  if (eq && eq->getPredicate() == ICmpInst::ICMP_EQ &&
      ((eq->getOperand(0) == owner.descriptor && eq->getOperand(1) == parent.descriptor) ||
       (eq->getOperand(1) == owner.descriptor && eq->getOperand(0) == parent.descriptor)) &&
      packet_word(parent, word, s->getFalseValue()))
    return s->getTrueValue();
  return nullptr;
}

std::vector<const SelectInst *> responses(Analyzer &a,
                                           const hierarchy_audit::Result &report,
                                           bool wake) {
  std::vector<const SelectInst *> paths;
  for (const auto &p : report.packets) {
    if (p.role != Role::ResponseParent)
      continue;
    const auto ref = hierarchy_audit::add(
        a.data_index(p.descriptor->getPointerOperand(), 10, 4),
        a.cell_index(10, 2, Expr{}), -1);
    const auto &f = *p.acquire->getFunction();
    DominatorTree dom(const_cast<Function &>(f));
    LoopInfo loops(dom);
    const auto *loop = loops.getLoopFor(p.acquire->getParent());
    need(loop && loop->getLoopPreheader(), "response packet lacks an influence loop");
    const LoadInst *offset = nullptr;
    for (const auto &phi : loop->getHeader()->phis())
      if (phi.getType()->isIntegerTy() && a.index(&phi) == ref)
        offset = load(integer_origin(
            phi.getIncomingValueForBlock(loop->getLoopPreheader())));
    need(offset && a.data_arg(offset->getPointerOperand()) == 12,
         "response loop lacks a recipient influence offset");
    const auto recipient = a.data_index(offset->getPointerOperand(), 12, 4);
    if (wake) {
      bool from_coverage = false;
      for (const auto &i : instructions(f))
        if (const auto *l = load(&i))
          from_coverage |= a.data_arg(l->getPointerOperand()) == 9 &&
                           a.index(l) == recipient && dom.dominates(l, offset);
      need(from_coverage, "wake response recipient lacks branch coverage");
    }
    const Packet *owner = nullptr;
    const std::map<int, const StoreInst *> *writes = nullptr;
    if (wake) {
      for (const auto &candidate : report.packets)
        if (candidate.role == Role::Own &&
            dom.dominates(candidate.acquire, p.acquire)) {
          need(!owner, "wake has multiple owning packets");
          owner = &candidate;
        }
      need(owner, "wake has no owning packet");
      for (const auto &publication : report.publications)
        if (publication.node == owner->node &&
            dom.dominates(publication.exchange, p.acquire)) {
          need(!writes, "wake has multiple preceding publications");
          writes = &report.writes.at(publication.exchange);
        }
      need(writes, "wake has no preceding publication");
    }
    int count = 0;
    const SelectInst *eligibility = nullptr;
    for (const auto &inst : instructions(f)) {
      const auto *x = dyn_cast<SelectInst>(&inst);
      if (!x || !dom.dominates(p.acquire, x) ||
          !cell(a, comparison(x->getCondition(), ICmpInst::ICMP_EQ, 2),
                10, 3, ref, 4))
        continue;
      const auto *dx = binary(x->getTrueValue(), Instruction::FSub);
      if (x->getType()->isIntegerTy(1)) {
        const auto *both = binary(x->getTrueValue(), Instruction::And);
        const auto branch = a.index(p.descriptor);
        need(both &&
                 cell(a, comparison(both->getOperand(0), ICmpInst::ICMP_NE, 0),
                      2, 8, branch, 4) &&
                 cell(a, comparison(both->getOperand(1), ICmpInst::ICMP_SGT, 1),
                      2, 2, branch, 4),
             "scoped response eligibility must use branch coverage");
        need(!eligibility, "multiple scoped gates for one influence");
        eligibility = x;
        continue;
      }
      if (!dx || !cell(a, dx->getOperand(1), 7, field::kPosX, recipient, 8))
        continue;
      const auto matches = [&](const Value *v, int word, bool next) {
        if (!wake)
          return packet_word(p, word, v);
        const auto *substituted = replacement(v, p, *owner, word);
        return next ? bits(substituted) == bits(writes->at(word)->getValueOperand())
                    : packet_word(*owner, word, substituted);
      };
      const bool next = wake && matches(dx->getOperand(0), 0, true);
      for (int word = 0; word < 3; ++word) {
        int matched = 0;
        for (const auto &candidate : *x->getParent())
          if (const auto *sel = dyn_cast<SelectInst>(&candidate))
            if (sel->getCondition() == x->getCondition())
              if (const auto *delta = binary(sel->getTrueValue(), Instruction::FSub))
                if (cell(a, delta->getOperand(1), 7, field::kPosX + word, recipient, 8) &&
                    matches(delta->getOperand(0), word, next))
                  ++matched;
        need(matched == 1, "scoped response must subtract the selected recipient XYZ from its owning branch");
      }
      const SelectInst *mass = nullptr;
      for (const auto &candidate : instructions(f))
        if (const auto *sel = dyn_cast<SelectInst>(&candidate))
          if (dom.dominates(x, sel) &&
              cell(a, comparison(sel->getCondition(), ICmpInst::ICMP_EQ, 0),
                   10, 3, ref, 4) && matches(sel->getFalseValue(), 3, next)) {
            need(!mass, "scoped response has multiple branch mass operands");
            mass = sel;
          }
      need(mass, "scoped response must use owning branch mass");
      bool contributes = false;
      for (const auto &candidate : instructions(f))
        if (const auto *mul = binary(&candidate, Instruction::FMul))
          for (int side = 0; side < 2; ++side)
            if (mul->getOperand(side) == x && depends_on(mul->getOperand(1 - side), mass)) {
              for (const auto &term : instructions(f))
                if (const auto *weight = binary(&term, Instruction::FMul))
                  contributes |= cell(a, weight->getOperand(0), 7, field::kMass,
                                      recipient, 8) &&
                                 exact_ratio(a, weight->getOperand(1), 11, ref) &&
                                 depends_on(mul, weight);
            }
      need(contributes, "scoped displacement and branch mass must feed the exact influence weight");
      need(!(count & (1 << next)), "duplicate scoped response in one influence");
      count |= 1 << next;
      paths.push_back(x);
    }
    need(eligibility && count == (wake ? 3 : 1),
         "missing scoped force or paired wake response");
  }
  need(!paths.empty(), "missing scoped response operands");
  return paths;
}

template <class Check, class Change>
void reject_control(const Module &original, const char *label,
                    const char *expected, Check check, Change change) {
  auto changed = CloneModule(original);
  change(*changed);
  need(!verifyModule(*changed, &errs()), "invalid scoped LLVM control");
  try {
    check(*changed);
  } catch (const std::runtime_error &e) {
    need(std::string(e.what()) == expected, e.what());
    std::cerr << "scoped_compiler_control," << label << ',' << e.what()
              << " rejected PASS\n";
    return;
  }
  throw std::runtime_error(std::string("scoped audit accepted ") + label);
}

void check_c3(const taichi::lang::Kernel &kernel, const Module &module, bool wake) {
  const int mode = wake ? 1 : 0;
  const auto audit = [&](Module &m) {
    Analyzer a(kernel, m, mode, wake);
    if (wake)
      unfold_poll_coordinates(m, a);
    const auto report = a.run();
    Analyzer abi(kernel, m, 5);
    if (wake)
      poll_rows(abi, report, scoped_rows(m, abi, false));
    return responses(abi, report, wake);
  };
  auto normalized = CloneModule(module);
  const auto paths = audit(*normalized);
  need(!verifyModule(*normalized, &errs()), "invalid scoped audit normalization");
  std::cerr << "scoped_compiler_audit," << (wake ? "poll" : "force")
            << ",response_paths=" << paths.size() << " PASS\n";
  if (wake) {
    reject_control(module, "shift-scoped-source", "scoped child must decode its own source row and exact ratio",
        [&](Module &m) { audit(m); }, [&](Module &m) {
          Analyzer a(kernel, m, 5);
          auto *body = const_cast<LoadInst *>(scoped_rows(m, a, false).front().body);
          IRBuilder<> b(body);
          body->setOperand(0, b.CreateGEP(body->getType(), body->getPointerOperand(),
                                          b.getInt64(1)));
        });
  } else {
    reject_control(module, "omit-scoped-response", "missing scoped force or paired wake response",
        [&](Module &m) { audit(m); }, [&](Module &m) {
          auto *x = const_cast<SelectInst *>(audit(m).front());
          x->setTrueValue(x->getFalseValue());
        });
  }
}
}  // namespace

void scoped_audit_no_unroll() {
  const char *options[] = {"c3-compiler-control", "-unroll-count=1",
                           "-unroll-max-count=1", "-unroll-full-max-count=1"};
  llvm::cl::ParseCommandLineOptions(4, options);
}

void audit_scoped_hierarchy(field::Harness &harness) {
  const auto c3 = field::testing::LiveHierarchyAccess::kernels(harness);
  const auto refresh = field::testing::LiveHierarchyAccess::refresh_kernels(harness);
  const auto *directory = std::getenv("HCP_SCOPED_IR_DIR");
  check_c3(engine::testing::CompiledKernelAccess::kernel(*c3[0]),
           engine::testing::CompiledKernelAccess::module(*c3[0]), false);
  check_c3(engine::testing::CompiledKernelAccess::kernel(*c3[1]),
           engine::testing::CompiledKernelAccess::module(*c3[1]), true);
  check_seed(engine::testing::CompiledKernelAccess::kernel(*refresh[0]),
             engine::testing::CompiledKernelAccess::module(*refresh[0]));
  if (!directory) return;
  const std::array kernels{c3[0], c3[1], refresh[0]};
  const std::array names{"force", "poll", "seed"};
  for (std::size_t i = 0; i < kernels.size(); ++i) {
    std::error_code error;
    llvm::raw_fd_ostream file(std::string(directory) + "/" + names[i] + ".ll",
                              error, llvm::sys::fs::OF_Text);
    if (error)
      throw std::runtime_error("cannot retain scoped IR");
    engine::testing::CompiledKernelAccess::module(*kernels[i]).print(file, nullptr);
  }
}
