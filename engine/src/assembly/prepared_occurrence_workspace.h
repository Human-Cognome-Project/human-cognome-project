#pragma once
#include "assembly/prepared_study_owner.h"

namespace assembly::detail {
// Reserved scratch and immutable admission templates, owned by the existing
// prepared owner. It owns no native execution or independent source authority.
class PreparedOccurrenceWorkspace {
  friend class PreparedStudyOwner;
 public:
  PreparedOccurrenceWorkspace(PreparedStudyOwner &, PreparationWork &);
 private:
  struct Participation {
    std::size_t occurrence{}, field{};
    std::uint64_t count{}, ordinal{};
    float share{};
  };
  field::UpdateResult prepare(const PreparedStudyOwner &, const PreparedOccurrenceEdit &,
                              PreparationWork &);
  field::LiveRefreshSpec hierarchy() const;
  void publish(PreparedStudyOwner &);
  ReservedVector<std::size_t> live_, next_, node_fields_;
  ReservedVector<AdmissionOccurrenceCommand> catalogue_;
  ReservedVector<std::size_t> templates_;
  ReservedVector<int> addresses_;
  ReservedVector<unsigned char> known_;
  AdmissionOccurrenceCommand incoming_source_;
  ReservedVector<engine::RecordHandle> next_bodies_;
  ReservedVector<field::UpdateCommand> definitions_, commands_;
  ReservedVector<field::PlacementIntrinsic> intrinsics_;
  ReservedVector<Participation> participation_;
  ReservedVector<field::LiveRefreshNode> nodes_;
  ReservedVector<int> children_;
  ReservedVector<field::LiveRefreshContribution> contributions_;
  field::PlacementIntrinsic incoming_;
  std::size_t incoming_template_{}, incoming_slot_{};
  int incoming_address_{};
  bool adding_{}, new_identity_{};
};
}  // namespace assembly::detail
