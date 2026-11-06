#pragma once
#include "assembly/exchange_coordinator.h"

namespace assembly {
namespace testing { struct ExchangeOwnerAccess; }
namespace detail {
// Private owner of logical exposure and committed mappings on one borrowed
// prepared execution. The harness outlives this owner; all calls are exclusive.
class ExchangeOwner {
 private:
  friend struct assembly::testing::ExchangeOwnerAccess;
  ExchangeOwner(field::Harness &, LetterProjection, std::vector<OccurrenceBinding>,
                std::vector<ExchangeFieldBinding>);
  void prepare(const OccurrenceKey &, warm::Exposure,
               const std::vector<ExchangeIncoming> &, const std::vector<ExchangeNewField> &);
  void seal(const field::LiveRefreshSpec &);
  void prepare_projection(const OccurrenceKey &, warm::Exposure);
  ProjectionStorage projection_storage() const;
  field::UpdateResult execute(std::uint64_t, std::uint64_t, std::uint64_t, std::uint64_t);
  field::Harness &h_;
  std::unique_ptr<LetterProjection> projection_, prospective_;
  std::vector<OccurrenceBinding> occurrences_;
  std::vector<ExchangeFieldBinding> fields_;
  std::unique_ptr<ExchangeCoordinator> plan_;
  std::uint64_t generation_{};
  std::array<double,8> outgoing_report_{};
  std::array<double,32> balance_report_{};
  bool completed_{};
};
}
}
