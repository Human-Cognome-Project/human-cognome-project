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
      std::optional<IntentRequest> request, std::optional<ResponseRequest> responses) {
    if (request && (!std::isfinite(request->gravity) || request->gravity < 0))
      throw std::invalid_argument("viewport: invalid diagnostic gravity");
    auto &h = owner.h_;
    auto report = owner.report_metadata(authorization,handle);
    const auto &bodies = owner.bodies(authorization,handle);
    auto binding = viewport::bind(h,std::move(report));
    if (binding.bodies.size() != bodies.size())
      throw std::invalid_argument("viewport: incomplete prepared owner mapping");
    for (std::size_t i=0; i<binding.bodies.size(); ++i) {
      const auto &a = binding.bodies[i], &b = bodies[i];
      if (a.context != b.context || a.address != b.address ||
          a.incarnation != b.incarnation || a.record != b.record)
        throw std::invalid_argument("viewport: mismatched prepared owner claim");
    }
    return viewport::observe(h,std::move(binding),request,responses);
  }
};
CompletedState observe(assembly::detail::PreparedStudyOwner &owner, const warm::Uuid &authorization,
    const assembly::ExecutionHandle &handle, std::optional<IntentRequest> request,
    std::optional<ResponseRequest> responses) {
  return PreparedOwnerAccess::observe(owner,authorization,handle,request,responses);
}
}
