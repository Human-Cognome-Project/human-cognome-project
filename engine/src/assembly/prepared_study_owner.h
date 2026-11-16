#pragma once
#include "assembly/structural_admission_plan.h"

namespace viewport { struct PreparedOwnerAccess; }
namespace assembly::detail {
struct PreparedOwnerResult;
enum class PreparedOwnerPhase { Prepared, Published, Failed };

// Private, exclusive logical owner on one borrowed prepared Harness. Runtime
// outlives Harness, which outlives this owner and every borrowed observation.
// Preparation pins an immutable source and reserves all publication metadata.
// Bootstrap uses native generated placement/commit; this is not a second engine
// and does not change the legacy ActiveSession or provide general live edits.
class PreparedStudyOwner {
 public:
  static PreparedOwnerResult prepare(field::Harness &,
      const std::shared_ptr<const ActiveAssembly> &, const warm::Uuid &session,
      const AdmissionBindings &, const std::shared_ptr<ReservationDomain> &,
      std::uint64_t plan_work, std::uint64_t metadata_work);
  PreparedStudyOwner(const PreparedStudyOwner &) = delete;
  PreparedStudyOwner &operator=(const PreparedStudyOwner &) = delete;
  const ExecutionHandle &handle() const { return handle_; }
  PreparedOwnerPhase phase() const { return phase_; }
  bool ready() const;
  std::uint64_t generation() const { return generation_; }
  field::UpdateResult bootstrap(const warm::Uuid &owner, const ExecutionHandle &,
      std::uint64_t placement_work, std::uint64_t cells, std::uint64_t hierarchy_work);
  field::TickResult tick(const warm::Uuid &owner, const ExecutionHandle &,
      const field::Parameters64 &, std::uint64_t tick_work = UINT64_MAX,
      std::uint64_t geometry_work = UINT64_MAX);
  const ActiveAssembly &source(const warm::Uuid &, const ExecutionHandle &) const;
  const StructuralAdmissionPlan &plan(const warm::Uuid &, const ExecutionHandle &) const;
  const ReservedVector<engine::RecordHandle> &bodies(const warm::Uuid &, const ExecutionHandle &) const;
  const ReservedVector<engine::RecordHandle> &fields(const warm::Uuid &, const ExecutionHandle &) const;
 private:
  friend struct viewport::PreparedOwnerAccess;
  PreparedStudyOwner(field::Harness &, const std::shared_ptr<const ActiveAssembly> &,
      ReservedUnique<StructuralAdmissionPlan>, const std::shared_ptr<ReservationDomain> &);
  void reserve_metadata(const warm::Uuid &, const AdmissionBindings &, PreparationWork &);
  void authorize(const warm::Uuid &, const ExecutionHandle &) const;
  void require_published(const warm::Uuid &, const ExecutionHandle &) const;
  field::Harness &h_;
  std::shared_ptr<ReservationDomain> domain_;
  std::shared_ptr<const ActiveAssembly> source_;
  ReservedUnique<StructuralAdmissionPlan> plan_;
  ExecutionHandle handle_;
  // Native handles align with the plan's immutable typed identity/index tables.
  // Only fixed-size assignments occur after native commit. No keys are copied.
  ReservedVector<engine::RecordHandle> bodies_, fields_;
  ReservedVector<ScaleBinding> scales_; // Pointer-free reporting metadata.
  std::uint64_t context_{}, generation_{}, initial_ticks_{};
  PreparedOwnerPhase phase_{PreparedOwnerPhase::Prepared};
};
struct PreparedOwnerResult {
  AdmissionPlanStatus status{AdmissionPlanStatus::InvalidInput};
  PreparationResult plan_work, metadata_work;
  field::BudgetFailure failure;
  ReservationResult reservation;
  ReservedUnique<PreparedStudyOwner> owner;
};
}
