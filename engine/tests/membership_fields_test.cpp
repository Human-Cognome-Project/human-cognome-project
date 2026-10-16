#include <algorithm>
#include <iostream>
#include <set>

#include "harness.h"
#include "membership_fixture.h"

namespace {
using namespace membership_fixture;
int checks = 0;
void check(bool ok, const char *label) {
  ++checks;
  CHECK_TRUE(ok, label);
}
template <class F>
void rejects(F run, const char *label) {
  bool refused = false;
  try {
    run();
  } catch (const warm::Error &) {
    refused = true;
  }
  check(refused, label);
}
void paths_and_contexts() {
  membership_fixture::Fixture f;
  const auto b = normalize_letters(
      f.base, {f.request(f.english, 0, warm::Exposure::Aggregate),
               f.request(f.english, 1, warm::Exposure::Aggregate)});
  const auto p = project_letters(b);
  const auto fields = membership_fields(p);
  const auto mass = totals(fields);
  check(fields.study() == p.study() && fields.contributions().size() == 6,
        "two uses of A each reach three membership fields");
  check(mass.at(f.en) == 4 && mass.at(f.en_alphabet) == 4 &&
            mass.at(f.family) == 4,
        "direct/alphabet-mediated English and ancestor paths deduplicate per "
        "occurrence");
  std::set<std::pair<std::string, std::string>> unique;
  bool identities = true, naming = true;
  for (const auto &edge : fields.contributions()) {
    unique.emplace(occurrence_text(edge.occurrence),
                   warm::key(edge.field.object));
    identities &= edge.field.kind == assembly::GroupKind::Membership &&
                  edge.target == f.ref(edge.field.object) &&
                  edge.occurrence.study == p.study() && edge.count == 2;
    naming &= !b.tools.at(f.tools.at(edge.target.object)).mass;
  }
  check(unique.size() == 6 && identities,
        "field kinds, full occurrence identities, pinned groups and exact "
        "counts retained");
  check(naming && p.occurrences().size() == 4 && p.parents().size() == 2,
        "NULL naming anchors add metadata/fields without particles or parent "
        "changes");
  bool unchanged = true;
  for (const auto &t : f.base.tools)
    unchanged &= warm::tool_body(t) ==
                 warm::tool_body(p.recipe().tools.at(f.tools.at(t.ref.object)));
  check(
      unchanged,
      "membership traversal preserves every source definition and membership");
  check(
      !mass.count(f.core_class),
      "core byte classification is not the owning language's membership graph");

  const auto mixed = project_letters(normalize_letters(
      f.base, {f.request(f.english, 0, warm::Exposure::Aggregate),
               f.request(f.english, 1, warm::Exposure::Parts),
               f.request(f.french, 2, warm::Exposure::Parts)}));
  const auto contextual = membership_fields(mixed);
  check(
      contextual.contributions().size() == 8,
      "mixed exposure emits memberships only for the three active occurrences");
  const auto active_byte =
      [&](std::uint64_t entry) -> const ProjectedOccurrence & {
    return *std::find_if(
        mixed.occurrences().begin(), mixed.occurrences().end(),
        [&](const auto &o) { return o.active && o.key.source.entry == entry; });
  };
  check(active_byte(1).definition == active_byte(2).definition,
        "English/French byte occurrences use exactly the same pinned byte "
        "definition");
  bool isolated = true, owners = true;
  for (const auto &edge : contextual.contributions()) {
    const bool french = edge.occurrence.source.entry == 2;
    const bool french_group =
        edge.field.object == f.fr || edge.field.object == f.fr_alphabet;
    isolated &= french == french_group && !(edge.field.object == f.core_class);
    if (!edge.occurrence.source.path.empty())
      owners &=
          active_byte(edge.occurrence.source.entry).byte->owner ==
          OccurrenceKey{mixed.study(), {edge.occurrence.source.entry, {}}};
  }
  check(isolated && owners,
        "shared byte definitions retain independent owner-context memberships "
        "without language leakage");
  const auto mixed_mass = totals(contextual);
  check(mixed_mass.at(f.en) == 4 && mixed_mass.at(f.en_alphabet) == 4 &&
            mixed_mass.at(f.fr) == 2,
        "active aggregate and byte uses contribute their own whole mass once "
        "per field");
  for (const auto &o : mixed.occurrences()) {
    if (o.active) continue;
    check(
        std::none_of(
            contextual.contributions().begin(),
            contextual.contributions().end(),
            [&](const auto &edge) { return edge.occurrence == o.key; }),
        "inactive aggregate/byte alternative has no membership participation");
  }
}
void mass_and_repetition() {
  membership_fixture::Fixture f;
  std::map<ObjectKey, std::uint64_t> aggregate_mass;
  for (const auto exposure :
       {warm::Exposure::Aggregate, warm::Exposure::Parts}) {
    const auto p = project_letters(normalize_letters(
        f.base,
        {f.request(f.repeated, 0, exposure), f.request(f.dutch, 1, exposure),
         f.request(f.accented, 2, exposure)}));
    const auto fields = membership_fields(p);
    const auto mass = totals(fields);
    check(
        mass.at(f.nl) == 22 && mass.at(f.nl_alphabet) == 22 && mass.size() == 2,
        "aa-combining-accent, Dutch and multibyte fixtures contribute exact "
        "mass 8+10+4");
    if (exposure == warm::Exposure::Aggregate) {
      aggregate_mass = mass;
      int repeated = 0;
      for (const auto &edge : p.parents())
        if (edge.occurrence.source.entry == 0 && edge.count == 2 &&
            edge.ordinal < 2)
          ++repeated;
      check(
          repeated == 2,
          "membership preparation leaves repeated source a ordinals separate");
    } else {
      check(mass == aggregate_mass && fields.contributions().size() == 22,
            "byte exposure conserves membership mass with one edge per target "
            "and byte position");
      check(std::all_of(fields.contributions().begin(),
                        fields.contributions().end(),
                        [](const auto &edge) {
                          return edge.count == 2 &&
                                 edge.occurrence.source.path.size() == 2;
                        }),
            "each exposed byte uses its mass and its distinct character/byte "
            "path");
      check(p.parents().size() == 22,
            "twenty-two ordered nibble contributions survive contextual "
            "membership preparation");
    }
  }
  letter_fixture::Fixture bare;
  const auto p = project_letters(normalize_letters(
      bare.base, {bare.request(bare.english, 0, warm::Exposure::Aggregate)}));
  check(membership_fields(p).contributions().empty(),
        "absent source ancestry stays absent; no inferred group paths");
}
void graph_failures() {
  membership_fixture::Fixture f;
  const auto b = normalize_letters(
      f.base, {f.request(f.english, 0, warm::Exposure::Aggregate)});
  auto prepare = [](const Bundle &recipe) {
    return membership_fields(project_letters(recipe));
  };
  auto bad = b;
  f.tool(bad, f.en).member_of.push_back({object("02.00.00.00.zz"), new_uuid()});
  rejects([&] { prepare(bad); },
          "missing pinned membership endpoint rejected before handoff");
  bad = b;
  f.tool(bad, f.en_alphabet).member_of[0].revision = new_uuid();
  rejects([&] { prepare(bad); }, "inconsistent membership revision rejected");
  bad = b;
  f.tool(bad, f.en).member_of = {f.ref(f.en_alphabet)};
  rejects([&] { prepare(bad); }, "indirect upward membership cycle rejected");
  bad = b;
  f.tool(bad, f.en).member_of = {f.ref(f.en)};
  rejects([&] { prepare(bad); }, "self membership cycle rejected");
  bad = b;
  f.tool(bad, f.english).member_of.push_back(f.ref(f.en));
  rejects([&] { prepare(bad); },
          "duplicate direct source row rejected; multiple valid paths remain "
          "supported");
}
}  // namespace

int main() {
  try {
    paths_and_contexts();
    mass_and_repetition();
    graph_failures();
  } catch (const std::exception &e) {
    harness::record(false, "unexpected exception", e.what());
  }
  std::cout << checks << " membership value checks\n";
  return harness::report("membership_fields_test");
}
