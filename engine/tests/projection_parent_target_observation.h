#pragma once
#include <algorithm>
#include <array>
#include <iostream>
#include "assembly/projection_access.h"

// Test-only ownership census and recipe-selector experiment. Production keeps
// complete parent field objects; no selector or new recipe lender is retained.
namespace projection_parent_target_observation {
using assembly::detail::PreparationWork;
using assembly::detail::ProjectionAccess;
struct Selectors {std::uint32_t occurrence{},parent{};};
struct CandidateParent {
  warm::OccurrenceId source;
  Selectors selected;
  assembly::GroupKind kind;
  std::uint32_t ordinal{};
  std::uint64_t count{};
};
inline void need(bool ok) {if(!ok)throw warm::Error("projection parent target inspection failed");}
inline const warm::ToolRef *resolve(const assembly::LetterProjection &p,bool spare,std::size_t index,
    Selectors selected,PreparationWork &work) {
  work.charge(1);
  if(index>=ProjectionAccess::parent_count(p,spare) || selected.occurrence>=p.occurrence_count())return nullptr;
  const auto &parent=ProjectionAccess::parent(p,spare,index);
  const auto &source=ProjectionAccess::row(p,selected.occurrence);
  if(parent.field.kind!=assembly::GroupKind::ParentValue || source.active==spare ||
      !work.equal(parent.source,source.source))return nullptr;
  const auto *tool=ProjectionAccess::resolve(p,selected.occurrence,&work);
  if(!tool || selected.parent>=tool->parents.size())return nullptr;
  const auto &edge=tool->parents[selected.parent];
  work.charge(1);
  if(edge.ordinal!=parent.ordinal || edge.contribution!=parent.count)return nullptr;
  return &edge.target;
}
inline std::optional<Selectors> select(const assembly::LetterProjection &p,bool spare,std::size_t index,
    PreparationWork &work) {
  work.charge(1);
  if(index>=ProjectionAccess::parent_count(p,spare))return {};
  const auto &parent=ProjectionAccess::parent(p,spare,index);
  for(std::size_t i=0;i<p.occurrence_count();++i) {
    if(!work.equal(ProjectionAccess::row(p,i).source,parent.source))continue;
    const auto *tool=ProjectionAccess::resolve(p,i,&work);
    if(!tool || i>UINT32_MAX)return {};
    for(std::size_t j=0;j<tool->parents.size();++j) {
      work.charge(1);const auto &edge=tool->parents[j];
      if(edge.ordinal!=parent.ordinal || edge.contribution!=parent.count ||
          !work.equal(edge.target.object,parent.field.object))continue;
      if(j>UINT32_MAX)return {};
      const Selectors selected{std::uint32_t(i),std::uint32_t(j)};
      if(resolve(p,spare,index,selected,work))return selected;
      return {};
    }
    return {};
  }
  return {};
}
inline bool matches(const assembly::LetterProjection &p,bool spare,std::size_t index,Selectors selected,
    const assembly::OccurrenceKey &key,PreparationWork &work) {
  return ProjectionAccess::parent_matches(p,spare,index,key,work) && resolve(p,spare,index,selected,work);
}
struct Pool {
  std::size_t rows{},capacity{},requests{},scope_bytes{},address_bytes{},field_capacity_bytes{},outer_bytes{};
  std::size_t unique_targets{},unique_requested{},selector_capacity_bytes{};
};
struct Cost {
  std::array<Pool,2> pools{};
  std::uint64_t selection_work{},guard_work{},object_comparison_work{};
};
template<class Access,class Visit> Cost inspect(const assembly::LetterProjection &p,
    const assembly::detail::ReservationDomain *domain,Visit visit) {
  Cost out;std::array<const void *,8192> seen{};std::size_t used=0;
  PreparationWork setup(UINT64_MAX),guard(UINT64_MAX),comparison(UINT64_MAX);
  const auto buffer=[&](const auto &value,std::size_t bytes) {
    need(value.get_allocator().domain.get()==domain);
    if(bytes) {
      const auto *pointer=static_cast<const void *>(value.data());
      need(used<seen.size() && std::find(seen.begin(),seen.begin()+used,pointer)==seen.begin()+used);
      seen[used++]=pointer;visit(pointer,bytes);
    }
  };
  for(const bool spare:{false,true}) {
    auto &cost=out.pools[spare];const auto &rows=Access::parent_rows(p,spare);
    need(rows.get_allocator().domain.get()==domain);
    cost.rows=rows.size();cost.capacity=rows.capacity();cost.outer_bytes=rows.capacity()*sizeof(rows[0]);
    cost.field_capacity_bytes=rows.capacity()*sizeof(assembly::FieldKey);
    cost.selector_capacity_bytes=rows.capacity()*sizeof(Selectors);
    for(std::size_t i=0;i<rows.size();++i) {
      const auto &row=ProjectionAccess::parent(p,spare,i);
      const auto &object=row.field.object;
      const auto scope=warm::recipe_text_backing(object.scope.str()),address=object.address.capacity()*sizeof(object.address[0]);
      buffer(object.scope.str(),scope);buffer(object.address,address);
      cost.scope_bytes+=scope;cost.address_bytes+=address;cost.requests+=(scope!=0)+(address!=0);
      bool first=true;
      for(std::size_t j=0;j<i;++j)if(rows[j].field.kind==row.field.kind && rows[j].field.object==object)first=false;
      cost.unique_targets+=first;if(first)cost.unique_requested+=scope+address;
      const auto selected=select(p,spare,i,setup);need(bool(selected));
      const auto *target=resolve(p,spare,i,*selected,guard);need(target);
      need(comparison.equal(object,target->object) && object.scope.str().data()!=target->object.scope.str().data() &&
           (object.address.empty() || object.address.data()!=target->object.address.data()));
      need(target->object.scope.str().get_allocator().domain.get()==domain &&
           target->object.address.get_allocator().domain.get()==domain && target->revision.str().get_allocator().domain.get()==domain);
      const auto recipe_scope=warm::recipe_text_backing(target->object.scope.str());
      if(recipe_scope)visit(target->object.scope.str().data(),recipe_scope);
      if(!target->object.address.empty())visit(target->object.address.data(),target->object.address.capacity()*sizeof(target->object.address[0]));
      const auto revision=warm::recipe_text_backing(target->revision.str());
      if(revision)visit(target->revision.str().data(),revision);
    }
  }
  out.selection_work=setup.result().charged;out.guard_work=guard.result().charged;
  out.object_comparison_work=comparison.result().charged;
  return out;
}
inline void print(const char *record,std::size_t fixture,std::size_t view,const Cost &cost) {
  for(const bool spare:{false,true}) {
    const auto &c=cost.pools[spare];
    std::cout<<record<<",fixture="<<fixture<<",view="<<view<<",spare="<<spare
      <<",rows="<<c.rows<<",capacity="<<c.capacity<<",requests="<<c.requests
      <<",scope_requested="<<c.scope_bytes<<",address_requested="<<c.address_bytes
      <<",unique_targets="<<c.unique_targets<<",representative_requested="<<c.unique_requested
      <<",field_member_capacity_bytes="<<c.field_capacity_bytes<<",outer_requested="<<c.outer_bytes
      <<",selector_capacity_bytes="<<c.selector_capacity_bytes<<",candidate_row_bytes="<<sizeof(CandidateParent)
      <<",candidate_selector_bytes="<<sizeof(Selectors)<<",selection_work="<<cost.selection_work
      <<",indexed_source_parent_guard_work="<<cost.guard_work<<",stored_object_comparison_work="<<cost.object_comparison_work
      <<",observer_allocations=0,recipe_target_owned=1,runtime_saving=0 PASS\n";
  }
}
}
