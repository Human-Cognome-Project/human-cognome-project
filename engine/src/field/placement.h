#pragma once

#include "field/field.h"

namespace field::detail {
class Placement {
 public:
  static void append_layout(const BudgetDemand &, std::vector<StorageExtent> &);
  ~Placement();
  PlacementResult query(std::uint64_t,
                        const UpdateCommand *,
                        std::size_t,
                        const PlacementIntrinsic *,
                        std::size_t,
                        std::uint64_t,
                        std::uint64_t);
  const PlacementInspection &inspect(const PlacementResult &);
  std::size_t binding_bytes() const;
  std::uint64_t native_bytes() const {
    return native_bytes_;
  }

 private:
  friend class field::Harness;
  friend struct field::testing::PlacementAccess;
  explicit Placement(Harness &);
  Harness &h_;
  std::unique_ptr<taichi::lang::Ndarray> rows_, stats_, focal_, keep_,
      requests_, occupancy_, cells_, attempt_, published_, counts_;
  engine::CompiledKernel collect_, capture_, solve_, publish_;
  std::vector<int> keep_values_;
  std::vector<double> request_values_, result_values_;
  std::vector<PlacementIntrinsic> intrinsics_, published_intrinsics_;
  std::vector<UpdateCommand> published_commands_;
  PlacementInspection inspection_;
  std::array<std::int64_t, 16> geometry_counts_{};
  std::array<double, 4> focal_values_{};
  std::array<std::uint64_t, 16> count_values_{};
  PlacementResult last_;
  std::uint64_t serial_{}, native_bytes_{};
  std::size_t published_count_{}, published_commands_count_{};
  void build();
  void collect();
  void capture();
  void solve(int, std::uint64_t, std::uint64_t, int);
  void publish(bool enabled);
};
}  // namespace field::detail
