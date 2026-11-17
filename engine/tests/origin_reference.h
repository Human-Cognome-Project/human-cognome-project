#pragma once
#include "assembly/active_workbench.h"
#include "letter_projection_fixture.h"

// Test-only old value shape/producer from #353 at 9bf2b956. Seeds are obtained
// outside measurement; this reconstructs the old producer's owning copies and
// vector growth without retaining a legacy definition in production.
namespace origin_reference {
using namespace assembly;
struct LegacyOrigin {
  OccurrenceKey occurrence;
  warm::ObjectKey definition;
  std::optional<warm::ToolRef> shared;
  std::optional<LocalRef> local;
};
inline std::vector<LegacyOrigin> produce(const std::vector<Origin> &seeds) {
  std::vector<LegacyOrigin> out;
  for (const auto &o : seeds)
    out.push_back({o.occurrence,o.definition(),o.shared,o.local});
  return out;
}
// The reference conversion alone accepts an old duplicate key. Refuse a
// mismatch explicitly; there is no production legacy-key fallback API.
inline Origin checked(const LegacyOrigin &old) {
  Origin out{old.occurrence,old.shared,old.local};
  if (!(out.definition() == old.definition))
    throw warm::Error("legacy origin definition differs from provenance pin");
  return out;
}
inline LocalInput local_input(const warm::Uuid &instance, const warm::Uuid &owner) {
  LocalInput out;
  out.source = {letter_fixture::object("z0.00.00.00.01",instance),warm::new_uuid(),owner};
  const LocalRef value{letter_fixture::object("z0.00.00.00.02",instance),warm::new_uuid(),owner};
  out.values.push_back({value,42,3,{},{{{},0.125}},{{40,0,0},{}}});
  return out;
}
inline bool same_local(const std::optional<LocalRef> &a, const std::optional<LocalRef> &b) {
  return a.has_value() == b.has_value() &&
      (!a || (a->object == b->object && a->owner == b->owner && a->version == b->version));
}
}  // namespace origin_reference
