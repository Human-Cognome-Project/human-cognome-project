#pragma once

#include "engine/runtime.h"

namespace engine::numeric {
enum class Real { F32, F64 };
int fraction_bits(Real format);
std::size_t element_bytes(Real format);
taichi::lang::DataType native_type(Real format);

// Explicit numerical choices for native kernel construction, not a live
// Harness selector or a calibrated LoD/production rounding policy.
class Precision {
 public:
  Precision(Real calculation, Real storage, int retained_fraction_bits);
  Real calculation() const {
    return calculation_;
  }
  Real storage() const {
    return storage_;
  }
  int retained_fraction_bits() const {
    return retained_;
  }
  void require_backend(const Runtime &runtime) const;

 private:
  Real calculation_, storage_;
  int retained_;
};

struct QuantizedPosition {
  taichi::lang::Stmt *value, *valid;
};

// Experimental Q_b: convert an already combined result to the declared stored
// container, then clear its terminal fraction bits. Callers must promote inputs
// BEFORE the arithmetic selected by Precision::calculation(). This helper
// cannot recover precision lost in an earlier expression. Compile with
// MathMode::Strict. Invalid input/conversion gives valid=0 and a zero
// placeholder, never a valid saturated coordinate. There is no remainder and no
// mutation of physical state.
// The fork enables flush-to-zero. This bounded primitive conservatively rejects
// nonzero f64->f32 inputs below f32's smallest normal, rather than silently
// accepting flushed conversion results. Same-container bit trimming still
// preserves encoded subnormals. Full gradual-underflow arithmetic is not
// claimed.
QuantizedPosition truncate_position(taichi::lang::IRBuilder &builder,
                                    taichi::lang::Stmt *calculated,
                                    const Precision &precision);
}  // namespace engine::numeric
