#include "monitor/field_monitor.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

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

namespace {

bool particle_finite(const float *p, int n, int i) {
  static const int fields[] = {field::kPosX,   field::kPosY,   field::kPosZ,
                               field::kVelX,   field::kVelY,   field::kVelZ,
                               field::kMass,   field::kForceX, field::kForceY,
                               field::kForceZ};
  for (int f : fields) {
    if (!std::isfinite(at(p, f, n, i))) {
      return false;
    }
  }
  return true;
}

bool group_finite(const float *g, int count, int k) {
  for (int f = 0; f < field::kGroupFieldCount; ++f) {
    if (!std::isfinite(at(g, f, count, k))) {
      return false;
    }
  }
  return true;
}

double position_distance(const float *a, const float *b, int n, int i) {
  return length3(double(at(a, field::kPosX, n, i)) - at(b, field::kPosX, n, i),
                 double(at(a, field::kPosY, n, i)) - at(b, field::kPosY, n, i),
                 double(at(a, field::kPosZ, n, i)) - at(b, field::kPosZ, n, i));
}

// Centre of mass over finite particles; returns false if there is no mass.
bool centre_of_mass(const float *p, int n, double out[3], double *mass) {
  double m_total = 0.0, c[3] = {0.0, 0.0, 0.0};
  for (int i = 0; i < n; ++i) {
    if (!particle_finite(p, n, i)) {
      continue;
    }
    const double m = at(p, field::kMass, n, i);
    m_total += m;
    c[0] += m * at(p, field::kPosX, n, i);
    c[1] += m * at(p, field::kPosY, n, i);
    c[2] += m * at(p, field::kPosZ, n, i);
  }
  *mass = m_total;
  for (int k = 0; k < 3; ++k) {
    out[k] = m_total > 0.0 ? c[k] / m_total : 0.0;
  }
  return m_total > 0.0;
}

}  // namespace

FieldMonitor::State FieldMonitor::copy_of(const FieldView &view, long tick) {
  State s;
  s.tick = tick;
  s.n = view.particles;
  s.g = view.groups;
  s.e = view.edges;
  s.particles.assign(view.particle_data,
                     view.particle_data + std::size_t(s.n) * field::kParticleFieldCount);
  s.groups.assign(view.group_data,
                  view.group_data + std::size_t(s.g) * field::kGroupFieldCount);
  s.edges.assign(view.edge_int_data,
                 view.edge_int_data + std::size_t(s.e) * field::kEdgeIntFieldCount);
  return s;
}

void FieldMonitor::reset() {
  have_last_ = false;
  com0_[0] = com0_[1] = com0_[2] = 0.0;
  window_start_ = State{};
  last_ = State{};
  steps_ = 0;
  max_gap_ = 0;
  max_step_displacement_ = 0.0;
  reversals_ = 0;
  touched_particle_.clear();
  touched_centroid_.clear();
}

void FieldMonitor::observe(const FieldView &view, long tick) {
  if (have_last_ && (view.particles != last_.n || view.groups != last_.g ||
                     view.edges != last_.e)) {
    throw std::logic_error(
        "FieldMonitor::observe: loaded shape changed; call reset() for a new base");
  }
  State cur = copy_of(view, tick);
  if (!have_last_) {
    // First observation of this base: it is the baseline.
    double mass = 0.0;
    centre_of_mass(cur.particles.data(), cur.n, com0_, &mass);
    window_start_ = cur;
    touched_particle_.assign(std::size_t(cur.n), 0);
    touched_centroid_.assign(std::size_t(cur.g), 0);
    steps_ = 0;
    max_gap_ = 0;
    max_step_displacement_ = 0.0;
    reversals_ = 0;
    last_ = std::move(cur);
    have_last_ = true;
    return;
  }

  const int n = cur.n;
  const float *p = cur.particles.data();
  const float *q = last_.particles.data();
  for (int i = 0; i < n; ++i) {
    if (!particle_finite(p, n, i) || !particle_finite(q, n, i)) {
      touched_particle_[std::size_t(i)] = 1;  // a broken particle is never settled
      continue;
    }
    const double d = position_distance(p, q, n, i);
    max_step_displacement_ = std::max(max_step_displacement_, d);
    if (d > settings_.motion_threshold) {
      touched_particle_[std::size_t(i)] = 1;
    }
    const double dot = double(at(p, field::kVelX, n, i)) * at(q, field::kVelX, n, i) +
                       double(at(p, field::kVelY, n, i)) * at(q, field::kVelY, n, i) +
                       double(at(p, field::kVelZ, n, i)) * at(q, field::kVelZ, n, i);
    if (dot < 0.0) {
      ++reversals_;
    }
  }
  const int g = cur.g;
  const float *gc = cur.groups.data();
  const float *gq = last_.groups.data();
  for (int k = 0; k < g; ++k) {
    const double d = length3(double(at(gc, field::kCenX, g, k)) - at(gq, field::kCenX, g, k),
                             double(at(gc, field::kCenY, g, k)) - at(gq, field::kCenY, g, k),
                             double(at(gc, field::kCenZ, g, k)) - at(gq, field::kCenZ, g, k));
    if (!(d <= settings_.motion_threshold)) {  // NaN counts as touched
      touched_centroid_[std::size_t(k)] = 1;
    }
  }
  max_gap_ = std::max(max_gap_, cur.tick - last_.tick);
  ++steps_;
  last_ = std::move(cur);
}

