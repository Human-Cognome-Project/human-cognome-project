#pragma once

#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>

#include "assembly/active_workbench.h"
#include "engine/packed_view.h"
#include "harness.h"

namespace active_checks {
using namespace assembly;
inline int checks = 0;
inline void check(bool ok, const char *text) {
  ++checks;
  harness::record(ok, text, "contract failed");
  if (!ok) throw warm::Error(text);
}
template <class F>
void rejects(F f, const char *text) {
  bool rejected = false;
  try {
    f();
  } catch (const std::exception &) {
    rejected = true;
  }
  check(rejected, text);
}
inline warm::ObjectKey object(const std::string &id, warm::Uuid scope = {}) {
  return {scope, *codec::decode_token_id(id)};
}
inline bool bits(const std::vector<float> &a, const std::vector<float> &b) {
  return a.size() == b.size() &&
         !std::memcmp(a.data(), b.data(), a.size() * sizeof(float));
}
inline void verify(const ActiveReport &seed, const ActiveReport &tick,
                   const field::Parameters &parameters) {
  const auto &p = seed.prepared;
  const auto P = p.capacity, G = int(p.groups.size());
  std::vector<double> mass(G), x(G), y(G), z(G), fx(P), fy(P), fz(P);
  for (const auto &e : p.participation) {
    const auto &a = seed.particles;
    const double m = a[field::kMass * P + e.slot] * double(e.share);
    mass[e.group] += m;
    x[e.group] += a[field::kPosX * P + e.slot] * m;
    y[e.group] += a[field::kPosY * P + e.slot] * m;
    z[e.group] += a[field::kPosZ * P + e.slot] * m;
  }
  for (int g = 0; g < G; ++g) {
    if (mass[g]) {
      x[g] /= mass[g];
      y[g] /= mass[g];
      z[g] /= mass[g];
    }
    check(std::abs(seed.groups[field::kCenM * G + g] - mass[g]) < 1e-5 &&
              std::abs(seed.groups[field::kCenX * G + g] - x[g]) < 1e-5 &&
              std::abs(seed.groups[field::kCenY * G + g] - y[g]) < 1e-5 &&
              std::abs(seed.groups[field::kCenZ * G + g] - z[g]) < 1e-5,
          "seeded inclusive centroid matches independent weighted sum");
  }
  for (const auto &e : p.participation) {
    const double dx = x[e.group] - seed.particles[field::kPosX * P + e.slot];
    const double dy = y[e.group] - seed.particles[field::kPosY * P + e.slot];
    const double dz = z[e.group] - seed.particles[field::kPosZ * P + e.slot];
    const double d2 = dx * dx + dy * dy + dz * dz;
    const double m =
        seed.particles[field::kMass * P + e.slot] * double(e.share);
    if (!d2 || mass[e.group] == m) continue;
    const double f =
        parameters.gravity * m * mass[e.group] / (d2 * std::sqrt(d2));
    fx[e.slot] += f * dx;
    fy[e.slot] += f * dy;
    fz[e.slot] += f * dz;
  }
  bool correct = true, finite = true, inactive = true, acceleration = true;
  for (int i = 0; i < P; ++i) {
    for (int axis = 0; axis < 3; ++axis) {
      const double f = axis == 0 ? fx[i] : (axis == 1 ? fy[i] : fz[i]);
      correct &=
          std::abs(tick.particles[(field::kForceX + axis) * P + i] - f) <= 1e-6;
      const auto m = seed.particles[field::kMass * P + i];
      // dt=0 in this first-force fixture: no integration or damping movement.
      if (m)
        acceleration &= tick.particles[(field::kVelX + axis) * P + i] ==
                        seed.particles[(field::kVelX + axis) * P + i];
    }
    for (int f = 0; f < field::kParticleFieldCount; ++f) {
      const float v = tick.particles[f * P + i];
      finite &= std::isfinite(v);
      if (seed.particles[field::kMass * P + i] == 0) inactive &= v == 0;
    }
  }
  check(correct, "first force matches unchanged field law within 1e-6");
  check(finite && inactive && acceleration,
        "finite state, inert slots and zero-dt state coherence");
}
inline LocalInput local_fixture(const ActiveAssembly &a) {
  const auto version = warm::new_uuid();
  const auto ref = [&](const char *id) {
    return LocalRef{object(id, a.instance()), version, a.owner()};
  };
  LocalInput local;
  local.source = ref("y0.00.00.00.00");
  // Equal address, different scope: no shared definition substitution.
  local.values.push_back(
      {ref("02.00.00.00.03"), 99, 2, {}, {{{}, 0.125}}, {{-8, 0, 0}, {}}});
  const auto group = ref("y0.00.00.00.01");
  local.context = {{{0, {}}, group},
                   {{1, {0, 0}}, group},
                   {{99, {}}, group},
                   {{0, {}}, group}};  // explicit duplicate collapses once
  local.placements.push_back({{0, {}}, {{-2, 0, 0}, {}}});
  local.geometry.push_back({object("02.00.00.00.03"), {}, {{{}, 0.25}}});
  return local;
}
inline void run_focus(const warm::Bundle &recipe, bool cuda) {
  const auto owner = warm::new_uuid();
  const auto shared = ActiveAssembly::assemble(recipe, owner, true);
  engine::InstanceSettings settings;
  settings.cuda = cuda;
  settings.read_environment = false;
  settings.cpu_threads = 2;
  const std::vector<field::HostPath> paths =
      cuda ? std::vector<field::HostPath>{field::HostPath::BulkCopy}
           : std::vector<field::HostPath>{field::HostPath::BulkCopy,
                                          field::HostPath::CpuDirect};
  for (const auto path : paths)
    for (const double offset : {0.0, 134217728.0}) {
      auto local = local_fixture(shared);
      local.values.at(0).initial = {{offset, 0, 0}, {}};
      // The bounded mixed fixture has its byte at root units and letter at 64.
      // All supplied points coincide in the operative frame; only the letter
      // has unit operative velocity. No rounded absolute f32 oracle is used.
      local.placements = {{{0, {}}, {{offset / 64, 0, 0}, {1.0 / 64, 0, 0}}},
                          {{1, {0, 0}}, {{offset, 0, 0}, {}}}};
      const std::vector<FocalField> focus{
          {GroupKind::SiblingDefinition,
           warm::key(local.values.at(0).ref.object)}};
      auto active = shared.revise(local).focus(focus);
      field::Parameters p;
      p.gravity = 0;
      p.contact_stiffness = 0;
      p.reach_strength = 0;
      p.dt = 1;
      ExecutionHandle old;
      ActiveReport old_report;
      {
        ActiveSession session(settings);
        auto handle = session.submit(active, p, path);
        const auto initial = session.report(handle, owner);
        const int N = initial.prepared.capacity,
                  G = int(initial.prepared.groups.size());
        const auto id = occurrence_text({active.study(), {0, {}}});
        int moving = -1, context = -1;
        for (const auto &particle : initial.prepared.particles)
          if (particle.occurrence.id == id) moving = particle.occurrence.slot;
        for (int g = 0; g < G; ++g)
          if (initial.prepared.groups[g].kind == GroupKind::Membership &&
              initial.prepared.groups[g].value ==
                  warm::key(local.context.front().target.object))
            context = g;
        check(moving >= 0 && context >= 0 &&
                  initial.frame_anchor[0] == offset &&
                  initial.prepared.focal_groups.size() == 1,
              "focused workbench retains supplied local frame and exact typed "
              "selection on both host paths");
        std::set<int> touched;
        double context_mass = 0,
               moving_mass = initial.particles[field::kMass * N + moving];
        for (const auto &edge : initial.prepared.participation) {
          if (edge.slot == moving) touched.insert(edge.group);
          if (edge.group == context)
            context_mass += initial.particles[field::kMass * N + edge.slot] *
                            double(edge.share);
        }
        for (int tick = 1; tick <= 4; ++tick) {
          session.tick(handle, p);
          const auto report = session.report(handle, owner);
          const auto d = session.demand(handle, owner);
          bool motion = true;
          for (const auto &particle : report.prepared.particles)
            motion &=
                report.particles[field::kPosX * N + particle.occurrence.slot] ==
                (particle.occurrence.slot == moving ? tick : 0);
          if (!motion || report.particles[field::kVelX * N + moving] != 1) {
            for (const auto &particle : report.prepared.particles)
              std::cerr << "focal particle node="
                        << particle.occurrence.study_node
                        << " slot=" << particle.occurrence.slot
                        << " seed=" << particle.position.x
                        << " vx=" << particle.velocity.x << " current="
                        << report.particles[field::kPosX * N +
                                            particle.occurrence.slot]
                        << '\n';
            std::cerr << "centroid="
                      << report.groups[field::kCenX * G + context]
                      << " expected=" << tick * moving_mass / context_mass
                      << '\n';
          }
          check(motion && report.particles[field::kVelX * N + moving] == 1 &&
                    std::abs(report.groups[field::kCenX * G + context] -
                             tick * moving_mass / context_mass) <= 1e-6,
                "independent focused workbench motion and centroid expectation "
                "survives 2pow27 translation");
          check(d.particles_moved == 1 &&
                    d.particles_woken == (tick == 1 ? 1 : 0) &&
                    d.particles_evaluated == (tick == 1 ? N : 1) &&
                    d.native_particle_entries == d.particles_evaluated &&
                    d.native_centroid_entries == touched.size() &&
                    d.centroids_flagged == touched.size() &&
                    d.centroids_recomputed == touched.size() &&
                    d.force_evaluations == 0 &&
                    report.demand.particles_moved == d.particles_moved,
                "both host paths preserve continued motion, centroid touches "
                "and owner-scoped demand");
          std::cout << "FOCUS_DEMAND,"
                    << (path == field::HostPath::BulkCopy ? "bulk" : "direct")
                    << ',' << offset << ',' << tick << ','
                    << d.particles_exposed << ',' << d.particles_evaluated
                    << ',' << d.particles_moved << ',' << d.particles_woken
                    << ',' << d.centroids_flagged << ','
                    << d.centroids_recomputed << ',' << d.gate_rejected << ','
                    << d.force_requests << ',' << d.touch_adjacency_visits
                    << ',' << d.native_particle_entries << ','
                    << d.native_centroid_entries << ','
                    << d.zero_response_touches << ','
                    << d.invalidation_particle_visits << ',' << d.tick_ms
                    << '\n';
        }
        rejects([&] { session.demand(handle, warm::new_uuid()); },
                "private-derived focal demand requires active owner");
        const auto before = session.report(handle, owner);
        auto invalid = active.focus({{GroupKind::Membership, "absent focus"}});
        rejects([&] { session.refresh(handle, invalid, p); },
                "invalid focus refresh fails closed on both host paths");
        const ExecutionHandle failed{handle.session, invalid.study(),
                                     invalid.generation()};
        check(!session.ready(handle) && !session.ready(failed),
              "invalid focal generation cannot execute old numerical state");
        rejects([&] { session.accept(before, owner); },
                "old focal report rejected after failed refresh");
        active = invalid.focus(focus);
        handle = session.refresh(failed, active, p);
        check(session.ready(handle) &&
                  session.report(handle, owner).frame_anchor[0] == offset,
              "valid focal refresh reseeds reference and activity buffers on "
              "both host paths");
        session.tick(handle, p);
        check(session.demand(handle, owner).particles_woken == 1,
              "refreshed activity state preserves initial positional wake");
        old = handle;
        old_report = session.report(handle, owner);
      }
      const auto rebuilt = active.restart();
      {
        ActiveSession restarted(settings);
        rejects([&] { restarted.accept(old_report, owner); },
                "old focused report rejected by restarted session");
        rejects([&] { restarted.tick(old, p); },
                "old focused handle rejected by restarted session");
        const auto handle = restarted.submit(rebuilt, p, path);
        const auto seed = restarted.report(handle, owner);
        check(seed.frame_anchor[0] == offset &&
                  seed.prepared.focal_groups.size() == 1,
              "restart reconstructs supplied focus, frame reference and native "
              "buffers on both paths");
        restarted.tick(handle, p);
        check(restarted.demand(handle, owner).particles_moved == 1 &&
                  restarted.demand(handle, owner).particles_woken == 1,
              "restarted focused study retains independently expected unit "
              "motion");
      }
    }
}
inline void run(const warm::Bundle &recipe, bool cuda) {
  const auto owner = warm::new_uuid();
  auto shared = ActiveAssembly::assemble(recipe, owner, false);
  auto incomplete = ActiveAssembly::assemble(recipe, owner, true);
  check(shared.inputs_available() && !incomplete.inputs_available(),
        "shared tools available and required-local inputs availability are "
        "distinct");
  auto local = local_fixture(incomplete);
  auto joint = incomplete.revise(local);
  check(joint.study() == incomplete.study() &&
            joint.generation() != incomplete.generation(),
        "supplied local generation preserves study and changes generation "
        "identity");
  auto bad_owner = local;
  bad_owner.source.owner = warm::new_uuid();
  rejects([&] { incomplete.revise(bad_owner); },
          "foreign analyst local snapshot rejected");
  auto bad_scope = local;
  bad_scope.values[0].ref.object.scope = warm::new_uuid();
  rejects([&] { incomplete.revise(bad_scope); },
          "foreign instance local value rejected");
  auto partial = local;
  partial.source.object.address.back().partial = true;
  rejects([&] { incomplete.revise(partial); },
          "local object requires full couplets");
  std::vector<float> storage(22);
  engine::PackedView<float> view(storage, {11, 2}, 1);
  check(view.bytes() == 88 && view.generation() == 1,
        "typed span schema/byte count/generation exact");
  rejects([&] { view.at(11, 0); }, "typed span row bounds checked");
  rejects(
      [&] {
        engine::PackedView<float> bad(storage, {11, 3}, 1);
      },
      "typed span shape mismatch rejected");
  rejects(
      [&] {
        engine::PackedView<float> bad(storage, {11, 2}, 0);
      },
      "unsealed span generation rejected");
  engine::InstanceSettings settings;
  settings.cuda = cuda;
  settings.read_environment = false;
  settings.cpu_threads = 2;
  field::Parameters parameters;
  parameters.gravity = 0.125f;
  parameters.dt = 0;
  ExecutionHandle stale;
  ActiveReport previous;
  {
    ActiveSession session(settings);
    rejects([&] { session.submit(incomplete, parameters); },
            "missing local inputs never execution-ready");
    const auto hs = session.submit(shared, parameters);
    auto hj = session.submit(joint, parameters);
    check(session.ready(hs) && session.ready(hj) && hs.study != hj.study,
          "two independent studies seeded and ready in one session");
    const auto rs = session.report(hs, owner), rj = session.report(hj, owner);
    check(rs.prepared.particles.size() == 2 &&
              rj.prepared.particles.size() == 3 &&
              rs.prepared.scales.at(1).units == 64 && rj.origins.back().local &&
              !rj.origins.back().shared,
          "mixed local/shared live objects retain scope, origin and explicit "
          "units");
    check(rj.prepared.groups.size() == rs.prepared.groups.size() + 2 &&
              rj.prepared.participation.size() ==
                  rs.prepared.participation.size() + 4,
          "only supplied local sibling/context fields added; repeated context "
          "deduplicated");
    bool transfer = true;
    for (const auto &p : rs.prepared.particles) {
      const auto i = p.occurrence.slot, P = rs.prepared.capacity;
      transfer &= rs.particles[field::kMass * P + i] == p.mass &&
                  rs.particles[field::kRadius * P + i] == p.radius &&
                  rs.particles[field::kPosX * P + i] == float(p.position.x) &&
                  rs.particles[field::kVelX * P + i] == float(p.velocity.x);
    }
    check(transfer, "initial f32 values survive bulk transfer exactly");
    session.tick(hs, parameters);
    session.tick(hj, parameters);
    verify(rs, session.report(hs, owner), parameters);
    verify(rj, session.report(hj, owner), parameters);
    auto moving = parameters;
    moving.dt = 0.001f;
    for (int step = 0; step < 16; ++step) session.tick(hj, moving);
    const auto evolved = session.report(hj, owner);
    bool bounded = true;
    for (float v : evolved.particles)
      bounded &= std::isfinite(v) && std::abs(v) < 1e6;
    for (int i = 0; i < evolved.prepared.capacity; ++i)
      bounded &=
          evolved.particles[field::kMass * evolved.prepared.capacity + i] ==
          rj.particles[field::kMass * evolved.prepared.capacity + i];
    check(bounded &&
              evolved.local_provenance->source.version == local.source.version,
          "bounded nonzero-dt execution preserves live mass and analyst "
          "snapshot provenance");
    rejects([&] { session.report(hj, warm::new_uuid()); },
            "joint report restricted to active owner");
    check(bits(rs.particles, session.report(hs, owner).particles) == false,
          "tick updates execution accumulators independently of immutable "
          "initial recipe");
    auto replacement = joint.revise(local);
    auto refreshed = session.refresh(hj, replacement, parameters);
    check(session.ready(refreshed) && !session.ready(hj),
          "equal-count refresh reseeds and invalidates old generation");
    rejects([&] { session.accept(rj, owner); },
            "old generation report rejected after refresh");
    auto invalid = replacement.revise({});
    rejects([&] { session.refresh(refreshed, invalid, parameters); },
            "incomplete refresh fails closed without manager fallback");
    const ExecutionHandle failed{refreshed.session, invalid.study(),
                                 invalid.generation()};
    check(!session.ready(failed),
          "incomplete publication leaves study non-runnable");
    rejects([&] { session.tick(failed, parameters); },
            "ticking failed publication rejected");
    auto recovered = invalid.revise(local);
    auto recovered_handle = session.refresh(failed, recovered, parameters);
    check(session.ready(recovered_handle),
          "valid equal-count refresh recovers failed study");
    auto invalid_parameters = parameters;
    invalid_parameters.gravity = std::numeric_limits<float>::quiet_NaN();
    auto rejected = recovered.revise(local);
    rejects(
        [&] {
          session.refresh(recovered_handle, rejected, invalid_parameters);
        },
        "invalid numerical publication fails closed");
    const ExecutionHandle rejected_handle{failed.session, rejected.study(),
                                          rejected.generation()};
    auto final = rejected.revise(local);
    stale = session.refresh(rejected_handle, final, parameters);
    previous = session.report(stale, owner);
    const auto t = session.timings(stale);
    std::cout << "PHASE integration assembly_ms=" << session.assembly_ms()
              << " allocation_ms=" << t.allocation_ms
              << " compile_or_cache_ms=" << t.compilation_ms
              << " upload_ms=" << t.upload_ms << " seed_ms=" << t.seed_ms
              << " kernels_ms=" << t.kernel_ms << " report_ms=" << t.report_ms
              << '\n';
  }
  check(joint.local()->source.version == local.source.version &&
            joint.shared_recipe().ref.revision == recipe.ref.revision,
        "host logical inputs and local version survive controlled Program "
        "teardown");
  auto rebuilt = joint.restart();
  check(rebuilt.study() != joint.study(),
        "restart builds a new study identity from retained inputs");
  {
    ActiveSession session(settings);
    rejects([&] { session.report(stale, owner); },
            "old session handle rejected after restart");
    rejects([&] { session.accept(previous, owner); },
            "old session report rejected after restart");
    const auto h = session.submit(rebuilt, parameters);
    const auto seeded = session.report(h, owner);
    session.tick(h, parameters);
    verify(seeded, session.report(h, owner), parameters);
    check(bits(previous.particles, seeded.particles),
          "reconstructed initial scalar transfer bit patterns exact");
  }
  bool intact = true;
  for (std::size_t i = 0; i < recipe.tools.size(); ++i)
    intact &= warm::tool_body(recipe.tools[i]) ==
              warm::tool_body(joint.shared_recipe().tools[i]);
  check(intact && recipe.geometry[0].spheres[0].radius != 0.25,
        "private geometry, placements and context never rewrite shared source "
        "definitions/recipe");
  run_focus(recipe, cuda);
}
}  // namespace active_checks
