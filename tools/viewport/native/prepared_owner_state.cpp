#include "snapshot.h"
#include "assembly/prepared_study_owner.h"
#include "field/prepared_storage.h"
#include <cmath>
#include <stdexcept>

namespace viewport {
// Observation is separately requested work. These detached report copies are
// made only after the private owner's publication/authorization checks; they
// are never part of native commit or owner-map publication.
struct PreparedOwnerAccess {
  static CompletedState observe(assembly::detail::PreparedStudyOwner &owner,
      const warm::Uuid &authorization, const assembly::ExecutionHandle &handle,
      std::optional<IntentRequest> request) {
    const auto associations = owner.associations(authorization,handle).read();
    if (request && (!std::isfinite(request->gravity) || request->gravity < 0))
      throw std::invalid_argument("viewport: invalid diagnostic gravity");
    auto &h = owner.h_;
    const auto &s = h.prepared_storage();
    const auto &plan = associations.plan;
    assembly::ActiveReport report;
    report.handle = owner.handle();
    report.owner = associations.source.owner();
    report.instance = associations.source.instance();
    report.origins = associations.source.origins();
    auto &mapping = report.prepared;
    mapping.capacity = h.particle_capacity();
    mapping.scales.assign(associations.scales.begin(),associations.scales.end());
    mapping.groups.resize(h.field_capacity());
    for (std::size_t i=0; i<plan.fields().size(); ++i) {
      const auto field = associations.field(i);
      mapping.groups.at(field.claim.record) = {field.source.field.kind,warm::key(field.source.field.object)};
    }
    for (std::size_t i=0; i<plan.occurrences().size(); ++i) {
      const auto body = associations.body(i);
      const auto &entry = body.source;
      // Identity/scale metadata only. All physical values are subsequently read
      // from the authoritative wide execution by the existing Harness observer.
      assembly::Occurrence occurrence{assembly::InputText(assembly::occurrence_text(entry.occurrence)),
          assembly::InputText(warm::key(entry.definition.object)),body.claim.record,entry.node,{},{}};
      mapping.particles.push_back({std::move(occurrence),body.exact_mass,{},{},0,0});
    }
    for (std::size_t i=0; i<s.live.focus; ++i) mapping.focal_groups.push_back(s.focal[i].record);
    auto binding = viewport::bind(h,std::move(report));
    if (binding.bodies.size() != associations.bodies.size())
      throw std::invalid_argument("viewport: incomplete prepared owner mapping");
    for (std::size_t i=0; i<binding.bodies.size(); ++i) {
      const auto &a = binding.bodies[i], &b = associations.body(i).claim;
      if (a.context != b.context || a.address != b.address ||
          a.incarnation != b.incarnation || a.record != b.record)
        throw std::invalid_argument("viewport: mismatched prepared owner claim");
    }
    return viewport::observe(h,std::move(binding),request);
  }
};
CompletedState observe(assembly::detail::PreparedStudyOwner &owner, const warm::Uuid &authorization,
    const assembly::ExecutionHandle &handle, std::optional<IntentRequest> request) {
  return PreparedOwnerAccess::observe(owner,authorization,handle,request);
}
}
