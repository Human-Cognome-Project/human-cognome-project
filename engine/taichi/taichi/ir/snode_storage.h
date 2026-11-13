#pragma once

#include <cstddef>

namespace taichi::lang {

// Fixed-work snapshots of live, quiescent host objects, not recursive totals.
// Object bytes include embedded controls; vector capacity is separate storage.
struct SNodeStorage {
  bool available{false};
  bool overflow{false};
  bool complete{false};
  bool string_storage_available{false};
  bool gradient_storage_available{false};
  const void *owner{nullptr};
  std::size_t object_bytes{0};
  std::size_t child_slots{0};
  std::size_t child_capacity_bytes{0};
  std::size_t index_offset_entries{0};
  std::size_t index_offset_capacity_bytes{0};
};

struct SNodeTreeStorage {
  bool available{false};
  bool complete{false};
  const void *owner{nullptr};
  const void *root_identity{nullptr};
  std::size_t object_bytes{0};
};

}  // namespace taichi::lang
