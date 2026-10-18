#pragma once

#include "field/field.h"

namespace field::detail {
enum ParticleState {
  Candidate,
  Continued,
  Moving,
  PriorMoving,
  ParticleStateCount
};
enum GroupState { Flagged, Changed, GroupStateCount };
enum Counter {
  Evaluated,
  ForceRequests,
  Forces,
  Contacts,
  Moved,
  Woken,
  Flags,
  Recomputed,
  Rejected,
  GeometryRejected,
  GeometryChecks,
  TouchEdges,
  ReducedEdges,
  ExposedFields,
  CounterCount
};

// Numerical control state belongs to the execution session. It carries no
// source identities, private codes, or compiler specialization values.
class Activity {
 public:
  Activity(engine::Runtime &runtime, int particles, int groups, int edges,
           int contacts);
  void reset();
  void index(const std::vector<int> &edges, const std::vector<int> &contacts);
  void set_focus(const std::vector<int> &groups);
  void set_frame_anchor(const FrameAnchor &anchor);
  void begin(Harness &h, bool all_loaded, bool invalidate);
  void touch(Harness &h);
  void mark(Harness &h);
  Demand report(std::uint64_t exposed);
  double compilation_ms{0};
  std::unique_ptr<taichi::lang::Ndarray> particle_flags, particle_delta;
  // pending weighted member motion xyz, previous published centroid xyz.
  std::unique_ptr<taichi::lang::Ndarray> group_flags, group_delta, counters,
      focus;
  // Stable source order and one writer per numerical reduction.
  std::unique_ptr<taichi::lang::Ndarray> particle_offsets, particle_edges,
      group_offsets, group_edges, contact_a_offsets, contact_a_edges,
      contact_b_offsets, contact_b_edges;

 private:
  engine::Runtime &runtime_;
  int p_, g_, e_, k_;
  std::vector<int> focus_mask_;
  std::array<float, 3> fixed_origin_{};
  std::unique_ptr<taichi::lang::Ndarray> focus_mask_array_;
  engine::CompiledKernel begin_, touch_, mark_;
};
}  // namespace field::detail
