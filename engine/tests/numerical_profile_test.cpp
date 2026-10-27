#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>

#include "allocation_observation.h"
#include "assembly/study.h"
#include "prepared_fixture.h"
#include "prepared_metrics.h"

namespace {
using namespace field;
using namespace field::testing;
using engine::RecordHandle;
void require(bool ok, const char *message) {
  if (!ok) throw std::runtime_error(message);
}
template <class F>
void no_new(F f) {
  allocation_observation::calls = 0;
  allocation_observation::enabled = true;
  try {
    f();
  } catch (...) {
    allocation_observation::enabled = false;
    throw;
  }
  allocation_observation::enabled = false;
  require(!allocation_observation::calls,
          "numerical profile boundary allocated C++ metadata");
}
template <class T>
bool bytes(const std::vector<T> &a, const std::vector<T> &b) {
  return a.size() == b.size() &&
         (!a.size() || !std::memcmp(a.data(), b.data(), a.size() * sizeof(T)));
}
struct Snapshot {
  std::vector<double> particles, groups, speed, motion;
  std::vector<float> shares;
  double universal;
  std::vector<int> state, flags, contacts, edges;
  std::vector<RecordHandle> particle_headers, field_headers, focal;
  std::vector<detail::RelationshipHeader> participation, contact_headers;
  std::vector<std::uint64_t> masses, counts;
  FrameAnchor anchor;
  std::array<double, 11> focus;
  std::array<unsigned char, sizeof(Demand)> demand;
  detail::PreparedCounts live;
  std::uint64_t generation, ticks;
  explicit Snapshot(Harness &h)
      : particles(h.particles64()),
        groups(h.groups64()),
        speed(h.determined64()),
        motion(h.prepared_storage().motion64()),
        shares(h.edge_float_data),
        universal(h.universal64()),
        state(h.prepared_storage().inspection_state),
        flags(h.prepared_storage().inspection_flags),
        contacts(h.contact_int_data),
        edges(h.edge_int_data),
        particle_headers(h.prepared_storage().particle_headers),
        field_headers(h.prepared_storage().field_headers),
        focal(h.prepared_storage().focal),
        participation(h.prepared_storage().edges),
        contact_headers(h.prepared_storage().contacts),
        masses(h.prepared_storage().source_masses),
        counts(h.prepared_storage().source_counts),
        anchor(h.frame_anchor()),
        focus(h.prepared_storage().inspection_focus64),
        live(h.prepared_storage().live),
        generation(h.prepared_storage().generation),
        ticks(h.prepared_storage().completed_ticks) {
    std::memcpy(demand.data(), &h.demand(), demand.size());
  }
  void physical(Harness &h) const {
    h.inspect_prepared();
    PreparedFixtureAccess::read_relationships(h);
    const auto &s = h.prepared_storage();
    require(bytes(particles, h.particles64()) && bytes(groups, h.groups64()) &&
                bytes(speed, h.determined64()) && bytes(motion, s.motion64()) &&
                bytes(shares, h.edge_float_data) &&
                universal == h.universal64() && state == s.inspection_state &&
                flags == s.inspection_flags && contacts == h.contact_int_data &&
                edges == h.edge_int_data && anchor == h.frame_anchor() &&
                !std::memcmp(focus.data(), s.inspection_focus64.data(),
                             sizeof(focus)) &&
                !std::memcmp(demand.data(), &h.demand(), demand.size()) &&
                ticks == s.completed_ticks && h.ready(),
            "metadata changed physical/control/activity/history/Demand state");
  }
  void unchanged(Harness &h) const {
    physical(h);
    const auto &s = h.prepared_storage();
    require(bytes(particle_headers, s.particle_headers) &&
                bytes(field_headers, s.field_headers) &&
                bytes(focal, s.focal) && bytes(participation, s.edges) &&
                bytes(contact_headers, s.contacts) &&
                masses == s.source_masses && counts == s.source_counts &&
                generation == s.generation &&
                live.particles == s.live.particles &&
                live.fields == s.live.fields && live.edges == s.live.edges &&
                live.contacts == s.live.contacts && live.focus == s.live.focus,
            "ordinary rejection/no-op changed committed source metadata");
  }
};
template <class F>
void rejects(F f) {
  try {
    f();
  } catch (const std::logic_error &) {
    return;
  }
  throw std::runtime_error("invalid typed/provenance view accepted");
}
Parameters64 inert() {
  Parameters64 p;
  p.gravity = 0;
  p.contact_stiffness = 0;
  p.reach_strength = 0;
  return p;
}
Parameters legacy_inert() {
  Parameters p;
  p.gravity = 0;
  p.contact_stiffness = 0;
  p.reach_strength = 0;
  return p;
}
BudgetDemand limits(bool large,
                    NumericalProfile profile = NumericalProfile::trial(48,
                                                                       48)) {
  const std::uint64_t p = large ? 4096 : 70, g = large ? 257 : 9;
  BudgetDemand l;
  l.values = {
      large ? 16777216u : 1048576u, p, g, 4 * p, 2 * p, 16 * p + g + 6, g,
      large ? 256u : 32u,           0};
  l[Budget::GeometryWork] = 1ull << 40;
  l[Budget::CascadeNodes] = 3;
  l[Budget::CascadeLinks] = 2;
  l[Budget::CascadeReferences] = 4 * p;
  l[Budget::CascadeCoverage] = 3 * p;
  l[Budget::CascadeLevels] = 2;
  l[Budget::CascadeWork] = 1ull << 40;
  l[Budget::ScratchBytes] =
      detail::PreparedStorage::required_scratch(l, false, profile);
  return l;
}
FixtureSource fixture(int n, int groups) {
  FixtureSource src;
  src.kinds.resize(groups, 1);
  src.exact_masses.resize(n, 3);
  for (int g = 0; g < groups; ++g) src.kinds[g] = 1 + g % 3;
  for (int i = 0; i < n; ++i) {
    std::array<float, kParticleFieldCount> p{};
    p[kPosX] = float(i * 4 + 1);
    p[kMass] = 1 + i % 7;
    p[kRadius] = .25f;
    src.particles.push_back(p);
  }
  if (n == 8) {
    const float x[] = {1, -1, 1.8f, 1.9f, -1.8f, -1.9f, 64, 1};
    for (int i = 0; i < n; ++i) src.particles[i][kPosX] = x[i];
  }
  src.particles[0][kVelX] = std::ldexp(1.f, -30);
  for (int g = 0; g < 3; ++g)
    for (int i = 0; i < n; ++i) {
      const auto add = [&](std::uint64_t c) {
        src.edges.push_back({i, g, float(double(c) / 3), src.edges.size(), c});
      };
      if (g == 0) {
        add(1);
        add(1);
      } else
        add(g == 1 ? 1 : 3);
    }
  src.focus = {2};
  return src;
}
void refresh(Harness &h) {
  h.inspect_prepared();
  PreparedFixtureAccess::read_relationships(h);
}
double quantize(double x, int bits) {
  if (!x) return x;
  int e;
  std::frexp(std::fabs(static_cast<long double>(x)), &e);
  return std::copysign(
      double(
          std::ldexp(std::trunc(std::ldexp(
                         std::fabs(static_cast<long double>(x)), bits - e + 1)),
                     e - 1 - bits)),
      x);
}
void fields_oracle(Harness &h) {
  const auto &s = h.prepared_storage();
  const int ps = h.particle_capacity(), gs = h.field_capacity();
  for (int g = 0; g < gs; ++g) {
    long double mass = 0, sum = 0, max = 0;
    int count = 0;
    for (std::size_t e = 0; e < s.live.edges; ++e)
      if (s.edges[e].second.record == g) {
        const int p = s.edges[e].first.record;
        const auto x = h.particles64()[kPosX * ps + p];
        const long double m =
            static_cast<long double>(h.particles64()[kMass * ps + p]) *
            s.source_counts[e] / s.source_masses[p];
        mass += m;
        sum += m * x;
        max = std::max(max, std::fabs(static_cast<long double>(x)));
        ++count;
      }
    if (!count) {
      require(h.group64(kCenM, g) == 0, "empty field gained mass");
      continue;
    }
    const double m = h.group64(kCenM, g), x = h.group64(kCenX, g);
    const long double eps = std::numeric_limits<double>::epsilon(),
                      tolerance = 16 * (count + 1) * eps * std::max(1.L, max);
    require(std::fabs(m - mass) <= 8 * (count + 1) * eps * mass,
            "f64 participating mass differs from independent exact-count "
            "reference");
    int exponent;
    std::frexp(std::fabs(sum / mass), &exponent);
    const auto quantum = std::ldexp(
        1.L, exponent - 1 - h.numerical_profile().field_fraction_bits);
    require(std::fabs(x - sum / mass) <= tolerance + quantum,
            "f64 field centre outside reduction/publication bound");
    require(x == quantize(x, h.numerical_profile().field_fraction_bits),
            "field publication retained discarded bits");
    for (int a = 0; a < 3; ++a)
      require(s.motion64()[3 * ps + a * gs + g] == 0,
              "positional remainder survived publication");
  }
}
struct Tree {
  std::vector<CascadeNode> nodes;
  std::vector<int> children{0, 1};
  std::vector<CascadeContribution> rows;
  Tree(const Harness &h, const PreparedFixtureAccess::Input &in) {
    const auto &s = h.prepared_storage();
    for (int g = 0; g < 3; ++g) {
      CascadeNode node;
      node.field = in.field_rows[g];
      node.comparison = g + 1;
      node.level = g == 2 ? 1 : 0;
      node.retained_fraction_bits = h.numerical_profile().field_fraction_bits;
      node.cohort_begin = rows.size();
      for (std::size_t e = 0; e < s.live.edges; ++e)
        if (s.edges[e].second.record == node.field.record)
          rows.push_back(
              {s.edges[e].first, {s.generation, e, s.edges[e].ordinal}});
      node.cohort_count = rows.size() - node.cohort_begin;
      if (g == 2) {
        node.child_count = 2;
        node.child_begin = 0;
      }
      nodes.push_back(node);
    }
  }
  void refresh(const Harness &h) {
    const auto &s = h.prepared_storage();
    for (auto &r : rows) {
      r.source.generation = s.generation;
      r.source.ordinal = s.edges[r.source.row].ordinal;
    }
  }
  WeightedCascadeSpec spec() const {
    return {nodes.data(),    nodes.size(), children.data(),
            children.size(), rows.data(),  rows.size()};
  }
};
void run(engine::Runtime &r, HostPath path, bool large, bool maximum) {
  const auto profile = NumericalProfile::trial(48, 48);
  const auto l = limits(large, profile);
  auto start = std::chrono::steady_clock::now();
  Harness h(r, PreparedCapacity(l, profile), path);
  const auto us = [&](auto t) {
    return std::chrono::duration<double, std::micro>(
               std::chrono::steady_clock::now() - t)
        .count();
  };
  const double setup_us = us(start);
  const auto &s = h.prepared_storage();
  prepared_test::Metrics metrics(r, PreparedFixtureAccess::nodes(h));
  const auto prepared = metrics.sample();
  const auto src =
      fixture(maximum ? int(l[Budget::Particles]) : 8, int(l[Budget::Fields]));
  PreparedFixtureAccess::Input in(l[Budget::Particles], l[Budget::Fields],
                                  l[Budget::Batch], l[Budget::Focus]);
  PreparedFixtureAccess::load(h, src, in, legacy_inert(), true);
  refresh(h);
  fields_oracle(h);
  require(h.particle_data.empty() && h.group_data.empty() &&
              h.universal_data.empty(),
          "trial owns a shadow f32 payload");
  Tree tree(h, in);
  CascadeBinding binding;
  no_new([&] {
    binding = h.bind_weighted_centroid_cascade(s.generation, tree.spec());
  });
  require(binding.status == CascadeStatus::Ok,
          "f64 source hierarchy bind failed");
  start = std::chrono::steady_clock::now();
  const auto geometry = h.query_prepared_geometry(s.generation);
  const double geometry_us = us(start);
  require(geometry.status == GeometryStatus::Ok &&
              geometry.numerical_profile.wide(),
          "f64 geometry did not use the live profile");
  const auto ps = h.particle_capacity(), gs = h.field_capacity(),
             moving = in.particle_rows[0].record;
  const double initial = h.particle64(kPosX, moving);
  double tick_us = 0, update_us = 0, query_us = 0, inspect_us = 0;
  std::uint64_t upload_bytes = 0, readback_bytes = 0;
  for (int cycle = 0; cycle < 32; ++cycle) {
    const auto old_groups = h.groups64();
    const auto old_position = h.particle64(kPosX, moving);
    start = std::chrono::steady_clock::now();
    no_new([&] { h.tick64(inert()); });
    tick_us += us(start);
    start = std::chrono::steady_clock::now();
    no_new([&] { refresh(h); });
    inspect_us += us(start);
    require(h.demand().motion_attempts == 1 && h.demand().particles_moved == 1,
            "f64 motion/publication counts wrong");
    require(h.particle64(kPosX, moving) == old_position + std::ldexp(1., -30),
            "sub-f32 stored motion was narrowed");
    require(s.motion64()[moving] == h.particle64(kPosX, moving) - old_position,
            "accepted displacement lost or became trial residue");
    for (int g = 0; g < 3; ++g)
      require(s.motion64()[3 * ps + 3 * gs + in.field_rows[g].record] ==
                  old_groups[kCenX * gs + in.field_rows[g].record],
              "previous published history overwritten by an arithmetic anchor");
    fields_oracle(h);
    const Snapshot before(h);
    CascadeResult q;
    if (path == HostPath::BulkCopy) h.particle64(kPosX, moving) = -12345;
    start = std::chrono::steady_clock::now();
    no_new([&] {
      q = h.query_centroid_cascade(s.generation);
      h.inspect_centroid_cascade(q);
    });
    query_us += us(start);
    require(q.status == CascadeStatus::Ok &&
                q.required_work == 7 * src.particles.size() + 8,
            "f64 current candidate read failed");
    before.unchanged(h);
    UpdateCommand edit;
    edit.operation = UpdateOp::ReplaceParticipation;
    edit.row = 0;
    edit.generation = s.generation;
    edit.ordinal = 100 + cycle;
    edit.first = UpdateRef::existing(s.edges[0].first);
    edit.second = UpdateRef::existing(s.edges[0].second);
    edit.share = s.edges[0].share;
    edit.exact_count = s.source_counts[0];
    UpdateResult result;
    start = std::chrono::steady_clock::now();
    no_new([&] { result = h.update_prepared(s.generation, &edit, 1); });
    update_us += us(start);
    upload_bytes += result.upload_bytes;
    readback_bytes += result.readback_bytes;
    require(result.status == UpdateStatus::Ok, "f64 repeated commit failed");
    refresh(h);
    require(bytes(before.particles, h.particles64()) &&
                bytes(before.groups, h.groups64()) &&
                bytes(before.motion, s.motion64()) &&
                bytes(before.speed, h.determined64()) &&
                before.universal == h.universal64(),
            "metadata commit reset survivor state/history");
    rejects([&] { h.inspect_centroid_cascade(q); });
    require(
        h.query_centroid_cascade(s.generation).status == CascadeStatus::Stale,
        "f64 source edit did not stale candidate");
    tree.refresh(h);
    no_new([&] {
      binding = h.bind_weighted_centroid_cascade(s.generation, tree.spec());
    });
    require(binding.status == CascadeStatus::Ok, "f64 rebind failed");
  }
  require(h.particle64(kPosX, moving) == initial + std::ldexp(32., -30) &&
              float(h.particle64(kPosX, moving)) == float(initial),
          "f64 detail did not survive 32 ticks/repairs/readbacks");
  require(prepared.same_backing(metrics.sample()),
          "f64 setup/first use/reuse acquired backing/layout/compiler state");
  prepared.print("after_setup");
  metrics.sample().print("after_profile_cycles");
  for (const auto &e : s.manifest)
    std::cout << "manifest," << e.name << ',' << e.count << ','
              << e.element_bytes << ',' << e.copies << ',' << e.bytes << '\n';
  for (const auto &e : s.capacity.numerical_shapes())
    std::cout << "shape," << e.name << ',' << e.rows << ',' << e.stride << ','
              << e.element_bytes << ',' << e.bytes << '\n';
  std::cout << "numerical_profile," << ps << ','
            << (path == HostPath::BulkCopy ? "bulk" : "direct") << ','
            << (maximum ? "maximum" : "holes/motion")
            << ",setup_us=" << setup_us << ",geometry_us=" << geometry_us
            << ",mean_tick_us=" << tick_us / 32
            << ",mean_commit_us=" << update_us / 32
            << ",mean_query_inspect_us=" << query_us / 32
            << ",mean_physical_inspect_us=" << inspect_us / 32
            << ",S=" << l[Budget::ScratchBytes]
            << ",bindings=" << s.binding_bytes
            << ",mean_commit_upload_bytes=" << upload_bytes / 32
            << ",mean_commit_readback_bytes=" << readback_bytes / 32
            << ",source_ratio_bytes="
            << s.live_ratios.size() * sizeof(std::uint64_t)
            << ",cpp_new=0 PASS\n";
}
FixtureSource pair_source(bool moving = true) {
  FixtureSource s;
  s.kinds = {1};
  s.exact_masses = {1, 1};
  for (int i = 0; i < 2; ++i) {
    std::array<float, kParticleFieldCount> p{};
    p[kPosX] = i ? 3.f : 1.5f;
    p[kMass] = 1;
    p[kRadius] = 1;
    if (!i && moving) p[kVelX] = 1.f / 128;
    s.particles.push_back(p);
  }
  s.edges = {{0, 0, 1, 0, 1}, {1, 0, 1, 1, 1}};
  s.focus = {0};
  return s;
}
void watch_point(engine::Runtime &r, HostPath path, int bits) {
  const auto profile = NumericalProfile::trial(bits, bits);
  const auto l = limits(false, profile);
  Harness h(r, PreparedCapacity(l, profile), path);
  PreparedFixtureAccess::Input in(70, 9, 32, 9);
  auto src = pair_source();
  if (!bits) src.particles[0][kVelX] = .5f;
  PreparedFixtureAccess::load(h, src, in, legacy_inert(), true);
  refresh(h);
  const int a = in.particle_rows[0].record, b = in.particle_rows[1].record;
  const auto position = h.particle64(kPosX, a),
             velocity = h.particle64(kVelX, a);
  std::uint64_t attempts = 0, changes = 0, work = 0, edges = 0;
  auto start = std::chrono::steady_clock::now();
  for (int i = 0; i < 32; ++i) {
    no_new([&] {
      h.tick64(inert());
      refresh(h);
    });
    attempts += h.demand().motion_attempts;
    changes += h.demand().particles_moved;
    work += h.demand().particles_evaluated;
    edges += h.demand().centroid_reduction_edges;
    require(
        h.particle64(kPosX, a) == position &&
            h.particle64(kVelX, a) == velocity &&
            h.prepared_storage().inspection_counts[1] == 0 &&
            h.determined64()[a] == std::fabs(velocity),
        "quiet sub-grid publication lost velocity/speed, retained activity "
        "or recovered a tail");
    require(h.prepared_storage().motion64()[a] == 0,
            "unpublished trial became accepted displacement");
  }
  require(attempts == 1 && !changes && work == 2 && !edges,
          "quiet sub-grid work was not consumed after one evaluation");
  const auto elapsed = std::chrono::duration<double, std::micro>(
                           std::chrono::steady_clock::now() - start)
                           .count();
  std::cout << "quiet_subgrid,bits=" << bits
            << ",cycles=32,attempts=" << attempts
            << ",published_changes=" << changes << ",active_work=" << work
            << ",centroid_edges=" << edges << ",elapsed_us=" << elapsed
            << ",preserved_velocity=" << velocity
            << ",pending_particles=0,force_equilibrium_claim=0 PASS\n";
  if (!bits) return;
  auto force = inert();
  force.gravity = .1;
  h.tick64(force);
  refresh(h);
  require(h.particle64(kPosX, a) != position &&
              h.particle64(kVelX, a) != velocity &&
              h.demand().particles_evaluated == 2,
          "changing force did not wake/cross a publication boundary");
  PreparedFixtureAccess::inject_value64(h, kVelX, a, 0);
  PreparedFixtureAccess::inject_value64(h, kVelX, b, 0);
  h.tick64(inert());
  h.tick64(inert());
  refresh(h);
  require(h.prepared_storage().inspection_counts[1] == 0,
          "fixed-state bits were not consumed");
  UpdateCommand contact;
  contact.operation = UpdateOp::InsertContact;
  contact.first = UpdateRef::existing(in.particle_rows[0]);
  contact.second = UpdateRef::existing(in.particle_rows[1]);
  contact.position = 0;
  require(
      h.update_prepared(h.prepared_storage().generation, &contact, 1).status ==
          UpdateStatus::Ok,
      "contact perturbation failed");
  auto p = inert();
  p.contact_stiffness = .25;
  h.tick64(p);
  refresh(h);
  require(
      h.demand().contact_evaluations == 2 && h.particle64(kVelX, a) < 0 &&
          h.particle64(kVelX, b) > 0,
      "contact perturbation lost one index side or failed to recover sleepers");
  std::cout << "f64 changing-force boundary crossing, fixed-state consumption "
               "and two-sided contact/sleeper recovery PASS\n";
}
void coverage_repair(engine::Runtime &r, HostPath path) {
  const auto profile = NumericalProfile::trial();
  const auto l = limits(false, profile);
  Harness h(r, PreparedCapacity(l, profile), path);
  const auto d = std::uint64_t{1} << 53;
  FixtureSource src;
  src.kinds = {1};
  src.exact_masses = {d};
  std::array<float, kParticleFieldCount> body{};
  body[kPosX] = 1.5f;
  body[kMass] = 1;
  src.particles = {body};
  src.edges = {{0, 0, 1, 0, d - 1}};
  src.focus = {0};
  PreparedFixtureAccess::Input in(70, 9, 32, 9);
  PreparedFixtureAccess::load(h, src, in, legacy_inert(), true);
  h.tick64(inert());
  refresh(h);
  const auto &s = h.prepared_storage();
  const auto old_mass = h.group64(kCenM, in.field_rows[0].record),
             position = h.particle64(kPosX, in.particle_rows[0].record);
  const auto generation = s.generation;
  UpdateCommand edit;
  edit.operation = UpdateOp::ReplaceParticipation;
  edit.generation = generation;
  edit.row = 0;
  edit.ordinal = 0;
  edit.first = UpdateRef::existing(in.particle_rows[0]);
  edit.second = UpdateRef::existing(in.field_rows[0]);
  edit.share = 1;
  edit.exact_count = d - 2;
  UpdateResult result;
  no_new([&] { result = h.update_prepared(generation, &edit, 1); });
  refresh(h);
  require(result.status == UpdateStatus::Ok && s.generation == generation + 1 &&
              h.group64(kCenM, in.field_rows[0].record) ==
                  double(d - 2) / double(d) &&
              h.group64(kCenM, in.field_rows[0].record) != old_mass,
          "same-f32-share count edit missed numerical repair");
  require(h.group64(kCenX, in.field_rows[0].record) == 1.5 &&
              h.particle64(kPosX, in.particle_rows[0].record) == position &&
              s.inspection_counts[1] == 1,
          "mass-only change did not preserve position and invalidate inputs");
  auto self = inert();
  self.gravity = .1;
  h.tick64(self);
  refresh(h);
  require(h.demand().force_requests == 1 && h.demand().force_evaluations == 0,
          "single-member field gained fictitious self response");
  const Snapshot before(h);
  edit.generation = s.generation;
  edit.exact_count.reset();
  no_new([&] { result = h.update_prepared(s.generation, &edit, 1); });
  require(result.status == UpdateStatus::InvalidInput,
          "unknown C1 source count accepted");
  before.unchanged(h);
  edit.exact_count = d + 1;
  no_new([&] { result = h.update_prepared(s.generation, &edit, 1); });
  require(result.status == UpdateStatus::InvalidInput,
          "invalid source count accepted");
  before.unchanged(h);
  rejects([&] { h.particle(kPosX, 0) = 0; });
  rejects([&] { h.tick(legacy_inert()); });
  before.unchanged(h);
  auto bad = inert();
  bad.dt = std::numeric_limits<double>::denorm_min();
  rejects([&] { h.tick64(bad); });
  before.unchanged(h);
  bad = inert();
  bad.dt = std::numeric_limits<double>::infinity();
  rejects([&] { h.tick64(bad); });
  before.unchanged(h);
  std::array<UpdateCommand, 2> commands;
  commands[0].operation = UpdateOp::DefineParticle;
  commands[0].address = 5000;
  commands[0].exact_mass = 3;
  commands[1].operation = UpdateOp::InsertParticipation;
  commands[1].first = UpdateRef::pending(0);
  commands[1].second = UpdateRef::existing(in.field_rows[0]);
  commands[1].position = 1;
  commands[1].ordinal = 1;
  commands[1].share = float(1. / 3);
  commands[1].exact_count = 1;
  std::array<double, kParticleFieldCount> precise{};
  precise[kPosX] = 1 + std::ldexp(1., -40);
  precise[kMass] = 1;
  precise[kRadius] = .25;
  PreparedFixtureAccess::stage_new(h, 0, precise);
  no_new([&] {
    result = h.update_prepared(s.generation, commands.data(), commands.size());
  });
  require(result.status == UpdateStatus::Ok, "typed f64 new row failed");
  refresh(h);
  const auto fresh = s.resolved[0];
  require(h.particle64(kPosX, fresh.record) == precise[kPosX],
          "update silently narrowed a double start");
  CascadeNode node;
  node.field = in.field_rows[0];
  node.comparison = 1;
  node.cohort_count = 1;
  node.retained_fraction_bits = 52;
  CascadeContribution row{fresh, {s.generation, 1, 1}};
  WeightedCascadeSpec spec{&node, 1, nullptr, 0, &row, 1};
  require(h.bind_weighted_centroid_cascade(s.generation, spec).status ==
              CascadeStatus::Ok,
          "f64 new row candidate bind failed");
  const auto q = h.query_centroid_cascade(s.generation);
  std::cout << "typed_candidate,status=" << int(q.status)
            << ",x=" << q.centre[0] << ",mass=" << q.mass
            << ",expected_x=" << precise[kPosX] << ",expected_mass=" << 1. / 3
            << '\n';
  require(q.status == CascadeStatus::Ok && q.centre[0] == precise[kPosX] &&
              q.mass == 1. / 3 && q.mass != double(float(1. / 3)),
          "candidate used narrowed body/share data");
  const auto geo = h.query_prepared_geometry(s.generation);
  require(geo.status == GeometryStatus::Ok &&
              std::fabs(geo.focal_point[0] -
                        h.group64(kCenX, in.field_rows[0].record)) < 1e-15,
          "geometry narrowed f64 focal state");
  const Snapshot after(h);
  const auto rejected = h.query_prepared_geometry(s.generation, 0);
  require(rejected.status == GeometryStatus::CapacityExhausted,
          "geometry work limit ignored");
  after.unchanged(h);
  std::cout << "f64 same-share count repair, physical mass separation, typed "
               "start/query/geometry, unsupported metadata/typed views/FTZ "
               "parameters and atomic rejection PASS\n";
}
void overflow_and_reuse(engine::Runtime &r, HostPath path) {
  const auto profile = NumericalProfile::trial();
  auto l = limits(false, profile);
  l[Budget::Particles] = 2;
  l[Budget::ScratchBytes] =
      detail::PreparedStorage::required_scratch(l, false, profile);
  Harness h(r, PreparedCapacity(l, profile), path);
  auto src = pair_source(false);
  src.exact_masses[0] = UINT64_MAX;
  src.edges[0].exact_count = UINT64_MAX - 1;
  PreparedFixtureAccess::Input in(2, 9, 32, 9);
  PreparedFixtureAccess::load(h, src, in, legacy_inert(), false);
  refresh(h);
  const auto &s = h.prepared_storage();
  const Snapshot before(h);
  UpdateCommand over;
  over.operation = UpdateOp::InsertParticipation;
  over.first = UpdateRef::existing(in.particle_rows[0]);
  over.second = UpdateRef::existing(in.field_rows[0]);
  over.position = 2;
  over.ordinal = 2;
  over.share = float(2.L / UINT64_MAX);
  over.exact_count = 2;
  UpdateResult result;
  no_new([&] { result = h.update_prepared(s.generation, &over, 1); });
  require(result.status == UpdateStatus::InvalidInput,
          "u64 per-field coverage wrapped");
  before.unchanged(h);
  auto victim = in.particle_rows[0];
  const auto original = victim;
  const auto survivor = in.particle_rows[1];
  prepared_test::Metrics metrics(r, PreparedFixtureAccess::nodes(h));
  const auto prepared = metrics.sample();
  for (int i = 0; i < 32; ++i) {
    std::size_t row = 0;
    while (row < s.live.edges && s.edges[row].first.record != victim.record)
      ++row;
    std::array<UpdateCommand, 2> remove;
    remove[0].operation = UpdateOp::RemoveParticipation;
    remove[0].row = row;
    remove[0].generation = s.generation;
    remove[1].operation = UpdateOp::RetireParticle;
    remove[1].first = UpdateRef::existing(victim);
    no_new([&] {
      result = h.update_prepared(s.generation, remove.data(), remove.size());
    });
    require(
        result.status == UpdateStatus::Ok && !s.source_masses[victim.record],
        "trial retirement retained metadata");
    std::array<UpdateCommand, 2> add;
    add[0].operation = UpdateOp::DefineParticle;
    add[0].address = original.address;
    add[0].exact_mass = 3;
    add[1].operation = UpdateOp::InsertParticipation;
    add[1].first = UpdateRef::pending(0);
    add[1].second = UpdateRef::existing(in.field_rows[0]);
    add[1].position = 1;
    add[1].ordinal = 10 + i;
    add[1].share = 1;
    add[1].exact_count = 3;
    std::array<double, kParticleFieldCount> body{};
    body[kPosX] = 2 + std::ldexp(1., -40);
    body[kMass] = 2;
    PreparedFixtureAccess::stage_new(h, 0, body);
    no_new([&] {
      result = h.update_prepared(s.generation, add.data(), add.size());
    });
    require(result.status == UpdateStatus::Ok, "trial reuse failed");
    victim = s.resolved[0];
    no_new([&] {
      h.tick64(inert());
      refresh(h);
    });
    require(victim.record == original.record &&
                victim.incarnation != original.incarnation &&
                h.particle64(kPosX, victim.record) == body[kPosX] &&
                h.particle64(kPosX, survivor.record) == 3,
            "recycling inherited state or reset a survivor");
  }
  CascadeNode node;
  node.field = in.field_rows[0];
  node.comparison = 1;
  node.cohort_count = 1;
  CascadeContribution old{original, {s.generation, 1, s.edges[1].ordinal}};
  require(h.bind_weighted_centroid_cascade(s.generation,
                                           {&node, 1, nullptr, 0, &old, 1})
                  .status == CascadeStatus::InvalidReference,
          "recycled row legitimized retired handle");
  require(prepared.same_backing(metrics.sample()),
          "f64 claim/release acquired backing/compiler state");
  PreparedFixtureAccess::inject_share_value(h, 1, .5f);
  h.set_tick_mode(TickMode::Selective);
  bool failed = false;
  try {
    h.tick64(inert());
  } catch (const std::runtime_error &) {
    failed = true;
  }
  require(failed && !h.ready(), "contradictory native source stayed healthy");
  std::cout
      << "f64 exact overflow rejection, 32 retirement/reuse cycles, survivor "
         "preservation, retired handle and unready native-source fault PASS\n";
}
void selective_reference(engine::Runtime &r, HostPath path) {
  const auto profile = NumericalProfile::trial(48, 48);
  const auto l = limits(false, profile);
  Harness a(r, PreparedCapacity(l, profile), path),
      b(r, PreparedCapacity(l, profile), path);
  auto src = fixture(8, 9);
  src.contacts = {{0, 1}, {1, 2}, {2, 7}};
  for (auto &p : src.particles) p[kRadius] = 1;
  PreparedFixtureAccess::Input ia(70, 9, 32, 9), ib(70, 9, 32, 9);
  PreparedFixtureAccess::load(a, src, ia, legacy_inert(), true);
  PreparedFixtureAccess::load(b, src, ib, legacy_inert(), true);
  b.set_tick_mode(TickMode::AllLoaded);
  auto p = inert();
  p.gravity = .0001;
  p.contact_stiffness = .001;
  p.dt = .015625;
  p.reach_strength = .01;
  for (int cycle = 0; cycle < 32; ++cycle) {
    a.tick64(p);
    b.tick64(p);
    refresh(a);
    refresh(b);
    for (int i = 0; i < 8; ++i)
      for (int f = 0; f < kParticleFieldCount; ++f)
        require(a.particle64(f, ia.particle_rows[i].record) ==
                    b.particle64(f, ib.particle_rows[i].record),
                "selective/all-loaded f64 body trajectories differ");
    for (int g = 0; g < 3; ++g)
      for (int f = 0; f < 4; ++f)
        require(a.group64(f, ia.field_rows[g].record) ==
                    b.group64(f, ib.field_rows[g].record),
                "selective/all-loaded f64 field publications differ");
  }
  std::cout << "f64 native selective/all-loaded combined fields, fractional "
               "ordinals, focal radial control and contacts: 32 ticks PASS\n";
}
void repeated_single_occurrence(engine::Runtime &r, HostPath path) {
  const auto profile = NumericalProfile::trial(52, 4);
  const auto l = limits(false, profile);
  Harness h(r, PreparedCapacity(l, profile), path);
  PreparedFixtureAccess::Input in(70, 9, 32, 9);
  FixtureSource src;
  src.kinds = {1};
  src.exact_masses = {3};
  std::array<float, kParticleFieldCount> body{};
  body[kPosX] = -1.55f;
  body[kMass] = 7;
  src.particles = {body};
  src.edges = {{0, 0, float(1. / 3), 0, 1}, {0, 0, float(2. / 3), 1, 2}};
  src.focus = {0};
  PreparedFixtureAccess::load(h, src, in, legacy_inert(), true);
  refresh(h);
  const auto p = in.particle_rows[0].record, g = in.field_rows[0].record;
  const double x = h.particle64(kPosX, p);
  require(h.group64(kCenX, g) != x,
          "fixture missed coarse-field self-response edge");
  auto force = inert();
  force.gravity = .1;
  no_new([&] {
    h.tick64(force);
    refresh(h);
  });
  require(h.particle64(kPosX, p) == x && h.particle64(kVelX, p) == 0 &&
              h.demand().force_requests == 2 && !h.demand().force_evaluations,
          "repeated ordinals/coarse field gained fictitious self-force");
  std::cout << "f64 single occurrence/repeated ordinals with distinct "
               "body/field precision: no self-response PASS\n";
}
void assembly_sources(engine::Runtime &r, HostPath path) {
  using namespace assembly;
  taichi::lang::SNode root(0, taichi::lang::SNodeType::root);
  StudyInput input;
  input.capacity = 3;
  input.nodes = {{&root, -1, ScaleRole::Root}};
  input.definitions = {
      {"letter", 3, {}, {{{}, .125}}, {{"A", 0, 1}, {"B", 1, 1}, {"A", 2, 1}}}};
  for (int i = 0; i < 3; ++i) {
    const auto name = std::to_string(i);
    input.occurrences.push_back(
        {name, "letter", i, 0, {double(i * 4) - 4, 0, 0}, {}});
    input.memberships.push_back({name, "language"});
  }
  const auto prepared = prepare(input);
  FixtureSource src;
  for (const auto &g : prepared.groups) src.kinds.push_back(int(g.kind) + 1);
  for (const auto &p : prepared.particles) {
    std::array<float, kParticleFieldCount> body{};
    body[kPosX] = p.position.x;
    body[kMass] = p.mass;
    body[kRadius] = p.radius;
    src.particles.push_back(body);
    src.exact_masses.push_back(p.exact_mass);
  }
  for (const auto &e : prepared.participation)
    src.edges.push_back({e.slot, e.group, e.share, e.ordinal, e.exact_count});
  const auto profile = NumericalProfile::trial(48, 48);
  const auto l = limits(false, profile);
  Harness h(r, PreparedCapacity(l, profile), path);
  PreparedFixtureAccess::Input in(70, 9, 32, 9);
  PreparedFixtureAccess::load(h, src, in, legacy_inert(), true);
  refresh(h);
  fields_oracle(h);
  require(h.universal64() == 9 && h.present_particles() == 3,
          "independent letter copies duplicated/aliased physical mass");
  for (int cycle = 0; cycle < 8; ++cycle) {
    auto p = inert();
    p.gravity = .0001;
    p.dt = .015625;
    no_new([&] {
      h.tick64(p);
      refresh(h);
    });
    fields_oracle(h);
  }
  std::cout << "f64 assembly-derived letter counts, repeated parents, "
               "independent copies and universal physical mass PASS\n";
}
void independent_step(engine::Runtime &r, HostPath path) {
  const auto profile = NumericalProfile::trial(32, 32);
  const auto l = limits(false, profile);
  Harness h(r, PreparedCapacity(l, profile), path);
  FixtureSource src;
  src.kinds = {1};
  src.exact_masses = {3, 3};
  for (int i = 0; i < 2; ++i) {
    std::array<float, kParticleFieldCount> body{};
    body[kPosX] = i ? 2 : -2;
    body[kMass] = i ? 2 : 1;
    src.particles.push_back(body);
    src.edges.push_back(
        {i, 0, float((i + 1) / 3.), std::uint64_t(i), std::uint64_t(i + 1)});
  }
  PreparedFixtureAccess::Input in(70, 9, 32, 9);
  PreparedFixtureAccess::load(h, src, in, legacy_inert(), true);
  refresh(h);
  const long double centre = h.group64(kCenX, in.field_rows[0].record),
                    gm = h.group64(kCenM, in.field_rows[0].record);
  auto params = inert();
  params.gravity = .0123456789012345;
  params.dt = .125;
  std::array<double, 2> next, vel;
  for (int i = 0; i < 2; ++i) {
    const long double x = i ? 2 : -2, m = i ? 2 : 1, m1 = m * (i + 1) / 3,
                      dx = centre - x, d2 = dx * dx;
    const long double force = static_cast<long double>(params.gravity) * m1 *
                              gm * dx / (d2 * std::sqrt(d2));
    const long double v = force / m * params.dt,
                      ratio = std::fabs(v) * params.dt / std::fabs(force),
                      brake = std::exp(-std::pow(ratio, 4));
    vel[i] = double(v * brake);
    next[i] = quantize(double(x + v * brake * params.dt), 32);
  }
  no_new([&] {
    h.tick64(params);
    refresh(h);
  });
  for (int i = 0; i < 2; ++i) {
    const auto p = in.particle_rows[i].record;
    require(
        h.particle64(kPosX, p) == next[i] &&
            std::fabs(h.particle64(kVelX, p) - vel[i]) <=
                16 * std::numeric_limits<double>::epsilon() * std::fabs(vel[i]),
        "f64 combined force/integration differs from long-double scalar "
        "reference");
  }
  std::cout << "f64 fractional force, typed scalars, brake/integration and "
               "both-sign Q_32: independent long-double step PASS\n";
}
void collective_motion(engine::Runtime &r, HostPath path) {
  const auto profile = NumericalProfile::trial(52, 4);
  const auto l = limits(false, profile);
  Harness h(r, PreparedCapacity(l, profile), path);
  FixtureSource src;
  src.kinds = {1};
  src.exact_masses = {1, 1, 1};
  for (int i = 0; i < 3; ++i) {
    std::array<float, kParticleFieldCount> p{};
    p[kPosX] = 1 + i * .125f;
    p[kMass] = 1;
    p[kVelX] = i < 2 ? .03125f : 0;
    src.particles.push_back(p);
    src.edges.push_back({i, 0, 1, std::uint64_t(i), 1});
  }
  PreparedFixtureAccess::Input in(70, 9, 32, 9);
  PreparedFixtureAccess::load(h, src, in, legacy_inert(), true);
  refresh(h);
  const auto g = in.field_rows[0].record;
  const double first = h.group64(kCenX, g);
  for (int cycle = 0; cycle < 4; ++cycle) {
    no_new([&] {
      h.tick64(inert());
      refresh(h);
    });
    fields_oracle(h);
    require(h.demand().centroids_recomputed == 1 &&
                h.demand().centroid_reduction_edges == 3,
            "dirty-field reduction omitted sleepers or repeated field work");
    if (!cycle)
      require(h.group64(kCenX, g) == first,
              "first small collective motion crossed unexpectedly");
  }
  require(h.group64(kCenX, g) > first &&
              h.particle64(kPosX, in.particle_rows[2].record) == 1.25,
          "fresh reduction lost collective accepted motion or moved sleeper");
  std::cout << "f64 collective accepted motion crosses field Q cell through "
               "fresh full-member reduction, including sleeper PASS\n";
}
void failures(engine::Runtime &r, HostPath path) {
  const auto profile = NumericalProfile::trial();
  auto l = limits(false, profile);
  --l[Budget::ScratchBytes];
  const auto ids = r.issued_snode_ids();
  bool rejected = false;
  try {
    Harness h(r, PreparedCapacity(l, profile), path);
  } catch (const CapacityError &e) {
    rejected = e.failure.budget == Budget::ScratchBytes;
  }
  require(rejected && r.issued_snode_ids() == ids,
          "f64 scratch failure mutated layout");
  bool invalid = false;
  try {
    PreparedCapacity c(l, {NumericalKind::TrialF64, 53, 52});
  } catch (const std::invalid_argument &) {
    invalid = true;
  }
  require(invalid, "unsupported numerical profile accepted");
  auto too_wide = limits(false, profile);
  too_wide[Budget::Participation] = 1ull << 30;
  invalid = false;
  try {
    PreparedCapacity c(too_wide, profile);
  } catch (const std::invalid_argument &) {
    invalid = true;
  }
  require(invalid && r.issued_snode_ids() == ids,
          "oversized trial descriptor mutated layout");
  {
    auto disabled = limits(false, profile);
    for (auto kind :
         {Budget::GeometryWork, Budget::CascadeNodes, Budget::CascadeLinks,
          Budget::CascadeReferences, Budget::CascadeCoverage,
          Budget::CascadeLevels, Budget::CascadeWork})
      disabled[kind] = 0;
    disabled[Budget::ScratchBytes] =
        detail::PreparedStorage::required_scratch(disabled, false, profile);
    Harness empty(r, PreparedCapacity(disabled, profile), path);
    no_new([&] {
      empty.tick64(inert());
      empty.inspect_prepared();
    });
    require(
        empty.ready() && !empty.present_particles() && empty.universal64() == 0,
        "empty f64 execution requires optional geometry/hierarchy");
  }
  l = limits(false, profile);
  Harness h(r, PreparedCapacity(l, profile), path);
  PreparedFixtureAccess::Input in(70, 9, 32, 9);
  PreparedFixtureAccess::load(h, pair_source(false), in, legacy_inert(), true);
  auto p = inert();
  p.gravity = std::numeric_limits<double>::max();
  bool failed = false;
  try {
    h.tick64(p);
  } catch (const std::runtime_error &) {
    failed = true;
  }
  require(failed && !h.ready(),
          "post-mutation invalid arithmetic returned healthy");
  std::cout << "f64 independent scratch/profile rejection and post-mutation "
               "numerical failure policy PASS\n";
}
}  // namespace
int main(int argc, char **argv) {
  try {
    std::cout << std::setprecision(17);
    bool direct = false, large = false, maximum = false;
    for (int i = 1; i < argc; ++i) {
      std::string arg = argv[i];
      if (arg == "--direct")
        direct = true;
      else if (arg == "--large")
        large = true;
      else if (arg == "--maximum")
        maximum = true;
      else
        throw std::invalid_argument("unknown mode");
    }
    engine::InstanceSettings settings;
    settings.read_environment = false;
    settings.cpu_threads = 2;
    engine::Runtime runtime(settings);
    require(!runtime.on_cuda(), "expected actual CPU backend");
    const auto path = direct ? HostPath::CpuDirect : HostPath::BulkCopy;
    std::cout << "actual_backend=cpu,path=" << (direct ? "direct" : "bulk")
              << ",profile=trial_f64\n";
    std::cout << "typed_widths,Demand=" << sizeof(Demand)
              << ",GeometryResult=" << sizeof(GeometryResult)
              << ",Harness=" << sizeof(Harness)
              << ",PreparedStorage=" << sizeof(detail::PreparedStorage)
              << ",RealBuffer=" << sizeof(detail::RealBuffer) << '\n';
    run(runtime, path, large, maximum);
    if (!large && !maximum) {
      watch_point(runtime, path, 4);
      watch_point(runtime, path, 0);
      coverage_repair(runtime, path);
      repeated_single_occurrence(runtime, path);
      assembly_sources(runtime, path);
      independent_step(runtime, path);
      collective_motion(runtime, path);
      overflow_and_reuse(runtime, path);
      selective_reference(runtime, path);
      failures(runtime, path);
    }
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
