#pragma once
#include <algorithm>
#include <array>
#include "assembly/projection_access.h"
#include "assembly/preparation_work.h"

// Census of the actual header and preserved byte-owner stamps after migration.
namespace projection_study_observation {
inline void need(bool ok) {if(!ok)throw warm::Error("projection study ownership inspection failed");}
struct Cost {
  std::size_t rows{},capacity{},requests{},bytes{},header_bytes{},uuid_capacity_bytes{},outer_bytes{};
  std::size_t byte_owner_requests{},byte_owner_bytes{};
  std::uint64_t comparison_work{},source_work{};
};
template<class Visit> Cost inspect(const assembly::LetterProjection &p,
    const assembly::detail::ReservationDomain *domain,Visit visit) {
  Cost out;
  assembly::detail::PreparationWork work(UINT64_MAX),source(UINT64_MAX);
  const auto &rows=assembly::detail::ProjectionAccess::rows(p);
  need(rows.get_allocator().domain.get()==domain && p.study().str().get_allocator().domain.get()==domain);
  out.rows=rows.size();out.capacity=rows.capacity();
  out.outer_bytes=out.capacity*sizeof(rows[0]);
  std::array<const void *,4096> seen{};std::size_t used=0;
  const auto buffer=[&](const warm::Uuid &value) {
    const auto &text=value.str();need(text.get_allocator().domain.get()==domain);
    const auto bytes=warm::recipe_text_backing(text);
    if(bytes) {
      need(used<seen.size() && std::find(seen.begin(),seen.begin()+used,text.data())==seen.begin()+used);
      seen[used++]=text.data();visit(text.data(),bytes);
    }
    return bytes;
  };
  out.header_bytes=buffer(p.study());
  for(std::size_t index=0;index<rows.size();++index) {
    const auto &row=rows[index];
    need(assembly::detail::ProjectionAccess::resolve(p,index,&source));
    if(row.byte) {
      need(assembly::detail::ProjectionAccess::byte_owner_index(p,index,&work)<rows.size());
    }
  }
  out.comparison_work=work.result().charged;out.source_work=source.result().charged;
  return out;
}
inline void independent(const assembly::LetterProjection &a,const assembly::LetterProjection &b) {
  const auto &left=assembly::detail::ProjectionAccess::rows(a),&right=assembly::detail::ProjectionAccess::rows(b);
  need(a.study()==b.study() && a.study().str().data()!=b.study().str().data() && left.data()!=right.data());
  need(left.size()==right.size());
  for(std::size_t i=0;i<left.size();++i)if(left[i].byte)
    need(right[i].byte && left[i].byte->owner_index==right[i].byte->owner_index);
}
}
