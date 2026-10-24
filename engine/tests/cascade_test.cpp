#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <limits>
#include <stdexcept>

#include "allocation_observation.h"
#include "assembly/study.h"
#include "prepared_fixture.h"
#include "prepared_metrics.h"

namespace {
using namespace field;
using namespace field::testing;
void require(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
template <class F>
void rejects(F f) {
  try {
    f();
  } catch (const std::logic_error &) {
    return;
  }
  throw std::runtime_error("stale/unready cascade observation accepted");
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
          "cascade boundary allocated C++ metadata");
}
template <class T>
bool bytes(const std::vector<T> &a, const std::vector<T> &b) {
  return a.size() == b.size() &&
         !std::memcmp(a.data(), b.data(), a.size() * sizeof(T));
}
struct Snapshot {
  std::vector<float> particles, groups, speed, motion, shares, universal;
  std::vector<int> state, flags, contacts, edges;
  std::vector<engine::RecordHandle> particle_headers, field_headers;
  std::vector<detail::RelationshipHeader> participation, contact_headers;
  std::array<float, 11> focus;
  FrameAnchor anchor;
  std::array<unsigned char, sizeof(Demand)> demand{};
  detail::PreparedCounts live;
  std::uint64_t generation, ticks;
  explicit Snapshot(Harness &h)
      : particles(h.particle_data),
        groups(h.group_data),
        speed(h.determined()),
        motion(h.prepared_storage().inspection_motion),
        shares(h.edge_float_data),
        universal(h.universal_data),
        state(h.prepared_storage().inspection_state),
        flags(h.prepared_storage().inspection_flags),
        contacts(h.contact_int_data),
        edges(h.edge_int_data),
        particle_headers(h.prepared_storage().particle_headers),
        field_headers(h.prepared_storage().field_headers),
        participation(h.prepared_storage().edges),
        contact_headers(h.prepared_storage().contacts),
        focus(h.prepared_storage().inspection_focus),
        anchor(h.frame_anchor()),
        live(h.prepared_storage().live),
        generation(h.prepared_storage().generation),
        ticks(h.prepared_storage().completed_ticks) {
    std::memcpy(demand.data(), &h.demand(), demand.size());
  }
  void unchanged(Harness &h) const {
    require(h.ready(), "candidate operation changed readiness");
    h.inspect_prepared();
    PreparedFixtureAccess::read_relationships(h);
    const auto &s = h.prepared_storage();
    require(bytes(particles, h.particle_data) && bytes(groups, h.group_data) &&
                bytes(speed, h.determined()) &&
                bytes(motion, s.inspection_motion) &&
                bytes(shares, h.edge_float_data) &&
                bytes(universal, h.universal_data) &&
                state == s.inspection_state && flags == s.inspection_flags &&
                contacts == h.contact_int_data && edges == h.edge_int_data &&
                bytes(particle_headers, s.particle_headers) &&
                bytes(field_headers, s.field_headers) &&
                bytes(participation, s.edges) &&
                bytes(contact_headers, s.contacts) &&
                !std::memcmp(focus.data(), s.inspection_focus.data(),
                             sizeof(focus)) &&
                anchor == h.frame_anchor() &&
                !std::memcmp(demand.data(), &h.demand(), demand.size()) &&
                generation == s.generation && ticks == s.completed_ticks &&
                live.particles == s.live.particles &&
                live.fields == s.live.fields && live.edges == s.live.edges &&
                live.contacts == s.live.contacts && live.focus == s.live.focus,
            "cascade changed committed "
            "numerical/source/activity/history/Demand state");
  }
};
Parameters inert() {
  Parameters p;
  p.gravity = 0;
  p.contact_stiffness = 0;
  p.reach_strength = 0;
  return p;
}
BudgetDemand limits(engine::Runtime &r, bool large) {
  BudgetDemand l;
  l.values = {large ? 16777216u : 1048576u,
              large ? 4096u : 70u,
              large ? 257u : 9u,
              large ? 16384u : 280u,
              large ? 8192u : 140u,
              large ? 53762u : 928u,
              large ? 257u : 9u,
              large ? 256u : 32u,
              0};
  l[Budget::CascadeNodes] = 7;
  l[Budget::CascadeLinks] = 6;
  l[Budget::CascadeReferences] = 3 * l[Budget::Particles];
  l[Budget::CascadeLevels] = 3;
  l[Budget::CascadeWork] = std::uint64_t{1} << 40;
  l[Budget::ScratchBytes] =
      detail::PreparedStorage::required_scratch(l, r.on_cuda());
  return l;
}
FixtureSource fixture(std::size_t n, std::size_t groups, bool moving) {
  FixtureSource src;
  src.kinds.resize(groups, 1);
  for (std::size_t g = 0; g < groups; ++g)
    src.kinds[g] = 1 + int(g % 3);
  for (std::size_t i = 0; i < n; ++i) {
    std::array<float, kParticleFieldCount> p{};
    p[kPosX] = float(i * 4);
    p[kMass] = 1 + int(i % 7);
    p[kRadius] = .25f;
    src.particles.push_back(p);
  }
  if (n == 8) {
    const std::array<float, 8> x{
        {1, std::nextafter(1.0f, 2.0f), 1.8f, 1.9f, -1.8f, -1.9f, 64, 1}};
    const std::array<float, 8> m{{1, 1, 1, 2, 1, 2, 2, 3}};
    for (int i = 0; i < 8; ++i) {
      src.particles[i][kPosX] = x[i];
      src.particles[i][kMass] = m[i];
    }
  }
  if (moving)
    src.particles[0][kVelX] = .125f;
  for (int g = 0; g < 8; ++g)
    for (std::size_t i = 0; i < n; ++i) {
      bool belongs = g < 4    ? i >= g * n / 4 && i < (g + 1) * n / 4
                     : g == 4 ? i < n / 2
                     : g == 5 ? i >= n / 2
                              : true;
      if (belongs)
        src.edges.push_back({int(i), g, 1, std::uint64_t(src.edges.size())});
    }
  src.focus = {6};
  return src;
}
std::vector<int> range(int begin, int end) {
  std::vector<int> result;
  for (int i = begin; i < end; ++i)
    result.push_back(i);
  return result;
}
struct Hierarchy {
  std::vector<CascadeNode> nodes;
  std::vector<int> children;
  std::vector<engine::RecordHandle> cohorts;
  void add(const PreparedFixtureAccess::Input &in,
           int field,
           std::uint64_t context,
           int level,
           bool publish,
           int bits,
           const std::vector<int> &members,
           const std::vector<int> &parts = {}) {
    CascadeNode node;
    node.field = in.field_rows[field];
    node.comparison = context;
    node.level = level;
    node.publish = publish;
    node.retained_fraction_bits = bits;
    node.cohort_begin = cohorts.size();
    node.cohort_count = members.size();
    for (int p : members)
      cohorts.push_back(in.particle_rows[p]);
    node.child_begin = children.size();
    node.child_count = parts.size();
    children.insert(children.end(), parts.begin(), parts.end());
    nodes.push_back(node);
  }
  CascadeSpec spec() const {
    return {nodes.data(),    nodes.size(),   children.data(),
            children.size(), cohorts.data(), cohorts.size()};
  }
};
Hierarchy hierarchy(const PreparedFixtureAccess::Input &in, int n, int shape) {
  Hierarchy result;
  if (shape == 1)
    result.add(in, 6, 100, 0, true, 52, range(0, n));
  if (shape == 2) {
    result.add(in, 4, 101, 0, true, 52, range(0, n / 2));
    result.add(in, 5, 102, 0, true, 52, range(n / 2, n));
    result.add(in, 6, 103, 2, true, 52, range(0, n), {0, 1});
  }
  if (shape == 3) {
    for (int i = 0; i < 4; ++i)
      result.add(in, i, 200 + i, 0, true, 52,
                 range(i * n / 4, (i + 1) * n / 4));
    result.add(in, 4, 204, 1, true, 52, range(0, n / 2), {0, 1});
    result.add(in, 5, 205, 1, true, 52, range(n / 2, n), {2, 3});
    result.add(in, 6, 206, 2, true, 52, range(0, n), {4, 5});
  }
  if (shape == 4) {
    result.add(in, 6, 300, 0, false, 52, range(0, n / 2));
    result.add(in, 6, 300, 0, false, 52, range(n / 2, n));
    result.add(in, 6, 300, 0, true, 52, range(0, n), {0, 1});
  }
  return result;
}
double quantize(double x, int bits) {
  if (x == 0)
    return x;
  int e = 0;
  std::frexp(std::fabs(static_cast<long double>(x)), &e);
  e = std::max(e - 1, -1022);
  return std::copysign(
      double(std::ldexp(std::trunc(std::ldexp(
                            std::fabs(static_cast<long double>(x)), bits - e)),
                        e - bits)),
      x);
}
void oracle(const CascadeResult &q,
            const CascadeInspection &v,
            const Snapshot &physical,
            int pstride,
            int nstride) {
  for (std::size_t node = 0; node < q.nodes; ++node) {
    const auto &def = v.nodes[node];
    long double mass = 0;
    std::array<long double, 3> sum{}, maximum{};
    int count = 0;
    const auto add = [&](long double m, const std::array<long double, 3> &x) {
      mass += m;
      for (int axis = 0; axis < 3; ++axis) {
        sum[axis] += m * x[axis];
        maximum[axis] = std::max(maximum[axis], std::fabs(x[axis]));
      }
      ++count;
    };
    if (def.child_count) {
      for (std::uint64_t k = def.child_begin;
           k < def.child_begin + def.child_count; ++k) {
        const int child = v.children[k];
        std::array<long double, 3> x;
        for (int axis = 0; axis < 3; ++axis)
          x[axis] = v.values[(3 + axis) * nstride + child];
        add(v.values[6 * nstride + child], x);
      }
    } else
      for (std::uint64_t k = def.cohort_begin;
           k < def.cohort_begin + def.cohort_count; ++k) {
        const int row = v.cohorts[k].record;
        std::array<long double, 3> x;
        for (int axis = 0; axis < 3; ++axis)
          x[axis] = physical.particles[(kPosX + axis) * pstride + row];
        add(physical.particles[kMass * pstride + row], x);
      }
    require(v.valid[node] == 1 && v.values[6 * nstride + node] == double(mass),
            "incorrect hierarchy mass/validity");
    for (int axis = 0; axis < 3; ++axis) {
      const double trial = v.values[axis * nstride + node],
                   published = v.values[(3 + axis) * nstride + node];
      const long double expected = sum[axis] / mass,
                        tolerance = 8 * (count + 1) *
                                    std::numeric_limits<double>::epsilon() *
                                    std::max(1.0L, maximum[axis]);
      require(
          std::fabs(static_cast<long double>(trial) - expected) <= tolerance,
          "independent local/rolled-up centroid disagrees");
      const double want =
          def.publish ? quantize(trial, def.retained_fraction_bits) : trial;
      require(!std::memcmp(&want, &published, sizeof(want)),
              "publication boundary/identity mask incorrect");
    }
  }
  require(q.frame_anchor == physical.anchor &&
              q.mass == v.values[6 * nstride + q.root] &&
              q.centre[0] == v.values[3 * nstride + q.root],
          "root summary disagrees with candidate");
}
void run(engine::Runtime &r, HostPath path, bool large, bool maximum) {
  auto l = limits(r, large);
  const auto setup_start = std::chrono::steady_clock::now();
  Harness h(r, PreparedCapacity(l), path);
  const auto setup_us = std::chrono::duration<double, std::micro>(
                            std::chrono::steady_clock::now() - setup_start)
                            .count();
  const auto &s = h.prepared_storage();
  auto empty = h.query_centroid_cascade(0);
  require(
      empty.status == CascadeStatus::Empty && empty.root == -1 && !empty.mass,
      "empty cascade fabricated a centre");
  h.inspect_centroid_cascade(empty);
  const auto src =
      fixture(maximum ? l[Budget::Particles] : 8, l[Budget::Fields], !maximum);
  PreparedFixtureAccess::Input input(l[Budget::Particles], l[Budget::Fields],
                                     l[Budget::Batch], l[Budget::Focus]);
  prepared_test::Metrics metrics(r, PreparedFixtureAccess::nodes(h));
  const auto prepared = metrics.sample();
  PreparedFixtureAccess::load(h, src, input, inert(), true);
  h.inspect_prepared();
  const Snapshot before(h);
  auto tree = hierarchy(input, int(src.particles.size()), 3);
  CascadeBinding binding;
  const auto start = std::chrono::steady_clock::now();
  no_new([&] { binding = h.bind_centroid_cascade(s.generation, tree.spec()); });
  const auto bind_us = std::chrono::duration<double, std::micro>(
                           std::chrono::steady_clock::now() - start)
                           .count();
  require(binding.status == CascadeStatus::Ok && binding.nodes == 7 &&
              binding.links == 6 &&
              binding.references == 3 * src.particles.size() &&
              binding.levels == 3,
          "prepared hierarchy binding failed");
  if (path == HostPath::BulkCopy) {
    std::fill(h.particle_data.begin(), h.particle_data.end(), -12345);
    std::fill(h.group_data.begin(), h.group_data.end(), -12345);
    std::fill(h.edge_int_data.begin(), h.edge_int_data.end(), -1);
    std::fill(h.edge_float_data.begin(), h.edge_float_data.end(), -.5f);
  }
  CascadeResult q;
  const CascadeInspection *v = nullptr;
  auto measured = std::chrono::steady_clock::now();
  no_new([&] { q = h.query_centroid_cascade(s.generation); });
  const auto first_query_us = std::chrono::duration<double, std::micro>(
                                  std::chrono::steady_clock::now() - measured)
                                  .count();
  measured = std::chrono::steady_clock::now();
  no_new([&] { v = &h.inspect_centroid_cascade(q); });
  const auto first_inspection_us =
      std::chrono::duration<double, std::micro>(
          std::chrono::steady_clock::now() - measured)
          .count();
  require(q.status == CascadeStatus::Ok &&
              q.required_work == 4 * src.particles.size() + 20 &&
              q.reference_visits == 3 * src.particles.size() &&
              q.reduction_visits == src.particles.size() + 6 &&
              q.node_reductions == 7 && q.readback_bytes == 184 &&
              q.synchronizations == 2,
          "current cascade visits/summary failed");
  oracle(q, *v, before, h.particle_capacity(), int(l[Budget::CascadeNodes]));
  const double first_centroid = v->values[0];
  before.unchanged(h);
  require(prepared.same_backing(metrics.sample()),
          "first binding/query acquired prepared backing/layout/compilation");
  if (!maximum) {
    require(first_centroid == 1 + std::ldexp(1.0, -24) &&
                first_centroid !=
                    double(h.group(kCenX, input.field_rows[0].record)),
            "f64 centroid reduction narrowed to baseline f32 centre");
    for (int shape : {1, 2, 4}) {
      auto request = hierarchy(input, 8, shape);
      CascadeResult old = q;
      no_new([&] {
        binding = h.bind_centroid_cascade(s.generation, request.spec());
        q = h.query_centroid_cascade(s.generation);
        v = &h.inspect_centroid_cascade(q);
      });
      require(
          binding.status == CascadeStatus::Ok && q.status == CascadeStatus::Ok,
          "single/two/internal hierarchy failed");
      rejects([&] { h.inspect_centroid_cascade(old); });
      oracle(q, *v, before, h.particle_capacity(),
             int(l[Budget::CascadeNodes]));
      before.unchanged(h);
    }
    // All 53 f64 masks are runtime data in one generic prepared kernel. Trim
    // one child so the oracle detects re-entry of discarded detail at parents.
    for (int bits = 0; bits <= 52; ++bits) {
      tree.nodes[1].retained_fraction_bits = bits;
      no_new([&] {
        binding = h.bind_centroid_cascade(s.generation, tree.spec());
        q = h.query_centroid_cascade(s.generation);
        v = &h.inspect_centroid_cascade(q);
      });
      require(
          binding.status == CascadeStatus::Ok && q.status == CascadeStatus::Ok,
          "retained precision rebind failed");
      oracle(q, *v, before, h.particle_capacity(),
             int(l[Budget::CascadeNodes]));
      before.unchanged(h);
    }
    tree.nodes[1].retained_fraction_bits = 3;
    binding = h.bind_centroid_cascade(s.generation, tree.spec());
    q = h.query_centroid_cascade(s.generation);
    const auto needed = q.required_work;
    auto failed = h.query_centroid_cascade(s.generation, needed - 1);
    require(failed.status == CascadeStatus::CapacityExhausted &&
                failed.failure.budget == Budget::CascadeWork &&
                failed.failure.required == needed && !failed.synchronizations,
            "query work limit failed");
    rejects([&] { h.inspect_centroid_cascade(q); });
    before.unchanged(h);
    q = h.query_centroid_cascade(s.generation);
    auto foreign = q;
    ++foreign.context;
    rejects([&] { h.inspect_centroid_cascade(foreign); });
    h.tick(inert());
    rejects([&] { h.inspect_centroid_cascade(q); });
    h.inspect_prepared();
    const Snapshot moved(h);
    q = h.query_centroid_cascade(s.generation);
    v = &h.inspect_centroid_cascade(q);
    require(q.awake_reference_visits == 3 &&
                q.centre[0] != before.groups[kCenX * h.field_capacity() +
                                             input.field_rows[6].record],
            "sleepers/current motion were not distinguished");
    oracle(q, *v, moved, h.particle_capacity(), int(l[Budget::CascadeNodes]));
    moved.unchanged(h);
    // Controlled native source traces: f32 body detail, f64 reductions, then
    // one expressed child boundary. Repeated queries must not recover a tail.
    tree.nodes[1].retained_fraction_bits = 4;
    require(h.bind_centroid_cascade(s.generation, tree.spec()).status ==
                CascadeStatus::Ok,
            "wobble profile bind failed");
    for (const float x : {1.5f, std::nextafter(1.5f, 2.f), 1.5625f, -1.5f,
                          std::nextafter(-1.5f, -2.f), -1.5625f}) {
      PreparedFixtureAccess::inject_position_value(
          h, input.particle_rows[2].record, x);
      PreparedFixtureAccess::inject_position_value(
          h, input.particle_rows[3].record, x);
      const Snapshot current(h);
      double published = 0;
      for (int repeat = 0; repeat < 3; ++repeat) {
        no_new([&] {
          q = h.query_centroid_cascade(s.generation);
          v = &h.inspect_centroid_cascade(q);
        });
        require(q.status == CascadeStatus::Ok, "wobble trace query failed");
        oracle(q, *v, current, h.particle_capacity(),
               int(l[Budget::CascadeNodes]));
        published = v->values[3 * l[Budget::CascadeNodes] + 1];
        require(published == quantize(double(x), 4),
                "wobble/crossing publication failed");
        current.unchanged(h);
      }
      std::cout << "publication_trace,input=" << x << ",published=" << published
                << ",origin_bias=" << (published - double(x)) << '\n';
    }
  }
  double repeated_query_us = 0;
  for (int cycle = 0; cycle < 32; ++cycle) {
    h.tick(inert());
    h.inspect_prepared();
    const Snapshot current(h);
    measured = std::chrono::steady_clock::now();
    no_new([&] {
      q = h.query_centroid_cascade(s.generation);
      v = &h.inspect_centroid_cascade(q);
    });
    repeated_query_us += std::chrono::duration<double, std::micro>(
                             std::chrono::steady_clock::now() - measured)
                             .count();
    require(q.status == CascadeStatus::Ok, "repeated current cascade failed");
    oracle(q, *v, current, h.particle_capacity(), int(l[Budget::CascadeNodes]));
    current.unchanged(h);
  }
  require(prepared.same_backing(metrics.sample()),
          "cascade reuse changed native backing/layout/compilation");
  prepared.print("after_setup");
  metrics.sample().print("after_cascade_cycles");
  for (const auto &entry : s.manifest)
    std::cout << "manifest," << entry.name << ',' << entry.count << ','
              << entry.element_bytes << ',' << entry.copies << ','
              << entry.bytes << '\n';
  std::cout << "cascade," << (large ? 4096 : 70) << ','
            << (path == HostPath::BulkCopy ? "bulk" : "direct") << ','
            << (maximum ? "maximum" : "holes/motion/precision")
            << ",setup_us=" << setup_us << ",bind_us=" << bind_us
            << ",first_query_us=" << first_query_us
            << ",first_inspection_us=" << first_inspection_us
            << ",repeat_query_inspection_mean_us=" << repeated_query_us / 32
            << ",bind_uploaded_bytes=" << binding.uploaded_bytes
            << ",bind_readback_bytes=" << binding.readback_bytes
            << ",preparation_visits=" << binding.preparation_visits
            << ",work=" << q.required_work << ",refs=" << q.references
            << ",S=" << l[Budget::ScratchBytes]
            << ",bindings=" << s.binding_bytes << ",cpp_new=0 PASS\n";
}
void rejections(engine::Runtime &r, HostPath path) {
  auto l = limits(r, false);
  Harness h(r, PreparedCapacity(l), path);
  const auto &s = h.prepared_storage();
  auto src = fixture(8, l[Budget::Fields], false);
  PreparedFixtureAccess::Input input(l[Budget::Particles], l[Budget::Fields],
                                     l[Budget::Batch], l[Budget::Focus]);
  PreparedFixtureAccess::load(h, src, input, inert(), true);
  h.tick(inert());
  h.inspect_prepared();
  const Snapshot before(h);
  auto good = hierarchy(input, 8, 3);
  const auto reject = [&](Hierarchy bad, CascadeStatus expected) {
    CascadeBinding b;
    no_new([&] { b = h.bind_centroid_cascade(s.generation, bad.spec()); });
    require(b.status == expected,
            "hierarchy/reference rejection misclassified");
    before.unchanged(h);
    require(
        h.query_centroid_cascade(s.generation).status == CascadeStatus::Stale,
        "failed rebind retained a partial hierarchy");
  };
  auto bad = good;
  bad.nodes[0].retained_fraction_bits = -1;
  reject(bad, CascadeStatus::UnsupportedProfile);
  bad = good;
  bad.nodes[0].calculation = engine::numeric::Real::F32;
  reject(bad, CascadeStatus::UnsupportedProfile);
  bad = good;
  bad.nodes[0].field.context++;
  reject(bad, CascadeStatus::InvalidReference);
  bad = good;
  bad.cohorts[0].incarnation++;
  reject(bad, CascadeStatus::InvalidReference);
  bad = good;
  bad.nodes[0].cohort_count = 0;
  reject(bad, CascadeStatus::InvalidHierarchy);
  bad = good;
  std::swap(bad.cohorts[0], bad.cohorts[1]);
  reject(bad, CascadeStatus::InvalidHierarchy);
  bad = good;
  std::swap(bad.children[0], bad.children[1]);
  reject(bad, CascadeStatus::InvalidHierarchy);
  bad = good;
  bad.nodes[4].level = 0;
  reject(bad, CascadeStatus::InvalidHierarchy);
  bad = good;
  bad.children[0] = 4;
  reject(bad, CascadeStatus::InvalidHierarchy);
  bad = good;
  bad.nodes.back().publish = false;
  reject(bad, CascadeStatus::InvalidHierarchy);
  bad = good;
  bad.nodes[4].comparison = bad.nodes[0].comparison;
  reject(bad, CascadeStatus::InvalidHierarchy);
  bad = hierarchy(input, 8, 4);
  bad.nodes[0].retained_fraction_bits = 50;
  reject(bad, CascadeStatus::InvalidHierarchy);
  Hierarchy overlap;
  overlap.add(input, 6, 1, 0, true, 52, {0, 1});
  overlap.add(input, 6, 2, 0, true, 52, {1, 2});
  overlap.add(input, 6, 3, 1, true, 52, {0, 1, 2}, {0, 1});
  reject(overlap, CascadeStatus::InvalidHierarchy);
  auto aliases = hierarchy(input, 8, 1);
  aliases.cohorts.push_back(input.particle_rows[0]);
  ++aliases.nodes[0].cohort_count;
  auto bound = h.bind_centroid_cascade(s.generation, aliases.spec());
  require(bound.status == CascadeStatus::Ok && bound.aliases == 1 &&
              bound.references == 8,
          "membership alias was not coalesced");
  auto q = h.query_centroid_cascade(s.generation);
  before.unchanged(h);
  const auto &borrowed = h.inspect_centroid_cascade(q);
  const CascadeSpec borrowed_spec{borrowed.nodes.data(),    1,
                                  borrowed.children.data(), 0,
                                  borrowed.cohorts.data(),  8};
  require(h.bind_centroid_cascade(s.generation, borrowed_spec).status ==
              CascadeStatus::InvalidHierarchy,
          "mutable inspection accepted as binding input");
  auto child_aliases = hierarchy(input, 8, 2);
  child_aliases.children.push_back(0);
  ++child_aliases.nodes.back().child_count;
  bound = h.bind_centroid_cascade(s.generation, child_aliases.spec());
  require(bound.status == CascadeStatus::Ok && bound.aliases == 1 &&
              bound.links == 2,
          "child alias was not coalesced");
  q = h.query_centroid_cascade(s.generation);
  require(q.status == CascadeStatus::Ok && q.mass == 13,
          "child alias changed physical mass");
  before.unchanged(h);
  bound = h.bind_centroid_cascade(s.generation, good.spec(), 0);
  require(bound.status == CascadeStatus::CapacityExhausted &&
              bound.failure.budget == Budget::CascadeWork &&
              bound.failure.required == 1,
          "preparation work bound failed");
  rejects([&] { h.inspect_centroid_cascade(q); });
  before.unchanged(h);
  require(h.bind_centroid_cascade(s.generation + 1, good.spec()).status ==
              CascadeStatus::Stale,
          "stale source generation bound");
  // Distinct source ordinals remain distinct rows. This first component rejects
  // their unsupported repeated participation instead of silently deduplicating.
  UpdateCommand duplicate;
  duplicate.operation = UpdateOp::InsertParticipation;
  duplicate.first = UpdateRef::existing(input.particle_rows[0]);
  duplicate.second = UpdateRef::existing(input.field_rows[6]);
  duplicate.position = s.live.edges;
  duplicate.ordinal = 1000;
  const auto duplicate_row = s.live.edges;
  require(
      h.update_prepared(s.generation, &duplicate, 1).status == UpdateStatus::Ok,
      "distinct source ordinal fixture failed");
  h.inspect_prepared();
  PreparedFixtureAccess::read_relationships(h);
  const Snapshot with_duplicate(h);
  require(h.bind_centroid_cascade(s.generation, good.spec()).status ==
              CascadeStatus::UnsupportedParticipation,
          "distinct source ordinals silently coalesced");
  with_duplicate.unchanged(h);
  duplicate.operation = UpdateOp::RemoveParticipation;
  duplicate.row = duplicate_row;
  duplicate.generation = s.generation;
  require(
      h.update_prepared(s.generation, &duplicate, 1).status == UpdateStatus::Ok,
      "distinct source ordinal restoration failed");
  require(h.bind_centroid_cascade(s.generation, good.spec()).status ==
              CascadeStatus::Ok,
          "numeric rejection fixture bind failed");
  PreparedFixtureAccess::inject_position_value(
      h, input.particle_rows[0].record,
      std::numeric_limits<float>::quiet_NaN());
  h.inspect_prepared();
  const Snapshot nonfinite(h);
  const auto invalid = h.query_centroid_cascade(s.generation);
  require(invalid.status == CascadeStatus::InvalidNumerics &&
              invalid.synchronizations == 1 && invalid.node_reductions == 0,
          "nonfinite current source was reduced");
  rejects([&] { h.inspect_centroid_cascade(invalid); });
  nonfinite.unchanged(h);
  require(h.bind_centroid_cascade(s.generation, good.spec()).status ==
              CascadeStatus::InvalidNumerics,
          "nonfinite source binding succeeded");
  nonfinite.unchanged(h);
  PreparedFixtureAccess::inject_position_value(
      h, input.particle_rows[0].record,
      before.particles[kPosX * h.particle_capacity() +
                       input.particle_rows[0].record]);
  // A source edit invalidates the hierarchy even when the edited field is not
  // among its contexts; explicit rebind is required after any topology change.
  bound = h.bind_centroid_cascade(s.generation, good.spec());
  q = h.query_centroid_cascade(s.generation);
  UpdateCommand edit;
  edit.operation = UpdateOp::ReplaceParticipation;
  edit.generation = s.generation;
  edit.row = 24;
  edit.first = UpdateRef::existing(input.particle_rows[0]);
  edit.second = UpdateRef::existing(input.field_rows[7]);
  edit.share = .5f;
  edit.ordinal = 24;
  require(h.update_prepared(s.generation, &edit, 1).status == UpdateStatus::Ok,
          "source edit fixture failed");
  rejects([&] { h.inspect_centroid_cascade(q); });
  require(h.query_centroid_cascade(s.generation).status == CascadeStatus::Stale,
          "source edit failed to stale binding");
  require(h.bind_centroid_cascade(s.generation, good.spec()).status ==
              CascadeStatus::Ok,
          "explicit rebind after source edit failed");
  auto partial = hierarchy(input, 8, 1);
  partial.nodes[0].field = input.field_rows[7];
  require(h.bind_centroid_cascade(s.generation, partial.spec()).status ==
              CascadeStatus::UnsupportedParticipation,
          "fractional participation accepted");
  // Binding guard failures are unexpected/unready, distinct from ordinary
  // resource/input rejections. No simulated hardware recovery is claimed.
  require(h.bind_centroid_cascade(s.generation, good.spec()).status ==
              CascadeStatus::Ok,
          "final rebind failed");
  h.particle_data.push_back(0);
  rejects([&] { h.query_centroid_cascade(s.generation); });
  require(!h.ready(), "binding failure left execution healthy");
  std::cout << "cascade aliases, invalid hierarchies/profiles/references, "
               "work/stale/source edits and unready guard PASS\n";
}
void capacities(engine::Runtime &r, HostPath path) {
  for (Budget budget :
       {Budget::CascadeNodes, Budget::CascadeLinks, Budget::CascadeReferences,
        Budget::CascadeLevels, Budget::References}) {
    auto l = limits(r, false);
    if (budget == Budget::CascadeNodes)
      l[budget] = 6;
    if (budget == Budget::CascadeLinks)
      l[budget] = 5;
    if (budget == Budget::CascadeReferences)
      l[budget] = 23;
    if (budget == Budget::CascadeLevels)
      l[budget] = 2;
    if (budget == Budget::References)
      l[budget] = 118;
    l[Budget::ScratchBytes] =
        detail::PreparedStorage::required_scratch(l, false);
    Harness h(r, PreparedCapacity(l), path);
    const auto src = fixture(8, l[Budget::Fields], false);
    PreparedFixtureAccess::Input input(l[Budget::Particles], l[Budget::Fields],
                                       l[Budget::Batch], l[Budget::Focus]);
    PreparedFixtureAccess::load(h, src, input, inert(), true);
    h.inspect_prepared();
    const Snapshot before(h);
    auto request = hierarchy(input, 8, 3);
    CascadeBinding b;
    no_new([&] {
      b = h.bind_centroid_cascade(h.prepared_storage().generation,
                                  request.spec());
    });
    require(b.status == CascadeStatus::CapacityExhausted &&
                b.failure.budget == budget,
            "independent cascade budget exhaustion failed");
    before.unchanged(h);
  }
  auto l = limits(r, false);
  --l[Budget::ScratchBytes];
  const auto ids = r.issued_snode_ids();
  bool rejected = false;
  try {
    Harness h(r, PreparedCapacity(l), path);
  } catch (const CapacityError &e) {
    rejected = e.failure.budget == Budget::ScratchBytes;
  }
  require(rejected && ids == r.issued_snode_ids(),
          "scratch rejected after layout mutation");
  l = limits(r, false);
  for (Budget b :
       {Budget::CascadeNodes, Budget::CascadeLinks, Budget::CascadeReferences,
        Budget::CascadeLevels, Budget::CascadeWork})
    l[b] = 0;
  l[Budget::ScratchBytes] = detail::PreparedStorage::required_scratch(l, false);
  Harness disabled(r, PreparedCapacity(l), path);
  require(
      disabled.query_centroid_cascade(0).status == CascadeStatus::Unsupported,
      "unreserved cascade executed");
  std::cout << "cascade independent N/H/R/L/global-R/S exhaustion and disabled "
               "path PASS\n";
}
void units(engine::Runtime &r, HostPath path) {
  using namespace assembly;
  using namespace taichi::lang;
  auto root = std::make_unique<SNode>(0, SNodeType::root);
  auto &primary = root->dense(Axis(0), 1);
  auto &compressed = primary.dense(Axis(1), 1);
  auto &named = compressed.dense(Axis(2), 1);
  StudyInput input;
  input.capacity = 2;
  input.nodes = {{root.get(), -1, ScaleRole::Root},
                 {&primary, 0, ScaleRole::PrimaryStep},
                 {&compressed, 1, ScaleRole::SecondaryCompression},
                 {&named, 2, ScaleRole::NamingEquivalent}};
  input.definitions = {{"same", 1, {}, {{{}, 1}}, {}}};
  input.occurrences = {{"a", "same", 0, 0, {1, 0, 0}, {}},
                       {"b", "same", 1, 3, {1, 0, 0}, {}}};
  const auto study = prepare(input);
  require(study.particles[0].position.x == 1 &&
              study.particles[1].position.x == 64 &&
              study.scales[1].units == study.scales[3].units,
          "source transition/compression fixture incorrect");
  auto l = limits(r, false);
  Harness h(r, PreparedCapacity(l), path);
  FixtureSource src;
  src.kinds = {1};
  for (const auto &p : study.particles) {
    std::array<float, kParticleFieldCount> body{};
    body[kPosX] = float(p.position.x);
    body[kMass] = 1;
    src.particles.push_back(body);
  }
  src.edges = {{0, 0, 1, 0}, {1, 0, 1, 1}};
  PreparedFixtureAccess::Input map(l[Budget::Particles], l[Budget::Fields],
                                   l[Budget::Batch], l[Budget::Focus]);
  PreparedFixtureAccess::load(h, src, map, inert(), true);
  Hierarchy request;
  request.add(map, 0, 1, 0, false, 52, {0});
  request.add(map, 0, 1, 0, false, 52, {1});
  request.add(map, 0, 1, 0, true, 52, {0, 1}, {0, 1});
  const auto b =
      h.bind_centroid_cascade(h.prepared_storage().generation, request.spec());
  const auto q = h.query_centroid_cascade(h.prepared_storage().generation);
  require(b.status == CascadeStatus::Ok && q.status == CascadeStatus::Ok &&
              q.centre[0] == 32.5 && q.mass == 2,
          "internal grouping applied another distance factor");
  std::cout << "cascade declared source units once, same-LoD internal grouping "
               "and 1:1 compression PASS\n";
}
}  // namespace
int main(int argc, char **argv) {
  try {
    std::cout << std::setprecision(17);
    bool direct = false, large = false, maximum = false;
    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--direct")
        direct = true;
      else if (arg == "--large")
        large = true;
      else if (arg == "--maximum")
        maximum = true;
      else
        throw std::invalid_argument("expected --direct --large --maximum");
    }
    engine::InstanceSettings settings;
    settings.read_environment = false;
    settings.cpu_threads = 2;
    engine::Runtime runtime(settings);
    require(!runtime.on_cuda(), "CPU fixture unexpectedly selected CUDA");
    std::cout << "actual_backend=cpu,path=" << (direct ? "direct" : "bulk")
              << '\n';
    const auto path = direct ? HostPath::CpuDirect : HostPath::BulkCopy;
    run(runtime, path, large, maximum);
    if (!large && !maximum) {
      rejections(runtime, path);
      capacities(runtime, path);
      units(runtime, path);
    }
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
