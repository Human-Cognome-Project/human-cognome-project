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
// The same request/domain fields also describe allocator-aware sequences.
using RuntimeContainerStorage = RuntimeMapStorage;

struct RuntimeThreadStorage {
  bool available{false}, present{false}, overflow{false}, complete{false};
  const void *owner{nullptr};
  std::size_t object_bytes{0}, thread_handles{0}, handle_capacity_bytes{0};
};

// Retained host bookkeeping only, not a live backing registry or full runtime B.
// Identities are synchronous: the runtime and all domains must remain quiescent
// and alive. Counts can include retired allocation descriptors.
struct RuntimeBookkeeping {
  bool available{false}, complete{false};
  const void *executor_identity{nullptr}, *root_manager_identity{nullptr};
  std::size_t executor_base_object_bytes{0}, root_manager_object_bytes{0};
  RuntimeMapStorage root_records, tree_allocations, runtime_allocations;
  // Existing native pool object/handle vector only, not OS stacks or worker state.
  RuntimeThreadStorage thread_pool;
};

template <typename Container>
RuntimeContainerStorage runtime_container_storage(const Container &map) {
  RuntimeContainerStorage result;
  result.entries = map.size();
  if (map.size() > std::numeric_limits<std::size_t>::max() / sizeof(typename Container::value_type)) {
    result.overflow = true;
    return result;
  }
  const auto *domain = map.get_allocator().domain();
  result.available = true;
  result.allocation_domain = domain;
  result.allocation_request_bytes = domain->live_bytes;
  result.counter_object_bytes = sizeof(*domain);
  result.counter_control_block_bytes = domain->control_block_bytes;
  result.value_payload_bytes = map.size() * sizeof(typename Container::value_type);
  return result;
}
template <typename Map>
RuntimeMapStorage runtime_map_storage(const Map &map) {
  return runtime_container_storage(map);
}
}  // namespace taichi::lang
