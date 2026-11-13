#pragma once

#include <cstddef>
#include <limits>
#include "taichi/program/runtime_metadata.h"

namespace taichi::lang {

struct ProgramVectorStorage {
  bool available{false}, overflow{false};
  std::size_t slots{0}, capacity_bytes{0};
};

// Fixed metadata only. A slot is not proof of a live pointee or device backing.
// Requires a live, quiescent Program; identity is not a lifetime lease.
struct ProgramStorage {
  bool available{false}, complete{false};
  const void *owner{nullptr};
  std::size_t object_bytes{0};
  ProgramVectorStorage kernels, functions, trees, textures;
  RuntimeMapStorage function_registry, ndarray_registry, argpack_registry;
  RuntimeContainerStorage free_ids;
  // All Program maps: field-expression and accessor maps remain uncovered.
  bool map_storage_available{false};
  bool free_id_storage_available{false};
  bool pointee_storage_available{false};
};

template <typename Vector>
ProgramVectorStorage program_vector_storage(const Vector &vector) {
  ProgramVectorStorage result;
  result.slots = vector.size();
  if (vector.capacity() > std::numeric_limits<std::size_t>::max() /
                              sizeof(typename Vector::value_type)) {
    result.overflow = true;
    return result;
  }
  result.available = true;
  result.capacity_bytes = vector.capacity() * sizeof(typename Vector::value_type);
  return result;
}

}  // namespace taichi::lang
