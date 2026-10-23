#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>

#include "allocation_observation.h"
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
void rejects(F run) {
  try {
    run();
  } catch (const std::logic_error &) {
    return;
  }
  throw std::runtime_error("stale geometry observation accepted");
}
template <class F>
void no_new(F run) {
  allocation_observation::calls = 0;
  allocation_observation::enabled = true;
  try {
    run();
  } catch (...) {
    allocation_observation::enabled = false;
    throw;
  }
  allocation_observation::enabled = false;
  require(allocation_observation::calls == 0,
          "geometry boundary allocated C++ metadata");
}
bool bits(const std::vector<float> &a, const std::vector<float> &b) {
  return a.size() == b.size() &&
         !std::memcmp(a.data(), b.data(), a.size() * sizeof(float));
}
struct Snapshot {
  std::vector<float> particles, groups, speed, motion, shares, universal;
  std::vector<int> state, flags, contacts, edges;
  std::array<float, 11> focus;
  std::array<unsigned char, sizeof(Demand)> demand{};
  std::uint64_t generation, ticks;
  Snapshot(Harness &h)
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
        focus(h.prepared_storage().inspection_focus),
        generation(h.prepared_storage().generation),
        ticks(h.prepared_storage().completed_ticks) {
    std::memcpy(demand.data(), &h.demand(), demand.size());
  }
  void unchanged(Harness &h) const {
    require(h.ready(), "geometry preflight changed readiness");
    h.inspect_prepared();
    PreparedFixtureAccess::read_contacts(h);
    require(bits(particles, h.particle_data) && bits(groups, h.group_data) &&
                bits(speed, h.determined()) &&
                bits(motion, h.prepared_storage().inspection_motion) &&
                bits(shares, h.edge_float_data) &&
                bits(universal, h.universal_data) &&
                state == h.prepared_storage().inspection_state &&
                flags == h.prepared_storage().inspection_flags &&
                contacts == h.contact_int_data && edges == h.edge_int_data &&
                focus == h.prepared_storage().inspection_focus &&
                generation == h.prepared_storage().generation &&
                ticks == h.prepared_storage().completed_ticks &&
                !std::memcmp(demand.data(), &h.demand(), demand.size()),
            "query changed committed numerical/contact/activity/history/report "
            "state");
  }
};
BudgetDemand limits(engine::Runtime &r, bool large, std::size_t contacts = 0) {
  BudgetDemand l;
  l.values = {large ? 16777216u : 1048576u,
              large ? 4096u : 70u,
              large ? 257u : 9u,
              large ? 16384u : 280u,
              contacts ? contacts : (large ? 8192u : 140u),
              large ? 53762u : 928u,
              large ? 257u : 9u,
              large ? 256u : 32u,
              0};
  l[Budget::GeometryWork] = std::uint64_t{1} << 28;
  l[Budget::ScratchBytes] =
      detail::PreparedStorage::required_scratch(l, r.on_cuda());
  return l;
}
std::array<float, kParticleFieldCount> body(float x, float radius = 1) {
  std::array<float, kParticleFieldCount> result{};
  result[kPosX] = x;
  result[kMass] = 1;
  result[kRadius] = radius;
  return result;
}
Parameters inert() {
  Parameters p;
  p.gravity = 0;
  p.reach_strength = 0;
  p.contact_stiffness = 0;
  return p;
}
void csr(const GeometryResult &q, const GeometryInspection &v, int p, int c) {
  for (int side = 0; side < 2; ++side) {
    int total = 0;
    for (int row = 0; row < p; ++row) {
      int begin = v.offsets[(2 * side) * p + row],
          end = v.offsets[(2 * side + 1) * p + row];
      require(begin == total && end >= begin && end <= int(q.contacts),
              "incomplete candidate CSR");
      for (int k = begin; k < end; ++k) {
        int pair = v.orders[side * c + k];
        require(pair >= 0 && pair < int(q.contacts) &&
                    v.pairs[side * c + pair] == row,
                "candidate CSR names wrong endpoint");
      }
      total = end;
    }
    require(total == int(q.contacts), "candidate CSR omitted a side");
  }
}
void run(engine::Runtime &runtime, HostPath path, bool large, bool maximum) {
  const auto l = limits(runtime, large);
  Harness h(runtime, PreparedCapacity(l), path);
  const auto &s = h.prepared_storage();
  auto empty = h.query_prepared_geometry(0);
  require(empty.status == GeometryStatus::Ok &&
              empty.focus == GeometryFocus::Empty && !empty.present,
          "empty query fabricated geometry");
  FixtureSource source;
  if (maximum) {
    for (std::size_t i = 0; i < l[Budget::Particles]; ++i)
      source.particles.push_back(body(float(i * 4), 0.25f));
    source.kinds.resize(l[Budget::Fields], 1);
    for (std::size_t i = 0; i < l[Budget::Participation]; ++i)
      source.edges.push_back({int(i % source.particles.size()),
                              int(i / source.particles.size()), 1, i});
    for (std::size_t i = 0; i < l[Budget::Contacts]; ++i) {
      const int a =
          int(source.particles.size() - 1 - i % source.particles.size());
      source.contacts.push_back({a, int((a + 1 + i / source.particles.size()) %
                                        source.particles.size())});
    }
    source.focus = {0};
  } else {
    source.particles = {body(-0.5f),     body(0.5f),  body(10, 0.25f),
                        body(10, 0.25f), body(30, 0), body(40, 0.5f)};
    source.particles[0][kVelX] = 0.125f;
    source.particles[1][kVelX] = 0.125f;
    source.kinds = {1, 2, 3};
    for (int i = 0; i < 6; ++i)
      source.edges.push_back({i, i / 2, 1, std::uint64_t(100 + i)});
    source.contacts = {{{5, 0}}, {{1, 0}}, {{0, 5}}};
    source.focus = {0, 1};
  }
  PreparedFixtureAccess::Input input(l[Budget::Particles], l[Budget::Fields],
                                     l[Budget::Batch], l[Budget::Focus]);
  prepared_test::Metrics metrics(runtime, PreparedFixtureAccess::nodes(h));
  const auto prepared = metrics.sample();
  PreparedFixtureAccess::load(h, source, input, inert(), true);
  h.inspect_prepared();
  const Snapshot before(h);
  if (path == HostPath::BulkCopy) {
    std::fill(h.particle_data.begin(), h.particle_data.end(), -12345);
    std::fill(h.group_data.begin(), h.group_data.end(), -12345);
    std::fill(h.contact_int_data.begin(), h.contact_int_data.end(), -1);
  }
  GeometryResult q;
  const auto query_start = std::chrono::steady_clock::now();
  no_new([&] { q = h.query_prepared_geometry(s.generation); });
  const auto query_us = std::chrono::duration<double, std::micro>(
                            std::chrono::steady_clock::now() - query_start)
                            .count();
  require(q.status == GeometryStatus::Ok &&
              q.present == source.particles.size() &&
              q.presence_entries == q.present &&
              q.compaction_slots == l[Budget::Particles] &&
              q.pair_tests == q.present * (q.present - 1) / 2 &&
              q.focus == GeometryFocus::Valid,
          "current native presence/focus/work query failed");
  require(q.readback_bytes == 272 && q.synchronizations == 3,
          "normal query downloaded payload or hid boundary synchronizations");
  require(prepared.same_backing(metrics.sample()),
          "first native query changed prepared backing or compilation");
  const GeometryInspection *inspection = nullptr;
  no_new([&] { inspection = &h.inspect_prepared_geometry(q); });
  const auto &v = *inspection;
  std::vector<int> expected;
  for (std::size_t i = 0; i < source.particles.size(); ++i)
    expected.push_back(input.particle_rows[i].record);
  std::sort(expected.begin(), expected.end());
  require(
      std::equal(expected.begin(), expected.end(), v.occupied_rows.begin()) &&
          expected.back() == h.particle_capacity() - 1,
      "occupancy missed native holes/final slot or used potential addresses");
  csr(q, v, h.particle_capacity(), h.contact_capacity());
  if (maximum) {
    require(q.contacts == source.contacts.size() && q.derived == 0 &&
                q.source_order_tests == q.contacts * q.contacts &&
                q.union_order_tests == q.contacts * q.contacts,
            "maximum contact/source candidate missing");
    for (std::size_t row = 0; row < source.contacts.size(); ++row) {
      require(
          v.pairs[row] == input.particle_rows[source.contacts[row][0]].record &&
              v.pairs[h.contact_capacity() + row] ==
                  input.particle_rows[source.contacts[row][1]].record &&
              v.pair_flags[row] == ExplicitContact &&
              v.source_to_pair[row] == int(row),
          "source order/orientation was lost");
    }
  } else {
    require(q.contacts == 3 && q.derived == 2 && q.required_work == 33 &&
                q.focal_point[0] == 5 && q.focal_mass == 4,
            "contact dedup or current ordered focus incorrect");
    require(v.source_to_pair[0] == 0 && v.source_to_pair[1] == 1 &&
                v.source_to_pair[2] == 0 &&
                v.pair_flags[0] == ExplicitContact &&
                v.pair_flags[1] == (ExplicitContact | DerivedContact) &&
                v.pair_flags[2] == DerivedContact &&
                v.pairs[0] == input.particle_rows[5].record &&
                v.pairs[h.contact_capacity()] == input.particle_rows[0].record,
            "explicit/derived identity/order lost");
  }
  before.unchanged(h);
  auto foreign = q;
  ++foreign.context;
  rejects([&] { h.inspect_prepared_geometry(foreign); });
  if (!maximum) {
    auto too_large =
        h.query_prepared_geometry(s.generation, l[Budget::GeometryWork] + 1);
    require(too_large.status == GeometryStatus::CapacityExhausted &&
                !too_large.readback_bytes && !too_large.synchronizations,
            "unreserved per-query work was dispatched");
    auto failed = h.query_prepared_geometry(s.generation, 23);
    require(failed.status == GeometryStatus::CapacityExhausted &&
                failed.failure.budget == Budget::GeometryWork &&
                failed.failure.required == 24 && !failed.pair_tests,
            "pair/order work exhaustion boundary incorrect");
    rejects([&] { h.inspect_prepared_geometry(q); });
    before.unchanged(h);
    failed = h.query_prepared_geometry(s.generation, 24);
    require(failed.status == GeometryStatus::CapacityExhausted &&
                failed.failure.required == 33 && failed.pair_tests == 15 &&
                !failed.union_order_tests,
            "post-discovery order-work exhaustion boundary incorrect");
    before.unchanged(h);
    const auto stale = h.query_prepared_geometry(s.generation + 1);
    require(stale.status == GeometryStatus::Stale,
            "stale source generation accepted");
    q = h.query_prepared_geometry(s.generation);
    h.tick(inert());
    rejects([&] { h.inspect_prepared_geometry(q); });
    q = h.query_prepared_geometry(s.generation);
    require(q.awake == 2 && q.contacts == 3 && q.derived == 2 &&
                q.focal_point[0] == 5.0625f,
            "query omitted sleepers or reused pre-motion focal origin");
    for (int cycle = 0; cycle < 32; ++cycle) {
      h.tick(inert());
      h.inspect_prepared();
      const Snapshot live(h);
      no_new([&] {
        q = h.query_prepared_geometry(s.generation);
        h.inspect_prepared_geometry(q);
      });
      require(q.status == GeometryStatus::Ok && q.present == 6,
              "repeated current geometry failed");
      live.unchanged(h);
    }
    q = h.query_prepared_geometry(s.generation);
    UpdateCommand define;
    define.operation = UpdateOp::DefineField;
    define.address = 7;
    define.kind = 2;
    auto result = h.update_prepared(s.generation, &define, 1);
    require(result.status == UpdateStatus::Ok,
            "independent field declaration failed");
    rejects([&] { h.inspect_prepared_geometry(q); });
    q = h.query_prepared_geometry(s.generation);
    require(q.status == GeometryStatus::Ok && q.present == 6,
            "field counted as physical occupancy");
  }
  require(prepared.same_backing(metrics.sample()),
          "geometry cycles changed prepared backing");
  prepared.print("after_setup");
  metrics.sample().print("after_query_cycles");
  for (const auto &entry : s.manifest)
    std::cout << "manifest," << entry.name << ',' << entry.count << ','
              << entry.element_bytes << ',' << entry.copies << ','
              << entry.bytes << '\n';
  std::cout << "geometry " << (large ? 4096 : 70) << ' '
            << (path == HostPath::BulkCopy ? "bulk" : "direct") << ' '
            << (maximum ? "maximum" : "holes/motion/rejections")
            << " PASS; S=" << l[Budget::ScratchBytes]
            << " work=" << q.required_work << " first_query_us=" << query_us
            << " bindings=" << s.binding_bytes << '\n';
}
void failures(engine::Runtime &runtime, HostPath path) {
  for (bool references : {false, true}) {
    auto l = limits(runtime, false, references ? 15 : 2);
    if (references)
      l[Budget::References] = 20;
    l[Budget::ScratchBytes] =
        detail::PreparedStorage::required_scratch(l, false);
    Harness h(runtime, PreparedCapacity(l), path);
    FixtureSource src;
    src.kinds = {1};
    src.focus = {0};
    const int n = references ? 6 : 3;
    for (int i = 0; i < n; ++i) {
      src.particles.push_back(body(float(i) * 0.01f));
      src.edges.push_back({i, 0, 1, std::uint64_t(i)});
    }
    PreparedFixtureAccess::Input input(l[Budget::Particles], l[Budget::Fields],
                                       l[Budget::Batch], l[Budget::Focus]);
    PreparedFixtureAccess::load(h, src, input, inert(), true);
    h.inspect_prepared();
    const Snapshot before(h);
    GeometryResult q;
    no_new([&] {
      q = h.query_prepared_geometry(h.prepared_storage().generation);
    });
    require(q.status == GeometryStatus::CapacityExhausted &&
                q.failure.budget ==
                    (references ? Budget::References : Budget::Contacts),
            "independent candidate capacity exhaustion missed");
    before.unchanged(h);
    rejects([&] { h.inspect_prepared_geometry(q); });
  }
  auto l = limits(runtime, false);
  l[Budget::GeometryWork] = 0;
  l[Budget::ScratchBytes] = detail::PreparedStorage::required_scratch(l, false);
  Harness disabled(runtime, PreparedCapacity(l), path);
  require(disabled.query_prepared_geometry(0).status ==
                  GeometryStatus::Unsupported &&
              !disabled.prepared_storage().geometry_rows,
          "unreserved geometry path allocated or executed");
}
void boundaries(engine::Runtime &runtime, HostPath path) {
  for (int scenario = 0; scenario < 3; ++scenario) {
    auto l = limits(runtime, false);
    l[Budget::Contacts] = 0;
    l[Budget::ScratchBytes] =
        detail::PreparedStorage::required_scratch(l, false);
    Harness h(runtime, PreparedCapacity(l), path);
    FixtureSource src;
    if (scenario == 0)
      src.particles = {body(0), body(2), body(10, 0), body(10, 0)};
    if (scenario == 1)
      src.particles = {body(0), body(0)};
    if (scenario == 2)
      src.particles = {body(-1e30f), body(1e30f)};
    PreparedFixtureAccess::Input input(l[Budget::Particles], l[Budget::Fields],
                                       l[Budget::Batch], l[Budget::Focus]);
    PreparedFixtureAccess::load(h, src, input, inert(), true);
    h.inspect_prepared();
    const Snapshot before(h);
    GeometryResult q;
    no_new([&] {
      q = h.query_prepared_geometry(h.prepared_storage().generation);
    });
    require(q.focus == GeometryFocus::MissingSelection,
            "missing focus fabricated a placement origin");
    if (scenario == 0) {
      require(q.status == GeometryStatus::Ok && !q.contacts && !q.derived,
              "tangency/zero extents fabricated a contact or consumed C=0 "
              "sentinel");
      const auto &v = h.inspect_prepared_geometry(q);
      csr(q, v, h.particle_capacity(), 1);
    } else if (scenario == 1) {
      require(q.status == GeometryStatus::CapacityExhausted &&
                  q.failure.budget == Budget::Contacts &&
                  q.failure.required == 1 && q.failure.capacity == 0,
              "positive-radius coincidence escaped C=0 rejection");
    } else {
      require(q.status == GeometryStatus::InvalidNumerics && q.pair_tests == 1,
              "nonrepresentable finite-payload geometry accepted");
    }
    before.unchanged(h);
  }
}
}  // namespace
int main(int argc, char **argv) {
  try {
    bool direct = false, large = false, maximum = false;
    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      direct |= arg == "--direct";
      large |= arg == "--large";
      maximum |= arg == "--maximum";
    }
    engine::InstanceSettings settings;
    settings.read_environment = false;
    settings.cpu_threads = 4;
    engine::Runtime runtime(settings);
    run(runtime, direct ? HostPath::CpuDirect : HostPath::BulkCopy, large,
        maximum);
    if (!large && !maximum) {
      failures(runtime, direct ? HostPath::CpuDirect : HostPath::BulkCopy);
      boundaries(runtime, direct ? HostPath::CpuDirect : HostPath::BulkCopy);
    }
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
