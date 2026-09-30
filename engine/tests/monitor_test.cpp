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
  harness::note("baseline window");
  State s = base();
  monitor::FieldMonitor mon;
  mon.observe(s.view(), 0);
  const monitor::Sample a = mon.take_sample();
  CHECK_TRUE(near(a.total_mass, 3.0), "total mass is summed");
  CHECK_EQ_INT(a.steps_observed, 0, "a lone observation has no steps");
  CHECK_EQ_INT(a.touched_particles, 0, "no steps, nothing touched");
  CHECK_EQ_INT(a.motion_edges, 0, "no steps, no motion edges");
  CHECK_TRUE(near(a.centre_of_mass_drift, 0.0), "the first observation is the drift origin");
}

void check_motion_and_motion_edges() {
  harness::note("motion and the touched surface");
  State s = base();
  monitor::FieldMonitor mon;
  mon.observe(s.view(), 0);
  mon.take_sample();

  s.p(field::kPosX, 2) += 0.5f;
  mon.observe(s.view(), 1);
  const monitor::Sample b = mon.take_sample();
  CHECK_EQ_INT(b.steps_observed, 1, "one step in the window");
  CHECK_EQ_INT(b.ticks_per_step, 1, "every tick observed");
  CHECK_EQ_INT(b.touched_particles, 1, "one particle touched");
  CHECK_TRUE(near(b.max_step_displacement, 0.5), "largest single step");
  CHECK_TRUE(near(b.net_max_displacement, 0.5), "net displacement over the window");
  CHECK_TRUE(near(b.net_mean_displacement, 0.5 / 3.0), "net mean over all particles");
  CHECK_EQ_INT(b.motion_edges, 1, "only the moved particle's edge is a motion edge");
  CHECK_TRUE(near(b.centre_of_mass_drift, 0.5 / 3.0), "centre of mass drift from the origin");

  s.c(field::kCenX, 0) += 1.0f;
  mon.observe(s.view(), 2);
  const monitor::Sample c = mon.take_sample();
  CHECK_EQ_INT(c.touched_particles, 0, "the window reset: nothing moved since");
  CHECK_EQ_INT(c.touched_centroids, 1, "one centroid moved");
  CHECK_EQ_INT(c.motion_edges, 2, "both edges into the moved centroid are motion edges");
}

// Motion that returns to where it started inside one window must still
// count: touched and reversals come from every step, net shows zero.
void check_oscillate_and_return() {
  harness::note("oscillate and return within a window");
  State s = base();
  s.p(field::kVelX, 0) = 1.0f;
  monitor::FieldMonitor mon;
  mon.observe(s.view(), 0);
  mon.take_sample();

  s.p(field::kPosX, 0) += 1.0f;   // 0 -> 1
  s.c(field::kCenX, 0) += 0.5f;
  mon.observe(s.view(), 1);
  s.p(field::kPosX, 0) -= 1.0f;   // 1 -> 0
  s.p(field::kVelX, 0) = -1.0f;   // turned around
  s.c(field::kCenX, 0) -= 0.5f;
  mon.observe(s.view(), 2);
  const monitor::Sample w = mon.take_sample();
  CHECK_EQ_INT(w.steps_observed, 2, "two steps in the window");
  CHECK_EQ_INT(w.touched_particles, 1, "the returning particle is still touched");
  CHECK_EQ_INT(w.touched_centroids, 1, "the returning centroid is still touched");
  CHECK_EQ_INT(w.motion_edges, 2, "its edges are still motion edges");
  CHECK_EQ_INT(w.velocity_reversals, 1, "the reversal inside the window is counted");
  CHECK_TRUE(near(w.max_step_displacement, 1.0), "the largest step is recorded");
  CHECK_TRUE(near(w.net_max_displacement, 0.0), "net displacement is zero, labelled as net");
}

// A monitor measures one base. A new base needs reset(), which also moves
// the centre-of-mass drift origin; a changed shape without reset() is an
// error rather than a silent mix of two bases.
void check_new_base() {
  harness::note("new base");
  State first = base();
  monitor::FieldMonitor mon;
  mon.observe(first.view(), 0);
  mon.take_sample();

  State shifted = base();
  for (int i = 0; i < 3; ++i) {
    shifted.p(field::kPosX, i) += 100.0f;
  }
  mon.reset();
  mon.observe(shifted.view(), 0);
  const monitor::Sample a = mon.take_sample();
  CHECK_TRUE(near(a.centre_of_mass_drift, 0.0), "reset() moves the drift origin to the new base");
  CHECK_EQ_INT(a.steps_observed, 0, "reset() starts a fresh baseline");

  State bigger(4, 2, 3);
  bool threw = false;
  try {
    mon.observe(bigger.view(), 1);
  } catch (const std::logic_error &) {
    threw = true;
  }
  CHECK_TRUE(threw, "a changed shape without reset() is refused");
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
  mon.observe(s.view(), 1);
  CHECK_EQ_INT(mon.take_sample().touched_particles, 1, "motion below the threshold is not counted");
}

