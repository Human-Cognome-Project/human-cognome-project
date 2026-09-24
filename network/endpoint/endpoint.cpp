#include "endpoint.h"

#include <stdexcept>

namespace endpoint {

Registry::Registry(uint32_t standing_capacity, uint32_t ephemeral_capacity)
    : standing_capacity_(standing_capacity),
      ephemeral_capacity_(ephemeral_capacity),
      ephemeral_boxes_(ephemeral_capacity),
      ephemeral_generation_(ephemeral_capacity, 0) {
  freelist_.reserve(ephemeral_capacity);
  for (uint32_t i = 0; i < ephemeral_capacity; ++i) {
    freelist_.push_back(standing_capacity_ + i);
  }
}

bool Registry::is_standing_slot(uint32_t slot) const {
  return slot < standing_capacity_;
}

bool Registry::is_ephemeral_slot(uint32_t slot) const {
  return slot >= standing_capacity_ && slot < standing_capacity_ + ephemeral_capacity_;
}

EndpointId Registry::register_standing(uint32_t slot, box::Box *box) {
  if (!is_standing_slot(slot)) {
    throw std::invalid_argument(
        "endpoint::Registry::register_standing: slot outside the standing range");
  }
  standing_[slot] = box;
  return EndpointId{slot, kStandingGeneration};
}

EndpointId Registry::allocate() {
  if (freelist_.empty()) {
    throw std::runtime_error("endpoint::Registry::allocate: ephemeral slot pool exhausted");
  }
  uint32_t slot = freelist_.back();
  freelist_.pop_back();
  uint32_t idx = slot - standing_capacity_;
  return EndpointId{slot, ephemeral_generation_[idx]};
}

box::Box *Registry::resolve(EndpointId id) {
  if (is_standing_slot(id.slot)) {
    auto it = standing_.find(id.slot);
    return it == standing_.end() ? nullptr : it->second;
  }
  if (is_ephemeral_slot(id.slot)) {
    uint32_t idx = id.slot - standing_capacity_;
    // Contract here is "in-range + generation matches," NOT "currently
    // allocated": a freed slot sitting in the freelist at its current
    // generation would also pass this check. That is unreachable in
    // practice -- every EndpointId in circulation originates from
    // allocate(), which only ever hands out a slot's CURRENT generation,
    // so no caller can be holding a matching id for a slot that is idle
    // in the freelist. But resolve() itself does not enforce
    // "currently allocated," so a future caller must not lean on that
    // stronger guarantee -- only on "not stale," which this does check.
    if (id.generation != ephemeral_generation_[idx]) {
      return nullptr;  // stale generation -- slot recycled since
    }
    return &ephemeral_boxes_[idx];
  }
  return nullptr;  // out of both ranges
}

void Registry::recycle(EndpointId id) {
  if (!is_ephemeral_slot(id.slot)) {
    throw std::invalid_argument("endpoint::Registry::recycle: slot is not an ephemeral slot");
  }
  uint32_t idx = id.slot - standing_capacity_;
  if (id.generation != ephemeral_generation_[idx]) {
    throw std::invalid_argument("endpoint::Registry::recycle: stale generation");
  }
  ++ephemeral_generation_[idx];
  freelist_.push_back(id.slot);
}

}  // namespace endpoint
