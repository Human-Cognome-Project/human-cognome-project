#pragma once
#include <algorithm>
#include <array>
#include "assembly/projection_access.h"
#include "projection_indices_access.h"
#include "assembly/preparation_work.h"

// Test-only census of recipe-backed definitions. Only numeric observations
// outlive the projection; every contextual read checks its source association.
namespace projection_definition_observation {
inline void need(bool ok) {if(!ok)throw warm::Error("projection definition ownership inspection failed");}
inline std::size_t lookup(const assembly::LetterProjection &projection,
    const assembly::ProjectedOccurrence &row,const warm::BundleRef &recipe,
    assembly::detail::PreparationWork &work) {
  work.charge(1);
  const auto &b=projection.recipe();
  if(!work.equal(projection.study(),row.key.study) || !work.equal(b.ref.selection,recipe.selection) ||
      !work.equal(b.ref.revision,recipe.revision)) return SIZE_MAX;
  const auto source=[&](const auto &rows) {
    return std::find_if(rows.begin(),rows.end(),[&](const auto &value) {return work.equal(value.id,row.key.source);});
  };
  if(b.recipe_version==1) {
    const auto it=source(b.occurrences);
    if(it==b.occurrences.end() || !work.equal(it->definition,row.definition.object) || it->node!=row.node) return SIZE_MAX;
  } else {
    const auto it=source(b.structural_occurrences);
    if(it==b.structural_occurrences.end() || !work.equal(it->definition,row.definition.object) || it->node!=row.node) return SIZE_MAX;
  }
  const auto tool=std::find_if(b.tools.begin(),b.tools.end(),[&](const auto &value) {return work.equal(value.ref,row.definition);});
  if(tool==b.tools.end() || !tool->mass || !*tool->mass) return SIZE_MAX;
  return std::size_t(tool-b.tools.begin());
}
struct Cost {
  std::size_t rows{},active{},unique_definitions{},requests{},bytes{},pin_capacity_bytes{},outer_bytes{},index_capacity_bytes{};
  std::uint64_t lookup_work{};
};
template<class Visit> Cost inspect(const assembly::LetterProjection &p,
    const assembly::detail::ReservationDomain *domain,Visit visit) {
  (void)visit; // Definition pins now belong only to the already counted recipe.
  Cost out;
  assembly::detail::PreparationWork work(UINT64_MAX);
  const auto &rows=assembly::detail::ProjectionAccess::rows(p);
  need(rows.get_allocator().domain.get()==domain);
  out.index_capacity_bytes=rows.capacity()*2*sizeof(std::uint32_t);
  out.outer_bytes=rows.capacity()*sizeof(rows[0]);
  for(std::size_t i=0;i<rows.size();++i) {
    const auto &row=rows[i];
    const auto &definition=assembly::detail::ProjectionAccess::definition(p,row,&work);
    need(&definition==&p.recipe().tools[row.definition_index].ref);
    ++out.rows;out.active+=row.active;
    bool first=true;
    for(std::size_t j=0;j<i;++j)
      if(work.equal(assembly::detail::ProjectionAccess::definition(p,rows[j],&work),definition)){first=false;break;}
    out.unique_definitions+=first;
  }
  out.lookup_work=work.result().charged;
  return out;
}
inline void independent(const assembly::LetterProjection &a,const assembly::LetterProjection &b) {
  const auto &left=assembly::detail::ProjectionAccess::rows(a),&right=assembly::detail::ProjectionAccess::rows(b);
  need(&a.recipe()==&b.recipe() && left.data()!=right.data() && left.size()==right.size());
  for(std::size_t i=0;i<left.size();++i)
    need(left[i].key==right[i].key && left[i].source_index==right[i].source_index &&
         left[i].definition_index==right[i].definition_index &&
         &assembly::detail::ProjectionAccess::definition(a,left[i])==
             &assembly::detail::ProjectionAccess::definition(b,right[i]));
}
} // namespace projection_definition_observation
