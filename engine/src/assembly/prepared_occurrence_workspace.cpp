#include "assembly/prepared_occurrence_workspace.h"
#include "field/prepared_storage.h"
#include "field/cascade_source.h"
#include <algorithm>
#include <stdexcept>
#include <type_traits>

namespace assembly::detail {
namespace {
template<class T> void reserve(ReservedVector<T> &v, std::size_t n, PreparationWork &work) {
  work.elements(n,sizeof(T));
  v.reserve(n);
}
AdmissionOccurrenceCommand copy_source(const AdmissionOccurrenceCommand &source,
    const std::shared_ptr<ReservationDomain> &domain, std::size_t path_capacity, PreparationWork &work) {
  work.copy_key(source.occurrence); work.copy_key(source.definition);
  auto scope=warm::Uuid(source.definition.object.scope,domain);
  codec::Address address(source.definition.object.address.begin(),source.definition.object.address.end(),
      SharedReservationAllocator<codec::AddressElement>(domain));
  auto revision=warm::Uuid(source.definition.revision,domain);
  warm::OccurrencePath path(source.occurrence.path.begin(),source.occurrence.path.end(),
      SharedReservationAllocator<std::uint32_t>(domain));
  work.elements(path_capacity,sizeof(std::uint32_t)); path.reserve(path_capacity);
  warm::OccurrenceId occurrence{source.occurrence.entry,std::move(path)};
  warm::ObjectKey object{std::move(scope),std::move(address)};
  warm::ToolRef definition{std::move(object),std::move(revision)};
  return {std::move(occurrence),std::move(definition),source.command,source.node,source.units};
}
}
PreparedOccurrenceWorkspace::PreparedOccurrenceWorkspace(PreparedStudyOwner &owner,
    PreparationWork &work)
    : live_(ReservationAllocator<std::size_t>(owner.domain_.get())),
      next_(ReservationAllocator<std::size_t>(owner.domain_.get())),
      node_fields_(ReservationAllocator<std::size_t>(owner.domain_.get())),
      catalogue_(ReservationAllocator<AdmissionOccurrenceCommand>(owner.domain_.get())),
      templates_(ReservationAllocator<std::size_t>(owner.domain_.get())),
      addresses_(ReservationAllocator<int>(owner.domain_.get())),
      known_(ReservationAllocator<unsigned char>(owner.domain_.get())),
      incoming_source_(copy_source(owner.plan_->occurrences().front(),owner.domain_,0,work)),
      next_bodies_(ReservationAllocator<engine::RecordHandle>(owner.domain_.get())),
      definitions_(ReservationAllocator<field::UpdateCommand>(owner.domain_.get())),
      commands_(ReservationAllocator<field::UpdateCommand>(owner.domain_.get())),
      intrinsics_(ReservationAllocator<field::PlacementIntrinsic>(owner.domain_.get())),
      participation_(ReservationAllocator<Participation>(owner.domain_.get())),
      nodes_(ReservationAllocator<field::LiveRefreshNode>(owner.domain_.get())),
      children_(ReservationAllocator<int>(owner.domain_.get())),
      contributions_(ReservationAllocator<field::LiveRefreshContribution>(owner.domain_.get())) {
  const auto &plan = *owner.plan_;
  const auto count = plan.occurrences().size();
  const auto &limits = owner.h_.prepared_storage().capacity.limits();
  const auto slots=limits[field::Budget::Particles];
  reserve(live_,slots,work); reserve(next_,slots,work); reserve(next_bodies_,slots,work);
  reserve(owner.bodies_,slots,work);
  reserve(catalogue_,slots,work); reserve(templates_,slots,work);
  reserve(addresses_,slots,work); reserve(known_,slots,work);
  std::size_t path_capacity=0;
  for (const auto &row : plan.occurrences()) {
    work.charge(1); path_capacity=std::max(path_capacity,row.occurrence.path.size());
  }
  work.elements(path_capacity,sizeof(std::uint32_t)); incoming_source_.occurrence.path.reserve(path_capacity);
  for (std::size_t i=0; i<slots; ++i) {
    work.charge(1);
    catalogue_.push_back(copy_source(plan.occurrences()[i<count ? i : 0],owner.domain_,path_capacity,work));
    templates_.push_back(i<count ? i : 0); known_.push_back(i<count);
    int address=-1;
    if (i<count) address=plan.commands()[plan.occurrences()[i].command].address;
    else for (std::uint64_t candidate=0; candidate<limits[field::Budget::Potential]; ++candidate) {
      work.charge(1); bool used=false;
      for (int previous : addresses_) { work.charge(1); used |= previous == int(candidate); }
      for (const auto &row : plan.occurrences()) {
        work.charge(1); used |= plan.commands()[row.command].address == int(candidate);
      }
      if (!used) { address=int(candidate); break; }
    }
    if (address<0) throw std::runtime_error("editable owner: missing potential declaration");
    addresses_.push_back(address);
  }
  reserve(definitions_,count,work); reserve(intrinsics_,count,work);
  reserve(commands_,limits[field::Budget::Batch],work);
  reserve(participation_,plan.commands().size(),work);
  const auto hierarchy = plan.hierarchy();
  reserve(nodes_,hierarchy.node_count,work); reserve(node_fields_,hierarchy.node_count,work);
  reserve(children_,hierarchy.child_count,work);
  reserve(contributions_,limits[field::Budget::CascadeReferences],work);
  for (std::size_t i=0; i<count; ++i) {
    work.charge(1); live_.push_back(i);
    const auto command = plan.occurrences()[i].command;
    definitions_.push_back(plan.commands().at(command));
    bool found = false;
    for (const auto &intrinsic : plan.intrinsics()) {
      work.charge(1);
      if (intrinsic.command == std::uint64_t(command)) {
        intrinsics_.push_back(intrinsic); found = true; break;
      }
    }
    if (!found) throw std::runtime_error("editable owner: missing intrinsic");
  }
  const auto field_index = [&](int command) {
    for (std::size_t i=0; i<plan.fields().size(); ++i) {
      work.charge(1);
      if (plan.fields()[i].command == command) return i;
    }
    throw std::runtime_error("editable owner: missing field");
  };
  for (const auto &command : plan.commands()) {
    work.charge(1);
    if (command.operation != field::UpdateOp::InsertParticipation) continue;
    std::size_t body = count;
    for (std::size_t i=0; i<count; ++i) {
      work.charge(1);
      if (plan.occurrences()[i].command == command.first.command) { body=i; break; }
    }
    if (body == count || !command.exact_count)
      throw std::runtime_error("editable owner: invalid participation template");
    participation_.push_back({body,field_index(command.second.command),
        *command.exact_count,command.ordinal,command.share});
  }
  for (std::size_t i=0; i<hierarchy.node_count; ++i) {
    work.charge(1);
    nodes_.push_back(hierarchy.nodes[i]);
    node_fields_.push_back(field_index(hierarchy.nodes[i].field_command));
  }
  for (std::size_t i=0; i<hierarchy.child_count; ++i) {
    work.charge(1); children_.push_back(hierarchy.children[i]);
  }
}

field::UpdateResult PreparedOccurrenceWorkspace::prepare(const PreparedStudyOwner &owner,
    const PreparedOccurrenceEdit &request, PreparationWork &work) {
  using field::UpdateCommand; using field::UpdateOp; using field::UpdateRef;
  field::UpdateResult result;
  const auto &s = owner.h_.prepared_storage();
  result.generation = s.generation;
  if (request.expected_generation != s.generation) { result.status=field::UpdateStatus::Stale; return result; }
  if (!work.equal(request.occurrence.study,owner.source_->study())) return result;
  std::size_t selected = catalogue_.size();
  for (std::size_t i=0; i<catalogue_.size(); ++i) {
    work.charge(1); if (!known_[i]) continue;
    work.copy_key(request.occurrence.source);
    work.copy_key(catalogue_[i].occurrence);
    if (request.occurrence.source == catalogue_[i].occurrence) { selected=i; break; }
  }
  // A declared destination keeps its immutable template even after its
  // reusable instance metadata has been evicted by another occurrence.
  std::size_t declared_template=definitions_.size();
  if (request.present) for (std::size_t i=0; i<owner.occurrences_.size(); ++i) {
    work.copy_key(request.occurrence.source); work.copy_key(owner.occurrences_[i].occurrence);
    if (request.occurrence.source == owner.occurrences_[i].occurrence) { declared_template=i; break; }
  }
  std::size_t source_template=definitions_.size();
  if (request.definition_pin) {
    if (!request.present || request.clone_from || !request.occurrence.source.path.empty()) return result;
    const auto preferred = selected != catalogue_.size() ? templates_[selected] : declared_template;
    source_template = owner.pinned_template(owner.occurrences_,preferred,*request.definition_pin,work);
    if (source_template == definitions_.size() || owner.occurrences_[source_template].node != 0) return result;
  } else if (request.clone_from) {
    if (!request.present || !request.occurrence.source.path.empty() ||
        !work.equal(request.clone_from->study,owner.source_->study())) return result;
    for (std::size_t i=0; i<owner.occurrences_.size(); ++i) {
      work.copy_key(owner.occurrences_[i].occurrence); work.copy_key(request.clone_from->source);
      if (request.clone_from->source == owner.occurrences_[i].occurrence) { source_template=i; break; }
    }
    if (source_template == definitions_.size() ||
        !owner.occurrences_[source_template].occurrence.path.empty() || owner.occurrences_[source_template].node != 0)
      return result;
    if (declared_template != definitions_.size() && source_template != declared_template) return result;
    if (selected != catalogue_.size() && templates_[selected] != source_template) return result;
  } else if (selected == catalogue_.size() && request.present) {
    source_template=declared_template;
  }
  new_identity_=selected == catalogue_.size();
  if (new_identity_) {
    if (!request.present || source_template == definitions_.size()) return result;
    // Immutable source templates stay available. Reuse an instance metadata
    // slot only after its old occurrence has left the native model.
    for (std::size_t i=0; i<catalogue_.size(); ++i) {
      work.charge(1); bool active=false;
      for (auto ordinal : live_) { work.charge(1); active |= ordinal == i; }
      if (!active) { selected=i; break; }
    }
    if (selected == catalogue_.size()) {
      result.status=field::UpdateStatus::CapacityExhausted;
      result.failure={field::Budget::Particles,live_.size()+1,s.capacity.limits()[field::Budget::Particles]};
      return result;
    }
    // Every destination key buffer was sized at setup. This root-only clone
    // copies the validated declared pin without allocating source metadata.
    work.copy_key(owner.occurrences_[source_template].definition);
    incoming_source_.definition=owner.occurrences_[source_template].definition;
    work.copy_key(request.occurrence.source);
    incoming_source_.occurrence=request.occurrence.source;
    incoming_source_.command=owner.occurrences_[source_template].command;
    incoming_source_.node=owner.occurrences_[source_template].node;
    incoming_source_.units=owner.occurrences_[source_template].units;
  } else source_template=templates_[selected];
  std::size_t at = live_.size();
  for (std::size_t i=0; i<live_.size(); ++i) {
    work.charge(1); if (live_[i] == selected) at=i;
  }
  if (request.present == (at != live_.size())) { result.status=field::UpdateStatus::NoChange; return result; }
  // Fixed comparison/focus fields must retain a nonempty supported cohort.
  if (!request.present && live_.size() == 1) { result.status=field::UpdateStatus::Unsupported; return result; }
  adding_ = request.present;
  incoming_template_=source_template; incoming_slot_=selected; incoming_address_=addresses_[selected];
  commands_.clear(); contributions_.clear(); next_.clear(); next_bodies_.clear();
  const auto append = [&](const UpdateCommand &command) {
    work.charge(sizeof(command));
    const auto limit = s.capacity.limits()[field::Budget::Batch];
    if (commands_.size() >= limit) {
      result.status=field::UpdateStatus::CapacityExhausted;
      result.failure={field::Budget::Batch,commands_.size()+1,limit}; return false;
    }
    commands_.push_back(command); return true;
  };
  engine::RecordHandle outgoing;
  if (adding_) {
    auto definition=definitions_[source_template]; definition.address=incoming_address_;
    if (!append(definition)) return result;
    incoming_=intrinsics_[source_template]; incoming_.command=0; incoming_.order=0;
    std::uint64_t position=s.live.edges;
    for (const auto &row : participation_) {
      work.charge(1); if (row.occurrence != source_template) continue;
      UpdateCommand command; command.operation=UpdateOp::InsertParticipation;
      command.first=UpdateRef::pending(0); command.second=UpdateRef::existing(owner.fields_[row.field]);
      command.position=position++; command.ordinal=row.ordinal;
      command.share=row.share; command.exact_count=row.count;
      if (!append(command)) return result;
    }
  } else {
    outgoing=owner.bodies_[at];
    for (std::size_t i=0; i<s.live.edges; ++i) {
      work.charge(1); if (!field::detail::same_handle(s.edges[i].first,outgoing)) continue;
      UpdateCommand command; command.operation=UpdateOp::RemoveParticipation;
      command.row=i; command.generation=s.generation;
      if (!append(command)) return result;
    }
    for (std::size_t i=0; i<s.live.contacts; ++i) {
      work.charge(1);
      if (!field::detail::same_handle(s.contacts[i].first,outgoing) &&
          !field::detail::same_handle(s.contacts[i].second,outgoing)) continue;
      UpdateCommand command; command.operation=UpdateOp::RemoveContact;
      command.row=i; command.generation=s.generation;
      if (!append(command)) return result;
    }
    UpdateCommand command; command.operation=UpdateOp::RetireParticle;
    command.first=UpdateRef::existing(outgoing);
    if (!append(command)) return result;
  }
  for (std::size_t i=0; i<live_.size(); ++i) {
    work.charge(1); if (!adding_ && i == at) continue;
    next_.push_back(live_[i]); next_bodies_.push_back(owner.bodies_[i]);
  }
  if (adding_) { next_.push_back(selected); next_bodies_.push_back({}); }
  for (std::size_t i=0; i<nodes_.size(); ++i) {
    work.charge(1);
    auto &node=nodes_[i]; node.field=owner.fields_[node_fields_[i]]; node.field_command=-1;
    node.cohort_begin=contributions_.size();
    const auto contribution = [&](const field::LiveRefreshContribution &row) {
      work.charge(sizeof(row));
      const auto limit=s.capacity.limits()[field::Budget::CascadeReferences];
      if (contributions_.size() >= limit) {
        result.status=field::UpdateStatus::CapacityExhausted;
        result.failure={field::Budget::CascadeReferences,contributions_.size()+1,limit}; return false;
      }
      contributions_.push_back(row); return true;
    };
    for (std::size_t row=0; row<s.live.edges; ++row) {
      work.charge(1); const auto &edge=s.edges[row];
      if (!field::detail::same_handle(edge.second,node.field) ||
          (!adding_ && field::detail::same_handle(edge.first,outgoing))) continue;
      if (!contribution({UpdateRef::existing(edge.first),{s.generation,row,edge.ordinal},-1})) return result;
    }
    if (adding_) for (std::size_t command=1; command<commands_.size(); ++command) {
      work.charge(1); const auto &edge=commands_[command];
      if (field::detail::same_handle(edge.second.handle,node.field))
        if (!contribution({UpdateRef::pending(0),{},int(command)})) return result;
    }
    node.cohort_count=contributions_.size()-node.cohort_begin;
    if (!node.cohort_count) { result.status=field::UpdateStatus::Unsupported; return result; }
  }
  result.status=field::UpdateStatus::Ok;
  return result;
}
field::LiveRefreshSpec PreparedOccurrenceWorkspace::hierarchy() const {
  return {nodes_.data(),nodes_.size(),children_.data(),children_.size(),
          contributions_.data(),contributions_.size()};
}
void PreparedOccurrenceWorkspace::publish(PreparedStudyOwner &owner) {
  const auto &s=owner.h_.prepared_storage();
  if (adding_) {
    const auto claim=s.resolved.at(0);
    if (claim.context != s.particles.context() || !claim.incarnation || claim.record < 0 ||
        std::size_t(claim.record) >= s.particle_headers.size() ||
        claim.address != incoming_address_ ||
        !field::detail::same_handle(claim,s.particle_headers[claim.record]) ||
        s.source_masses[claim.record] != *definitions_[incoming_template_].exact_mass)
      throw std::runtime_error("editable owner: invalid committed claim");
    next_bodies_.back()=claim;
    if (new_identity_) {
      static_assert(std::is_nothrow_move_constructible<AdmissionOccurrenceCommand>::value &&
                    std::is_nothrow_move_assignable<AdmissionOccurrenceCommand>::value);
      std::swap(catalogue_[incoming_slot_],incoming_source_);
      templates_[incoming_slot_]=incoming_template_; known_[incoming_slot_]=1;
    }
  }
  owner.bodies_.swap(next_bodies_);
  live_.swap(next_);
}

PreparedOccurrenceEditResult PreparedStudyOwner::edit(const warm::Uuid &authorization,
    const ExecutionHandle &handle, const PreparedOccurrenceEdit &request) {
  authorize(authorization,handle);
  PreparedOccurrenceEditResult out;
  out.publication.generation=h_.prepared_storage().generation;
  if (!ready()) { out.publication.status=field::UpdateStatus::Unready; return out; }
  if(lod_)return edit_lod_presence(request);
  if (!edits_) { out.publication.status=field::UpdateStatus::Unsupported; return out; }
  PreparationWork work(request.preparation_work);
  try { out.publication=edits_->prepare(*this,request,work); }
  catch (const PreparationStop &stop) { out.preparation=stop.result; return out; }
  out.preparation=work.result();
  if (out.publication.status != field::UpdateStatus::Ok) return out;
  const auto tick=h_.prepared_storage().completed_ticks;
  try {
    if (edits_->adding_)
      out.publication=h_.stage_current_admission(generation_,edits_->commands_.data(),edits_->commands_.size(),
          &edits_->incoming_,1,edits_->hierarchy(),request.placement_work,request.placement_cells,
          request.hierarchy_work,true);
    else
      out.publication=h_.refresh_live_hierarchy(generation_,edits_->commands_.data(),edits_->commands_.size(),
          edits_->hierarchy(),request.hierarchy_work);
    if (out.publication.status == field::UpdateStatus::Ok) {
      if (h_.prepared_storage().completed_ticks != tick ||
          h_.prepared_storage().generation != out.publication.generation)
        throw std::runtime_error("editable owner: inconsistent committed edit");
      edits_->publish(*this);
      generation_=out.publication.generation;
      if (!ready()) throw std::runtime_error("editable owner: stale published mapping");
    }
    if (!h_.ready()) phase_=PreparedOwnerPhase::Failed;
  } catch (...) {
    h_.ready_=false; phase_=PreparedOwnerPhase::Failed; throw;
  }
  return out;
}
}  // namespace assembly::detail
