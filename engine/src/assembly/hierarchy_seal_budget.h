#pragma once
#include <cstdint>
#include <cstddef>

namespace assembly::detail {
// Per-sealing payload allowance, not a whole-owner/process memory limit.
struct HierarchySealBudget { std::uint64_t bytes{}, copy_work{}; };
enum class HierarchySealStatus { Ok, ByteExhausted, WorkExhausted, Overflow };
struct HierarchySealResult {
  HierarchySealStatus status{HierarchySealStatus::Overflow};
  std::uint64_t input_bytes{}, retained_bytes{}, required_bytes{}, required_work{};
};
inline bool seal_add(std::uint64_t &value, std::uint64_t count, std::uint64_t size) {
  if (count > UINT64_MAX / size) return false;
  const auto n = std::uint64_t(count) * size;
  if (n > UINT64_MAX-value) return false;
  value += n;
  return true;
}
}
