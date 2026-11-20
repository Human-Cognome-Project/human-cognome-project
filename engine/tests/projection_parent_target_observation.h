#pragma once
#include <algorithm>
#include <array>
#include <iostream>
#include "assembly/projection_access.h"

// Census of actual production selectors and existing recipe target ownership.
// The independent source/ordinal search remains a test-only replay.
namespace projection_parent_target_observation {
using assembly::detail::PreparationWork;
using assembly::detail::ProjectionAccess;
struct Selectors {std::uint32_t occurrence{},parent{};};
inline void need(bool ok) {if(!ok)throw warm::Error("projection parent target inspection failed");}
inline const warm::ToolRef *resolve(const assembly::LetterProjection &p,bool spare,std::size_t index,
    PreparationWork &work) {
  const auto *edge=ProjectionAccess::resolve_parent(p,spare,index,&work);
  return edge?&edge->target:nullptr;
}
inline std::optional<Selectors> select(const assembly::LetterProjection &p,bool spare,std::size_t index,
    PreparationWork &work) {
  work.charge(1);
  if(index>=ProjectionAccess::parent_count(p,spare))return {};
  const auto &parent=ProjectionAccess::parent(p,spare,index);
  if(parent.kind!=assembly::GroupKind::ParentValue)return {};
  for(std::size_t i=0;i<p.occurrence_count();++i) {
    if(!work.equal(ProjectionAccess::row(p,i).source,parent.source))continue;
    const auto *tool=ProjectionAccess::resolve(p,i,&work);
    if(!tool || i>UINT32_MAX)return {};
    for(std::size_t j=0;j<tool->parents.size();++j) {
      work.charge(1);const auto &edge=tool->parents[j];
      if(edge.ordinal!=parent.ordinal || edge.contribution!=parent.count)continue;
      if(j>UINT32_MAX)return {};
      const Selectors selected{std::uint32_t(i),std::uint32_t(j)};
      return selected;
    }
    return {};
  }
  return {};
}
inline bool matches(const assembly::LetterProjection &p,bool spare,std::size_t index,
    const assembly::OccurrenceKey &key,PreparationWork &work) {
  return ProjectionAccess::parent_matches(p,spare,index,key,work);
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
  Cost out;
  PreparationWork setup(UINT64_MAX),guard(UINT64_MAX),comparison(UINT64_MAX);
  for(const bool spare:{false,true}) {
    auto &cost=out.pools[spare];const auto &rows=Access::parent_rows(p,spare);
    need(rows.get_allocator().domain.get()==domain);
    cost.rows=rows.size();cost.capacity=rows.capacity();cost.outer_bytes=rows.capacity()*sizeof(rows[0]);
    cost.selector_capacity_bytes=rows.capacity()*sizeof(Selectors);
    for(std::size_t i=0;i<rows.size();++i) {
      const auto &row=ProjectionAccess::parent(p,spare,i);
      const auto *target=resolve(p,spare,i,guard);need(target);
      const auto &object=target->object;
      const auto scope=warm::recipe_text_backing(object.scope.str()),address=object.address.capacity()*sizeof(object.address[0]);
      bool first=true;
      for(std::size_t j=0;j<i;++j)if(rows[j].kind==row.kind && ProjectionAccess::parent_target(p,spare,j).object==object)first=false;
      cost.unique_targets+=first;if(first)cost.unique_requested+=scope+address;
      const auto selected=select(p,spare,i,setup);need(bool(selected));
      need(selected->occurrence==row.occurrence_index && selected->parent==row.parent_index);
      const auto &expected=ProjectionAccess::tool(p,selected->occurrence).parents[selected->parent].target;
      need(&expected==target && comparison.equal(object,expected.object));
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
      <<",selector_capacity_bytes="<<c.selector_capacity_bytes<<",private_row_bytes="<<sizeof(assembly::detail::ProjectionParentContribution)
      <<",selector_bytes="<<sizeof(Selectors)<<",selection_replay_work="<<cost.selection_work
      <<",indexed_source_parent_guard_work="<<cost.guard_work<<",target_object_comparison_work="<<cost.object_comparison_work
      <<",observer_allocations=0,recipe_target_owned=1,private_field_backing=0 PASS\n";
  }
}
}