void check_observation_gap() {
  harness::note("observation gap is reported");
  State s = base();
  monitor::FieldMonitor mon;
  mon.observe(s.view(), 0);
  mon.observe(s.view(), 5);
  CHECK_EQ_INT(mon.take_sample().ticks_per_step, 5, "a coarser observation interval is visible");
}

void check_reversals() {
  harness::note("velocity reversal and last-state motion");
  State s = base();
  s.p(field::kVelX, 0) = 1.0f;
  s.p(field::kVelX, 1) = 1.0f;
  monitor::FieldMonitor mon;
  mon.observe(s.view(), 0);

  s.p(field::kVelX, 0) = -1.0f;  // turned around
  s.p(field::kVelX, 1) = 0.8f;   // same direction
  mon.observe(s.view(), 1);
  const monitor::Sample b = mon.take_sample();
  CHECK_EQ_INT(b.velocity_reversals, 1, "one particle reversed");
  CHECK_TRUE(near(b.max_speed, 1.0), "max speed");
  CHECK_TRUE(near(b.kinetic_proxy, 1.0 + 0.64), "kinetic proxy sums m|v|^2");
}

// A broken run must never read as settled: every field a metric uses is
// checked, and a broken particle or centroid is counted, excluded from the
// sums, and treated as touched.
void check_nonfinite() {
  harness::note("non-finite state");
  const float nan = std::numeric_limits<float>::quiet_NaN();
  const float inf = std::numeric_limits<float>::infinity();
  {
    State s = base();
    s.p(field::kPosY, 1) = nan;
    monitor::FieldMonitor mon;
    mon.observe(s.view(), 0);
    const monitor::Sample a = mon.take_sample();
    CHECK_EQ_INT(a.nonfinite_particles, 1, "a NaN position is counted");
    CHECK_TRUE(near(a.total_mass, 2.0), "a non-finite particle is left out of the sums");
  }
  {
    State s = base();
    monitor::FieldMonitor mon;
    mon.observe(s.view(), 0);
    s.p(field::kMass, 0) = nan;
    mon.observe(s.view(), 1);
    const monitor::Sample a = mon.take_sample();
    CHECK_EQ_INT(a.nonfinite_particles, 1, "a NaN mass is counted");
    CHECK_TRUE(near(a.total_mass, 2.0) && std::isfinite(a.centre_of_mass_drift) &&
                   std::isfinite(a.kinetic_proxy),
               "a NaN mass does not poison mass, drift or kinetic sums");
    CHECK_EQ_INT(a.touched_particles, 1, "a broken particle is never counted as settled");
  }
  {
    State s = base();
    s.p(field::kForceZ, 2) = inf;
    monitor::FieldMonitor mon;
    mon.observe(s.view(), 0);
    CHECK_EQ_INT(mon.take_sample().nonfinite_particles, 1, "a non-finite force is counted");
  }
  {
    State s = base();
    monitor::FieldMonitor mon;
    mon.observe(s.view(), 0);
    s.c(field::kCenY, 1) = nan;
    mon.observe(s.view(), 1);
    const monitor::Sample a = mon.take_sample();
    CHECK_EQ_INT(a.nonfinite_groups, 1, "a non-finite centroid is counted");
    CHECK_EQ_INT(a.touched_centroids, 1, "a broken centroid is never counted as settled");
  }
  {
    // Mass alone goes bad while the centroid position stays put.
    State s = base();
    monitor::FieldMonitor mon;
    mon.observe(s.view(), 0);
    s.c(field::kCenM, 1) = nan;
    mon.observe(s.view(), 1);
    const monitor::Sample a = mon.take_sample();
    CHECK_EQ_INT(a.nonfinite_groups, 1, "a group with only a non-finite mass is counted");
    CHECK_EQ_INT(a.touched_centroids, 1, "a group with only a non-finite mass is never settled");
    CHECK_EQ_INT(a.motion_edges, 1, "its one edge is flagged in the motion diagnostic");
  }
}

}  // namespace

int main() {
  check_baseline();
  check_motion_and_motion_edges();
  check_oscillate_and_return();
  check_new_base();
  check_threshold();
  check_observation_gap();
  check_reversals();
  check_nonfinite();
  return harness::report("monitor_test");
}
