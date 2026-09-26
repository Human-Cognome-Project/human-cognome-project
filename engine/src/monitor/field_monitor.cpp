#include "monitor/field_monitor.h"

#include <algorithm>
#include <cmath>

namespace monitor {
namespace {

float at(const float *data, int field, int count, int i) {
  return data[std::size_t(field) * std::size_t(count) + std::size_t(i)];
}

double length3(double x, double y, double z) {
  return std::sqrt(x * x + y * y + z * z);
}

}  // namespace

FieldView view_of(const field::Harness &h) {
  FieldView v;
  v.particles = h.particle_count();
  v.groups = h.group_count();
  v.edges = h.edge_count();
  v.particle_data = h.particle_data.data();
  v.group_data = h.group_data.data();
  v.edge_int_data = h.edge_int_data.data();
  return v;
}

Sample FieldMonitor::observe(const FieldView &view, long tick) {
  Sample s;
  s.tick = tick;
  const int n = view.particles;
  const int g = view.groups;
  const float *p = view.particle_data;
  const bool have_previous =
      have_previous_ &&
      previous_particles_.size() == std::size_t(n) * field::kParticleFieldCount &&
      previous_groups_.size() == std::size_t(g) * field::kGroupFieldCount;
  s.ticks_since_previous = have_previous ? tick - previous_tick_ : 0;

  double com[3] = {0.0, 0.0, 0.0};
  std::vector<double> displacement;
  displacement.reserve(have_previous ? std::size_t(n) : 0);
  std::vector<char> particle_moved(std::size_t(n), 0);

  for (int i = 0; i < n; ++i) {
    const float x = at(p, field::kPosX, n, i);
    const float y = at(p, field::kPosY, n, i);
    const float z = at(p, field::kPosZ, n, i);
    const float vx = at(p, field::kVelX, n, i);
    const float vy = at(p, field::kVelY, n, i);
    const float vz = at(p, field::kVelZ, n, i);
    const float m = at(p, field::kMass, n, i);
    if (!(std::isfinite(x) && std::isfinite(y) && std::isfinite(z) &&
          std::isfinite(vx) && std::isfinite(vy) && std::isfinite(vz))) {
      ++s.nonfinite_particles;
      continue;
    }
    s.total_mass += m;
    com[0] += double(m) * x;
    com[1] += double(m) * y;
    com[2] += double(m) * z;
    const double speed = length3(vx, vy, vz);
    s.max_speed = std::max(s.max_speed, speed);
    s.kinetic_proxy += double(m) * speed * speed;

    const double reach = length3(at(p, field::kForceX, n, i),
                                 at(p, field::kForceY, n, i),
                                 at(p, field::kForceZ, n, i));
    if (reach > 0.0) {
      const double ratio = speed * settings_.dt / reach;
      if (ratio > 1.0) {
        ++s.past_reach;
      } else if (ratio > 0.5) {
        ++s.near_reach;
      }
    }

    if (have_previous) {
      const float *q = previous_particles_.data();
      const double d = length3(x - at(q, field::kPosX, n, i),
                               y - at(q, field::kPosY, n, i),
                               z - at(q, field::kPosZ, n, i));
      displacement.push_back(d);
      if (d > settings_.motion_threshold) {
        particle_moved[std::size_t(i)] = 1;
        ++s.moving_particles;
      }
      const double dot = double(vx) * at(q, field::kVelX, n, i) +
                         double(vy) * at(q, field::kVelY, n, i) +
                         double(vz) * at(q, field::kVelZ, n, i);
      if (dot < 0.0) {
        ++s.velocity_reversals;
      }
    }
  }

  if (s.total_mass > 0.0) {
    for (double &c : com) {
      c /= s.total_mass;
    }
  }
  if (!have_previous_) {
    std::copy(com, com + 3, com0_);
  }
  s.centre_of_mass_drift =
      length3(com[0] - com0_[0], com[1] - com0_[1], com[2] - com0_[2]);

  if (!displacement.empty()) {
    double sum = 0.0;
    for (double d : displacement) {
      sum += d;
      s.max_displacement = std::max(s.max_displacement, d);
    }
    s.mean_displacement = sum / double(displacement.size());
    const std::size_t k = std::min(
        displacement.size() - 1,
        std::size_t(std::ceil(0.99 * double(displacement.size()))) - 1);
    std::nth_element(displacement.begin(), displacement.begin() + k,
                     displacement.end());
    s.p99_displacement = displacement[k];
  }

  std::vector<char> centroid_moved(std::size_t(g), 0);
  if (have_previous) {
    const float *gd = view.group_data;
    const float *gq = previous_groups_.data();
    for (int k = 0; k < g; ++k) {
      const double d = length3(at(gd, field::kCenX, g, k) - at(gq, field::kCenX, g, k),
                               at(gd, field::kCenY, g, k) - at(gq, field::kCenY, g, k),
                               at(gd, field::kCenZ, g, k) - at(gq, field::kCenZ, g, k));
      if (!(d <= settings_.motion_threshold)) {  // NaN counts as moved
        centroid_moved[std::size_t(k)] = 1;
        ++s.moving_centroids;
      }
    }
    const int e = view.edges;
    for (int j = 0; j < e; ++j) {
      const int pi = view.edge_int_data[std::size_t(field::kEdgeParticle) * e + j];
      const int gi = view.edge_int_data[std::size_t(field::kEdgeGroup) * e + j];
      const bool touched =
          (pi >= 0 && pi < n && particle_moved[std::size_t(pi)]) ||
          (gi >= 0 && gi < g && centroid_moved[std::size_t(gi)]);
      if (touched) {
        ++s.active_edges;
      }
    }
  }

  previous_particles_.assign(p, p + std::size_t(n) * field::kParticleFieldCount);
  previous_groups_.assign(view.group_data,
                          view.group_data + std::size_t(g) * field::kGroupFieldCount);
  previous_tick_ = tick;
  have_previous_ = true;
  return s;
}

void write_csv_header(std::FILE *out) {
  std::fprintf(out,
               "tick,ticks_since_previous,nonfinite_particles,total_mass,"
               "centre_of_mass_drift,max_displacement,mean_displacement,"
               "p99_displacement,max_speed,kinetic_proxy,moving_particles,"
               "moving_centroids,active_edges,velocity_reversals,near_reach,"
               "past_reach,tick_ms,download_ms,download_bytes\n");
}

void write_csv_row(std::FILE *out, const Sample &s) {
  std::fprintf(out,
               "%ld,%ld,%d,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%d,%d,%d,%d,%d,%d,"
               "%.6f,%.6f,%zu\n",
               s.tick, s.ticks_since_previous, s.nonfinite_particles,
               s.total_mass, s.centre_of_mass_drift, s.max_displacement,
               s.mean_displacement, s.p99_displacement, s.max_speed,
               s.kinetic_proxy, s.moving_particles, s.moving_centroids,
               s.active_edges, s.velocity_reversals, s.near_reach,
               s.past_reach, s.tick_ms, s.download_ms, s.download_bytes);
}

}  // namespace monitor
