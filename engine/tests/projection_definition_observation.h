#pragma once
#include <algorithm>
#include <array>
#include "assembly/letter_projection.h"
#include "assembly/preparation_work.h"

// Test-only comparison with the projection's existing immutable recipe.
// No recipe index or borrowed pointer escapes into production storage.
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
  std::size_t rows{},active{},unique_definitions{},requests{},bytes{},pin_capacity_bytes{},outer_bytes{};
  std::uint64_t lookup_work{};
};
template<class Visit> Cost inspect(const assembly::LetterProjection &p,
    const assembly::detail::ReservationDomain *domain,Visit visit) {
  Cost out;
  std::array<const void *,4096> seen{};
  std::size_t used=0;
  assembly::detail::PreparationWork work(UINT64_MAX);
  const auto &rows=p.occurrences();
  need(rows.get_allocator().domain.get()==domain);
  out.pin_capacity_bytes=rows.capacity()*sizeof(warm::ToolRef);
  out.outer_bytes=rows.capacity()*sizeof(assembly::ProjectedOccurrence);
  for(std::size_t i=0;i<rows.size();++i) {
    const auto &row=rows[i];
    const auto index=lookup(p,row,p.recipe().ref,work);
    need(index<p.recipe().tools.size());
    const auto &original=p.recipe().tools[index].ref;
    ++out.rows; out.active+=row.active;
    bool first=true;
    for(std::size_t j=0;j<i;++j) if(work.equal(rows[j].definition,row.definition)){first=false;break;}
    out.unique_definitions+=first;
    const auto request=[&](const void *pointer,const void *source,std::size_t bytes) {
      if(!bytes)return;
      need(pointer!=source && used<seen.size() && std::find(seen.begin(),seen.begin()+used,pointer)==seen.begin()+used);
      seen[used++]=pointer; visit(pointer,bytes); ++out.requests; out.bytes+=bytes;
    };
    const auto text=[&](const auto &value,const auto &source) {
      need(value.get_allocator().domain.get()==domain);
      request(value.data(),source.data(),warm::recipe_text_backing(value));
    };
    text(row.definition.object.scope.str(),original.object.scope.str());
    text(row.definition.revision.str(),original.revision.str());
    need(row.definition.object.address.get_allocator().domain.get()==domain);
    request(row.definition.object.address.data(),original.object.address.data(),
            row.definition.object.address.capacity()*sizeof(row.definition.object.address[0]));
  }
  out.lookup_work=work.result().charged;
  return out;
}
inline void independent(const assembly::LetterProjection &a,const assembly::LetterProjection &b) {
  need(&a.recipe()==&b.recipe() && a.occurrences().data()!=b.occurrences().data());
  for(const auto &left:a.occurrences())for(const auto &right:b.occurrences())
    need(left.definition.object.scope.str().data()!=right.definition.object.scope.str().data() &&
         left.definition.object.address.data()!=right.definition.object.address.data() &&
         left.definition.revision.str().data()!=right.definition.revision.str().data());
}
} // namespace projection_definition_observation
