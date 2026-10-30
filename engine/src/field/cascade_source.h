#pragma once
#include "field/cascade.h"
#include "field/prepared_storage.h"

namespace field::detail {
inline bool same_handle(engine::RecordHandle a, engine::RecordHandle b) {
  return a.context == b.context && a.incarnation == b.incarnation &&
         a.address == b.address && a.record == b.record;
}
// Shared committed/prospective weighted source authority. Typed pending
// handles are resolved by structural preflight, not by this numerical view.
struct CascadeSourceView {
  const std::vector<RelationshipHeader> &edges;
  const std::vector<std::uint64_t> &counts, &masses;
  std::size_t size;
  std::uint64_t generation;
  CascadeStatus contribution(engine::RecordHandle occurrence,
                             engine::RecordHandle field,
                             CascadeSourceRow source,
                             std::uint64_t &numerator,
                             std::uint64_t &denominator) const {
    if (source.generation != generation)
      return CascadeStatus::Stale;
    if (source.row >= size || occurrence.record < 0 ||
        std::size_t(occurrence.record) >= masses.size())
      return CascadeStatus::InvalidReference;
    const auto &edge = edges[source.row];
    if (!same_handle(edge.first, occurrence) ||
        !same_handle(edge.second, field) || source.ordinal != edge.ordinal)
      return CascadeStatus::InvalidReference;
    numerator = counts[source.row];
    denominator = masses[occurrence.record];
    if (!numerator || !denominator || numerator > denominator ||
        edge.share != float(double(numerator) / double(denominator)))
      return CascadeStatus::UnsupportedAccounting;
    return CascadeStatus::Ok;
  }
};
inline bool add_coverage(CascadeCoverage &target,
                         std::uint64_t numerator,
                         std::uint64_t denominator) {
  if (target.denominator != denominator || target.count > denominator ||
      numerator > denominator - target.count)
    return false;
  target.count += numerator;
  return true;
}
inline bool account_child(const CascadeCoverage &parent,
                          const CascadeCoverage &child,
                          std::uint64_t &sum) {
  if (parent.denominator != child.denominator || sum > parent.count ||
      child.count > parent.count - sum)
    return false;
  sum += child.count;
  return true;
}
}  // namespace field::detail
