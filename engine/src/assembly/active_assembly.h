#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "assembly/membership_fields.h"

namespace assembly {
namespace testing { struct ActiveAssemblyAccess; }
namespace detail { class StructuralAdmissionPlan; }
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
  std::optional<warm::ToolRef> shared;
  std::optional<LocalRef> local;
  // Exactly one complete pin owns the definition. The reference is borrowed
  // from this value, not the source assembly or native execution.
  const warm::ObjectKey &definition() const;
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
  // Coordinate-free v2 handoff for native prepared admission. No live slots,
  // starts or native execution are created. Local overlays are not supported yet.
  StructuralStudyInput bind_structure(const std::vector<StudyNode> &nodes) const;

 private:
  friend class detail::StructuralAdmissionPlan;
  friend struct testing::ActiveAssemblyAccess;
  ActiveAssembly(LetterProjection projection, warm::Uuid owner, bool required,
                 std::optional<LocalInput> local);
  LetterProjection projection_;
  MembershipFields memberships_;
  warm::Uuid owner_, generation_;
  bool requires_local_;
  std::optional<LocalInput> local_;
  std::vector<FocalField> focal_fields_;
  bool focus_override_{false};
};
}  // namespace assembly
