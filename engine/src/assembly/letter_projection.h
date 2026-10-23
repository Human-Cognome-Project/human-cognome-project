#pragma once

#include <optional>

#include "assembly/study_input.h"
#include "warm/values.h"

namespace assembly {

struct InitialState {
  warm::Point position, velocity;
};
struct BytePlacement {
  std::vector<std::uint32_t> path;  // character ordinal, byte ordinal
  InitialState initial;
};
struct LetterRequest {
  std::uint64_t entry{0};
  warm::ToolRef letter;
  warm::Exposure exposure{warm::Exposure::Aggregate};
  InitialState aggregate;
  std::vector<BytePlacement> bytes;  // Every alternative, in source order.
};

// Legacy v1 fixture: fill A's normalized recipe from pinned definitions,
// supplied geometry, contacts and explicit placements. Input nodes/occurrences/selections must be
// empty. Metadata/ToolRefs belong to the caller; this allocates no addresses.
warm::Bundle normalize_letters(warm::Bundle definitions,
                               const std::vector<LetterRequest> &requests);
struct StructuralLetterRequest {
  std::uint64_t entry{0};
  warm::ToolRef letter;
  warm::Exposure exposure{warm::Exposure::Aggregate};
};
// Recipe v2. Exact byte alternatives are derived from pinned construction;
// caller supplies intrinsic geometry and explicit focus, never world starts.
warm::Bundle normalize_structural_letters(
    warm::Bundle definitions,
    const std::vector<StructuralLetterRequest> &requests);

struct OccurrenceKey {
  warm::Uuid study;
  warm::OccurrenceId source;
};
bool operator==(const OccurrenceKey &a, const OccurrenceKey &b);
std::string occurrence_text(const OccurrenceKey &key);
struct FieldKey {
  GroupKind kind;
  warm::ObjectKey object;
};
struct ByteProvenance {
  OccurrenceKey owner;
  warm::ToolRef character;
  std::uint32_t character_ordinal{0}, byte_ordinal{0};
};
struct ProjectedOccurrence {
  OccurrenceKey key;
  warm::ToolRef definition;
  int slot{-1};  // Legacy-v1 fixture metadata only; v2 has none.
  int node{0};
  bool active{false};
  std::optional<ByteProvenance> byte;
  // Only a single-scalar aggregate has complete-character equivalence.
  std::optional<warm::ToolRef> complete_character;
};
struct ParentContribution {
  OccurrenceKey occurrence;
  FieldKey field;
  std::uint32_t ordinal{0};
  std::uint64_t count{0};
};

class LetterProjection {
 public:
  const warm::Uuid &study() const { return study_; }
  const warm::Bundle &recipe() const { return recipe_; }
  const std::vector<ProjectedOccurrence> &occurrences() const {
    return occurrences_;
  }
  const std::vector<ParentContribution> &parents() const { return parents_; }
  // Legacy v1 numerical handoff. Caller owns real SNodes; native prepare
  // verifies their structure and converts local geometry/state. No SNode
  // creation, engine staging, DB acquisition or live refresh happens here.
  StudyInput bind(const std::vector<StudyNode> &nodes) const;
  // V2 structural handoff, with no numerical slots/starts. Native admission is
  // a later segment; this does not silently call the legacy numerical prepare.
  StructuralStudyInput bind_structure(
      const std::vector<StudyNode> &nodes) const;

 private:
  LetterProjection() = default;
  friend LetterProjection project_letters(const warm::Bundle &);
  warm::Uuid study_;
  warm::Bundle recipe_;
  std::vector<ProjectedOccurrence> occurrences_;
  std::vector<ParentContribution> parents_;
  StudyInput input_;
  StructuralStudyInput structural_input_;
};

// Validate a versioned normalized recipe, including exact source-form
// UTF-8, and project it into a NEW study. No mutable cross-study identity.
LetterProjection project_letters(const warm::Bundle &recipe);

}  // namespace assembly
