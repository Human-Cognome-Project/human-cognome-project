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
};
}  // namespace assembly::testing
