#pragma once
#include "assembly/exchange_owner.h"
namespace assembly::testing {
struct ExchangeOwnerAccess {
  static auto adopt(field::Harness &h, LetterProjection p,
      std::vector<detail::OccurrenceBinding> rows, std::vector<detail::ExchangeFieldBinding> fields) {
    return std::unique_ptr<detail::ExchangeOwner>(
        new detail::ExchangeOwner(h, std::move(p), std::move(rows), std::move(fields)));
  }
  static void prepare(detail::ExchangeOwner &o, const OccurrenceKey &key, warm::Exposure target,
      const std::vector<detail::ExchangeIncoming> &incoming,
      const std::vector<detail::ExchangeNewField> &fields) { o.prepare(key,target,incoming,fields); }
  static const auto &projection(const detail::ExchangeOwner &o) { return *o.projection_; }
  static const auto &mappings(const detail::ExchangeOwner &o) { return o.occurrences_; }
  static const auto &fields(const detail::ExchangeOwner &o) { return o.fields_; }
  static const auto &plan(const detail::ExchangeOwner &o) { return *o.plan_; }
  static bool pending(const detail::ExchangeOwner &o) { return o.plan_ && !o.completed_; }
  static auto generation(const detail::ExchangeOwner &o) { return o.generation_; }
  static const auto &outgoing_report(const detail::ExchangeOwner &o) { return o.outgoing_report_; }
  static const auto &balance_report(const detail::ExchangeOwner &o) { return o.balance_report_; }
  static void seal(detail::ExchangeOwner &o, const field::LiveRefreshSpec &spec) { o.seal(spec); }
  static auto execute(detail::ExchangeOwner &o, std::uint64_t generation,
      std::uint64_t work = UINT64_MAX) { return o.execute(generation,work,UINT64_MAX,UINT64_MAX); }
};
}
