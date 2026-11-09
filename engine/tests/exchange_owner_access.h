#pragma once
#include "assembly/exchange_owner.h"
#include "exchange_coordinator_access.h"
namespace assembly::testing {
struct ExchangeOwnerAccess {
  static auto resources(const detail::ExchangeOwner &o) { return o.storage(); }
  static auto reservations(const detail::ExchangeOwner &o) { return o.reservations_->result(); }
  static auto reservation_domain(const detail::ExchangeOwner &o) { return o.reservations_; }
  static auto projection_tables(const LetterProjection &p) {
    return std::array<std::pair<const void *,std::size_t>,4>{{
      {p.exposures_.data(),p.exposures_.capacity()*sizeof(warm::Exposure)},
      {p.occurrences_.data(),p.occurrences_.capacity()*sizeof(ProjectedOccurrence)},
      {p.parents_.data(),p.parents_.capacity()*sizeof(ParentContribution)},
      {p.spare_parents_.data(),p.spare_parents_.capacity()*sizeof(ParentContribution)}}};
  }
  static auto projection_table_bytes(const LetterProjection &p) {
    std::uint64_t bytes = 0;
    for (const auto &entry : projection_tables(p)) bytes += entry.second;
    return bytes;
  }
  static auto projection_domains(const LetterProjection &p) {
    return std::array<detail::ReservationDomain *,4>{{p.exposures_.get_allocator().domain.get(),
      p.occurrences_.get_allocator().domain.get(),p.parents_.get_allocator().domain.get(),
      p.spare_parents_.get_allocator().domain.get()}};
  }
  static auto projection_copy(const LetterProjection &p, const std::shared_ptr<detail::ReservationDomain> &domain) {
    return LetterProjection(p,domain);
  }
  static void reserve_projection(LetterProjection &p) {
    detail::PreparationWork meter(UINT64_MAX); p.reserve_exposure(meter);
  }
  static auto projection_parent_sizes(const LetterProjection &p) {
    return std::make_pair(p.parents_.size(),p.spare_parents_.size());
  }
  static auto unique_objects(const detail::ExchangeOwner &o) {
    return std::array<std::pair<const void *,std::size_t>,5>{{
      {&o,sizeof(o)}, {o.projection_.get(),sizeof(*o.projection_)},
      {o.prospective_.get(),sizeof(*o.prospective_)}, {o.plan_.get(),sizeof(*o.plan_)},
      {ExchangeCoordinatorAccess::binding(*o.plan_),sizeof(detail::ExchangeBindings)}}};
  }
  static auto map_outer_bytes(const detail::ExchangeOwner &o) {
    return o.occurrences_.storage().capacity()*sizeof(detail::OccurrenceBinding)+
        o.fields_.storage().capacity()*sizeof(detail::ExchangeFieldBinding)+
        ExchangeCoordinatorAccess::map_outer_bytes(*o.plan_);
  }
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
    detail::ReservationResult reservation;
    detail::ProjectionSetupResult setup;
    return detail::ExchangeOwner::adopt(h,p,rows,fields,budget,result,{UINT64_MAX,UINT64_MAX},reservation,UINT64_MAX,setup);
  }
  static auto adopt(field::Harness &h, const LetterProjection &p,
      const std::vector<detail::OccurrenceBinding> &rows,
      const std::vector<detail::ExchangeFieldBinding> &fields,
      detail::ReservationBudget budget, detail::ReservationResult &result) {
    detail::AdoptionInputResult input;
    detail::ProjectionSetupResult setup;
    return detail::ExchangeOwner::adopt(h,p,rows,fields,{UINT64_MAX,UINT64_MAX,UINT64_MAX},input,budget,result,UINT64_MAX,setup);
  }
  static auto adopt(field::Harness &h, const LetterProjection &p,
      const std::vector<detail::OccurrenceBinding> &rows,
      const std::vector<detail::ExchangeFieldBinding> &fields,
      std::uint64_t setup_work, detail::ProjectionSetupResult &setup) {
    detail::AdoptionInputResult input;
    detail::ReservationResult reservation;
    return detail::ExchangeOwner::adopt(h,p,rows,fields,{UINT64_MAX,UINT64_MAX,UINT64_MAX},input,
        {UINT64_MAX,UINT64_MAX},reservation,setup_work,setup);
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
