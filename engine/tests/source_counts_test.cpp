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
constexpr std::uint64_t wide_mass = (std::uint64_t{1} << 63) + 123;
void require(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
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
          "source-count boundary allocated C++ metadata");
}
template <class T>
bool bytes(const std::vector<T> &a, const std::vector<T> &b) {
  return a.size() == b.size() &&
         (!a.size() || !std::memcmp(a.data(), b.data(), a.size() * sizeof(T)));
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
Parameters params() {
  Parameters p;
  p.gravity = .0001f;
  p.dt = .015625f;
  p.contact_stiffness = .001f;
  p.reach_strength = 0;
  return p;
}
BudgetDemand limits(bool large) {
  BudgetDemand l;
  const std::uint64_t p = large ? 4096 : 70, g = large ? 257 : 9;
  l.values = {
      large ? 16777216u : 1048576u, p, g, 5 * p - 2, 2 * p, 20 * p + g + 32, g,
      large ? 256u : 32u,           0};
  l[Budget::CascadeNodes] = 1;
  l[Budget::CascadeReferences] = 1;
  l[Budget::CascadeLevels] = 1;
  l[Budget::CascadeWork] = std::uint64_t{1} << 30;
  l[Budget::ScratchBytes] = detail::PreparedStorage::required_scratch(l, false);
  return l;
}
assembly::PreparedStudy study(int n, taichi::lang::SNode &root) {
  using namespace assembly;
  StudyInput s;
  s.capacity = n;
  s.nodes = {{&root, -1, ScaleRole::Root}};
  s.definitions = {
      {"thirds", 3, {}, {{{}, .125}}, {{"A", 0, 1}, {"B", 1, 1}, {"A", 2, 1}}},
      {"wide",
       wide_mass,
       {},
       {{{}, .125}},
       {{"wide-A", 7, wide_mass - 1}, {"wide-B", 9, 1}}}};
  for (int i = 0; i < n; ++i) {
    const auto name = std::to_string(i);
    s.occurrences.push_back({name,
                             i + 1 == n ? "wide" : "thirds",
                             i,
                             0,
                             {double(i * 2), 0, 0},
                             {.125, 0, 0}});
    if (i + 1 < n)
      s.memberships.push_back({name, "language"});
  }
  return prepare(s);
}
struct Input {
  std::vector<UpdateCommand> commands;
  std::vector<RecordHandle> particles, fields;
  Input(const BudgetDemand &l)
      : commands(l[Budget::Batch]),
        particles(l[Budget::Particles]),
        fields(l[Budget::Fields]) {
  }
};
UpdateResult submit(Harness &h, const UpdateCommand *c, std::size_t n) {
  UpdateResult out;
  no_new(
      [&] { out = h.update_prepared(h.prepared_storage().generation, c, n); });
  return out;
}
void refresh(Harness &h) {
  h.inspect_prepared();
  PreparedFixtureAccess::read_relationships(h);
}
void load(Harness &h,
          Input &input,
          const assembly::PreparedStudy &src,
          bool exact) {
  const auto b = input.commands.size();
  for (std::size_t first = 0; first < input.fields.size(); first += b) {
    const auto n = std::min(b, input.fields.size() - first);
    for (std::size_t j = 0; j < n; ++j) {
      auto &c = input.commands[j];
      c = {};
      c.operation = UpdateOp::DefineField;
      c.address = int(first + j);
      c.kind = 1 + int((first + j) % 3);
    }
    require(submit(h, input.commands.data(), n).status == UpdateStatus::Ok,
            "field source loading failed");
    for (std::size_t j = 0; j < n; ++j)
      input.fields[first + j] = h.prepared_storage().resolved[j];
  }
  for (std::size_t first = 0; first < src.particles.size(); first += b) {
    const auto n = std::min(b, src.particles.size() - first);
    for (std::size_t j = 0; j < n; ++j) {
      const auto &p = src.particles[first + j];
      auto &c = input.commands[j];
      c = {};
      c.operation = UpdateOp::DefineParticle;
      c.address = int(first + j);
      if (exact)
        c.exact_mass = p.exact_mass;
      std::array<float, kParticleFieldCount> body{};
      body[kMass] = p.mass;
      body[kRadius] = p.radius;
      body[kPosX] = float(p.position.x);
      body[kVelX] = float(p.velocity.x);
      PreparedFixtureAccess::stage_new(h, int(j), body);
    }
    require(submit(h, input.commands.data(), n).status == UpdateStatus::Ok,
            "particle source loading failed");
    for (std::size_t j = 0; j < n; ++j)
      input.particles[first + j] = h.prepared_storage().resolved[j];
  }
  for (std::size_t first = 0; first < src.participation.size(); first += b) {
    const auto n = std::min(b, src.participation.size() - first);
    for (std::size_t j = 0; j < n; ++j) {
      const auto &e = src.participation[first + j];
      auto &c = input.commands[j];
      c = {};
      c.operation = UpdateOp::InsertParticipation;
      c.first = UpdateRef::existing(input.particles[e.slot]);
      c.second = UpdateRef::existing(input.fields[e.group]);
      c.position = first + j;
      c.ordinal = e.ordinal;
      c.share = e.share;
      if (exact)
        c.exact_count = e.exact_count;
    }
    require(submit(h, input.commands.data(), n).status == UpdateStatus::Ok,
            "participation source loading failed");
  }
  refresh(h);
}
UpdateCommand replace(Harness &h,
                      std::size_t row,
                      std::optional<std::uint64_t> count) {
  const auto &e = h.prepared_storage().edges[row];
  UpdateCommand c;
  c.operation = UpdateOp::ReplaceParticipation;
  c.row = row;
  c.generation = h.prepared_storage().generation;
  c.first = UpdateRef::existing(e.first);
  c.second = UpdateRef::existing(e.second);
  c.ordinal = e.ordinal;
  c.share = e.share;
  c.exact_count = count;
  return c;
}
void parity(Harness &a, Harness &b, const Input &ia, const Input &ib) {
  refresh(a);
  refresh(b);
  const auto &x = a.prepared_storage();
  const auto &y = b.prepared_storage();
  const auto equal = [](float a, float b) {
    return !std::memcmp(&a, &b, sizeof(a));
  };
  const auto p = a.particle_capacity(), g = a.field_capacity();
  for (std::size_t i = 0; i < ia.particles.size(); ++i) {
    const auto u = ia.particles[i].record, v = ib.particles[i].record;
    for (int f = 0; f < kParticleFieldCount; ++f)
      require(equal(a.particle(f, u), b.particle(f, v)),
              "legacy logical body data differs");
    require(equal(a.determined()[u], b.determined()[v]) &&
                x.inspection_flags[u] == y.inspection_flags[v],
            "legacy logical body presence/speed differs");
    for (int f = 0; f < 2; ++f)
      require(x.inspection_state[f * p + u] == y.inspection_state[f * p + v],
              "legacy logical body state differs");
    for (int f = 0; f < 3; ++f)
      require(
          equal(x.inspection_motion[f * p + u], y.inspection_motion[f * p + v]),
          "legacy logical body displacement differs");
  }
  for (std::size_t i = 0; i < ia.fields.size(); ++i) {
    const auto u = ia.fields[i].record, v = ib.fields[i].record;
    for (int f = 0; f < kGroupFieldCount; ++f)
      require(equal(a.group(f, u), b.group(f, v)),
              "legacy logical field data differs");
    require(x.inspection_flags[p + u] == y.inspection_flags[p + v],
            "legacy logical field presence differs");
    for (int f = 0; f < 2; ++f)
      require(x.inspection_state[2 * p + f * g + u] ==
                  y.inspection_state[2 * p + f * g + v],
              "legacy logical field state differs");
    for (int f = 0; f < 6; ++f)
      require(equal(x.inspection_motion[3 * p + f * g + u],
                    y.inspection_motion[3 * p + f * g + v]),
              "legacy logical field history differs");
  }
  require(x.live.edges == y.live.edges, "legacy participation counts differ");
  for (std::size_t row = 0; row < x.live.edges; ++row)
    require(x.edges[row].first.address == y.edges[row].first.address &&
                x.edges[row].second.address == y.edges[row].second.address &&
                x.edges[row].ordinal == y.edges[row].ordinal,
            "legacy ordered logical participation differs");
  require(bytes(a.edge_float_data, b.edge_float_data) &&
              a.contact_int_data == b.contact_int_data &&
              bytes(a.universal_data, b.universal_data) &&
              !std::memcmp(x.inspection_focus.data(), y.inspection_focus.data(),
                           sizeof(x.inspection_focus)) &&
              a.frame_anchor() == b.frame_anchor(),
          "exact metadata changed legacy numerical/activity results");
  for (const float value : a.particle_data)
    require(std::isfinite(value), "nonfinite legacy parity fixture");
}
void run(engine::Runtime &runtime, HostPath path, bool large) {
  const auto l = limits(large);
  taichi::lang::SNode root(0, taichi::lang::SNodeType::root);
  const auto src = study(int(l[Budget::Particles]), root);
  require(src.participation.size() == l[Budget::Participation],
          "source fixture did not fill participation capacity");
  const auto setup_start = std::chrono::steady_clock::now();
  Harness exact(runtime, PreparedCapacity(l), path),
      legacy(runtime, PreparedCapacity(l), path);
  const auto setup_us = std::chrono::duration<double, std::micro>(
                            std::chrono::steady_clock::now() - setup_start)
                            .count();
  Input a(l), b(l);
  prepared_test::Metrics metrics(runtime, PreparedFixtureAccess::nodes(exact));
  prepared_test::Metrics legacy_metrics(runtime,
                                        PreparedFixtureAccess::nodes(legacy));
  const auto before = metrics.sample(), legacy_before = legacy_metrics.sample();
  const auto loading_start = std::chrono::steady_clock::now();
  load(exact, a, src, true);
  load(legacy, b, src, false);
  const auto loading_us = std::chrono::duration<double, std::micro>(
                              std::chrono::steady_clock::now() - loading_start)
                              .count();
  parity(exact, legacy, a, b);
  const auto &s = exact.prepared_storage();
  const auto &t = legacy.prepared_storage();
  require(s.source_masses.size() == l[Budget::Particles] &&
              s.source_counts.size() == l[Budget::Participation],
          "source metadata reserved at potential or wrong capacity");
  for (std::size_t i = 0; i < src.particles.size(); ++i) {
    require(
        s.source_masses[a.particles[i].record] == src.particles[i].exact_mass &&
            !t.source_masses[b.particles[i].record],
        "source denominator/copy/unknown provenance lost");
  }
  for (std::size_t i = 0; i < src.participation.size(); ++i)
    require(s.source_counts[i] == src.participation[i].exact_count &&
                !t.source_counts[i] &&
                s.edges[i].ordinal == src.participation[i].ordinal,
            "ordered source count was inferred, aliased or lost");
  const auto last = a.particles.back();
  const std::array<RecordHandle, 2> originals{last, b.particles.back()};
  require(std::any_of(a.particles.begin(), a.particles.end(),
                      [&](const auto &h) {
                        return h.record == int(l[Budget::Particles]) - 1;
                      }),
          "maximal source omitted final record");
  require(src.participation[1].ordinal == 0 &&
              src.participation[3].ordinal == 2 &&
              src.participation[1].group == src.participation[3].group,
          "real source repeated-ordinal fixture missing");
  std::size_t wide_row = 0;
  for (std::size_t i = 0; i < src.participation.size(); ++i)
    if (src.participation[i].slot == int(src.particles.size()) - 1 &&
        src.participation[i].ordinal == 7)
      wide_row = i;
  require(s.source_counts[wide_row] == wide_mass - 1,
          "unsigned source count narrowed above 2^63");
  CascadeNode node;
  node.field = a.fields[src.participation[wide_row - 1].group];
  node.comparison = 1;
  node.cohort_count = 1;
  CascadeSpec spec{&node, 1, nullptr, 0, &last, 1};
  require(exact.bind_centroid_cascade(s.generation, spec).status ==
              CascadeStatus::Ok,
          "legacy whole query rejected metadata");
  const auto q = exact.query_centroid_cascade(s.generation);
  Snapshot committed(exact);
  auto change = replace(exact, wide_row, wide_mass - 2);
  const auto edit_start = std::chrono::steady_clock::now();
  auto result = submit(exact, &change, 1);
  const auto edit_us = std::chrono::duration<double, std::micro>(
                           std::chrono::steady_clock::now() - edit_start)
                           .count();
  require(result.status == UpdateStatus::Ok &&
              result.generation == committed.generation + 1 &&
              s.source_counts[wide_row] == wide_mass - 2,
          "count-only edit did not advance source generation");
  committed.physical(exact);
  require(
      exact.query_centroid_cascade(s.generation).status == CascadeStatus::Stale,
      "count-only edit retained old hierarchy");
  bool stale = false;
  try {
    exact.inspect_centroid_cascade(q);
  } catch (const std::logic_error &) {
    stale = true;
  }
  require(stale, "count-only edit retained old observation");
  refresh(exact);
  Snapshot stable(exact);
  change.generation = s.generation;
  require(submit(exact, &change, 1).status == UpdateStatus::NoChange,
          "identical exact metadata was not a no-op");
  stable.unchanged(exact);
  const auto reject = [&](UpdateCommand invalid, Harness &h) {
    refresh(h);
    Snapshot snap(h);
    require(submit(h, &invalid, 1).status == UpdateStatus::InvalidInput,
            "invalid accounting accepted/misclassified");
    snap.unchanged(h);
  };
  reject(replace(exact, 1, 0), exact);
  reject(replace(exact, 1, 4), exact);
  auto invalid = replace(exact, 1, 1);
  invalid.share = .5f;
  reject(invalid, exact);
  invalid = replace(exact, 1, 1);
  invalid.share = std::nextafter(invalid.share, 1.f);
  reject(invalid, exact);
  invalid = replace(exact, 1, 1);
  invalid.exact_mass = 3;
  reject(invalid, exact);
  reject(replace(legacy, 1, 1), legacy);
  invalid = replace(exact, 1, 1);
  ++invalid.first.handle.incarnation;
  reject(invalid, exact);
  invalid = replace(exact, 1, 1);
  ++invalid.first.handle.context;
  reject(invalid, exact);
  invalid = replace(exact, 1, 1);
  --invalid.generation;
  stable = Snapshot(exact);
  require(submit(exact, &invalid, 1).status == UpdateStatus::Stale,
          "stale count row accepted");
  stable.unchanged(exact);
  invalid = replace(exact, wide_row, wide_mass - 3);
  stable = Snapshot(exact);
  PreparedFixtureAccess::inject_failure(exact, 1);
  require(submit(exact, &invalid, 1).status == UpdateStatus::InvalidInput,
          "preclaim failure injection failed");
  PreparedFixtureAccess::inject_failure(exact, 0);
  stable.unchanged(exact);
  invalid = {};
  invalid.operation = UpdateOp::DefineParticle;
  invalid.address = 99999;
  invalid.exact_mass = 0;
  reject(invalid, exact);
  invalid = {};
  invalid.operation = UpdateOp::DefineField;
  invalid.address = 8;
  invalid.exact_count = 1;
  reject(invalid, exact);
  change = replace(exact, 1, std::nullopt);
  committed = Snapshot(exact);
  require(submit(exact, &change, 1).status == UpdateStatus::Ok &&
              !s.source_counts[1],
          "replace silently retained old count");
  committed.physical(exact);
  change = replace(exact, 1, 1);
  require(submit(exact, &change, 1).status == UpdateStatus::Ok,
          "count restore failed");
  // Numerical edits use exactly the same old f32 share on both paths. No count
  // cap spans unrelated field contexts and no new physics weighting is used.
  change = replace(exact, 1, 2);
  change.share = float(2.0 / 3.0);
  auto unknown = replace(legacy, 1, std::nullopt);
  unknown.share = change.share;
  require(submit(exact, &change, 1).status == UpdateStatus::Ok &&
              submit(legacy, &unknown, 1).status == UpdateStatus::Ok,
          "valid source count/share edit failed");
  require(s.source_counts[5] == 1,
          "editing one copy changed another source occurrence");
  parity(exact, legacy, a, b);
  change = replace(exact, 1, 1);
  change.share = float(1.0 / 3.0);
  unknown = replace(legacy, 1, std::nullopt);
  unknown.share = change.share;
  require(submit(exact, &change, 1).status == UpdateStatus::Ok &&
              submit(legacy, &unknown, 1).status == UpdateStatus::Ok,
          "source share restoration failed");
  parity(exact, legacy, a, b);
  // Remove the wide occurrence and all its ordered rows, then reuse its sole
  // free record. Survivor metadata must keep its own logical occurrence.
  for (int side = 0; side < 2; ++side) {
    auto &h = side ? legacy : exact;
    auto &input = side ? b : a;
    std::size_t n = 0;
    const auto &storage = h.prepared_storage();
    for (std::size_t row = 0; row < storage.live.edges; ++row)
      if (storage.edges[row].first.record == originals[side].record) {
        auto &c = input.commands[n++];
        c = {};
        c.operation = UpdateOp::RemoveParticipation;
        c.row = row;
        c.generation = storage.generation;
      }
    auto &c = input.commands[n++];
    c = {};
    c.operation = UpdateOp::RetireParticle;
    c.first = UpdateRef::existing(input.particles.back());
    require(submit(h, input.commands.data(), n).status == UpdateStatus::Ok,
            "source retirement failed");
    require(!storage.source_masses[originals[side].record],
            "retirement retained structural denominator");
    for (std::size_t row = storage.live.edges;
         row < storage.source_counts.size(); ++row)
      require(!storage.source_counts[row],
              "compacted tail retained exact counts");
  }
  const auto cycles_start = std::chrono::steady_clock::now();
  for (int cycle = 0; cycle < 32; ++cycle) {
    for (int side = 0; side < 2; ++side) {
      auto &h = side ? legacy : exact;
      auto &input = side ? b : a;
      auto &define = input.commands[0];
      define = {};
      define.operation = UpdateOp::DefineParticle;
      define.address = int(l[Budget::Particles]) - 1;
      if (!side && (cycle & 1))
        define.exact_mass = src.particles[0].exact_mass;
      auto &link = input.commands[1];
      link = {};
      link.operation = UpdateOp::InsertParticipation;
      link.first = UpdateRef::pending(0);
      link.second =
          UpdateRef::existing(input.fields[src.participation.back().group]);
      link.position = h.prepared_storage().live.edges;
      link.ordinal = cycle;
      if (define.exact_mass)
        link.exact_count = *define.exact_mass;
      std::array<float, kParticleFieldCount> body{};
      body[kMass] = 3;
      body[kRadius] = .125f;
      body[kPosX] = float(cycle + 1);
      body[kVelX] = .125f;
      PreparedFixtureAccess::stage_new(h, 0, body);
      require(submit(h, input.commands.data(), 2).status == UpdateStatus::Ok,
              "source recycled definition failed");
      const auto fresh = h.prepared_storage().resolved[0];
      require(fresh.record == originals[side].record &&
                  fresh.incarnation != originals[side].incarnation &&
                  h.prepared_storage().source_masses[fresh.record] ==
                      define.exact_mass.value_or(0),
              "record reuse inherited/aliased an old denominator");
      input.particles.back() = fresh;
      no_new([&] {
        h.tick(params());
        h.inspect_prepared();
      });
    }
    parity(exact, legacy, a, b);
    for (std::size_t i = 0; i + 1 < src.particles.size(); ++i)
      require(
          s.source_masses[a.particles[i].record] == src.particles[i].exact_mass,
          "recycling changed a survivor denominator");
    std::size_t survivor_row = 0;
    for (const auto &e : src.participation)
      if (e.slot + 1 < int(src.particles.size()))
        require(s.source_counts[survivor_row++] == e.exact_count,
                "compaction/recycling misassociated a survivor count");
    for (int side = 0; side < 2; ++side) {
      auto &h = side ? legacy : exact;
      auto &input = side ? b : a;
      auto &remove = input.commands[0];
      remove = {};
      remove.operation = UpdateOp::RemoveParticipation;
      remove.row = h.prepared_storage().live.edges - 1;
      remove.generation = h.prepared_storage().generation;
      auto &retire = input.commands[1];
      retire = {};
      retire.operation = UpdateOp::RetireParticle;
      retire.first = UpdateRef::existing(input.particles.back());
      require(submit(h, input.commands.data(), 2).status == UpdateStatus::Ok,
              "repeated count retirement failed");
      require(!h.prepared_storage().source_masses[originals[side].record],
              "repeated retirement retained denominator");
    }
  }
  const auto cycles_us = std::chrono::duration<double, std::micro>(
                             std::chrono::steady_clock::now() - cycles_start)
                             .count();
  require(before.same_backing(metrics.sample()) &&
              legacy_before.same_backing(legacy_metrics.sample()),
          "source-count first touch/reuse changed native "
          "backing/layout/compilation");
  const auto ids = runtime.issued_snode_ids();
  auto short_s = l;
  --short_s[Budget::ScratchBytes];
  bool exhausted = false;
  try {
    Harness insufficient(runtime, PreparedCapacity(short_s), path);
  } catch (const CapacityError &e) {
    exhausted = e.failure.budget == Budget::ScratchBytes;
  }
  require(exhausted && ids == runtime.issued_snode_ids(),
          "source metadata scratch rejected after layout mutation");
  before.print("known_after_setup");
  metrics.sample().print("known_after_cycles");
  legacy_before.print("legacy_after_setup");
  legacy_metrics.sample().print("legacy_after_cycles");
  for (const auto &item : s.manifest)
    std::cout << "manifest," << item.name << ',' << item.count << ','
              << item.element_bytes << ',' << item.copies << ',' << item.bytes
              << '\n';
  std::cout << "source_counts," << l[Budget::Particles] << ','
            << (path == HostPath::BulkCopy ? "bulk" : "direct")
            << ",rows=" << src.participation.size()
            << ",S=" << l[Budget::ScratchBytes]
            << ",bindings=" << s.binding_bytes
            << ",command_bytes=" << sizeof(UpdateCommand)
            << ",optional_bytes=" << sizeof(std::optional<std::uint64_t>)
            << ",source_integer_bytes=" << sizeof(std::uint64_t)
            << ",wide_mass=" << wide_mass << ",setup_pair_us=" << setup_us
            << ",loading_pair_us=" << loading_us
            << ",count_only_edit_us=" << edit_us
            << ",cycles_with_inspection_pair_us=" << cycles_us
            << ",cycles=32,cpp_new=0 PASS\n";
}
}  // namespace
int main(int argc, char **argv) {
  try {
    bool direct = false, large = false;
    for (int i = 1; i < argc; ++i) {
      const std::string a = argv[i];
      if (a == "--direct")
        direct = true;
      else if (a == "--large")
        large = true;
      else
        throw std::invalid_argument("expected --direct --large");
    }
    std::cout << std::setprecision(17);
    engine::InstanceSettings settings;
    settings.read_environment = false;
    settings.cpu_threads = 2;
    engine::Runtime runtime(settings);
    require(!runtime.on_cuda(), "CPU fixture unexpectedly selected CUDA");
    std::cout << "actual_backend=cpu\n";
    run(runtime, direct ? HostPath::CpuDirect : HostPath::BulkCopy, large);
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
