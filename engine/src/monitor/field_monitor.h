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
  // A particle or centroid counts as moving when it travelled farther than
  // this since the previous sample.
  float motion_threshold = 1e-4f;
  // The step the tick integrated with; used to turn speed into travel.
  float dt = 1.0f;
};

struct Sample {
  long tick = 0;
  long ticks_since_previous = 0;

  // Integrity.
  int nonfinite_particles = 0;
  double total_mass = 0.0;
  double centre_of_mass_drift = 0.0;  // from the first sample

  // Motion since the previous sample.
  double max_displacement = 0.0;
  double mean_displacement = 0.0;
  double p99_displacement = 0.0;
  double max_speed = 0.0;
  double kinetic_proxy = 0.0;  // sum of m * |v|^2

  // The surface that would need work if only what changed were computed.
  int moving_particles = 0;
  int moving_centroids = 0;
  int active_edges = 0;  // edges whose particle or centroid moved

  // Lag expression: particles whose velocity turned by more than 90 degrees.
  int velocity_reversals = 0;

  // Post-brake travel |v|*dt against reach |F_net|.
  int near_reach = 0;  // 0.5 < travel/reach <= 1
  int past_reach = 0;  // travel/reach > 1

  // Cost, filled in by the caller that timed the work.
  double tick_ms = 0.0;
  double download_ms = 0.0;
  std::size_t download_bytes = 0;
};

class FieldMonitor {
 public:
  explicit FieldMonitor(Settings settings = {}) : settings_(settings) {}

  // Measure `view` as the state at `tick`. The first call establishes the
  // baseline: motion fields are zero and the centre of mass is recorded.
  Sample observe(const FieldView &view, long tick);

  const Settings &settings() const { return settings_; }

 private:
  Settings settings_;
  bool have_previous_ = false;
  long previous_tick_ = 0;
  std::vector<float> previous_particles_;
  std::vector<float> previous_groups_;
  double com0_[3] = {0.0, 0.0, 0.0};
};

// One CSV row per sample, header first.
void write_csv_header(std::FILE *out);
void write_csv_row(std::FILE *out, const Sample &s);

}  // namespace monitor
