#include <cstring>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include "allocation_observation.h"
#include "prepared_metrics.h"
#include "prepared_sources.h"

namespace {
using namespace field;
using namespace field::testing;
using engine::RecordHandle;
void require(bool good, const char *message) {
  if (!good)
    throw std::runtime_error(message);
}
bool same(RecordHandle a, RecordHandle b) {
  return a.context == b.context && a.incarnation == b.incarnation &&
         a.address == b.address && a.record == b.record;
}
template <class F>
void no_new(F f) {
  allocation_observation::calls = 0;
  allocation_observation::bytes = 0;
  allocation_observation::enabled = true;
  try {
    f();
  } catch (...) {
    allocation_observation::enabled = false;
    throw;
  }
  allocation_observation::enabled = false;
  require(allocation_observation::calls == 0,
          "boundary allocated C++ metadata");
}
bool bits(float a, float b) {
  return std::memcmp(&a, &b, sizeof(float)) == 0;
}
bool bytes(const std::vector<float> &a, const std::vector<float> &b) {
  return a.size() == b.size() &&
         (a.empty() ||
          std::memcmp(a.data(), b.data(), a.size() * sizeof(float)) == 0);
}
bool near(float a, float b) {
  return std::isfinite(a) && std::isfinite(b) &&
         std::abs(a - b) <= 2e-5f * std::max({1.f, std::abs(a), std::abs(b)});
}
std::array<float, kParticleFieldCount> body(float x, float mass = 1) {
  std::array<float, kParticleFieldCount> v{};
  v[kPosX] = x;
  v[kMass] = mass;
  v[kRadius] = 0.01f;
  return v;
}
UpdateCommand define(bool particle, int address) {
  UpdateCommand c;
  c.operation = particle ? UpdateOp::DefineParticle : UpdateOp::DefineField;
  c.address = address;
  c.kind = particle ? 1 : 2;
  return c;
}
UpdateCommand edge(UpdateRef p,
                   UpdateRef g,
                   std::size_t position,
                   std::uint64_t ordinal = 0) {
  UpdateCommand c;
  c.operation = UpdateOp::InsertParticipation;
  c.first = p;
  c.second = g;
  c.position = position;
  c.ordinal = ordinal;
  return c;
}
UpdateCommand remove(bool participation,
                     std::size_t row,
                     std::uint64_t generation) {
  UpdateCommand c;
  c.operation =
      participation ? UpdateOp::RemoveParticipation : UpdateOp::RemoveContact;
  c.row = row;
  c.generation = generation;
  return c;
}
UpdateCommand retire(bool particle, RecordHandle h) {
  UpdateCommand c;
  c.operation = particle ? UpdateOp::RetireParticle : UpdateOp::RetireField;
  c.first = UpdateRef::existing(h);
  return c;
}
BudgetDemand scratch(BudgetDemand l, bool cuda = false) {
  l[Budget::ScratchBytes] = detail::PreparedStorage::required_scratch(l, cuda);
  return l;
}
// Compare complete committed numerical/activity state, including sleeping and
// untouched rounding history. Scratch candidates/results are intentionally not
// part of a committed snapshot.
struct Snapshot {
  std::vector<float> particles, groups, determined, motion;
  std::vector<int> state, flags;
  std::array<float, 11> focus;
  float mass;
  std::uint64_t generation;
  detail::PreparedCounts counts;
  std::vector<RecordHandle> particle_handles, field_handles, focal;
  std::vector<detail::RelationshipHeader> edges, contacts;
  explicit Snapshot(Harness &h)
      : particles(h.particle_data),
        groups(h.group_data),
        determined(h.determined()),
        motion(h.prepared_storage().inspection_motion),
        state(h.prepared_storage().inspection_state),
        flags(h.prepared_storage().inspection_flags),
        focus(h.prepared_storage().inspection_focus),
        mass(h.universal_data[0]),
        generation(h.prepared_storage().generation),
        counts(h.prepared_storage().live),
        particle_handles(h.prepared_storage().particle_headers),
        field_handles(h.prepared_storage().field_headers),
        focal(h.prepared_storage().focal),
        edges(h.prepared_storage().edges),
        contacts(h.prepared_storage().contacts) {
  }
  void unchanged(Harness &h) const {
    const auto &s = h.prepared_storage();
    require(
        counts.particles == s.live.particles &&
            counts.fields == s.live.fields && counts.edges == s.live.edges &&
            counts.contacts == s.live.contacts && counts.focus == s.live.focus,
        "rejection changed committed counts");
    for (std::size_t row = 0; row < particle_handles.size(); ++row)
      require(same(particle_handles[row], s.particle_headers[row]),
              "rejection changed particle handle");
    for (std::size_t row = 0; row < field_handles.size(); ++row)
      require(same(field_handles[row], s.field_headers[row]),
              "rejection changed field handle");
    for (std::size_t row = 0; row < focal.size(); ++row)
      require(same(focal[row], s.focal[row]),
              "rejection changed focal selection");
    const auto ledger_same = [](const auto &before, const auto &after) {
      for (std::size_t row = 0; row < before.size(); ++row)
        if (!same(before[row].first, after[row].first) ||
            !same(before[row].second, after[row].second) ||
            before[row].ordinal != after[row].ordinal ||
            !bits(before[row].share, after[row].share))
          return false;
      return true;
    };
    require(ledger_same(edges, s.edges) && ledger_same(contacts, s.contacts),
            "rejection changed committed ledger");
    require(bytes(particles, h.particle_data) && bytes(groups, h.group_data) &&
                bytes(determined, h.determined()) &&
                bytes(motion, s.inspection_motion) &&
                state == s.inspection_state && flags == s.inspection_flags &&
                std::memcmp(focus.data(), s.inspection_focus.data(),
                            sizeof(focus)) == 0 &&
                bits(mass, h.universal_data[0]) && generation == s.generation &&
                h.ready(),
            "ordinary rejection/no-op changed committed state");
  }
};
void repeated(engine::Runtime &runtime,
              HostPath path,
              BudgetDemand limits,
              const FixtureSource &source,
              const char *label) {
  limits = scratch(limits, runtime.on_cuda());
  const auto setup_start = std::chrono::steady_clock::now();
  Harness h(runtime, PreparedCapacity(limits), path);
  const auto setup_us = std::chrono::duration_cast<std::chrono::microseconds>(
                            std::chrono::steady_clock::now() - setup_start)
                            .count();
  Parameters params;
  params.gravity = 0.0001f;
  params.dt = 0.015625f;
  params.contact_stiffness = 0.001f;
  params.reach_strength = 0;
  PreparedFixtureAccess::Input init(
      limits[Budget::Particles], limits[Budget::Fields], limits[Budget::Batch],
      limits[Budget::Focus]);
  no_new([&] {
    PreparedFixtureAccess::load(h, source, init, params, true);
    h.inspect_prepared();
  });
  const auto definition =
      source;  // immutable reusable definition/source content
  const auto first = init.particle_rows[0], other = init.particle_rows[1];
  require(!same(first, other), "simultaneous occurrences share a handle");
  h.tick(params);
  h.inspect_prepared();
  prepared_test::Metrics metrics(runtime, PreparedFixtureAccess::nodes(h));
  const auto prepared = metrics.sample();
  RecordHandle previous_p, previous_g, retired_handle;
  const bool retain_new_field =
      source.kinds.size() + 2 > limits[Budget::Fields];
  std::array<UpdateCommand, 16> commands{};
  std::vector<float> final_start;
  std::uint64_t visits = 0, indexes = 0, uploads = 0, reads = 0, syncs = 0,
                collections = 0;
  std::int64_t update_us = 0, first_us = 0, tick_us = 0, inspect_us = 0;
  for (int cycle = 0; cycle < 32; ++cycle) {
    const auto &s = h.prepared_storage();
    Snapshot before(h);
    const auto old_generation = s.generation;
    auto original = std::find_if(
        s.edges.begin(), s.edges.begin() + s.live.edges, [&](const auto &e) {
          return same(e.first, first) && e.second.address >= 0 &&
                 std::size_t(e.second.address) < source.kinds.size() &&
                 source.kinds[e.second.address] == 3;
        });
    if (original == s.edges.begin() + s.live.edges)
      original =
          std::find_if(s.edges.begin(), s.edges.begin() + s.live.edges,
                       [&](const auto &e) { return same(e.first, first); });
    require(original != s.edges.begin() + s.live.edges,
            "original occurrence lost its participation");
    const auto original_row = std::size_t(original - s.edges.begin());
    const int changed_group = original->second.record;
    std::size_t count = 0;
    commands[count++] = define(true, int((std::uint64_t(cycle + 91) * 7919) %
                                         limits[Budget::Potential]));
    const bool create_field = !retain_new_field || previous_g.record < 0;
    const int retired_field = !retain_new_field ? previous_g.record : -1;
    if (create_field)
      commands[count++] = define(
          false, int(source.kinds.size()) + (retain_new_field ? 0 : cycle % 2));
    commands[count++] = edge(
        UpdateRef::pending(0),
        create_field ? UpdateRef::pending(1) : UpdateRef::existing(previous_g),
        0, 0);
    commands[count++] = edge(UpdateRef::pending(0),
                             UpdateRef::existing(original->second), 1, 0);
    UpdateCommand contact;
    contact.operation = UpdateOp::InsertContact;
    contact.first =
        cycle % 2 ? UpdateRef::pending(0) : UpdateRef::existing(first);
    contact.second =
        cycle % 2 ? UpdateRef::existing(first) : UpdateRef::pending(0);
    contact.position = 0;
    commands[count++] = contact;
    auto replacement =
        edge(UpdateRef::existing(original->first),
             UpdateRef::existing(original->second), 0, original->ordinal);
    replacement.operation = UpdateOp::ReplaceParticipation;
    replacement.row = original_row;
    replacement.generation = old_generation;
    replacement.share = cycle % 2 ? 1.f : 0.5f;
    commands[count++] = replacement;
    if (previous_p.record >= 0) {
      const auto e = std::find_if(
          s.edges.begin(), s.edges.begin() + s.live.edges,
          [&](const auto &row) { return same(row.first, previous_p); });
      const auto c = std::find_if(
          s.contacts.begin(), s.contacts.begin() + s.live.contacts,
          [&](const auto &row) {
            return same(row.first, previous_p) || same(row.second, previous_p);
          });
      require(e != s.edges.begin() + s.live.edges &&
                  c != s.contacts.begin() + s.live.contacts,
              "previous new occurrence disconnected");
      commands[count++] = remove(true, e - s.edges.begin(), old_generation);
      const auto e2 = std::find_if(
          e + 1, s.edges.begin() + s.live.edges,
          [&](const auto &row) { return same(row.first, previous_p); });
      require(e2 != s.edges.begin() + s.live.edges,
              "shared occurrence membership was lost");
      commands[count++] = remove(true, e2 - s.edges.begin(), old_generation);
      commands[count++] = remove(false, c - s.contacts.begin(), old_generation);
      commands[count++] = retire(true, previous_p);
      if (!retain_new_field)
        commands[count++] = retire(false, previous_g);
      retired_handle = previous_p;
    }
    // A third copy of the same body definition has independent execution state.
    auto newcomer = source.particles[0];
    newcomer[kPosX] = 4.f + cycle * 0.125f;
    newcomer[kVelX] = -0.0078125f;
    if (cycle == 16 && path == HostPath::BulkCopy) {
      std::fill(h.particle_data.begin(), h.particle_data.end(), -777.f);
      std::fill(h.group_data.begin(), h.group_data.end(), -999.f);
    }
    UpdateResult result;
    const auto update_start = std::chrono::steady_clock::now();
    no_new([&] {
      PreparedFixtureAccess::stage_new(h, 0, newcomer);
      result = h.update_prepared(old_generation, commands.data(), count);
    });
    const auto elapsed_update =
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - update_start)
            .count();
    update_us += elapsed_update;
    if (!cycle)
      first_us = elapsed_update;
    if (result.status != UpdateStatus::Ok)
      std::cerr << "failed profile=" << label << ",cycle=" << cycle
                << ",status=" << int(result.status)
                << ",budget=" << budget_name(result.failure.budget)
                << ",required=" << result.failure.required
                << ",capacity=" << result.failure.capacity
                << ",sourceG=" << source.kinds.size() << '\n';
    require(result.status == UpdateStatus::Ok &&
                result.generation == old_generation + 1 && h.ready(),
            "repeated coherent commit failed");
    no_new([&] { h.inspect_prepared(); });
    require(prepared.same_backing(metrics.sample()),
            "update or inspection acquired native backing/layout/compilation");
    const auto &after = h.prepared_storage();
    previous_p = after.resolved[0];
    if (create_field)
      previous_g = after.resolved[1];
    require(previous_p.context && previous_g.context &&
                !same(previous_p, first) && !same(previous_p, other),
            "new occurrence reused live identity");
    require(after.live.particles == source.particles.size() + 1 &&
                after.live.fields == source.kinds.size() + 1 &&
                after.live.edges == source.edges.size() + 2 &&
                after.live.contacts == source.contacts.size() + 1,
            "cross-family counts are not coherent");
    require(after.edges[0].first.record == previous_p.record &&
                after.edges[0].second.record == previous_g.record &&
                after.edges[0].ordinal == 0 &&
                after.inspected_generation == result.generation,
            "ordered new relationship or inspection generation wrong");
    for (int row = 0; row < int(limits[Budget::Particles]); ++row) {
      if (!(before.flags[row] & 1) || row == retired_handle.record)
        continue;
      for (int f = 0; f < kParticleFieldCount; ++f)
        require(bits(h.particle(f, row),
                     before.particles[f * limits[Budget::Particles] + row]),
                "survivor body state replayed or advanced at commit");
      require(bits(h.determined()[row], before.determined[row]),
              "survivor determiner changed at commit");
      for (int f = 0; f < 2; ++f)
        require(after.inspection_state[f * limits[Budget::Particles] + row] ==
                    before.state[f * limits[Budget::Particles] + row],
                "survivor motion flags changed");
      for (int f = 0; f < 3; ++f)
        require(
            bits(after.inspection_motion[f * limits[Budget::Particles] + row],
                 before.motion[f * limits[Budget::Particles] + row]),
            "survivor displacement changed");
    }
    for (int row = 0; row < int(limits[Budget::Fields]); ++row) {
      if (!(before.flags[limits[Budget::Particles] + row] & 1) ||
          row == changed_group || row == previous_g.record ||
          row == retired_field)
        continue;
      for (int f = 0; f < 8; ++f)
        require(bits(h.group(f, row),
                     before.groups[f * limits[Budget::Fields] + row]),
                "untouched group changed through ledger compaction");
      for (int f = 0; f < 6; ++f)
        require(bits(after.inspection_motion[3 * limits[Budget::Particles] +
                                             f * limits[Budget::Fields] + row],
                     before.motion[3 * limits[Budget::Particles] +
                                   f * limits[Budget::Fields] + row]),
                "untouched residue/history changed");
      require(after.inspection_flags[limits[Budget::Particles] + row] ==
                  before.flags[limits[Budget::Particles] + row],
              "untouched field activity changed");
      for (int f = 0; f < 2; ++f)
        require(after.inspection_state[2 * limits[Budget::Particles] +
                                       f * limits[Budget::Fields] + row] ==
                    before.state[2 * limits[Budget::Particles] +
                                 f * limits[Budget::Fields] + row],
                "untouched field flags changed");
    }
    require(h.group(kCenM, previous_g.record) == newcomer[kMass] &&
                h.group(kCenX, previous_g.record) == newcomer[kPosX],
            "new field did not use current independent occurrence");
    double universal = 0;
    for (int row = 0; row < int(limits[Budget::Particles]); ++row)
      if (after.particle_headers[row].record >= 0)
        universal += h.particle(kMass, row);
    require(near(h.universal_data[0], float(universal)),
            "universal mass lost an occurrence or counted an empty row");
    for (int group : {changed_group, previous_g.record}) {
      double mass = 0, x = 0;
      for (std::size_t e = 0; e < after.live.edges; ++e)
        if (after.edges[e].second.record == group) {
          const auto &edge = after.edges[e];
          const double m = h.particle(kMass, edge.first.record) * edge.share;
          mass += m;
          x += m * h.particle(kPosX, edge.first.record);
        }
      require(near(h.group(kCenM, group), float(mass)) &&
                  near(h.group(kCenX, group), float(x / mass)),
              "repaired centre/mass differs from independent current-state "
              "reduction");
    }
    if (retired_handle.record >= 0) {
      const auto stale = retire(true, retired_handle);
      UpdateResult rejected;
      no_new(
          [&] { rejected = h.update_prepared(after.generation, &stale, 1); });
      require(rejected.status == UpdateStatus::Stale && h.ready(),
              "recycled record accepted retired incarnation");
    }
    if (cycle == 31)
      final_start = h.particle_data;
    const auto tick_start = std::chrono::steady_clock::now();
    no_new([&] { h.tick(params); });
    tick_us += std::chrono::duration_cast<std::chrono::microseconds>(
                   std::chrono::steady_clock::now() - tick_start)
                   .count();
    const auto inspection_start = std::chrono::steady_clock::now();
    no_new([&] { h.inspect_prepared(); });
    inspect_us += std::chrono::duration_cast<std::chrono::microseconds>(
                      std::chrono::steady_clock::now() - inspection_start)
                      .count();
    require(prepared.same_backing(metrics.sample()),
            "updated tick obtained backing/layout/compilation");
    visits += result.structural_visits;
    indexes += result.index_visits;
    uploads += result.upload_bytes;
    reads += result.readback_bytes;
    syncs += result.synchronizations;
    collections += result.collections;
  }
  const auto final = metrics.sample();
  // The full boundary history checks above precede this separate comparison
  // against established kernels. A reseeded trajectory is not the sole oracle.
  Harness reference(runtime, int(limits[Budget::Particles]),
                    int(limits[Budget::Fields]), int(source.edges.size() + 2),
                    int(source.contacts.size() + 1), path);
  reference.particle_data = final_start;
  const auto &s = h.prepared_storage();
  for (std::size_t e = 0; e < s.live.edges; ++e) {
    reference.edge_int(kEdgeParticle, int(e)) = s.edges[e].first.record;
    reference.edge_int(kEdgeGroup, int(e)) = s.edges[e].second.record;
    reference.edge_float(kShare, int(e)) = s.edges[e].share;
  }
  for (std::size_t c = 0; c < s.live.contacts; ++c) {
    reference.contact_int(kContactA, int(c)) = s.contacts[c].first.record;
    reference.contact_int(kContactB, int(c)) = s.contacts[c].second.record;
  }
  reference.publish(params);
  reference.tick(params);
  reference.download();
  for (int row = 0; row < int(limits[Budget::Particles]); ++row)
    for (int f = 0; f < 6; ++f)
      require(near(h.particle(f, row), reference.particle(f, row)),
              "next-tick body result differs from existing kernels");
  require(source.particles == definition.particles &&
              source.kinds == definition.kinds &&
              source.edges.size() == definition.edges.size(),
          "reusable definition was mutated");
  for (std::size_t e = 0; e < source.edges.size(); ++e)
    require(source.edges[e].particle == definition.edges[e].particle &&
                source.edges[e].group == definition.edges[e].group &&
                source.edges[e].ordinal == definition.edges[e].ordinal &&
                bits(source.edges[e].share, definition.edges[e].share),
            "definition relationship was mutated");
  std::cout << "updates,profile=" << label
            << ",path=" << (path == HostPath::CpuDirect ? "direct" : "bulk")
            << ",cycles=32,generation=" << s.generation
            << ",scratch=" << limits[Budget::ScratchBytes]
            << ",bindings=" << s.binding_bytes
            << ",structural_visits=" << visits << ",index_visits=" << indexes
            << ",upload_bytes=" << uploads << ",readback_bytes=" << reads
            << ",syncs=" << syncs << ",collections=" << collections
            << ",C++new=0\n";
  std::cout << "phase_us,setup=" << setup_us << ",first_update=" << first_us
            << ",31_ordinary_updates=" << update_us - first_us
            << ",32_ticks=" << tick_us << ",32_inspections=" << inspect_us
            << '\n';
  prepared.print("before_updates");
  final.print("after_32_updates_ticks_inspections");
  for (const auto &entry : s.manifest)
    std::cout << "workspace," << entry.name << ',' << entry.count << ','
              << entry.element_bytes << ',' << entry.copies << ','
              << entry.bytes << '\n';
}
FixtureSource small_source() {
  FixtureSource s;
  s.particles = {body(0), body(2), body(8)};
  s.kinds = {1, 2};
  s.edges = {{0, 0, 1, 0}, {2, 1, 1, 0}};
  s.focus = {0};
  return s;
}
void load_small(Harness &h,
                const FixtureSource &source,
                const Parameters &params) {
  const auto &l = h.prepared_storage().capacity.limits();
  PreparedFixtureAccess::Input init(l[Budget::Particles], l[Budget::Fields],
                                    l[Budget::Batch], l[Budget::Focus]);
  PreparedFixtureAccess::load(h, source, init, params, false);
  h.tick(params);
  h.inspect_prepared();
}
void reject_and_preserve(
    Harness &h,
    const UpdateCommand *commands,
    std::size_t count,
    UpdateStatus expected,
    Budget budget = Budget::Count,
    std::uint64_t generation = std::numeric_limits<std::uint64_t>::max()) {
  Snapshot before(h);
  UpdateResult result;
  no_new([&] {
    result = h.update_prepared(
        generation == std::numeric_limits<std::uint64_t>::max()
            ? before.generation
            : generation,
        commands, count);
  });
  require(result.status == expected &&
              (budget == Budget::Count || result.failure.budget == budget),
          "wrong ordinary update rejection");
  no_new([&] { h.inspect_prepared(); });
  before.unchanged(h);
  if (expected == UpdateStatus::NoChange)
    require(!result.upload_bytes && !result.readback_bytes &&
                !result.synchronizations,
            "no-op performed physical work");
}
void rejections(engine::Runtime &runtime, HostPath path) {
  BudgetDemand limits;
  limits.values = {1048576, 4, 3, 3, 1, 100, 1, 4, 0};
  Harness h(runtime, PreparedCapacity(scratch(limits, runtime.on_cuda())),
            path);
  Parameters params;
  params.gravity = params.contact_stiffness = params.reach_strength = 0;
  load_small(h, small_source(), params);
  const auto &s = h.prepared_storage();
  const auto gen = s.generation;
  const auto p = s.edges[0].first, g = s.edges[0].second;
  UpdateCommand replace =
      edge(UpdateRef::existing(p), UpdateRef::existing(g), 0);
  replace.operation = UpdateOp::ReplaceParticipation;
  replace.row = 0;
  replace.generation = gen;
  reject_and_preserve(h, nullptr, 0, UpdateStatus::NoChange);
  reject_and_preserve(h, &replace, 1, UpdateStatus::NoChange);
  reject_and_preserve(h, &replace, 1, UpdateStatus::Stale, Budget::Count,
                      gen - 1);
  auto cmd = replace;
  cmd.generation = gen - 1;
  reject_and_preserve(h, &cmd, 1, UpdateStatus::Stale);
  cmd = replace;
  ++cmd.first.handle.context;
  reject_and_preserve(h, &cmd, 1, UpdateStatus::InvalidInput);
  cmd = replace;
  ++cmd.first.handle.incarnation;
  reject_and_preserve(h, &cmd, 1, UpdateStatus::InvalidInput);
  cmd = retire(true, p);
  reject_and_preserve(h, &cmd, 1, UpdateStatus::InvalidInput);
  cmd = retire(false, g);
  reject_and_preserve(h, &cmd, 1, UpdateStatus::InvalidInput);
  cmd.operation = UpdateOp::ReplaceFocus;
  reject_and_preserve(h, &cmd, 1, UpdateStatus::Unsupported);
  cmd = remove(true, 0, gen);
  reject_and_preserve(h, &cmd, 1, UpdateStatus::InvalidNumerics);
  cmd = replace;
  cmd.share = std::numeric_limits<float>::quiet_NaN();
  reject_and_preserve(h, &cmd, 1, UpdateStatus::InvalidInput);
  cmd = edge(UpdateRef::existing(p), UpdateRef::existing(g), 1000);
  reject_and_preserve(h, &cmd, 1, UpdateStatus::InvalidInput);
  cmd = edge(UpdateRef::pending(0), UpdateRef::existing(g), 0);
  reject_and_preserve(h, &cmd, 1, UpdateStatus::InvalidInput);
  std::array<UpdateCommand, 5> batch{};
  batch[0] = replace;
  batch[1] = remove(true, 0, gen);
  reject_and_preserve(h, batch.data(), 2, UpdateStatus::InvalidInput);
  batch[0] = define(true, 777);
  batch[1] = define(true, 778);
  batch[2] = retire(true, s.particle_headers[1]);
  reject_and_preserve(h, batch.data(), 3, UpdateStatus::CapacityExhausted,
                      Budget::Particles);
  batch[0] = define(false, 2);
  batch[1] = define(false, 2);
  reject_and_preserve(h, batch.data(), 2, UpdateStatus::CapacityExhausted,
                      Budget::Fields);
  batch[0] = edge(UpdateRef::existing(p), UpdateRef::existing(g), 0);
  batch[1] = batch[0];
  batch[1].position = 1;
  reject_and_preserve(h, batch.data(), 2, UpdateStatus::CapacityExhausted,
                      Budget::Participation);
  batch[0].operation = UpdateOp::InsertContact;
  batch[0].second = UpdateRef::existing(s.particle_headers[1]);
  batch[1] = batch[0];
  batch[1].position = 1;
  reject_and_preserve(h, batch.data(), 2, UpdateStatus::CapacityExhausted,
                      Budget::Contacts);
  reject_and_preserve(h, batch.data(), 5, UpdateStatus::CapacityExhausted,
                      Budget::Batch);
  cmd = define(true, 12345);
  reject_and_preserve(h, &cmd, 1, UpdateStatus::InvalidNumerics);
  // Both commands fit P, but naming the same potential location is invalid.
  batch[0] = define(false, 2);
  batch[1] = define(false, 2);
  // A duplicate claim also has a valid standalone conflict case below.
  cmd = define(false, s.field_headers[0].address);
  reject_and_preserve(h, &cmd, 1, UpdateStatus::InvalidInput);
  cmd = replace;
  cmd.share = 0.5f;
  PreparedFixtureAccess::inject_failure(h, 1);
  reject_and_preserve(h, &cmd, 1, UpdateStatus::InvalidInput);
  PreparedFixtureAccess::inject_failure(h, 0);
  // Logical R peak fails although final references fit and P has spare room.
  auto refs = limits;
  refs[Budget::References] = s.live.references();
  Harness r(runtime, PreparedCapacity(scratch(refs, runtime.on_cuda())), path);
  load_small(r, small_source(), params);
  batch[0] = define(true, 777);
  batch[1] = retire(true, r.prepared_storage().particle_headers[1]);
  PreparedFixtureAccess::stage_new(r, 0, body(3));
  reject_and_preserve(r, batch.data(), 2, UpdateStatus::CapacityExhausted,
                      Budget::References);
  // R alone rejects added participation with all storage families still spare.
  cmd = edge(UpdateRef::existing(r.prepared_storage().edges[0].first),
             UpdateRef::existing(r.prepared_storage().edges[0].second), 0);
  reject_and_preserve(r, &cmd, 1, UpdateStatus::CapacityExhausted,
                      Budget::References);
  auto fields = limits;
  fields[Budget::Fields] = 2;
  Harness peak_fields(
      runtime, PreparedCapacity(scratch(fields, runtime.on_cuda())), path);
  load_small(peak_fields, small_source(), params);
  batch[0] = define(false, 1);
  batch[1] = retire(false, peak_fields.prepared_storage().field_headers[1]);
  reject_and_preserve(peak_fields, batch.data(), 2,
                      UpdateStatus::CapacityExhausted, Budget::Fields);
  // Stale ledger references after a successful compacting commit.
  cmd = replace;
  cmd.share = 0.5f;
  require(h.update_prepared(gen, &cmd, 1).status == UpdateStatus::Ok,
          "valid edit failed after rejection");
  h.inspect_prepared();
  reject_and_preserve(h, &replace, 1, UpdateStatus::Stale);
  std::cout << "ordinary rejection/no-op/independent P,G,E,C,R,B peaks and "
               "stale generation PASS\n";
}
void wakes_and_residue(engine::Runtime &runtime, HostPath path) {
  BudgetDemand limits;
  limits.values = {1048576, 70, 9, 280, 140, 928, 9, 32, 0};
  Parameters params;
  params.gravity = params.contact_stiffness = params.reach_strength = 0;
  for (bool radial : {false, true}) {
    params.reach_strength = radial ? 1 : 0;
    Harness h(runtime, PreparedCapacity(scratch(limits, runtime.on_cuda())),
              path);
    load_small(h, small_source(), params);
    const auto &s = h.prepared_storage();
    const auto p = s.edges[0].first, g = s.edges[0].second;
    require(s.inspection_counts[1] == 0, "stationary fixture failed to sleep");
    std::array<UpdateCommand, 2> duplicate{define(true, 700000),
                                           define(true, 700000)};
    PreparedFixtureAccess::stage_new(h, 0, body(1));
    PreparedFixtureAccess::stage_new(h, 1, body(1));
    reject_and_preserve(h, duplicate.data(), 2, UpdateStatus::InvalidInput);
    auto cmd = edge(UpdateRef::existing(p), UpdateRef::existing(g), 0);
    if (!radial) {
      cmd.operation = UpdateOp::ReplaceParticipation;
      cmd.row = 0;
      cmd.generation = s.generation;
      cmd.share = 0.5f;
    } else
      cmd.first = UpdateRef::existing(s.particle_headers[1]);
    Snapshot before(h);
    require(h.update_prepared(s.generation, &cmd, 1).status == UpdateStatus::Ok,
            "wake fixture edit failed");
    h.inspect_prepared();
    require(h.particle_data == before.particles,
            "repair advanced a particle phase");
    if (radial) {
      require(s.inspection_counts[1] == std::int64_t(s.live.particles) &&
                  s.inspection_focus[0] == 1,
              "focal-origin change did not wake globally");
      h.tick(params);
      h.inspect_prepared();
      require(h.demand().particles_evaluated == s.live.particles,
              "written focal control masked next begin wake");
      cmd = define(true, 900000);
      PreparedFixtureAccess::stage_new(h, 0, body(12));
      require(
          h.update_prepared(s.generation, &cmd, 1).status == UpdateStatus::Ok,
          "radial U edit failed");
      h.inspect_prepared();
      require(s.inspection_counts[1] == std::int64_t(s.live.particles),
              "universal mass change did not wake global sleepers");
    } else {
      require(s.inspection_flags[p.record] == 3 && s.inspection_flags[1] == 1 &&
                  s.inspection_flags[2] == 1 &&
                  h.group(kCenX, g.record) == before.groups[g.record],
              "same-centre edit failed local structural wake or woke unrelated "
              "sleeper");
      h.tick(params);
      h.inspect_prepared();
      cmd = remove(true, 1, s.generation);
      require(
          h.update_prepared(s.generation, &cmd, 1).status == UpdateStatus::Ok,
          "nonselected removal failed");
      h.inspect_prepared();
      require(s.inspection_flags[2] == 3 && s.inspection_flags[1] == 1 &&
                  h.group(kCenM, 1) == 0,
              "removed field contribution did not wake old member");
      h.tick(params);
      h.inspect_prepared();
      cmd.operation = UpdateOp::InsertContact;
      cmd.first = UpdateRef::existing(s.particle_headers[1]);
      cmd.second = UpdateRef::existing(s.particle_headers[2]);
      cmd.position = 0;
      require(
          h.update_prepared(s.generation, &cmd, 1).status == UpdateStatus::Ok,
          "local contact insert failed");
      h.tick(params);
      h.inspect_prepared();
      cmd = remove(false, 0, s.generation);
      require(
          h.update_prepared(s.generation, &cmd, 1).status == UpdateStatus::Ok,
          "local contact remove failed");
      h.inspect_prepared();
      require(s.inspection_flags[1] == 3 && s.inspection_flags[2] == 3 &&
                  s.inspection_flags[0] == 1,
              "contact removal failed to wake both endpoints locally");
    }
  }
  params.reach_strength = 0;
  auto source = small_source();
  source.particles[0] = body(1048576);
  source.particles[1] = body(1048576.125f, 2);
  source.particles[1][kVelX] = 0.125f;
  source.edges.insert(source.edges.begin() + 1, {1, 0, 1, 1});
  Harness h(runtime, PreparedCapacity(scratch(limits, runtime.on_cuda())),
            path);
  load_small(h, source, params);
  const auto &s = h.prepared_storage();
  const auto p = s.edges[0].first, g = s.edges[0].second;
  require(s.inspection_motion[3 * 70 + g.record] != 0 &&
              h.particle(kPosX, 1) == 1048576.25f,
          "fixture did not retain pending local-f32 motion");
  auto cmd = edge(UpdateRef::existing(p), UpdateRef::existing(g), 0);
  cmd.operation = UpdateOp::ReplaceParticipation;
  cmd.row = 0;
  cmd.generation = s.generation;
  reject_and_preserve(h, &cmd, 1, UpdateStatus::NoChange);
  const float old_centre = h.group(kCenX, g.record);
  cmd.share = 0.5f;
  require(h.update_prepared(s.generation, &cmd, 1).status == UpdateStatus::Ok,
          "pending-residue repair failed");
  h.inspect_prepared();
  require(h.group(kCenX, g.record) == 1048576.25f &&
              h.group(kCenM, g.record) == 2.5f &&
              s.inspection_motion[3 * 70 + g.record] == -0.125f &&
              s.inspection_motion[3 * 70 + 3 * 9 + g.record] == old_centre,
          "fresh reduction double-added old residue or lost previous-centre "
          "history");
  std::cout << "local structural wakes, unchanged sleepers, global U/focus "
               "wakes, exact pending-motion repair PASS\n";
}
void maximum_batches(engine::Runtime &runtime,
                     HostPath path,
                     BudgetDemand limits) {
  Harness h(runtime, PreparedCapacity(scratch(limits, runtime.on_cuda())),
            path);
  const auto &s = h.prepared_storage();
  const auto p = limits[Budget::Particles], b = limits[Budget::Batch];
  std::vector<UpdateCommand> commands(b);
  prepared_test::Metrics metrics(runtime, PreparedFixtureAccess::nodes(h));
  const auto ready = metrics.sample();
  for (std::size_t begin = 0; begin < p; begin += b) {
    const auto count = std::min(b, p - begin);
    UpdateResult result;
    no_new([&] {
      for (std::size_t c = 0; c < count; ++c) {
        commands[c] = define(
            true, int(((begin + c) * 104729 + 29) % limits[Budget::Potential]));
        PreparedFixtureAccess::stage_new(h, int(c), body(float(begin + c)));
      }
      result = h.update_prepared(s.generation, commands.data(), count);
    });
    require(result.status == UpdateStatus::Ok,
            "maximum batch definition failed");
    no_new([&] { h.inspect_prepared(); });
    require(ready.same_backing(metrics.sample()),
            "first/max batch acquired native backing or compilation");
    if (!begin)
      metrics.sample().print("after_first_maximum_update");
  }
  require(s.particle_headers[p - 1].record == int(p - 1) &&
              s.live.particles == p && h.universal_data[0] == float(p),
          "last usable record was not populated by updates");
  const auto old = s.particle_headers[p - 1];
  commands[0] = define(true, 999999);
  commands[1] = retire(true, old);
  reject_and_preserve(h, commands.data(), 2, UpdateStatus::CapacityExhausted,
                      Budget::Particles);
  while (s.live.particles) {
    std::size_t count = 0;
    for (const auto &handle : s.particle_headers)
      if (handle.record >= 0 && count < b)
        commands[count++] = retire(true, handle);
    UpdateResult result;
    no_new([&] {
      result = h.update_prepared(s.generation, commands.data(), count);
    });
    require(result.status == UpdateStatus::Ok && result.collections == 1,
            "maximum native retirement batch failed");
  }
  no_new([&] { h.inspect_prepared(); });
  require(s.inspection_counts == std::array<std::int64_t, 4>{} &&
              h.universal_data[0] == 0 && ready.same_backing(metrics.sample()),
          "retirement retained payload/activity/mass or acquired backing");
  commands[0] = define(true, old.address);
  UpdateResult result;
  no_new([&] {
    PreparedFixtureAccess::stage_new(h, 0, body(1));
    result = h.update_prepared(s.generation, commands.data(), 1);
  });
  require(result.status == UpdateStatus::Ok && !same(s.resolved[0], old),
          "reuse retained old native incarnation");
  no_new([&] { h.inspect_prepared(); });
  commands[0] = retire(true, old);
  reject_and_preserve(h, commands.data(), 1, UpdateStatus::Stale);
  require(ready.same_backing(metrics.sample()),
          "recycled use acquired native backing");
  std::cout << "maximum-batch,P=" << p << ",B=" << b
            << ",full-bank,last-record,retirement,reuse,C++new=0 PASS\n";
}
void failures(engine::Runtime &runtime, HostPath path) {
  BudgetDemand limits;
  limits.values = {1048576, 70, 9, 280, 140, 928, 9, 32, 0};
  Parameters params;
  params.gravity = params.contact_stiffness = params.reach_strength = 0;
  for (int point = 2; point <= 4; ++point) {
    Harness h(runtime, PreparedCapacity(scratch(limits, runtime.on_cuda())),
              path);
    load_small(h, small_source(), params);
    const auto generation = h.prepared_storage().generation;
    std::array<UpdateCommand, 3> commands{
        define(true, 12345), define(false, 7),
        edge(UpdateRef::pending(0), UpdateRef::pending(1), 0)};
    PreparedFixtureAccess::stage_new(h, 0, body(5));
    PreparedFixtureAccess::inject_failure(h, point);
    bool failed = false;
    try {
      h.update_prepared(generation, commands.data(), commands.size());
    } catch (const std::runtime_error &) {
      failed = true;
    }
    require(failed && !h.ready() &&
                h.prepared_storage().generation == generation &&
                h.prepared_storage().inspected_generation == generation,
            "post-mutation failure returned healthy partial generation");
    failed = false;
    try {
      h.tick(params);
    } catch (const std::logic_error &) {
      failed = true;
    }
    require(failed, "unready tick accepted");
    failed = false;
    try {
      h.inspect_prepared();
    } catch (const std::logic_error &) {
      failed = true;
    }
    require(failed, "unready inspection accepted");
    require(h.update_prepared(generation, nullptr, 0).status ==
                UpdateStatus::Unready,
            "unready update accepted");
  }
  std::cout << "failure after claims/numerical/topology writes refuses "
               "tick/inspection and cleans up PASS\n";
}
}  // namespace
int main(int argc, char **argv) {
  try {
    bool direct = false, large = false, letters = false, cuda = false;
    for (int a = 1; a < argc; ++a) {
      const std::string arg = argv[a];
      if (arg == "--direct")
        direct = true;
      else if (arg == "--large")
        large = true;
      else if (arg == "--cuda")
        cuda = true;
      else if (arg == "--letters")
        letters = true;
      else
        throw std::invalid_argument("unknown prepared update test argument");
    }
    engine::InstanceSettings settings;
    settings.read_environment = false;
    settings.cuda = cuda;
    if (cuda)
      settings.device_memory_gb = 0.5;
    require(!(cuda && direct), "direct path is CPU only");
    settings.cpu_threads = 2;
    engine::Runtime runtime(settings);
    require(!cuda || runtime.on_cuda(),
            "requested CUDA, actual backend is CPU");
    std::cout << "actual_backend=" << (runtime.on_cuda() ? "cuda" : "cpu")
              << '\n';
    const auto path = direct ? HostPath::CpuDirect : HostPath::BulkCopy;
    BudgetDemand limits;
    limits.values =
        large ? std::array<std::uint64_t, 9>{16777216, 4096, 257, 16384, 8192,
                                             53762,    257,  256, 0}
              : std::array<std::uint64_t, 9>{1048576, 70, 9,  280, 140,
                                             928,     9,  32, 0};
    if (letters)
      for (int mode = 0; mode < 3; ++mode)
        repeated(runtime, path, limits,
                 prepared_sources::letters(runtime, mode),
                 mode == 0   ? "letters"
                 : mode == 1 ? "bytes"
                             : "mixed");
    else
      repeated(runtime, path, limits, prepared_sources::working(limits),
               large ? "working-set" : "padded");
    if (!letters)
      maximum_batches(runtime, path, limits);
    if (!large && !letters) {
      rejections(runtime, path);
      wakes_and_residue(runtime, path);
      failures(runtime, path);
    }
  } catch (const std::exception &e) {
    std::cerr << "prepared_update: " << e.what() << '\n';
    return 1;
  }
}
