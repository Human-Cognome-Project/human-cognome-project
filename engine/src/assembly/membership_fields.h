#pragma once

#include "assembly/letter_projection.h"

namespace assembly {
namespace detail { class ExchangeOwner; class ExchangeBindings; }
namespace testing { struct MembershipFieldsAccess; }
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
  const auto &contributions() const {
    return contributions_;
  }
  // Extend this same projection's bounded input values. The native prepare
  // path validates active references and handles membership as share-1 edges.
  // Caller still owns layout, runtime and (in D) any warm pins/materialization.
  StudyInput bind(const LetterProjection &projection,
                  const std::vector<StudyNode> &nodes) const;
  StructuralStudyInput bind_structure(
      const LetterProjection &projection,
      const std::vector<StudyNode> &nodes) const;

 private:
  MembershipFields(const warm::Uuid &study, const warm::BundleRef &recipe,
      const std::shared_ptr<detail::ReservationDomain> &domain)
      : study_(study,domain),
        recipe_{warm::Uuid(recipe.selection,domain),warm::Uuid(recipe.revision,domain)},
        contributions_(detail::SharedReservationAllocator<MembershipContribution>(domain)) {}
  friend MembershipFields membership_fields(const LetterProjection &);
  friend class detail::ExchangeOwner;
  friend class detail::ExchangeBindings;
  friend struct testing::MembershipFieldsAccess;
  friend struct testing::ExchangeOwnerAccess;
  static MembershipFields collect(const LetterProjection &, bool alternatives);
  static MembershipFields collect(const LetterProjection &, bool alternatives,
      const std::shared_ptr<detail::ReservationDomain> &);
  warm::Uuid study_;
  warm::BundleRef recipe_;
  bool alternatives_{};
  detail::SharedReservedVector<MembershipContribution> contributions_;
};

// Follow only actual pinned upward memberships of each owning letter. Dedup
// targets per live occurrence; never rewrite source graphs or merge uses.
// Exposed bytes inherit their owner's groups, not other uses of the same byte.
MembershipFields membership_fields(const LetterProjection &projection);
}  // namespace assembly
