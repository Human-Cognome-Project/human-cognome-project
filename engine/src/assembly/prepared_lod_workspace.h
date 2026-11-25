#pragma once
#include "assembly/prepared_study_owner.h"

namespace assembly::detail {
// Prepared metadata/command scratch in the existing owner's reservation domain.
// The first supported field has one physical row per logical occurrence at
// either exposure (the declared English single-byte composition).
// Both immutable plans and all maps/scratch live in the owner's reservation
// domain. No native state or execution authority lives here. The owner swaps
// these maps only after the one native transaction has completely committed.
class PreparedLodWorkspace {
  friend class PreparedStudyOwner;
 public:
  PreparedLodWorkspace(PreparedStudyOwner &, const ActiveAssembly &,
      ReservedUnique<StructuralAdmissionPlan>, PreparationWork &);
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
  bool changing_{}, adding_{}, new_identity_{};
};
}
