#pragma once

#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

#include "field/field.h"

// Observation of the field model, never participation in it.
//
// The monitor reads host arrays the caller has already downloaded and
// compares them with the previous sample. It launches no kernels, writes no
// engine state, and has no opinion about what a tick should do. Every
// quantity below is measured over the ticks since the previous sample.
namespace monitor {

// The packed host layout of one field state, as field::Harness stages it.
// A plain view so the measurements can be exercised without a runtime.
struct FieldView {
  int particles = 0;
  int groups = 0;
  int edges = 0;
  const float *particle_data = nullptr;  // kParticleFieldCount x particles
  const float *group_data = nullptr;     // kGroupFieldCount x groups
  const int *edge_int_data = nullptr;    // kEdgeIntFieldCount x edges
};

// View of a harness after download().
FieldView view_of(const field::Harness &h);

struct Settings {
  // A particle or centroid counts as touched in a window when any single
  // observed step moved it farther than this.
  float motion_threshold = 1e-4f;
};

// One window of observations. Step quantities are accumulated over every
// observed step in the window, so motion that returns to where it started
// is still counted; net quantities compare only the window's first and
// last states and are labelled as such.
struct Sample {
  long tick = 0;                // tick of the last observation in the window
  long steps_observed = 0;      // observed steps accumulated in this window
  long ticks_per_step = 0;      // largest tick gap between observations
                                // (1 means every tick was observed)

  // Integrity of the last observed state. A particle is non-finite when
  // any of position, velocity, mass or force is; a group when any centroid
  // or accumulator field is. Non-finite particles are left out of every
  // sum below, so a broken run cannot read as settled: check these first.
  int nonfinite_particles = 0;
  int nonfinite_groups = 0;
  double total_mass = 0.0;
  double centre_of_mass_drift = 0.0;  // from the first observation

  // Accumulated over every observed step in the window.
  double max_step_displacement = 0.0;
  int touched_particles = 0;   // moved past the threshold on any step
  int touched_centroids = 0;
  // Motion diagnostic only: edges whose particle or centroid moved past the
  // threshold. It is not the calculable surface. A particle can be still
  // because its fields balance, so motion is not a predicate for excluding
  // work, and the current tick evaluates every loaded edge regardless.
  // Evaluated, skipped and woken work must be counted by the control
  // implementation, not inferred here.
  int motion_edges = 0;
  int velocity_reversals = 0;  // summed over steps: velocity turned > 90 degrees

  // Net over the window: first observation to last. Motion that returned
  // to where it started does not show here.
  double net_max_displacement = 0.0;
  double net_mean_displacement = 0.0;
  double net_p99_displacement = 0.0;

  // The last observed state.
  double max_speed = 0.0;
  double kinetic_proxy = 0.0;  // sum of m * |v|^2

  // Cost, filled in by the caller that timed the work.
  double tick_ms = 0.0;
  double download_ms = 0.0;
  std::size_t download_bytes = 0;
};

class FieldMonitor {
 public:
  explicit FieldMonitor(Settings settings = {}) : settings_(settings) {}

  // Record the state at `tick`. Call it for every tick whose effect should
  // count (every tick, for settling and surface claims); each call after
  // the first is one step accumulated into the open window.
  //
  // A monitor measures one loaded base. If the loaded shape (particle,
  // group or edge count) changes, observe() throws std::logic_error rather
  // than silently mixing two bases; call reset() when loading a new base.
  void observe(const FieldView &view, long tick);

  // Start a new base: forget every previous observation, including the
  // centre-of-mass drift origin. The next observe() is the new baseline.
  void reset();

  // Close the open window and return its sample. The last observed state
  // becomes the start of the next window. Requires at least one observe().
  Sample take_sample();

  const Settings &settings() const { return settings_; }

 private:
  struct State {
    long tick = 0;
    std::vector<float> particles;
    std::vector<float> groups;
    std::vector<int> edges;
    int n = 0, g = 0, e = 0;
  };
  static State copy_of(const FieldView &view, long tick);

  Settings settings_;
  double com0_[3] = {0.0, 0.0, 0.0};
  State window_start_;
  State last_;
  bool have_last_ = false;
  long steps_ = 0;
  long max_gap_ = 0;
  double max_step_displacement_ = 0.0;
  int reversals_ = 0;
  std::vector<char> touched_particle_;
  std::vector<char> touched_centroid_;
};

// One CSV row per sample, header first.
void write_csv_header(std::FILE *out);
void write_csv_row(std::FILE *out, const Sample &s);

}  // namespace monitor
