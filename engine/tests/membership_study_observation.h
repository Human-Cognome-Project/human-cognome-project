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
inline bool matches(const assembly::MembershipFields &fields,
    const assembly::MembershipContribution &row,const assembly::OccurrenceKey &key,
    assembly::detail::PreparationWork &work) {
  return work.equal(fields.study(),key.study) && work.equal(row.occurrence.source,key.source);
}
struct Cost {
  std::size_t rows{},requests{},bytes{},header_bytes{},capacity{},outer_bytes{},uuid_member_bytes{};
  std::uint64_t comparison_work{};
};
template<class Visit> Cost inspect(const assembly::MembershipFields &fields,
    const assembly::LetterProjection &projection,
    const assembly::detail::ReservationDomain *domain,Visit visit) {
  Cost out;
  assembly::detail::PreparationWork work(UINT64_MAX);
  need(fields.study()==projection.study() && fields.study().str().get_allocator().domain.get()==domain);
  out.header_bytes=warm::recipe_text_backing(fields.study().str());
  visit(fields.study().str().data(),out.header_bytes);
  out.capacity=fields.contributions().capacity();
  out.outer_bytes=out.capacity*sizeof(assembly::MembershipContribution);
  out.uuid_member_bytes=out.capacity*sizeof(warm::Uuid);
  std::array<const void *,4096> seen{};
  for (const auto &row:fields.contributions()) {
    const auto &text=row.occurrence.study.str();
    need(row.occurrence.study==fields.study() && text.get_allocator().domain.get()==domain &&
         text.data()!=fields.study().str().data() && out.rows<seen.size() &&
         std::find(seen.begin(),seen.begin()+out.rows,text.data())==seen.begin()+out.rows);
    seen[out.rows++]=text.data();
    const auto bytes=warm::recipe_text_backing(text);
    need(bytes!=0);
    visit(text.data(),bytes); out.bytes+=bytes; ++out.requests;
    const auto occurrence=std::find_if(projection.occurrences().begin(),projection.occurrences().end(),
        [&](const auto &o){return o.key==row.occurrence;});
    need(occurrence!=projection.occurrences().end() && matches(fields,row,occurrence->key,work));
  }
  out.comparison_work=work.result().charged;
  return out;
}
}  // namespace membership_study_observation
