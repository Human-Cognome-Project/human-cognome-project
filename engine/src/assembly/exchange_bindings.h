#pragma once

#include "assembly/exchange_contract.h"
#include "field/field.h"
#include "field/prepared_storage.h"

namespace assembly {
namespace testing { struct ExchangeBindingAccess; struct ExchangeCoordinatorAccess; }
namespace detail {
class ExchangeCoordinator;
struct OccurrenceBinding {
  OccurrenceKey occurrence;
  engine::RecordHandle handle;
};
struct ExchangeFieldBinding {
  FieldKey field;
  engine::RecordHandle handle;
};
// Private preparation boundary. Initial identity bindings must come from the
// trusted owner's committed admission results, never an exchange request.
// Harness lifetime and exclusive ownership encompass this generation-bound view.
class ExchangeBindings {
 private:
  friend class ExchangeCoordinator;
  friend struct assembly::testing::ExchangeBindingAccess;
  friend struct assembly::testing::ExchangeCoordinatorAccess;
  ExchangeBindings(field::Harness &, const LetterProjection &, LetterExchangeContract,
                   std::vector<OccurrenceBinding>, std::vector<ExchangeFieldBinding>);
  field::UpdateResult validate(std::uint64_t generation, std::uint64_t work) const;
  struct Edge {
    std::size_t row;
    field::detail::RelationshipHeader header;
    std::uint64_t count;
  };
  field::Harness &h_;
  LetterExchangeContract contract_;
  std::vector<OccurrenceBinding> occurrences_;
  std::vector<ExchangeFieldBinding> fields_;
  std::vector<std::uint64_t> masses_;
  std::vector<engine::RecordHandle> outgoing_;
  std::vector<Edge> edges_;
  std::vector<bool> outgoing_rows_, edge_rows_;
  std::uint64_t generation_{}, work_{};
  std::size_t source_edges_{};
};
}  // namespace detail
}  // namespace assembly
