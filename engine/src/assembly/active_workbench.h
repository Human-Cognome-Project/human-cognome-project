#pragma once

#include <map>
#include <optional>

#include "assembly/membership_fields.h"
#include "assembly/study.h"

namespace assembly {
struct LocalRef {
  warm::ObjectKey object;
  warm::Uuid version, owner;
};
struct LocalValue {
  LocalRef ref;
  std::uint64_t entry{0}, mass{0};
  Point centroid;
  std::vector<Sphere> components;
  InitialState initial;
};
struct ContextOverlay {
  warm::OccurrenceId occurrence;
  LocalRef target;
};
struct LocalPlacement {
  warm::OccurrenceId occurrence;
  InitialState initial;
};
struct LocalGeometry {
  warm::ObjectKey shared_definition;
  Point centroid;
  std::vector<Sphere> components;
};
// Supplied immutable analyst snapshot, not a warm Tool or Bundle. The owner
// attests version/coherence; this interface never opens a private source.
struct LocalInput {
  LocalRef source;
  std::vector<LocalValue> values;
  std::vector<ContextOverlay> context;
  std::vector<LocalPlacement> placements;
  std::vector<LocalGeometry> geometry;
};
struct Origin {
  OccurrenceKey occurrence;
  warm::ObjectKey definition;
  std::optional<warm::ToolRef> shared;
  std::optional<LocalRef> local;
};

// Pure logical hot state. No Program, SNode pointers, manager or SQL handles.
// Even private-derived placements referring only to shared tools live here.
class ActiveAssembly {
 public:
  static ActiveAssembly assemble(warm::Bundle shared, warm::Uuid owner,
                                 bool requires_local,
                                 std::optional<LocalInput> local = {});
  ActiveAssembly revise(std::optional<LocalInput> local) const;
  ActiveAssembly restart() const;
  ActiveAssembly focus(std::vector<FocalField> fields) const;
  bool inputs_available() const {
    return !requires_local_ || local_.has_value();
  }
  const warm::Uuid &study() const { return projection_.study(); }
  const warm::Uuid &generation() const { return generation_; }
  const warm::Uuid &owner() const { return owner_; }
  const warm::Uuid &instance() const { return projection_.recipe().owner; }
  const warm::Bundle &shared_recipe() const { return projection_.recipe(); }
  const std::optional<LocalInput> &local() const { return local_; }
  std::vector<Origin> origins() const;
  StudyInput bind(const std::vector<StudyNode> &nodes) const;

 private:
  ActiveAssembly(LetterProjection projection, warm::Uuid owner, bool required,
                 std::optional<LocalInput> local);
  LetterProjection projection_;
  MembershipFields memberships_;
  warm::Uuid owner_, generation_;
  bool requires_local_;
  std::optional<LocalInput> local_;
  std::vector<FocalField> focal_fields_;
};

struct ExecutionHandle {
  warm::Uuid session, study, generation;
};
struct ActiveReport {
  ExecutionHandle handle;
  warm::Uuid owner, instance;
  std::vector<Origin> origins;
  std::optional<LocalInput> local_provenance;
  PreparedStudy prepared;
  std::vector<float> particles, groups, determined;
  field::FrameAnchor frame_anchor;
  field::Demand demand;
};
// Owns all native bindings. Destruction drains work, destroys studies/kernels
// while their Program is alive, then finalizes it. Logical assemblies/pins are
// outside this owner. No process-failure supervisor is implied.
class ActiveSession {
 public:
  explicit ActiveSession(const engine::InstanceSettings &settings);
  ~ActiveSession();
  ExecutionHandle submit(const ActiveAssembly &assembly,
                         const field::Parameters &parameters,
                         field::HostPath path = field::HostPath::BulkCopy);
  ExecutionHandle refresh(const ExecutionHandle &old,
                          const ActiveAssembly &assembly,
                          const field::Parameters &parameters);
  bool ready(const ExecutionHandle &handle) const;
  void tick(const ExecutionHandle &handle, const field::Parameters &parameters);
  ActiveReport report(const ExecutionHandle &handle, const warm::Uuid &owner);
  void accept(const ActiveReport &report, const warm::Uuid &owner) const;
  field::PhaseTimes timings(const ExecutionHandle &handle) const;
  field::Demand demand(const ExecutionHandle &handle, const warm::Uuid &owner) const;
  double assembly_ms() const { return assembly_ms_; }

 private:
  struct Execution;
  Execution &get(const ExecutionHandle &handle) const;
  warm::Uuid session_{warm::new_uuid()};
  std::unique_ptr<engine::Runtime> runtime_;
  std::map<warm::Uuid, std::unique_ptr<Execution>> executions_;
  double assembly_ms_{0};
};
}  // namespace assembly
