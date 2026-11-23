#pragma once

#include <limits>
#include <stdexcept>

namespace taichi::lang::cpu::detail {

// The caller serializes the existing session counter. Issuance precedes all
// fallible creation work; an issued name is never reused after failure.
inline int issue_jit_module_id(int &next, int *issued_id = nullptr) {
  if (next == std::numeric_limits<int>::max())
    throw std::overflow_error("CPU JIT module IDs exhausted");
  const auto id = next++;
  if (issued_id) *issued_id = id;
  return id;
}

}  // namespace taichi::lang::cpu::detail
