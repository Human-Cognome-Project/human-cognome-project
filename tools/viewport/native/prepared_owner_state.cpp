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
    owner.require_published(authorization,handle);
    if (request && (!std::isfinite(request->gravity) || request->gravity < 0))
      throw std::invalid_argument("viewport: invalid diagnostic gravity");
    auto &h = owner.h_;
    const auto &s = h.prepared_storage();
    const auto &plan = *owner.plan_;
    assembly::ActiveReport report;
    report.handle = owner.handle_;
    report.owner = owner.source_->owner();
    report.instance = owner.source_->instance();
    report.origins = owner.source_->origins();
    auto &mapping = report.prepared;
    mapping.capacity = h.particle_capacity();
    mapping.scales.assign(owner.scales_.begin(),owner.scales_.end());
    mapping.groups.resize(h.field_capacity());
    for (std::size_t i=0; i<plan.fields().size(); ++i) {
      const auto &field = plan.fields()[i].field;
      mapping.groups.at(owner.fields_[i].record) = {field.kind,warm::key(field.object)};
    }
    for (std::size_t i=0; i<plan.occurrences().size(); ++i) {
      const auto &entry = plan.occurrences()[i];
      // Identity/scale metadata only. All physical values are subsequently read
      // from the authoritative wide execution by the existing Harness observer.
      assembly::Occurrence occurrence{assembly::InputText(assembly::occurrence_text(entry.occurrence)),
          assembly::InputText(warm::key(entry.definition.object)),owner.bodies_[i].record,entry.node,{},{}};
      const auto mass = *plan.commands()[entry.command].exact_mass;
      mapping.particles.push_back({std::move(occurrence),mass,{},{},0,0});
    }
    for (std::size_t i=0; i<s.live.focus; ++i) mapping.focal_groups.push_back(s.focal[i].record);
    auto binding = viewport::bind(h,std::move(report));
    if (binding.bodies.size() != owner.bodies_.size())
      throw std::invalid_argument("viewport: incomplete prepared owner mapping");
    for (std::size_t i=0; i<binding.bodies.size(); ++i) {
      const auto &a = binding.bodies[i], &b = owner.bodies_[i];
      if (a.context != b.context || a.address != b.address ||
          a.incarnation != b.incarnation || a.record != b.record)
        throw std::invalid_argument("viewport: mismatched prepared owner claim");
    }
    return viewport::observe(h,binding,request);
  }
};
CompletedState observe(assembly::detail::PreparedStudyOwner &owner, const warm::Uuid &authorization,
    const assembly::ExecutionHandle &handle, std::optional<IntentRequest> request) {
  return PreparedOwnerAccess::observe(owner,authorization,handle,request);
}
}
