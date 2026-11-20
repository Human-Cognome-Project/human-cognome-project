#pragma once
#include <algorithm>
#include <array>
#include "assembly/projection_access.h"

// Inspection only. Actual complete owner keys remain in production rows.
// The index replay keeps an independent owner association and borrows no row
// from a different projection. Only integer observations escape its lifetime.
namespace projection_byte_owner_observation {
using assembly::detail::PreparationWork;
using assembly::detail::ProjectionAccess;
inline void need(bool ok) { if(!ok)throw warm::Error("projection byte-owner inspection failed"); }
inline bool relation(const assembly::LetterProjection &p,std::size_t byte_index,
                     std::size_t owner_index,PreparationWork &work) {
  const auto *byte_tool=ProjectionAccess::resolve(p,byte_index,&work);
  const auto *owner_tool=ProjectionAccess::resolve(p,owner_index,&work);
  if(!byte_tool || !owner_tool) return false;
  const auto &byte=ProjectionAccess::row(p,byte_index),&owner=ProjectionAccess::row(p,owner_index);
  work.charge(1);
  if(!byte.byte || byte.source.path.size()!=2 || !owner.source.path.empty() || owner.byte ||
     owner.source.entry!=byte.source.entry || byte.byte->character_ordinal!=byte.source.path[0] ||
     byte.byte->byte_ordinal!=byte.source.path[1] || byte.source.path[0]>=owner_tool->parents.size()) return false;
  const auto &character_parent=owner_tool->parents[byte.source.path[0]];
  work.charge(1);
  if(character_parent.ordinal!=byte.source.path[0] || !work.equal(character_parent.target,byte.byte->character)) return false;
  const auto &recipe=p.recipe();
  const auto character=std::find_if(recipe.tools.begin(),recipe.tools.end(),[&](const auto &tool) {
    return work.equal(tool.ref,byte.byte->character);
  });
  if(character==recipe.tools.end() || !character->mass || !*character->mass) return false;
  if(*character->mass==2) {
    if(byte.source.path[1]!=0 || !work.equal(character->ref,byte_tool->ref)) return false;
  } else {
    if(byte.source.path[1]>=character->parents.size()) return false;
    const auto &parent=character->parents[byte.source.path[1]];
    work.charge(1);
    if(parent.ordinal!=byte.source.path[1] || !work.equal(parent.target,byte_tool->ref)) return false;
  }
  const auto selection=std::find_if(recipe.selections.begin(),recipe.selections.end(),[&](const auto &value) {
    work.charge(1);return value.entry==owner.source.entry;
  });
  return selection!=recipe.selections.end() &&
      std::any_of(selection->parts.begin(),selection->parts.end(),[&](const auto &id) {return work.equal(id,byte.source);});
}
inline bool matches(const assembly::LetterProjection &p,std::size_t byte_index,std::size_t owner_index,
                    const assembly::OccurrenceKey &key,PreparationWork &work) {
  work.charge(1);
  return owner_index<p.occurrence_count() && ProjectionAccess::matches(p,owner_index,key,work) &&
      relation(p,byte_index,owner_index,work);
}
struct Cost {
  std::size_t rows{},capacity{},byte_rows{},active{},owners{},requests{},requested{},path_requested{},header_requested{};
  std::size_t engaged_key_bytes{},key_capacity_bytes{},outer_requested{};
  std::uint64_t full_key_work{},index_relation_work{};
};
template<class Visit> Cost inspect(const assembly::LetterProjection &p,
    const assembly::detail::ReservationDomain *domain,Visit visit) {
  Cost out;
  const auto &rows=ProjectionAccess::rows(p);
  need(rows.get_allocator().domain.get()==domain && p.study().str().get_allocator().domain.get()==domain);
  out.rows=rows.size();out.capacity=rows.capacity();out.outer_requested=rows.capacity()*sizeof(rows[0]);
  out.key_capacity_bytes=rows.capacity()*sizeof(assembly::OccurrenceKey);
  out.header_requested=warm::recipe_text_backing(p.study().str());
  visit(p.study().str().data(),out.header_requested);
  std::array<const void *,4096> seen{};std::size_t used=0;
  seen[used++]=p.study().str().data();
  PreparationWork full(UINT64_MAX),replay(UINT64_MAX);
  for(std::size_t i=0;i<rows.size();++i) {
    const auto &row=rows[i];if(!row.byte)continue;
    ++out.byte_rows;out.active+=row.active;
    const auto &key=row.byte->owner;
    need(key.study.str().get_allocator().domain.get()==domain && key.source.path.get_allocator().domain.get()==domain &&
         key.source.path.empty() && key.source.path.capacity()==0);
    const auto owner=ProjectionAccess::find(p,key,full);
    need(owner<p.occurrence_count() && ProjectionAccess::matches(p,owner,key,full) && relation(p,i,owner,replay));
    bool first=true;
    for(std::size_t j=0;j<i;++j)if(rows[j].byte && rows[j].byte->owner.source.entry==key.source.entry)first=false;
    out.owners+=first;
    const auto n=warm::recipe_text_backing(key.study.str());
    need(n && used<seen.size() && std::find(seen.begin(),seen.begin()+used,key.study.str().data())==seen.begin()+used);
    seen[used++]=key.study.str().data();visit(key.study.str().data(),n);
    ++out.requests;out.requested+=n;
    out.path_requested+=key.source.path.capacity()*sizeof(std::uint32_t);
  }
  out.engaged_key_bytes=out.byte_rows*sizeof(assembly::OccurrenceKey);
  out.full_key_work=full.result().charged;out.index_relation_work=replay.result().charged;
  return out;
}
}
