#pragma once
#include "assembly/exchange_coordinator.h"
namespace assembly::testing {
struct ExchangeCoordinatorAccess {
  static bool consumed(const detail::ExchangeCoordinator &c) { return c.consumed_; }
  template<class Visit> static void map_paths(const detail::ExchangeCoordinator &c, Visit visit) {
    for (const auto &row : c.bound_->occurrences_.storage()) visit(row.occurrence.source.path);
    for (const auto &row : c.next_.storage()) visit(row.occurrence.source.path);
  }
  template<class Visit> static void map_addresses(const detail::ExchangeCoordinator &c, Visit visit) {
    for (const auto &row : c.bound_->fields_.storage()) visit(row.field.object.address);
    for (const auto &row : c.next_fields_.storage()) visit(row.field.object.address);
  }
  static auto map_outer_bytes(const detail::ExchangeCoordinator &c) {
    const auto bytes = [](const auto &map) { return map.storage().capacity()*sizeof(map.storage()[0]); };
    return bytes(c.bound_->occurrences_)+bytes(c.bound_->fields_)+bytes(c.next_)+bytes(c.next_fields_);
  }
  static auto &field_command_buffer(detail::ExchangeCoordinator &c) { return c.field_commands_; }
  static auto map_backing(const detail::ExchangeCoordinator &c) {
    std::vector<std::pair<const void *,std::size_t>> result;
    const auto append = [&](const auto &map) {
      const auto &slots = map.storage();
      result.push_back({slots.data(),slots.capacity()*sizeof(slots[0])});
      for (const auto &row : slots) {
        const auto &key = detail::key_buffer(row);
        result.push_back({key.data(),key.capacity()*sizeof(key[0])});
      }
    };
    append(c.bound_->occurrences_); append(c.bound_->fields_);
    append(c.next_); append(c.next_fields_);
    return result;
  }
  static const auto *memberships(const detail::ExchangeCoordinator &c) { return c.bound_->memberships_.get(); }
  static void stage_binding(detail::ExchangeCoordinator &c,
      std::shared_ptr<const LetterExchangeContract> declaration,
      std::shared_ptr<const MembershipFields> memberships,
      const detail::ExchangeMap<detail::OccurrenceBinding> &rows,
      const detail::ExchangeMap<detail::ExchangeFieldBinding> &fields) {
    detail::PreparationWork meter(UINT64_MAX);
    c.bound_->stage(std::move(declaration),std::move(memberships),rows,fields,meter);
  }
  static auto capture(detail::ExchangeCoordinator &c, const LetterProjection &p, std::uint64_t work) {
    return c.bound_->capture(p,work);
  }
  static void capture(detail::ExchangeCoordinator &c, const LetterProjection &p) {
    if (capture(c,p,UINT64_MAX).status != detail::CaptureStatus::Ok)
      throw warm::Error("test capture work overflow");
  }
  static const auto *binding(const detail::ExchangeCoordinator &c) { return c.bound_.get(); }
  static auto binding_backing(const detail::ExchangeCoordinator &c) {
    const auto &b = *c.bound_;
    return std::array<std::pair<const void *,std::size_t>,5>{{
      {b.masses_.data(),b.masses_.capacity()}, {b.outgoing_.data(),b.outgoing_.capacity()},
      {b.edges_.data(),b.edges_.capacity()}, {nullptr,b.outgoing_rows_.capacity()},
      {nullptr,b.edge_rows_.capacity()}}};
  }
  static auto binding_status(const detail::ExchangeCoordinator &c) {
    return c.bound_->validate(c.bound_->h_.prepared_storage().generation,UINT64_MAX).status;
  }
  static void rebind(detail::ExchangeCoordinator &c, const LetterProjection &p,
      std::shared_ptr<const LetterExchangeContract> declaration,
      const detail::ExchangeMap<detail::OccurrenceBinding> &rows,
      const detail::ExchangeMap<detail::ExchangeFieldBinding> &fields) {
    if (c.bound_->prepare(p,std::move(declaration),c.bound_->memberships_,rows,fields,UINT64_MAX).status !=
        detail::CaptureStatus::Ok) throw warm::Error("test rebind work overflow");
  }
  static const auto *declaration(const detail::ExchangeCoordinator &c) { return c.bound_->contract_.get(); }
  static auto backing(const detail::ExchangeCoordinator &c) {
    return std::array<std::pair<const void *, std::size_t>,5>{{
      {c.commands_.data(),c.commands_.capacity()},
      {c.intrinsics_.data(),c.intrinsics_.capacity()},
      {c.nodes_.data(),c.nodes_.capacity()},
      {c.children_.data(),c.children_.capacity()},
      {c.contributions_.data(),c.contributions_.capacity()}}};
  }
  static auto prepare(detail::ReservedUnique<detail::ExchangeBindings> b,
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
    const auto charge = c.hierarchy_charge(spec);
    if (c.prepare_hierarchy(spec,{charge.required_bytes,charge.required_work}).status !=
        detail::HierarchySealStatus::Ok) throw warm::Error("test hierarchy sealing rejected");
  }
  static auto hierarchy_charge(const detail::ExchangeCoordinator &c, const field::LiveRefreshSpec &s) {
    return c.hierarchy_charge(s);
  }
  static bool sealed(const detail::ExchangeCoordinator &c) { return c.sealed_; }
  static auto execute(detail::ExchangeCoordinator &c, std::uint64_t generation,
      std::uint64_t work = UINT64_MAX, std::uint64_t cells = UINT64_MAX,
      std::uint64_t hierarchy_work = UINT64_MAX) {
    return c.execute(generation, work, cells, hierarchy_work);
  }
};
}
