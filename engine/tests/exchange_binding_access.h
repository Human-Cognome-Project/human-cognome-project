#pragma once
#include "assembly/exchange_bindings.h"

namespace assembly::testing {
struct ExchangeBindingAccess {
  static detail::ReservedUnique<detail::ExchangeBindings> prepare(field::Harness &h,
      const LetterProjection &p, LetterExchangeContract c,
      std::vector<detail::OccurrenceBinding> rows,
      std::vector<detail::ExchangeFieldBinding> fields) {
    return detail::reserve_unique<detail::ExchangeBindings>({},[&](auto *storage) {
      ::new (static_cast<void *>(storage)) detail::ExchangeBindings(h, p, std::move(c), std::move(rows), std::move(fields));
    });
  }
  static auto validate(const detail::ExchangeBindings &b, std::uint64_t generation = UINT64_MAX,
                       std::uint64_t work = UINT64_MAX) {
    return b.validate(generation == UINT64_MAX ? b.generation_ : generation,
                      work == UINT64_MAX ? b.work_ : work);
  }
  static auto outgoing(const detail::ExchangeBindings &b) { return b.outgoing_; }
  static auto work(const detail::ExchangeBindings &b) { return b.work_; }
  static const auto &mappings(const detail::ExchangeBindings &b) { return b.occurrences_; }
  static const auto &fields(const detail::ExchangeBindings &b) { return b.fields_; }
  static const auto &masses(const detail::ExchangeBindings &b) { return b.masses_; }
  static const auto &outgoing_view(const detail::ExchangeBindings &b) { return b.outgoing_; }
  static const auto &edges(const detail::ExchangeBindings &b) { return b.edges_; }
  static const auto &outgoing_mask(const detail::ExchangeBindings &b) { return b.outgoing_rows_; }
  static const auto &edge_mask(const detail::ExchangeBindings &b) { return b.edge_rows_; }
  static auto generation(const detail::ExchangeBindings &b) { return b.generation_; }
  static const auto &declaration(const detail::ExchangeBindings &b) { return b.contract_; }
  static const auto &catalogue(const detail::ExchangeBindings &b) { return b.memberships_; }
  template<class Inspect> static auto scratch(field::Harness &h,
      const std::shared_ptr<detail::ReservationDomain> &domain, std::uint64_t allowance, Inspect inspect) {
    detail::PreparationWork meter(allowance);
    meter.charge(sizeof(detail::ExchangeBindings));
    detail::ExchangeBindings check(h,domain,&meter);
    inspect(check,meter);
    return meter.result();
  }
  static auto recapture(detail::ExchangeBindings &b, const LetterProjection &p,
      std::shared_ptr<const LetterExchangeContract> declaration, warm::Exposure target,
      std::shared_ptr<const MembershipFields> memberships,
      const detail::ExchangeMap<detail::OccurrenceBinding> &rows,
      const detail::ExchangeMap<detail::ExchangeFieldBinding> &fields, detail::PreparationWork &meter) {
    return b.capture_adoption(p,std::move(declaration),target,std::move(memberships),rows,fields,meter);
  }
  static auto validate_adoption(const detail::ExchangeBindings &b,
      const detail::ExchangeMap<detail::OccurrenceBinding> &rows,
      const detail::ExchangeMap<detail::ExchangeFieldBinding> &fields) {
    return b.validate(b.generation_,b.work_,rows,fields);
  }
  template<class Inspect> static auto adoption(field::Harness &h, const LetterProjection &p,
      std::shared_ptr<const LetterExchangeContract> declaration,
      std::shared_ptr<const MembershipFields> memberships,
      const detail::ExchangeMap<detail::OccurrenceBinding> &rows,
      const detail::ExchangeMap<detail::ExchangeFieldBinding> &fields,
      const std::shared_ptr<detail::ReservationDomain> &domain, std::uint64_t allowance, Inspect inspect) {
    detail::PreparationWork meter(allowance);
    meter.charge(sizeof(detail::ExchangeBindings)); // Same stack-object setup fee as owner adoption.
    detail::ExchangeBindings check(h,p,std::move(declaration),std::move(memberships),rows,fields,domain,meter);
    inspect(check);
    return meter.result();
  }
};
}  // namespace assembly::testing
