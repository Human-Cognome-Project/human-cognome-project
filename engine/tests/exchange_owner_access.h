#pragma once
#include "assembly/exchange_owner.h"
#include "exchange_coordinator_access.h"
namespace assembly::testing {
struct ExchangeOwnerAccess {
  static auto map_backing(const detail::ExchangeOwner &o) {
    auto result = ExchangeCoordinatorAccess::map_backing(*o.plan_);
    const auto append = [&](const auto &map) {
      const auto &slots = map.storage();
      result.push_back({slots.data(),slots.capacity()*sizeof(slots[0])});
      for (const auto &row : slots) {
        const auto &key = detail::key_buffer(row);
        result.push_back({key.data(),key.capacity()*sizeof(key[0])});
      }
    };
    append(o.occurrences_); append(o.fields_);
    std::sort(result.begin(),result.end(),[](const auto &a, const auto &b) {
      return std::less<const void *>{}(a.first,b.first);
    });
    return result;
  }
  static const auto &memberships(const detail::ExchangeOwner &o) { return o.memberships_; }
  static bool equivalent(const LetterExchangeContract &a, const LetterExchangeContract &b) {
    if (!(a.owner() == b.owner()) || a.recipe().selection != b.recipe().selection ||
        a.recipe().revision != b.recipe().revision || a.from() != b.from() || a.to() != b.to() ||
        a.mass() != b.mass() || a.match_work() != b.match_work() || a.leaves().size() != b.leaves().size() ||
        !a.matches(b.outgoing().data(),b.outgoing().size(),b.incoming().data(),b.incoming().size(),UINT64_MAX))
      return false;
    for (std::size_t i = 0; i < a.leaves().size(); ++i) {
      const auto &x = a.leaves()[i], &y = b.leaves()[i];
      if (x.character != y.character || x.byte != y.byte || x.nibble != y.nibble ||
          !(x.definition == y.definition) || x.count != y.count) return false;
    }
    return true;
  }
  static const auto &declarations(const detail::ExchangeOwner &o) { return o.declarations_; }
  static auto declaration(const detail::ExchangeOwner &o, const OccurrenceKey &key, warm::Exposure target) {
    return o.declaration(key,target);
  }
  static auto adopt(field::Harness &h, LetterProjection p,
      std::vector<detail::OccurrenceBinding> rows, std::vector<detail::ExchangeFieldBinding> fields) {
    return std::unique_ptr<detail::ExchangeOwner>(
        new detail::ExchangeOwner(h, std::move(p), std::move(rows), std::move(fields)));
  }
  static void prepare(detail::ExchangeOwner &o, const OccurrenceKey &key, warm::Exposure target,
      const std::vector<detail::ExchangeIncoming> &incoming,
      const std::vector<detail::ExchangeNewField> &fields) { o.prepare(key,target,incoming,fields); }
  static const auto &projection(const detail::ExchangeOwner &o) { return *o.projection_; }
  static const auto &spare_projection(const detail::ExchangeOwner &o) { return *o.prospective_; }
  static auto storage(const detail::ExchangeOwner &o) { return o.projection_storage(); }
  static void stage_projection(detail::ExchangeOwner &o, const OccurrenceKey &key, warm::Exposure target) {
    o.prepare_projection(key,target);
  }
  static const auto &mappings(const detail::ExchangeOwner &o) { return o.occurrences_; }
  static const auto &fields(const detail::ExchangeOwner &o) { return o.fields_; }
  static const auto &plan(const detail::ExchangeOwner &o) { return *o.plan_; }
  static auto &plan(detail::ExchangeOwner &o) { return *o.plan_; }
  static bool pending(const detail::ExchangeOwner &o) { return o.plan_ && !o.completed_; }
  static auto generation(const detail::ExchangeOwner &o) { return o.generation_; }
  static const auto &outgoing_report(const detail::ExchangeOwner &o) { return o.outgoing_report_; }
  static const auto &balance_report(const detail::ExchangeOwner &o) { return o.balance_report_; }
  static void seal(detail::ExchangeOwner &o, const field::LiveRefreshSpec &spec) { o.seal(spec); }
  static auto execute(detail::ExchangeOwner &o, std::uint64_t generation,
      std::uint64_t work = UINT64_MAX) { return o.execute(generation,work,UINT64_MAX,UINT64_MAX); }
};
}
