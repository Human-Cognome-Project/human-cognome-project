#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>

namespace field {
enum class NumericalKind { LegacyF32, TrialF64 };
struct NumericalProfile {
  NumericalKind kind{NumericalKind::LegacyF32};
  int body_fraction_bits{52}, field_fraction_bits{52};
  static NumericalProfile trial(int body_bits = 52, int field_bits = 52) {
    NumericalProfile p{NumericalKind::TrialF64, body_bits, field_bits};
    p.validate();
    return p;
  }
  bool wide() const { return kind == NumericalKind::TrialF64; }
  std::size_t bytes() const { return wide() ? sizeof(double) : sizeof(float); }
  static bool supported_real(double x) {
    std::uint64_t bits;
    std::memcpy(&bits, &x, sizeof(bits));
    bits &= 0x7fffffffffffffffULL;
    const auto exponent = bits & 0x7ff0000000000000ULL;
    return bits == 0 || (exponent && exponent != 0x7ff0000000000000ULL);
  }
  void validate() const {
    if ((kind != NumericalKind::LegacyF32 && kind != NumericalKind::TrialF64) ||
        body_fraction_bits < 0 || body_fraction_bits > 52 ||
        field_fraction_bits < 0 || field_fraction_bits > 52 ||
        (!wide() && (body_fraction_bits != 52 || field_fraction_bits != 52)))
      throw std::invalid_argument("unsupported numerical profile");
  }
};
}  // namespace field
