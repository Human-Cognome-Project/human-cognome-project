#pragma once
#include "assembly/structural_admission_plan.h"
#include "assembly/prepared_occurrence_edit.h"

namespace viewport { struct PreparedOwnerAccess; }
namespace assembly::detail {
struct PreparedOwnerResult;
class PreparedOccurrenceWorkspace;
enum class PreparedOwnerPhase { Prepared, Published, Failed };

// Private, exclusive logical owner on one borrowed prepared Harness. Runtime
// outlives Harness, which outlives this owner and every borrowed observation.
// Preparation pins an immutable source and reserves all publication metadata.
// Bootstrap uses native generated placement/commit; this is not a second engine
// and does not change legacy execution. Opt-in edits reuse declared templates;
// definition/exposure replacement and general live revision remain separate.
class PreparedStudyOwner {
 public:
  static PreparedOwnerResult prepare(field::Harness &,
      const std::shared_ptr<const ActiveAssembly> &, const warm::Uuid &session,
      const AdmissionBindings &, const std::shared_ptr<ReservationDomain> &,
      std::uint64_t plan_work, std::uint64_t metadata_work,
      bool editable_occurrences = false);
  ~PreparedStudyOwner();
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
  PreparedOccurrenceEditResult edit(const warm::Uuid &, const ExecutionHandle &,
      const PreparedOccurrenceEdit &);
  // Detached completed physical values; source/claim metadata has one builder
  // shared with the viewport observer. No comparison policy is selected here.
  ActiveReport report(const warm::Uuid &, const ExecutionHandle &);
  const ActiveAssembly &source(const warm::Uuid &, const ExecutionHandle &) const;
  const ReservedVector<engine::RecordHandle> &bodies(const warm::Uuid &, const ExecutionHandle &) const;
  const ReservedVector<engine::RecordHandle> &fields(const warm::Uuid &, const ExecutionHandle &) const;
  // Loans into the pinned source, consumed typed rows and committed claims. The
  // owner outlives the view. Rows/references from read() expire at the next
  // mutation; retain the stamped view, not its rows, to check again afterwards.
  struct AssociationRows {
    // Live order indexes immutable catalogue rows without copying source pins.
    struct Occurrences {
      const ReservedVector<AdmissionOccurrenceCommand> &catalogue;
      const ReservedVector<std::size_t> *live{};
      std::size_t size() const { return live ? live->size() : catalogue.size(); }
      const AdmissionOccurrenceCommand &at(std::size_t i) const {
        return catalogue.at(live ? live->at(i) : i);
      }
      const AdmissionOccurrenceCommand &operator[](std::size_t i) const { return at(i); }
    };
    const ActiveAssembly &source;
    Occurrences occurrences;
    const ReservedVector<AdmissionFieldCommand> &field_sources;
    const ReservedVector<engine::RecordHandle> &bodies, &fields;
    const ReservedVector<ScaleBinding> &scales;
    const std::vector<std::uint64_t> &exact_masses; // Native exact ledger, never physical floating mass.
    struct Body {
      const AdmissionOccurrenceCommand &source;
      const engine::RecordHandle &claim;
      const ScaleBinding &scale;
      std::uint64_t exact_mass;
    };
    struct Field {
      const AdmissionFieldCommand &source;
      const engine::RecordHandle &claim;
    };
    Body body(std::size_t ordinal) const;
    Field field(std::size_t ordinal) const;
  };
  class AssociationView {
   public:
    AssociationRows read() const;
   private:
    friend class PreparedStudyOwner;
    explicit AssociationView(const PreparedStudyOwner &, std::uint64_t tick);
    const PreparedStudyOwner *owner_;
    std::uint64_t generation_, tick_;
  };
  AssociationView associations(const warm::Uuid &, const ExecutionHandle &) const;
 private:
  friend struct viewport::PreparedOwnerAccess;
  friend class PreparedOccurrenceWorkspace;
  ActiveReport report_metadata(const warm::Uuid &, const ExecutionHandle &) const;
  PreparedStudyOwner(field::Harness &, const std::shared_ptr<const ActiveAssembly> &,
      ReservedUnique<StructuralAdmissionPlan>, const std::shared_ptr<ReservationDomain> &);
  void reserve_metadata(const warm::Uuid &, const AdmissionBindings &, PreparationWork &);
  void authorize(const warm::Uuid &, const ExecutionHandle &) const;
  void require_published(const warm::Uuid &, const ExecutionHandle &) const;
  field::Harness &h_;
  std::shared_ptr<ReservationDomain> domain_;
  std::shared_ptr<const ActiveAssembly> source_;
  ReservedUnique<StructuralAdmissionPlan> plan_; // Preparing/retry only; no published plan loan.
  ExecutionHandle handle_;
  // Native handles align with the live association view. The optional workspace
  // indexes reserved instance metadata; default owners retain original order.
  // Only prepared assignments/swaps occur after commit; no key allocation.
  ReservedVector<engine::RecordHandle> bodies_, fields_;
  ReservedVector<ScaleBinding> scales_; // Pointer-free reporting metadata.
  // Moved from the private plan after all native claim checks. No key copy or
  // allocation at publication. Original command ordinals are historical only;
  // replay storage is retired, while standalone plan APIs remain unchanged.
  ReservedVector<AdmissionOccurrenceCommand> occurrences_;
  ReservedVector<AdmissionFieldCommand> field_sources_;
  ReservedUnique<PreparedOccurrenceWorkspace> edits_;
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
