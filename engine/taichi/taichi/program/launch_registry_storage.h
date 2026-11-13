#pragma once
#include <cstddef>

namespace taichi::lang {
enum class LaunchMetadataStatus { Unavailable, Absent, Present, Invalid, InsufficientWork, Overflow };
struct LaunchRegistryStorage {
  LaunchMetadataStatus status{LaunchMetadataStatus::Unavailable};
  bool complete{false};
  const void *owner{nullptr};
  std::size_t object_bytes{0}, contexts{0}, context_capacity_bytes{0};
};
struct LaunchContextStorage {
  LaunchMetadataStatus status{LaunchMetadataStatus::Unavailable};
  bool complete{false};
  const void *owner{nullptr};
  std::size_t required_parameters{0}, visited_parameters{0};
  std::size_t task_capacity_bytes{0}, parameter_capacity_bytes{0};
  std::size_t path_capacity_bytes{0}, element_shape_capacity_bytes{0};
  // Conservative capacity+terminator charge, potentially inline in a parameter.
  // NOT a disjoint heap request to add to the containing parameter object.
  std::size_t name_capacity_charge_bytes{0};
};
}  // namespace taichi::lang
