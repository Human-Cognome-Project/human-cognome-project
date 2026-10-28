#include "assembly/active_workbench.h"

#include <algorithm>
#include <chrono>
#include <limits>
#include <set>

namespace assembly {
namespace {
void require(bool ok) {
  if (!ok) throw warm::Error("invalid or incomplete active input");
}
Point point(warm::Point p) { return {p.x, p.y, p.z}; }
void local_ref(const LocalRef &r, const warm::Uuid &instance,
               const warm::Uuid &owner) {
  require(
      !r.object.scope.nil() && r.object.scope == instance && r.owner == owner &&
      !r.version.nil() && r.object.address.size() == 5 &&
      codec::decode_token_id(warm::address_text(r.object.address)).has_value());
  for (const auto &part : r.object.address) require(!part.partial);
}
std::string local_name(const LocalRef &r) {
  return warm::key(
      r.object);  // Version stays in provenance, not field identity.
}
std::string occurrence(const warm::Uuid &study, warm::OccurrenceId id) {
  return occurrence_text({study, std::move(id)});
}
}  // namespace

ActiveAssembly::ActiveAssembly(LetterProjection p, warm::Uuid owner,
                               bool required, std::optional<LocalInput> local)
    : projection_(std::move(p)),
      memberships_(membership_fields(projection_)),
      owner_(std::move(owner)),
      generation_(warm::new_uuid()),
      requires_local_(required),
      local_(std::move(local)) {
  require(!owner_.nil());
  for (const auto &t : projection_.recipe().tools)
    require(t.ref.object.scope.nil());
  if (!local_) return;
  local_ref(local_->source, instance(), owner_);
  std::map<warm::ObjectKey, LocalRef> refs;
  const auto register_ref = [&](const LocalRef &r) {
    local_ref(r, instance(), owner_);
    const auto found = refs.emplace(r.object, r);
    require(found.second || (found.first->second.version == r.version &&
                             found.first->second.owner == r.owner));
  };
  register_ref(local_->source);
  std::set<std::uint64_t> entries;
  for (const auto &o : projection_.occurrences())
    entries.insert(o.key.source.entry);
  std::set<warm::ObjectKey> definitions;
  for (const auto &v : local_->values) {
    register_ref(v.ref);
    require(v.mass > 0 &&
            v.mass <= std::uint64_t(std::numeric_limits<std::int64_t>::max()) &&
            definitions.insert(v.ref.object).second &&
            entries.insert(v.entry).second);
    enclosing_radius(v.centroid, v.components);
  }
  for (const auto &e : local_->context) register_ref(e.target);
}
ActiveAssembly ActiveAssembly::assemble(warm::Bundle shared, warm::Uuid owner,
                                        bool required,
                                        std::optional<LocalInput> local) {
  return ActiveAssembly(project_letters(shared), std::move(owner), required,
                        std::move(local));
}
ActiveAssembly ActiveAssembly::revise(std::optional<LocalInput> local) const {
  auto revised = ActiveAssembly(projection_, owner_, requires_local_, std::move(local));
  revised.focal_fields_ = focal_fields_;
  return revised;
}
ActiveAssembly ActiveAssembly::restart() const {
  auto rebuilt = assemble(projection_.recipe(), owner_, requires_local_, local_);
  rebuilt.focal_fields_ = focal_fields_;
  return rebuilt;
}
ActiveAssembly ActiveAssembly::focus(std::vector<FocalField> fields) const {
  auto focused = revise(local_);
  focused.focal_fields_ = std::move(fields);
  return focused;
}
std::vector<Origin> ActiveAssembly::origins() const {
  std::vector<Origin> out;
  for (const auto &o : projection_.occurrences())
    out.push_back({o.key, o.definition.object, o.definition, {}});
  if (local_)
    for (const auto &v : local_->values)
      out.push_back({{study(), {v.entry, {}}}, v.ref.object, {}, v.ref});
  return out;
}
StudyInput ActiveAssembly::bind(const std::vector<StudyNode> &nodes) const {
  require(inputs_available());
  auto input = memberships_.bind(projection_, nodes);
  input.focal_fields = focal_fields_;
  if (!local_) return input;
  for (const auto &v : local_->values) {
    require(input.capacity < std::numeric_limits<int>::max());
    const auto id = occurrence(study(), {v.entry, {}});
    input.definitions.push_back(
        {local_name(v.ref), v.mass, v.centroid, v.components, {}});
    input.occurrences.push_back({id, local_name(v.ref), input.capacity++, 0,
                                 point(v.initial.position),
                                 point(v.initial.velocity)});
  }
  for (const auto &e : local_->context)
    input.memberships.push_back(
        {occurrence(study(), e.occurrence), local_name(e.target)});
  std::set<warm::OccurrenceId> moved;
  for (const auto &p : local_->placements) {
    require(moved.insert(p.occurrence).second);
    const auto id = occurrence(study(), p.occurrence);
    auto found =
        std::find_if(input.occurrences.begin(), input.occurrences.end(),
                     [&](const auto &o) { return o.id == id; });
    require(found != input.occurrences.end());
    found->position = point(p.initial.position);
    found->velocity = point(p.initial.velocity);
  }
  std::set<warm::ObjectKey> resized;
  for (const auto &g : local_->geometry) {
    require(g.shared_definition.scope.nil() &&
            resized.insert(g.shared_definition).second);
    auto found = std::find_if(
        input.definitions.begin(), input.definitions.end(),
        [&](const auto &d) { return d.id == warm::key(g.shared_definition); });
    require(found != input.definitions.end());
    found->centroid = g.centroid;
    found->components = g.components;
  }
  return input;
}

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
