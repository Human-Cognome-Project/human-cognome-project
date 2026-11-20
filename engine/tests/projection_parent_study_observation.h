#pragma once
#include <algorithm>
#include <array>
#include <iostream>
#include "assembly/projection_access.h"

// Test-only census of actual numeric parent pools and owning-header context.
namespace projection_parent_study_observation {
using assembly::detail::PreparationWork;
using assembly::detail::ProjectionAccess;
inline void need(bool ok) { if(!ok)throw warm::Error("projection parent study inspection failed"); }
template<class Access> bool matches(const assembly::LetterProjection &p,bool spare,std::size_t index,
    const assembly::OccurrenceKey &key,PreparationWork &work) {
  return ProjectionAccess::parent_matches(p,spare,index,key,work);
}
// Active output uses the real public API. Only the private-pool lifetime control
// reconstructs an inactive value; this creates no public inactive-pool API.
template<class Access> assembly::ParentContribution value(const assembly::LetterProjection &p,bool spare,std::size_t index) {
  if(!spare)return p.parent(index);
  const auto &row=ProjectionAccess::parent(p,spare,index);
  auto study=warm::Uuid(p.study(),{});
  auto source=warm::OccurrenceId(row.source);
  assembly::OccurrenceKey occurrence{std::move(study),std::move(source)};
  auto object=warm::ObjectKey(ProjectionAccess::parent_target(p,spare,index).object);
  return {std::move(occurrence),{row.kind,std::move(object)},row.ordinal,row.count};
}
// Stronger independent witness, not a replacement production reader or a
// proposal to rescan occurrence/recipe tables on every frozen parent lookup.
template<class Access> bool provenance(const assembly::LetterProjection &p,bool spare,std::size_t index,
    PreparationWork &work) {
  const auto &parents=Access::parent_rows(p,spare);
  work.charge(1);
  if(index>=parents.size())return false;
  const auto &parent=parents[index];
  if(parent.kind!=assembly::GroupKind::ParentValue)return false;
  const auto *actual=ProjectionAccess::resolve_parent(p,spare,index,&work);
  if(!actual)return false;
  for(std::size_t i=0;i<p.occurrence_count();++i) {
    const auto &row=ProjectionAccess::row(p,i);
    if(!work.equal(row.source,parent.source))continue;
    const auto *tool=ProjectionAccess::resolve(p,i,&work);
    if(!tool || row.active==spare)return false;
    for(const auto &edge:tool->parents) {
      work.charge(1);
      if(edge.ordinal==parent.ordinal)
        return edge.contribution==parent.count && work.equal(edge.target,actual->target);
    }
    return false;
  }
  return false;
}
struct Pool {std::size_t rows{},capacity{},outer{},study_requests{},study_bytes{},uuid_capacity_bytes{};};
struct Cost {
  std::array<Pool,2> pools{};
  std::size_t header_bytes{};
  std::uint64_t numeric_work{},provenance_work{};
};
template<class Access,class Visit> Cost inspect(const assembly::LetterProjection &p,
    const assembly::detail::ReservationDomain *domain,Visit visit) {
  Cost out;
  std::array<const void *,4096> seen{};std::size_t used=0;
  const auto study=[&](const warm::Uuid &id) {
    const auto &text=id.str();
    need(text.get_allocator().domain.get()==domain);
    const auto bytes=warm::recipe_text_backing(text);
    if(bytes) {
      need(used<seen.size() && std::find(seen.begin(),seen.begin()+used,text.data())==seen.begin()+used);
      seen[used++]=text.data();visit(text.data(),bytes);
    }
    return bytes;
  };
  out.header_bytes=study(p.study());
  PreparationWork numeric(UINT64_MAX),proof(UINT64_MAX);
  for(const bool spare:{false,true}) {
    auto &cost=out.pools[spare];const auto &rows=Access::parent_rows(p,spare);
    need(rows.get_allocator().domain.get()==domain);
    cost.rows=rows.size();cost.capacity=rows.capacity();
    cost.outer=rows.capacity()*sizeof(rows[0]);
    for(std::size_t i=0;i<rows.size();++i) {
      const auto &row=rows[i];
      need(row.source.path.get_allocator().domain.get()==domain);
      need(&ProjectionAccess::parent(p,spare,i)==&row &&
           ProjectionAccess::parent_matches(p,spare,i,p.study(),row.source,numeric) && provenance<Access>(p,spare,i,proof));
      for(std::size_t j=0;j<i;++j)
        need(!(rows[j].source==row.source && rows[j].ordinal==row.ordinal));
      if(spare)for(const auto &active:Access::parent_rows(p,false))
        need(!(active.source==row.source && active.ordinal==row.ordinal));
    }
  }
  out.numeric_work=numeric.result().charged;
  out.provenance_work=proof.result().charged;
  return out;
}
template<class Access> void independent(const assembly::LetterProjection &a,const assembly::LetterProjection &b) {
  need(a.study()==b.study() && a.study().str().data()!=b.study().str().data());
  for(const bool spare:{false,true}) {
    const auto &left=Access::parent_rows(a,spare),&right=Access::parent_rows(b,spare);
    need(left.empty() || left.data()!=right.data());
    for(const auto &l:left) {
      for(const bool other:{false,true})for(const auto &r:Access::parent_rows(b,other))
        need(l.source.path.empty() || l.source.path.data()!=r.source.path.data());
    }
  }
}
inline void print(const char *record,std::size_t fixture,std::size_t view,const Cost &cost) {
  for(const bool spare:{false,true}) {
    const auto &c=cost.pools[spare];
    std::cout<<record<<",fixture="<<fixture<<",view="<<view<<",spare="<<spare
      <<",rows="<<c.rows<<",capacity="<<c.capacity<<",outer_requested="<<c.outer
      <<",study_requests="<<c.study_requests<<",study_requested="<<c.study_bytes
      <<",header_requested="<<cost.header_bytes<<",uuid_member_capacity_bytes="<<c.uuid_capacity_bytes
      <<",public_row_bytes="<<sizeof(assembly::ParentContribution)<<",private_row_bytes="<<sizeof(assembly::detail::ProjectionParentContribution)
      <<",numeric_comparison_work="<<cost.numeric_work
      <<",provenance_replay_work="<<cost.provenance_work
      <<",observer_allocations=0,private_study_backing=0,retained_reader_bytes=0 PASS\n";
  }
}
}
