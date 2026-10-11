// Does the field harness do what the model says?
//
// One law, evaluated against group centroids that include the subject.
// Proportional response absorbed by the whole mass. A
// discretization brake that suppresses overshoot without varying the step.
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "engine/runtime.h"
#include "field/field.h"
#include "harness.h"
#include "taichi/platform/cuda/detect_cuda.h"

namespace {

using engine::InstanceSettings;
using engine::Runtime;
using field::Harness;
using field::Parameters;

// The working base for this machine: sized to identifiers issued over a
// process life, not to what is live at once. About 25 MB of preallocation.
InstanceSettings base_settings(bool cuda) {
  InstanceSettings settings;
  settings.cuda = cuda;
  settings.snode_capacity = 1 << 20;
  settings.snode_tree_capacity = 1 << 16;
  return settings;
}

bool finite(float v) {
  return std::isfinite(v);
}

void set_particle(Harness &h, int i, float x, float y, float z, float mass) {
  h.particle(field::kPosX, i) = x;
  h.particle(field::kPosY, i) = y;
  h.particle(field::kPosZ, i) = z;
  h.particle(field::kMass, i) = mass;
}

// Whole participation: the entire mass, sitting at the centre.
void set_whole_edge(Harness &h, int e, int particle, int group) {
  h.edge_int(field::kEdgeParticle, e) = particle;
  h.edge_int(field::kEdgeGroup, e) = group;
  h.edge_float(field::kShare, e) = 1.0f;
}

float origin_distance(Harness &h, int i) {
  const float x = h.particle(field::kPosX, i);
  const float y = h.particle(field::kPosY, i);
  const float z = h.particle(field::kPosZ, i);
  return std::sqrt(x * x + y * y + z * z);
}

float separation(Harness &h, int a, int b) {
  const float dx = h.particle(field::kPosX, a) - h.particle(field::kPosX, b);
  const float dy = h.particle(field::kPosY, a) - h.particle(field::kPosY, b);
  const float dz = h.particle(field::kPosZ, a) - h.particle(field::kPosZ, b);
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// The centroid carries the group's total mass at its mass-weighted location,
// with the subject included.
void check_centroid(bool cuda, const std::string &arch) {
  harness::note(arch + ": centroid");
  Runtime runtime(base_settings(cuda));
  Harness h(runtime, /*particles=*/3, /*groups=*/1, /*edges=*/3, /*contacts=*/0);
  set_particle(h, 0, 0.0f, 0.0f, 0.0f, 1.0f);
  set_particle(h, 1, 4.0f, 0.0f, 0.0f, 3.0f);
  set_particle(h, 2, 0.0f, 8.0f, 0.0f, 4.0f);
  for (int e = 0; e < 3; ++e) {
    set_whole_edge(h, e, e, 0);
  }
  h.upload();
  h.seed_centroids(Parameters{});
  h.download();

  // (1*0 + 3*4 + 4*0)/8 = 1.5 ; (1*0 + 3*0 + 4*8)/8 = 4.0
  const float cx = h.group(field::kCenX, 0);
  const float cy = h.group(field::kCenY, 0);
  const float cm = h.group(field::kCenM, 0);
  harness::record(std::fabs(cx - 1.5f) < 1e-4f, arch + ": centroid x",
                  "got " + std::to_string(cx));
  harness::record(std::fabs(cy - 4.0f) < 1e-4f, arch + ": centroid y",
                  "got " + std::to_string(cy));
  harness::record(std::fabs(cm - 8.0f) < 1e-4f, arch + ": centroid mass",
                  "got " + std::to_string(cm));
}

// Two unconstrained mass-1 particles at separation 1 overlap at their
// centroid in one time unit. That is what fixes the constant, so it is the
// one number this harness has to reproduce. The retained fixture places the
// closing axis (y) perpendicular to the radial direction (x); inward closing
// motion is untouched by outward radial damping. There is no universal pull.
void check_calibration(bool cuda, const std::string &arch) {
  harness::note(arch + ": calibration");
  Runtime runtime(base_settings(cuda));
  Harness h(runtime, 2, 1, 2, /*contacts=*/0);
  set_particle(h, 0, 50.0f, -0.5f, 0.0f, 1.0f);
  set_particle(h, 1, 50.0f, 0.5f, 0.0f, 1.0f);
  set_whole_edge(h, 0, 0, 0);
  set_whole_edge(h, 1, 1, 0);
  h.upload();

  Parameters p;
  p.dt = 0.002f;
  h.seed_centroids(p);

  float met_at = -1.0f;
  for (int step = 1; step <= 1500 && met_at < 0.0f; ++step) {
    h.tick(p);
    h.download();
    if (separation(h, 0, 1) < 0.02f) {
      met_at = float(step) * p.dt;
    }
  }
  harness::note(arch + ": met at " + std::to_string(met_at));
  harness::record(met_at > 0.9f && met_at < 1.3f,
                  arch + ": meets at the centroid in about one time unit",
                  "met at " + std::to_string(met_at));

  // Symmetric setup, symmetric outcome, and they close on the midpoint of
  // the closing axis (y).
  const float mid =
      0.5f * (h.particle(field::kPosY, 0) + h.particle(field::kPosY, 1));
  harness::record(std::fabs(mid) < 1e-3f, arch + ": meets at the midpoint",
                  "midpoint " + std::to_string(mid));
}

// Two particles at the exact same point. The one law has no epsilon, so the
// d==0 gate must yield zero force rather than infinity, and both must stay
// finite. This is the test that the gate is exact, not softened.
void check_coincidence(bool cuda, const std::string &arch) {
  harness::note(arch + ": coincidence");
  Runtime runtime(base_settings(cuda));
  Harness h(runtime, 2, 1, 2, /*contacts=*/0);
  set_particle(h, 0, 1.0f, 1.0f, 1.0f, 1.0f);
  set_particle(h, 1, 1.0f, 1.0f, 1.0f, 1.0f);
  set_whole_edge(h, 0, 0, 0);
  set_whole_edge(h, 1, 1, 0);
  h.upload();
  Parameters p;
  h.seed_centroids(p);
  for (int step = 0; step < 8; ++step) {
    h.tick(p);
  }
  h.download();
  const bool ok = finite(h.particle(field::kPosX, 0)) &&
                  finite(h.particle(field::kVelX, 0)) &&
                  finite(h.particle(field::kPosX, 1)) &&
                  finite(h.particle(field::kVelX, 1));
  harness::record(ok, arch + ": coincident particles stay finite",
                  "x0 " + std::to_string(h.particle(field::kPosX, 0)));
}

// A group of one has its centroid exactly on its only member, every tick.
// The exact coincidence gate keeps that finite. The
// singleton has nothing else to pull against within its own group (the
// engaged gate is exactly zero there); with the universal inward pull now
// absent and the particle at rest, nothing pulls on it at all, so it
// must stay exactly where it started.
void check_singleton(bool cuda, const std::string &arch) {
  harness::note(arch + ": singleton group");
  Runtime runtime(base_settings(cuda));
  Harness h(runtime, 1, 1, 1, /*contacts=*/0);
  set_particle(h, 0, 2.0f, -3.0f, 1.0f, 1.0f);
  set_whole_edge(h, 0, 0, 0);
  h.upload();
  Parameters p;
  h.seed_centroids(p);
  const float initial_distance = origin_distance(h, 0);
  for (int step = 0; step < 16; ++step) {
    h.tick(p);
  }
  h.download();
  const float x = h.particle(field::kPosX, 0);
  const float vx = h.particle(field::kVelX, 0);
  harness::record(finite(x) && finite(vx), arch + ": singleton stays finite",
                  "x " + std::to_string(x) + " vx " + std::to_string(vx));
  const float final_distance = origin_distance(h, 0);
  harness::record(std::fabs(final_distance - initial_distance) < 1e-4f,
                  arch + ": singleton stays put, nothing left to pull it",
                  "final " + std::to_string(final_distance) + " initial " +
                      std::to_string(initial_distance));
}

// Unequal masses with two repeated parent occurrences on one whole particle.
void check_proportional_response(bool cuda, const std::string &arch) {
  Runtime runtime(base_settings(cuda));
  Harness h(runtime, 2, 1, 3);
  set_particle(h, 0, 0.0f, 0.0f, 0.0f, 6.0f);
  set_particle(h, 1, 8.0f, 0.0f, 0.0f, 2.0f);
  for (int e = 0; e < 2; ++e) {
    set_whole_edge(h, e, 0, 0);
    h.edge_float(field::kShare, e) = 0.25f;
  }
  set_whole_edge(h, 2, 1, 0);
  Parameters p;
  p.dt = 0.001f;
  h.publish(p);
  h.download();
  harness::record(std::fabs(h.group(field::kCenX, 0) - 3.2f) < 1e-5f &&
                      h.group(field::kCenM, 0) == 5.0f,
                  arch + ": repeated participating mass sets the centre centroid", "");
  h.tick(p);
  h.download();
  const float expected = p.gravity * 15.0f / (3.2f * 3.2f);
  harness::record(std::fabs(h.particle(field::kForceX, 0) - expected) < 1e-5f,
                  arch + ": repeated shares add whole-particle force", "");
  harness::record(std::fabs(h.particle(field::kVelX, 0) - expected / 6.0f * p.dt) < 1e-7f,
                  arch + ": total mass absorbs proportional force", "");
  harness::record(h.particle(field::kForceY, 0) == 0.0f &&
                      h.particle(field::kForceZ, 0) == 0.0f,
                  arch + ": parent response has no internal application offset", "");
}

// The brake is a discretization correction on the resultant force: per the
// model, it must sit at ~1 whenever a tick's own travel is a small fraction
// of the force's reach ("≈1 in any tick that does not overshoot the
// destination"), and it must visibly attenuate once the travel would
// overshoot. Starting the particle from rest lets the *unbraked* velocity be
// read straight off Newton's law (dv = F/m * dt) using the force the harness
// itself accumulated that tick, so the assertion is anchored to the model's
// stated behaviour, not to the exponential's own constants.
void check_brake_overshoot(bool cuda, const std::string &arch) {
  harness::note(arch + ": brake");
  auto run = [&](float dt) -> float {
    Runtime runtime(base_settings(cuda));
    Harness h(runtime, 2, 1, 2);
    set_particle(h, 0, -0.5f, 0.0f, 0.0f, 1.0f);
    set_particle(h, 1, 0.5f, 0.0f, 0.0f, 1.0f);
    set_whole_edge(h, 0, 0, 0);
    set_whole_edge(h, 1, 1, 0);
    h.upload();
    Parameters p;
    p.dt = dt;
    h.seed_centroids(p);
    h.tick(p);
    h.download();

    // Reach: the resultant force this tick accumulated for particle 0 --
    // still resident, since the clear kernel only zeroes it at the start of
    // the next tick.
    const float fx = h.particle(field::kForceX, 0);
    const float fy = h.particle(field::kForceY, 0);
    const float fz = h.particle(field::kForceZ, 0);
    const float reach = std::sqrt(fx * fx + fy * fy + fz * fz);
    const float mass = h.particle(field::kMass, 0);

    // Starting from rest, the unbraked velocity is exactly F/m * dt -- plain
    // Newtonian mechanics, not the brake under test.
    const float unbraked_speed = reach / mass * dt;

    const float vx = h.particle(field::kVelX, 0);
    const float vy = h.particle(field::kVelY, 0);
    const float vz = h.particle(field::kVelZ, 0);
    const float actual_speed = std::sqrt(vx * vx + vy * vy + vz * vz);

    // The ratio of what happened to what would have happened unbraked *is*
    // the brake, read out behaviourally rather than recomputed from the
    // kernel's own formula.
    return actual_speed / unbraked_speed;
  };

  // dt this small makes the tick's travel a negligible fraction of the
  // force's reach: the brake must be indistinguishable from unity.
  const float undershoot_brake = run(1e-4f);
  harness::record(finite(undershoot_brake) &&
                      std::fabs(undershoot_brake - 1.0f) < 1e-3f,
                  arch + ": brake is ~1 well short of the destination",
                  "brake " + std::to_string(undershoot_brake));

  // dt this large makes the tick's travel overshoot the force's reach many
  // times over: the brake must bite, visibly below unity.
  const float overshoot_brake = run(2.0f);
  harness::record(finite(overshoot_brake) && overshoot_brake < 0.5f,
                  arch + ": brake is well below 1 on overshoot",
                  "brake " + std::to_string(overshoot_brake));
}

// Build the same chaotic 64-particle ring check_stability and
// check_stability_agreement both run: everyone in one shared group, radii
// spread over the ring so the mutual field pull is genuinely many-body.
void build_ring(Harness &h, int n) {
  for (int i = 0; i < n; ++i) {
    const float a = float(i) * 0.61f;
    set_particle(h, i, std::cos(a) * (1.0f + 0.03f * i),
                 std::sin(a) * (1.0f + 0.03f * i), 0.02f * i, 1.0f);
    set_whole_edge(h, i, i, 0);
  }
}

// The brake exists to stop fine systems acquiring jitter and exploding, but
// "runaway" is escape to unbounded space, not high transient speed -- speed
// is the brake's own concern, damped as a particle nears its target, not a
// hard cap. Boundedness is now the radial velocity dampening's job (the
// universal inward pull that used to do this is absent): nothing should
// grow without bound, and nothing should go non-finite.
void check_stability(bool cuda, const std::string &arch) {
  harness::note(arch + ": stability");
  Runtime runtime(base_settings(cuda));
  const int n = 64;
  Harness h(runtime, n, 1, n);
  build_ring(h, n);
  h.upload();
  Parameters p;
  h.seed_centroids(p);

  // A generous bound: "nothing escapes" means finite, not that this densely
  // packed 64-body cluster stays anywhere near its starting radius. This
  // shared-group ring is genuinely many-body (every particle pulls toward
  // the one shared centroid at the full 64x mass), and at dt=1 a close
  // encounter can sling a particle out hard. The dampening only acts on the
  // position a particle is already at, not on the kick a close encounter is
  // about to impart, so a single violent tick can still carry a particle a
  // long way out before the dampening reels the following ticks' outward
  // speed back in -- it bounds the trajectory, not the single-tick jump.
  // Measured farthest excursions over repeat runs with the dampening active
  // ranged from ~370 to ~38000 (cuda; the GPU's unordered atomic
  // accumulation makes the exact trajectory non-deterministic run to run,
  // which this chaotic a system then amplifies) and a deterministic ~1520
  // on cpu (down from the pre-dampening deterministic ~4300). The bound
  // below is tightened from the pre-dampening trial (which was 100000x this
  // radius) but still set well above the worst observed, so it still
  // catches genuine divergence (overflow-scale growth, not this system's
  // ordinary chaos) without being tripped by that legitimate variance.
  const float max_initial_radius = 1.0f + 0.03f * float(n - 1);
  const float bound = 60000.0f * max_initial_radius;

  float worst_distance = 0.0f;
  bool all_finite = true;
  for (int step = 0; step < 400; ++step) {
    h.tick(p);
    if (step % 50 == 0 || step == 399) {
      h.download();
      for (int i = 0; i < n; ++i) {
        all_finite = all_finite && finite(h.particle(field::kPosX, i)) &&
                     finite(h.particle(field::kVelX, i));
        worst_distance = std::max(worst_distance, origin_distance(h, i));
      }
    }
  }
  harness::note(arch + ": stability farthest excursion observed " +
               std::to_string(worst_distance));
  harness::record(all_finite, arch + ": 400 ticks stay finite", "went bad");
  harness::record(worst_distance < bound, arch + ": positions stay bounded",
                  "farthest " + std::to_string(worst_distance) + " bound " +
                      std::to_string(bound));

  // The determiner stage ran on the products of the primary calculation.
  h.download();
  const float vx = h.particle(field::kVelX, 0);
  const float vy = h.particle(field::kVelY, 0);
  const float vz = h.particle(field::kVelZ, 0);
  const float speed = std::sqrt(vx * vx + vy * vy + vz * vz);
  harness::record(std::fabs(speed - h.determined()[0]) < 1e-5f,
                  arch + ": determiners ran on the products",
                  "speed " + std::to_string(speed) + " determined " +
                      std::to_string(h.determined()[0]));
}

// The same ring, run to the same long horizon on both backends. The intent
// was: exact position diverges under discretization, but the settled
// RELATIVE structure -- the sorted list of every pairwise distance, a
// translation- and rotation-invariant summary -- should still agree.
//
// Measured instead: this shared-group ring never settles by tick 400 --
// it is a genuinely chaotic many-body cluster (see check_stability), and
// on cuda the unordered atomic accumulation makes even repeat runs of the
// *same* binary land on different trajectories (observed farthest-particle
// distances of ~300, ~19000, ~13000 across three back-to-back runs).
// Comparing sorted pairwise distances at tick 400 is comparing two
// snapshots of an unsettled, sensitive-dependence system, not a common
// equilibrium: measured largest pairwise-distance differences were
// ~3100 and ~44000 across two runs -- no fixed "modest tolerance" is
// honest here without becoming vacuous. Flagged rather than forced: this
// is a finding about the fixture (many-body ring at dt=1, no dissipation
// beyond the brake), not something reorienting the fixture away from the
// universal pull can fix, since the divergence is dominated by the ring's
// own chaos, not by the universal pull. Left as a printed observation, not
// an assertion, pending a call on whether to use a non-chaotic fixture for
// this comparison instead.
void check_stability_agreement() {
  harness::note("cpu vs cuda relative structure after the chaotic ring (observational, not asserted -- see comment)");
  const int n = 64;
  std::vector<float> pairwise[2];
  for (int which = 0; which < 2; ++which) {
    Runtime runtime(base_settings(/*cuda=*/which == 1));
    Harness h(runtime, n, 1, n);
    build_ring(h, n);
    h.upload();
    Parameters p;
    h.seed_centroids(p);
    for (int step = 0; step < 400; ++step) {
      h.tick(p);
    }
    h.download();
    pairwise[which].reserve(std::size_t(n) * (n - 1) / 2);
    for (int i = 0; i < n; ++i) {
      for (int j = i + 1; j < n; ++j) {
        pairwise[which].push_back(separation(h, i, j));
      }
    }
    std::sort(pairwise[which].begin(), pairwise[which].end());
  }
  float worst = 0.0f;
  for (std::size_t i = 0; i < pairwise[0].size(); ++i) {
    worst = std::max(worst, std::fabs(pairwise[0][i] - pairwise[1][i]));
  }
  harness::note("cpu vs cuda largest pairwise-distance difference: " +
                std::to_string(worst));
}

// Unequal supplied radii set the contact boundary. No field attraction here.
void check_contact(bool cuda, const std::string &arch) {
  Runtime runtime(base_settings(cuda));
  Harness h(runtime, 2, 0, 0, 1);
  Parameters p;
  p.dt = 0.001f;
  p.contact_stiffness = 4.0f;
  for (float distance : {0.5f, 1.0f, 1.5f, 0.0f}) {
    std::fill(h.particle_data.begin(), h.particle_data.end(), 0.0f);
    set_particle(h, 0, 0.0f, 0.0f, 0.0f, 1.0f);
    set_particle(h, 1, distance, 0.0f, 0.0f, 3.0f);
    h.particle(field::kRadius, 0) = 0.25f;
    h.particle(field::kRadius, 1) = 0.75f;
    h.contact_int(field::kContactA, 0) = 0;
    h.contact_int(field::kContactB, 0) = 1;
    h.publish(p);
    h.tick(p);
    h.download();
    const float expected = distance == 0.5f ? -2.0f : 0.0f;
    const std::string label = arch + ": contact at distance " + std::to_string(distance);
    harness::record(std::fabs(h.particle(field::kForceX, 0) - expected) < 1e-5f,
                    label + " follows summed radii", "");
    harness::record(std::fabs(h.particle(field::kForceX, 0) +
                             h.particle(field::kForceX, 1)) < 1e-6f,
                    label + " is equal and opposite", "");
    harness::record(std::all_of(h.particle_data.begin(), h.particle_data.end(), finite),
                    label + " stays finite", "");
    if (distance == 0.0f) {
      harness::record(separation(h, 0, 1) == 0.0f,
                      arch + ": coincidence gate does not separate centres", "");
    }
  }
}

// Radial damping still reads total particle mass, counted once.
// same universal mass cell (here, just this one particle's mass) for its
// reach scale and only bites once outward radial speed would carry the
// particle past that reach.
void check_radial_damping(bool cuda, const std::string &arch) {
  harness::note(arch + ": radial damping without universal attraction");

  // Off the origin, at rest: with no inward pull and no field
  // force (sole member), the particle must simply stay put.
  {
    Runtime runtime(base_settings(cuda));
    Harness h(runtime, /*particles=*/1, /*groups=*/1, /*edges=*/1,
              /*contacts=*/0);
    set_particle(h, 0, 3.0f, 0.0f, 0.0f, 1.0f);
    set_whole_edge(h, 0, 0, 0);
    h.upload();
    Parameters p;
    h.seed_centroids(p);

    bool all_finite = true;
    for (int step = 0; step < 8; ++step) {
      h.tick(p);
      h.download();
      all_finite = all_finite && finite(h.particle(field::kPosX, 0)) &&
                   finite(h.particle(field::kVelX, 0));
    }
    const float final_distance = origin_distance(h, 0);
    harness::record(all_finite,
                    arch + ": resting sole particle stays finite",
                    "went bad");
    harness::record(
        std::fabs(final_distance - 3.0f) < 1e-4f,
        arch +
            ": without universal attraction, a sole particle at rest stays put",
        "final distance " + std::to_string(final_distance));
  }

  // Given a strong outward radial velocity, well past the reach, the
  // dampening must bleed off the outward speed rather than let it run away.
  {
    Runtime runtime(base_settings(cuda));
    Harness h(runtime, /*particles=*/1, /*groups=*/1, /*edges=*/1,
              /*contacts=*/0);
    set_particle(h, 0, 3.0f, 0.0f, 0.0f, 1.0f);
    h.particle(field::kVelX, 0) = 50.0f;  // outward, well past the reach
    set_whole_edge(h, 0, 0, 0);
    h.upload();
    Parameters p;
    p.dt = 0.01f;
    h.seed_centroids(p);
    const float initial_speed = h.particle(field::kVelX, 0);
    for (int step = 0; step < 8; ++step) {
      h.tick(p);
    }
    h.download();
    const float vx = h.particle(field::kVelX, 0);
    harness::record(finite(vx),
                    arch + ": dampened outward runaway stays finite",
                    "vx " + std::to_string(vx));
    harness::record(
        vx < initial_speed,
        arch + ": radial dampening bleeds off outward runaway speed",
        "initial " + std::to_string(initial_speed) + " final " +
            std::to_string(vx));
  }

  // At the origin: the exact d==0 gate must yield zero pull, not NaN, and
  // the particle must not move.
  {
    Runtime runtime(base_settings(cuda));
    Harness h(runtime, /*particles=*/1, /*groups=*/1, /*edges=*/1,
              /*contacts=*/0);
    set_particle(h, 0, 0.0f, 0.0f, 0.0f, 1.0f);
    set_whole_edge(h, 0, 0, 0);
    h.upload();
    Parameters p;
    h.seed_centroids(p);
    for (int step = 0; step < 8; ++step) {
      h.tick(p);
    }
    h.download();
    const float x = h.particle(field::kPosX, 0);
    const float y = h.particle(field::kPosY, 0);
    const float z = h.particle(field::kPosZ, 0);
    const bool ok = finite(x) && finite(y) && finite(z) &&
                    finite(h.particle(field::kVelX, 0));
    harness::record(ok,
                    arch + ": radial damping at the origin stays finite",
                    "x " + std::to_string(x));
    harness::record(std::fabs(x) < 1e-5f && std::fabs(y) < 1e-5f &&
                        std::fabs(z) < 1e-5f,
                    arch + ": a particle at the origin does not move",
                    "x " + std::to_string(x) + " y " + std::to_string(y) +
                        " z " + std::to_string(z));
  }
}

// One kernel, one law, every level. The two architectures must agree.
void check_architectures_agree() {
  harness::note("cpu and cuda agree");
  std::vector<float> result[2];
  for (int which = 0; which < 2; ++which) {
    Runtime runtime(base_settings(/*cuda=*/which == 1));
    Harness h(runtime, 8, 2, 12);
    for (int i = 0; i < 8; ++i) {
      set_particle(h, i, float(i) - 3.5f, float((i * 5) % 7) - 3.0f,
                   float(i % 3), 1.0f + 0.25f * i);
    }
    for (int i = 0; i < 8; ++i) {
      set_whole_edge(h, i, i, i % 2);
    }
    for (int k = 0; k < 4; ++k) {
      h.edge_int(field::kEdgeParticle, 8 + k) = k;
      h.edge_int(field::kEdgeGroup, 8 + k) = (k + 1) % 2;
      h.edge_float(field::kShare, 8 + k) = 0.5f;
    }
    h.upload();
    Parameters p;
    p.dt = 0.05f;
    h.seed_centroids(p);
    for (int step = 0; step < 40; ++step) {
      h.tick(p);
    }
    h.download();
    result[which] = h.particle_data;
  }
  float worst = 0.0f;
  for (std::size_t i = 0; i < result[0].size(); ++i) {
    worst = std::max(worst, std::fabs(result[0][i] - result[1][i]));
  }
  harness::record(worst < 2e-3f, "cpu and cuda agree after 40 ticks",
                  "largest difference " + std::to_string(worst));
}

// Kernels address device memory directly with staged indices, so upload()
// must refuse an out-of-range edge or contact before anything reaches the device.
bool upload_throws(Harness &h) {
  try {
    h.upload();
  } catch (const std::out_of_range &) {
    return true;
  }
  return false;
}

void check_index_validation(bool cuda, const std::string &arch) {
  harness::note(arch + ": staged index validation");
  Runtime runtime(base_settings(cuda));
  {
    Harness h(runtime, 2, 1, 2, /*contacts=*/1);
    set_particle(h, 0, 0.0f, 0.0f, 0.0f, 1.0f);
    set_particle(h, 1, 1.0f, 0.0f, 0.0f, 1.0f);
    set_whole_edge(h, 0, 0, 0);
    set_whole_edge(h, 1, 1, 0);
    h.contact_int(field::kContactA, 0) = 0;
    h.contact_int(field::kContactB, 0) = 1;
    bool ok = true;
    try {
      h.upload();
    } catch (const std::exception &) {
      ok = false;
    }
    harness::record(ok, arch + ": in-range edges and contacts upload", "");
    bool tick_refused = false;
    try { h.tick(Parameters{}); } catch (const std::logic_error &) { tick_refused = true; }
    harness::record(tick_refused && !h.ready(), arch + ": upload alone cannot tick unseeded centroids", "");

    set_whole_edge(h, 1, 2, 0);
    harness::record(upload_throws(h), arch + ": edge particle past the count is refused", "");
    set_whole_edge(h, 1, 1, 1);
    harness::record(upload_throws(h), arch + ": edge group past the count is refused", "");
    set_whole_edge(h, 1, -1, 0);
    harness::record(upload_throws(h), arch + ": negative edge particle is refused", "");
    set_whole_edge(h, 1, 1, 0);
    h.contact_int(field::kContactB, 0) = 2;
    harness::record(upload_throws(h), arch + ": contact particle past the count is refused", "");
  }
  bool negative_refused = false;
  try {
    Harness h(runtime, -1, 1, 1);
  } catch (const std::invalid_argument &) {
    negative_refused = true;
  }
  harness::record(negative_refused, arch + ": negative count is refused", "");
}

void check_staging_validation(bool cuda, const std::string &arch) {
  Runtime runtime(base_settings(cuda));
  Harness h(runtime, 2, 1, 1);
  set_particle(h, 0, 0, 0, 0, 1);
  set_whole_edge(h, 0, 0, 0);
  auto refused = [&] {
    try { h.upload(); } catch (const std::invalid_argument &) { return true; }
    return false;
  };
  h.particle(field::kRadius, 0) = -1;
  harness::record(refused(), arch + ": negative prepared radius refused", "");
  h.particle(field::kRadius, 0) = std::numeric_limits<float>::quiet_NaN();
  harness::record(refused(), arch + ": non-finite prepared radius refused", "");
  h.particle(field::kRadius, 0) = 0.5f;
  h.edge_float(field::kShare, 0) = 0;
  harness::record(refused(), arch + ": zero participation refused", "");
  h.edge_float(field::kShare, 0) = 1;
  h.edge_int(field::kEdgeParticle, 0) = 1;
  harness::record(refused(), arch + ": field reference to a free slot refused", "");
  h.edge_int(field::kEdgeParticle, 0) = 0;
  h.particle(field::kVelX, 1) = 1;
  harness::record(refused(), arch + ": moving free slot refused", "");
  h.particle(field::kVelX, 1) = 0;
  h.publish(Parameters{});
  h.download();
  harness::record(h.determined()[1] == 0, arch + ": initial publication initializes free determiner state", "");
  h.particle_data.pop_back();
  harness::record(refused(), arch + ": malformed staging size refused", "");
  Harness contact(runtime, 2, 0, 0, 1);
  set_particle(contact, 0, 0, 0, 0, 1);
  contact.contact_int(field::kContactA, 0) = 0;
  contact.contact_int(field::kContactB, 0) = 1;
  bool free_contact_refused = false;
  try { contact.upload(); } catch (const std::invalid_argument &) { free_contact_refused = true; }
  harness::record(free_contact_refused, arch + ": contact reference to a free slot refused", "");
}

void check_arch(bool cuda) {
  const std::string arch = cuda ? "cuda" : "cpu";
  check_centroid(cuda, arch);
  check_calibration(cuda, arch);
  check_coincidence(cuda, arch);
  check_singleton(cuda, arch);
  check_proportional_response(cuda, arch);
  check_brake_overshoot(cuda, arch);
  check_stability(cuda, arch);
  check_contact(cuda, arch);
  check_radial_damping(cuda, arch);
  check_index_validation(cuda, arch);
  check_staging_validation(cuda, arch);
}

}  // namespace

int main() {
  check_arch(/*cuda=*/false);
  if (taichi::is_cuda_api_available()) {
    check_arch(/*cuda=*/true);
    check_architectures_agree();
    check_stability_agreement();
  } else {
    harness::note("no cuda on this host, cpu only");
  }
  return harness::report("field_test");
}
