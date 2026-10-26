#include "field/prepared_storage.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

#include "engine/transfer.h"
#include "field/centroid_cascade.h"

namespace field::detail {
namespace {
std::size_t multiply(std::size_t a, std::size_t b) {
  if (a && b > std::numeric_limits<std::size_t>::max() / a)
    throw std::overflow_error("prepared workspace product overflow");
  return a * b;
}
void add(std::size_t &sum, std::size_t value) {
  if (value > std::numeric_limits<std::size_t>::max() - sum)
    throw std::overflow_error("prepared workspace sum overflow");
  sum += value;
}
std::size_t physical(std::size_t n) {
  return std::max(n, std::size_t{1});
}
PreparedCapacity checked_storage(engine::Runtime &runtime,
                                 const PreparedCapacity &capacity) {
  if (capacity.limits()[Budget::Batch] > (std::uint64_t{1} << 20))
    throw std::invalid_argument(
        "prepared native batch exceeds supported command axis");
  const auto required = PreparedStorage::required_scratch(
      capacity.limits(), runtime.on_cuda(), capacity.profile());
  if (required > capacity.limits()[Budget::ScratchBytes])
    throw CapacityError({Budget::ScratchBytes, required,
                         capacity.limits()[Budget::ScratchBytes]});
  return capacity;
}
}  // namespace

std::vector<StorageExtent> PreparedStorage::workspace_layout(
    const BudgetDemand &l, NumericalProfile profile) {
  const PreparedCapacity validate(l, profile);
  const auto p = l[Budget::Particles], g = l[Budget::Fields],
             e = l[Budget::Participation], c = l[Budget::Contacts],
             f = l[Budget::Focus], b = l[Budget::Batch];
  std::vector<StorageExtent> result;
  const auto extent = [&](const char *name, std::size_t n, std::size_t bytes,
                          std::size_t copies) {
    result.push_back({name, physical(n), bytes, copies,
                      multiply(multiply(physical(n), bytes), copies)});
  };
  // Headers and commands are host metadata; only actual typed kernel inputs
  // have backend copies. There is no duplicate opaque reserve.
  extent("particle-headers", p, sizeof(engine::RecordHandle), 2);
  extent("source-masses", p, sizeof(std::uint64_t), 2);
  extent("field-headers", g, sizeof(engine::RecordHandle), 2);
  extent("participation-headers", e, sizeof(RelationshipHeader), 2);
  extent("source-counts", e, sizeof(std::uint64_t), 2);
  extent("contact-headers", c, sizeof(RelationshipHeader), 2);
  extent("focal-references", f, sizeof(engine::RecordHandle), 1);
  extent("batch-commands", b, sizeof(UpdateCommand), 1);
  extent("batch-results", b, sizeof(engine::RecordResult), 1);
  extent("retired-references", b, sizeof(engine::RecordHandle), 1);
  extent("resolved-references", b, sizeof(engine::RecordHandle), 1);
  // Sentinels belong to each of the eight separate logical CSR arrays.
  extent("prospective-csr",
         6 * physical(p) + 2 * physical(g) + 2 * physical(e) + 2 * physical(c),
         sizeof(int), 1);
  extent("csr-counts", std::max(p, g), sizeof(int), 1);
  extent("csr-cursors", std::max(p, g), sizeof(int), 1);
  extent("validation", 2 * p + g + e + c, sizeof(int), 2);
  extent("derived-results", 14 * physical(g) + 12, profile.bytes(), 1);
  extent("candidate-edges", 2 * physical(e), sizeof(int), 2);
  extent("candidate-shares", e, sizeof(float), 2);
  extent("candidate-group-offsets", 2 * physical(g), sizeof(int), 1);
  extent("candidate-group-order", e, sizeof(int), 1);
  extent("destination-records", p + g, sizeof(int), 2);
  extent("native-claim-inputs", 3 * physical(b), sizeof(int), 1);
  extent("new-fixture-rows", 12 * physical(b), profile.bytes(), 2);
  extent("candidate-status", 1, sizeof(int), 2);
  extent("inspection-motion", 3 * physical(p) + 6 * physical(g),
         profile.bytes(), 1);
  extent("inspection-state", 2 * (physical(p) + physical(g)), sizeof(int), 1);
  extent("inspection-focus", 11, profile.bytes(), 1);
  extent("inspection-flags", p + g, sizeof(int), 2);
  extent("inspection-counts", 4, sizeof(std::int64_t), 2);
  extent("activity-reset-zeros",
         (profile.wide() ? 2 : 1) * std::max(3 * physical(p), 6 * physical(g)),
         sizeof(int), 1);
  extent("selected-focus-host", f, sizeof(int),
         2);  // slots and sorted selection, no backend blob copy
  extent("map-batch-buffers", 3 + 10 * physical(b), sizeof(std::int64_t),
         4);  // two maps, host+backend
  extent("map-single-command-buffers", 10, sizeof(std::int64_t),
         2);  // two maps, backend only
  if (l[Budget::GeometryWork]) {
    extent("geometry-occupied-rows", p, sizeof(int), 2);
    extent("geometry-temporary-pairs", 4 * physical(c), sizeof(int), 1);
    extent("geometry-pairs", 2 * physical(c), sizeof(int), 2);
    extent("geometry-pair-flags", c, sizeof(int), 2);
    extent("geometry-source-order", c, sizeof(int), 1);
    extent("geometry-source-group-end", c, sizeof(int), 1);
    extent("geometry-source-map", c, sizeof(int), 2);
    extent("geometry-permutation", c, sizeof(int), 1);
    extent("geometry-offsets", 4 * physical(p), sizeof(int), 2);
    extent("geometry-cursors", 2 * physical(p), sizeof(int), 1);
    extent("geometry-orders", 2 * physical(c), sizeof(int), 2);
    extent("geometry-counters", 16, sizeof(std::int64_t), 2);
    extent("geometry-focus", 4, profile.bytes(), 2);
  }
  if (profile.wide())
    extent("live-source-ratios-and-cohort-flags", 2 * physical(e) + physical(g),
           sizeof(std::uint64_t), 4);
  CentroidCascade::append_layout(l, result);
  return result;
}
std::size_t PreparedStorage::workspace_bytes(const BudgetDemand &l,
                                             NumericalProfile profile) {
  std::size_t result = 0;
  for (const auto &entry : workspace_layout(l, profile))
    add(result, entry.bytes);
  return result;
}
std::size_t PreparedStorage::binding_reserve_bytes(bool cuda, bool geometry,
                                                   bool cascade, bool weighted,
                                                   bool wide) {
  // Seven Harness signatures: 8+12+11+12+3+12+5 = 63 parameters.
  // Six Activity signatures: 14+10+7+13+1+0 = 45. Two maps, each with
  // mutate/find/batch signatures: 12. Inspection: 2. Total 122, 20 contexts.
  // Candidate: 14; commit: 15. The two additional families bring the total to
  // 151 parameters/22 contexts. Flat rank <=2, no gradients: each argument is
  // at most 24 ABI bytes, plus native scalar-cache and source-array metadata
  // slots for every argument.
  // Geometry adds collect/source-order/discover/finalize: 10+4+9+10 arguments.
  // Cascade: 9+13 parameters, plus one descriptor argument per kernel when
  // weighted.
  // Trial adds six arguments, including final-speed output in integration.
  const std::size_t parameters = 151 + (geometry ? 33 : 0) +
                                 (cascade ? 22 : 0) +
                                 (cascade && weighted ? 2 : 0) + (wide ? 6 : 0),
                    contexts = 22 + (geometry ? 4 : 0) + (cascade ? 2 : 0),
                    abi_per_arg = 24;
  using Scalar = taichi::lang::LaunchContextBuilder::Prepared::Scalar;
  using Array = taichi::lang::LaunchContextBuilder::ArrayBinding;
  return parameters * (abi_per_arg + sizeof(Scalar) + sizeof(Array)) +
         (cuda ? parameters * abi_per_arg +
                     contexts * (1 + sizeof(std::uint64_t) + sizeof(void *))
               : 0);
}
std::size_t PreparedStorage::required_scratch(const BudgetDemand &l, bool cuda,
                                              NumericalProfile profile) {
  auto result = workspace_bytes(l, profile);
  for (const auto &entry : workspace_layout(l, profile))
    if (entry.count > (std::size_t{1} << 30))
      throw std::invalid_argument(
          "prepared typed workspace exceeds supported axis");
  add(result,
      binding_reserve_bytes(cuda, l[Budget::GeometryWork] != 0,
                            l[Budget::CascadeNodes] != 0,
                            l[Budget::CascadeCoverage] != 0, profile.wide()));
  return result;
}
PreparedStorage::PreparedStorage(engine::Runtime &r, const PreparedCapacity &c)
    : capacity(checked_storage(r, c)),
      particles(r,
                int(c.limits()[Budget::Potential]),
                int(c.limits()[Budget::Particles]),
                int(c.limits()[Budget::Batch])),
      fields(r,
             int(physical(c.limits()[Budget::Fields])),
             int(c.limits()[Budget::Fields]),
             int(c.limits()[Budget::Batch])) {
  const auto &l = capacity.limits();
  const auto profile = capacity.profile();
  const auto p = physical(l[Budget::Particles]),
             g = physical(l[Budget::Fields]),
             e = physical(l[Budget::Participation]),
             k = physical(l[Budget::Contacts]), f = physical(l[Budget::Focus]),
             b = physical(l[Budget::Batch]);
  if (required_scratch(l, r.on_cuda(), profile) > l[Budget::ScratchBytes])
    throw CapacityError({Budget::ScratchBytes,
                         required_scratch(l, r.on_cuda(), profile),
                         l[Budget::ScratchBytes]});
  manifest = workspace_layout(l, profile);
  particle_headers.resize(p);
  prospective_particles.resize(p);
  source_masses.resize(p);
  prospective_source_masses.resize(p);
  field_headers.resize(g);
  prospective_fields.resize(g);
  edges.resize(e);
  prospective_edges.resize(e);
  source_counts.resize(e);
  prospective_source_counts.resize(e);
  contacts.resize(k);
  prospective_contacts.resize(k);
  focal.resize(f);
  commands.resize(b);
  results.resize(b);
  retired.resize(b);
  resolved.resize(b);
  prospective_csr.resize(6 * p + 2 * g + 2 * e + 2 * k);
  counts.resize(std::max(p, g));
  cursors.resize(std::max(p, g));
  validation.resize(physical(2 * l[Budget::Particles] + l[Budget::Fields] +
                             l[Budget::Participation] + l[Budget::Contacts]));
  inspection_flags.resize(physical(l[Budget::Particles] + l[Budget::Fields]));
  destinations.resize(physical(l[Budget::Particles] + l[Budget::Fields]));
  claim_addresses.resize(b);
  claim_kinds.resize(b);
  command_indexes.resize(b);
  candidate_edges.resize(2 * e);
  candidate_shares.resize(e);
  new_row_values.configure(profile.wide());
  new_row_values.resize(12 * b);
  inspection_state.resize(2 * (p + g));
  inspection_motion_values.configure(profile.wide());
  inspection_motion_values.resize(3 * p + 6 * g);
  using namespace taichi::lang;
  const auto array = [&](DataType type, int rows, int n) {
    return std::make_unique<Ndarray>(
        &r.program(), type,
        rows == 1 ? std::vector<int>{n} : std::vector<int>{rows, n});
  };
  validation_array = array(PrimitiveType::i32, 1, int(validation.size()));
  derived_array =
      array(profile.wide() ? PrimitiveType::f64 : PrimitiveType::f32, 1,
            int(14 * g + 12));
  destinations_array = array(PrimitiveType::i32, 1, int(destinations.size()));
  candidate_edges_array = array(PrimitiveType::i32, 2, int(e));
  candidate_shares_array = array(PrimitiveType::f32, 1, int(e));
  new_rows_array = array(
      profile.wide() ? PrimitiveType::f64 : PrimitiveType::f32, 12, int(b));
  candidate_status_array = array(PrimitiveType::i32, 1, 1);
  candidate_offsets_array = array(PrimitiveType::i32, 2, int(g));
  candidate_order_array = array(PrimitiveType::i32, 1, int(e));
  if (l[Budget::GeometryWork]) {
    geometry_rows = array(PrimitiveType::i32, 1, int(p));
    geometry_temporary = array(PrimitiveType::i32, 4, int(k));
    geometry_pairs = array(PrimitiveType::i32, 2, int(k));
    geometry_flags = array(PrimitiveType::i32, 1, int(k));
    geometry_source_order = array(PrimitiveType::i32, 1, int(k));
    geometry_group_end = array(PrimitiveType::i32, 1, int(k));
    geometry_source_map = array(PrimitiveType::i32, 1, int(k));
    geometry_permutation = array(PrimitiveType::i32, 1, int(k));
    geometry_offsets = array(PrimitiveType::i32, 4, int(p));
    geometry_cursors = array(PrimitiveType::i32, 2, int(p));
    geometry_orders = array(PrimitiveType::i32, 2, int(k));
    geometry_counters = array(PrimitiveType::i64, 1, 16);
    geometry_focus =
        array(profile.wide() ? PrimitiveType::f64 : PrimitiveType::f32, 1, 4);
    for (auto *a :
         {geometry_rows.get(), geometry_temporary.get(), geometry_pairs.get(),
          geometry_flags.get(), geometry_source_order.get(),
          geometry_group_end.get(), geometry_source_map.get(),
          geometry_permutation.get(), geometry_offsets.get(),
          geometry_cursors.get(), geometry_orders.get(),
          geometry_counters.get(), geometry_focus.get()})
      r.program().fill_ndarray_fast_u32(a, 0);
    geometry_inspection.occupied_rows.resize(p);
    geometry_inspection.pairs.resize(2 * k);
    geometry_inspection.pair_flags.resize(k);
    geometry_inspection.source_to_pair.resize(k);
    geometry_inspection.offsets.resize(4 * p);
    geometry_inspection.orders.resize(2 * k);
  }
  for (auto *a :
       {validation_array.get(), derived_array.get(), destinations_array.get(),
        candidate_edges_array.get(), candidate_shares_array.get(),
        new_rows_array.get(), candidate_status_array.get(),
        candidate_offsets_array.get(), candidate_order_array.get()})
    r.program().fill_ndarray_fast_u32(a, 0);
  if (profile.wide()) {
    live_ratios.resize(2 * e + g);
    candidate_ratios.resize(2 * e + g);
    live_ratios_array = array(PrimitiveType::u64, 1, int(2 * e + g));
    candidate_ratios_array = array(PrimitiveType::u64, 1, int(2 * e + g));
    r.program().fill_ndarray_fast_u32(live_ratios_array.get(), 0);
    r.program().fill_ndarray_fast_u32(candidate_ratios_array.get(), 0);
  }
  inspection_flags_array =
      std::make_unique<Ndarray>(&r.program(), PrimitiveType::i32,
                                std::vector<int>{int(inspection_flags.size())});
  inspection_counts_array = std::make_unique<Ndarray>(
      &r.program(), PrimitiveType::i64, std::vector<int>{4});
  engine::upload(*inspection_flags_array, inspection_flags.data(),
                 inspection_flags.size() * sizeof(int));
  engine::upload(*inspection_counts_array, inspection_counts.data(),
                 sizeof(inspection_counts));
}
BudgetFailure PreparedStorage::check(const PreparedCounts &n,
                                     std::size_t batch) const {
  BudgetDemand demand;
  demand[Budget::Particles] = n.particles;
  demand[Budget::Fields] = n.fields;
  demand[Budget::Participation] = n.edges;
  demand[Budget::Contacts] = n.contacts;
  demand[Budget::Focus] = n.focus;
  demand[Budget::References] = n.references();
  demand[Budget::Batch] = batch;
  return capacity.check(demand);
}
bool PreparedStorage::prepare_ratios(bool prospective, std::size_t count,
                                     std::uint64_t &visits) {
  const auto &rows = prospective ? prospective_edges : edges;
  const auto &counts = prospective ? prospective_source_counts : source_counts;
  const auto &masses = prospective ? prospective_source_masses : source_masses;
  auto &ratios = prospective ? candidate_ratios : live_ratios;
  if (!capacity.profile().wide()) return true;
  const auto stride =
      std::max<std::uint64_t>(capacity.limits()[Budget::Participation], 1);
  std::fill(ratios.begin(), ratios.end(), 0);
  // Reuse candidate edge scratch before CSR packing; sorting validates only
  // integer coverage and never changes numerical source order.
  for (std::size_t row = 0; row < count; ++row) candidate_edges[row] = int(row);
  std::sort(candidate_edges.begin(), candidate_edges.begin() + count,
            [&](int a, int b) {
              ++visits;
              const auto &x = rows[a], &y = rows[b];
              if (x.second.record != y.second.record)
                return x.second.record < y.second.record;
              if (x.first.record != y.first.record)
                return x.first.record < y.first.record;
              return a < b;
            });
  int previous_body = -1, previous_field = -1;
  std::uint64_t sum = 0;
  for (std::size_t j = 0; j < count; ++j) {
    ++visits;
    const auto row = std::size_t(candidate_edges[j]);
    const auto &edge = rows[row];
    if (edge.first.record < 0 ||
        std::size_t(edge.first.record) >= masses.size())
      return false;
    const auto d = masses[edge.first.record], c = counts[row];
    if (!c || !d || c > d || edge.share != float(double(c) / double(d)))
      return false;
    if (edge.second.record == previous_field &&
        edge.first.record != previous_body)
      ratios[2 * stride + edge.second.record] = 1;
    if (edge.first.record != previous_body ||
        edge.second.record != previous_field)
      sum = 0;
    if (c > d - sum) return false;
    sum += c;
    previous_body = edge.first.record;
    previous_field = edge.second.record;
    ratios[row] = c;
    ratios[stride + row] = d;
  }
  return true;
}
}  // namespace field::detail
