#pragma once
#include <array>
#include <memory>
#include <vector>
#include "field/cascade.h"
#include "taichi/program/ndarray.h"
namespace field {
struct LiveHierarchyResult {
  CascadeStatus status{CascadeStatus::Unsupported};
  BudgetFailure failure{};
  std::uint64_t generation{}, nodes{}, responses{}, preparation_visits{},
      reference_work{}, upload_bytes{}, readback_bytes{}, synchronizations{};
};
struct LiveHierarchyInspection {
  // Borrowed until the next synchronized operation. Logical prefixes use the
  // successful installation's node count; precision is per comparison.
  std::vector<CascadeNode> nodes;
  std::vector<double> slots, previous;
  std::vector<int> selectors, fields;
  std::vector<std::int64_t> visits;
  std::array<std::int64_t, 10> counters{};
};
class Harness;
namespace testing {
struct LiveHierarchyAccess;
}
namespace detail {
struct StorageExtent;
class LiveHierarchy {
 public:
  static void append_layout(const BudgetDemand &, std::vector<StorageExtent> &);
  explicit LiveHierarchy(Harness &);
  ~LiveHierarchy();
  bool installed() const;
  std::uint64_t response_count() const;
  void validate() const;
  LiveHierarchyResult install(std::uint64_t,
                              const WeightedCascadeSpec &,
                              std::uint64_t);
  void force(double);
  void poll(double, double contact_stiffness);
  const LiveHierarchyInspection &inspect();
  std::size_t binding_bytes() const;
  const taichi::lang::Ndarray &leaf_fields() const;

 private:
  friend struct field::testing::LiveHierarchyAccess;
  std::array<const engine::CompiledKernel *, 3> kernels() const;
  void poll_target(double, int, double contact_stiffness = 0);
  std::size_t fixed_object_bytes() const;
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}  // namespace detail
}  // namespace field
