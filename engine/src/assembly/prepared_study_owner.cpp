#include "assembly/prepared_study_owner.h"
#include "field/cascade_source.h"
#include <new>
#include <stdexcept>

namespace assembly::detail {
namespace {
ExecutionHandle empty_handle(const std::shared_ptr<ReservationDomain> &domain) {
  // Independent allocating owners avoid GCC 11 nested aggregate unwind gaps.
  auto session = warm::Uuid(domain);
  auto study = warm::Uuid(domain);
  auto generation = warm::Uuid(domain);
  return {std::move(session),std::move(study),std::move(generation)};
}
bool empty(const field::Harness &h) {
  if (!h.prepared() || !h.ready()) return false;
  const auto &s = h.prepared_storage();
  return !s.populated && !s.live.references() && !h.live_hierarchy_installed();
}
}

PreparedOwnerResult PreparedStudyOwner::prepare(field::Harness &h,
    const std::shared_ptr<const ActiveAssembly> &source, const warm::Uuid &session,
    const AdmissionBindings &bindings, const std::shared_ptr<ReservationDomain> &domain,
    std::uint64_t plan_allowance, std::uint64_t metadata_allowance) {
  PreparedOwnerResult out;
  PreparationWork work(metadata_allowance);
  try {
    work.charge(1); // Fixed owner header/control gate before allocation or copies.
    if (!source || !domain || session.nil() || !empty(h)) {
      out.metadata_work = work.result();
      if (domain) out.reservation = domain->result();
      return out;
    }
    auto prepared = StructuralAdmissionPlan::build(*source,bindings,
        h.prepared_storage().capacity,domain,plan_allowance);
    out.plan_work = prepared.work;
    out.failure = prepared.failure;
    out.status = prepared.status;
    if (!prepared.plan) { out.reservation = domain->result(); out.metadata_work = work.result(); return out; }
    auto owner = reserve_unique<PreparedStudyOwner>(domain,[&](auto *p) {
      ::new (static_cast<void *>(p)) PreparedStudyOwner(h,source,std::move(prepared.plan),domain);
    });
    owner->reserve_metadata(session,bindings,work);
    out.owner = std::move(owner);
    out.status = AdmissionPlanStatus::Ok;
  } catch (const PreparationStop &stop) {
    out.status = stop.result.status == PreparationStatus::Overflow
        ? AdmissionPlanStatus::Overflow : AdmissionPlanStatus::WorkExhausted;
    out.metadata_work = stop.result;
  } catch (const ReservationStop &) {
    out.status = domain->result().status == ReservationStatus::Overflow
        ? AdmissionPlanStatus::Overflow : AdmissionPlanStatus::AllocationExhausted;
  } catch (const std::bad_alloc &) {
    out.status = AdmissionPlanStatus::AllocationExhausted;
  }
  if (out.metadata_work.status == PreparationStatus::Ok) out.metadata_work = work.result();
  if (domain) out.reservation = domain->result();
  return out;
}

PreparedStudyOwner::PreparedStudyOwner(field::Harness &h,
    const std::shared_ptr<const ActiveAssembly> &source,
    ReservedUnique<StructuralAdmissionPlan> plan, const std::shared_ptr<ReservationDomain> &domain)
    : h_(h), domain_(domain), source_(source), plan_(std::move(plan)), handle_(empty_handle(domain)),
      bodies_(ReservationAllocator<engine::RecordHandle>(domain.get())),
      fields_(ReservationAllocator<engine::RecordHandle>(domain.get())),
      scales_(ReservationAllocator<ScaleBinding>(domain.get())),
      context_(h.prepared_storage().particles.context()), generation_(h.prepared_storage().generation),
      initial_ticks_(h.prepared_storage().completed_ticks) {}

void PreparedStudyOwner::reserve_metadata(const warm::Uuid &session,
    const AdmissionBindings &bindings, PreparationWork &work) {
  work.copy_key(session); work.copy_key(source_->study()); work.copy_key(source_->generation());
  handle_.session = session; handle_.study = source_->study(); handle_.generation = source_->generation();
  work.elements(plan_->occurrences().size(),sizeof(engine::RecordHandle));
  bodies_.resize(plan_->occurrences().size());
  work.elements(plan_->fields().size(),sizeof(engine::RecordHandle));
  fields_.resize(plan_->fields().size());
  work.elements(bindings.layout.size(),sizeof(ScaleBinding));
  scales_.reserve(bindings.layout.size());
  for (const auto &layout : bindings.layout) {
    work.charge(1);
    scales_.push_back({{nullptr,layout.node.parent,layout.node.role},layout.units});
  }
  // Prepay one successful publication's fixed handle verification/assignment
  // per entry. Preclaim retries never consume this work or publish partial maps.
  work.elements(bodies_.size(),1); work.elements(fields_.size(),1);
}

void PreparedStudyOwner::authorize(const warm::Uuid &owner, const ExecutionHandle &handle) const {
  if (owner != source_->owner()) throw warm::Unauthorized("prepared owner: wrong owner");
  if (handle.session != handle_.session || handle.study != handle_.study ||
      handle.generation != handle_.generation)
    throw warm::Stale("prepared owner: stale or foreign logical handle");
}
bool PreparedStudyOwner::ready() const {
  if (phase_ != PreparedOwnerPhase::Published || !h_.ready()) return false;
  const auto &s = h_.prepared_storage();
  if (s.generation != generation_ || s.particles.context() != context_ ||
      s.live.particles != bodies_.size() || s.live.fields != fields_.size()) return false;
  const auto current = [](const auto &handles, const auto &headers, std::uint64_t context) {
    for (const auto &v : handles)
      if (v.context != context || !v.incarnation || v.record < 0 ||
          std::size_t(v.record) >= headers.size() ||
          !field::detail::same_handle(v,headers[v.record])) return false;
    return true;
  };
  return current(bodies_,s.particle_headers,s.particles.context()) &&
         current(fields_,s.field_headers,s.fields.context());
}
void PreparedStudyOwner::require_published(const warm::Uuid &owner, const ExecutionHandle &handle) const {
  authorize(owner,handle);
  if (!ready()) throw warm::Stale("prepared owner: unpublished or stale execution");
}
const ActiveAssembly &PreparedStudyOwner::source(const warm::Uuid &owner, const ExecutionHandle &handle) const {
  require_published(owner,handle); return *source_;
}
const StructuralAdmissionPlan &PreparedStudyOwner::plan(const warm::Uuid &owner, const ExecutionHandle &handle) const {
  require_published(owner,handle); return *plan_;
}
const ReservedVector<engine::RecordHandle> &PreparedStudyOwner::bodies(const warm::Uuid &owner, const ExecutionHandle &handle) const {
  require_published(owner,handle); return bodies_;
}
const ReservedVector<engine::RecordHandle> &PreparedStudyOwner::fields(const warm::Uuid &owner, const ExecutionHandle &handle) const {
  require_published(owner,handle); return fields_;
}

field::UpdateResult PreparedStudyOwner::bootstrap(const warm::Uuid &owner,
    const ExecutionHandle &handle, std::uint64_t placement_work,
    std::uint64_t cells, std::uint64_t hierarchy_work) {
  authorize(owner,handle);
  field::UpdateResult result;
  const auto &s = h_.prepared_storage();
  result.generation = s.generation;
  if (phase_ == PreparedOwnerPhase::Failed || !h_.ready()) {
    result.status = field::UpdateStatus::Unready; return result;
  }
  if (s.particles.context() != context_ || s.generation != generation_) {
    result.status = field::UpdateStatus::Stale; return result;
  }
  if (phase_ == PreparedOwnerPhase::Published) {
    result.status = ready() ? field::UpdateStatus::NoChange : field::UpdateStatus::Stale;
    return result;
  }
  if (!empty(h_) || s.completed_ticks != initial_ticks_) return result;
  const auto &commands = plan_->commands();
  const auto &intrinsics = plan_->intrinsics();
  const auto &focus = plan_->focus();
  try {
    result = h_.stage_current_admission(generation_,commands.data(),commands.size(),
        intrinsics.data(),intrinsics.size(),plan_->hierarchy(),placement_work,cells,hierarchy_work,
        true,true,focus.data(),focus.size());
    if (result.status != field::UpdateStatus::Ok) {
      if (!h_.ready()) phase_ = PreparedOwnerPhase::Failed;
      return result;
    }
    // Native claims/source/numerics/hierarchy/focus/contacts committed together.
    // No callbacks, copies of keys, allocation or ordinary refusal remain here.
    if (!s.populated || !h_.live_hierarchy_installed() || s.generation != result.generation ||
        s.completed_ticks != initial_ticks_ || s.live.particles != bodies_.size() ||
        s.live.fields != fields_.size())
      throw std::runtime_error("prepared owner: incomplete committed bootstrap");
    const auto resolve = [&](int command, const auto &map, const auto &headers) {
      if (command < 0 || std::size_t(command) >= commands.size())
        throw std::runtime_error("prepared owner: invalid sealed command index");
      const auto h = s.resolved[command];
      if (h.context != map.context() || !h.incarnation || h.address != commands[command].address ||
          h.record < 0 || std::size_t(h.record) >= headers.size() ||
          !field::detail::same_handle(h,headers[h.record]))
        throw std::runtime_error("prepared owner: invalid actual native claim");
      return h;
    };
    for (std::size_t i=0; i<bodies_.size(); ++i)
      bodies_[i] = resolve(plan_->occurrences()[i].command,s.particles,s.particle_headers);
    for (std::size_t i=0; i<fields_.size(); ++i)
      fields_[i] = resolve(plan_->fields()[i].command,s.fields,s.field_headers);
    if (s.failure_point == 7)
      throw std::runtime_error("injected failure before prepared owner publication");
    generation_ = result.generation;
    phase_ = PreparedOwnerPhase::Published; // The only map visibility boundary.
    return result;
  } catch (...) {
    h_.ready_ = false;
    phase_ = PreparedOwnerPhase::Failed;
    throw;
  }
}
field::TickResult PreparedStudyOwner::tick(const warm::Uuid &owner,
    const ExecutionHandle &handle, const field::Parameters64 &parameters,
    std::uint64_t tick_work, std::uint64_t geometry_work) {
  authorize(owner,handle);
  if (!ready()) { field::TickResult out; out.status = field::TickStatus::Unready; return out; }
  try {
    const auto result = h_.tick_prepared(generation_,parameters,tick_work,geometry_work);
    if (!h_.ready()) phase_ = PreparedOwnerPhase::Failed;
    return result;
  } catch (...) {
    h_.ready_ = false; phase_ = PreparedOwnerPhase::Failed; throw;
  }
}
}
