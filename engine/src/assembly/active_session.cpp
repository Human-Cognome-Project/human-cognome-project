#include "assembly/active_workbench.h"

#include <chrono>

namespace assembly {
namespace {
void require(bool ok) {
  if (!ok) throw warm::Error("invalid or incomplete active input");
}
}  // namespace

struct ActiveSession::Execution {
  explicit Execution(ActiveAssembly a) : assembly(std::move(a)) {}
  ActiveAssembly assembly;
  std::vector<StudyNode> nodes;
  std::unique_ptr<FixedStudy> study;
  std::uint64_t completed_ticks{0};
};
ActiveSession::ActiveSession(const engine::InstanceSettings &settings)
    : runtime_(std::make_unique<engine::Runtime>(settings)) {}
ActiveSession::~ActiveSession() {
  runtime_->synchronize();
  executions_.clear();
  runtime_.reset();
}
ActiveSession::Execution &ActiveSession::get(const ExecutionHandle &h) const {
  if (h.session != session_) throw warm::Stale("stale execution session");
  const auto found = executions_.find(h.study);
  if (found == executions_.end() ||
      found->second->assembly.generation() != h.generation)
    throw warm::Stale("stale execution generation");
  return *found->second;
}
ExecutionHandle ActiveSession::submit(const ActiveAssembly &a,
                                      const field::Parameters &p,
                                      field::HostPath path) {
  require(a.inputs_available() && !executions_.count(a.study()));
  const auto start = std::chrono::steady_clock::now();
  auto e = std::make_unique<Execution>(a);
  using namespace taichi::lang;
  auto root = std::make_unique<SNode>(0, SNodeType::root);
  auto *parent = root.get();
  e->nodes.push_back({parent, -1, ScaleRole::Root});
  for (std::size_t i = 1; i < a.shared_recipe().nodes.size(); ++i) {
    require(a.shared_recipe().nodes[i].role == warm::ScaleRole::PrimaryStep &&
            i == 1);
    parent = &parent->dense(Axis(0), 1);
    e->nodes.push_back({parent, 0, ScaleRole::PrimaryStep});
  }
  auto &leaf = parent->insert_children(SNodeType::place);
  leaf.dt = PrimitiveType::i32;
  // All native pointers are scoped to this session. Logical records carry
  // roles.
  const auto input = a.bind(e->nodes);
  prepare(
      input);  // Completeness/geometry/conversion before allocation/compile.
  runtime_->program().add_snode_tree(std::move(root), false);
  assembly_ms_ += std::chrono::duration<double, std::milli>(
                      std::chrono::steady_clock::now() - start)
                      .count();
  e->study = std::make_unique<FixedStudy>(*runtime_, input, p, path);
  executions_.emplace(a.study(), std::move(e));
  return {session_, a.study(), a.generation()};
}
ExecutionHandle ActiveSession::refresh(const ExecutionHandle &old,
                                       const ActiveAssembly &a,
                                       const field::Parameters &p) {
  auto &e = get(old);
  // Invalidate even if binding/validation fails before FixedStudy::publish.
  // A failed target replaces the generation; no old result can be presented.
  require(a.study() == old.study && a.owner() == e.assembly.owner() &&
          a.instance() == e.assembly.instance() &&
          a.generation() != old.generation);
  e.assembly = a;
  e.completed_ticks = 0;
  try {
    e.study->publish(a.bind(e.nodes), p);
  } catch (...) {
    // publish invalidates before preparation; incomplete bind must do so too.
    StudyInput invalid;
    try {
      e.study->publish(invalid, p);
    } catch (...) {
    }
    throw;
  }
  return {session_, a.study(), a.generation()};
}
bool ActiveSession::ready(const ExecutionHandle &h) const {
  try {
    return get(h).study->ready();
  } catch (const warm::Stale &) {
    return false;
  }
}
void ActiveSession::tick(const ExecutionHandle &h, const field::Parameters &p) {
  auto &e = get(h);
  e.study->tick(p);
  ++e.completed_ticks;
}
std::uint64_t ActiveSession::completed_ticks(const ExecutionHandle &h) const {
  return get(h).completed_ticks;
}
ActiveReport ActiveSession::report(const ExecutionHandle &h,
                                   const warm::Uuid &owner) {
  auto &e = get(h);
  if (owner != e.assembly.owner())
    throw warm::Unauthorized("active owner required");
  require(e.study->ready());
  e.study->download();
  auto prepared = e.study->prepared();
  // Reports retain roles/units, never dangling native SNode pointers.
  for (auto &s : prepared.scales) s.node.layout = nullptr;
  const auto &f = e.study->fields();
  return {h,
          owner,
          e.assembly.instance(),
          e.assembly.origins(),
          e.assembly.local(),
          std::move(prepared),
          f.particle_data,
          f.group_data,
          f.determined(),
          f.frame_anchor(),
          f.demand(),
          f.numerical_profile(), {}, {}, {}};
}
void ActiveSession::accept(const ActiveReport &r,
                           const warm::Uuid &owner) const {
  auto &e = get(r.handle);
  if (r.owner != owner || owner != e.assembly.owner() ||
      r.instance != e.assembly.instance())
    throw warm::Unauthorized("active owner required");
  require(e.study->ready());
}
field::PhaseTimes ActiveSession::timings(const ExecutionHandle &h) const {
  return get(h).study->fields().timings();
}
field::Demand ActiveSession::demand(const ExecutionHandle &h, const warm::Uuid &owner) const {
  const auto &e = get(h);
  if (owner != e.assembly.owner()) throw warm::Unauthorized("active owner required");
  require(e.study->ready());
  return e.study->demand();
}
}  // namespace assembly
