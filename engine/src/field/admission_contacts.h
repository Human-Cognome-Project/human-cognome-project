#pragma once

#include "field/field.h"

namespace field::detail {
struct PreparedCounts;
// Attempt-local geometry only. No claims, activity or public observation token.
class AdmissionContacts {
 public:
  static void append_layout(const BudgetDemand &, std::vector<StorageExtent> &);
  explicit AdmissionContacts(Harness &);
  GeometryResult validate(const PreparedCounts &, std::uint64_t work);
  std::size_t binding_bytes() const;
  std::uint64_t upload_bytes() const { return upload_bytes_; }
  void publish(UpdateResult &);

 private:
  friend struct field::testing::PlacementAccess;
  enum Array { Bodies, Source, Rows, Temporary, Pairs, Flags, SourceOrder,
               GroupEnd, SourceMap, Permutation, Offsets, Cursors, Orders,
               Stats, OriginalMap, ArrayCount };
  Harness &h_;
  std::array<std::unique_ptr<taichi::lang::Ndarray>, ArrayCount> arrays_;
  std::array<engine::CompiledKernel, 5> kernels_;
  engine::CompiledKernel remap_, publish_;
  std::vector<int> source_;
  std::array<std::int64_t, 16> counts_{};
  std::uint64_t upload_bytes_{};
  void launch(int phase, int sources);
  void remap(bool);
  void copy(bool);
};
}  // namespace field::detail
