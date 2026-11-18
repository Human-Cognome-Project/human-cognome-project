#pragma once
#include <memory>
#include "field/cascade.h"
namespace taichi::lang {
class Ndarray;
}
namespace field {
class Harness;
namespace testing {
struct LiveHierarchyAccess;
}
namespace detail {
struct StorageExtent;
class CentroidCascade {
 public:
  static void append_layout(const BudgetDemand &, std::vector<StorageExtent> &);
  explicit CentroidCascade(Harness &);
  ~CentroidCascade();
  std::size_t binding_bytes() const;
  CascadeBinding bind(std::uint64_t, const CascadeSpec &, std::uint64_t);
  CascadeBinding bind_weighted(std::uint64_t,
                               const WeightedCascadeSpec &,
                               std::uint64_t,
                               bool same_level_live = false);
  CascadeResult query(std::uint64_t, std::uint64_t);
  const CascadeInspection &inspect(const CascadeResult &);

 private:
  friend class LiveHierarchy;
  friend struct field::testing::LiveHierarchyAccess;
  void validate_observation(const CascadeResult &);
  // Synchronous installation-only access; numerical host vectors are not read
  // back here. Public inspection keeps its full observation/lifetime contract.
  const CascadeInspection &inspect_descriptors(const CascadeResult &);
  void copy_published(const CascadeResult &, taichi::lang::Ndarray &);
  CascadeBinding bind_impl(std::uint64_t,
                           const CascadeSpec &,
                           const WeightedCascadeSpec *,
                           std::uint64_t,
                           bool same_level_live = false);
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}  // namespace detail
}  // namespace field
