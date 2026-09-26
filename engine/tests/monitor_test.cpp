// Does the monitor measure what it says? Hand-built field states, no
// runtime: the monitor only ever reads host arrays.
#include <cmath>
#include <limits>
#include <string>
#include <vector>

#include "harness.h"
#include "monitor/field_monitor.h"

namespace {

// A small field state in the packed host layout.
struct State {
  int n, g, e;
  std::vector<float> particles, groups;
  std::vector<int> edges;

  State(int n_, int g_, int e_)
      : n(n_), g(g_), e(e_),
        particles(std::size_t(n_) * field::kParticleFieldCount, 0.0f),
        groups(std::size_t(g_) * field::kGroupFieldCount, 0.0f),
        edges(std::size_t(e_) * field::kEdgeIntFieldCount, 0) {}

  float &p(int f, int i) { return particles[std::size_t(f) * n + i]; }
  float &c(int f, int k) { return groups[std::size_t(f) * g + k]; }
  void edge(int j, int particle, int group) {
    edges[std::size_t(field::kEdgeParticle) * e + j] = particle;
    edges[std::size_t(field::kEdgeGroup) * e + j] = group;
  }
  monitor::FieldView view() const {
    monitor::FieldView v;
    v.particles = n;
    v.groups = g;
    v.edges = e;
    v.particle_data = particles.data();
    v.group_data = groups.data();
    v.edge_int_data = edges.data();
    return v;
  }
};

// Three unit-mass particles; particle 0 and 1 in group 0, particle 2 in
// group 1.
State base() {
  State s(3, 2, 3);
  for (int i = 0; i < 3; ++i) {
    s.p(field::kPosX, i) = float(i);
    s.p(field::kMass, i) = 1.0f;
  }
  s.edge(0, 0, 0);
  s.edge(1, 1, 0);
  s.edge(2, 2, 1);
  return s;
}

bool near(double a, double b) { return std::fabs(a - b) < 1e-6; }

void check_baseline() {
  harness::note("baseline sample");
  State s = base();
  monitor::FieldMonitor mon;
  const monitor::Sample a = mon.observe(s.view(), 0);
  CHECK_TRUE(near(a.total_mass, 3.0), "total mass is summed");
  CHECK_EQ_INT(a.moving_particles, 0, "the first sample has no motion to report");
  CHECK_EQ_INT(a.active_edges, 0, "the first sample has no active edges");
  CHECK_TRUE(near(a.centre_of_mass_drift, 0.0), "the first sample is the drift origin");
}

void check_motion_and_active_edges() {
  harness::note("motion and the would-be active set");
  State s = base();
  monitor::FieldMonitor mon;
  mon.observe(s.view(), 0);

  s.p(field::kPosX, 2) += 0.5f;
  const monitor::Sample b = mon.observe(s.view(), 10);
  CHECK_EQ_INT(b.ticks_since_previous, 10, "ticks since the previous sample");
  CHECK_EQ_INT(b.moving_particles, 1, "one particle moved");
  CHECK_TRUE(near(b.max_displacement, 0.5), "max displacement is the moved distance");
  CHECK_TRUE(near(b.mean_displacement, 0.5 / 3.0), "mean displacement over all particles");
  CHECK_EQ_INT(b.active_edges, 1, "only the moved particle's edge is active");
  CHECK_TRUE(near(b.centre_of_mass_drift, 0.5 / 3.0), "centre of mass drift from the origin");

  s.c(field::kCenX, 0) += 1.0f;
  const monitor::Sample c = mon.observe(s.view(), 20);
  CHECK_EQ_INT(c.moving_particles, 0, "nothing moved since the last sample");
  CHECK_EQ_INT(c.moving_centroids, 1, "one centroid moved");
  CHECK_EQ_INT(c.active_edges, 2, "both edges into the moved centroid are active");
}

void check_threshold() {
  harness::note("motion threshold");
  State s = base();
  monitor::Settings settings;
  settings.motion_threshold = 0.1f;
  monitor::FieldMonitor mon(settings);
  mon.observe(s.view(), 0);
  s.p(field::kPosX, 0) += 0.05f;
  s.p(field::kPosX, 1) += 0.2f;
  const monitor::Sample b = mon.observe(s.view(), 1);
  CHECK_EQ_INT(b.moving_particles, 1, "motion below the threshold is not counted");
}

void check_reversals_and_reach() {
  harness::note("velocity reversal and brake regime");
  State s = base();
  s.p(field::kVelX, 0) = 1.0f;
  s.p(field::kVelX, 1) = 1.0f;
  monitor::FieldMonitor mon;
  mon.observe(s.view(), 0);

  s.p(field::kVelX, 0) = -1.0f;  // turned around
  s.p(field::kVelX, 1) = 0.8f;   // same direction
  s.p(field::kForceX, 0) = 0.5f;  // travel 1 vs reach 0.5: past reach
  s.p(field::kForceX, 1) = 1.0f;  // travel 0.8 vs reach 1: near reach
  const monitor::Sample b = mon.observe(s.view(), 1);
  CHECK_EQ_INT(b.velocity_reversals, 1, "one particle reversed");
  CHECK_EQ_INT(b.past_reach, 1, "travel beyond reach is counted");
  CHECK_EQ_INT(b.near_reach, 1, "travel close to reach is counted");
  CHECK_TRUE(near(b.max_speed, 1.0), "max speed");
  CHECK_TRUE(near(b.kinetic_proxy, 1.0 + 0.64), "kinetic proxy sums m|v|^2");
}

void check_nonfinite() {
  harness::note("non-finite state");
  State s = base();
  s.p(field::kPosY, 1) = std::numeric_limits<float>::quiet_NaN();
  monitor::FieldMonitor mon;
  const monitor::Sample a = mon.observe(s.view(), 0);
  CHECK_EQ_INT(a.nonfinite_particles, 1, "a NaN position is counted");
  CHECK_TRUE(near(a.total_mass, 2.0), "a non-finite particle is left out of the sums");
}

}  // namespace

int main() {
  check_baseline();
  check_motion_and_active_edges();
  check_threshold();
  check_reversals_and_reach();
  check_nonfinite();
  return harness::report("monitor_test");
}
