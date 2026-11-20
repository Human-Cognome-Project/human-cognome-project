#pragma once
#include <algorithm>
#include <array>
#include "assembly/projection_access.h"
#include "assembly/preparation_work.h"

// Test-only study/header ownership census. No row/key representation changes.
namespace projection_study_observation {
inline void need(bool ok) {if(!ok)throw warm::Error("projection study ownership inspection failed");}
inline bool matches(const warm::Uuid &header,const warm::OccurrenceId &numeric,
                    const assembly::OccurrenceKey &key,assembly::detail::PreparationWork &work) {
  return work.equal(header,key.study) && work.equal(numeric,key.source);
}
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
  out.uuid_capacity_bytes=out.capacity*sizeof(warm::Uuid);
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
  for(const auto &row:rows) {
    need(assembly::detail::ProjectionAccess::resolve(p,row,&source) && matches(p.study(),row.key.source,row.key,work));
    const auto bytes=buffer(row.key.study);out.requests+=bool(bytes);out.bytes+=bytes;
    if(row.byte) {
      need(work.equal(p.study(),row.byte->owner.study) &&
           row.byte->owner.source.entry==row.key.source.entry && row.byte->owner.source.path.empty());
      const auto owner_bytes=buffer(row.byte->owner.study);
      out.byte_owner_requests+=bool(owner_bytes);out.byte_owner_bytes+=owner_bytes;
    }
  }
  out.comparison_work=work.result().charged;out.source_work=source.result().charged;
  return out;
}
inline void independent(const assembly::LetterProjection &a,const assembly::LetterProjection &b) {
  const auto &left=assembly::detail::ProjectionAccess::rows(a),&right=assembly::detail::ProjectionAccess::rows(b);
  need(a.study()==b.study() && a.study().str().data()!=b.study().str().data() && left.data()!=right.data());
  for(const auto &x:left)for(const auto &y:right) {
    need(x.key.study.str().data()!=y.key.study.str().data());
    if(x.byte)need(x.byte->owner.study.str().data()!=y.key.study.str().data());
    if(y.byte)need(x.key.study.str().data()!=y.byte->owner.study.str().data());
    if(x.byte && y.byte)need(x.byte->owner.study.str().data()!=y.byte->owner.study.str().data());
  }
}
}
