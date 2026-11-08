#pragma once
#include "assembly/exchange_owner.h"
#include "exchange_coordinator_access.h"
namespace assembly::testing {
struct ExchangeOwnerAccess {
  static auto resources(const detail::ExchangeOwner &o) { return o.storage(); }
  static auto &inactive_path(detail::ExchangeOwner &o) {
    return o.occurrences_[o.occurrences_.storage().size()-1].occurrence.source.path;
  }
  static auto &field_command_buffer(detail::ExchangeOwner &o) {
    return ExchangeCoordinatorAccess::field_command_buffer(*o.plan_);
  }
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
    detail::PreparationWork meter(UINT64_MAX); return o.declaration(key,target,meter);
  }
  static auto input_admission(const LetterProjection &p,
      const std::vector<detail::OccurrenceBinding> &rows,
      const std::vector<detail::ExchangeFieldBinding> &fields, detail::AdoptionInputBudget budget) {
    return detail::ExchangeOwner::input_admission(p,rows,fields,budget);
  }
  static auto adopt(field::Harness &h, const LetterProjection &p,
      const std::vector<detail::OccurrenceBinding> &rows,
      const std::vector<detail::ExchangeFieldBinding> &fields,
      detail::AdoptionInputBudget budget, detail::AdoptionInputResult &result) {
    return detail::ExchangeOwner::adopt(h,p,rows,fields,budget,result);
  }
  static auto adopt(field::Harness &h, const LetterProjection &p,
      const std::vector<detail::OccurrenceBinding> &rows,
      const std::vector<detail::ExchangeFieldBinding> &fields) {
    detail::AdoptionInputResult result;
    auto owner = adopt(h,p,rows,fields,{UINT64_MAX,UINT64_MAX,UINT64_MAX},result);
    if (!owner) throw warm::Error("test owner input admission failed");
    return owner;
  }
  static void prepare(detail::ExchangeOwner &o, const OccurrenceKey &key, warm::Exposure target,
      const std::vector<detail::ExchangeIncoming> &incoming,
      const std::vector<detail::ExchangeNewField> &fields) {
    if (o.prepare(key,target,incoming,fields,UINT64_MAX,UINT64_MAX,UINT64_MAX).status != detail::CaptureStatus::Ok)
      throw warm::Error("test owner capture work overflow");
  }
  static auto prepare(detail::ExchangeOwner &o, const OccurrenceKey &key, warm::Exposure target,
      const std::vector<detail::ExchangeIncoming> &incoming,
      const std::vector<detail::ExchangeNewField> &fields, std::uint64_t capture_work) {
    const auto result = o.prepare(key,target,incoming,fields,UINT64_MAX,capture_work,UINT64_MAX);
    if (result.phase == detail::PreparationPhase::Commands) throw warm::Error("test command work overflow");
    return result.capture;
  }
  static auto prepare(detail::ExchangeOwner &o, const OccurrenceKey &key, warm::Exposure target,
      const std::vector<detail::ExchangeIncoming> &incoming,
      const std::vector<detail::ExchangeNewField> &fields,
      std::uint64_t capture_work, std::uint64_t command_work) {
    return o.prepare(key,target,incoming,fields,UINT64_MAX,capture_work,command_work);
  }
  static auto prepare(detail::ExchangeOwner &o, const OccurrenceKey &key, warm::Exposure target,
      const std::vector<detail::ExchangeIncoming> &incoming,
      const std::vector<detail::ExchangeNewField> &fields,
      std::uint64_t staging, std::uint64_t capture, std::uint64_t commands) {
    return o.prepare(key,target,incoming,fields,staging,capture,commands);
  }
  static const auto &projection(const detail::ExchangeOwner &o) { return *o.projection_; }
  static const auto &spare_projection(const detail::ExchangeOwner &o) { return *o.prospective_; }
  static auto storage(const detail::ExchangeOwner &o) { return o.projection_storage(); }
  static void stage_projection(detail::ExchangeOwner &o, const OccurrenceKey &key, warm::Exposure target) {
    detail::PreparationWork meter(UINT64_MAX); o.prepare_projection(key,target,meter);
  }
  static const auto &mappings(const detail::ExchangeOwner &o) { return o.occurrences_; }
  static const auto &fields(const detail::ExchangeOwner &o) { return o.fields_; }
  static const auto &plan(const detail::ExchangeOwner &o) { return *o.plan_; }
  static auto &plan(detail::ExchangeOwner &o) { return *o.plan_; }
  static bool pending(const detail::ExchangeOwner &o) { return o.plan_ && !o.completed_; }
  static auto generation(const detail::ExchangeOwner &o) { return o.generation_; }
  static const auto &outgoing_report(const detail::ExchangeOwner &o) { return o.outgoing_report_; }
  static const auto &balance_report(const detail::ExchangeOwner &o) { return o.balance_report_; }
  static auto seal(detail::ExchangeOwner &o, const field::LiveRefreshSpec &spec,
      detail::HierarchySealBudget budget) { return o.seal(spec,budget); }
  static void seal(detail::ExchangeOwner &o, const field::LiveRefreshSpec &spec) {
    const auto charge = ExchangeCoordinatorAccess::hierarchy_charge(*o.plan_,spec);
    if (o.seal(spec,{charge.required_bytes,charge.required_work}).status != detail::HierarchySealStatus::Ok)
      throw warm::Error("test owner hierarchy sealing rejected");
  }
  static auto execute(detail::ExchangeOwner &o, std::uint64_t generation,
      std::uint64_t work = UINT64_MAX) { return o.execute(generation,work,UINT64_MAX,UINT64_MAX); }
};
}
