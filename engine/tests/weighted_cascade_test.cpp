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
using engine::RecordHandle;
bool same(const RecordHandle &a, const RecordHandle &b) {
  return a.context == b.context && a.incarnation == b.incarnation &&
         a.address == b.address && a.record == b.record;
}
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
  std::vector<RecordHandle> particle_headers, field_headers, focal;
  std::vector<detail::RelationshipHeader> participation, contact_headers;
  std::vector<std::uint64_t> masses, counts;
  FrameAnchor anchor;
  std::array<float, 11> focus;
  std::array<unsigned char, sizeof(Demand)> demand;
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
        focal(h.prepared_storage().focal),
        participation(h.prepared_storage().edges),
        contact_headers(h.prepared_storage().contacts),
        masses(h.prepared_storage().source_masses),
        counts(h.prepared_storage().source_counts),
        anchor(h.frame_anchor()),
        focus(h.prepared_storage().inspection_focus),
        live(h.prepared_storage().live),
        generation(h.prepared_storage().generation),
        ticks(h.prepared_storage().completed_ticks) {
    std::memcpy(demand.data(), &h.demand(), demand.size());
  }
  void physical(Harness &h) const {
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
                anchor == h.frame_anchor() &&
                !std::memcmp(focus.data(), s.inspection_focus.data(),
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
Parameters inert() {
  Parameters p;
  p.gravity = 0;
  p.contact_stiffness = 0;
  p.reach_strength = 0;
  return p;
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
BudgetDemand limits(bool large) {
  BudgetDemand l;
  const std::uint64_t p = large ? 4096 : 70, g = large ? 257 : 9;
  l.values = {
      large ? 16777216u : 1048576u, p, g, 8 * p, 2 * p, 32 * p + g + 14, g,
      large ? 256u : 32u,           0};
  l[Budget::CascadeNodes] = 7;
  l[Budget::CascadeLinks] = 6;
  l[Budget::CascadeReferences] = 8 * p;
  l[Budget::CascadeCoverage] = 7 * p;
  l[Budget::CascadeLevels] = 3;
  l[Budget::CascadeWork] = std::uint64_t{1} << 40;
  l[Budget::ScratchBytes] = detail::PreparedStorage::required_scratch(l, false);
  return l;
}
FixtureSource fixture(int n, int groups, bool moving) {
  FixtureSource src;
  src.kinds.resize(groups);
  for (int g = 0; g < groups; ++g)
    src.kinds[g] = 1 + g % 3;
  src.exact_masses.resize(n, 24);
  for (int i = 0; i < n; ++i) {
    std::array<float, kParticleFieldCount> p{};
    p[kPosX] = float(i * 4);
    p[kPosY] = float((i % 3) - 1);
    p[kMass] = 1 + i % 7;
    p[kRadius] = .25f;
    src.particles.push_back(p);
  }
  if (n == 8) {
    const float x[] = {
        1, std::nextafter(1.f, 2.f), 1.8f, 1.9f, -1.8f, -1.9f, 64, 1};
    for (int i = 0; i < n; ++i)
      src.particles[i][kPosX] = x[i];
  }
  if (moving)
    src.particles[0][kVelX] = .125f;
  for (int g = 0; g < 7; ++g)
    for (int i = 0; i < n; ++i) {
      const std::uint64_t base[] = {2, 6, 4, 12};
      std::uint64_t c[4];
      for (int j = 0; j < 4; ++j)
        c[j] = base[(i + j) % 4];
      const auto add = [&](std::uint64_t count) {
        src.edges.push_back(
            {i, g, float(double(count) / 24), src.edges.size(), count});
      };
      if (g == 3) {
        add(1);
        add(c[3] - 1);
      } else
        add(g < 3 ? c[g] : g == 4 ? c[0] + c[1] : g == 5 ? c[2] + c[3] : 24);
    }
  src.focus = {6};
  return src;
}
struct Hierarchy {
  std::vector<CascadeNode> nodes;
  std::vector<int> children;
  std::vector<CascadeContribution> rows;
  void add(const Harness &h,
           int field,
           std::uint64_t context,
           int level,
           bool publish,
           const std::vector<int> &selected,
           const std::vector<int> &parts = {}) {
    const auto &s = h.prepared_storage();
    CascadeNode n;
    n.field = s.field_headers[field];
    n.comparison = context;
    n.level = level;
    n.publish = publish;
    n.cohort_begin = rows.size();
    n.cohort_count = selected.size();
    n.child_begin = children.size();
    n.child_count = parts.size();
    for (int row : selected)
      rows.push_back(
          {s.edges[row].first,
           {s.generation, std::uint64_t(row), s.edges[row].ordinal}});
    children.insert(children.end(), parts.begin(), parts.end());
    nodes.push_back(n);
  }
  WeightedCascadeSpec spec() const {
    return {nodes.data(),    nodes.size(), children.data(),
            children.size(), rows.data(),  rows.size()};
  }
};
std::vector<int> source_rows(const Harness &h,
                             RecordHandle field,
                             int parity = -1) {
  const auto &s = h.prepared_storage();
  std::vector<int> rows;
  int index = 0;
  for (std::size_t e = 0; e < s.live.edges; ++e)
    if (same(s.edges[e].second, field)) {
      if (parity < 0 || index % 2 == parity)
        rows.push_back(int(e));
      ++index;
    }
  return rows;
}
Hierarchy hierarchy(const Harness &h,
                    const PreparedFixtureAccess::Input &in,
                    int shape) {
  Hierarchy t;
  const auto add = [&](int g, int context, int level, bool publish,
                       const std::vector<int> &parts = {}, int parity = -1) {
    t.add(h, in.field_rows[g].record, context, level, publish,
          source_rows(h, in.field_rows[g], parity), parts);
  };
  if (shape == 1)
    add(6, 100, 0, true);
  if (shape == 2) {
    add(0, 100, 0, true);
    add(1, 101, 0, true);
    add(4, 104, 2, true, {0, 1});
  }
  if (shape == 3) {
    for (int g = 0; g < 4; ++g)
      add(g, 100 + g, 0, true);
    add(4, 104, 1, true, {0, 1});
    add(5, 105, 1, true, {2, 3});
    add(6, 106, 2, true, {4, 5});
  }
  if (shape == 4) {
    add(3, 100, 0, false, {}, 0);
    add(3, 100, 0, false, {}, 1);
    add(3, 100, 0, true, {0, 1});
  }
  return t;
}
void oracle(const Harness &h,
            const CascadeResult &q,
            const CascadeInspection &v,
            const Snapshot &physical) {
  const auto &s = h.prepared_storage();
  const int pstride = h.particle_capacity(),
            nstride = int(s.capacity.limits()[Budget::CascadeNodes]);
  require(q.weighted && q.coverage_entries > 0,
          "weighted result lost coverage provenance");
  for (std::size_t node = 0; node < q.nodes; ++node) {
    const auto &def = v.nodes[node];
    // Independent exact accounting, keyed by full occurrence handle. Distinct
    // source ordinals contribute separately, even to the same occurrence.
    std::vector<CascadeCoverage> exact;
    for (auto k = def.cohort_begin; k < def.cohort_begin + def.cohort_count;
         ++k) {
      const auto &source = v.source_rows[k];
      const auto ref = v.cohorts[k];
      require(source.generation == s.generation && source.row < s.live.edges &&
                  same(s.edges[source.row].first, ref) &&
                  same(s.edges[source.row].second, def.field) &&
                  s.edges[source.row].ordinal == source.ordinal,
              "inspection source identity mismatch");
      auto i = std::find_if(exact.begin(), exact.end(), [&](const auto &e) {
        return same(e.occurrence, ref);
      });
      if (i == exact.end()) {
        exact.push_back({ref, 0, s.source_masses[ref.record]});
        i = exact.end() - 1;
      }
      require(s.source_counts[source.row] <= i->denominator - i->count,
              "oracle oversubscription");
      i->count += s.source_counts[source.row];
    }
    require(v.coverage_offsets[node + 1] - v.coverage_offsets[node] ==
                int(exact.size()),
            "coverage CSR size mismatch");
    for (std::size_t j = 0; j < exact.size(); ++j) {
      const auto &got = v.coverage[v.coverage_offsets[node] + j],
                 &want = exact[j];
      require(same(got.occurrence, want.occurrence) &&
                  got.count == want.count &&
                  got.denominator == want.denominator,
              "exact coverage mismatch");
      if (def.child_count) {
        std::uint64_t sum = 0;
        for (auto k = def.child_begin; k < def.child_begin + def.child_count;
             ++k) {
          const int child = v.children[k];
          for (int c = v.coverage_offsets[child];
               c < v.coverage_offsets[child + 1]; ++c)
            if (same(v.coverage[c].occurrence, want.occurrence)) {
              require(v.coverage[c].count <= want.count - sum,
                      "oracle child oversubscription");
              sum += v.coverage[c].count;
            }
        }
        require(sum == want.count, "parent/child integer coverage differs");
      }
    }
    long double mass = 0;
    std::array<long double, 3> sum{}, maximum{};
    int count = 0;
    const auto add = [&](long double m, const std::array<long double, 3> &x) {
      mass += m;
      ++count;
      for (int a = 0; a < 3; ++a) {
        sum[a] += m * x[a];
        maximum[a] = std::max(maximum[a], std::fabs(x[a]));
      }
    };
    if (def.child_count) {
      for (auto k = def.child_begin; k < def.child_begin + def.child_count;
           ++k) {
        int c = v.children[k];
        std::array<long double, 3> x;
        for (int a = 0; a < 3; ++a)
          x[a] = v.values[(3 + a) * nstride + c];
        add(v.values[6 * nstride + c], x);
      }
    } else
      for (auto k = def.cohort_begin; k < def.cohort_begin + def.cohort_count;
           ++k) {
        const int row = v.cohorts[k].record;
        const auto e = v.source_rows[k].row;
        std::array<long double, 3> x;
        for (int a = 0; a < 3; ++a)
          x[a] = physical.particles[(kPosX + a) * pstride + row];
        const long double m = static_cast<long double>(
                                  physical.particles[kMass * pstride + row]) *
                              static_cast<long double>(s.source_counts[e]) /
                              static_cast<long double>(s.source_masses[row]);
        add(m, x);
      }
    const auto epsilon = std::numeric_limits<double>::epsilon();
    require(
        v.valid[node] == 1 && std::fabs(v.values[6 * nstride + node] - mass) <=
                                  8 * (count + 1) * epsilon * mass,
        "weighted participating mass disagrees");
    for (int a = 0; a < 3; ++a) {
      const double trial = v.values[a * nstride + node],
                   published = v.values[(3 + a) * nstride + node];
      require(std::fabs(trial - sum[a] / mass) <=
                  16 * (count + 1) * epsilon * std::max(1.L, maximum[a]),
              "hierarchical weighted centre disagrees");
      const double want =
          def.publish ? quantize(trial, def.retained_fraction_bits) : trial;
      require(!std::memcmp(&want, &published, sizeof(want)),
              "publication boundary/identity incorrect");
    }
  }
  require(q.frame_anchor == physical.anchor &&
              q.mass == v.values[6 * nstride + q.root] &&
              q.centre[0] == v.values[3 * nstride + q.root],
          "root summary disagrees");
}
void refresh(Harness &h) {
  h.inspect_prepared();
  PreparedFixtureAccess::read_relationships(h);
}
void run(engine::Runtime &r, HostPath path, bool large, bool maximum) {
  const auto l = limits(large);
  const auto start = std::chrono::steady_clock::now();
  Harness h(r, PreparedCapacity(l), path);
  const double setup_us = std::chrono::duration<double, std::micro>(
                              std::chrono::steady_clock::now() - start)
                              .count();
  const auto &s = h.prepared_storage();
  const WeightedCascadeSpec empty_spec{};
  require(h.bind_weighted_centroid_cascade(s.generation, empty_spec).status ==
              CascadeStatus::Empty,
          "empty weighted bind failed");
  auto q = h.query_centroid_cascade(s.generation);
  require(
      q.status == CascadeStatus::Empty && q.root == -1 && !q.mass && q.weighted,
      "empty weighted query fabricated data");
  h.inspect_centroid_cascade(q);
  auto src = fixture(maximum ? int(l[Budget::Particles]) : 8,
                     int(l[Budget::Fields]), !maximum);
  PreparedFixtureAccess::Input in(l[Budget::Particles], l[Budget::Fields],
                                  l[Budget::Batch], l[Budget::Focus]);
  prepared_test::Metrics metrics(r, PreparedFixtureAccess::nodes(h));
  const auto prepared = metrics.sample();
  PreparedFixtureAccess::load(h, src, in, inert(), true);
  refresh(h);
  const Snapshot before(h);
  auto tree = hierarchy(h, in, 3);
  CascadeBinding b;
  auto measured = std::chrono::steady_clock::now();
  no_new(
      [&] { b = h.bind_weighted_centroid_cascade(s.generation, tree.spec()); });
  const double bind_us = std::chrono::duration<double, std::micro>(
                             std::chrono::steady_clock::now() - measured)
                             .count();
  require(b.status == CascadeStatus::Ok && b.weighted && b.nodes == 7 &&
              b.links == 6 && b.levels == 3 &&
              b.references == 8 * src.particles.size() &&
              b.coverage_entries == 7 * src.particles.size(),
          "weighted hierarchy bind failed");
  if (path == HostPath::BulkCopy) {
    std::fill(h.particle_data.begin(), h.particle_data.end(), -12345);
    std::fill(h.group_data.begin(), h.group_data.end(), -12345);
    std::fill(h.edge_int_data.begin(), h.edge_int_data.end(), -1);
    std::fill(h.edge_float_data.begin(), h.edge_float_data.end(), -.5f);
  }
  const CascadeInspection *v;
  measured = std::chrono::steady_clock::now();
  no_new([&] { q = h.query_centroid_cascade(s.generation); });
  const double query_us = std::chrono::duration<double, std::micro>(
                              std::chrono::steady_clock::now() - measured)
                              .count();
  measured = std::chrono::steady_clock::now();
  no_new([&] { v = &h.inspect_centroid_cascade(q); });
  const double inspection_us = std::chrono::duration<double, std::micro>(
                                   std::chrono::steady_clock::now() - measured)
                                   .count();
  require(q.status == CascadeStatus::Ok &&
              q.required_work == 13 * src.particles.size() + 20 &&
              q.reference_visits == 8 * src.particles.size() &&
              q.reduction_visits == 5 * src.particles.size() + 6 &&
              q.node_reductions == 7 && q.readback_bytes == 184 &&
              q.synchronizations == 2,
          "weighted query visits/transfer mismatch");
  oracle(h, q, *v, before);
  before.unchanged(h);
  require(prepared.same_backing(metrics.sample()),
          "first weighted bind/query changed backing/layout/compiler");
  if (!maximum) {
    for (int shape : {1, 2, 4}) {
      auto t = hierarchy(h, in, shape);
      require(h.bind_weighted_centroid_cascade(s.generation, t.spec()).status ==
                  CascadeStatus::Ok,
              "alternate weighted shape rejected");
      q = h.query_centroid_cascade(s.generation);
      v = &h.inspect_centroid_cascade(q);
      oracle(h, q, *v, before);
      before.unchanged(h);
    }
    // Every legal experimental Q profile; parents read published child values.
    for (int bits = 0; bits <= 52; ++bits) {
      tree.nodes[0].retained_fraction_bits = bits;
      no_new([&] {
        b = h.bind_weighted_centroid_cascade(s.generation, tree.spec());
      });
      require(b.status == CascadeStatus::Ok,
              "legal weighted precision rejected");
      no_new([&] {
        q = h.query_centroid_cascade(s.generation);
        v = &h.inspect_centroid_cascade(q);
      });
      oracle(h, q, *v, before);
    }
    tree.nodes[0].retained_fraction_bits = 4;
    require(
        h.bind_weighted_centroid_cascade(s.generation, tree.spec()).status ==
            CascadeStatus::Ok,
        "trace bind failed");
    for (float x : {1.5f, std::nextafter(1.5f, 2.f), 1.5625f, -1.5f,
                    std::nextafter(-1.5f, -2.f), -1.5625f}) {
      for (int i = 0; i < 8; ++i)
        PreparedFixtureAccess::inject_position_value(
            h, in.particle_rows[i].record, x);
      const Snapshot current(h);
      for (int repeat = 0; repeat < 3; ++repeat) {
        no_new([&] {
          q = h.query_centroid_cascade(s.generation);
          v = &h.inspect_centroid_cascade(q);
        });
        oracle(h, q, *v, current);
        require(v->values[3 * l[Budget::CascadeNodes]] == quantize(x, 4),
                "discarded weighted tail recovered");
      }
      current.unchanged(h);
      std::cout << "weighted_publication_trace,input=" << x
                << ",published=" << v->values[3 * l[Budget::CascadeNodes]]
                << '\n';
    }
    const auto needed = q.required_work;
    require(h.query_centroid_cascade(s.generation, needed - 1).status ==
                CascadeStatus::CapacityExhausted,
            "query work bound ignored");
    rejects([&] { h.inspect_centroid_cascade(q); });
  }
  double reuse_us = 0;
  for (int cycle = 0; cycle < 32; ++cycle) {
    h.tick(inert());
    refresh(h);
    const Snapshot current(h);
    measured = std::chrono::steady_clock::now();
    no_new([&] {
      q = h.query_centroid_cascade(s.generation);
      v = &h.inspect_centroid_cascade(q);
    });
    reuse_us += std::chrono::duration<double, std::micro>(
                    std::chrono::steady_clock::now() - measured)
                    .count();
    require(q.status == CascadeStatus::Ok &&
                (maximum || q.awake_reference_visits == 8),
            "sleepers/current motion distinction failed");
    oracle(h, q, *v, current);
    current.unchanged(h);
  }
  require(prepared.same_backing(metrics.sample()),
          "weighted reuse changed backing/layout/compiler");
  prepared.print("after_setup");
  metrics.sample().print("after_weighted_cycles");
  for (const auto &e : s.manifest)
    std::cout << "manifest," << e.name << ',' << e.count << ','
              << e.element_bytes << ',' << e.copies << ',' << e.bytes << '\n';
  std::cout << "weighted_cascade," << l[Budget::Particles] << ','
            << (path == HostPath::BulkCopy ? "bulk" : "direct") << ','
            << (maximum ? "maximum" : "holes/motion/precision")
            << ",setup_us=" << setup_us << ",bind_us=" << bind_us
            << ",first_query_us=" << query_us
            << ",first_inspection_us=" << inspection_us
            << ",repeat_query_inspection_mean_us=" << reuse_us / 32
            << ",bind_uploaded_bytes=" << b.uploaded_bytes
            << ",bind_readback_bytes=" << b.readback_bytes
            << ",preparation_visits=" << b.preparation_visits
            << ",work=" << q.required_work << ",refs=" << q.references
            << ",coverage=" << q.coverage_entries
            << ",S=" << l[Budget::ScratchBytes]
            << ",bindings=" << s.binding_bytes << ",cpp_new=0 PASS\n";
}
void rejections(engine::Runtime &r, HostPath path) {
  const auto l = limits(false);
  Harness h(r, PreparedCapacity(l), path);
  const auto &s = h.prepared_storage();
  auto src = fixture(8, int(l[Budget::Fields]), false);
  src.edges[0].exact_count = 0;
  PreparedFixtureAccess::Input in(l[Budget::Particles], l[Budget::Fields],
                                  l[Budget::Batch], l[Budget::Focus]);
  PreparedFixtureAccess::load(h, src, in, inert(), true);
  refresh(h);
  auto good = hierarchy(h, in, 3);
  {
    const Snapshot before(h);
    require(
        h.bind_weighted_centroid_cascade(s.generation, good.spec()).status ==
            CascadeStatus::UnsupportedAccounting,
        "unknown counts guessed from source share");
    before.unchanged(h);
  }
  UpdateCommand edit;
  edit.operation = UpdateOp::ReplaceParticipation;
  edit.generation = s.generation;
  edit.row = 0;
  edit.ordinal = s.edges[0].ordinal;
  edit.first = UpdateRef::existing(s.edges[0].first);
  edit.second = UpdateRef::existing(s.edges[0].second);
  edit.share = s.edges[0].share;
  edit.exact_count = 2;
  require(h.update_prepared(s.generation, &edit, 1).status == UpdateStatus::Ok,
          "known count restoration failed");
  good = hierarchy(h, in, 3);
  refresh(h);
  const Snapshot before(h);
  const auto reject = [&](const Hierarchy &bad, CascadeStatus expected) {
    require(
        h.bind_weighted_centroid_cascade(s.generation, good.spec()).status ==
            CascadeStatus::Ok,
        "rejection baseline bind failed");
    const auto old = h.query_centroid_cascade(s.generation);
    CascadeBinding b;
    no_new([&] {
      b = h.bind_weighted_centroid_cascade(s.generation, bad.spec());
    });
    if (b.status != expected)
      std::cerr << "expected=" << int(expected) << ",actual=" << int(b.status)
                << '\n';
    require(b.status == expected, "weighted rejection misclassified");
    rejects([&] { h.inspect_centroid_cascade(old); });
    require(
        h.query_centroid_cascade(s.generation).status == CascadeStatus::Stale,
        "failed weighted bind retained hierarchy");
    before.unchanged(h);
  };
  auto bad = good;
  bad.rows[0].source.generation--;
  reject(bad, CascadeStatus::Stale);
  bad = good;
  bad.rows[0].source.row = s.live.edges;
  reject(bad, CascadeStatus::InvalidReference);
  bad = good;
  bad.rows[0].source.ordinal++;
  reject(bad, CascadeStatus::InvalidReference);
  bad = good;
  bad.rows[0].occurrence.context++;
  reject(bad, CascadeStatus::InvalidReference);
  bad = good;
  bad.rows[0].occurrence.incarnation++;
  reject(bad, CascadeStatus::InvalidReference);
  bad = good;
  bad.rows[0].occurrence = in.particle_rows[1];
  reject(bad, CascadeStatus::InvalidReference);
  bad = good;
  bad.nodes[0].field = in.field_rows[1];
  reject(bad, CascadeStatus::InvalidReference);
  bad = good;
  std::swap(bad.rows[0], bad.rows[1]);
  reject(bad, CascadeStatus::InvalidHierarchy);
  bad = good;
  std::swap(bad.children[0], bad.children[1]);
  reject(bad, CascadeStatus::InvalidHierarchy);
  bad = good;
  bad.children[0] = 4;
  reject(bad, CascadeStatus::InvalidHierarchy);
  bad = good;
  bad.nodes[4].level = 0;
  reject(bad, CascadeStatus::InvalidHierarchy);
  bad = good;
  bad.nodes[0].retained_fraction_bits = 53;
  reject(bad, CascadeStatus::UnsupportedProfile);
  bad = good;
  bad.nodes[0].storage = engine::numeric::Real::F32;
  reject(bad, CascadeStatus::UnsupportedProfile);
  bad = good;
  bad.nodes.back().publish = false;
  reject(bad, CascadeStatus::InvalidHierarchy);
  bad = good;
  --bad.nodes.back().cohort_count;
  reject(bad, CascadeStatus::InvalidCoverage);
  bad = good;
  bad.nodes[4].field = in.field_rows[5];
  const auto replacement = source_rows(h, in.field_rows[5]);
  for (std::size_t i = 0; i < replacement.size(); ++i) {
    const auto e = replacement[i];
    bad.rows[bad.nodes[4].cohort_begin + i] = {
        s.edges[e].first, {s.generation, std::uint64_t(e), s.edges[e].ordinal}};
  }
  reject(bad, CascadeStatus::InvalidCoverage);
  bad = hierarchy(h, in, 4);
  bad.nodes[0].retained_fraction_bits = 4;
  reject(bad, CascadeStatus::InvalidHierarchy);
  auto alias = hierarchy(h, in, 1);
  alias.rows.push_back(alias.rows[0]);
  ++alias.nodes[0].cohort_count;
  auto b = h.bind_weighted_centroid_cascade(s.generation, alias.spec());
  require(b.status == CascadeStatus::Ok && b.aliases == 1 &&
              b.references == 8 && b.coverage_entries == 8,
          "same-row alias acquired coverage/mass");
  auto q = h.query_centroid_cascade(s.generation);
  const auto &v = h.inspect_centroid_cascade(q);
  WeightedCascadeSpec borrow{v.nodes.data(),    1, nullptr, 0,
                             alias.rows.data(), 8};
  require(h.bind_weighted_centroid_cascade(s.generation, borrow).status ==
              CascadeStatus::InvalidHierarchy,
          "borrowed node input accepted");
  alias = hierarchy(h, in, 4);
  alias.children.push_back(0);
  ++alias.nodes.back().child_count;
  b = h.bind_weighted_centroid_cascade(s.generation, alias.spec());
  require(b.status == CascadeStatus::Ok && b.aliases == 1 &&
              b.references == 32 && b.coverage_entries == 24,
          "child alias/distinct ordinals coalesced incorrectly");
  q = h.query_centroid_cascade(s.generation);
  oracle(h, q, h.inspect_centroid_cascade(q), before);
  before.unchanged(h);
  b = h.bind_weighted_centroid_cascade(s.generation, good.spec(), 0);
  require(b.status == CascadeStatus::CapacityExhausted &&
              b.failure.budget == Budget::CascadeWork &&
              b.failure.required == 1,
          "bind work budget ignored");
  before.unchanged(h);
  // The original API still accepts only unambiguous whole rows, even in a
  // weighted-enabled context. Fractional or repeated rows remain unsupported.
  auto whole = hierarchy(h, in, 1);
  std::vector<RecordHandle> handles;
  for (const auto &row : whole.rows)
    handles.push_back(row.occurrence);
  CascadeSpec legacy{whole.nodes.data(), 1, nullptr, 0, handles.data(),
                     handles.size()};
  require(
      h.bind_centroid_cascade(s.generation, legacy).status == CascadeStatus::Ok,
      "whole API regressed with weighted resources");
  q = h.query_centroid_cascade(s.generation);
  require(!q.weighted && !q.coverage_entries && q.status == CascadeStatus::Ok,
          "whole API silently became weighted");
  whole.nodes[0].field = in.field_rows[0];
  require(h.bind_centroid_cascade(s.generation, legacy).status ==
              CascadeStatus::UnsupportedParticipation,
          "old fractional rejection changed");
  whole.nodes[0].field = in.field_rows[3];
  require(h.bind_centroid_cascade(s.generation, legacy).status ==
              CascadeStatus::UnsupportedParticipation,
          "old repeated ordinal rejection changed");
  require(h.bind_weighted_centroid_cascade(s.generation, good.spec()).status ==
              CascadeStatus::Ok,
          "numeric baseline failed");
  q = h.query_centroid_cascade(s.generation);
  h.tick(inert());
  rejects([&] { h.inspect_centroid_cascade(q); });
  PreparedFixtureAccess::inject_position_value(
      h, in.particle_rows[0].record, std::numeric_limits<float>::quiet_NaN());
  const Snapshot nan(h);
  auto invalid = h.query_centroid_cascade(s.generation);
  require(invalid.status == CascadeStatus::InvalidNumerics &&
              invalid.synchronizations == 1 && !invalid.node_reductions,
          "nonfinite weighted body reduced");
  rejects([&] { h.inspect_centroid_cascade(invalid); });
  nan.unchanged(h);
  PreparedFixtureAccess::inject_position_value(h, in.particle_rows[0].record,
                                               1);
  // Committed source edits require an explicit fresh row/generation binding.
  require(h.bind_weighted_centroid_cascade(s.generation, good.spec()).status ==
              CascadeStatus::Ok,
          "edit baseline failed");
  q = h.query_centroid_cascade(s.generation);
  edit.generation = s.generation;
  edit.exact_count = 3;
  edit.share = .125f;
  require(h.update_prepared(s.generation, &edit, 1).status == UpdateStatus::Ok,
          "count edit fixture failed");
  rejects([&] { h.inspect_centroid_cascade(q); });
  require(
      h.query_centroid_cascade(s.generation).status == CascadeStatus::Stale &&
          h.bind_weighted_centroid_cascade(s.generation, good.spec()).status ==
              CascadeStatus::Stale,
      "source edit followed stale row descriptors");
  auto single = hierarchy(h, in, 1);
  require(
      h.bind_weighted_centroid_cascade(s.generation, single.spec()).status ==
          CascadeStatus::Ok,
      "fresh generation bind failed");
  // Restore the first source row via remove/insert. Compaction never grants an
  // old row descriptor authority, even if the occurrence and ordinal survive.
  q = h.query_centroid_cascade(s.generation);
  auto old = single;
  UpdateCommand remove;
  remove.operation = UpdateOp::RemoveParticipation;
  remove.generation = s.generation;
  remove.row = 0;
  require(
      h.update_prepared(s.generation, &remove, 1).status == UpdateStatus::Ok,
      "row compaction fixture failed");
  require(h.bind_weighted_centroid_cascade(s.generation, old.spec()).status ==
              CascadeStatus::Stale,
          "compacted rows followed old generation");
  rejects([&] { h.inspect_centroid_cascade(q); });
  single = hierarchy(h, in, 1);
  require(
      h.bind_weighted_centroid_cascade(s.generation, single.spec()).status ==
          CascadeStatus::Ok,
      "source fault baseline failed");
  PreparedFixtureAccess::inject_share_value(h, int(single.rows[0].source.row),
                                            .5f);
  rejects([&] { h.query_centroid_cascade(s.generation); });
  require(!h.ready(),
          "native source contradiction left weighted execution healthy");
  std::cout
      << "weighted exact provenance, unknown counts, aliases/ordinals, overlap "
         "mismatch, legacy isolation, nonfinite/stale/compaction PASS\n";
}
// Test-only integer encoding oracle: round exact positive u64 to nearest-even
// binary64 using integer bits, independently of either native or host casts.
double integer_f64(std::uint64_t n) {
  int width = 0;
  for (auto t = n; t; t >>= 1)
    ++width;
  std::uint64_t significand;
  if (width <= 53)
    significand = n << (53 - width);
  else {
    const int shift = width - 53;
    significand = n >> shift;
    const auto rem = n & ((std::uint64_t{1} << shift) - 1),
               half = std::uint64_t{1} << (shift - 1);
    if (rem > half || (rem == half && (significand & 1)))
      ++significand;
    if (significand == (std::uint64_t{1} << 53)) {
      significand >>= 1;
      ++width;
    }
  }
  const std::uint64_t bits = (std::uint64_t(width - 1 + 1023) << 52) |
                             (significand & ((std::uint64_t{1} << 52) - 1));
  double value;
  std::memcpy(&value, &bits, sizeof(value));
  return value;
}
void wide_and_thirds(engine::Runtime &r, HostPath path) {
  auto l = limits(false);
  Harness h(r, PreparedCapacity(l), path);
  const auto &s = h.prepared_storage();
  FixtureSource src;
  src.kinds = {1, 1, 1, 1, 1};
  const std::uint64_t max = UINT64_MAX, counts[] = {1,
                                                    (1ull << 53) - 1,
                                                    (1ull << 53) + 1,
                                                    (1ull << 53) + 3,
                                                    (1ull << 63) - 1,
                                                    (1ull << 63) + 1,
                                                    max - 1,
                                                    max};
  for (int i = 0; i < 10; ++i) {
    std::array<float, kParticleFieldCount> p{};
    p[kPosX] = i % 2 ? -1 : 1;
    p[kMass] = 1;
    src.particles.push_back(p);
    src.exact_masses.push_back(i < 8 ? max : 3);
    const auto c = i < 8 ? counts[i] : 1;
    src.edges.push_back({i, 0,
                         float(double(c) / double(src.exact_masses.back())),
                         std::uint64_t(i), c});
  }
  // Individually valid rows which cannot be combined, including sums beyond
  // UINT64_MAX. Source membership itself need not be globally disjoint.
  src.edges.push_back({0, 1, 1, 10, max - 1});
  src.edges.push_back({0, 1, float(2.L / max), 11, 2});
  src.edges.push_back({0, 2, 1, 12, max});
  src.edges.push_back({0, 3, float(1.L / max), 13, 1});
  src.edges.push_back({0, 4, 1, 14, max});
  PreparedFixtureAccess::Input in(l[Budget::Particles], l[Budget::Fields],
                                  l[Budget::Batch], l[Budget::Focus]);
  PreparedFixtureAccess::load(h, src, in, inert(), true);
  refresh(h);
  const Snapshot before(h);
  long double worst_relative = 0;
  for (int i = 0; i < 10; ++i) {
    Hierarchy t;
    t.add(h, in.field_rows[0].record, 1, 0, true, {i});
    require(h.bind_weighted_centroid_cascade(s.generation, t.spec()).status ==
                CascadeStatus::Ok,
            "wide count bind failed");
    const auto q = h.query_centroid_cascade(s.generation);
    const auto &v = h.inspect_centroid_cascade(q);
    oracle(h, q, v, before);
    const auto denom = src.exact_masses[i];
    const auto count = i < 8 ? counts[i] : 1;
    const double want = integer_f64(count) / integer_f64(denom);
    require(q.mass == want,
            "native unsigned count conversion/division differs from integer "
            "encoding oracle");
    const long double exact = static_cast<long double>(count) / denom;
    const auto relative =
        std::fabs(static_cast<long double>(q.mass) - exact) / exact;
    require(relative <= 4 * std::numeric_limits<double>::epsilon(),
            "u64/f64 ratio error exceeds bound");
    worst_relative = std::max(worst_relative, relative);
    if (i >= 8)
      require(q.mass != double(src.edges[i].share),
              "weighted thirds narrowed through f32 share");
    std::cout << "weighted_ratio,count=" << count << ",denominator=" << denom
              << ",mass=" << q.mass << ",legacy_share=" << src.edges[i].share
              << ",relative_error=" << double(relative) << '\n';
  }
  Hierarchy bad;
  bad.add(h, in.field_rows[1].record, 1, 0, true, {10, 11});
  require(h.bind_weighted_centroid_cascade(s.generation, bad.spec()).status ==
              CascadeStatus::InvalidCoverage,
          "u64 selected-row sum wrapped");
  bad = {};
  bad.add(h, in.field_rows[2].record, 1, 0, true, {12});
  bad.add(h, in.field_rows[3].record, 2, 0, true, {13});
  bad.add(h, in.field_rows[4].record, 3, 1, true, {14}, {0, 1});
  require(h.bind_weighted_centroid_cascade(s.generation, bad.spec()).status ==
              CascadeStatus::InvalidCoverage,
          "u64 child sum wrapped");
  before.unchanged(h);
  // A count-only edit whose rounded f32 representation does not change still
  // advances provenance and stales both hierarchy and observation.
  Hierarchy t;
  t.add(h, in.field_rows[0].record, 1, 0, true, {7});
  require(h.bind_weighted_centroid_cascade(s.generation, t.spec()).status ==
              CascadeStatus::Ok,
          "wide edit bind failed");
  const auto q = h.query_centroid_cascade(s.generation);
  const auto generation = s.generation;
  UpdateCommand edit;
  edit.operation = UpdateOp::ReplaceParticipation;
  edit.generation = generation;
  edit.row = 7;
  edit.ordinal = 7;
  edit.first = UpdateRef::existing(in.particle_rows[7]);
  edit.second = UpdateRef::existing(in.field_rows[0]);
  edit.share = 1;
  edit.exact_count = max - 1;
  require(h.update_prepared(generation, &edit, 1).status == UpdateStatus::Ok &&
              s.generation == generation + 1,
          "same-share count edit did not advance generation");
  before.physical(h);
  rejects([&] { h.inspect_centroid_cascade(q); });
  require(h.bind_weighted_centroid_cascade(s.generation, t.spec()).status ==
              CascadeStatus::Stale,
          "same-share count edit reused stale descriptors");
  std::cout << "weighted independent u64/f64 conversion, thirds, overflow, "
               "same-share count edit; max_relative_error="
            << double(worst_relative) << " PASS\n";
}
void recycled(engine::Runtime &r, HostPath path) {
  auto l = limits(false);
  l[Budget::Particles] = 8;
  l[Budget::ScratchBytes] = detail::PreparedStorage::required_scratch(l, false);
  Harness h(r, PreparedCapacity(l), path);
  const auto &s = h.prepared_storage();
  PreparedFixtureAccess::Input in(l[Budget::Particles], l[Budget::Fields],
                                  l[Budget::Batch], l[Budget::Focus]);
  PreparedFixtureAccess::load(h, fixture(8, int(l[Budget::Fields]), false), in,
                              inert(), false);
  const auto original = in.particle_rows[0];
  auto victim = original;
  for (int known = 0; known < 2; ++known) {
    std::vector<UpdateCommand> commands;
    for (std::size_t e = 0; e < s.live.edges; ++e)
      if (same(s.edges[e].first, victim)) {
        UpdateCommand c;
        c.operation = UpdateOp::RemoveParticipation;
        c.row = e;
        c.generation = s.generation;
        commands.push_back(c);
      }
    UpdateCommand retire;
    retire.operation = UpdateOp::RetireParticle;
    retire.first = UpdateRef::existing(victim);
    commands.push_back(retire);
    require(h.update_prepared(s.generation, commands.data(), commands.size())
                    .status == UpdateStatus::Ok,
            "weighted retirement failed");
    std::array<UpdateCommand, 2> fresh;
    fresh[0].operation = UpdateOp::DefineParticle;
    fresh[0].address = original.address;
    if (known)
      fresh[0].exact_mass = 24;
    fresh[1].operation = UpdateOp::InsertParticipation;
    fresh[1].first = UpdateRef::pending(0);
    fresh[1].second = UpdateRef::existing(in.field_rows[6]);
    fresh[1].position = s.live.edges;
    fresh[1].ordinal = 1000 + known;
    if (known)
      fresh[1].exact_count = 24;
    std::array<float, kParticleFieldCount> body{};
    body[kPosX] = 2;
    body[kMass] = 7;
    PreparedFixtureAccess::stage_new(h, 0, body);
    require(
        h.update_prepared(s.generation, fresh.data(), fresh.size()).status ==
            UpdateStatus::Ok,
        "weighted recycled definition failed");
    victim = s.resolved[0];
    require(victim.record == original.record &&
                victim.incarnation != original.incarnation,
            "weighted fixture failed to recycle native row");
    refresh(h);
    const Snapshot before(h);
    Hierarchy t;
    t.add(h, in.field_rows[6].record, 1, 0, true, {int(s.live.edges - 1)});
    if (!known) {
      require(h.bind_weighted_centroid_cascade(s.generation, t.spec()).status ==
                  CascadeStatus::UnsupportedAccounting,
              "unknown recycled denominator inherited old accounting");
      const CascadeSpec legacy{t.nodes.data(), 1, nullptr, 0, &victim, 1};
      require(h.bind_centroid_cascade(s.generation, legacy).status ==
                      CascadeStatus::Ok &&
                  h.query_centroid_cascade(s.generation).mass == 7,
              "unknown accounting changed legacy whole API");
    } else {
      require(h.bind_weighted_centroid_cascade(s.generation, t.spec()).status ==
                  CascadeStatus::Ok,
              "fresh recycled accounting failed");
      const auto q = h.query_centroid_cascade(s.generation);
      require(q.status == CascadeStatus::Ok && q.mass == 7,
              "structural denominator replaced recycled physical mass");
      oracle(h, q, h.inspect_centroid_cascade(q), before);
    }
    before.unchanged(h);
    t.rows[0].occurrence = original;
    require(h.bind_weighted_centroid_cascade(s.generation, t.spec()).status ==
                CascadeStatus::InvalidReference,
            "new row/generation legitimized retired occurrence handle");
    before.unchanged(h);
  }
  std::cout << "weighted retirement/recycling, unknown denominator, legacy "
               "independence and full-handle rejection PASS\n";
}
void capacities(engine::Runtime &r, HostPath path) {
  for (Budget budget :
       {Budget::CascadeNodes, Budget::CascadeLinks, Budget::CascadeReferences,
        Budget::CascadeLevels, Budget::CascadeCoverage, Budget::References}) {
    auto l = limits(false);
    if (budget == Budget::CascadeNodes)
      l[budget] = 6;
    if (budget == Budget::CascadeLinks)
      l[budget] = 5;
    if (budget == Budget::CascadeReferences)
      l[budget] = 63;
    if (budget == Budget::CascadeLevels)
      l[budget] = 2;
    if (budget == Budget::CascadeCoverage)
      l[budget] = 55;
    if (budget == Budget::References)
      l[budget] = 278;
    l[Budget::ScratchBytes] =
        detail::PreparedStorage::required_scratch(l, false);
    Harness h(r, PreparedCapacity(l), path);
    PreparedFixtureAccess::Input in(l[Budget::Particles], l[Budget::Fields],
                                    l[Budget::Batch], l[Budget::Focus]);
    PreparedFixtureAccess::load(h, fixture(8, int(l[Budget::Fields]), false),
                                in, inert(), true);
    refresh(h);
    const Snapshot before(h);
    const auto t = hierarchy(h, in, 3);
    CascadeBinding b;
    no_new([&] {
      b = h.bind_weighted_centroid_cascade(h.prepared_storage().generation,
                                           t.spec());
    });
    if (b.status != CascadeStatus::CapacityExhausted ||
        b.failure.budget != budget)
      std::cerr << "capacity expected=" << int(budget)
                << ",actual=" << int(b.failure.budget)
                << ",status=" << int(b.status) << '\n';
    require(b.status == CascadeStatus::CapacityExhausted &&
                b.failure.budget == budget,
            "independent weighted capacity did not reject");
    before.unchanged(h);
  }
  auto l = limits(false);
  --l[Budget::ScratchBytes];
  const auto ids = r.issued_snode_ids();
  bool rejected = false;
  try {
    Harness h(r, PreparedCapacity(l), path);
  } catch (const CapacityError &e) {
    rejected = e.failure.budget == Budget::ScratchBytes;
  }
  require(rejected && r.issued_snode_ids() == ids,
          "weighted scratch failure mutated native layout");
  l = limits(false);
  l[Budget::CascadeCoverage] = 0;
  l[Budget::ScratchBytes] = detail::PreparedStorage::required_scratch(l, false);
  Harness disabled(r, PreparedCapacity(l), path);
  require(disabled.bind_weighted_centroid_cascade(0, {}).status ==
              CascadeStatus::Unsupported,
          "weighted path ran without coverage reservation");
  const auto &manifest = disabled.prepared_storage().manifest;
  require(std::none_of(manifest.begin(), manifest.end(),
                       [](const auto &e) {
                         return std::string(e.name) == "cascade-source-ratios";
                       }),
          "disabled weighted profile reserved descriptor payload");
  std::cout << "weighted independent N/H/R/L/coverage/global-R/S/work "
               "exhaustion and optional-zero path PASS\n";
}
void assembly_objects(engine::Runtime &r, HostPath path) {
  using namespace assembly;
  using namespace taichi::lang;
  auto root = std::make_unique<SNode>(0, SNodeType::root);
  auto &primary = root->dense(Axis(0), 1),
       &compressed = primary.dense(Axis(1), 1),
       &named = compressed.dense(Axis(2), 1);
  StudyInput input;
  input.capacity = 4;
  input.nodes = {{root.get(), -1, ScaleRole::Root},
                 {&primary, 0, ScaleRole::PrimaryStep},
                 {&compressed, 1, ScaleRole::SecondaryCompression},
                 {&named, 2, ScaleRole::NamingEquivalent}};
  input.definitions = {{"a", 1, {}, {{{}, .25}}, {{"A", 0, 1}}},
                       {"b", 2, {}, {{{}, .25}}, {{"A", 0, 1}, {"B", 1, 1}}},
                       {"c", 1, {}, {{{}, .25}}, {{"B", 0, 1}}}};
  input.occurrences = {{"a", "a", 0, 0, {1, 0, 0}, {}},
                       {"b", "b", 1, 0, {1, 0, 0}, {}},
                       {"c", "c", 2, 0, {-1, 0, 0}, {}},
                       {"b-copy", "b", 3, 3, {1, 0, 0}, {}}};
  for (const auto &occ : input.occurrences) {
    input.memberships.push_back({occ.id, "Root"});
    input.memberships.push_back({occ.id, "Study"});
  }
  const auto prepared = prepare(input);
  require(prepared.particles[3].position.x == 64 &&
              prepared.scales[1].units == prepared.scales[3].units,
          "source conceptual/1:1 scale conversion incorrect");
  auto l = limits(false);
  Harness h(r, PreparedCapacity(l), path);
  FixtureSource src;
  src.kinds.resize(prepared.groups.size(), 1);
  for (const auto &p : prepared.particles) {
    std::array<float, kParticleFieldCount> body{};
    body[kPosX] = float(p.position.x);
    body[kMass] = p.mass;
    body[kRadius] = p.radius;
    src.particles.push_back(body);
    src.exact_masses.push_back(p.exact_mass);
  }
  for (const auto &edge : prepared.participation)
    src.edges.push_back(
        {edge.slot, edge.group, edge.share, edge.ordinal, edge.exact_count});
  PreparedFixtureAccess::Input in(l[Budget::Particles], l[Budget::Fields],
                                  l[Budget::Batch], l[Budget::Focus]);
  PreparedFixtureAccess::load(h, src, in, inert(), true);
  refresh(h);
  const Snapshot before(h);
  const auto group = [&](GroupKind kind, const char *value) {
    auto g = std::find_if(
        prepared.groups.begin(), prepared.groups.end(),
        [&](const auto &g) { return g.kind == kind && g.value == value; });
    require(g != prepared.groups.end(), "real assembly group absent");
    return in.field_rows[g - prepared.groups.begin()];
  };
  Hierarchy tree;
  const auto add = [&](RecordHandle g, int context, int level,
                       const std::vector<int> &children = {}) {
    tree.add(h, g.record, context, level, true, source_rows(h, g), children);
  };
  add(group(GroupKind::ParentValue, "A"), 1, 0);
  add(group(GroupKind::ParentValue, "B"), 2, 0);
  add(group(GroupKind::Membership, "Root"), 3, 1, {0, 1});
  add(group(GroupKind::Membership, "Study"), 4, 2, {2});
  auto b = h.bind_weighted_centroid_cascade(h.prepared_storage().generation,
                                            tree.spec());
  auto q = h.query_centroid_cascade(h.prepared_storage().generation);
  require(b.status == CascadeStatus::Ok && q.status == CascadeStatus::Ok &&
              q.mass == 6 && std::fabs(q.centre[0] - 130. / 6) < 1e-13,
          "real a/b/c/copy coverage or scale applied twice");
  oracle(h, q, h.inspect_centroid_cascade(q), before);
  before.unchanged(h);
  std::cout
      << "weighted real a/b/c source counts, independent live copy, "
         "three-level overlap, scale once and compression/naming 1:1 PASS\n";
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
    require(!runtime.on_cuda(), "weighted CPU test selected CUDA");
    const auto path = direct ? HostPath::CpuDirect : HostPath::BulkCopy;
    std::cout << "actual_backend=cpu,path=" << (direct ? "direct" : "bulk")
              << '\n';
    std::cout << "typed_widths,RecordHandle=" << sizeof(RecordHandle)
              << ",CascadeNode=" << sizeof(CascadeNode)
              << ",CascadeSourceRow=" << sizeof(CascadeSourceRow)
              << ",CascadeContribution=" << sizeof(CascadeContribution)
              << ",CascadeCoverage=" << sizeof(CascadeCoverage)
              << ",CascadeBinding=" << sizeof(CascadeBinding)
              << ",CascadeResult=" << sizeof(CascadeResult)
              << ",BudgetDemand=" << sizeof(BudgetDemand) << '\n';
    run(runtime, path, large, maximum);
    if (!large && !maximum) {
      rejections(runtime, path);
      wide_and_thirds(runtime, path);
      recycled(runtime, path);
      capacities(runtime, path);
      assembly_objects(runtime, path);
    }
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
