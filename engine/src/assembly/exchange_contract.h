#pragma once

#include "assembly/letter_projection.h"

namespace assembly {
namespace detail { class ExchangeOwner; }
// Structural source descriptors only. No positions, velocities or native slots.
struct ExchangeSource {
  OccurrenceKey occurrence;
  warm::ToolRef definition;
  std::uint64_t mass{};
  int node{-1};
  std::vector<warm::Parent> parents;
};
struct ExchangeLeaf {
  std::uint32_t character{}, byte{}, nibble{};
  warm::ToolRef definition;
  std::uint64_t count{};
};

// Prepared from one immutable validated v2 projection. This is a declaration,
// not proof of a current native binding or a reusable publication authority.
class LetterExchangeContract {
 public:
  const OccurrenceKey &owner() const { return owner_; }
  const warm::BundleRef &recipe() const { return recipe_; }
  warm::Exposure from() const { return from_; }
  warm::Exposure to() const { return to_; }
  std::uint64_t mass() const { return mass_; }
  const auto &outgoing() const { return outgoing_; }
  const auto &incoming() const { return incoming_; }
  const auto &leaves() const { return leaves_; }
  std::uint64_t match_work() const { return match_work_; }
  // Exact bijections, independent of adapter row order. No allocation/mutation.
  bool matches(const ExchangeSource *outgoing, std::size_t outgoing_count,
               const ExchangeSource *incoming, std::size_t incoming_count,
               std::uint64_t work) const;

 private:
  friend class detail::ExchangeOwner;
  friend struct testing::ExchangeOwnerAccess;
  explicit LetterExchangeContract(const std::shared_ptr<detail::ReservationDomain> &);
  LetterExchangeContract(const LetterExchangeContract &, const std::shared_ptr<detail::ReservationDomain> &);
  static LetterExchangeContract collect(const LetterProjection &, const OccurrenceKey &,
      warm::Exposure, const std::shared_ptr<detail::ReservationDomain> &);
  friend LetterExchangeContract declare_letter_exchange(
      const LetterProjection &, const OccurrenceKey &, warm::Exposure);
  OccurrenceKey owner_;
  warm::BundleRef recipe_;
  warm::Exposure from_{}, to_{};
  std::uint64_t mass_{}, match_work_{};
  detail::SharedReservedVector<ExchangeSource> outgoing_, incoming_;
  detail::SharedReservedVector<ExchangeLeaf> leaves_;
};

// Preparation operation. Target must be the other declared alternative of the
// exact aggregate occurrence in this study; legacy starts are never adapted.
LetterExchangeContract declare_letter_exchange(
    const LetterProjection &, const OccurrenceKey &, warm::Exposure target);
}  // namespace assembly
