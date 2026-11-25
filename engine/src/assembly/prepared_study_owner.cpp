#include "assembly/prepared_study_owner.h"
#include "assembly/prepared_occurrence_workspace.h"
#include "assembly/prepared_lod_workspace.h"
#include "assembly/prepared_submission.h"
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
    std::uint64_t plan_allowance, std::uint64_t metadata_allowance, bool editable_occurrences,
    const PreparedLodAlternative *alternative) {
  PreparedOwnerResult out;
  PreparationWork work(metadata_allowance);
  try {
    work.charge(1); // Fixed owner header/control gate before allocation or copies.
    if (!source || !domain || session.nil() || !empty(h)) {
      out.metadata_work = work.result();
      if (domain) out.reservation = domain->result();
      return out;
    }
    if ((editable_occurrences || alternative) &&
        (bindings.character_rollup || (alternative && alternative->bindings.character_rollup))) {
      out.status = AdmissionPlanStatus::Unsupported;
      out.metadata_work = work.result();
      out.reservation = domain->result();
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
    if (alternative) {
      if(!editable_occurrences || !alternative->source || prepared.work.charged>plan_allowance) {
        owner.reset();out.status=AdmissionPlanStatus::InvalidInput;out.metadata_work=work.result();out.reservation=domain->result();return out;
      }
      auto other=StructuralAdmissionPlan::build(*alternative->source,alternative->bindings,
          h.prepared_storage().capacity,domain,plan_allowance-prepared.work.charged);
      out.plan_work=other.work;
      if(out.plan_work.charged<=UINT64_MAX-prepared.work.charged)out.plan_work.charged+=prepared.work.charged;
      if(out.plan_work.next_required) {
        if(out.plan_work.next_required>UINT64_MAX-prepared.work.charged) {
          out.plan_work.status=PreparationStatus::Overflow;
          out.plan_work.next_required=0;
          other.status=AdmissionPlanStatus::Overflow;
        } else out.plan_work.next_required+=prepared.work.charged;
      }
      if(!other.plan) {
        out.status=other.status;out.failure=other.failure;owner.reset();out.metadata_work=work.result();out.reservation=domain->result();return out;
      }
      try {
        owner->lod_=reserve_unique<PreparedLodWorkspace>(domain,[&](auto *p){
          ::new(static_cast<void*>(p))PreparedLodWorkspace(*owner,*alternative->source,std::move(other.plan),work);
        });
      } catch(const warm::Error &) {
        owner.reset();out.status=AdmissionPlanStatus::Unsupported;out.metadata_work=work.result();out.reservation=domain->result();return out;
      }
    } else if (editable_occurrences) {
      const auto &limits=h.prepared_storage().capacity.limits();
      if (limits[field::Budget::Potential] < limits[field::Budget::Particles]) {
        out.status=AdmissionPlanStatus::CapacityExhausted;
        out.failure={field::Budget::Potential,limits[field::Budget::Particles],limits[field::Budget::Potential]};
        owner.reset();
        out.metadata_work=work.result(); out.reservation=domain->result(); return out;
      }
      owner->edits_ = reserve_unique<PreparedOccurrenceWorkspace>(domain,[&](auto *p) {
        ::new (static_cast<void *>(p)) PreparedOccurrenceWorkspace(*owner,work);
      });
    }
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
      occurrences_(ReservationAllocator<AdmissionOccurrenceCommand>(domain.get())),
      field_sources_(ReservationAllocator<AdmissionFieldCommand>(domain.get())),
      context_(h.prepared_storage().particles.context()), generation_(h.prepared_storage().generation),
      initial_ticks_(h.prepared_storage().completed_ticks) {}

PreparedStudyOwner::~PreparedStudyOwner() = default;

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
  // Prepay successful publication's fixed claim checks/assignments and each
  // body's native exact-source check. Preclaim retries do not publish maps.
  work.elements(bodies_.size(),2); work.elements(fields_.size(),1);
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
  if (plan_ || (lod_ ? lod_->live_.size() : edits_ ? edits_->live_.size() : occurrences_.size()) != bodies_.size() ||
      (lod_ ? lod_->field_sources().size() : field_sources_.size()) != fields_.size() ||
      s.generation != generation_ || s.particles.context() != context_ ||
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
const ReservedVector<engine::RecordHandle> &PreparedStudyOwner::bodies(const warm::Uuid &owner, const ExecutionHandle &handle) const {
  require_published(owner,handle); return bodies_;
}
const ReservedVector<engine::RecordHandle> &PreparedStudyOwner::fields(const warm::Uuid &owner, const ExecutionHandle &handle) const {
  require_published(owner,handle); return fields_;
}

PreparedStudyOwner::AssociationRows::Body PreparedStudyOwner::AssociationRows::body(std::size_t ordinal) const {
  const auto &entry = occurrences.at(ordinal);
  const auto &claim = bodies.at(ordinal);
  return {entry,claim,scales.at(entry.node),exact_masses.at(claim.record)};
}
PreparedStudyOwner::AssociationRows::Field PreparedStudyOwner::AssociationRows::field(std::size_t ordinal) const {
  return {field_sources.at(ordinal),fields.at(ordinal)};
}
PreparedStudyOwner::AssociationView::AssociationView(const PreparedStudyOwner &owner, std::uint64_t tick)
    : owner_(&owner), generation_(owner.generation_), tick_(tick) {}
PreparedStudyOwner::AssociationRows PreparedStudyOwner::AssociationView::read() const {
  // All lifetime/currentness checks precede loaning semantic rows/native ledger.
  // ready() retains full context/incarnation/address/record validation.
  if (generation_ != owner_->generation_ || !owner_->ready() ||
      tick_ != owner_->h_.prepared_storage().completed_ticks)
    throw warm::Stale("prepared owner: stale association view");
  return {*owner_->source_,{owner_->lod_ ? owner_->lod_->catalogue_ : owner_->edits_ ? owner_->edits_->catalogue_ : owner_->occurrences_,
                          owner_->lod_ ? &owner_->lod_->live_ : owner_->edits_ ? &owner_->edits_->live_ : nullptr},
          owner_->lod_ ? owner_->lod_->field_sources() : owner_->field_sources_,owner_->bodies_,owner_->fields_,
          owner_->scales_,owner_->h_.prepared_storage().source_masses};
}
PreparedStudyOwner::AssociationView PreparedStudyOwner::associations(const warm::Uuid &owner,
    const ExecutionHandle &handle) const {
  require_published(owner,handle);
  return AssociationView(*this,h_.prepared_storage().completed_ticks);
}

ActiveReport PreparedStudyOwner::report_metadata(const warm::Uuid &authorization,
    const ExecutionHandle &handle) const {
  const auto rows = associations(authorization,handle).read();
  ActiveReport report;
  report.handle = handle_;
  report.prepared_generation = generation_;
  report.prepared_completed_tick = h_.prepared_storage().completed_ticks;
  if(lod_)report.prepared_exposure=lod_->exposure();
  report.owner = rows.source.owner();
  report.instance = rows.source.instance();
  report.origins = rows.source.origins();
  auto &mapping = report.prepared;
  mapping.capacity = h_.particle_capacity();
  mapping.scales.assign(rows.scales.begin(),rows.scales.end());
  mapping.groups.resize(h_.field_capacity());
  for (std::size_t i=0; i<rows.field_sources.size(); ++i) {
    const auto field = rows.field(i);
    mapping.groups.at(field.claim.record) = {field.source.field.kind,warm::key(field.source.field.object)};
  }
  for (std::size_t i=0; i<rows.occurrences.size(); ++i) {
    const auto body = rows.body(i);
    const auto &entry = body.source;
    if (edits_ || lod_) {
      bool origin=false;
      for (const auto &item : report.origins)
        if (item.occurrence.study == rows.source.study() && item.occurrence.source == entry.occurrence) {
          origin=true; break;
        }
      if (!origin)
        report.origins.push_back({{rows.source.study(),entry.occurrence},entry.definition,{}});
    }
    Occurrence occurrence{InputText(occurrence_text(rows.source.study(),entry.occurrence)),
        InputText(warm::key(entry.definition.object)),body.claim.record,entry.node,{},{}};
    mapping.particles.push_back({std::move(occurrence),body.exact_mass,{},{},0,0});
  }
  const auto &s = h_.prepared_storage();
  for (std::size_t i=0; i<s.live.focus; ++i) mapping.focal_groups.push_back(s.focal[i].record);
  return report;
}
ActiveReport PreparedStudyOwner::report(const warm::Uuid &authorization,
    const ExecutionHandle &handle) {
  auto out = report_metadata(authorization,handle);
  h_.inspect_prepared();
  out.frame_anchor = h_.frame_anchor();
  out.demand = h_.demand();
  out.numerical_profile = h_.numerical_profile();
  out.particles64 = h_.particles64();
  out.groups64 = h_.groups64();
  out.determined64 = h_.determined64();
  require_published(authorization,handle);
  return out;
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
    for (std::size_t i=0; i<bodies_.size(); ++i) {
      const auto command = plan_->occurrences()[i].command;
      const auto claim = resolve(command,s.particles,s.particle_headers);
      if (!commands[command].exact_mass || s.source_masses[claim.record] != *commands[command].exact_mass)
        throw std::runtime_error("prepared owner: inconsistent committed exact source mass");
      bodies_[i] = claim;
    }
    for (std::size_t i=0; i<fields_.size(); ++i)
      fields_[i] = resolve(plan_->fields()[i].command,s.fields,s.field_headers);
    if (s.failure_point == 7)
      throw std::runtime_error("injected failure before prepared owner publication");
    // These vectors share the same reservation allocator. Their constant-time
    // swaps transfer all nested pins without allocating or copying keys. No
    // association/plan loan exists before Published. Retire replay data only
    // after final claim verification; native storage now owns exact masses.
    if(lod_)lod_->sides_[0].plan=std::move(plan_);
    else {
      occurrences_.swap(plan_->occurrences_);
      field_sources_.swap(plan_->fields_);
      plan_.reset();
    }
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
    if (result.status == field::TickStatus::Ok) {
      if (result.context != context_ || result.generation != h_.prepared_storage().generation)
        throw std::runtime_error("prepared owner: inconsistent completed tick identity");
      // Contact coverage is native source state: a successful candidate tick
      // can advance its generation while preserving occurrence/field handles.
      generation_ = result.generation;
      if (!ready()) throw std::runtime_error("prepared owner: stale maps after completed tick");
    }
    if (!h_.ready()) phase_ = PreparedOwnerPhase::Failed;
    return result;
  } catch (...) {
    h_.ready_ = false; phase_ = PreparedOwnerPhase::Failed; throw;
  }
}
}
