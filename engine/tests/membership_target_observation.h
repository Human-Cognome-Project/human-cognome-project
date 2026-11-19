#pragma once
#include <algorithm>
#include <array>
#include "assembly/membership_fields.h"
#include "assembly/preparation_work.h"
#include "membership_fields_access.h"

// Test-only attribution of the actual owning pins in a returned catalogue.
// Recipe lookup supplies a comparison witness, not a retained runtime loan.
namespace membership_target_observation {
inline void need(bool ok) {
  if (!ok) throw warm::Error("membership target ownership inspection failed");
}
inline const warm::Tool *lookup(const warm::Uuid &study,const warm::BundleRef &recipe,
    const assembly::LetterProjection &projection,const warm::ToolRef &target,
    assembly::detail::PreparationWork &work) {
  work.charge(1);
  if (!work.equal(study,projection.study()) ||
      !work.equal(recipe.selection,projection.recipe().ref.selection) ||
      !work.equal(recipe.revision,projection.recipe().ref.revision)) return nullptr;
  const auto &tools=projection.recipe().tools;
  const auto it=std::find_if(tools.begin(),tools.end(),[&](const auto &tool) {
    return work.equal(tool.ref,target);
  });
  return it==tools.end()?nullptr:&*it;
}
struct Cost {
  std::size_t rows{},unique_targets{},requests{},bytes{},unique_requests{},unique_bytes{},pin_capacity_bytes{};
  std::size_t index_capacity_bytes{},table_capacity{};
  std::uint64_t lookup_work{};
};
template<class Rows,class Visit> Cost inspect(const assembly::MembershipFields &fields,
    const warm::BundleRef &recipe,const assembly::LetterProjection &projection,const Rows &rows,
    const assembly::detail::ReservationDomain *domain,Visit visit) {
  Cost out;
  std::array<const void *,4096> seen{};
  std::size_t used=0;
  assembly::detail::PreparationWork work(UINT64_MAX);
  using Access=assembly::testing::MembershipFieldsAccess;
  const auto &targets=Access::targets(fields);
  out.table_capacity=targets.capacity();
  out.pin_capacity_bytes=targets.capacity()*sizeof(warm::ToolRef);
  out.index_capacity_bytes=rows.capacity()*sizeof(rows[0].target_index);
  need(targets.get_allocator().domain.get()==domain);
  if (out.pin_capacity_bytes) visit(targets.data(),out.pin_capacity_bytes);
  for (std::size_t i=0;i<rows.size();++i) {
    const auto &row=rows[i];
    const auto &target=Access::target(fields,row);
    const auto *source=lookup(fields.study(),recipe,projection,target,work);
    need(source && row.kind==assembly::GroupKind::Membership);
    ++out.rows;
    bool first=true;
    for (std::size_t j=0;j<i;++j) if (work.equal(Access::target(fields,rows[j]),target)) {
      need(rows[j].target_index==row.target_index && &Access::target(fields,rows[j])==&target);
      first=false;break;
    }
    out.unique_targets+=first;
    if (!first) continue;
    const auto request=[&](const void *p,const void *original,std::size_t bytes) {
      if (!bytes) return;
      need(p!=original && used<seen.size() &&
           std::find(seen.begin(),seen.begin()+used,p)==seen.begin()+used);
      seen[used++]=p;
      visit(p,bytes); ++out.requests; out.bytes+=bytes;
      if (first) {++out.unique_requests;out.unique_bytes+=bytes;}
    };
    const auto text=[&](const auto &value,const auto &original) {
      need(value.get_allocator().domain.get()==domain);
      request(value.data(),original.data(),warm::recipe_text_backing(value));
    };
    text(target.object.scope.str(),source->ref.object.scope.str());
    text(target.revision.str(),source->ref.revision.str());
    need(target.object.address.get_allocator().domain.get()==domain);
    request(target.object.address.data(),source->ref.object.address.data(),
            target.object.address.capacity()*sizeof(target.object.address[0]));
  }
  out.lookup_work=work.result().charged;
  need(out.unique_targets==targets.size());
  return out;
}
} // namespace membership_target_observation
