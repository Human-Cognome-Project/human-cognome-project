#include <algorithm>
#include <array>
#include <cmath>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <set>
#include <map>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
#ifdef HCP_HAS_VIEWPORT
#include <filesystem>
#include "snapshot.h"
#include "capture_options.h"
#endif
#include "allocation_observation.h"
#include "prepared_fixture.h"
#include "prepared_metrics.h"
#include "field/live_hierarchy.h"
#include "assembly/study.h"
#include "membership_fixture.h"
#include "taichi/program/kernel.h"
#include "taichi/codegen/llvm/compiled_kernel_data.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Operator.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "taichi/ir/type_factory.h"

namespace engine::testing {
struct CompiledKernelAccess {
  static const taichi::lang::Kernel &kernel(const CompiledKernel &k) {
    return *k.kernel_;
  }
  static const taichi::lang::CompiledKernelData &compiled(
      const CompiledKernel &k) {
    return *k.compiled_;
  }
};
}  // namespace engine::testing
namespace field::testing {
struct LiveHierarchyAccess {
  static auto kernels(Harness &h) {
    return h.live_hierarchy_->kernels();
  }
  static std::size_t fixed_bytes(Harness &h) {
    return h.live_hierarchy_->fixed_object_bytes();
  }
  static void force(Harness &h, const Parameters64 &p) {
    h.activity_->begin(h, true, true, p.reach_strength != 0);
    h.launch_clear(false);
    h.live_hierarchy_->force(p.gravity);
    h.runtime_->synchronize();
  }
  static void poll(Harness &h,
                   double gravity,
                   int target = -1,
                   double stiffness = 0) {
    h.live_hierarchy_->poll_target(gravity, target, stiffness);
    h.runtime_->synchronize();
  }
  static auto particle_set(Harness &h) {
    return h.activity_->particle_activity;
  }
  static auto field_set(Harness &h) {
    return h.activity_->centroids;
  }
  static void invalidate(Harness &h) {
    h.invalidated_ = true;
  }
  static void mass_total(Harness &h, double mass) {
    h.universal_values_[0] = mass;
    if (h.path_ == HostPath::BulkCopy)
      engine::upload(*h.universal_array_, h.universal_values_.data(),
                     sizeof(double));
    h.runtime_->synchronize();
  }
};
}  // namespace field::testing
namespace {
using namespace field;
using namespace field::testing;
void require(bool v, const char *s) {
  if (!v)
    throw std::runtime_error(s);
}
void near(double x, double y, const char *s) {
  if (std::abs(x - y) > 1e-11 * std::max(1., std::abs(y))) {
    std::cerr << s << ": " << std::setprecision(17) << x << " expected " << y
              << '\n';
    throw std::runtime_error(s);
  }
}
void audit(const engine::CompiledKernel &compiled,
           int mode,
           const llvm::Module *replacement = nullptr,
           bool verbose = true) {
  using namespace taichi::lang;
  const auto &kernel = engine::testing::CompiledKernelAccess::kernel(compiled);
  const auto &retained =
      *dynamic_cast<const LLVM::CompiledKernelData &>(
           engine::testing::CompiledKernelAccess::compiled(compiled))
           .get_internal_data()
           .compiled_data.module;
  const auto &module = replacement ? *replacement : retained;
  if (verbose)
    std::cout << "production_module,name=" << kernel.get_name()
              << ",mode=" << mode << '\n';
  if (const char *dir = verbose ? std::getenv("HCP_C3_IR_DIR") : nullptr) {
    std::error_code ec;
    llvm::raw_fd_ostream file(
        std::string(dir) + "/c3-" + std::to_string(mode) + ".ll", ec,
        llvm::sys::fs::OF_Text);
    require(!ec, "IR diagnostic write");
    module.print(file, nullptr);
  }
  const auto data_arg = [&](const llvm::Value *ptr) {
    const auto *base =
        llvm::dyn_cast<llvm::LoadInst>(llvm::getUnderlyingObject(ptr));
    if (!base || !base->getType()->isPointerTy())
      return -1;
    std::int64_t offset = 0;
    const auto *buffer =
        llvm::dyn_cast<llvm::LoadInst>(llvm::GetPointerBaseWithConstantOffset(
            base->getPointerOperand(), offset, module.getDataLayout()));
    if (!buffer)
      return -1;
    std::int64_t context_offset = 0;
    const auto *context =
        llvm::dyn_cast<llvm::Argument>(llvm::GetPointerBaseWithConstantOffset(
            buffer->getPointerOperand(), context_offset,
            module.getDataLayout()));
    if (!context || context->getArgNo() != 0 || context_offset != 0 ||
        context->getParent() != base->getFunction())
      return -1;
    for (int id = 0; id < 30; ++id)
      if (id < 24 || id >= 28)
        if (offset == kernel.args_type->get_element_offset(
                          {id, TypeFactory::DATA_PTR_POS_IN_NDARRAY}))
          return id;
    throw std::runtime_error(
        "unattributed ndarray access in production kernel");
  };
  const auto roots = [&](const llvm::Value *value) {
    std::set<const llvm::Value *> seen;
    std::set<int> result;
    const auto visit = [&](const auto &self, const llvm::Value *v) -> void {
      if (!seen.insert(v).second)
        return;
      if (const auto *load = llvm::dyn_cast<llvm::LoadInst>(v)) {
        int id = data_arg(load->getPointerOperand());
        if (id == 3 || id == 10 || id == 18)
          result.insert(id);
      }
      if (const auto *u = llvm::dyn_cast<llvm::User>(v))
        for (const auto &op : u->operands())
          self(self, op.get());
    };
    visit(visit, value);
    return result;
  };
  const auto response_column = [&](const llvm::Value *value) {
    std::map<const llvm::Value *, long long> memo;
    std::set<const llvm::Value *> visiting;
    const auto coeff = [&](const auto &self,
                           const llvm::Value *v) -> long long {
      if (memo.count(v))
        return memo[v];
      if (!visiting.insert(v).second)
        return 0;
      long long result = 0;
      if (const auto *load = llvm::dyn_cast<llvm::LoadInst>(v)) {
        std::int64_t offset = 0;
        const auto *buffer = llvm::dyn_cast<llvm::LoadInst>(
            llvm::GetPointerBaseWithConstantOffset(
                load->getPointerOperand(), offset, module.getDataLayout()));
        if (buffer &&
            offset == kernel.args_type->get_element_offset(
                          {10, TypeFactory::SHAPE_POS_IN_NDARRAY, 1})) {
          std::int64_t context_offset = 0;
          if (llvm::isa<llvm::Argument>(llvm::GetPointerBaseWithConstantOffset(
                  buffer->getPointerOperand(), context_offset,
                  module.getDataLayout())) &&
              context_offset == 0)
            result = 1;
        }
      } else if (const auto *cast = llvm::dyn_cast<llvm::CastInst>(v))
        result = self(self, cast->getOperand(0));
      else if (const auto *gep = llvm::dyn_cast<llvm::GEPOperator>(v)) {
        result = self(self, gep->getPointerOperand());
        const auto bytes = module.getDataLayout()
                               .getTypeAllocSize(gep->getSourceElementType())
                               .getFixedValue();
        for (const auto &index : gep->indices())
          result += bytes * self(self, index.get());
      } else if (const auto *op = llvm::dyn_cast<llvm::BinaryOperator>(v)) {
        if (op->getOpcode() == llvm::Instruction::Add)
          result =
              self(self, op->getOperand(0)) + self(self, op->getOperand(1));
        if (op->getOpcode() == llvm::Instruction::Sub)
          result =
              self(self, op->getOperand(0)) - self(self, op->getOperand(1));
        if (op->getOpcode() == llvm::Instruction::Mul) {
          if (const auto *c =
                  llvm::dyn_cast<llvm::ConstantInt>(op->getOperand(0)))
            result = c->getSExtValue() * self(self, op->getOperand(1));
          else if (const auto *c =
                       llvm::dyn_cast<llvm::ConstantInt>(op->getOperand(1)))
            result = c->getSExtValue() * self(self, op->getOperand(0));
        }
        if (op->getOpcode() == llvm::Instruction::Shl)
          if (const auto *c =
                  llvm::dyn_cast<llvm::ConstantInt>(op->getOperand(1)))
            result = self(self, op->getOperand(0)) * (1LL << c->getZExtValue());
      }
      visiting.erase(v);
      return memo[v] = result;
    };
    std::set<int> columns;
    std::set<const llvm::Value *> seen;
    const auto visit = [&](const auto &self, const llvm::Value *v) -> void {
      if (!seen.insert(v).second)
        return;
      if (const auto *load = llvm::dyn_cast<llvm::LoadInst>(v))
        if (data_arg(load->getPointerOperand()) == 10) {
          const auto bytes = coeff(coeff, load->getPointerOperand());
          require(bytes % 4 == 0, "non-integral response array column");
          columns.insert(int(bytes / 4));
          return;
        }
      if (const auto *u = llvm::dyn_cast<llvm::User>(v))
        for (const auto &op : u->operands())
          self(self, op.get());
    };
    visit(visit, value);
    require(columns.size() == 1, "response selector column is ambiguous");
    return *columns.begin();
  };
  int own = 0, child = 0, response = 0, pubs = 0, recipient_child = 0,
      recipient_parent = 0;
  const llvm::Value *own_ptr = nullptr, *pub_ptr = nullptr;
  std::map<const llvm::AtomicRMWInst *, int> packet_loads;
  for (const auto &fn : module)
    for (const auto &bb : fn)
      for (const auto &inst : bb) {
        if (const auto *l = llvm::dyn_cast<llvm::LoadInst>(&inst))
          require(data_arg(l->getPointerOperand()) != 1,
                  "plain production selector load");
        if (const auto *s = llvm::dyn_cast<llvm::StoreInst>(&inst))
          require(data_arg(s->getPointerOperand()) != 1,
                  "plain production selector store");
        if (const auto *a = llvm::dyn_cast<llvm::AtomicRMWInst>(&inst))
          if (data_arg(a->getPointerOperand()) == 1) {
            require(a->getOrdering() ==
                        llvm::AtomicOrdering::SequentiallyConsistent,
                    "production selector lost SC");
            const auto *v =
                llvm::dyn_cast<llvm::ConstantInt>(a->getValOperand());
            require(v && v->getType()->isIntegerTy(32),
                    "invalid production selector operand");
            const auto ids = roots(a->getPointerOperand());
            std::string role;
            if (a->getOperation() == llvm::AtomicRMWInst::Xor &&
                v->equalsInt(1)) {
              ++pubs;
              role = "publish";
              pub_ptr = a->getPointerOperand();
              require(ids == std::set<int>{18}, "publication owner path");
            } else {
              require(
                  a->getOperation() == llvm::AtomicRMWInst::Or && v->isZero(),
                  "unattributed production selector operation");
              if (ids.count(10)) {
                ++response;
                const int col = response_column(a->getPointerOperand());
                require(col == 1 || col == 2,
                        "wrong response child/parent index column");
                if (col == 1) {
                  ++recipient_child;
                  role = "response_child";
                } else {
                  ++recipient_parent;
                  role = "response_parent";
                }
              } else if (ids.count(3)) {
                ++child;
                role = "child";
              } else if (ids == std::set<int>{18}) {
                ++own;
                role = "own";
                own_ptr = a->getPointerOperand();
              } else
                throw std::runtime_error(
                    "unknown production acquire index path");
              packet_loads[a] = 0;
            }
            std::string text;
            llvm::raw_string_ostream os(text);
            a->print(os);
            if (verbose)
              std::cout << "production_selector_ir,kernel=" << mode
                        << ",role=" << role << "," << os.str() << '\n';
          }
      }
  if (mode != 2)
    for (const auto &fn : module)
      for (const auto &bb : fn)
        for (const auto &inst : bb)
          if (const auto *load = llvm::dyn_cast<llvm::LoadInst>(&inst))
            if (data_arg(load->getPointerOperand()) == 0) {
              std::set<const llvm::Value *> seen;
              std::set<const llvm::AtomicRMWInst *> selectors;
              const auto visit = [&](const auto &self,
                                     const llvm::Value *v) -> void {
                if (!seen.insert(v).second)
                  return;
                if (const auto *a = llvm::dyn_cast<llvm::AtomicRMWInst>(v))
                  if (data_arg(a->getPointerOperand()) == 1) {
                    selectors.insert(a);
                    return;
                  }
                if (const auto *u = llvm::dyn_cast<llvm::User>(v))
                  for (const auto &op : u->operands())
                    self(self, op.get());
              };
              visit(visit, load->getPointerOperand());
              require(selectors.size() == 1,
                      "publication word has no unique acquired selector");
              auto *a = *selectors.begin();
              require(packet_loads.count(a),
                      "word uses a non-acquire selector");
              ++packet_loads[a];
            }
  for (const auto &entry : packet_loads) {
    require(entry.second == 4,
            "acquired production packet does not have four attributed loads");
    if (verbose)
      std::cout << "production_packet_audit,kernel=" << mode
                << ",words=" << entry.second << " PASS\n";
  }
  if (mode == 0)
    require(recipient_child == 3 && recipient_parent == 3 && !own && !child &&
                !pubs,
            "missing force production reader clone/epilogue");
  if (mode == 1)
    require(own == 1 && child == 5 && recipient_child == 1 &&
                recipient_parent == 1 && pubs == 1 && own_ptr == pub_ptr,
            "missing production poll reader/writer clone/epilogue");
  if (mode == 2)
    require(!own && !child && !response && !pubs,
            "control unexpectedly uses selectors");
  if (const char *dir = verbose ? std::getenv("HCP_C3_IR_DIR") : nullptr) {
    std::error_code ec;
    llvm::raw_fd_ostream file(
        std::string(dir) + "/c3-" + std::to_string(mode) + ".ll", ec,
        llvm::sys::fs::OF_Text);
    require(!ec, "IR diagnostic write");
    module.print(file, nullptr);
  }
  if (verbose)
    std::cout << "production_selector_audit,kernel=" << mode << ",own=" << own
              << ",child=" << child << ",response=" << response
              << ",publish=" << pubs << ",plain=0 PASS\n";
}
void audit_negative(const engine::CompiledKernel &compiled) {
  using namespace taichi::lang;
  const auto &original =
      *dynamic_cast<const LLVM::CompiledKernelData &>(
           engine::testing::CompiledKernelAccess::compiled(compiled))
           .get_internal_data()
           .compiled_data.module;
  for (int control = 0; control < 3; ++control) {
    auto module = llvm::CloneModule(original);
    llvm::AtomicRMWInst *reader = nullptr;
    for (auto &fn : *module)
      for (auto &bb : fn)
        for (auto &inst : bb)
          if (auto *a = llvm::dyn_cast<llvm::AtomicRMWInst>(&inst))
            if (a->getOperation() == llvm::AtomicRMWInst::Or &&
                a->getType()->isIntegerTy(32))
              if (auto *c =
                      llvm::dyn_cast<llvm::ConstantInt>(a->getValOperand()))
                if (c->isZero())
                  reader = a;
    require(reader, "negative selector audit has no reader");
    if (control == 0)
      reader->setOrdering(llvm::AtomicOrdering::Monotonic);
    else {
      llvm::Value *value =
          control == 1 ? static_cast<llvm::Value *>(new llvm::LoadInst(
                             reader->getType(), reader->getPointerOperand(),
                             "unsafe_plain", reader))
                       : static_cast<llvm::Value *>(
                             llvm::ConstantInt::get(reader->getType(), 0));
      reader->replaceAllUsesWith(value);
      reader->eraseFromParent();
    }
    bool rejected = false;
    try {
      audit(compiled, 0, module.get(), false);
    } catch (const std::runtime_error &) {
      rejected = true;
    }
    require(rejected, "production audit accepted demoted/plain/missing reader");
    std::cout << "compiler_negative,control=" << control << " rejected PASS\n";
  }
}
template <class F>
void no_new(F f) {
  allocation_observation::calls = 0;
  allocation_observation::enabled = true;
  try {
    f();
  } catch (...) {
    allocation_observation::enabled = false;
    throw;
  }
  allocation_observation::enabled = false;
  require(!allocation_observation::calls,
          "live hierarchy boundary allocated C++ metadata");
}
BudgetDemand limits(int responses = 1024) {
  BudgetDemand l;
  l[Budget::Potential] = 1048576;
  l[Budget::Particles] = 70;
  l[Budget::Fields] = 70;
  l[Budget::Participation] = 512;
  l[Budget::Contacts] = 70;
  l[Budget::Focus] = 70;
  l[Budget::Batch] = 70;
  l[Budget::References] = 10000;
  l[Budget::CascadeNodes] = 70;
  l[Budget::CascadeLinks] = 140;
  l[Budget::CascadeReferences] = 512;
  l[Budget::CascadeLevels] = 70;
  l[Budget::CascadeWork] = 10000000;
  l[Budget::CascadeCoverage] = 512;
  l[Budget::LiveResponses] = responses;
  l[Budget::GeometryWork] = 1000000;
  l[Budget::ScratchBytes] = detail::PreparedStorage::required_scratch(
      l, false, NumericalProfile::trial());
  return l;
}
struct Controls {
  engine::CompiledKernel kernel;
  explicit Controls(engine::Runtime &r, Harness &h) {
    using namespace taichi::lang;
    IRBuilder b;
    auto *op = b.create_arg_load({0}, PrimitiveType::i32, false, 0);
    auto *row = b.create_arg_load({1}, PrimitiveType::i32, false, 0);
    const auto particles = LiveHierarchyAccess::particle_set(h),
               fields = LiveHierarchyAccess::field_set(h);
    for (auto set : {particles, fields}) {
      auto *loop = b.create_struct_for(set.cells, false, r.cpu_threads());
      auto scope = b.get_loop_guard(loop);
      auto clear = b.get_if_guard(
          b.create_if(b.create_logical_or(
              b.create_cmp_eq(op, b.get_int32(0)),
              b.create_logical_and(b.create_cmp_eq(op, b.get_int32(4)),
                                   b.get_int32(set.cells == particles.cells)))),
          true);
      set.deactivate(b, b.get_loop_index(loop));
    }
    for (int which = 1; which <= 2; ++which) {
      auto selected = b.get_if_guard(
          b.create_if(b.create_cmp_eq(op, b.get_int32(which))), true);
      (which == 1 ? fields : particles).activate(b, row);
    }
    {
      auto *loop =
          b.create_struct_for(h.prepared_storage().fields.bindings().presence,
                              false, r.cpu_threads());
      auto scope = b.get_loop_guard(loop);
      auto all = b.get_if_guard(
          b.create_if(b.create_cmp_eq(op, b.get_int32(3))), true);
      fields.activate(b, b.get_loop_index(loop));
    }
    kernel = engine::CompiledKernel(
        r, "live_control_" + std::to_string(fields.cells->id), b.extract_ir(),
        {engine::Param::scalar(PrimitiveType::i32),
         engine::Param::scalar(PrimitiveType::i32)});
    set(0, 0);
    r.synchronize();
  }
  void set(int op, int row = 0) {
    auto call = kernel.invocation(true);
    auto &c = call.get();
    engine::argument_int(c, 0, op);
    engine::argument_int(c, 1, row);
    kernel.launch(c);
  }
};
struct Fixture {
  const std::chrono::steady_clock::time_point setup_start =
      std::chrono::steady_clock::now();
  engine::Runtime &runtime;
  Harness h;
  PreparedFixtureAccess::Input in{70, 70, 70, 70};
  std::vector<CascadeNode> nodes;
  std::vector<int> children;
  std::vector<CascadeContribution> refs;
  std::unique_ptr<Controls> control;
  std::vector<std::uint64_t> denominators;
  std::vector<std::vector<std::uint64_t>> coverage;
  std::vector<std::vector<int>> adjacency;
  std::vector<int> parents;
  std::vector<int> reference_active;
  std::vector<long double> force_condition;
  Fixture(engine::Runtime &r,
          HostPath path,
          std::vector<std::vector<int>> edges,
          std::vector<std::vector<std::uint64_t>> count,
          std::vector<std::uint64_t> denominators,
          std::vector<double> mass,
          std::vector<double> positions,
          int body_bits = 52,
          int field_bits = 52,
          bool repeated = false,
          bool contact = false,
          bool focus = false,
          bool permute = false,
          int response_capacity = 1024,
          const std::vector<double> &radii = {})
      : runtime(r),
        h(r,
          PreparedCapacity(limits(response_capacity),
                           NumericalProfile::trial(body_bits, field_bits)),
          path) {
    FixtureSource src;
    src.kinds.assign(edges.size(), 1);
    src.exact_masses = denominators;
    for (std::size_t i = 0; i < mass.size(); ++i) {
      std::array<float, kParticleFieldCount> p{};
      p[kMass] = float(mass[i]);
      p[kPosX] = float(positions[i]);
      p[kRadius] = radii.empty() ? .125f : float(radii[i]);
      src.particles.push_back(p);
    }
    if (contact)
      src.contacts.push_back({0, 1});
    if (focus)
      src.focus.push_back(
          permute ? int(((edges.size() - 1) * 37 + 13) % edges.size())
                  : int(edges.size() - 1));
    for (std::size_t i = 0; i < edges.size(); ++i) {
      if (!edges[i].empty()) {
        std::fill(count[i].begin(), count[i].end(), 0);
        for (auto c : edges[i])
          for (std::size_t j = 0; j < count[i].size(); ++j)
            count[i][j] += count[c][j];
      }
      for (std::size_t j = 0; j < count[i].size(); ++j)
        if (count[i][j]) {
          const auto field =
              permute ? int((i * 37 + 13) % edges.size()) : int(i);
          const auto add = [&](std::uint64_t c) {
            src.edges.push_back({int(j), field,
                                 float(double(c) / denominators[j]),
                                 src.edges.size(), c});
          };
          if (repeated && i == 0 && count[i][j] > 1) {
            add(1);
            add(count[i][j] - 1);
          } else
            add(count[i][j]);
        }
    }
    Parameters p;
    p.gravity = p.contact_stiffness = p.reach_strength = 0;
    no_new([&] { PreparedFixtureAccess::load(h, src, in, p, true); });
    const auto &s = h.prepared_storage();
    for (std::size_t i = 0; i < edges.size(); ++i) {
      CascadeNode n;
      n.field = in.field_rows[permute ? (i * 37 + 13) % edges.size() : i];
      n.comparison = i + 1;
      n.level = 0;
      n.retained_fraction_bits = field_bits;
      n.child_begin = children.size();
      n.child_count = edges[i].size();
      for (auto c : edges[i]) {
        children.push_back(c);
        n.level = std::max(n.level, nodes[c].level + 2);
      }
      n.cohort_begin = refs.size();
      for (std::size_t e = 0; e < s.live.edges; ++e)
        if (s.edges[e].second.record == n.field.record)
          refs.push_back(
              {s.edges[e].first, {s.generation, e, s.edges[e].ordinal}});
      n.cohort_count = refs.size() - n.cohort_begin;
      nodes.push_back(n);
    }
    this->denominators = denominators;
    coverage = count;
    adjacency = edges;
    parents.assign(nodes.size(), -1);
    for (std::size_t n = 0; n < edges.size(); ++n)
      for (int c : edges[n])
        parents[c] = int(n);
    control = std::make_unique<Controls>(r, h);
    std::cout << "fixture_setup,nodes=" << nodes.size()
              << ",bodies=" << this->denominators.size() << ",elapsed_ms="
              << std::chrono::duration<double, std::milli>(
                     std::chrono::steady_clock::now() - setup_start)
                     .count()
              << '\n';
  }
  WeightedCascadeSpec spec() const {
    return {nodes.data(),    nodes.size(), children.data(),
            children.size(), refs.data(),  refs.size()};
  }
  void install() {
    const auto start = std::chrono::steady_clock::now();
    LiveHierarchyResult result;
    no_new([&] {
      result =
          h.install_live_hierarchy(h.prepared_storage().generation, spec());
    });
    if (result.status != CascadeStatus::Ok) {
      std::cerr << "install status " << int(result.status) << " budget "
                << int(result.failure.budget) << " need "
                << result.failure.required << '\n';
      throw std::runtime_error("installation failed");
    }
    std::cout << "installed,nodes=" << result.nodes
              << ",responses=" << result.responses
              << ",work=" << result.preparation_visits
              << ",reference_work=" << result.reference_work
              << ",upload_bytes=" << result.upload_bytes
              << ",readback_bytes=" << result.readback_bytes
              << ",synchronizations=" << result.synchronizations
              << ",elapsed_ms="
              << std::chrono::duration<double, std::milli>(
                     std::chrono::steady_clock::now() - start)
                     .count()
              << '\n';
    int mode = 0;
    for (auto *k : LiveHierarchyAccess::kernels(h))
      audit(*k, mode++);
  }
};
void collect_compiler_ir(engine::Runtime &r, HostPath path) {
  const char *dir = std::getenv("HCP_C3_IR_DIR");
  require(dir && *dir, "IR collector requires HCP_C3_IR_DIR");
  Fixture f(r, path, {{}, {}, {0, 1}}, {{1, 1, 0}, {0, 0, 1}, {0, 0, 0}},
            {1, 1, 1}, {1, 1, 2}, {0, 2, 5});
  const auto result =
      f.h.install_live_hierarchy(f.h.prepared_storage().generation, f.spec());
  require(result.status == CascadeStatus::Ok,
          "IR collector installation failed");
  std::ofstream metadata(std::string(dir) + "/configuration.txt");
  require(bool(metadata), "IR collector metadata write");
  const auto &cfg = r.program().compile_config();
  metadata << "diagnostic_only=1; semantic acceptance is not performed by this "
              "collector\n"
           << "actual_backend=x64\nworkers=" << r.cpu_threads()
           << "\nsource_path="
           << (path == HostPath::BulkCopy ? "bulk" : "direct")
           << "\njit_opt_level=" << cfg.opt_level
           << "\njit_simd_width=" << cfg.simd_width
           << "\nkernel_math=strict (C3 construction)\n";
  int mode = 0;
  for (const auto *compiled : LiveHierarchyAccess::kernels(f.h)) {
    const auto &kernel =
        engine::testing::CompiledKernelAccess::kernel(*compiled);
    const auto &module =
        *dynamic_cast<const taichi::lang::LLVM::CompiledKernelData &>(
             engine::testing::CompiledKernelAccess::compiled(*compiled))
             .get_internal_data()
             .compiled_data.module;
    std::error_code ec;
    llvm::raw_fd_ostream file(
        std::string(dir) + "/c3-" + std::to_string(mode) + ".ll", ec,
        llvm::sys::fs::OF_Text);
    require(!ec, "IR collector module write");
    module.print(file, nullptr);
    metadata << "mode=" << mode << ",kernel=" << kernel.get_name()
             << ",triple=" << module.getTargetTriple()
             << ",layout=" << module.getDataLayoutStr() << '\n';
    for (int arg = 0; arg < 30; ++arg)
      if (arg < 24 || arg >= 28)
        metadata << "argument=" << arg << ",data_offset="
                 << kernel.args_type->get_element_offset(
                        {arg,
                         taichi::lang::TypeFactory::DATA_PTR_POS_IN_NDARRAY})
                 << '\n';
    ++mode;
  }
  std::cout << "compiler_ir_collection,modules=3,diagnostic_only=1,no_semantic_"
               "acceptance_claim=1\n";
}

using Packet = std::array<double, 4>;
using Vector = std::array<long double, 3>;
double quantize(double value, int bits) {
  if (value == 0)
    return value;
  int exponent;
  const auto x =
      std::frexp(std::abs(static_cast<long double>(value)), &exponent);
  return std::copysign(double(std::ldexp(std::trunc(std::ldexp(x, bits + 1)),
                                         exponent - bits - 1)),
                       value);
}
Packet selected(const LiveHierarchyInspection &v, int n) {
  const auto h = v.selectors.size();
  Packet p{};
  for (int k = 0; k < 4; ++k)
    p[k] = v.slots[(v.selectors[n] * 4 + k) * h + n];
  return p;
}
Packet fold(const std::vector<Packet> &inputs, int bits) {
  require(!inputs.empty(), "empty reference fold");
  std::array<long double, 4> sum{};
  for (auto p : inputs) {
    for (int k = 0; k < 3; ++k)
      sum[k] += static_cast<long double>(p[3]) * p[k];
    sum[3] += p[3];
  }
  Packet p{};
  p[3] = double(sum[3]);
  for (int k = 0; k < 3; ++k)
    p[k] = quantize(double(sum[k] / sum[3]), bits);
  return p;
}
bool packet_near(const Packet &a, const Packet &b, int bits) {
  for (int k = 0; k < 4; ++k) {
    if (k < 3 && bits < 52 && a[k] != b[k])
      return false;
    if (std::abs(a[k] - b[k]) > 4e-12 * std::max(1., std::abs(b[k])))
      return false;
  }
  return true;
}
Packet leaf_reference(Fixture &f, int n) {
  std::vector<Packet> inputs;
  const auto &node = f.nodes[n];
  const auto &s = f.h.prepared_storage();
  for (std::size_t j = node.cohort_begin;
       j < node.cohort_begin + node.cohort_count; ++j) {
    const auto &ref = f.refs[j];
    int body = ref.occurrence.record;
    Packet p{};
    for (int k = 0; k < 3; ++k)
      p[k] = f.h.particle64(kPosX + k, body);
    p[3] = double(static_cast<long double>(f.h.particle64(kMass, body)) *
                  s.source_counts[ref.source.row] / s.source_masses[body]);
    inputs.push_back(p);
  }
  return fold(inputs, node.retained_fraction_bits);
}
void hierarchy_reference(Fixture &f) {
  const auto &v = f.h.inspect_live_hierarchy();
  std::vector<Packet> expected;
  for (std::size_t n = 0; n < f.nodes.size(); ++n) {
    if (f.adjacency[n].empty())
      expected.push_back(leaf_reference(f, int(n)));
    else {
      std::vector<Packet> inputs;
      for (int c : f.adjacency[n])
        inputs.push_back(expected[c]);
      expected.push_back(fold(inputs, f.nodes[n].retained_fraction_bits));
    }
    require(
        packet_near(selected(v, int(n)), expected[n],
                    f.nodes[n].retained_fraction_bits),
        "settled hierarchy differs from independent published-child reference");
    for (int k = 0; k < 4; ++k)
      near(f.h.group64(k, f.nodes[n].field.record), selected(v, int(n))[k],
           "flat mirror differs from selected packet");
  }
}
void poll_lifetime(Fixture &f, const LiveHierarchyInspection &before) {
  const auto &after = f.h.inspect_live_hierarchy();
  const int H = after.selectors.size();
  for (std::size_t n = 0; n < f.nodes.size(); ++n) {
    const auto visits = after.visits[n] - before.visits[n],
               pubs = after.visits[H + n] - before.visits[H + n];
    require(visits >= 0 && visits <= 1 && pubs >= 0 && pubs <= visits,
            "multiple node writers/publications in one poll");
    require((before.selectors[n] ^ after.selectors[n]) == pubs,
            "selector/publication mismatch");
    for (int k = 0; k < 4; ++k)
      require(before.slots[(before.selectors[n] * 4 + k) * H + n] ==
                  after.slots[(before.selectors[n] * 4 + k) * H + n],
              "old slot overwritten before poll readers finished");
    if (pubs)
      for (int k = 0; k < 4; ++k)
        require(after.previous[k * H + n] == selected(before, int(n))[k],
                "old complete publication history lost");
    if (!visits)
      continue;
    const auto &inspection = f.h.prepared_storage();
    const int group = f.nodes[n].field.record, G = f.h.field_capacity();
    require(
        inspection.inspection_state[2 * f.h.particle_capacity() +
                                    detail::Flagged * G + group] == 1 &&
            inspection.inspection_state[2 * f.h.particle_capacity() +
                                        detail::Changed * G + group] == pubs,
        "last-evaluation flags do not match installed publication");
    const auto actual = selected(after, int(n));
    if (f.adjacency[n].empty())
      require(packet_near(actual, leaf_reference(f, int(n)),
                          f.nodes[n].retained_fraction_bits),
              "live leaf reduction/reference mismatch");
    else {
      require(f.adjacency[n].size() <= 8, "reference combination bound");
      bool complete = false;
      for (unsigned mask = 0; mask < (1u << f.adjacency[n].size()); ++mask) {
        std::vector<Packet> inputs;
        int k = 0;
        for (int c : f.adjacency[n])
          inputs.push_back(selected(mask & (1u << k++) ? after : before, c));
        complete |=
            packet_near(actual, fold(inputs, f.nodes[n].retained_fraction_bits),
                        f.nodes[n].retained_fraction_bits);
      }
      require(complete,
              "parent did not use complete pre/post-poll child packets");
    }
  }
}
Vector attraction(long double weight,
                  const Packet &field,
                  const Vector &position,
                  double gravity) {
  Vector delta{}, f{};
  long double d2 = 0;
  for (int k = 0; k < 3; ++k) {
    delta[k] = field[k] - position[k];
    d2 += delta[k] * delta[k];
  }
  if (d2 == 0 || gravity == 0)
    return f;
  const auto scale = gravity * weight * field[3] / (d2 * std::sqrt(d2));
  for (int k = 0; k < 3; ++k)
    f[k] = scale * delta[k];
  return f;
}
std::vector<double> physical_reference(Fixture &f,
                                       const Parameters64 &p,
                                       bool invalidate) {
  const auto &state = f.h.prepared_storage();
  const auto &v = f.h.inspect_live_hierarchy();
  auto expected = f.h.particles64();
  const int P = f.h.particle_capacity();
  f.reference_active.assign(P, 0);
  f.force_condition.assign(3 * P, 0);
  Vector origin{};
  long double focal_mass = 0;
  for (std::size_t j = 0; j < state.live.focus; ++j) {
    int field = state.focal[j].record;
    const auto m = f.h.group64(kCenM, field);
    focal_mass += m;
    for (int k = 0; k < 3; ++k)
      origin[k] += m * f.h.group64(kCenX + k, field);
  }
  for (int k = 0; k < 3; ++k) {
    if (focal_mass)
      origin[k] /= focal_mass;
    else
      origin[k] = -f.h.frame_anchor()[k];
    if (p.reach_strength != 0 &&
        double(origin[k]) != state.inspection_focus64[k])
      invalidate = true;
  }
  for (std::size_t i = 0; i < f.denominators.size(); ++i) {
    const int body = f.in.particle_rows[i].record;
    if (!invalidate && !(state.inspection_flags[body] & 2))
      continue;
    f.reference_active[body] = 1;
    Vector force{}, position{}, velocity{};
    long double physical_mass = f.h.particle64(kMass, body);
    for (int k = 0; k < 3; ++k) {
      position[k] = f.h.particle64(kPosX + k, body);
      velocity[k] = f.h.particle64(kVelX + k, body);
    }
    const auto add = [&](const Vector &value) {
      for (int k = 0; k < 3; ++k) {
        force[k] += value[k];
        f.force_condition[k * P + body] += std::abs(value[k]);
      }
    };
    for (std::size_t n = 0; n < f.nodes.size(); ++n) {
      const auto &node = f.nodes[n];
      if (f.adjacency[n].empty()) {
        int distinct = 0;
        for (auto c : f.coverage[n])
          distinct += c > 0;
        if (distinct > 1)
          for (std::size_t j = node.cohort_begin;
               j < node.cohort_begin + node.cohort_count; ++j)
            if (f.refs[j].occurrence.record == body) {
              const auto weight = physical_mass *
                                  state.source_counts[f.refs[j].source.row] /
                                  state.source_masses[body];
              add(attraction(weight, selected(v, int(n)), position, p.gravity));
            }
      }
      if (f.parents[n] >= 0 && f.coverage[n][i]) {
        int parent = f.parents[n], distinct = 0;
        for (auto c : f.coverage[parent])
          distinct += c > 0;
        if (distinct > 1 && f.adjacency[parent].size() > 1) {
          const auto cn = selected(v, int(n));
          Vector x{cn[0], cn[1], cn[2]};
          add(attraction(physical_mass * f.coverage[n][i] / f.denominators[i],
                         selected(v, parent), x, p.gravity));
        }
      }
    }
    for (int side = 0; side < 2; ++side)
      for (std::size_t j = 0; j < state.live.contacts; ++j) {
        const auto &pair = state.contacts[j];
        const int a = pair.first.record, b = pair.second.record;
        if ((side ? b : a) != body)
          continue;
        Vector delta{};
        long double d2 = 0;
        for (int k = 0; k < 3; ++k) {
          delta[k] =
              f.h.particle64(kPosX + k, b) - f.h.particle64(kPosX + k, a);
          d2 += delta[k] * delta[k];
        }
        if (d2 == 0)
          continue;
        const auto d = std::sqrt(d2),
                   overlap = std::max(0.L, static_cast<long double>(
                                               f.h.particle64(kRadius, a) +
                                               f.h.particle64(kRadius, b)) -
                                               d);
        for (int k = 0; k < 3; ++k) {
          const auto force_value =
              (side ? 1 : -1) * p.contact_stiffness * overlap * delta[k] / d;
          force[k] += force_value;
          f.force_condition[k * P + body] += std::abs(force_value);
        }
      }
    long double reach2 = 0, speed2 = 0;
    for (int k = 0; k < 3; ++k) {
      velocity[k] += force[k] * p.dt / physical_mass;
      reach2 += force[k] * force[k];
      speed2 += velocity[k] * velocity[k];
      expected[(kForceX + k) * P + body] = double(force[k]);
    }
    const auto reach = std::sqrt(reach2), travel = std::sqrt(speed2) * p.dt;
    const auto ratio = reach ? travel / reach : 0;
    const auto brake = std::exp(-ratio * ratio * ratio * ratio);
    for (auto &x : velocity)
      x *= brake;
    Vector radial{};
    long double d2 = 0;
    for (int k = 0; k < 3; ++k) {
      radial[k] = position[k] - origin[k];
      d2 += radial[k] * radial[k];
    }
    if (d2 > 0) {
      const auto d = std::sqrt(d2);
      long double vr = 0;
      for (int k = 0; k < 3; ++k) {
        radial[k] /= d;
        vr += velocity[k] * radial[k];
      }
      const auto damping = std::exp(
          -std::max(0.L, p.reach_strength * d2 / f.h.universal64() - 1));
      for (int k = 0; k < 3; ++k)
        velocity[k] -= std::max(0.L, vr) * (1 - damping) * radial[k];
    }
    for (int k = 0; k < 3; ++k) {
      expected[(kVelX + k) * P + body] = double(velocity[k]);
      expected[(kPosX + k) * P + body] =
          quantize(double(position[k] + velocity[k] * p.dt),
                   f.h.numerical_profile().body_fraction_bits);
    }
  }
  return expected;
}
std::uint64_t publication_boundaries = 0;
struct VelocityReference {
  Vector value{};
  long double bound{};
};
VelocityReference velocity_reference(const Vector &force,
                                     const Vector &position,
                                     const Vector &old,
                                     const Vector &origin,
                                     long double mass,
                                     long double universal,
                                     const Parameters64 &p) {
  Vector trial{};
  long double speed2 = 0, force2 = 0, scale = 1;
  for (int k = 0; k < 3; ++k) {
    const auto acceleration = force[k] * p.dt / mass;
    trial[k] = old[k] + acceleration;
    speed2 += trial[k] * trial[k];
    force2 += force[k] * force[k];
    scale += std::abs(old[k]) + std::abs(acceleration) + std::abs(trial[k]);
  }
  const auto ratio = force2 ? std::sqrt(speed2) * p.dt / std::sqrt(force2) : 0;
  const auto brake = std::exp(-ratio * ratio * ratio * ratio);
  for (auto &v : trial)
    v *= brake;
  Vector direction{};
  long double d2 = 0;
  for (int k = 0; k < 3; ++k) {
    direction[k] = position[k] - origin[k];
    d2 += direction[k] * direction[k];
  }
  if (d2) {
    const auto d = std::sqrt(d2);
    long double radial = 0;
    for (int k = 0; k < 3; ++k) {
      direction[k] /= d;
      radial += trial[k] * direction[k];
    }
    const auto damping =
        std::exp(-std::max(0.L, p.reach_strength * d2 / universal - 1));
    const auto factor = std::max(0.L, radial) * (1 - damping);
    scale += std::abs(factor);
    for (int k = 0; k < 3; ++k) {
      scale += std::abs(trial[k]);
      trial[k] -= factor * direction[k];
    }
  }
  // Bound against intermediate magnitudes: outward damping can subtract nearly
  // equal large values. This is reference error accounting, never a sleep gate.
  return {trial, 128 * std::numeric_limits<double>::epsilon() * scale};
}
void verify_physical(Fixture &f,
                     const std::vector<double> &expected,
                     const std::vector<double> &old,
                     const Parameters64 &p) {
  const int P = f.h.particle_capacity();
  for (const auto &body : f.in.particle_rows)
    if (body.context && body.record >= 0 &&
        std::size_t(body.record) <
            f.h.prepared_storage().particle_headers.size() &&
        f.h.prepared_storage().particle_headers[body.record].context) {
      long double speed2 = 0;
      if (f.reference_active[body.record]) {
        Vector force{}, position{}, velocity{}, origin{};
        for (int k = 0; k < 3; ++k) {
          force[k] = f.h.particle64(kForceX + k, body.record);
          const auto bound =
              64 * std::numeric_limits<double>::epsilon() *
              (f.nodes.size() + f.refs.size() + 1) *
              std::max(1.L, f.force_condition[k * P + body.record]);
          require(std::abs(force[k] -
                           expected[(kForceX + k) * P + body.record]) <= bound,
                  "combined force differs from independently bounded ordered "
                  "contribution reference");
          position[k] = old[(kPosX + k) * P + body.record];
          velocity[k] = old[(kVelX + k) * P + body.record];
          origin[k] = f.h.demand().focal_origin[k];
        }
        const auto step = velocity_reference(force, position, velocity, origin,
                                             old[kMass * P + body.record],
                                             f.h.universal64(), p);
        for (int k = 0; k < 3; ++k)
          require(std::abs(f.h.particle64(kVelX + k, body.record) -
                           step.value[k]) <= step.bound,
                  "one combined physical integration/reference");
      } else
        for (int k = kVelX; k < kParticleFieldCount; ++k)
          require(f.h.particle64(k, body.record) == old[k * P + body.record],
                  "inactive physical state changed");
      require(
          f.h.particle64(kMass, body.record) == old[kMass * P + body.record] &&
              f.h.particle64(kRadius, body.record) ==
                  old[kRadius * P + body.record],
          "tick changed physical mass/extent");
      for (int k = 0; k < 3; ++k) {
        volatile double displacement =
            f.h.particle64(kVelX + k, body.record) * p.dt;
        const auto required =
            quantize(old[(kPosX + k) * P + body.record] + displacement,
                     f.h.numerical_profile().body_fraction_bits);
        require(f.h.particle64(kPosX + k, body.record) == required,
                "position is not one Q of old position plus final physical "
                "velocity times dt");
        if (f.h.numerical_profile().body_fraction_bits < 52 &&
            required != expected[(kPosX + k) * P + body.record])
          ++publication_boundaries;
      }
      for (int k = 0; k < 3; ++k) {
        const auto x = f.h.particle64(kVelX + k, body.record);
        speed2 += static_cast<long double>(x) * x;
      }
      near(f.h.determined64()[body.record], double(std::sqrt(speed2)),
           "final speed published before consumption");
    }
}
std::uint64_t state_hash(Fixture &f) {
  f.h.inspect_prepared();
  std::uint64_t hash = 1469598103934665603ULL;
  const auto block = [&](const void *ptr, std::size_t bytes) {
    const auto *p = static_cast<const unsigned char *>(ptr);
    for (std::size_t i = 0; i < bytes; ++i) {
      hash ^= p[i];
      hash *= 1099511628211ULL;
    }
  };
  const auto vector = [&](const auto &v) {
    block(v.data(), v.size() * sizeof(v[0]));
  };
  const auto &s = f.h.prepared_storage();
  vector(f.h.particles64());
  vector(f.h.groups64());
  vector(f.h.determined64());
  vector(s.motion64());
  vector(s.inspection_state);
  vector(s.inspection_flags);
  vector(s.source_masses);
  vector(s.source_counts);
  vector(s.particle_headers);
  vector(s.field_headers);
  vector(s.edges);
  vector(s.contacts);
  vector(s.focal);
  block(&s.generation, sizeof(s.generation));
  block(&s.completed_ticks, sizeof(s.completed_ticks));
  block(&f.h.demand(), sizeof(Demand));
  block(s.inspection_focus64.data(), sizeof(s.inspection_focus64));
  const auto mass = f.h.universal64();
  block(&mass, sizeof(mass));
  if (f.h.live_hierarchy_installed()) {
    const auto &v = f.h.inspect_live_hierarchy();
    vector(v.slots);
    vector(v.previous);
    vector(v.selectors);
    vector(v.visits);
    block(v.counters.data(), sizeof(v.counters));
  }
  return hash;
}
void preinstall_rejections(Fixture &f) {
  const auto baseline = state_hash(f);
  const auto generation = f.h.prepared_storage().generation;
  const auto nodes = f.nodes;
  const auto refs = f.refs;
  const auto children = f.children;
  const auto reject = [&](const char *name, std::uint64_t gen,
                          std::uint64_t work = UINT64_MAX) {
    LiveHierarchyResult result;
    no_new([&] { result = f.h.install_live_hierarchy(gen, f.spec(), work); });
    require(result.status != CascadeStatus::Ok &&
                !f.h.live_hierarchy_installed() && f.h.ready(),
            "invalid installation accepted/damaged readiness");
    require(state_hash(f) == baseline,
            "installation rejection mutated committed state");
    std::cout << "installation_reject,name=" << name
              << ",status=" << int(result.status) << " PASS\n";
    f.nodes = nodes;
    f.refs = refs;
    f.children = children;
  };
  reject("stale_generation", generation + 1);
  f.nodes[1].field = f.nodes[0].field;
  reject("aliased_output", generation);
  ++f.nodes[0].field.incarnation;
  reject("recycled_field", generation);
  ++f.refs[0].source.generation;
  reject("stale_source", generation);
  ++f.refs[0].source.ordinal;
  reject("wrong_ordinal", generation);
  f.nodes[0].publish = false;
  reject("anonymous_grouping", generation);
  f.children[0] = 2;
  reject("cycle", generation);
  f.nodes[2].child_count = 1;
  reject("incomplete_root", generation);
  reject("work_limit", generation, 0);
  // Source row b is omitted from both its leaf and the root: accounting still
  // forms a valid candidate tree, but production ownership requires full
  // ledgers.
  f.refs.erase(f.refs.begin() + 1);
  --f.nodes[0].cohort_count;
  for (std::size_t n = 1; n < f.nodes.size(); ++n)
    --f.nodes[n].cohort_begin;
  const auto b = f.in.particle_rows[1];
  const auto first = f.nodes[2].cohort_begin;
  for (std::size_t j = first; j < f.refs.size(); ++j)
    if (f.refs[j].occurrence.record == b.record) {
      f.refs.erase(f.refs.begin() + j);
      --f.nodes[2].cohort_count;
      break;
    }
  reject("partial_owner_ledger", generation);
}
void installed_rejections(Fixture &f) {
  const auto baseline = state_hash(f),
             generation = f.h.prepared_storage().generation;
  UpdateCommand cmd;
  cmd.operation = UpdateOp::RetireParticle;
  cmd.first = UpdateRef::existing(f.in.particle_rows[0]);
  UpdateResult result;
  no_new([&] { result = f.h.update_prepared(generation, &cmd, 1); });
  require(result.status == UpdateStatus::Unsupported,
          "installed structural edit was not rejected");
  require(state_hash(f) == baseline,
          "unsupported installed update changed state");
  require(f.h.install_live_hierarchy(generation, f.spec()).status ==
              CascadeStatus::Unsupported,
          "installed replacement accepted before C3-2");
  require(state_hash(f) == baseline,
          "installed replacement rejection changed state");
  const auto reject = [&](auto action) {
    bool caught = false;
    try {
      action();
    } catch (const std::logic_error &) {
      caught = true;
    }
    require(caught && f.h.ready(),
            "unsupported publication failed after mutation");
    require(state_hash(f) == baseline,
            "unsupported publication changed installed state");
  };
  reject([&] { f.h.upload(); });
  reject([&] { f.h.publish({}); });
  reject([&] { f.h.seed_centroids({}); });
  reject([&] { f.h.set_focus({f.nodes.back().field.record}); });
  Parameters64 invalid;
  invalid.gravity = std::numeric_limits<double>::quiet_NaN();
  reject([&] { f.h.tick64(invalid); });
  std::cout << "installed_rejection,update_replace_upload_publish_seed_focus_"
               "invalid_tick PASS\n";
}
void reader_consistency(Fixture &f, bool resolved = true) {
  const auto before = state_hash(f),
             generation = f.h.prepared_storage().generation;
  const auto root =
      selected(f.h.inspect_live_hierarchy(), int(f.nodes.size() - 1));
  CascadeBinding binding;
  CascadeResult query;
  GeometryResult geometry;
  no_new([&] {
    binding = f.h.bind_weighted_centroid_cascade(generation, f.spec());
    query = f.h.query_centroid_cascade(generation);
    geometry = f.h.query_prepared_geometry(generation);
  });
  require(binding.status == CascadeStatus::Ok &&
              query.status == CascadeStatus::Ok &&
              geometry.status == GeometryStatus::Ok,
          "installed synchronized candidate/geometry reader rejected");
  for (int k = 0; k < 3; ++k) {
    if (resolved)
      near(query.centre[k], root[k],
           "resolved candidate differs from installed root");
    near(geometry.focal_point[k], root[k],
         "geometry did not read completed focal mirror");
  }
  near(query.mass, root[3], "candidate mass reference");
  near(geometry.focal_mass, root[3], "geometry focal mass");
  require(state_hash(f) == before,
          "candidate/geometry query mutated installed "
          "physical/publication/history/activity state");
  std::cout << "reader_consistency,candidate_query=1,geometry=1,coherent_"
               "mirror=1,source_and_live_unchanged=1 PASS\n";
}
void ordered_handoff(Fixture &f) {
  Parameters64 p;
  p.gravity = 1;
  p.contact_stiffness = p.reach_strength = 0;
  for (int order = 0; order < 2; ++order) {
    f.h.inspect_prepared();
    const double starts[]{0, 2, 5};
    for (int i = 0; i < 3; ++i)
      f.h.particle64(kPosX, f.in.particle_rows[i].record) = starts[i];
    no_new([&] {
      PreparedFixtureAccess::upload_body_fixture64(f.h);
      f.control->set(3);
    });
    for (int i = 0; i < 3; ++i) {
      const auto before = f.h.inspect_live_hierarchy();
      no_new([&] { LiveHierarchyAccess::poll(f.h, 0); });
      f.h.inspect_prepared();
      poll_lifetime(f, before);
    }
    no_new([&] {
      LiveHierarchyAccess::force(f.h, p);
      f.control->set(0);
      f.control->set(1, f.nodes[2].field.record);
    });
    f.h.inspect_prepared();
    const int unchanged_body = f.in.particle_rows[1].record;
    const auto baseline = f.h.particle64(kForceX, unchanged_body);
    f.h.particle64(kPosX, f.in.particle_rows[0].record) = .25;
    f.h.particle64(kPosX, f.in.particle_rows[2].record) = 4.875;
    no_new([&] {
      PreparedFixtureAccess::upload_body_fixture64(f.h);
      for (int repeat = 0; repeat < 64; ++repeat) {
        f.control->set(1, f.nodes[0].field.record);
        f.control->set(1, f.nodes[1].field.record);
      }
    });
    const auto step = [&](int target) {
      const auto before = f.h.inspect_live_hierarchy();
      no_new([&] { LiveHierarchyAccess::poll(f.h, 1, target); });
      f.h.inspect_prepared();
      poll_lifetime(f, before);
    };
    if (order == 0) {
      step(2);
      step(0);
      step(1);
    } else {
      step(0);
      step(2);
      step(1);
    }
    require((f.h.prepared_storage().inspection_flags[f.h.particle_capacity() +
                                                     f.nodes[2].field.record] &
             2) != 0,
            "late child notification lost after parent consumption");
    step(-1);
    hierarchy_reference(f);
    const auto expected = physical_reference(f, p, true);
    require(expected[kForceX * f.h.particle_capacity() + unchanged_body] !=
                baseline,
            "ordered pending-input witness is not consequential");
    require((f.h.prepared_storage().inspection_flags[unchanged_body] & 2) != 0,
            "consequential changed combined response left recipient asleep");
    std::cout << "ordered_handoff,order=" << order
              << ",duplicate_notifications=64,parent_pending=1,recipient_"
                 "pending=1 PASS\n";
  }
}
int held_settle(Fixture &f, const char *name, int round, int stage) {
  Parameters64 p;
  p.gravity = p.contact_stiffness = p.reach_strength = 0;
  f.h.inspect_prepared();
  const auto physical = f.h.particles64();
  int ticks = 0;
  const auto step = [&]() {
    const auto before = f.h.inspect_live_hierarchy();
    no_new([&] {
      f.h.tick64(p);
      f.h.inspect_prepared();
    });
    poll_lifetime(f, before);
    require(f.h.particles64() == physical,
            "held-body propagation mutated physical state");
  };
  for (; ticks < 16; ++ticks) {
    step();
    const auto &s = f.h.prepared_storage();
    if (!s.inspection_counts[1] && !s.inspection_counts[3])
      break;
  }
  require(ticks < 16, "held-input live hierarchy exceeded 16 polls");
  hierarchy_reference(f);
  for (int i = 0; i < 8; ++i) {
    step();
    const auto &s = f.h.prepared_storage();
    const auto &v = f.h.inspect_live_hierarchy();
    require(!s.inspection_counts[1] && !s.inspection_counts[3] &&
                !f.h.demand().particles_evaluated && !v.counters[1],
            "quiet window stranded/recreated work");
  }
  std::cout << "held,name=" << name << ",round=" << round << ",stage=" << stage
            << ",polls=" << ticks + 1 << ",quiet=8 PASS\n";
  return ticks + 1;
}
void cumulative(engine::Runtime &r, HostPath path, bool cancel) {
  Fixture f(r, path, {{}, {}, {0, 1}}, {{1, 0}, {0, 3}, {0, 0}}, {4, 4}, {1, 1},
            {1.125, 1.125});
  f.nodes.back().retained_fraction_bits = 4;
  f.install();
  Parameters64 p;
  p.gravity = p.contact_stiffness = p.reach_strength = 0;
  no_new([&] {
    f.h.tick64(p);
    f.h.inspect_prepared();
  });
  prepared_test::Metrics metrics(r, PreparedFixtureAccess::nodes(f.h));
  const auto before = metrics.sample();
  int max_polls = 0;
  for (int round = 0; round < 32; ++round)
    for (int stage = 0; stage < 3; ++stage) {
      f.h.inspect_prepared();
      const int sign = round % 2 ? -1 : 1;
      for (int i = 0; i < 2; ++i) {
        double x = 1.125;
        if (stage)
          x += (i == 0 ? 3. : (cancel ? -1. : 1.)) / 16;
        if (stage == 2 && i == 0)
          x += 1. / 32;
        f.h.particle64(kPosX, f.in.particle_rows[i].record) = sign * x;
      }
      no_new([&] {
        PreparedFixtureAccess::upload_body_fixture64(f.h);
        f.control->set(3);
      });
      max_polls = std::max(
          max_polls,
          held_settle(f, cancel ? "cancellation" : "quarters", round, stage));
      if (stage == 1) {
        const auto actual = selected(f.h.inspect_live_hierarchy(), 2);
        near(actual[0], sign * (cancel ? 1.125 : 1.1875),
             "cumulative crossing/cancellation cell");
      }
    }
  const auto after = metrics.sample();
  before.print("cumulative_before");
  after.print("cumulative_after");
  require(before.same_backing(after),
          "live reuse changed prepared native backing/compiler counters");
  std::cout << "cumulative,cancel=" << cancel << ",max_polls=" << max_polls
            << ",reuse=32 PASS\n";
}
void basic(engine::Runtime &r, HostPath path) {
  Fixture f(r, path, {{}, {}, {0, 1}}, {{1, 1, 0}, {0, 0, 1}, {0, 0, 0}},
            {1, 1, 1}, {1, 1, 2}, {0, 2, 5});
  preinstall_rejections(f);
  prepared_test::Metrics metrics(r, PreparedFixtureAccess::nodes(f.h));
  const auto first_before = metrics.sample();
  f.install();
  std::size_t scratch = 0;
  for (const auto &e : f.h.prepared_storage().manifest) {
    std::cout << "storage,name=" << e.name << ",count=" << e.count
              << ",element_bytes=" << e.element_bytes << ",copies=" << e.copies
              << ",bytes=" << e.bytes << '\n';
    scratch += e.bytes;
  }
  require(scratch == limits()[Budget::ScratchBytes],
          "installed manifest does not sum to scratch reservation");
  const auto shapes = f.h.prepared_storage().capacity.numerical_shapes();
  for (std::size_t i = 0; i < shapes.size(); ++i) {
    const auto &s = shapes[i];
    const bool primary = i < 6 || i == 21;
    std::cout << "numerical_shape,name=" << s.name << ",rows=" << s.rows
              << ",stride=" << s.stride << ",element_bytes=" << s.element_bytes
              << ",per_copy_bytes=" << s.bytes << ",host_copies=" << primary
              << ",native_owned_copies="
              << (!primary || path == HostPath::BulkCopy) << '\n';
  }
  std::cout << "storage_summary,scratch=" << scratch
            << ",binding_bytes=" << f.h.prepared_storage().binding_bytes
            << ",native_publication_bytes=4760,native_history_bytes=2240,"
               "retained_force_extra_bytes=0\n";
  std::cout << "object_sizes,harness=" << sizeof(Harness)
            << ",prepared_storage=" << sizeof(detail::PreparedStorage)
            << ",live_impl=" << LiveHierarchyAccess::fixed_bytes(f.h)
            << ",live_wrapper=" << sizeof(detail::LiveHierarchy)
            << ",live_inspection=" << sizeof(LiveHierarchyInspection)
            << ",node=" << sizeof(CascadeNode)
            << ",source_row=" << sizeof(CascadeSourceRow)
            << ",ndarray_wrapper=" << sizeof(taichi::lang::Ndarray)
            << ",owned_live_ndarrays=16,compiled_kernel_wrapper="
            << sizeof(engine::CompiledKernel) << '\n';
  Parameters64 p;
  p.gravity = 1;
  p.contact_stiffness = p.reach_strength = 0;
  audit_negative(*LiveHierarchyAccess::kernels(f.h)[0]);
  no_new([&] {
    LiveHierarchyAccess::force(f.h, p);
    f.h.inspect_prepared();
  });
  const double expected[]{3, -1, -2};
  for (int i = 0; i < 3; ++i)
    near(f.h.particle64(kForceX, f.in.particle_rows[i].record), expected[i],
         "two-level combined force");
  near(f.h.universal64(), 4, "universal mass");
  const auto old = f.h.particles64(),
             reference = physical_reference(f, p, true);
  no_new([&] {
    f.h.tick64(p);
    f.h.inspect_prepared();
  });
  verify_physical(f, reference, old, p);
  const auto first_after = metrics.sample();
  first_before.print("first_use_before");
  first_after.print("first_use_after");
  require(
      first_before.same_backing(first_after),
      "first installed use changed prepared native backing/compiler counters");
  installed_rejections(f);
  ordered_handoff(f);
  // A numerical failure after native mutation is unready, never a healthy
  // rejection/rollback. The private fixture adapter supplies this fault only.
  PreparedFixtureAccess::inject_value64(f.h, kMass,
                                        f.in.particle_rows[0].record,
                                        std::numeric_limits<double>::max());
  bool failed = false;
  try {
    f.h.tick64(p);
  } catch (const std::runtime_error &) {
    failed = true;
  }
  require(failed && !f.h.ready(),
          "post-mutation numerical failure remained ready");
  require(
      f.h.install_live_hierarchy(f.h.prepared_storage().generation, f.spec())
              .status == CascadeStatus::Unready,
      "unready hierarchy could be reinstalled as healthy");
  std::cout << "post_mutation_failure,unready=1,rollback_claimed=0 PASS\n";
  std::cout << "basic PASS\n";
}
void force_examples(engine::Runtime &r, HostPath path) {
  {
    Fixture f(
        r, path, {{}, {}, {}, {0, 1}, {2, 3}},
        {{1, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}},
        {1, 2, 1, 1}, {1, 2, 1, 4}, {0, 2, 4, 8});
    f.install();
    Parameters64 p;
    p.gravity = 1;
    p.contact_stiffness = p.reach_strength = 0;
    no_new([&] {
      LiveHierarchyAccess::force(f.h, p);
      f.h.inspect_prepared();
    });
    const double expected[]{62. / 9, 16. / 9, -46. / 9, -32. / 9};
    for (int i = 0; i < 4; ++i)
      near(f.h.particle64(kForceX, f.in.particle_rows[i].record), expected[i],
           "three-level overlap ownership");
    near(f.h.universal64(), 8, "overlap physical universal mass");
    std::cout << "force_example,overlap_and_third_level PASS\n";
  }
  {
    Fixture f(r, path, {{}, {}, {0, 1}}, {{2, 1, 0}, {0, 0, 1}, {0, 0, 0}},
              {3, 1, 1}, {6, 2, 2}, {0, 0, 4}, 52, 52, true);
    const auto alias = f.refs.front();
    f.refs.insert(f.refs.begin() + 1, alias);
    ++f.nodes[0].cohort_count;
    for (std::size_t n = 1; n < f.nodes.size(); ++n)
      ++f.nodes[n].cohort_begin;
    f.install();
    Parameters64 p;
    p.gravity = .25;
    p.contact_stiffness = p.reach_strength = 0;
    no_new([&] {
      LiveHierarchyAccess::force(f.h, p);
      f.h.inspect_prepared();
    });
    const double expected[]{8, 4, -4. / 9};
    for (int i = 0; i < 3; ++i)
      near(f.h.particle64(kForceX, f.in.particle_rows[i].record), expected[i],
           "repeated ordinal force distribution");
    near(f.h.universal64(), 10, "partial root universal mass");
    std::cout << "force_example,repeated_ordinals_alias_partial_root PASS\n";
  }
  {
    const auto D = (std::uint64_t{1} << 53) + 1;
    Fixture f(r, path, {{}, {}, {0, 1}}, {{D - 1, 1, 0}, {1, 0, 1}, {0, 0, 0}},
              {D, 1, 1}, {1, 2, 3}, {1.125, 2, 4});
    f.install();
    f.h.inspect_prepared();
    hierarchy_reference(f);
    Parameters64 p;
    p.gravity = .01;
    p.contact_stiffness = p.reach_strength = 0;
    const auto old = f.h.particles64(),
               expected = physical_reference(f, p, true);
    no_new([&] {
      f.h.tick64(p);
      f.h.inspect_prepared();
    });
    verify_physical(f, expected, old, p);
    std::cout << "force_example,wide_source_count_gt_2p53 PASS\n";
  }
}
void singleton_and_capacity(engine::Runtime &r, HostPath path) {
  {
    Fixture f(r, path, {{}, {0}, {1}, {2}, {3}}, {{2}, {0}, {0}, {0}, {0}}, {3},
              {2}, {1.125}, 52, 52, true);
    f.nodes[1].level = f.nodes[0].level;
    f.nodes[1].retained_fraction_bits = 4;
    require(f.h.bind_weighted_centroid_cascade(
                   f.h.prepared_storage().generation, f.spec())
                    .status != CascadeStatus::Ok,
            "candidate-only level contract was silently broadened");
    f.install();
    Parameters64 p;
    p.gravity = 1;
    p.contact_stiffness = p.reach_strength = 0;
    no_new([&] {
      LiveHierarchyAccess::force(f.h, p);
      f.h.inspect_prepared();
    });
    near(f.h.particle64(kForceX, f.in.particle_rows[0].record), 0,
         "singleton repeated ordinals self-force");
    Parameters64 inert;
    p = inert;
    p.gravity = p.contact_stiffness = p.reach_strength = 0;
    no_new([&] {
      f.h.tick64(p);
      f.h.inspect_prepared();
    });
    f.h.particle64(kPosX, f.in.particle_rows[0].record) = 1.15625;
    no_new([&] {
      PreparedFixtureAccess::upload_body_fixture64(f.h);
      f.control->set(1, f.nodes[0].field.record);
    });
    held_settle(f, "same_level_unary", 0, 0);
    const auto before = f.h.inspect_live_hierarchy();
    f.h.particle64(kMass, f.in.particle_rows[0].record) = 3;
    no_new([&] {
      PreparedFixtureAccess::upload_body_fixture64(f.h);
      LiveHierarchyAccess::mass_total(f.h, 3);
      f.control->set(1, f.nodes[0].field.record);
    });
    held_settle(f, "mass_only", 0, 1);
    const auto &after = f.h.inspect_live_hierarchy();
    for (int n = 0; n < 5; ++n) {
      const auto old = selected(before, n), now = selected(after, n);
      near(now[0], old[0], "mass-only centre changed");
      near(now[3], 2, "mass-only participating mass stale");
    }
    std::cout << "singleton,same_level_unary_mass_only PASS\n";
  }
  {
    Fixture f(r, path, {{}, {}, {0, 1}}, {{1, 1, 0}, {0, 0, 1}, {0, 0, 0}},
              {1, 1, 1}, {1, 1, 2}, {0, 2, 5}, 52, 52, false, false, false,
              false, 5);
    const auto before = state_hash(f);
    LiveHierarchyResult result;
    no_new([&] {
      result = f.h.install_live_hierarchy(f.h.prepared_storage().generation,
                                          f.spec());
    });
    require(result.status == CascadeStatus::CapacityExhausted &&
                result.failure.budget == Budget::LiveResponses &&
                result.failure.required == 6 && state_hash(f) == before,
            "response budget exhaustion was not atomic");
    std::cout << "response_capacity,required=6,capacity=5 PASS\n";
  }
}
void boundary_tree(engine::Runtime &r, HostPath path) {
  constexpr int leaves = 35;
  std::vector<std::vector<int>> edges(leaves);
  std::vector<std::vector<std::uint64_t>> coverage(
      leaves, std::vector<std::uint64_t>(leaves));
  for (int i = 0; i < leaves; ++i)
    coverage[i][i] = 1;
  std::vector<int> level(leaves);
  for (int i = 0; i < leaves; ++i)
    level[i] = i;
  while (level.size() > 1) {
    std::vector<int> next;
    for (std::size_t i = 0; i < level.size(); i += 2) {
      if (i + 1 == level.size()) {
        next.push_back(level[i]);
        continue;
      }
      next.push_back(int(edges.size()));
      edges.push_back(
          {std::min(level[i], level[i + 1]), std::max(level[i], level[i + 1])});
      coverage.emplace_back(leaves);
    }
    level = std::move(next);
  }
  edges.push_back({level[0]});
  coverage.emplace_back(leaves);
  require(edges.size() == 70, "boundary tree fixture shape");
  std::vector<double> masses(leaves, 1), positions(leaves);
  for (int i = 0; i < leaves; ++i)
    positions[i] = 1. + i * .25;
  Fixture f(r, path, edges, coverage, std::vector<std::uint64_t>(leaves, 1),
            masses, positions, 52, 16, false, false, false, true);
  f.install();
  Parameters64 p;
  p.gravity = p.contact_stiffness = p.reach_strength = 0;
  no_new([&] {
    f.h.tick64(p);
    f.h.inspect_prepared();
  });
  for (int round = 0; round < 32; ++round) {
    f.h.particle64(kPosX, f.in.particle_rows[round % leaves].record) +=
        (round % 2 ? -.03125 : .0625);
    no_new([&] {
      PreparedFixtureAccess::upload_body_fixture64(f.h);
      f.control->set(1, f.nodes[round % leaves].field.record);
    });
    held_settle(f, "permuted_70_nodes", round, 0);
  }
  std::cout << "boundary_tree,nodes=70,final_field=69,word_and_pointer_blocks="
               "1,reuse=32 PASS\n";
  // A unary top root changes no eligible force by itself. An explicitly unsafe
  // test-only consumption exposes why paired zero is insufficient: the retained
  // last combined force still differs from the complete current input set.
  Parameters64 force;
  force.gravity = 1;
  force.contact_stiffness = force.reach_strength = 0;
  no_new([&] {
    LiveHierarchyAccess::force(f.h, force);
    f.control->set(0);
  });
  f.h.inspect_prepared();
  f.h.particle64(kPosX, f.in.particle_rows[0].record) += .0625;
  no_new([&] {
    PreparedFixtureAccess::upload_body_fixture64(f.h);
    f.control->set(1, f.nodes[0].field.record);
  });
  for (int node = 0; node < 69; ++node) {
    const auto before = f.h.inspect_live_hierarchy();
    no_new([&] { LiveHierarchyAccess::poll(f.h, 1, node); });
    f.h.inspect_prepared();
    poll_lifetime(f, before);
  }
  no_new([&] { f.control->set(4); });
  f.h.inspect_prepared();
  require(
      !f.h.prepared_storage().inspection_counts[1] &&
          (f.h.prepared_storage().inspection_flags[f.h.particle_capacity() +
                                                   f.nodes[69].field.record] &
           2),
      "baseline control did not expose sleeping recipients and a pending root");
  const auto before = f.h.inspect_live_hierarchy();
  const auto baseline_wakes = before.counters[7];
  no_new([&] { LiveHierarchyAccess::poll(f.h, 1, 69); });
  f.h.inspect_prepared();
  poll_lifetime(f, before);
  require(f.h.prepared_storage().inspection_counts[1] > 0 &&
              f.h.inspect_live_hierarchy().counters[7] > baseline_wakes,
          "cumulative baseline guard did not preserve consequential input");
  std::cout << "baseline_control,paired_only_delta=0,retained_combined_force_"
               "differs=1,recipient_reactivated=1 PASS\n";
}
struct LetterData {
  std::vector<double> masses, positions, radii;
  std::vector<std::uint64_t> denominators;
#ifdef HCP_HAS_VIEWPORT
  assembly::PreparedStudy mapping;
  std::vector<assembly::Origin> origins;
#endif
};
LetterData letter_data(engine::Runtime &r) {
  membership_fixture::Fixture source;
  auto first = source.request(source.english, 0, warm::Exposure::Aggregate);
  auto second = source.request(source.accented, 1, warm::Exposure::Parts);
  first.aggregate = {{1, 0, 0}, {0, 0, 0}};
  for (auto &byte : second.bytes)
    byte.initial = {{3, 0, 0}, {0, 0, 0}};
  const auto recipe = assembly::normalize_letters(source.base, {first, second});
  const auto projection = assembly::project_letters(recipe);
  const auto memberships = assembly::membership_fields(projection);
  using namespace taichi::lang;
  auto root = std::make_unique<SNode>(0, SNodeType::root);
  auto *node = root.get();
  auto &primary = root->dense(Axis(0), 1);
  primary.insert_children(SNodeType::place).dt = PrimitiveType::i32;
  std::vector<assembly::StudyNode> nodes{
      {node, -1, assembly::ScaleRole::Root},
      {&primary, 0, assembly::ScaleRole::PrimaryStep}};
  r.program().add_snode_tree(std::move(root), false);
  const auto prepared = assembly::prepare(memberships.bind(projection, nodes));
  LetterData out;
#ifdef HCP_HAS_VIEWPORT
  out.mapping = prepared;
  for (auto &scale : out.mapping.scales)
    scale.node.layout = nullptr;
  for (const auto &o : projection.occurrences())
    if (o.active)
      out.origins.push_back({o.key, o.definition.object, o.definition, {}});
#endif
  for (const auto &p : prepared.particles) {
    require(p.exact_mass > 0 && p.mass > 0 && p.radius > 0,
            "letter fixture missing intrinsic/source values");
    out.masses.push_back(p.mass);
    out.denominators.push_back(p.exact_mass);
    out.positions.push_back(p.position.x + 2);
    out.radii.push_back(p.radius);
  }
  require(out.masses.size() == 3, "letter/UTF-8 byte fixture cardinality");
  std::cout << "letter_inputs,English_A_aggregate=1,Dutch_accented_i_UTF8_"
               "bytes=2,fixture_geometry=1,prepared_frame_conversion=1\n";
  return out;
}
void publication_diagnostics(engine::Runtime &r, HostPath path) {
  const auto report = [](const char *label, int sign, double old,
                         double displacement, double published) {
    const double candidate = old + displacement;
    const double reference = candidate != 0 ? candidate : (old != 0 ? old : 1);
    const double ulp = std::ldexp(1., std::ilogb(std::abs(reference)) - 8);
    const double error = std::abs(published - candidate);
    require(std::isfinite(error) && error < ulp,
            "Q8 publication error exceeds its declared local cell");
    std::cout << "publication_probe,label=" << label << ",sign=" << sign
              << ",body_fraction_bits=8,old=" << old
              << ",calculated_displacement=" << displacement
              << ",candidate=" << candidate
              << ",published_displacement=" << published - old
              << ",absolute_publication_error=" << error
              << ",local_cell_ulp=" << ulp << ",error_ulps=" << error / ulp
              << ",ulp_reference=nonzero_candidate_else_old_else_one PASS\n";
  };
  {
    Fixture f(r, path, {{}}, {{1}}, {1}, {1}, {1}, 8, 6);
    f.install();
    Parameters64 p;
    p.gravity = p.contact_stiffness = p.reach_strength = 0;
    p.dt = 1;
    const int body = f.in.particle_rows[0].record;
    struct Case {
      const char *name;
      double x, step;
    };
    const Case cases[]{{"toward_zero_subcell", 1, -std::ldexp(1., -20)},
                       {"away_zero_subcell", 1, std::ldexp(1., -20)},
                       {"cell_boundary", 1, std::ldexp(1., -8)},
                       {"below_exponent_boundary", 2, -std::ldexp(1., -20)},
                       {"cross_exponent_boundary", 1.99609375, .0078125},
                       {"cross_zero", std::ldexp(1., -10), -std::ldexp(1., -9)},
                       {"arrive_zero", 1, -1},
                       {"quiet_exact_cell", 1, 0}};
    for (int sign : {1, -1})
      for (const auto &c : cases) {
        f.h.inspect_prepared();
        f.h.particle64(kPosX, body) = sign * c.x;
        f.h.particle64(kVelX, body) = sign * c.step;
        no_new([&] {
          PreparedFixtureAccess::upload_body_fixture64(f.h);
          f.control->set(3);
          f.control->set(2, body);
        });
        const auto old = f.h.particles64(),
                   expected = physical_reference(f, p, true);
        const auto before = f.h.inspect_live_hierarchy();
        no_new([&] {
          f.h.tick64(p);
          f.h.inspect_prepared();
        });
        verify_physical(f, expected, old, p);
        poll_lifetime(f, before);
        report(c.name, sign, sign * c.x, f.h.particle64(kVelX, body) * p.dt,
               f.h.particle64(kPosX, body));
      }
    // The final case has zero velocity and no forces. Here quiet is justified,
    // independently of whether an interacting moving fixture should settle.
    held_settle(f, "zero_force_zero_velocity_publication", 0, 0);
  }
  {
    Fixture f(r, path, {{}, {}, {0, 1}}, {{1, 1, 0}, {0, 0, 1}, {0, 0, 0}},
              {1, 1, 1}, {1, 1, 2}, {0, 2, 5}, 8, 6);
    f.install();
    Parameters64 p;
    p.gravity = 1;
    p.dt = 1;
    p.contact_stiffness = p.reach_strength = 0;
    const double velocities[]{-3, 1, 1};
    for (int i = 0; i < 3; ++i)
      f.h.particle64(kVelX, f.in.particle_rows[i].record) = velocities[i];
    no_new([&] { PreparedFixtureAccess::upload_body_fixture64(f.h); });
    const auto old = f.h.particles64(),
               expected = physical_reference(f, p, true);
    const auto before = f.h.inspect_live_hierarchy();
    no_new([&] {
      f.h.tick64(p);
      f.h.inspect_prepared();
    });
    verify_physical(f, expected, old, p);
    poll_lifetime(f, before);
    for (int i = 0; i < 3; ++i) {
      const int body = f.in.particle_rows[i].record;
      require(f.h.particle64(kVelX, body) == 0,
              "force/carried-velocity cancellation witness changed");
      report("force_velocity_cancellation", 0,
             old[kPosX * f.h.particle_capacity() + body], 0,
             f.h.particle64(kPosX, body));
    }
  }
}
struct MovingTrace {
  struct Dynamics {
    double x{}, max_abs_x{}, max_abs_v{}, max_abs_force{}, max_step{}, travel{};
    int velocity_sign{}, position_sign{};
    std::uint64_t velocity_reversals{}, position_reversals{};
  };
  const int stride;
  std::vector<double> ring;
  std::vector<Dynamics> dynamics;
  explicit MovingTrace(const Fixture &f)
      : stride(4 * (f.denominators.size() + f.nodes.size())),
        ring(64 * stride),
        dynamics(f.denominators.size()) {
  }
  static int sign(double x) {
    return (x > 0) - (x < 0);
  }
  void reset(const Fixture &f) {
    for (std::size_t i = 0; i < dynamics.size(); ++i) {
      auto &d = dynamics[i];
      d = {};
      const int slot = f.in.particle_rows[i].record;
      d.x = f.h.particle64(kPosX, slot);
      d.max_abs_x = std::abs(d.x);
      d.velocity_sign = sign(f.h.particle64(kVelX, slot));
    }
  }
  void sample(Fixture &f, int tick) {
    auto *row = ring.data() + (tick % 64) * stride;
    const auto &s = f.h.prepared_storage();
    for (std::size_t i = 0; i < f.denominators.size(); ++i) {
      int slot = f.in.particle_rows[i].record;
      const double x = f.h.particle64(kPosX, slot),
                   velocity = f.h.particle64(kVelX, slot),
                   force = f.h.particle64(kForceX, slot);
      auto &d = dynamics[i];
      const double step = x - d.x;
      const int vs = sign(velocity), ps = sign(step);
      d.velocity_reversals += vs && d.velocity_sign && vs != d.velocity_sign;
      d.position_reversals += ps && d.position_sign && ps != d.position_sign;
      if (vs)
        d.velocity_sign = vs;
      if (ps)
        d.position_sign = ps;
      d.max_abs_x = std::max(d.max_abs_x, std::abs(x));
      d.max_abs_v = std::max(d.max_abs_v, std::abs(velocity));
      d.max_abs_force = std::max(d.max_abs_force, std::abs(force));
      d.max_step = std::max(d.max_step, std::abs(step));
      d.travel += std::abs(step);
      d.x = x;
      *row++ = x;
      *row++ = velocity;
      *row++ = force;
      *row++ = (s.inspection_flags[slot] & 2) != 0;
    }
    const auto &v = f.h.inspect_live_hierarchy();
    for (std::size_t n = 0; n < f.nodes.size(); ++n) {
      auto p = selected(v, n);
      *row++ = p[0];
      *row++ = p[3];
      *row++ = v.selectors[n];
      *row++ = (s.inspection_flags[f.h.particle_capacity() +
                                   f.nodes[n].field.record] &
                2) != 0;
    }
  }
  void print_dynamics(const char *name, int round) const {
    for (std::size_t i = 0; i < dynamics.size(); ++i) {
      const auto &d = dynamics[i];
      std::cout << "moving_dynamics,name=" << name << ",round=" << round
                << ",body=" << i
                << ",velocity_reversals=" << d.velocity_reversals
                << ",published_direction_reversals=" << d.position_reversals
                << ",max_abs_x=" << d.max_abs_x
                << ",max_abs_velocity_x=" << d.max_abs_v
                << ",max_abs_force_x=" << d.max_abs_force
                << ",max_published_step_x=" << d.max_step
                << ",published_travel_x=" << d.travel << '\n';
    }
  }
  void print(const Fixture &f, const char *name, int round, int ticks) const {
    for (int t = std::max(0, ticks - 64); t < ticks; ++t) {
      const auto *row = ring.data() + (t % 64) * stride;
      std::cout << "moving_trace,name=" << name << ",round=" << round
                << ",tick=" << t;
      for (std::size_t i = 0; i < f.denominators.size(); ++i)
        for (const char *key : {"x", "v", "force", "awake"})
          std::cout << ",body" << i << '_' << key << '=' << *row++;
      for (std::size_t n = 0; n < f.nodes.size(); ++n)
        for (const char *key : {"x", "mass", "selector", "awake"})
          std::cout << ",node" << n << '_' << key << '=' << *row++;
      std::cout << '\n';
    }
  }
};
void moving_case(Fixture &f, const char *name) {
  f.install();
  Parameters64 p;
  p.gravity = .0771063443835106;
  p.dt = .5;
  p.contact_stiffness = 1;
  p.reach_strength = 1;
  std::unique_ptr<prepared_test::Metrics> metrics;
  prepared_test::Sample baseline;
  MovingTrace trace(f);
  int evolving_rounds = 0;
  for (int round = 0; round <= 32; ++round) {
    if (round) {
      f.h.inspect_prepared();
      const auto body = (round % 2) ? 0 : f.denominators.size() - 1;
      const int slot = f.in.particle_rows[body].record;
      f.h.particle64(kPosX, slot) += (round % 2 ? 1 : -1) * .5;
      no_new([&] {
        PreparedFixtureAccess::upload_body_fixture64(f.h);
        f.control->set(2, slot);
        for (std::size_t n = 0; n < f.nodes.size(); ++n)
          if (f.adjacency[n].empty() && f.coverage[n][body])
            f.control->set(1, f.nodes[n].field.record);
      });
    }
    trace.reset(f);
    int quiet = 0, ticks = 0;
    const auto started = std::chrono::steady_clock::now();
    double ordinary_ms = 0;
    std::uint64_t work = 0, fields = 0, attempts = 0, conservative = 0;
    for (; ticks < 4096 && quiet < 32; ++ticks) {
      f.h.inspect_prepared();
      const auto old = f.h.particles64(),
                 expected = physical_reference(f, p, round == 0 && ticks == 0);
      const auto before = f.h.inspect_live_hierarchy();
      no_new([&] {
        f.h.tick64(p);
        f.h.inspect_prepared();
      });
      verify_physical(f, expected, old, p);
      poll_lifetime(f, before);
      const auto &s = f.h.prepared_storage();
      const auto &v = f.h.inspect_live_hierarchy();
      work += f.h.demand().particles_evaluated;
      fields += v.counters[1];
      attempts += f.h.demand().motion_attempts;
      conservative += v.counters[7];
      ordinary_ms += f.h.demand().tick_ms;
      trace.sample(f, ticks);
      if (s.inspection_counts[1] == 0 && s.inspection_counts[3] == 0 &&
          !f.h.demand().particles_evaluated && !v.counters[1])
        ++quiet;
      else
        quiet = 0;
    }
    std::cout << "moving,name=" << name << ",round=" << round
              << ",contract=coherent_dynamics_v2,status="
              << (quiet == 32 ? "settled" : "evolving") << ",ticks=" << ticks
              << ",quiet=" << quiet << ",body_work=" << work
              << ",field_work=" << fields << ",attempts=" << attempts
              << ",baseline_wakes=" << conservative
              << ",ordinary_ms=" << ordinary_ms << ",observed_elapsed_ms="
              << std::chrono::duration<double, std::milli>(
                     std::chrono::steady_clock::now() - started)
                     .count()
              << '\n';
    trace.print_dynamics(name, round);
    if (quiet != 32) {
      ++evolving_rounds;
      // Observe without altering publications or pending native work. Bound the
      // serialized trace volume; all rounds still retain dynamics/work totals.
      if (evolving_rounds == 1 || round == 32)
        trace.print(f, name, round, ticks);
    } else {
      hierarchy_reference(f);
    }
    if (round == 0 || round == 32)
      reader_consistency(f, quiet == 32);
    if (!round) {
      metrics = std::make_unique<prepared_test::Metrics>(
          f.runtime, PreparedFixtureAccess::nodes(f.h));
      baseline = metrics->sample();
      baseline.print("moving_reuse_before");
    }
  }
  const auto after = metrics->sample();
  after.print("moving_reuse_after");
  require(baseline.same_backing(after),
          "moving recovery reuse changed prepared backing/compiler counters");
  std::cout << "moving_sequence,name=" << name
            << ",contract=coherent_dynamics_v2,rounds=33,perturbations=32,"
               "evolving_rounds="
            << evolving_rounds << ",continuous_publications=1,reuse=32 PASS\n";
  // The moving study is complete. Only now intervene by holding bodies fixed;
  // no later moving tick may resume from this altered diagnostic state.
  const auto physical = f.h.particles64();
  int polls = 0;
  for (; polls < 16 && f.h.prepared_storage().inspection_counts[3]; ++polls) {
    const auto previous = f.h.inspect_live_hierarchy();
    no_new([&] {
      LiveHierarchyAccess::poll(f.h, p.gravity, -1, p.contact_stiffness);
      f.h.inspect_prepared();
    });
    poll_lifetime(f, previous);
    require(f.h.particles64() == physical,
            "terminal frozen diagnostic changed physical state");
  }
  require(!f.h.prepared_storage().inspection_counts[3],
          "terminal frozen hierarchy did not converge within 16 polls");
  hierarchy_reference(f);
  std::cout << "frozen_after_sequence,name=" << name
            << ",terminal_intervention=1,polls=" << polls
            << ",hierarchy_reference=1,physical_unchanged=1,body_work_pending="
            << f.h.prepared_storage().inspection_counts[1] << '\n';
}
void moving(engine::Runtime &r, HostPath path, bool permute = false) {
  {
    Fixture f(r, path, {{}, {}, {0, 1}}, {{1, 1, 0}, {0, 0, 1}, {0, 0, 0}},
              {1, 1, 1}, {1, 1, 2}, {1, 1.5, 3}, 8, 6, false, true, true,
              permute);
    moving_case(f, permute ? "two_level_permuted" : "two_level");
  }
  {
    Fixture f(
        r, path, {{}, {}, {}, {0, 1}, {2, 3}},
        {{1, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}},
        {1, 2, 1, 1}, {1, 2, 1, 4}, {1, 1.5, 3, 5}, 8, 6, false, true, true,
        permute);
    moving_case(
        f, permute ? "three_level_overlap_permuted" : "three_level_overlap");
  }
}
#ifdef HCP_HAS_VIEWPORT
void capture_current(engine::Runtime &r,
                     HostPath path,
                     const std::string &output,
                     std::uint64_t ticks,
                     std::uint64_t every) {
  const auto data = letter_data(r);
  std::vector<std::vector<std::uint64_t>> count(3,
                                                std::vector<std::uint64_t>(3));
  count[0][0] = data.denominators[0];
  count[1][1] = data.denominators[1];
  count[1][2] = data.denominators[2];
  Fixture f(r, path, {{}, {}, {0, 1}}, count, data.denominators, data.masses,
            data.positions, 8, 6, false, true, true, false, 1024, data.radii);
  f.nodes.back().retained_fraction_bits = 5;  // Exercise a reported override.
  const int first_body = f.in.particle_rows[0].record;
  f.h.particle64(kPosX, first_body) += std::ldexp(1., -30);
  no_new([&] { PreparedFixtureAccess::upload_body_fixture64(f.h); });
  f.install();
  assembly::ActiveReport mapping;
  mapping.handle = {warm::new_uuid(), data.origins.front().occurrence.study,
                    warm::new_uuid()};
  mapping.owner = warm::new_uuid();
  mapping.instance = warm::new_uuid();
  mapping.origins = data.origins;
  mapping.prepared = data.mapping;
  mapping.prepared.capacity = f.h.particle_capacity();
  mapping.prepared.groups.assign(f.h.field_capacity(), {});
  for (std::size_t n = 0; n < f.nodes.size(); ++n)
    mapping.prepared.groups[f.nodes[n].field.record] = {
        assembly::GroupKind::Membership,
        "native-comparison:" + std::to_string(f.nodes[n].comparison)};
  mapping.prepared.focal_groups = {f.nodes.back().field.record};
  for (std::size_t i = 0; i < mapping.prepared.particles.size(); ++i)
    mapping.prepared.particles[i].occurrence.slot =
        f.in.particle_rows[i].record;
  const auto binding = viewport::bind(f.h, mapping);
  viewport::CaptureContext context;
  context.handle = mapping.handle;
  context.label =
      "Current installed C3 English A / Dutch accented i byte fixture";
  for (std::size_t i = 0; i < mapping.prepared.particles.size(); ++i) {
    const auto &o = mapping.prepared.particles[i].occurrence;
    const auto aggregate = i == 0 ? o.id : mapping.handle.study.str() + "/1";
    context.occurrences.emplace(
        o.id,
        viewport::DisplayContext{
            i == 0 ? "aggregate" : "parts", aggregate,
            i == 0 ? "English A" : "Dutch í UTF-8 byte " + std::to_string(i)});
  }
  Parameters64 p;
  p.dt = .5;
  context.source = {
      {"kind", "bounded_native_current_hierarchy"},
      {"data", "existing native letter/UTF-8 bootstrap fixture"},
      {"geometry", "supplied intrinsic test extents; common-frame x+2"},
      {"parameters",
       {{"dt", p.dt},
        {"gravity", p.gravity},
        {"contact_stiffness", p.contact_stiffness},
        {"reach_strength", p.reach_strength}}}};
  namespace fs = std::filesystem;
  const fs::path directory =
      output.empty() ? fs::temp_directory_path() /
                           ("hcp-current-capture-" + warm::new_uuid().str())
                     : fs::path(output);
  fs::create_directories(directory);
  struct Cleanup {
    fs::path path;
    bool remove;
    ~Cleanup() {
      if (remove) {
        std::error_code ignored;
        fs::remove_all(path, ignored);
      }
    }
  } cleanup{directory, output.empty()};
  double observation_ms = 0, export_ms = 0, engine_ms = 0;
  std::size_t copies = 0, captures = 0, readback = 0;
  const auto capture = [&]() {
    const auto before = state_hash(f);
    auto state = viewport::observe(f.h, binding);
    require(state_hash(f) == before,
            "capture changed same-execution state/pending inputs/Demand");
    context.tick = state.tick;
    auto value = viewport::snapshot(state, context);
    require(viewport::Json::parse(value.dump()) == value,
            "snapshot JSON round-trip changed values");
    require(state.tick == f.h.prepared_storage().completed_ticks &&
                state.backend == "native_cpu" &&
                state.comparisons.size() == 3 &&
                state.comparisons.back().retained_fraction_bits == 5,
            "capture backend/tick/precision identity mismatch");
    for (const auto &particle : value["particles"]) {
      const int slot = particle["slot"].get<int>();
      for (int axis = 0; axis < 3; ++axis)
        require(particle["position"][axis].get<double>() ==
                    f.h.particle64(kPosX + axis, slot),
                "capture narrowed/recomputed native coordinate");
      require(
          particle["radius"].get<double>() == f.h.particle64(kRadius, slot) &&
              particle["mass"].get<double>() == f.h.particle64(kMass, slot),
          "capture mass/extent differs from current execution");
    }
    auto bad_context = context;
    ++bad_context.tick;
    bool rejected = false;
    try {
      viewport::snapshot(state, bad_context);
    } catch (const std::invalid_argument &) {
      rejected = true;
    }
    require(rejected, "capture accepted stale tick metadata");
    auto bad = binding;
    ++bad.bodies.front().incarnation;
    rejected = false;
    try {
      viewport::observe(f.h, bad);
    } catch (const std::invalid_argument &) {
      rejected = true;
    }
    require(rejected, "capture accepted recycled source handle");
    const auto started = std::chrono::steady_clock::now();
    if (state.tick == 0)
      viewport::write_atomic((directory / "hierarchy-tick0.json").string(),
                             value);
    if (state.tick == ticks)
      viewport::write_atomic((directory / "hierarchy-final.json").string(),
                             value);
    viewport::write_atomic((directory / "latest.json").string(), value);
    export_ms += std::chrono::duration<double, std::milli>(
                     std::chrono::steady_clock::now() - started)
                     .count();
    observation_ms += state.observation_ms;
    copies += state.numeric_copy_bytes;
    readback += state.requested_readback_bytes;
    ++captures;
    require(state_hash(f) == before,
            "snapshot export changed active execution");
    if (state.tick == 0)
      require(
          value["particles"][0]["position"][0].get<double>() !=
              double(float(value["particles"][0]["position"][0].get<double>())),
          "capture fixture lacks f64 detail");
  };
  capture();
  for (std::uint64_t tick = 0; tick < ticks; ++tick) {
    f.h.inspect_prepared();
    const auto old = f.h.particles64(),
               expected = physical_reference(f, p, tick == 0);
    const auto previous = f.h.inspect_live_hierarchy();
    no_new([&] {
      f.h.tick64(p);
      f.h.inspect_prepared();
    });
    verify_physical(f, expected, old, p);
    poll_lifetime(f, previous);
    engine_ms += f.h.demand().tick_ms;
    if ((tick + 1) % every == 0 || tick + 1 == ticks)
      capture();
  }
  viewport::Json evidence{{"contract", "same_active_execution_v1"},
                          {"backend", "native_cpu"},
                          {"ticks", ticks},
                          {"captures", captures},
                          {"capture_state_equivalent", true},
                          {"source_mapping_checked", true},
                          {"f64_round_trip", true},
                          {"numeric_copy_bytes", copies},
                          {"requested_readback_bytes", readback},
                          {"observation_ms", observation_ms},
                          {"export_ms", export_ms},
                          {"engine_ms", engine_ms},
                          {"cuda_executed", false},
                          {"field_overlays", false}};
  viewport::write_atomic((directory / "hierarchy-evidence.json").string(),
                         evidence);
  std::cout << "current_capture," << evidence.dump() << " PASS\n";
}
#endif

}  // namespace
int main(int argc, char **argv) {
  try {
    engine::InstanceSettings settings;
    settings.cpu_threads = 1;
    HostPath path = HostPath::BulkCopy;
    bool quick = false;
#ifdef HCP_HAS_VIEWPORT
    std::string capture_output;
    std::uint64_t capture_ticks = 64, capture_every = 8;
#endif
    std::string only;
    for (int i = 1; i < argc; ++i) {
      if (std::string(argv[i]) == "--direct")
        path = HostPath::CpuDirect;
      else if (std::string(argv[i]) == "--quick")
        quick = true;
      else if (std::string(argv[i]) == "--case" && i + 1 < argc)
        only = argv[++i];
      else if (std::string(argv[i]) == "--threads" && i + 1 < argc)
        settings.cpu_threads = std::stoi(argv[++i]);
#ifdef HCP_HAS_VIEWPORT
      else if (std::string(argv[i]) == "--capture-output" && i + 1 < argc)
        capture_output = argv[++i];
      else if (std::string(argv[i]) == "--ticks" && i + 1 < argc)
        capture_ticks = viewport::parse_count("--ticks", argv[++i], 1, 1000000);
      else if (std::string(argv[i]) == "--every" && i + 1 < argc)
        capture_every = viewport::parse_count("--every", argv[++i], 1, 1000000);
#endif
      else if (std::string(argv[i]) == "--backend" && i + 1 < argc) {
        const std::string backend = argv[++i];
        if (backend != "cpu")
          throw std::runtime_error(
              "C3 runner supports CPU only; CUDA publication qualification "
              "remains required");
      } else
        throw std::runtime_error("unknown argument");
    }
    engine::Runtime r(settings);
    std::cout << std::setprecision(17)
              << "actual_backend=cpu,threads=" << *settings.cpu_threads
              << ",path=" << (path == HostPath::BulkCopy ? "bulk" : "direct")
              << '\n';
    require(r.arch() == taichi::Arch::x64, "actual CPU required");
    const auto wants = [&](const char *name) {
      return only.empty() || only == name;
    };
    if (only == "ir") {
      collect_compiler_ir(r, path);
      return 0;
    }
    int failures = 0;
    const auto run = [&](const char *name, auto action) {
      try {
        action();
        std::cout << "case_result,name=" << name << " PASS\n";
      } catch (const std::exception &e) {
        ++failures;
        std::cout << "case_failure,name=" << name << ",reason=" << e.what()
                  << '\n';
      }
    };
    if (wants("basic"))
      run("basic", [&] { basic(r, path); });
    if (!quick) {
      if (wants("forces"))
        run("forces", [&] { force_examples(r, path); });
      if (wants("singleton"))
        run("singleton", [&] { singleton_and_capacity(r, path); });
      if (wants("cumulative")) {
        run("cumulative_crossing", [&] { cumulative(r, path, false); });
        run("cumulative_cancellation", [&] { cumulative(r, path, true); });
      }
      if (wants("tree"))
        run("tree", [&] { boundary_tree(r, path); });
      if (wants("publication"))
        run("publication", [&] { publication_diagnostics(r, path); });
    }
    if (wants("moving"))
      run("moving", [&] { moving(r, path); });
    // Native row order is not a hierarchy freshness guarantee. Exercise the
    // same declared physics and settling gate with a different field mapping.
    if (wants("moving_permuted"))
      run("moving_permuted", [&] { moving(r, path, true); });
    if (!quick && wants("letters")) {
      run("letters", [&] {
        const auto data = letter_data(r);
        std::vector<std::vector<std::uint64_t>> count(
            3, std::vector<std::uint64_t>(3));
        count[0][0] = data.denominators[0];
        count[1][1] = data.denominators[1];
        count[1][2] = data.denominators[2];
        Fixture letters(r, path, {{}, {}, {0, 1}}, count, data.denominators,
                        data.masses, data.positions, 8, 6, false, true, true,
                        false, 1024, data.radii);
        moving_case(letters, "letter_and_UTF8_bytes");
      });
    }
#ifdef HCP_HAS_VIEWPORT
    if (!quick && wants("capture"))
      run("capture", [&] {
        capture_current(r, path, capture_output, capture_ticks, capture_every);
      });
#endif
    std::cout
        << "physical_reference,force=independent_long_double_sum,velocity=long_"
           "double_conditioned_on_verified_force,velocity_bound=128u_times_"
           "intermediate_magnitudes,no_activity_epsilon=1\n";
    std::cout << "publication_reference,conditioned_on_verified_final_velocity="
                 "1,long_double_cell_boundary_cases="
              << publication_boundaries << '\n';
    std::cout << "live_hierarchy_matrix,case_failures=" << failures
              << (failures ? " FAIL\n" : " PASS\n");
    return failures ? 1 : 0;
  } catch (const std::exception &e) {
    std::cerr << "live hierarchy: " << e.what() << '\n';
    return 1;
  }
}
