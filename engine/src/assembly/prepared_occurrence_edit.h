#pragma once
#include "assembly/letter_projection.h"
#include "assembly/preparation_work.h"
#include "field/prepared_update.h"

namespace assembly {
// Change live presence in this study. A new root occurrence must name a
// declared root template; its exact definition, mass, geometry and field
// participation are reused. No definition or exposure replacement is implied.
// A declared destination always keeps its original template, including after
// its retired instance metadata slot has been reused for another identity.
struct PreparedOccurrenceEdit {
  OccurrenceKey occurrence;
  bool present{};
  std::uint64_t expected_generation{};
  std::uint64_t preparation_work{UINT64_MAX};
  std::uint64_t placement_work{UINT64_MAX}, placement_cells{UINT64_MAX}, hierarchy_work{UINT64_MAX};
  std::optional<OccurrenceKey> clone_from{};
};
struct PreparedOccurrenceEditResult {
  field::UpdateResult publication;
  detail::PreparationResult preparation;
};
struct PreparedLodEdit {
  warm::Exposure exposure{warm::Exposure::Aggregate};
  std::uint64_t expected_generation{};
  std::uint64_t preparation_work{UINT64_MAX}, placement_work{UINT64_MAX},
      placement_cells{UINT64_MAX}, hierarchy_work{UINT64_MAX};
};
}  // namespace assembly
