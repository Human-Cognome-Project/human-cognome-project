#pragma once
#include "assembly/exchange_coordinator.h"
namespace assembly::testing {
struct ExchangeCoordinatorAccess {
  static auto prepare(std::unique_ptr<detail::ExchangeBindings> b,
                      const std::vector<detail::ExchangeIncoming> &incoming) {
    return std::unique_ptr<detail::ExchangeCoordinator>(
        new detail::ExchangeCoordinator(std::move(b), incoming));
  }
  static const auto &commands(const detail::ExchangeCoordinator &c) { return c.commands_; }
  static const auto &mappings(const detail::ExchangeCoordinator &c) { return c.bound_->occurrences_; }
  static auto generation(const detail::ExchangeCoordinator &c) { return c.mapping_generation_; }
  static auto work(const detail::ExchangeCoordinator &c) { return c.work_; }
  static void seal(detail::ExchangeCoordinator &c, const field::LiveRefreshSpec &spec) {
    c.prepare_hierarchy(spec);
  }
  static auto execute(detail::ExchangeCoordinator &c, std::uint64_t generation,
      std::uint64_t work = UINT64_MAX, std::uint64_t cells = UINT64_MAX,
      std::uint64_t hierarchy_work = UINT64_MAX) {
    return c.execute(generation, work, cells, hierarchy_work);
  }
};
}
