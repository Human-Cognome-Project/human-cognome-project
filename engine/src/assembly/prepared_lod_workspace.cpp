#include "assembly/prepared_lod_workspace.h"
#include "assembly/exchange_contract.h"
#include "assembly/projection_access.h"
#include "field/prepared_storage.h"
#include "field/cascade_source.h"
#include <algorithm>
#include <stdexcept>
#include <type_traits>

namespace assembly::detail {
namespace {
void need(bool v) { if(!v) throw warm::Error("unsupported whole-field alternatives"); }
bool same(const FieldKey &a,const FieldKey &b) {return a.kind==b.kind && a.object==b.object;}
template<class Range,class Value> bool contains(const Range &values,const Value &value,PreparationWork &w) {
  return std::find_if(values.begin(),values.end(),[&](const auto &candidate) {
    w.charge(1);return candidate==value;
  })!=values.end();
}
template<class T> void reserve(ReservedVector<T> &v,std::size_t n,PreparationWork &w) {
  w.elements(n,sizeof(T));v.reserve(n);
}
AdmissionOccurrenceCommand copy(const AdmissionOccurrenceCommand &s,
    const std::shared_ptr<ReservationDomain> &d,std::size_t paths,PreparationWork &w) {
  w.copy_key(s.occurrence);w.copy_key(s.definition);
  auto scope=warm::Uuid(s.definition.object.scope,d);
  codec::Address address(s.definition.object.address.begin(),s.definition.object.address.end(),
      SharedReservationAllocator<codec::AddressElement>(d));
  auto revision=warm::Uuid(s.definition.revision,d);
  warm::OccurrencePath path(s.occurrence.path.begin(),s.occurrence.path.end(),SharedReservationAllocator<std::uint32_t>(d));
  w.elements(paths,sizeof(std::uint32_t));path.reserve(paths);
  return {{s.occurrence.entry,std::move(path)},{{std::move(scope),std::move(address)},std::move(revision)},s.command,s.node,s.units};
}
}
PreparedLodWorkspace::PreparedLodWorkspace(PreparedStudyOwner &o,const ActiveAssembly &alternative,
    ReservedUnique<StructuralAdmissionPlan> plan,bool fixed_character,PreparationWork &w)
    :sides_{{Side(o.domain_.get()),Side(o.domain_.get())}},
     catalogue_(ReservationAllocator<AdmissionOccurrenceCommand>(o.domain_.get())),
     next_catalogue_(ReservationAllocator<AdmissionOccurrenceCommand>(o.domain_.get())),
     incoming_source_(fixed_character ? AdmissionOccurrenceCommand{} :
                      copy(o.plan_->occurrences().front(),o.domain_,2,w)),
     templates_(ReservationAllocator<std::size_t>(o.domain_.get())),live_(ReservationAllocator<std::size_t>(o.domain_.get())),
     next_(ReservationAllocator<std::size_t>(o.domain_.get())),order_(ReservationAllocator<std::size_t>(o.domain_.get())),
     known_(ReservationAllocator<unsigned char>(o.domain_.get())),
     addresses_(ReservationAllocator<int>(o.domain_.get())),field_commands_(ReservationAllocator<int>(o.domain_.get())),
     next_bodies_(ReservationAllocator<engine::RecordHandle>(o.domain_.get())),
     next_fields_(ReservationAllocator<engine::RecordHandle>(o.domain_.get())),outgoing_(ReservationAllocator<engine::RecordHandle>(o.domain_.get())),
     commands_(ReservationAllocator<field::UpdateCommand>(o.domain_.get())),
     intrinsics_(ReservationAllocator<field::PlacementIntrinsic>(o.domain_.get())),
     groups_(ReservationAllocator<field::PlacementExchangeGroup>(o.domain_.get())),
     nodes_(ReservationAllocator<field::LiveRefreshNode>(o.domain_.get())),children_(ReservationAllocator<int>(o.domain_.get())),
     contributions_(ReservationAllocator<field::LiveRefreshContribution>(o.domain_.get())),
     fixed_character_(fixed_character) {
  need(o.source_->projection_.recipe_.get()==alternative.projection_.recipe_.get() &&
       o.source_->study()==alternative.study() && o.source_->owner()==alternative.owner() &&
       !o.source_->local() && !alternative.local());
  const auto &a=*o.plan_; const auto &b=*plan;
  need(!a.occurrences().empty() &&
       a.focus().size()==1 && b.focus().size()==1);
  if (fixed_character_) {
    need(a.occurrences().size()==1 && b.occurrences().size()==2 &&
         o.source_->shared_recipe().contacts.empty());
    const auto &old=a.occurrences().front();
    const auto declaration=LetterExchangeContract::collect(o.source_->projection_,
        {o.source_->study(),old.occurrence},warm::Exposure::Parts,o.domain_,&w,
        o.source_->projection_.recipe_);
    need(old.occurrence.path.empty() && old.node==1 &&
         declaration.outgoing().size()==1 && declaration.incoming().size()==2 &&
         w.equal(declaration.outgoing()[0].definition,old.definition) &&
         alternative.projection_.exposure(old.occurrence.entry)==warm::Exposure::Parts);
    for (std::size_t i=0;i<b.occurrences().size();++i) {
      w.charge(1);
      const auto &byte=b.occurrences()[i];
      need(byte.node==0 && w.equal(declaration.incoming()[i].occurrence.source,byte.occurrence) &&
           w.equal(declaration.incoming()[i].definition,byte.definition) &&
           a.commands()[old.command].address!=b.commands()[byte.command].address);
    }
    for (const auto *side : {&a,&b})
      for (const auto &body : side->intrinsics()) {
        w.charge(1);
        need(body.mass==double(*side->commands()[body.command].exact_mass));
      }
    for (const auto &old_field : a.fields())
      for (const auto &new_field : b.fields()) {
        w.charge(1);
        need((old_field.field.kind==new_field.field.kind &&
              w.equal(old_field.field.object,new_field.field.object)) ==
             (a.commands()[old_field.command].address==b.commands()[new_field.command].address));
      }
  } else need(a.occurrences().size()==b.occurrences().size());
  const auto &limits=o.h_.prepared_storage().capacity.limits();
  const auto slots=limits[field::Budget::Particles],fields=limits[field::Budget::Fields];
  need(slots && (fixed_character_ || slots<=limits[field::Budget::Potential]/2));
  need(o.source_->projection_.exposure(a.occurrences()[0].occurrence.entry)==warm::Exposure::Aggregate);
  for(std::size_t i=0;!fixed_character_ && i<a.occurrences().size();++i) {
    w.charge(1); const auto &old=a.occurrences()[i]; const auto &fresh=b.occurrences()[i];
    need(old.occurrence.path.empty() && old.node==1 && fresh.node==0 &&
         fresh.occurrence.entry==old.occurrence.entry && fresh.occurrence.path.size()==2 &&
         fresh.occurrence.path[0]==0 && fresh.occurrence.path[1]==0 &&
         o.source_->projection_.exposure(old.occurrence.entry)==warm::Exposure::Aggregate &&
         alternative.projection_.exposure(old.occurrence.entry)==warm::Exposure::Parts);
    const auto declaration=LetterExchangeContract::collect(o.source_->projection_,{o.source_->study(),old.occurrence},
        warm::Exposure::Parts,o.domain_,&w,o.source_->projection_.recipe_);
    need(declaration.outgoing().size()==1 && declaration.incoming().size()==1 &&
         declaration.incoming()[0].occurrence.source==fresh.occurrence &&
         declaration.incoming()[0].definition==fresh.definition &&
         old.definition==a.occurrences()[0].definition && fresh.definition==b.occurrences()[0].definition);
    path_capacity_=std::max(path_capacity_,fresh.occurrence.path.size());
  }
  const auto focused=[&](const StructuralAdmissionPlan &p)->const AdmissionFieldCommand& {
    const auto command=p.focus()[0].command;
    const auto it=std::find_if(p.fields().begin(),p.fields().end(),[&](const auto &f){w.charge(1);return f.command==command;});
    need(it!=p.fields().end());return *it;
  };
  if (fixed_character_) {
    const auto &focus_a=focused(a), &focus_b=focused(b);
    const auto hierarchy=b.hierarchy();
    need(focus_a.field.kind==GroupKind::ParentValue && same(focus_a.field,focus_b.field) &&
         focus_b.command==hierarchy.nodes[hierarchy.node_count-1].field_command);
  } else {
    need(focused(a).field.kind==GroupKind::Membership && same(focused(a).field,focused(b).field) &&
         a.commands()[focused(a).command].address==b.commands()[focused(b).command].address);
  }
  sides_[1].plan=std::move(plan);
  for(std::size_t side=0;side<2;++side) {
    const auto &p=side?*sides_[1].plan:a;
    reserve(sides_[side].edges,p.commands().size(),w);
    for(const auto &c:p.commands()) {
      w.charge(1); if(c.operation!=field::UpdateOp::InsertParticipation) continue;
      auto body=p.occurrences().size(),field=p.fields().size();
      for(std::size_t j=0;j<p.occurrences().size();++j) {w.charge(1);if(p.occurrences()[j].command==c.first.command)body=j;}
      for(std::size_t j=0;j<p.fields().size();++j) {w.charge(1);if(p.fields()[j].command==c.second.command)field=j;}
      need(body<p.occurrences().size() && field<p.fields().size() && c.exact_count.has_value());
      sides_[side].edges.push_back({body,field,c});
    }
  }
  if(!fixed_character_) {
    reserve(catalogue_,slots,w);reserve(next_catalogue_,slots,w);reserve(templates_,slots,w);reserve(known_,slots,w);
    reserve(addresses_,2*slots,w);reserve(live_,slots,w);reserve(next_,slots,w);reserve(order_,slots,w);
  }
  reserve(next_bodies_,slots,w);
  reserve(o.bodies_,slots,w);reserve(o.fields_,fields,w);reserve(next_fields_,fields,w);reserve(field_commands_,fields,w);
  reserve(outgoing_,slots,w);reserve(intrinsics_,slots,w);reserve(groups_,slots,w);
  reserve(commands_,limits[field::Budget::Batch],w);reserve(nodes_,limits[field::Budget::CascadeNodes],w);
  reserve(children_,limits[field::Budget::CascadeLinks],w);reserve(contributions_,limits[field::Budget::CascadeReferences],w);
  for(std::size_t i=0;!fixed_character_ && i<slots;++i) {
    w.charge(1);const auto index=i<a.occurrences().size()?i:0;
    catalogue_.push_back(copy(a.occurrences()[index],o.domain_,path_capacity_,w));
    next_catalogue_.push_back(copy(a.occurrences()[index],o.domain_,path_capacity_,w));
    templates_.push_back(index);known_.push_back(i<a.occurrences().size());
    if(i<a.occurrences().size())live_.push_back(i);
    for(std::size_t side=0;side<2;++side) {
      const auto &p=side?*sides_[1].plan:a; int address=-1;
      if(i<a.occurrences().size()) address=p.commands()[p.occurrences()[i].command].address;
      else for(std::size_t candidate=0;candidate<limits[field::Budget::Potential];++candidate) {
        w.charge(1);bool used=contains(addresses_,int(candidate),w);
        for(const auto *q:std::array<const StructuralAdmissionPlan*,2>{{&a,sides_[1].plan.get()}}) for(const auto &row:q->occurrences()) {
          w.charge(1);used|=q->commands()[row.command].address==int(candidate);
        }
        if(!used) {address=int(candidate);break;}
      }
      need(address>=0 && !contains(addresses_,address,w));addresses_.push_back(address);
    }
  }
}
field::UpdateResult PreparedLodWorkspace::presence(const PreparedStudyOwner &o,const PreparedOccurrenceEdit &r,PreparationWork &w) {
  field::UpdateResult out;out.generation=o.generation_;
  if(r.expected_generation!=o.generation_) {out.status=field::UpdateStatus::Stale;return out;}
  if(fixed_character_){out.status=field::UpdateStatus::Unsupported;return out;}
  if(!w.equal(r.occurrence.study,o.source_->study()) || !r.occurrence.source.path.empty())return out;
  const auto &declared=sides_[0].plan->occurrences(); auto immutable=declared.size();
  for(std::size_t i=0;i<declared.size();++i){w.charge(1);if(declared[i].occurrence.entry==r.occurrence.source.entry)immutable=i;}
  selected_=catalogue_.size();
  for(std::size_t i=0;i<catalogue_.size();++i){w.charge(1);if(known_[i] && catalogue_[i].occurrence.entry==r.occurrence.source.entry)selected_=i;}
  source_template_=declared.size();
  if(r.clone_from) {
    if(!r.present || !w.equal(r.clone_from->study,o.source_->study()) || !r.clone_from->source.path.empty()) return out;
    for(std::size_t i=0;i<declared.size();++i){w.charge(1);if(declared[i].occurrence.entry==r.clone_from->source.entry)source_template_=i;}
    if(source_template_==declared.size() || (immutable!=declared.size() && source_template_!=immutable) ||
       (selected_!=catalogue_.size() && templates_[selected_]!=source_template_))return out;
  } else if(selected_!=catalogue_.size())source_template_=templates_[selected_];
  else source_template_=immutable;
  new_identity_=selected_==catalogue_.size();
  if(new_identity_) {
    if(!r.present || source_template_==declared.size())return out;
    for(std::size_t i=0;i<catalogue_.size();++i){w.charge(1);if(!contains(live_,i,w)){selected_=i;break;}}
    if(selected_==catalogue_.size()) {out.status=field::UpdateStatus::CapacityExhausted;out.failure={field::Budget::Particles,live_.size()+1,catalogue_.size()};return out;}
  }
  const bool active=contains(live_,selected_,w);
  if(active==r.present){out.status=field::UpdateStatus::NoChange;return out;}
  if(!r.present && live_.size()==1){out.status=field::UpdateStatus::Unsupported;return out;}
  changing_=false;adding_=r.present;target_=side_;
  if(new_identity_) {
    const auto &row=sides_[side_].plan->occurrences()[source_template_];w.copy_key(row.definition);w.copy_key(row.occurrence);
    incoming_source_=row;incoming_source_.occurrence.entry=r.occurrence.source.entry;
  }
  return build(o,w);
}
field::UpdateResult PreparedLodWorkspace::exposure(const PreparedStudyOwner &o,const PreparedLodEdit &r,PreparationWork &w) {
  field::UpdateResult out;out.generation=o.generation_;
  if(r.expected_generation!=o.generation_){out.status=field::UpdateStatus::Stale;return out;}
  if(r.exposure!=warm::Exposure::Aggregate && r.exposure!=warm::Exposure::Parts)return out;
  target_=r.exposure==warm::Exposure::Parts;
  if(target_==side_){out.status=field::UpdateStatus::NoChange;return out;}
  changing_=true;adding_=false;new_identity_=false;
  for(std::size_t i=0;i<catalogue_.size();++i) {
    w.charge(1);if(!known_[i])continue;
    const auto &row=sides_[target_].plan->occurrences()[templates_[i]];w.copy_key(row.definition);w.copy_key(row.occurrence);
    next_catalogue_[i]=row;next_catalogue_[i].occurrence.entry=catalogue_[i].occurrence.entry;
  }
  return build(o,w);
}
field::UpdateResult PreparedLodWorkspace::build(const PreparedStudyOwner &o,PreparationWork &w) {
  using field::UpdateOp;using field::UpdateRef;
  const auto &s=o.h_.prepared_storage();field::UpdateResult out;out.generation=s.generation;
  const auto &p=*sides_[target_].plan;
  commands_.clear();intrinsics_.clear();groups_.clear();outgoing_.clear();next_.clear();next_bodies_.clear();
  next_fields_.clear();field_commands_.clear();nodes_.clear();children_.clear();contributions_.clear();
  const auto append=[&](field::UpdateCommand c){w.charge(sizeof(c));if(commands_.size()==commands_.capacity()) {out.status=field::UpdateStatus::CapacityExhausted;out.failure={field::Budget::Batch,commands_.size()+1,commands_.capacity()};return false;}commands_.push_back(c);return true;};
  for(std::size_t i=0;i<live_.size();++i) {
    w.charge(1);const auto slot=live_[i];
    if(changing_ || (!adding_ && slot==selected_))outgoing_.push_back(o.bodies_[i]);
    if(!changing_ && !(!adding_ && slot==selected_)){next_.push_back(slot);next_bodies_.push_back(o.bodies_[i]);}
  }
  const auto incoming=[&](std::size_t slot,std::size_t which) {
    w.charge(1);const auto command=p.occurrences()[which].command;
    auto c=p.commands()[command];
    if(!fixed_character_)c.address=addresses_[2*slot+target_];
    const auto index=commands_.size();
    if(!append(c))return false;
    const auto it=std::find_if(p.intrinsics().begin(),p.intrinsics().end(),[&](const auto &v){w.charge(1);return v.command==std::uint64_t(command);});need(it!=p.intrinsics().end());
    auto intrinsic=*it;intrinsic.command=index;intrinsic.order=intrinsics_.size();intrinsics_.push_back(intrinsic);
    if(!fixed_character_)next_.push_back(slot);
    next_bodies_.push_back({});return true;
  };
  if(fixed_character_) {
    for(const auto &body:o.bodies_){w.charge(1);outgoing_.push_back(body);}
    for(std::size_t i=0;i<p.occurrences().size();++i) if(!incoming(i,i))return out;
    groups_.push_back({0,outgoing_.size(),0,intrinsics_.size()});
  } else if(changing_) {
    order_.clear();outgoing_.clear();
    for(const auto slot:live_){w.charge(1);order_.push_back(slot);}
    std::sort(order_.begin(),order_.end(),[&](auto a,auto b){w.charge(1);return catalogue_[a].occurrence.entry<catalogue_[b].occurrence.entry;});
    for(std::size_t i=0;i<order_.size();++i) {
      w.charge(1);const auto slot=order_[i];
      for(std::size_t old=0;old<live_.size();++old){w.charge(1);if(live_[old]==slot)outgoing_.push_back(o.bodies_[old]);}
      if(!incoming(slot,templates_[slot]))return out;
      groups_.push_back({i,1,i,1});
    }
  } else if(adding_ && !incoming(selected_,source_template_))return out;
  const auto &current_fields=field_sources();
  for(const auto &f:p.fields()) {
    w.copy_key(f.field.object);std::size_t at=current_fields.size();
    for(std::size_t i=0;i<current_fields.size();++i){w.charge(1);if(same(current_fields[i].field,f.field))at=i;}
    if(at!=current_fields.size()){next_fields_.push_back(o.fields_[at]);field_commands_.push_back(-1);}
    else {
      field_commands_.push_back(int(commands_.size()));next_fields_.push_back({});
      if(!append(p.commands()[f.command]))return out;
    }
  }
  const auto retired=[&](const engine::RecordHandle &h){for(const auto &v:outgoing_){w.charge(1);if(field::detail::same_handle(v,h))return true;}return false;};
  std::size_t removed=0;
  for(std::size_t row=0;row<s.live.edges;++row) {
    w.charge(1);if(!retired(s.edges[row].first))continue;
    field::UpdateCommand c;c.operation=UpdateOp::RemoveParticipation;c.row=row;c.generation=s.generation;if(!append(c))return out;++removed;
  }
  for(std::size_t row=0;row<s.live.contacts;++row) {
    w.charge(1);if(!retired(s.contacts[row].first)&&!retired(s.contacts[row].second))continue;
    field::UpdateCommand c;c.operation=UpdateOp::RemoveContact;c.row=row;c.generation=s.generation;if(!append(c))return out;
  }
  for(const auto &h:outgoing_) {field::UpdateCommand c;c.operation=UpdateOp::RetireParticle;c.first=UpdateRef::existing(h);if(!append(c))return out;}
  for(std::size_t i=0;i<current_fields.size();++i) {
    w.charge(1);const auto &f=current_fields[i];bool kept=false;for(const auto &n:p.fields()){w.charge(1);kept|=same(f.field,n.field);}
    if(!kept){field::UpdateCommand c;c.operation=UpdateOp::RetireField;c.first=UpdateRef::existing(o.fields_[i]);if(!append(c))return out;}
  }
  std::size_t position=s.live.edges-removed;
  for(std::size_t i=0;i<intrinsics_.size();++i) {
    const auto which=fixed_character_ ? i : changing_ ? templates_[order_[i]] : source_template_;
    for(const auto &edge:sides_[target_].edges) {
      w.charge(1);if(edge.occurrence!=which)continue;
      auto c=edge.command;c.first=UpdateRef::pending(int(intrinsics_[i].command));
      c.second=field_commands_[edge.field]<0?UpdateRef::existing(next_fields_[edge.field]):UpdateRef::pending(field_commands_[edge.field]);
      c.position=position++;if(!append(c))return out;
    }
  }
  const auto spec=p.hierarchy();
  for(std::size_t i=0;i<spec.child_count;++i){w.charge(1);children_.push_back(spec.children[i]);}
  for(std::size_t i=0;i<spec.node_count;++i) {
    w.charge(1);auto node=spec.nodes[i];auto field=p.fields().size();
    for(std::size_t j=0;j<p.fields().size();++j){w.charge(1);if(p.fields()[j].command==node.field_command)field=j;}
    need(field<p.fields().size());node.field=next_fields_[field];node.field_command=field_commands_[field];node.cohort_begin=contributions_.size();
    const auto contribution=[&](field::LiveRefreshContribution c) {
      w.charge(sizeof(c));if(contributions_.size()==contributions_.capacity()){out.status=field::UpdateStatus::CapacityExhausted;out.failure={field::Budget::CascadeReferences,contributions_.size()+1,contributions_.capacity()};return false;}
      contributions_.push_back(c);return true;
    };
    if(node.field_command<0)for(std::size_t row=0;row<s.live.edges;++row) {
      w.charge(1);const auto &e=s.edges[row];if(!retired(e.first)&&field::detail::same_handle(e.second,node.field))
        if(!contribution({UpdateRef::existing(e.first),{s.generation,row,e.ordinal},-1}))return out;
    }
    for(std::size_t c=0;c<commands_.size();++c) {
      w.charge(1);const auto &e=commands_[c];if(e.operation!=UpdateOp::InsertParticipation)continue;
      if(node.field_command<0?(e.second.command<0 && field::detail::same_handle(e.second.handle,node.field)):e.second.command==node.field_command)
        if(!contribution({e.first,{},int(c)}))return out;
    }
    node.cohort_count=contributions_.size()-node.cohort_begin;
    if(!node.cohort_count){out.status=field::UpdateStatus::Unsupported;return out;}
    nodes_.push_back(node);
  }
  out.status=field::UpdateStatus::Ok;return out;
}
field::LiveRefreshSpec PreparedLodWorkspace::hierarchy() const {return {nodes_.data(),nodes_.size(),children_.data(),children_.size(),contributions_.data(),contributions_.size()};}
void PreparedLodWorkspace::publish(PreparedStudyOwner &o) {
  const auto &s=o.h_.prepared_storage();const auto &p=*sides_[target_].plan;
  for(std::size_t i=0;i<intrinsics_.size();++i) {
    const auto command=intrinsics_[i].command;const auto claim=s.resolved.at(command);
    need(claim.context==s.particles.context() && claim.incarnation && claim.record>=0 &&
         field::detail::same_handle(claim,s.particle_headers.at(claim.record)) &&
         claim.address==commands_[command].address && s.source_masses.at(claim.record)==*commands_[command].exact_mass);
    next_bodies_[changing_?i:next_bodies_.size()-1]=claim;
  }
  for(std::size_t i=0;i<next_fields_.size();++i)if(field_commands_[i]>=0) {
    const auto claim=s.resolved.at(field_commands_[i]);
    need(claim.context==s.fields.context() && claim.incarnation && claim.record>=0 && field::detail::same_handle(claim,s.field_headers.at(claim.record)) &&
         claim.address==p.commands()[p.fields()[i].command].address);next_fields_[i]=claim;
  }
  if(changing_ && !fixed_character_)catalogue_.swap(next_catalogue_);
  else if(new_identity_) {
    static_assert(std::is_nothrow_move_constructible<AdmissionOccurrenceCommand>::value && std::is_nothrow_move_assignable<AdmissionOccurrenceCommand>::value);
    std::swap(catalogue_[selected_],incoming_source_);templates_[selected_]=source_template_;known_[selected_]=1;
  }
  live_.swap(next_);o.bodies_.swap(next_bodies_);o.fields_.swap(next_fields_);side_=target_;
}
PreparedOccurrenceEditResult PreparedStudyOwner::commit_lod(PreparedOccurrenceEditResult out,
    std::uint64_t placement_work,std::uint64_t cells,std::uint64_t hierarchy_work) {
  if(out.publication.status!=field::UpdateStatus::Ok)return out;
  const auto tick=h_.prepared_storage().completed_ticks;
  try {
    if(!lod_->intrinsics_.empty())out.publication=h_.stage_current_admission(generation_,lod_->commands_.data(),lod_->commands_.size(),
        lod_->intrinsics_.data(),lod_->intrinsics_.size(),lod_->hierarchy(),placement_work,cells,hierarchy_work,true,false,nullptr,0,
        lod_->changing_?lod_->outgoing_.data():nullptr,lod_->changing_?lod_->outgoing_.size():0,
        lod_->changing_?lod_->groups_.data():nullptr,lod_->changing_?lod_->groups_.size():0);
    else out.publication=h_.refresh_live_hierarchy(generation_,lod_->commands_.data(),lod_->commands_.size(),lod_->hierarchy(),hierarchy_work);
    if(out.publication.status==field::UpdateStatus::Ok) {
      if(h_.prepared_storage().failure_point==7)throw std::runtime_error("injected failure before whole-field mapping publication");
      if(h_.prepared_storage().completed_ticks!=tick || h_.prepared_storage().generation!=out.publication.generation)
        throw std::runtime_error("inconsistent whole-field commit");
      lod_->publish(*this);generation_=out.publication.generation;
      if(!ready())throw std::runtime_error("unpublished whole-field mapping");
    }
    if(!h_.ready())phase_=PreparedOwnerPhase::Failed;
  }catch(...){h_.ready_=false;phase_=PreparedOwnerPhase::Failed;throw;}
  return out;
}
PreparedOccurrenceEditResult PreparedStudyOwner::edit_lod_presence(const PreparedOccurrenceEdit &r) {
  PreparedOccurrenceEditResult out;out.publication.generation=generation_;PreparationWork w(r.preparation_work);
  try{out.publication=lod_->presence(*this,r,w);}catch(const PreparationStop &e){out.preparation=e.result;return out;}
  out.preparation=w.result();return commit_lod(out,r.placement_work,r.placement_cells,r.hierarchy_work);
}
PreparedOccurrenceEditResult PreparedStudyOwner::lod(const warm::Uuid &owner,const ExecutionHandle &h,const PreparedLodEdit &r) {
  authorize(owner,h);PreparedOccurrenceEditResult out;out.publication.generation=generation_;
  if(!ready()){out.publication.status=field::UpdateStatus::Unready;return out;}
  if(!lod_){out.publication.status=field::UpdateStatus::Unsupported;return out;}
  PreparationWork w(r.preparation_work);
  try{out.publication=lod_->exposure(*this,r,w);}catch(const PreparationStop &e){out.preparation=e.result;return out;}
  out.preparation=w.result();return commit_lod(out,r.placement_work,r.placement_cells,r.hierarchy_work);
}
}
