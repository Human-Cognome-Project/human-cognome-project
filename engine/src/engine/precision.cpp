#include "engine/precision.h"

#include <stdexcept>

#include "taichi/program/extension.h"

namespace engine::numeric {
using namespace taichi::lang;
int fraction_bits(Real format) {
  switch (format) {
    case Real::F32:
      return 23;
    case Real::F64:
      return 52;
  }
  throw std::invalid_argument("unsupported real format");
}
std::size_t element_bytes(Real format) {
  return fraction_bits(format) == 23 ? 4 : 8;
}
DataType native_type(Real format) {
  return fraction_bits(format) == 23 ? PrimitiveType::f32 : PrimitiveType::f64;
}
Precision::Precision(Real calculation, Real storage, int retained)
    : calculation_(calculation), storage_(storage), retained_(retained) {
  fraction_bits(calculation);
  if (retained < 0 || retained > fraction_bits(storage))
    throw std::invalid_argument("retained fraction bits outside container");
}
void Precision::require_backend(const Runtime &runtime) const {
  if (runtime.settings().cuda && !runtime.on_cuda())
    throw std::runtime_error(
        "requested CUDA numerical profile fell back to CPU");
  if ((calculation_ == Real::F64 || storage_ == Real::F64) &&
      !is_extension_supported(runtime.arch(), Extension::data64))
    throw std::runtime_error("actual backend does not advertise data64");
}
std::uint64_t Precision::fraction_mask() const {
  const int drop = fraction_bits(storage_) - retained_;
  return ~((std::uint64_t{1} << drop) - 1);
}
namespace {
Stmt *word(IRBuilder &b, Real type, std::uint64_t value) {
  return type == Real::F32
             ? static_cast<Stmt *>(b.get_uint32(std::uint32_t(value)))
             : static_cast<Stmt *>(b.get_uint64(value));
}
Stmt *bits(IRBuilder &b, Real type, Stmt *value) {
  return b.create_bit_cast(
      value, type == Real::F32 ? PrimitiveType::u32 : PrimitiveType::u64);
}
Stmt *finite(IRBuilder &b, Real type, Stmt *value) {
  auto *exponent =
      word(b, type, type == Real::F32 ? 0x7f800000ULL : 0x7ff0000000000000ULL);
  return b.create_cmp_ne(b.create_and(bits(b, type, value), exponent),
                         exponent);
}
Stmt *zero(IRBuilder &b, Real type) {
  return type == Real::F32 ? static_cast<Stmt *>(b.get_float32(0))
                           : static_cast<Stmt *>(b.get_float64(0));
}
}  // namespace
QuantizedPosition truncate_position(IRBuilder &b,
                                    Stmt *calculated,
                                    const Precision &p) {
  // Explicitly type the boundary; promotion inside the caller's arithmetic is
  // still required. Bit classification avoids floating comparisons/NaN
  // ordering.
  auto *value = b.create_cast(calculated, native_type(p.calculation()));
  auto *input_valid = finite(b, p.calculation(), value);
  if (p.calculation() == Real::F64 && p.storage() == Real::F32) {
    // Program enables FTZ, including on CPU. A native cast can therefore flush
    // an otherwise representable f32 subnormal. Do not change that global mode
    // or invent software floating conversion in this prerequisite slice.
    auto *magnitude = b.create_and(bits(b, Real::F64, value),
                                   b.get_uint64(0x7fffffffffffffffULL));
    auto *supported = b.create_logical_or(
        b.create_cmp_eq(magnitude, b.get_uint64(0)),
        b.create_cmp_ge(magnitude, b.get_uint64(0x3810000000000000ULL)));
    input_valid = b.create_logical_and(input_valid, supported);
  }
  auto *converted = b.create_cast(
      b.create_select(input_valid, value, zero(b, p.calculation())),
      native_type(p.storage()));
  auto *valid =
      b.create_logical_and(input_valid, finite(b, p.storage(), converted));
  auto *safe = b.create_select(valid, converted, zero(b, p.storage()));
  auto *cleared = b.create_and(bits(b, p.storage(), safe),
                               word(b, p.storage(), p.fraction_mask()));
  return {b.create_bit_cast(cleared, native_type(p.storage())), valid};
}
}  // namespace engine::numeric
