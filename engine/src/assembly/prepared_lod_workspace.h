#pragma once
#include "assembly/prepared_study_owner.h"

namespace assembly::detail {
// Prepared metadata/command scratch in the existing owner's reservation domain.
// Editable single-byte templates support presence edits. Fixed presence
// uses the ordered replacement cohorts of the two prepared exposure plans.
// Both immutable plans and all maps/scratch live in the owner's reservation
// domain. No native state or execution authority lives here. The owner swaps
// these maps only after the one native transaction has completely committed.
class PreparedLodWorkspace {
  friend class PreparedStudyOwner;
 public:
  PreparedLodWorkspace(PreparedStudyOwner &, const ActiveAssembly &,
      ReservedUnique<StructuralAdmissionPlan>, bool editable, bool fixed_character,
      const std::vector<std::uint64_t> &initially_live, PreparationWork &);
 private:
  struct Participation { std::size_t occurrence{}, field{}; field::UpdateCommand command; };
  struct LiveTemplate { std::size_t occurrence{}; field::UpdateRef body; };
  struct Side {
    ReservedUnique<StructuralAdmissionPlan> plan;
    ReservedVector<Participation> edges;
    ReservedVector<std::size_t> scope_publishers;
    explicit Side(ReservationDomain *d):edges(ReservationAllocator<Participation>(d)),
        scope_publishers(ReservationAllocator<std::size_t>(d)){}
  };
  field::UpdateResult presence(const PreparedStudyOwner &, const PreparedOccurrenceEdit &, PreparationWork &);
  field::UpdateResult exposure(const PreparedStudyOwner &, const PreparedLodEdit &, PreparationWork &);
  field::UpdateResult build(const PreparedStudyOwner &, PreparationWork &);
  field::UpdateResult build_catalogue(const PreparedStudyOwner &, PreparationWork &);
  void publish(PreparedStudyOwner &);
  field::LiveRefreshSpec hierarchy() const;
  const auto &field_sources() const { return sides_[side_].plan->fields(); }
  const auto &body_sources() const {
    return fixed_presence_ ? sides_[side_].plan->occurrences() : catalogue_edits_ ? rows_ : catalogue_;
  }
  const auto *body_selection() const {
    return fixed_presence_ ? nullptr : catalogue_edits_ ? &row_ordinals_ : &live_;
  }
  const auto *field_selection() const { return catalogue_edits_ ? &field_ordinals_ : nullptr; }
  warm::Exposure exposure() const { return side_ ? warm::Exposure::Parts : warm::Exposure::Aggregate; }
  std::array<Side,2> sides_;
  ReservedVector<AdmissionOccurrenceCommand> catalogue_, next_catalogue_;
  AdmissionOccurrenceCommand incoming_source_;
  ReservedVector<std::size_t> templates_, live_, next_, order_;
  ReservedVector<unsigned char> known_;
  ReservedVector<int> addresses_, field_commands_;
  ReservedVector<engine::RecordHandle> next_bodies_, next_fields_, outgoing_;
  ReservedVector<field::UpdateCommand> commands_;
  ReservedVector<field::PlacementIntrinsic> intrinsics_;
  ReservedVector<field::PlacementExchangeGroup> groups_, fixed_groups_;
  ReservedVector<field::LiveRefreshNode> nodes_;
  ReservedVector<int> children_;
  ReservedVector<field::LiveRefreshContribution> contributions_;
  ReservedVector<AdmissionOccurrenceCommand> rows_, candidate_rows_;
  ReservedVector<std::size_t> row_ordinals_, next_row_ordinals_, offsets_;
  ReservedVector<std::size_t> incoming_rows_, incoming_positions_;
  ReservedVector<std::size_t> field_ordinals_, next_field_ordinals_;
  ReservedVector<int> field_indices_, node_indices_, scope_branches_;
  ReservedVector<LiveTemplate> live_templates_;
  field::UpdateRef bootstrap_focus_;
  std::size_t side_{}, target_{}, selected_{}, source_template_{}, path_capacity_{}, address_stride_{};
  const bool catalogue_edits_;
  bool bootstrapping_{};
  const bool fixed_presence_;
  bool changing_{}, adding_{}, new_identity_{}, scoped_rows_{};
};
}
