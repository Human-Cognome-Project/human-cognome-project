#pragma once

#include "field/field.h"
#include "field/native_set.h"

namespace field::detail {
enum ParticleState { Moving, PriorMoving, ParticleStateCount };
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
  GroupGateEntries,
  GateEdges,
  AdjacencyVisits,
  ZeroTouches,
  InvalidatedSlots,
  CounterCount
};

// Numerical control state belongs to the execution session. It carries no
// source identities, private codes, or compiler specialization values.
class Activity {
 public:
  Activity(engine::Runtime &runtime,
           int particles,
           int groups,
           int edges,
           int contacts,
           PreparedStorage *prepared = nullptr);
  void provision();
  std::size_t prepared_binding_bytes() const;
  void reset();
  void index(const std::vector<int> &edges, const std::vector<int> &contacts);
  void set_focus(const std::vector<int> &groups);
  void set_frame_anchor(const FrameAnchor &anchor);
  void begin(Harness &h, bool all_loaded, bool invalidate, bool radial);
  void touch(Harness &h, float gravity);
  void touch_contacts(Harness &h, float stiffness);
  void mark(Harness &h, bool all_loaded);
  void clear();  // Publication only; type phases consume their own activity.
  Demand report(std::uint64_t exposed, std::uint64_t fields_exposed);
  NativeSet particle_activity, centroids;
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
  PreparedStorage *prepared_{};
  std::vector<int> zero_workspace_;
  std::vector<int> selected_slots_;
  std::vector<int> selected_groups_;
  std::array<float, 3> fixed_origin_{};
  std::unique_ptr<taichi::lang::Ndarray> selected_slots_array_;
  engine::CompiledKernel begin_, touch_, contact_touch_, mark_, reset_, clear_;
};
}  // namespace field::detail
