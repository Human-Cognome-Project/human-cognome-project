#pragma once

#include "assembly/letter_projection.h"

namespace assembly {
namespace detail { class ExchangeOwner; class ExchangeBindings; class ExchangeCoordinator; class PreparedLodWorkspace; }
namespace testing { struct ExchangeContractAccess; }
// Structural source descriptors only. No positions, velocities or native slots.
struct ExchangeSource {
  OccurrenceKey occurrence;
  warm::ToolRef definition;
  std::uint64_t mass{};
  int node{-1};
  detail::SharedReservedVector<warm::Parent> parents;
  ExchangeSource() = default;
  ExchangeSource(OccurrenceKey key, warm::ToolRef ref, std::uint64_t exact_mass, int scale,
      detail::SharedReservedVector<warm::Parent> values)
      : occurrence(std::move(key)), definition(std::move(ref)), mass(exact_mass),
        node(scale), parents(std::move(values)) {}
 private:
  friend class LetterExchangeContract;
  friend struct testing::ExchangeContractAccess;
  friend struct testing::ExchangeOwnerAccess;
  // Only interpreted with the containing declaration's strong recipe pin.
  std::size_t recipe_tool_{SIZE_MAX};
};

// Prepared from one immutable validated v2 projection. This is a declaration,
// not proof of a current native binding or a reusable publication authority.
class LetterExchangeContract {
 public:
  LetterExchangeContract(const LetterExchangeContract &);
  LetterExchangeContract(LetterExchangeContract &&) noexcept = default;
  LetterExchangeContract &operator=(const LetterExchangeContract &);
  LetterExchangeContract &operator=(LetterExchangeContract &&) noexcept;
  const OccurrenceKey &owner() const { return owner_; }
  const warm::BundleRef &recipe() const { return recipe_; }
  warm::Exposure from() const { return from_; }
  warm::Exposure to() const { return to_; }
  std::uint64_t mass() const { return mass_; }
  const auto &outgoing() const { return outgoing_; }
  const auto &incoming() const { return incoming_; }
  // Ordered nibble provenance is already owned by the byte side's paths/parents.
  // That side is outgoing for from()==Parts, incoming for from()==Aggregate.
  std::uint64_t match_work() const { return match_work_; }
  // Exact bijections of caller-owned descriptors, independent of adapter row
  // order. Private recipe indices require their declaration context instead.
  bool matches(const ExchangeSource *outgoing, std::size_t outgoing_count,
               const ExchangeSource *incoming, std::size_t incoming_count,
               std::uint64_t work) const;

 private:
  friend class detail::ExchangeOwner;
  friend class detail::PreparedLodWorkspace;
  friend class detail::ExchangeBindings;
  friend class detail::ExchangeCoordinator;
  friend struct testing::ExchangeOwnerAccess;
  friend struct testing::ExchangeContractAccess;
  LetterExchangeContract(const OccurrenceKey &, const warm::BundleRef &,
      const std::shared_ptr<detail::ReservationDomain> &, detail::PreparationWork * = nullptr);
  LetterExchangeContract(const LetterExchangeContract &, const std::shared_ptr<detail::ReservationDomain> &,
      detail::PreparationWork * = nullptr);
  static LetterExchangeContract collect(const LetterProjection &, const OccurrenceKey &,
      warm::Exposure, const std::shared_ptr<detail::ReservationDomain> &, detail::PreparationWork * = nullptr,
      std::shared_ptr<const warm::Bundle> = {});
  const detail::SharedReservedVector<warm::Parent> &parents(const ExchangeSource &,
      detail::PreparationWork * = nullptr) const;
  bool matches(const LetterExchangeContract &, bool reverse, std::uint64_t work) const;
  friend LetterExchangeContract declare_letter_exchange(
      const LetterProjection &, const OccurrenceKey &, warm::Exposure);
  OccurrenceKey owner_;
  warm::BundleRef recipe_;
  warm::Exposure from_{}, to_{};
  std::uint64_t mass_{}, match_work_{};
  detail::SharedReservedVector<ExchangeSource> outgoing_, incoming_;
  std::shared_ptr<const warm::Bundle> recipe_pin_;
};

// Preparation operation. Target must be the other declared alternative of the
// exact aggregate occurrence in this study; legacy starts are never adapted.
LetterExchangeContract declare_letter_exchange(
    const LetterProjection &, const OccurrenceKey &, warm::Exposure target);
}  // namespace assembly
