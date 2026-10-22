#include "field/prepared_storage.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

#include "engine/transfer.h"

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
  const auto required =
      PreparedStorage::required_scratch(capacity.limits(), runtime.on_cuda());
  if (required > capacity.limits()[Budget::ScratchBytes])
    throw CapacityError({Budget::ScratchBytes, required,
                         capacity.limits()[Budget::ScratchBytes]});
  return capacity;
}
}  // namespace

std::vector<StorageExtent> PreparedStorage::workspace_layout(
    const BudgetDemand &l) {
  const PreparedCapacity validate(l);
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
  extent("field-headers", g, sizeof(engine::RecordHandle), 2);
  extent("participation-headers", e, sizeof(RelationshipHeader), 2);
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
  extent("derived-results", 14 * physical(g) + 12, sizeof(float), 1);
  extent("candidate-edges", 2 * physical(e), sizeof(int), 2);
  extent("candidate-shares", e, sizeof(float), 2);
  extent("candidate-group-offsets", 2 * physical(g), sizeof(int), 1);
  extent("candidate-group-order", e, sizeof(int), 1);
  extent("destination-records", p + g, sizeof(int), 2);
  extent("native-claim-inputs", 3 * physical(b), sizeof(int), 1);
  extent("new-fixture-rows", 12 * physical(b), sizeof(float), 2);
  extent("candidate-status", 1, sizeof(int), 2);
  extent("inspection-motion", 3 * physical(p) + 6 * physical(g), sizeof(float),
         1);
  extent("inspection-state", 2 * (physical(p) + physical(g)), sizeof(int), 1);
  extent("inspection-focus", 11, sizeof(float), 1);
  extent("inspection-flags", p + g, sizeof(int), 2);
  extent("inspection-counts", 4, sizeof(std::int64_t), 2);
  extent("activity-reset-zeros", std::max(3 * physical(p), 6 * physical(g)),
         sizeof(int), 1);
  extent("selected-focus-host", f, sizeof(int),
         2);  // slots and sorted selection, no backend blob copy
  extent("map-batch-buffers", 3 + 10 * physical(b), sizeof(std::int64_t),
         4);  // two maps, host+backend
  extent("map-single-command-buffers", 10, sizeof(std::int64_t),
         2);  // two maps, backend only
  return result;
}
std::size_t PreparedStorage::workspace_bytes(const BudgetDemand &l) {
  std::size_t result = 0;
  for (const auto &entry : workspace_layout(l))
    add(result, entry.bytes);
  return result;
}
std::size_t PreparedStorage::binding_reserve_bytes(bool cuda) {
  // Seven Harness signatures: 8+12+11+12+3+12+5 = 63 parameters.
  // Six Activity signatures: 14+10+7+13+1+0 = 45. Two maps, each with
  // mutate/find/batch signatures: 12. Inspection: 2. Total 122, 20 contexts.
  // Candidate: 14; commit: 15. The two additional families bring the total to
  // 151 parameters/22 contexts. Flat rank <=2, no gradients: each argument is
  // at most 24 ABI bytes, plus native scalar-cache and source-array metadata
  // slots for every argument.
  constexpr std::size_t parameters = 151, contexts = 22, abi_per_arg = 24;
  using Scalar = taichi::lang::LaunchContextBuilder::Prepared::Scalar;
  using Array = taichi::lang::LaunchContextBuilder::ArrayBinding;
  return parameters * (abi_per_arg + sizeof(Scalar) + sizeof(Array)) +
         (cuda ? parameters * abi_per_arg +
                     contexts * (1 + sizeof(std::uint64_t) + sizeof(void *))
               : 0);
}
std::size_t PreparedStorage::required_scratch(const BudgetDemand &l,
                                              bool cuda) {
  auto result = workspace_bytes(l);
  for (const auto &entry : workspace_layout(l))
    if (entry.count > (std::size_t{1} << 30))
      throw std::invalid_argument(
          "prepared typed workspace exceeds supported axis");
  add(result, binding_reserve_bytes(cuda));
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
  const auto p = physical(l[Budget::Particles]),
             g = physical(l[Budget::Fields]),
             e = physical(l[Budget::Participation]),
             k = physical(l[Budget::Contacts]), f = physical(l[Budget::Focus]),
             b = physical(l[Budget::Batch]);
  if (required_scratch(l, r.on_cuda()) > l[Budget::ScratchBytes])
    throw CapacityError({Budget::ScratchBytes, required_scratch(l, r.on_cuda()),
                         l[Budget::ScratchBytes]});
  manifest = workspace_layout(l);
  particle_headers.resize(p);
  prospective_particles.resize(p);
  field_headers.resize(g);
  prospective_fields.resize(g);
  edges.resize(e);
  prospective_edges.resize(e);
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
  new_rows.resize(12 * b);
  inspection_state.resize(2 * (p + g));
  inspection_motion.resize(3 * p + 6 * g);
  using namespace taichi::lang;
  const auto array = [&](DataType type, int rows, int n) {
    return std::make_unique<Ndarray>(
        &r.program(), type,
        rows == 1 ? std::vector<int>{n} : std::vector<int>{rows, n});
  };
  validation_array = array(PrimitiveType::i32, 1, int(validation.size()));
  derived_array = array(PrimitiveType::f32, 1, int(14 * g + 12));
  destinations_array = array(PrimitiveType::i32, 1, int(destinations.size()));
  candidate_edges_array = array(PrimitiveType::i32, 2, int(e));
  candidate_shares_array = array(PrimitiveType::f32, 1, int(e));
  new_rows_array = array(PrimitiveType::f32, 12, int(b));
  candidate_status_array = array(PrimitiveType::i32, 1, 1);
  candidate_offsets_array = array(PrimitiveType::i32, 2, int(g));
  candidate_order_array = array(PrimitiveType::i32, 1, int(e));
  for (auto *a :
       {validation_array.get(), derived_array.get(), destinations_array.get(),
        candidate_edges_array.get(), candidate_shares_array.get(),
        new_rows_array.get(), candidate_status_array.get(),
        candidate_offsets_array.get(), candidate_order_array.get()})
    r.program().fill_ndarray_fast_u32(a, 0);
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
}  // namespace field::detail
