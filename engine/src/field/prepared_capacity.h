#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace field {
enum class Budget {
  Potential,
  Particles,
  Fields,
  Participation,
  Contacts,
  References,
  Focus,
  Batch,
  ScratchBytes,
  Count
};
struct BudgetDemand {
  // Potential means required address extent, not population or coordinates.
  std::array<std::uint64_t, static_cast<std::size_t>(Budget::Count)> values{};
  std::uint64_t &operator[](Budget kind) {
    return values[static_cast<std::size_t>(kind)];
  }
  std::uint64_t operator[](Budget kind) const {
    return values[static_cast<std::size_t>(kind)];
  }
};
struct BudgetFailure {
  Budget budget{Budget::Count};
  std::uint64_t required{0}, capacity{0};
  bool exhausted() const {
    return budget != Budget::Count;
  }
};
const char *budget_name(Budget budget);
class CapacityError final : public std::invalid_argument {
 public:
  explicit CapacityError(BudgetFailure failure)
      : std::invalid_argument(budget_name(failure.budget)), failure(failure) {
  }
  const BudgetFailure failure;
};
struct DenseShape {
  const char *name;
  std::size_t rows, stride, element_bytes, bytes;
};

// Checked logical limits and current numerical layout arithmetic only. Does
// not allocate backing, bind kernels, or certify scratch for a future batch.
class PreparedCapacity {
 public:
  explicit PreparedCapacity(BudgetDemand limits);
  const BudgetDemand &limits() const {
    return limits_;
  }
  BudgetFailure check(const BudgetDemand &demand) const;
  // The existing Harness/Activity array families at fixed capacity strides.
  // Native masks/map metadata, host mirrors, prospective indexes, reference
  // buffers and batch scratch require separate actual backing measurements.
  std::array<DenseShape, 22> numerical_shapes() const;
  std::size_t numerical_bytes() const;

 private:
  BudgetDemand limits_;
};
}  // namespace field
