#pragma once

#include <array>
#include "field/native_set.h"

namespace field::detail {
namespace testing {
struct NativeSelectionAccess;
}

// Exclusively owned transient selection for one candidate attempt. Numerical
// values and presence remain separate. The caller completes/validates all
// candidate work before publish. Input sets belong to this Runtime and have the
// declared logical bounds; their backing/list high-water is already prepared.
// This class does not make a numerical tick
// transactional by itself. Neither stage nor abandon writes committed bits.
class NativeSelectionWorkspace {
 public:
  NativeSelectionWorkspace(engine::Runtime &runtime,
                           NativeSet particles,
                           int particle_capacity,
                           NativeSet centroids,
                           int centroid_capacity);
  NativeSelectionWorkspace(const NativeSelectionWorkspace &) = delete;
  NativeSelectionWorkspace &operator=(const NativeSelectionWorkspace &) =
      delete;
  const NativeSet &particles() const {
    return candidate_[0];
  }
  const NativeSet &centroids() const {
    return candidate_[1];
  }

  // Snapshot current native membership into scratch, discarding earlier
  // candidate membership. Native type phases can then modify scratch normally.
  void stage();
  void abandon();
  // Publication is valid only at the owner's successful exclusive boundary.
  // An exception during this operation cannot be treated as healthy rejection.
  void publish();
  std::size_t binding_bytes() const;

 private:
  friend struct testing::NativeSelectionAccess;
  std::array<NativeSet, 2> committed_, candidate_;
  engine::CompiledKernel stage_, abandon_, publish_;
  static void launch(engine::CompiledKernel &, int enabled = 1);
};
}  // namespace field::detail
