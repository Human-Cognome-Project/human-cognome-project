#pragma once
#include "assembly/structural_admission_plan.h"

namespace assembly {
struct PreparedLodAlternative {
  std::shared_ptr<const ActiveAssembly> source;
  detail::AdmissionBindings bindings;
  // A supplied live set activates only those declared roots at bootstrap.
  // Both exposure bindings still describe the complete editable catalogue.
  std::vector<std::uint64_t> initially_live{};
};
// Explicit setup operation for the existing shared v2/f64 subset. Layout roles,
// units and all model/resource choices are supplied; native layout pointers must
// be null and are materialized by the session. No viewport values or starts.
// Passed by value so the session can consume these temporary binding tables.
struct PreparedStudySubmission {
  std::shared_ptr<const ActiveAssembly> source;
  detail::AdmissionBindings bindings;
  field::PreparedCapacity capacity;
  std::shared_ptr<detail::ReservationDomain> resources;
  field::HostPath path{field::HostPath::BulkCopy};
  // Reserve an owner-local edit workspace before readiness. The source's
  // declared definitions stay immutable; declared presence and bounded root
  // copies of those definitions can change in the live study.
  bool editable_occurrences{false};
  // Explicit setup of the other whole-field exposure on the same immutable
  // recipe/study. Native roles/geometry/fields are validated before readiness.
  // With presence edits disabled, each root keeps its own replacement cohort.
  std::optional<PreparedLodAlternative> lod_alternative{};
  std::uint64_t plan_work{UINT64_MAX}, metadata_work{UINT64_MAX};
  std::uint64_t placement_work{UINT64_MAX}, placement_cells{UINT64_MAX}, hierarchy_work{UINT64_MAX};
};
struct PreparedSubmissionResult {
  // Present only after native publication and active-session publication succeed.
  std::optional<ExecutionHandle> handle;
  detail::AdmissionPlanStatus preparation_status{detail::AdmissionPlanStatus::InvalidInput};
  detail::PreparationResult plan_work, metadata_work;
  field::BudgetFailure failure;
  detail::ReservationResult reservation;
  field::UpdateResult publication;
};
}  // namespace assembly
