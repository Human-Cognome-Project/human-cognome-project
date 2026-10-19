#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "engine/runtime.h"
#include "taichi/program/ndarray.h"

// A basic harness for the field model: one law, applied without exception.
//
// Field attraction is mass times mass over separation squared, evaluated
// against a group centroid rather than against another particle. A group's
// centroid always includes the subject. Parent and sibling participation are
// one edge shape: proportional participation acts at the particle centre.
//
// Nothing here decides what the data means. It moves state through one tick.
namespace field {

namespace detail {
class Activity;
}
enum class TickMode { Selective, AllLoaded };
using FrameAnchor = std::array<double, 3>;
struct Demand {
  std::uint64_t particles_loaded{0}, particles_exposed{0}, fields_loaded{0},
      fields_exposed{0}, edges_loaded{0}, edges_exposed{0};
  std::uint64_t particles_evaluated{0}, particles_skipped{0},
      particles_moved{0}, particles_woken{0};
  std::uint64_t force_requests{0}, force_evaluations{0}, force_zero_response{0},
      force_skipped{0}, contact_evaluations{0};
  std::uint64_t gate_checks{0}, gate_rejected{0}, geometry_rejected{0};
  std::uint64_t touch_adjacency_visits{0}, force_mask_visits{0},
      centroid_mask_visits{0};
  std::uint64_t centroid_gate_visits{0};
  std::uint64_t centroid_touch_edges{0}, centroid_reduction_edges{0};
  std::uint64_t centroids_flagged{0}, centroids_recomputed{0};
  std::uint64_t native_particle_entries{0}, native_centroid_entries{0};
  std::uint64_t zero_response_touches{0}, invalidation_particle_visits{0};
  std::array<float, 3> focal_origin{};
  FrameAnchor frame_anchor{};
  double tick_ms{0};
};
enum class HostPath { BulkCopy, CpuDirect };
struct PhaseTimes {
  double allocation_ms{0}, compilation_ms{0}, upload_ms{0}, seed_ms{0},
      kernel_ms{0}, report_ms{0};
  std::size_t upload_bytes{0}, download_bytes{0};
};

// Packed structure of arrays. Element f of item i lives at f * count + i, so
// every pass reads one contiguous run per field.
enum ParticleField {
  kPosX = 0,
  kPosY,
  kPosZ,
  kVelX,
  kVelY,
  kVelZ,
  kMass,
  kRadius,
  kForceX,
  kForceY,
  kForceZ,
  kParticleFieldCount
};

enum GroupField {
  // Read by the force pass: the centroid as of the end of the previous tick.
  kCenX = 0,
  kCenY,
  kCenZ,
  kCenM,
  // Weighted offsets from the previous published centre, plus cached mass.
  // Reducing offsets preserves collective sub-ULP changes in large frames.
  kAccX,
  kAccY,
  kAccZ,
  kAccM,
  kGroupFieldCount
};

// Participation of one particle in one group.
enum EdgeFloatField {
  // Share of the particle's total mass taking part. One for whole
  // participation, a fraction for a component.
  kShare = 0,
  kEdgeFloatFieldCount
};

enum EdgeIntField { kEdgeParticle = 0, kEdgeGroup, kEdgeIntFieldCount };

// Explicit ledgered body-contact coverage; no all-pairs discovery pass.
enum ContactIntField { kContactA = 0, kContactB, kContactIntFieldCount };

struct Parameters {
  // Calibration: two unconstrained mass-1 particles at separation 1 overlap at
  // their centroid in one time unit. Because the centroid carries the total
  // group mass and includes the subject, this is pi^2/128 rather than the
  // pairwise pi^2/16.
  float gravity{0.0771063443835106f};
  // One tick is one time unit; the brake is what makes a step that coarse
  // safe.
  float dt{1.0f};
  // Finite resistance to penetration of the summed spherical radii.
  // This is not a hard non-overlap constraint. At exact coincidence the
  // direction gate gives zero force, without separating the centres.
  float contact_stiffness{1.0f};
  // Radial velocity dampening reach: the field-strength threshold M/d^2 at
  // which the reach sits. Inside the reach (M/d^2 above this threshold) the
  // dampening is inert; beyond it, outward radial velocity is bled off. A
  // trial value -- reach == sqrt(M / reach_strength), so at 1.0 the reach
  // sits at sqrt of the construct's total mass; calibrate later.
  float reach_strength{1.0f};
};

// One instance of the model's state on one runtime. Host vectors stage the
// same packed layout as the device arrays.
class Harness {
 public:
  Harness(engine::Runtime &runtime, int particle_count, int group_count,
          int edge_count, int contact_count = 0,
          HostPath path = HostPath::BulkCopy);
  ~Harness();

