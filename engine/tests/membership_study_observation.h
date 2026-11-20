#include "assembly/projection_access.h"
#pragma once
#include <algorithm>
#include <array>
#include "assembly/membership_fields.h"
#include "assembly/preparation_work.h"

// Inspection only. A collected catalogue already owns one study identity;
// compare that header and numeric row identity without building an owning key.
namespace membership_study_observation {
inline void need(bool ok) {
  if (!ok) throw warm::Error("membership study ownership inspection failed");
}
template<class Row> bool matches(const assembly::MembershipFields &fields,
    const Row &row,const assembly::OccurrenceKey &key,
    assembly::detail::PreparationWork &work) {
  return work.equal(fields.study(),key.study) && work.equal(row.occurrence,key.source);
}
struct Cost {
  std::size_t rows{},requests{},bytes{},header_bytes{},capacity{},outer_bytes{},uuid_member_bytes{};
  std::uint64_t comparison_work{};
};
template<class Rows,class Visit> Cost inspect(const assembly::MembershipFields &fields,
    const assembly::LetterProjection &projection,const Rows &rows,
    const assembly::detail::ReservationDomain *domain,Visit visit) {
  Cost out;
  assembly::detail::PreparationWork work(UINT64_MAX);
  need(fields.study()==projection.study() && fields.study().str().get_allocator().domain.get()==domain);
  out.header_bytes=warm::recipe_text_backing(fields.study().str());
  visit(fields.study().str().data(),out.header_bytes);
  out.capacity=rows.capacity();
  out.outer_bytes=out.capacity*sizeof(rows[0]);
  for (const auto &row:rows) {
    ++out.rows;
    const auto occurrence=std::find_if(assembly::detail::ProjectionAccess::rows(projection).begin(),assembly::detail::ProjectionAccess::rows(projection).end(),
        [&](const auto &o){return o.source==row.occurrence;});
    need(occurrence!=assembly::detail::ProjectionAccess::rows(projection).end() &&
         work.equal(fields.study(),projection.study()) && work.equal(row.occurrence,occurrence->source));
  }
  out.comparison_work=work.result().charged;
  return out;
}
}  // namespace membership_study_observation
