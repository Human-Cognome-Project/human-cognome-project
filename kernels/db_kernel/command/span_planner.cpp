#include "span_planner.h"

#include <limits>

namespace command {

std::optional<codec::Address> successor(const codec::Address &address) {
  if (address.empty()) {
    return std::nullopt;
  }
  for (const auto &e : address) {
    if (e.partial) {
      return std::nullopt;
    }
  }

  codec::Address out = address;
  for (std::size_t i = out.size(); i-- > 0;) {
    const int code = static_cast<int>(out[i].first) * codec::kAlphabetSize +
                      out[i].second + 1;
    if (code < codec::kCoupletSpace) {
      out[i].first = static_cast<uint8_t>(code / codec::kAlphabetSize);
      out[i].second = static_cast<uint8_t>(code % codec::kAlphabetSize);
      return out;
    }
    // This element rolled over; reset it and carry into the previous
    // (more significant) element.
    out[i].first = 0;
    out[i].second = 0;
    if (i == 0) {
      // Carried out of the leftmost element: this address is already the
      // last one representable at its current depth. See successor()'s
      // doc comment -- G5, not resolved here.
      return std::nullopt;
    }
  }
  return std::nullopt;  // unreachable
}

std::optional<std::size_t> span_length(const codec::Address &from,
                                        const codec::Address &to) {
  if (from.size() != to.size()) {
    return std::nullopt;
  }
  for (const auto &e : from) {
    if (e.partial) return std::nullopt;
  }
  for (const auto &e : to) {
    if (e.partial) return std::nullopt;
  }

  // (to - from) as a base-kCoupletSpace number, least-significant (last)
  // element first, with borrow -- exact digit-wise subtraction, the same
  // positional arithmetic successor() increments by one step.
  std::vector<long long> diff(from.size());
  long long borrow = 0;
  for (std::size_t i = from.size(); i-- > 0;) {
    const long long a =
        static_cast<long long>(to[i].first) * codec::kAlphabetSize + to[i].second;
    const long long b = static_cast<long long>(from[i].first) * codec::kAlphabetSize +
                         from[i].second;
    long long d = a - b - borrow;
    if (d < 0) {
      d += codec::kCoupletSpace;
      borrow = 1;
    } else {
      borrow = 0;
    }
    diff[i] = d;
  }
  if (borrow != 0) {
    return std::nullopt;  // to precedes from: not a forward run.
  }

  constexpr unsigned long long kMax = std::numeric_limits<unsigned long long>::max();
  unsigned long long total = 0;
  for (long long digit : diff) {
    if (total > (kMax - static_cast<unsigned long long>(digit)) / codec::kCoupletSpace) {
      return std::nullopt;  // too large to represent as a slot count.
    }
    total = total * static_cast<unsigned long long>(codec::kCoupletSpace) +
            static_cast<unsigned long long>(digit);
  }
  if (total == kMax) {
    return std::nullopt;  // +1 (inclusive count) would overflow.
  }
  return static_cast<std::size_t>(total + 1);
}

namespace {

PlanResult Invalid(std::string reason) {
  PlanResult r;
  r.status = PlanStatus::kInvalid;
  r.reason = std::move(reason);
  return r;
}

}  // namespace

PlanResult plan(const AddressSpan &span, std::size_t n) {
  if (span.empty()) {
    if (n == 0) {
      PlanResult r;
      r.status = PlanStatus::kValid;
      return r;
    }
    return Invalid("empty ADDRESS span does not cover N=" + std::to_string(n));
  }

  std::vector<PlannedSlot> slots;
  bool pending_seam = false;

  for (std::size_t i = 0; i < span.size(); ++i) {
    const bool is_last = (i + 1 == span.size());
    const AddressSegment &seg = span[i];

    switch (seg.kind) {
      case AddressSegment::Kind::kDirect:
      case AddressSegment::Kind::kPin: {
        if (!seg.origin.has_value()) {
          return Invalid("segment " + std::to_string(i) +
                          " is direct/pin but carries no address value");
        }
        PlannedSlot slot;
        slot.address = seg.origin;
        slots.push_back(std::move(slot));
        break;
      }
      case AddressSegment::Kind::kUndeclared: {
        PlannedSlot slot;
        slot.is_undeclared_hook = true;
        slots.push_back(std::move(slot));
        break;
      }
      case AddressSegment::Kind::kNestedDeclare: {
        if (seg.nested == nullptr) {
          return Invalid("segment " + std::to_string(i) +
                          " is a nested declare but carries no statement");
        }
        PlannedSlot slot;
        slot.nested = seg.nested.get();
        slots.push_back(std::move(slot));
        break;
      }
      case AddressSegment::Kind::kFrom:
      case AddressSegment::Kind::kAfter: {
        const bool is_after = (seg.kind == AddressSegment::Kind::kAfter);
        if (!seg.origin.has_value()) {
          return Invalid("segment " + std::to_string(i) +
                          " is FROM/AFTER but carries no origin value");
        }

        if (seg.to_bound.has_value()) {
          // Closed run: fixed-count, self-delimiting, legal anywhere.
          if (is_after) {
            // The count is distance(resolved-start, to_bound), and
            // resolved-start ("the next block after b") needs the
            // deferred trunk map (G5). Cover-N cannot be verified past
            // this point -- report what was planned so far and stop.
            PlanResult r;
            r.status = PlanStatus::kPendingSeam;
            r.slots = std::move(slots);
            r.reason = "segment " + std::to_string(i) +
                       " is AFTER..TO: its slot count depends on the "
                       "deferred trunk map (G5) and cannot be resolved here";
            return r;
          }
          auto count = span_length(*seg.origin, *seg.to_bound);
          if (!count.has_value()) {
            return Invalid("segment " + std::to_string(i) +
                            " (FROM..TO) is not a forward, same-depth run");
          }
          codec::Address cur = *seg.origin;
          for (std::size_t k = 0; k < *count; ++k) {
            PlannedSlot slot;
            slot.address = cur;
            slots.push_back(std::move(slot));
            if (k + 1 < *count) {
              auto next = successor(cur);
              if (!next.has_value()) {
                return Invalid("segment " + std::to_string(i) +
                                " (FROM..TO) exceeds representable depth "
                                "before reaching its TO bound");
              }
              cur = *next;
            }
          }
        } else {
          // Open origin: elastic, legal only as the last segment.
          if (!is_last) {
            return Invalid("segment " + std::to_string(i) +
                            " is an open (un-TO'd) FROM/AFTER origin but is "
                            "not the last segment -- only the last segment "
                            "may be elastic");
          }
          if (slots.size() > n) {
            return Invalid("segments before the elastic tail already cover "
                            "more than N=" +
                            std::to_string(n) + " slots");
          }
          const std::size_t remaining = n - slots.size();
          if (is_after) {
            pending_seam = true;
            for (std::size_t k = 0; k < remaining; ++k) {
              slots.push_back(PlannedSlot{});
            }
          } else {
            codec::Address cur = *seg.origin;
            for (std::size_t k = 0; k < remaining; ++k) {
              PlannedSlot slot;
              slot.address = cur;
              slots.push_back(std::move(slot));
              if (k + 1 < remaining) {
                auto next = successor(cur);
                if (!next.has_value()) {
                  return Invalid("elastic FROM run exceeds representable "
                                  "depth before covering N=" +
                                  std::to_string(n));
                }
                cur = *next;
              }
            }
          }
        }
        break;
      }
    }
  }

  if (slots.size() != n) {
    return Invalid("ADDRESS span covers " + std::to_string(slots.size()) +
                    " slot(s), expected N=" + std::to_string(n));
  }

  PlanResult r;
  r.status = pending_seam ? PlanStatus::kPendingSeam : PlanStatus::kValid;
  r.slots = std::move(slots);
  return r;
}

namespace {
SpanShapeResult ShapeInvalid(std::string reason) {
  return SpanShapeResult{false, std::move(reason)};
}
}  // namespace

SpanShapeResult validate_span_shape(const AddressSpan &span) {
  for (std::size_t i = 0; i < span.size(); ++i) {
    const bool is_last = (i + 1 == span.size());
    const AddressSegment &seg = span[i];

    switch (seg.kind) {
      case AddressSegment::Kind::kDirect:
      case AddressSegment::Kind::kPin:
        if (!seg.origin.has_value() || !codec::is_valid_address(*seg.origin)) {
          return ShapeInvalid("segment " + std::to_string(i) +
                               " carries no valid address value");
        }
        break;
      case AddressSegment::Kind::kUndeclared:
        break;
      case AddressSegment::Kind::kNestedDeclare: {
        if (seg.nested == nullptr) {
          return ShapeInvalid("segment " + std::to_string(i) +
                               " is a nested declare but carries no statement");
        }
        auto nested_result = validate_declare(*seg.nested);
        if (!nested_result.ok()) {
          return ShapeInvalid("segment " + std::to_string(i) +
                               " (nested declare): " + nested_result.reason);
        }
        break;
      }
      case AddressSegment::Kind::kFrom:
      case AddressSegment::Kind::kAfter: {
        if (!seg.origin.has_value() || !codec::is_valid_address(*seg.origin)) {
          return ShapeInvalid("segment " + std::to_string(i) +
                               " is FROM/AFTER but carries no valid origin");
        }
        if (seg.to_bound.has_value()) {
          if (!codec::is_valid_address(*seg.to_bound)) {
            return ShapeInvalid("segment " + std::to_string(i) +
                                 " has an invalid TO bound");
          }
          if (seg.kind == AddressSegment::Kind::kFrom) {
            // A closed FROM..TO run's forward-walkability is a pure
            // address-arithmetic fact, checkable without N.
            auto count = span_length(*seg.origin, *seg.to_bound);
            if (!count.has_value()) {
              return ShapeInvalid("segment " + std::to_string(i) +
                                   " (FROM..TO) is not a forward, same-depth run");
            }
          }
          // kAfter..TO: whether it is a forward run depends on its
          // resolved start, which needs the trunk map (G5) -- not
          // checked here; the shape alone (an origin closed by TO) is
          // legal.
        } else if (!is_last) {
          return ShapeInvalid(
              "segment " + std::to_string(i) +
              " is an open (un-TO'd) FROM/AFTER origin but is not the "
              "last segment -- only the last segment may be elastic");
        }
        break;
      }
    }
  }
  return SpanShapeResult{true, ""};
}

}  // namespace command
