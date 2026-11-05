#pragma once

#include "field/field.h"

namespace field::detail {
class AdmissionContacts;
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
  std::unique_ptr<AdmissionContacts> contacts_;
  std::unique_ptr<taichi::lang::Ndarray> rows_, stats_, focal_, keep_,
      requests_, occupancy_, cells_, attempt_, published_, counts_;
  engine::CompiledKernel collect_, capture_, solve_, publish_, initialize_;
  engine::CompiledKernel bootstrap_solve_;
  engine::CompiledKernel exchange_reduce_, exchange_target_, exchange_balance_kernel_;
  std::unique_ptr<taichi::lang::Ndarray> exchange_state_;
  std::unique_ptr<taichi::lang::Ndarray> exchange_balance_, exchange_masses_;
  void build_exchange();
  void build_exchange_balance();
  void reduce_exchange(bool reconcile = false);
  void balance_exchange(int bodies);
  void target_exchange();
  PlacementResult stage_exchange(std::uint64_t, const UpdateCommand *, std::size_t,
      const PlacementIntrinsic *, std::size_t, const engine::RecordHandle *,
      std::size_t, std::uint64_t, std::uint64_t);
  // Attempt-local D7 prerequisite, not an exchange or reusable candidate token.
  UpdateResult stage_exchange_state(std::uint64_t, const engine::RecordHandle *,
                                    std::size_t, std::uint64_t, bool reconcile = false);
  std::unique_ptr<taichi::lang::Ndarray> bootstrap_contributions_;
  std::vector<std::uint64_t> bootstrap_values_;
  std::size_t bootstrap_edges_{};
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
  void solve(int, std::uint64_t, std::uint64_t, int, bool bootstrap = false);
  void publish(bool enabled);
  void initialize(int bodies, bool exchange = false);
  // Private admission preparation: native generated rows only, no record claims
  // or public candidate publication. Full admission owns subsequent validation.
  PlacementResult stage(std::uint64_t, const UpdateCommand *, std::size_t,
                        const PlacementIntrinsic *, std::size_t,
                        std::uint64_t, std::uint64_t, bool bootstrap = false,
                        const UpdateRef *focus = nullptr, std::size_t focus_count = 0);
  PlacementResult attempt(std::uint64_t, const UpdateCommand *, std::size_t,
                          const PlacementIntrinsic *, std::size_t,
                          std::uint64_t, std::uint64_t, bool publication,
                          bool bootstrap = false, const UpdateRef *focus = nullptr,
                          std::size_t focus_count = 0,
                          const engine::RecordHandle *outgoing = nullptr,
                          std::size_t outgoing_count = 0);
};
}  // namespace field::detail
