#pragma once
#include "assembly/prepared_study_owner.h"

namespace assembly::detail {
// Prepared metadata/command scratch in the existing owner's reservation domain.
// English single-byte templates support presence edits. The fixed Unicode
// character uses the ordered rows of its two prepared exposure plans.
// Both immutable plans and all maps/scratch live in the owner's reservation
// domain. No native state or execution authority lives here. The owner swaps
// these maps only after the one native transaction has completely committed.
class PreparedLodWorkspace {
  friend class PreparedStudyOwner;
 public:
  PreparedLodWorkspace(PreparedStudyOwner &, const ActiveAssembly &,
      ReservedUnique<StructuralAdmissionPlan>, bool fixed_character, PreparationWork &);
 private:
  struct Participation { std::size_t occurrence{}, field{}; field::UpdateCommand command; };
  struct Side {
    ReservedUnique<StructuralAdmissionPlan> plan;
    ReservedVector<Participation> edges;
    explicit Side(ReservationDomain *d):edges(ReservationAllocator<Participation>(d)){}
  };
  field::UpdateResult presence(const PreparedStudyOwner &, const PreparedOccurrenceEdit &, PreparationWork &);
  field::UpdateResult exposure(const PreparedStudyOwner &, const PreparedLodEdit &, PreparationWork &);
  field::UpdateResult build(const PreparedStudyOwner &, PreparationWork &);
  void publish(PreparedStudyOwner &);
  field::LiveRefreshSpec hierarchy() const;
  const auto &field_sources() const { return sides_[side_].plan->fields(); }
  const auto &body_sources() const {
    return fixed_character_ ? sides_[side_].plan->occurrences() : catalogue_;
  }
  const auto *body_selection() const { return fixed_character_ ? nullptr : &live_; }
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
  ReservedVector<field::PlacementExchangeGroup> groups_;
  ReservedVector<field::LiveRefreshNode> nodes_;
  ReservedVector<int> children_;
  ReservedVector<field::LiveRefreshContribution> contributions_;
  std::size_t side_{}, target_{}, selected_{}, source_template_{}, path_capacity_{};
  const bool fixed_character_;
  bool changing_{}, adding_{}, new_identity_{};
};
}
