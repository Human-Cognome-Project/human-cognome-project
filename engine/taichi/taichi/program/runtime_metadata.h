#pragma once
#include <cstddef>
#include <limits>

namespace taichi::lang {
struct RuntimeMapStorage {
  bool available{false}, overflow{false};
  const void *allocation_domain{nullptr};
  std::size_t entries{0}, allocation_request_bytes{0};
  std::size_t counter_object_bytes{0}, counter_control_block_bytes{0};
  // Subset of allocation_request_bytes, never an extra charge.
  std::size_t value_payload_bytes{0};
};

// Retained host bookkeeping only, not a live backing registry or full runtime B.
// Identities are synchronous: the runtime and all domains must remain quiescent
// and alive. Counts can include retired allocation descriptors.
struct RuntimeBookkeeping {
  bool available{false}, complete{false};
  const void *executor_identity{nullptr}, *root_manager_identity{nullptr};
  std::size_t executor_base_object_bytes{0}, root_manager_object_bytes{0};
  RuntimeMapStorage root_records, tree_allocations, runtime_allocations;
};

template <typename Map>
RuntimeMapStorage runtime_map_storage(const Map &map) {
  RuntimeMapStorage result;
  result.entries = map.size();
  if (map.size() > std::numeric_limits<std::size_t>::max() / sizeof(typename Map::value_type)) {
    result.overflow = true;
    return result;
  }
  const auto *domain = map.get_allocator().domain();
  result.available = true;
  result.allocation_domain = domain;
  result.allocation_request_bytes = domain->live_bytes;
  result.counter_object_bytes = sizeof(*domain);
  result.counter_control_block_bytes = domain->control_block_bytes;
  result.value_payload_bytes = map.size() * sizeof(typename Map::value_type);
  return result;
}
}  // namespace taichi::lang
