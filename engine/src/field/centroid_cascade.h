#pragma once
#include <memory>
#include "field/cascade.h"
namespace field {
class Harness;
namespace detail {
struct StorageExtent;
class CentroidCascade {
 public:
  static void append_layout(const BudgetDemand &, std::vector<StorageExtent> &);
  explicit CentroidCascade(Harness &);
  ~CentroidCascade();
  std::size_t binding_bytes() const;
  CascadeBinding bind(std::uint64_t, const CascadeSpec &, std::uint64_t);
  CascadeResult query(std::uint64_t, std::uint64_t);
  const CascadeInspection &inspect(const CascadeResult &);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}  // namespace detail
}  // namespace field
