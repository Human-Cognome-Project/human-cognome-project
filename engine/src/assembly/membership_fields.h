#pragma once

#include "assembly/letter_projection.h"

namespace assembly {
struct MembershipContribution {
  OccurrenceKey occurrence;
  FieldKey field;
  // Exact pinned group revision; naming mass is metadata.
  warm::ToolRef target;
  std::uint64_t count{0};
};

class MembershipFields {
 public:
  const warm::Uuid &study() const { return study_; }
  const std::vector<MembershipContribution> &contributions() const {
    return contributions_;
  }
  // Extend this same projection's bounded input values. The native prepare
  // path validates active references and handles membership as share-1 edges.
  // Caller still owns layout, runtime and (in D) any warm pins/materialization.
  StudyInput bind(const LetterProjection &projection,
                  const std::vector<StudyNode> &nodes) const;

 private:
  MembershipFields() = default;
  friend MembershipFields membership_fields(const LetterProjection &);
  warm::Uuid study_;
  std::vector<MembershipContribution> contributions_;
};

// Follow only actual pinned upward memberships of each owning letter. Dedup
// targets per live occurrence; never rewrite source graphs or merge uses.
// Exposed bytes inherit their owner's groups, not other uses of the same byte.
MembershipFields membership_fields(const LetterProjection &projection);
}  // namespace assembly
