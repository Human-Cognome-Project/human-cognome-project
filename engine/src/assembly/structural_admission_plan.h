#pragma once
#include "assembly/active_workbench.h"
#include "assembly/preparation_work.h"
#include "field/live_hierarchy.h"
#include "field/prepared_placement.h"

namespace assembly::detail {
// Private preparing-owner inputs, borrowed only during build. No native records
// or numerical starts may enter them. They do not grant admission authority.
struct AdmissionSource {
  warm::Uuid owner, study, generation;
  warm::BundleRef recipe;
};
struct AdmissionLayout {
  StudyNode node;
  double units{1}; // Explicit root-relative operative units, validated by role.
};
struct AdmissionBody {
  OccurrenceKey occurrence;
  warm::ToolRef definition;
  int node{-1}, kind{1}, address{-1};
  double mass{}; // Physical mass, distinct from the exact source denominator.
  field::PlacementGeometry geometry;
};
struct AdmissionField {
  FieldKey field;
  int kind{1}, address{-1}; // Native field declaration extent is Fields, not Potential.
};
struct AdmissionComparison {
  FieldKey field;
  std::uint64_t comparison{};
  int level{}, retained_fraction_bits{52};
  std::vector<int> children; // Explicit ordered topology; children precede parent.
};
struct AdmissionCharacterRollup {
  OccurrenceKey owner; // Inactive aggregate for this study's single scalar.
  warm::ToolRef character;
};
struct AdmissionBindings {
  AdmissionSource source;
  field::FrameAnchor frame_anchor{}; // Initial supported subset: root frame zero.
  std::vector<AdmissionLayout> layout;
  std::vector<AdmissionBody> bodies;
  std::vector<AdmissionField> fields;
  std::vector<AdmissionComparison> comparisons;
  std::vector<FieldKey> focus; // Must match the snapshot's requested order exactly.
  // Explicit byte-exposure root at ParentValue(character). The plan derives
  // its full per-byte counts and ordinals from the sealed projection.
  std::optional<AdmissionCharacterRollup> character_rollup;
};
struct AdmissionOccurrenceCommand {
  // The enclosing immutable plan owns the common study in source().study.
  warm::OccurrenceId occurrence;
  warm::ToolRef definition;
  int command{-1}, node{-1};
  double units{};
};
struct AdmissionFieldCommand {
  FieldKey field;
  int command{-1};
};
enum class AdmissionPlanStatus {
  Ok, InvalidInput, Unsupported, InvalidNumerics, CapacityExhausted,
  WorkExhausted, Overflow, AllocationExhausted
};
struct AdmissionPlanResult;

// Owns only a structural plan and pinned identity values, never a Harness or
// Program. Standalone outputs are immutable and die with this handle; hierarchy()
// borrows these stable tables. A private preparing owner may consume its own
// unloaned plan after native commit. A successful plan is NOT a native
// preflight token. The future exclusive owner must revalidate and commit it.
class StructuralAdmissionPlan {
 public:
  static AdmissionPlanResult build(const ActiveAssembly &, const AdmissionBindings &,
      const field::PreparedCapacity &, const std::shared_ptr<ReservationDomain> &,
      std::uint64_t preparation_work);
  StructuralAdmissionPlan(const StructuralAdmissionPlan &) = delete;
  StructuralAdmissionPlan &operator=(const StructuralAdmissionPlan &) = delete;
  const auto &source() const { return source_; }
  const auto &commands() const { return commands_; }
  const auto &intrinsics() const { return intrinsics_; }
  const auto &occurrences() const { return occurrences_; }
  const auto &fields() const { return fields_; }
  const auto &focus() const { return focus_; }
  field::LiveRefreshSpec hierarchy() const {
    return {nodes_.data(), nodes_.size(), children_.data(), children_.size(),
            contributions_.data(), contributions_.size()};
  }
 private:
  // Only the exclusive preparing owner may consume its own plan after native
  // commit. Standalone plans retain all immutable output for their lifetime.
  friend class PreparedStudyOwner;
  explicit StructuralAdmissionPlan(const std::shared_ptr<ReservationDomain> &);
  void prepare(const ActiveAssembly &, const AdmissionBindings &,
               const field::PreparedCapacity &, PreparationWork &);
  std::shared_ptr<ReservationDomain> domain_; // Outlives all attached tables.
  AdmissionSource source_;
  ReservedVector<field::UpdateCommand> commands_;
  ReservedVector<field::PlacementIntrinsic> intrinsics_;
  ReservedVector<AdmissionOccurrenceCommand> occurrences_;
  ReservedVector<AdmissionFieldCommand> fields_;
  ReservedVector<field::UpdateRef> focus_;
  ReservedVector<field::LiveRefreshNode> nodes_;
  ReservedVector<int> children_;
  ReservedVector<field::LiveRefreshContribution> contributions_;
};
struct AdmissionPlanResult {
  AdmissionPlanStatus status{AdmissionPlanStatus::InvalidInput};
  PreparationResult work;
  field::BudgetFailure failure;
  ReservationResult reservation;
  ReservedUnique<StructuralAdmissionPlan> plan;
};
}