  int particle_count() const { return particles_; }
  int group_count() const { return groups_; }
  int edge_count() const { return edges_; }
  int contact_count() const { return contacts_; }
  bool ready() const { return ready_; }
  void set_tick_mode(TickMode mode);
  // Explicit prepared group slots, not automatic focal-field selection.
  // An empty selection retains the legacy fixed coordinate origin.
  void set_focus(const std::vector<int> &groups);
  const Demand &demand() const { return demand_; }
  const FrameAnchor &frame_anchor() const { return frame_anchor_; }
  // Positions are local numerical values. Add this reference in double only
  // when interpreting an explicitly downloaded result in supplied coordinates.
  double supplied_position(int axis, int slot) const {
    return frame_anchor_.at(axis) + particle(kPosX + axis, slot);
  }
  const PhaseTimes &timings() const { return timings_; }

  // Host staging, in the packed layout above.
  std::vector<float> particle_data;
  std::vector<float> group_data;
  std::vector<float> edge_float_data;
  std::vector<int> edge_int_data;
  std::vector<int> contact_int_data;
  // The universal mass: the total mass of the construct, each particle
  // counted exactly once, used by radial velocity damping.
  // Rebuilt on-device at publication and reused until the next publication;
  // not staged from the host.
  std::vector<float> universal_data;

  // Offsets are computed in size_t: f * count overflows int once the item
  // count passes ~2^31 / field count.
  float &particle(int f, int i) {
    return particle_data[offset(f, particles_, i)];
  }
  float particle(int f, int i) const {
    return particle_data[offset(f, particles_, i)];
  }
  float &group(int f, int i) { return group_data[offset(f, groups_, i)]; }
  float group(int f, int i) const { return group_data[offset(f, groups_, i)]; }
  float &edge_float(int f, int i) {
    return edge_float_data[offset(f, edges_, i)];
  }
  int &edge_int(int f, int i) { return edge_int_data[offset(f, edges_, i)]; }
  int &contact_int(int f, int i) {
    return contact_int_data[offset(f, contacts_, i)];
  }

  // Throws std::out_of_range, before transferring anything, if a staged
  // edge or contact names a particle or group outside this harness's counts.
  // Malformed numerical staging and references to mass-zero free slots are
  // also refused. Upload invalidates readiness until centroids are seeded.
  void upload();
  // Diagnostic scalar bit comparison before seeding; not a reporting/refresh
  // path and not included in timed payload transfer measurements.
  bool uploaded_payload_matches();
  void download();
  void invalidate();
  void publish(const Parameters &p);
  void publish(const Parameters &p, const FrameAnchor &anchor);

  // Build the centroids once from the staged state, so the first tick reads a
  // consistent field rather than zeros.
  void seed_centroids(const Parameters &p);

  // Force, integrate, determiners, then the consolidation that builds the
  // centroids the next tick will read.
  void tick(const Parameters &p);

  // Products of the primary pass, one per particle, written by the determiner
  // stage. Speed for now: the stage exists to hold the shape.
  const std::vector<float> &determined() const { return determined_; }

 private:
  friend class detail::Activity;
  static std::size_t offset(int f, int count, int i) {
    return std::size_t(f) * std::size_t(count) + std::size_t(i);
  }
  void validate_staging() const;
  template <class T>
  void bind(taichi::lang::LaunchContextBuilder &ctx, int argument,
            const taichi::lang::Ndarray *array, std::vector<T> &values,
            std::vector<int> shape);

  void launch_clear(bool seed);
  void launch_force(const Parameters &p);
  void launch_contact(const Parameters &p);
  void launch_integrate(const Parameters &p);
  void launch_determine();
  void launch_centroid(const Parameters &p, bool seed);

  engine::Runtime *runtime_;
  int particles_;
  int groups_;
  int edges_;
  int contacts_;
  bool uploaded_{false};
  bool ready_{false};
  TickMode mode_{TickMode::Selective};
  bool invalidated_{true};
  Parameters previous_parameters_;
  std::uint64_t exposed_{0};
  std::uint64_t exposed_fields_{0};
  Demand demand_;
  FrameAnchor frame_anchor_{};
  std::unique_ptr<detail::Activity> activity_;
  HostPath path_;
  std::uint64_t generation_{0};
  PhaseTimes timings_;

  std::unique_ptr<taichi::lang::Ndarray> particle_array_;
  std::unique_ptr<taichi::lang::Ndarray> group_array_;
  std::unique_ptr<taichi::lang::Ndarray> edge_float_array_;
  std::unique_ptr<taichi::lang::Ndarray> edge_int_array_;
  std::unique_ptr<taichi::lang::Ndarray> contact_int_array_;
  std::unique_ptr<taichi::lang::Ndarray> determined_array_;
  std::unique_ptr<taichi::lang::Ndarray> universal_array_;

  std::vector<float> determined_;
  std::vector<float> empty_determined_{0.0f};

  engine::CompiledKernel clear_;
  engine::CompiledKernel force_;
  engine::CompiledKernel contact_;
  engine::CompiledKernel integrate_;
  engine::CompiledKernel determine_;
  engine::CompiledKernel centroid_;
  engine::CompiledKernel finalize_;
};

}  // namespace field
