#include "assembly/active_workbench.h"
#include "assembly/prepared_submission.h"
#include "assembly/prepared_study_owner.h"
#include "assembly/prepared_occurrence_edit.h"
#include "field/prepared_storage.h"
#include "engine/snode_tree_owner.h"

#include <chrono>

namespace assembly {
namespace {
void require(bool ok) {
  if (!ok) throw warm::Error("invalid or incomplete active input");
}

// The session checks source/owner/capability/duplicate identity first. This
// borrowed request preflight runs before staging or native allocation.
bool preflight_prepared_request(const PreparedStudySubmission &request,
                                const warm::Bundle &recipe,
                                PreparedSubmissionResult &out) {
  const auto &source = request.source;
  const auto &b = request.bindings;
  if (b.source.owner != source->owner() || b.source.study != source->study() ||
      b.source.generation != source->generation() ||
      b.source.recipe.selection != recipe.ref.selection ||
      b.source.recipe.revision != recipe.ref.revision || b.focus.empty())
    return false;
  // The supported letter subset declares root plus an optional primary step.
  // No caller-owned native pointer crosses this setup boundary.
  if (b.layout.size() != recipe.nodes.size() || b.layout.empty() || b.layout.size() > 2) {
    out.preparation_status = detail::AdmissionPlanStatus::Unsupported;
    return false;
  }
  for (std::size_t i=0; i<b.layout.size(); ++i) {
    const auto &v = b.layout[i];
    if (v.node.layout || v.node.parent != (i ? 0 : -1) ||
        v.node.role != (i ? ScaleRole::PrimaryStep : ScaleRole::Root) ||
        v.node.parent != recipe.nodes[i].parent || int(v.node.role) != int(recipe.nodes[i].role) ||
        v.units != (i ? 64. : 1.)) {
      out.preparation_status = detail::AdmissionPlanStatus::Unsupported;
      return false;
    }
  }
  for (double coordinate : b.frame_anchor) if (coordinate != 0) {
    out.preparation_status = detail::AdmissionPlanStatus::Unsupported;
    return false;
  }
  const auto &limits = request.capacity.limits();
  const auto scratch = field::detail::PreparedStorage::required_scratch(
      limits,false,request.capacity.profile());
  if (scratch > limits[field::Budget::ScratchBytes]) {
    out.preparation_status = detail::AdmissionPlanStatus::CapacityExhausted;
    out.failure = {field::Budget::ScratchBytes,scratch,limits[field::Budget::ScratchBytes]};
    return false;
  }
  return true;
}

// Called only with preflight-validated bindings. The caller registers the
// returned graph and retains the existing candidate's retirement guard.
std::unique_ptr<taichi::lang::SNode> make_prepared_source_layout(
    detail::AdmissionBindings &b) {
  using namespace taichi::lang;
  auto root = std::make_unique<SNode>(0,SNodeType::root);
  auto *parent = root.get();
  b.layout[0].node.layout = parent;
  if (b.layout.size() == 2) {
    parent = &parent->dense(Axis(0),1);
    b.layout[1].node.layout = parent;
  }
  parent->insert_children(SNodeType::place).dt = PrimitiveType::i32;
  return root;
}
}  // namespace

struct ActiveSession::Execution {
  explicit Execution(engine::Runtime &r) { layout.program = &r.program(); }
  Execution(engine::Runtime &r, ActiveAssembly a) : Execution(r) {
    assembly.emplace(std::move(a));
  }
  // Reverse member destruction drains first, then releases prepared before
  // Harness and legacy FixedStudy before the source tree, while Program lives.
  engine::SNodeTreeOwner layout;
  std::optional<ActiveAssembly> assembly; // Legacy value ownership is unchanged.
  std::vector<StudyNode> nodes;
  std::unique_ptr<FixedStudy> study;
  warm::Uuid authorization;
  std::unique_ptr<field::Harness> harness;
  detail::ReservedUnique<detail::PreparedStudyOwner> prepared;
  std::uint64_t completed_ticks{0};
  engine::SNodeTreeOwner::Drain drain{layout};
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
      (found->second->prepared ? found->second->prepared->handle().generation
                              : found->second->assembly->generation()) != h.generation)
    throw warm::Stale("stale execution generation");
  return *found->second;
}
ExecutionHandle ActiveSession::submit(const ActiveAssembly &a,
                                      const field::Parameters &p,
                                      field::HostPath path) {
  require(a.inputs_available() && !executions_.count(a.study()));
  const auto start = std::chrono::steady_clock::now();
  auto e = std::make_unique<Execution>(*runtime_, a);
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
  e->layout.node = runtime_->program().add_snode_tree(std::move(root), false);
  assembly_ms_ += std::chrono::duration<double, std::milli>(
                      std::chrono::steady_clock::now() - start)
                      .count();
  e->study = std::make_unique<FixedStudy>(*runtime_, input, p, path);
  executions_.emplace(a.study(), std::move(e));
  return {session_, a.study(), a.generation()};
}
PreparedSubmissionResult ActiveSession::submit_prepared(PreparedStudySubmission request,
                                                        const warm::Uuid &authorization) {
  PreparedSubmissionResult out;
  const auto &source = request.source;
  auto &b = request.bindings;
  if (!source || !request.resources) return out;
  if (authorization != source->owner()) throw warm::Unauthorized("active owner required");
  const auto &recipe = source->shared_recipe();
  if (recipe.recipe_version != 2 || !source->inputs_available() || source->local() ||
      !request.capacity.profile().wide() || runtime_->arch() != taichi::Arch::x64) {
    out.preparation_status = detail::AdmissionPlanStatus::Unsupported;
    return out;
  }
  if (executions_.count(source->study())) return out;
  if (!preflight_prepared_request(request,recipe,out)) return out;
  const auto start = std::chrono::steady_clock::now();
  // Reserve the publication map node before native publication. A successful
  // bootstrap transfers this node without allocation; failures never enter the
  // session's visible execution map or replace an existing execution.
  decltype(executions_) staging;
  auto candidate = staging.emplace(source->study(),std::make_unique<Execution>(*runtime_)).first;
  auto &e = *candidate->second;
  e.authorization = authorization;
  using namespace taichi::lang;
  auto root = make_prepared_source_layout(b);
  e.layout.node = runtime_->program().add_snode_tree(std::move(root),false);
  e.harness = std::make_unique<field::Harness>(*runtime_,request.capacity,request.path);
  auto prepared = detail::PreparedStudyOwner::prepare(*e.harness,source,session_,b,
      request.resources,request.plan_work,request.metadata_work,request.editable_occurrences);
  out.preparation_status = prepared.status;
  out.plan_work = prepared.plan_work;
  out.metadata_work = prepared.metadata_work;
  out.failure = prepared.failure;
  if (!prepared.owner) {
    staging.clear();
    out.reservation = request.resources->result();
    return out;
  }
  e.prepared = std::move(prepared.owner);
  auto handle = e.prepared->handle(); // Detached return storage before commit.
  out.publication = e.prepared->bootstrap(authorization,handle,request.placement_work,
      request.placement_cells,request.hierarchy_work);
  if (out.publication.status != field::UpdateStatus::Ok) {
    out.failure = out.publication.failure;
    staging.clear();
    out.reservation = request.resources->result();
    return out;
  }
  require(e.prepared->ready());
  const auto published = executions_.insert(staging.extract(candidate));
  require(published.inserted);
  out.handle.emplace(std::move(handle));
  out.reservation = request.resources->result();
  assembly_ms_ += std::chrono::duration<double,std::milli>(
      std::chrono::steady_clock::now()-start).count();
  return out;
}
ExecutionHandle ActiveSession::refresh(const ExecutionHandle &old,
                                       const ActiveAssembly &a,
                                       const field::Parameters &p) {
  auto &e = get(old);
  require(!e.prepared);
  // Invalidate even if binding/validation fails before FixedStudy::publish.
  // A failed target replaces the generation; no old result can be presented.
  require(a.study() == old.study && a.owner() == e.assembly->owner() &&
          a.instance() == e.assembly->instance() &&
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
    const auto &e = get(h);
    return e.prepared ? e.prepared->ready() : e.study->ready();
  } catch (const warm::Stale &) {
    return false;
  }
}
void ActiveSession::tick(const ExecutionHandle &h, const field::Parameters &p) {
  auto &e = get(h);
  require(!e.prepared);
  e.study->tick(p);
  ++e.completed_ticks;
}
PreparedOccurrenceEditResult ActiveSession::edit_prepared(const ExecutionHandle &h,
    const warm::Uuid &authorization, const PreparedOccurrenceEdit &request) {
  auto *owner = prepared_owner(h,authorization);
  require(owner != nullptr);
  return owner->edit(authorization,h,request);
}
field::TickResult ActiveSession::tick_prepared(const ExecutionHandle &h,
    const warm::Uuid &authorization, const field::Parameters64 &parameters,
    std::uint64_t tick_work, std::uint64_t geometry_work) {
  auto *owner = prepared_owner(h,authorization);
  require(owner != nullptr);
  return owner->tick(authorization,h,parameters,tick_work,geometry_work);
}
detail::PreparedStudyOwner *ActiveSession::prepared_owner(const ExecutionHandle &h,
    const warm::Uuid &authorization) const {
  auto &e = get(h);
  if (e.prepared) {
    e.prepared->source(authorization,h); // Authorization and current publication.
    return e.prepared.get();
  }
  if (authorization != e.assembly->owner()) throw warm::Unauthorized("active owner required");
  return nullptr;
}
std::uint64_t ActiveSession::completed_ticks(const ExecutionHandle &h) const {
  const auto &e = get(h);
  return e.prepared ? e.harness->prepared_storage().completed_ticks : e.completed_ticks;
}
ActiveReport ActiveSession::report(const ExecutionHandle &h,
                                   const warm::Uuid &owner) {
  auto &e = get(h);
  if (e.prepared) return e.prepared->report(owner,h);
  if (owner != e.assembly->owner())
    throw warm::Unauthorized("active owner required");
  require(e.study->ready());
  e.study->download();
  auto prepared = e.study->prepared();
  // Reports retain roles/units, never dangling native SNode pointers.
  for (auto &s : prepared.scales) s.node.layout = nullptr;
  const auto &f = e.study->fields();
  return {h,
          owner,
          e.assembly->instance(),
          e.assembly->origins(),
          e.assembly->local(),
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
  const auto &source = e.prepared ? e.prepared->source(owner,r.handle) : *e.assembly;
  if (r.owner != owner || owner != source.owner() ||
      r.instance != source.instance())
    throw warm::Unauthorized("active owner required");
  require(e.prepared ? e.prepared->ready() : e.study->ready());
  if (e.prepared && (r.prepared_generation != std::optional<std::uint64_t>(e.prepared->generation()) ||
                    r.prepared_completed_tick != std::optional<std::uint64_t>(completed_ticks(r.handle))))
    throw warm::Stale("prepared report: historical generation or completed tick");
}
field::PhaseTimes ActiveSession::timings(const ExecutionHandle &h) const {
  const auto &e = get(h);
  return e.prepared ? e.harness->timings() : e.study->fields().timings();
}
field::Demand ActiveSession::demand(const ExecutionHandle &h, const warm::Uuid &owner) const {
  const auto &e = get(h);
  if (e.prepared) {
    e.prepared->source(owner,h);
    return e.harness->demand();
  }
  if (owner != e.assembly->owner()) throw warm::Unauthorized("active owner required");
  require(e.study->ready());
  return e.study->demand();
}
}  // namespace assembly
