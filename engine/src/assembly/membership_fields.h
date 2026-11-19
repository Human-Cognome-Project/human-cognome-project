#pragma once

#include "assembly/letter_projection.h"

namespace assembly {
namespace detail { class ExchangeOwner; class ExchangeBindings; class ExchangeCoordinator; class StructuralAdmissionPlan; }
namespace testing { struct MembershipFieldsAccess; }
struct MembershipContribution {
  OccurrenceKey occurrence;
  GroupKind kind{GroupKind::Membership};
  // The complete target pin owns the field object; reads borrow target.object.
  // Exact pinned group revision; naming mass is metadata.
  warm::ToolRef target;
  std::uint64_t count{0};
};

class MembershipFields {
 public:
  const warm::Uuid &study() const { return study_; }
  std::size_t contribution_count() const { return contributions_.size(); }
  // Explicit detached value, including its complete owning study identity.
  // Internal numeric rows never escape as a context-free public reference.
  MembershipContribution contribution(std::size_t index) const;
  // Extend this same projection's bounded input values. The native prepare
  // path validates active references and handles membership as share-1 edges.
  // Caller still owns layout, runtime and (in D) any warm pins/materialization.
  StudyInput bind(const LetterProjection &projection,
                  const std::vector<StudyNode> &nodes) const;
  StructuralStudyInput bind_structure(
      const LetterProjection &projection,
      const std::vector<StudyNode> &nodes) const;

 private:
  struct Row {
    warm::OccurrenceId occurrence;
    GroupKind kind{GroupKind::Membership};
    warm::ToolRef target;
    std::uint64_t count{};
  };
  MembershipFields(const warm::Uuid &study, const warm::BundleRef &recipe,
      const std::shared_ptr<detail::ReservationDomain> &domain)
      : study_(study,domain),
        recipe_([&] {
          auto selection = warm::Uuid(recipe.selection,domain);
          auto revision = warm::Uuid(recipe.revision,domain);
          return warm::BundleRef{std::move(selection),std::move(revision)};
        }()),
        contributions_(detail::SharedReservationAllocator<Row>(domain)) {}
  friend MembershipFields membership_fields(const LetterProjection &);
  friend class detail::ExchangeOwner;
  friend class detail::ExchangeBindings;
  friend class detail::ExchangeCoordinator;
  friend class detail::StructuralAdmissionPlan;
  friend struct testing::MembershipFieldsAccess;
  friend struct testing::ExchangeOwnerAccess;
  static MembershipFields collect(const LetterProjection &, bool alternatives);
  static MembershipFields collect(const LetterProjection &, bool alternatives,
      const std::shared_ptr<detail::ReservationDomain> &, detail::PreparationWork * = nullptr);
  warm::Uuid study_;
  warm::BundleRef recipe_;
  bool alternatives_{};
  detail::SharedReservedVector<Row> contributions_;
};

// Follow only actual pinned upward memberships of each owning letter. Dedup
// targets per live occurrence; never rewrite source graphs or merge uses.
// Exposed bytes inherit their owner's groups, not other uses of the same byte.
MembershipFields membership_fields(const LetterProjection &projection);
}  // namespace assembly
