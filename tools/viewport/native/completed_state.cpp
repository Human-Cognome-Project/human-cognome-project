#include "snapshot.h"

#include <algorithm>
#include <chrono>
#include <set>
#include <stdexcept>

#include "field/prepared_storage.h"

namespace viewport {
namespace {
void require(bool ok, const char *text) {
  if (!ok)
    throw std::invalid_argument(text);
}
bool same(const engine::RecordHandle &a, const engine::RecordHandle &b) {
  return a.context == b.context && a.incarnation == b.incarnation &&
         a.address == b.address && a.record == b.record;
}
Json record(const engine::RecordHandle &h) {
  return {{"context", std::to_string(h.context)},
          {"incarnation", std::to_string(h.incarnation)},
          {"address", h.address},
          {"record", h.record}};
}
const char *backend(taichi::Arch arch) {
  if (arch == taichi::Arch::x64)
    return "native_cpu";
  if (arch == taichi::Arch::cuda)
    return "native_cuda";
  throw std::invalid_argument("viewport: unsupported actual backend");
}
using Clock = std::chrono::steady_clock;
double elapsed(Clock::time_point start) {
  return std::chrono::duration<double, std::milli>(Clock::now() - start)
      .count();
}
}  // namespace
HarnessBinding bind(field::Harness &h, assembly::ActiveReport mapping) {
  require(h.ready() && h.prepared(),
          "viewport: ready prepared execution required");
  const auto &s = h.prepared_storage();
  require(
      mapping.prepared.capacity == h.particle_capacity() &&
          mapping.prepared.groups.size() == std::size_t(h.field_capacity()) &&
          mapping.prepared.particles.size() == s.live.particles,
      "viewport: incomplete native source mapping");
  require(!mapping.handle.session.nil() && !mapping.handle.study.nil() &&
              !mapping.handle.generation.nil() && !mapping.owner.nil() &&
              !mapping.instance.nil(),
          "viewport: incomplete execution identity");
  HarnessBinding out{
      std::move(mapping), s.generation, h.live_hierarchy_installed(), {}, {}};
  std::set<int> slots;
  for (const auto &p : out.mapping.prepared.particles) {
    const int slot = p.occurrence.slot;
    require(slot >= 0 && slot < h.particle_capacity() &&
                slots.insert(slot).second &&
                s.particle_headers[slot].record == slot &&
                p.exact_mass == s.source_masses[slot],
            "viewport: occurrence/native source mismatch");
    out.bodies.push_back(s.particle_headers[slot]);
  }
  for (const auto &f : s.field_headers)
    if (f.context && f.record >= 0)
      out.fields.push_back(f);
  require(out.mapping.prepared.focal_groups.size() == s.live.focus,
          "viewport: focal mapping size mismatch");
  for (std::size_t i = 0; i < s.live.focus; ++i)
    require(out.mapping.prepared.focal_groups[i] == s.focal[i].record,
            "viewport: focal mapping mismatch");
  return out;
}
CompletedState observe(field::Harness &h, const HarnessBinding &binding) {
  const auto start = Clock::now();
  require(h.ready() && h.prepared(), "viewport: execution not ready");
  const auto &s = h.prepared_storage();
  require(binding.generation == s.generation &&
              binding.installed == h.live_hierarchy_installed() &&
              binding.bodies.size() == s.live.particles &&
              binding.fields.size() == s.live.fields &&
              binding.mapping.prepared.particles.size() == s.live.particles,
          "viewport: stale native snapshot binding");
  for (const auto &body : binding.bodies)
    require(body.record >= 0 && body.record < h.particle_capacity() &&
                same(body, s.particle_headers[body.record]),
            "viewport: foreign/recycled body binding");
  for (const auto &field : binding.fields)
    require(field.record >= 0 && field.record < h.field_capacity() &&
                same(field, s.field_headers[field.record]),
            "viewport: foreign/recycled field binding");
  CompletedState out;
  out.backend = backend(h.actual_arch());
  out.prepared_generation = s.generation;
  out.tick = s.completed_ticks;
  const auto primary_before = h.timings().download_bytes;
  h.inspect_prepared();  // Synchronizes and reads this execution, including
                         // sleepers.
  out.report = binding.mapping;
  auto &r = out.report;
  r.numerical_profile = h.numerical_profile();
  r.frame_anchor = h.frame_anchor();
  r.demand = h.demand();
  r.particles.clear();
  r.groups.clear();
  r.determined.clear();
  r.particles64.clear();
  r.groups64.clear();
  r.determined64.clear();
  if (r.numerical_profile.wide()) {
    r.particles64 = h.particles64();
    r.groups64 = h.groups64();
    r.determined64 = h.determined64();
    out.numeric_copy_bytes =
        (r.particles64.size() + r.groups64.size() + r.determined64.size()) *
        sizeof(double);
  } else {
    r.particles = h.particle_data;
    r.groups = h.group_data;
    r.determined = h.determined();
    out.numeric_copy_bytes =
        (r.particles.size() + r.groups.size() + r.determined.size()) *
        sizeof(float);
  }
  out.body_records = binding.bodies;
  const auto primary_bytes = h.timings().download_bytes - primary_before;
  const auto P = std::size_t(std::max(h.particle_capacity(), 1));
  const auto G = std::size_t(std::max(h.field_capacity(), 1));
  out.requested_readback_bytes =
      primary_bytes + s.inspection_flags.size() * sizeof(int) +
      sizeof(s.inspection_counts) + 2 * (P + G) * sizeof(int) +
      (3 * P + 6 * G + 11 + (primary_bytes ? 1 : 0)) *
          r.numerical_profile.bytes();
  if (binding.installed) {
    const auto &v = h.inspect_live_hierarchy();
    out.requested_readback_bytes +=
        (v.slots.size() + v.previous.size()) * sizeof(double) +
        v.selectors.size() * sizeof(int) +
        (v.counters.size() + v.visits.size()) * sizeof(std::int64_t);
    require(v.node_count == s.live.fields && v.order.size() >= v.node_count,
            "viewport: incomplete installed comparison metadata");
    // Refresh preserves packet slots by identity, so live comparisons can have
    // holes. Logical order is separate from their stable numerical locations.
    out.comparisons.reserve(v.node_count);
    for (std::size_t logical = 0; logical < v.node_count; ++logical) {
      const int slot = v.order[logical];
      require(slot >= 0 && std::size_t(slot) < v.nodes.size(),
              "viewport: invalid live comparison slot");
      out.comparisons.push_back(v.nodes[slot]);
    }
    for (const auto &node : out.comparisons)
      require(node.field.record >= 0 &&
                  node.field.record < h.field_capacity() &&
                  same(node.field, s.field_headers[node.field.record]),
              "viewport: stale installed comparison identity");
  }
  require(s.generation == binding.generation && s.completed_ticks == out.tick,
          "viewport: concurrent native mutation during observation");
  out.observation_ms = elapsed(start);
  return out;
}
CompletedState observe(assembly::ActiveSession &session,
                       const warm::Uuid &owner,
                       const assembly::ExecutionHandle &handle) {
  const auto start = Clock::now();
  const auto before = session.timings(handle).download_bytes;
  CompletedState out;
  out.report = session.report(handle, owner);
  out.backend = session.on_cuda() ? "native_cuda" : "native_cpu";
  out.tick = session.completed_ticks(handle);
  out.requested_readback_bytes =
      session.timings(handle).download_bytes - before;
  const auto &r = out.report;
  out.numeric_copy_bytes =
      (r.particles.size() + r.groups.size() + r.determined.size()) *
          sizeof(float) +
      (r.particles64.size() + r.groups64.size() + r.determined64.size()) *
          sizeof(double);
  out.observation_ms = elapsed(start);
  return out;
}
Json snapshot(const CompletedState &state, const CaptureContext &context) {
  require(state.tick == context.tick,
          "viewport: capture tick is not the completed native tick");
  require(state.backend == "native_cpu" || state.backend == "native_cuda",
          "viewport: unsupported actual capture backend");
  auto value = snapshot(state.report, context);
  Json nodes = Json::array(), bodies = Json::array();
  for (const auto &node : state.comparisons) {
    require(node.calculation == engine::numeric::Real::F64 &&
                node.storage == engine::numeric::Real::F64 &&
                node.retained_fraction_bits >= 0 &&
                node.retained_fraction_bits <= 52,
            "viewport: unsupported comparison precision");
    nodes.push_back({{"comparison", std::to_string(node.comparison)},
                     {"field", record(node.field)},
                     {"level", node.level},
                     {"calculation_bits", 64},
                     {"storage_bits", 64},
                     {"fraction_bits", node.retained_fraction_bits}});
  }
  for (const auto &h : state.body_records)
    bodies.push_back(record(h));
  value["execution"] = {
      {"backend", state.backend},
      {"mode", state.comparisons.empty() ? "flat" : "installed_hierarchy"},
      {"completed_tick", std::to_string(state.tick)},
      {"prepared_generation",
       state.prepared_generation
           ? Json(std::to_string(*state.prepared_generation))
           : Json()},
      {"body_records", bodies},
      {"comparisons", nodes}};
  return value;
}
}  // namespace viewport
