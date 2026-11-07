#pragma once
#include "assembly/exchange_bindings.h"
#include "assembly/hierarchy_seal_budget.h"

namespace assembly {
namespace testing { struct ExchangeCoordinatorAccess; }
namespace detail {
// Trusted prepared definition/layout binding, not a runtime exchange request.
// Geometry and native kind/scale installation remain the preparing owner's job.
struct ExchangeIncoming {
  OccurrenceKey occurrence;
  warm::ToolRef definition;
  int node{-1}, kind{1}, address{-1};
  double radius{};
  field::PlacementGeometry geometry;
};
struct ExchangeNewField {
  FieldKey field;
  int kind{1}, address{-1};
};

// One prepared, generation-bound transaction. A successful transaction consumes
// it and publishes the new complete occurrence map. Rejection permits retry.
class ExchangeCoordinator {
 private:
  friend struct assembly::testing::ExchangeCoordinatorAccess;
  friend class ExchangeOwner;
  explicit ExchangeCoordinator(field::Harness &);
  void reserve(const field::Harness &);
  void prepare(const std::vector<ExchangeIncoming> &,
               const std::vector<ExchangeNewField> &);
  ExchangeCoordinator(std::unique_ptr<ExchangeBindings>,
                      const std::vector<ExchangeIncoming> &,
                      const std::vector<ExchangeNewField> & = {});
  HierarchySealResult hierarchy_charge(const field::LiveRefreshSpec &) const;
  HierarchySealResult prepare_hierarchy(const field::LiveRefreshSpec &, HierarchySealBudget);
  field::UpdateResult execute(std::uint64_t generation, std::uint64_t placement_work,
                              std::uint64_t cells, std::uint64_t hierarchy_work);
  std::unique_ptr<ExchangeBindings> bound_;
  std::vector<field::UpdateCommand> commands_;
  std::vector<field::PlacementIntrinsic> intrinsics_;
  ExchangeMap<OccurrenceBinding> next_;
  ExchangeMap<ExchangeFieldBinding> next_fields_;
  // -1 for retained fields; otherwise the sealed DefineField command index.
  std::vector<int> field_commands_;
  std::vector<field::LiveRefreshNode> nodes_;
  std::vector<int> children_;
  std::vector<field::LiveRefreshContribution> contributions_;
  std::size_t survivors_{};
  std::uint64_t mapping_generation_{}, work_{};
  bool sealed_{}, consumed_{};
  // Descriptive report of the successful transaction, never numerical input.
  std::array<double, 8> outgoing_report_{};
  std::array<double, 32> balance_report_{};
};
}  // namespace detail
}  // namespace assembly
