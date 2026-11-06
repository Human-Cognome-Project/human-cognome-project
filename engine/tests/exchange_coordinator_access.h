#pragma once
#include "assembly/exchange_coordinator.h"
namespace assembly::testing {
struct ExchangeCoordinatorAccess {
  static auto backing(const detail::ExchangeCoordinator &c) {
    return std::array<std::pair<const void *, std::size_t>,5>{{
      {c.commands_.data(),c.commands_.capacity()},
      {c.intrinsics_.data(),c.intrinsics_.capacity()},
      {c.nodes_.data(),c.nodes_.capacity()},
      {c.children_.data(),c.children_.capacity()},
      {c.contributions_.data(),c.contributions_.capacity()}}};
  }
  static auto prepare(std::unique_ptr<detail::ExchangeBindings> b,
                      const std::vector<detail::ExchangeIncoming> &incoming,
                      const std::vector<detail::ExchangeNewField> &fields = {}) {
    return std::unique_ptr<detail::ExchangeCoordinator>(
        new detail::ExchangeCoordinator(std::move(b), incoming, fields));
  }
  static const auto &commands(const detail::ExchangeCoordinator &c) { return c.commands_; }
  static const auto &mappings(const detail::ExchangeCoordinator &c) { return c.bound_->occurrences_; }
  static const auto &fields(const detail::ExchangeCoordinator &c) { return c.bound_->fields_; }
  static const auto &planned_fields(const detail::ExchangeCoordinator &c) { return c.next_fields_; }
  static const auto &field_commands(const detail::ExchangeCoordinator &c) { return c.field_commands_; }
  static auto generation(const detail::ExchangeCoordinator &c) { return c.mapping_generation_; }
  static auto work(const detail::ExchangeCoordinator &c) { return c.work_; }
  static const auto &outgoing_report(const detail::ExchangeCoordinator &c) { return c.outgoing_report_; }
  static const auto &balance_report(const detail::ExchangeCoordinator &c) { return c.balance_report_; }
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
