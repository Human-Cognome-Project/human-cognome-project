#pragma once

#include <optional>
#include <memory>

#include "assembly/study_input.h"
#include "assembly/reservation_allocator.h"
#include "warm/values.h"

namespace assembly {
namespace detail { class ExchangeOwner; class PreparationWork; class PreparedLodWorkspace; struct ProjectionAccess; struct ProjectionOccurrence; }
namespace testing { struct ExchangeOwnerAccess; struct ProjectionIndicesAccess; }

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
std::string occurrence_text(const warm::Uuid &study, const warm::OccurrenceId &source);
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

namespace detail {
// The projection owns the study namespace for both active and inactive pools.
// Public extraction restores a complete independently owning contribution.
struct ProjectionParentContribution {
  warm::OccurrenceId source;
  std::uint32_t occurrence_index{}, parent_index{};
  GroupKind kind{GroupKind::ParentValue};
  std::uint32_t ordinal{};
  std::uint64_t count{};
};
// Actor provenance retains an independent aggregate selector in this view.
// Character pins/ordinals remain owning values; public exports restore the key.
struct ProjectionByteProvenance {
  std::uint32_t owner_index{};
  warm::ToolRef character;
  std::uint32_t character_ordinal{}, byte_ordinal{};
};
// Indices are meaningful only in this projection's retained immutable recipe.
// The projection owns the study namespace; entry/full path independently check
// the indexed source. Contextual readers accept that projection and an index.
// Actor exposure and provenance stay in the view; public values own full keys/pins.
struct ProjectionOccurrence {
  warm::OccurrenceId source;
  std::uint32_t definition_index{}, source_index{};
  int slot{-1}, node{};
  bool active{};
  std::optional<ProjectionByteProvenance> byte;
  std::optional<warm::ToolRef> complete_character;
};
}

// Retained host capacity charge, not allocator/RSS/driver accounting. Text
// capacity includes its terminator and conservatively counts inline strings.
struct ProjectionStorage {
  std::uint64_t object_bytes{}, vector_bytes{}, text_bytes{}, parent_slots{};
  std::uint64_t charged_bytes() const;
  void add(const ProjectionStorage &);
};

class LetterProjection {
 public:
  // Value copies own independent recipes. Assignment preserves the destination
  // domain; moves transfer it. A moved-from value can be assigned or destroyed.
  LetterProjection(const LetterProjection &);
  LetterProjection &operator=(const LetterProjection &);
  LetterProjection(LetterProjection &&) noexcept = default;
  LetterProjection &operator=(LetterProjection &&) noexcept;
  const warm::Uuid &study() const { return study_; }
  const warm::Bundle &recipe() const {
    if (!recipe_) throw warm::Error("moved-from projection has no recipe");
    return *recipe_;
  }
  // Current owner-local exposure; recipe() remains the immutable initial recipe.
  warm::Exposure exposure(std::uint64_t entry) const;
  std::size_t occurrence_count() const { return occurrences_.size(); }
  // Detached owning value, valid after projection/recipe/domain destruction.
  ProjectedOccurrence occurrence(std::size_t index) const;
  // Active contributions only; every extracted value owns its complete key.
  // The inactive pool remains private and both pools reuse their reserved rows.
  std::size_t parent_count() const { return parents_.size(); }
  ParentContribution parent(std::size_t index) const;
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
  LetterProjection(const LetterProjection &, const std::shared_ptr<detail::ReservationDomain> &);
  // Explicit actor exposure views and the private owner spare can share the
  // immutable recipe; ordinary value copies remain independently owning.
  struct OwnerSpare {};
  LetterProjection(const LetterProjection &, const std::shared_ptr<detail::ReservationDomain> &, OwnerSpare);
  LetterProjection(const LetterProjection &, const std::shared_ptr<detail::ReservationDomain> &,
                   std::shared_ptr<const warm::Bundle>);
  friend LetterProjection project_letters(const warm::Bundle &);
  friend class detail::ExchangeOwner;
  friend class ActiveAssembly;
  friend class detail::PreparedLodWorkspace;
  friend struct detail::ProjectionAccess;
  friend struct testing::ExchangeOwnerAccess;
  friend struct testing::ProjectionIndicesAccess;
  void reserve_exposure(detail::PreparationWork &);
  void prepare_exposure(LetterProjection &, const OccurrenceKey &, warm::Exposure,
                        detail::PreparationWork &) const;
  ProjectionStorage storage() const;
  ProjectionStorage storage(detail::PreparationWork *, bool logical, bool include_recipe = true) const;
  const warm::Tool *resolve_definition(std::size_t, detail::PreparationWork *) const;
  const warm::Tool *resolve_source(std::size_t, detail::PreparationWork *) const;
  const warm::Parent *resolve_parent(bool, std::size_t, detail::PreparationWork *) const;
  // Immutable association only: candidate pools may be partially repartitioned
  // after a work refusal. Steady-state readers separately require pool role.
  const warm::Parent *resolve_parent_source(bool, std::size_t, detail::PreparationWork *) const;
  warm::Uuid study_;
  std::shared_ptr<const warm::Bundle> recipe_;
  detail::SharedReservedVector<warm::Exposure> exposures_;
  detail::SharedReservedVector<detail::ProjectionOccurrence> occurrences_;
  detail::SharedReservedVector<detail::ProjectionParentContribution> parents_;
  // Inactive rows retain independent source paths and correlated selectors.
  detail::SharedReservedVector<detail::ProjectionParentContribution> spare_parents_;
  bool exposure_reserved_{};
  StudyInput input_;
  StructuralStudyInput structural_input_;
};

// Validate a versioned normalized recipe, including exact source-form
// UTF-8, and project it into a NEW study. No mutable cross-study identity.
LetterProjection project_letters(const warm::Bundle &recipe);

}  // namespace assembly