Sample FieldMonitor::take_sample() {
  Sample s;
  if (!have_last_) {
    return s;
  }
  const int n = last_.n;
  const int g = last_.g;
  const float *p = last_.particles.data();
  s.tick = last_.tick;
  s.steps_observed = steps_;
  s.ticks_per_step = max_gap_;
  s.max_step_displacement = max_step_displacement_;
  s.velocity_reversals = reversals_;

  for (int k = 0; k < g; ++k) {
    if (!group_finite(last_.groups.data(), g, k)) {
      ++s.nonfinite_groups;
    }
  }

  std::vector<double> net;
  net.reserve(std::size_t(n));
  const float *w = window_start_.particles.data();
  for (int i = 0; i < n; ++i) {
    if (!particle_finite(p, n, i)) {
      ++s.nonfinite_particles;
      continue;
    }
    const double m = at(p, field::kMass, n, i);
    const double speed = length3(at(p, field::kVelX, n, i), at(p, field::kVelY, n, i),
                                 at(p, field::kVelZ, n, i));
    s.max_speed = std::max(s.max_speed, speed);
    s.kinetic_proxy += m * speed * speed;
    const double reach = length3(at(p, field::kForceX, n, i), at(p, field::kForceY, n, i),
                                 at(p, field::kForceZ, n, i));
    if (reach > 0.0) {
      const double ratio = speed * settings_.dt / reach;
      if (ratio > 1.0) {
        ++s.past_reach;
      } else if (ratio > 0.5) {
        ++s.near_reach;
      }
    }
    if (steps_ > 0 && particle_finite(w, n, i)) {
      net.push_back(position_distance(p, w, n, i));
    }
  }

  double com[3];
  centre_of_mass(p, n, com, &s.total_mass);
  s.centre_of_mass_drift =
      length3(com[0] - com0_[0], com[1] - com0_[1], com[2] - com0_[2]);

  if (!net.empty()) {
    double sum = 0.0;
    for (double d : net) {
      sum += d;
      s.net_max_displacement = std::max(s.net_max_displacement, d);
    }
    s.net_mean_displacement = sum / double(net.size());
    const std::size_t k = std::min(
        net.size() - 1, std::size_t(std::ceil(0.99 * double(net.size()))) - 1);
    std::nth_element(net.begin(), net.begin() + k, net.end());
    s.net_p99_displacement = net[k];
  }

  for (char t : touched_particle_) s.touched_particles += t;
  for (char t : touched_centroid_) s.touched_centroids += t;
  const int e = last_.e;
  for (int j = 0; j < e; ++j) {
    const int pi = last_.edges[std::size_t(field::kEdgeParticle) * e + j];
    const int gi = last_.edges[std::size_t(field::kEdgeGroup) * e + j];
    if ((pi >= 0 && pi < n && touched_particle_[std::size_t(pi)]) ||
        (gi >= 0 && gi < g && touched_centroid_[std::size_t(gi)])) {
      ++s.touched_edges;
    }
  }

  // The last state opens the next window.
  window_start_ = last_;
  std::fill(touched_particle_.begin(), touched_particle_.end(), 0);
  std::fill(touched_centroid_.begin(), touched_centroid_.end(), 0);
  steps_ = 0;
  max_gap_ = 0;
  max_step_displacement_ = 0.0;
  reversals_ = 0;
  return s;
}

void write_csv_header(std::FILE *out) {
  std::fprintf(out,
               "tick,steps_observed,ticks_per_step,nonfinite_particles,"
               "nonfinite_groups,total_mass,centre_of_mass_drift,"
               "max_step_displacement,touched_particles,touched_centroids,"
               "touched_edges,velocity_reversals,net_max_displacement,"
               "net_mean_displacement,net_p99_displacement,max_speed,"
               "kinetic_proxy,near_reach,past_reach,tick_ms,download_ms,"
               "download_bytes\n");
}

void write_csv_row(std::FILE *out, const Sample &s) {
  std::fprintf(out,
               "%ld,%ld,%ld,%d,%d,%.9g,%.9g,%.9g,%d,%d,%d,%d,%.9g,%.9g,%.9g,"
               "%.9g,%.9g,%d,%d,%.6f,%.6f,%zu\n",
               s.tick, s.steps_observed, s.ticks_per_step, s.nonfinite_particles,
               s.nonfinite_groups, s.total_mass, s.centre_of_mass_drift,
               s.max_step_displacement, s.touched_particles, s.touched_centroids,
               s.touched_edges, s.velocity_reversals, s.net_max_displacement,
               s.net_mean_displacement, s.net_p99_displacement, s.max_speed,
               s.kinetic_proxy, s.near_reach, s.past_reach, s.tick_ms,
               s.download_ms, s.download_bytes);
}

}  // namespace monitor
