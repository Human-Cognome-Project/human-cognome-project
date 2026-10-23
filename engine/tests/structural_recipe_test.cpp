#include <algorithm>
#include <iostream>
#include <limits>

#include "assembly/membership_fields.h"
#include "letter_projection_fixture.h"

namespace {
using namespace letter_fixture;
int checks = 0;
void check(bool ok, const char *label) {
  if (!ok)
    throw warm::Error(label);
  ++checks;
}
template <class F>
void rejects(F run, const char *label) {
  try {
    run();
  } catch (const warm::Error &) {
    ++checks;
    return;
  }
  throw warm::Error(label);
}
}  // namespace

int main() {
  try {
    Fixture f;
    auto base = f.base;
    base.focal_fields = {
        {warm::FieldKind::ParentValue, f.ref(f.english).object}};
    // Select a real positive parent field, not an arbitrary definition name.
    base.focal_fields[0].object =
        f.base.tools.at(f.tools.at(f.english)).parents[0].target.object;
    const auto max_entry = std::numeric_limits<std::uint64_t>::max();
    auto b = normalize_structural_letters(
        base, {{max_entry, f.ref(f.english), warm::Exposure::Aggregate},
               {3, f.ref(f.english), warm::Exposure::Parts},
               {7, f.ref(f.dutch), warm::Exposure::Aggregate},
               {8, f.ref(f.repeated), warm::Exposure::Parts}});
    check(b.recipe_version == 2 && b.occurrences.empty() &&
              b.structural_occurrences.size() == 15,
          "coordinate-free aggregate and all exact byte alternatives");
    auto p = project_letters(b);
    check(std::all_of(p.occurrences().begin(), p.occurrences().end(),
                      [](const auto &o) { return o.slot == -1; }),
          "structural projection assigns no numerical slots");
    check(p.parents().size() == 14,
          "ordered parent contribution rows preserved");
    std::vector<StudyNode> nodes;
    for (const auto &n : b.nodes)
      nodes.push_back({reinterpret_cast<const taichi::lang::SNode *>(1),
                       n.parent, static_cast<assembly::ScaleRole>(n.role)});
    auto input = membership_fields(p).bind_structure(p, nodes);
    check(input.occurrences.size() == 15 && input.selections.size() == 4 &&
              input.focal_fields.size() == 1 && input.nodes.size() == 2,
          "structural handoff retains definitions, selection, focus and roles");
    rejects([&] { p.bind(nodes); }, "v2 cannot enter legacy numerical binding");
    auto aliased = b;
    aliased.nodes.push_back({0, warm::ScaleRole::SecondaryCompression});
    aliased.nodes.push_back({2, warm::ScaleRole::NamingEquivalent});
    for (auto &o : aliased.structural_occurrences)
      if (!o.id.path.empty())
        o.node = 3;
    warm::validate(aliased);
    auto alias_nodes = nodes;
    alias_nodes.push_back(
        {nodes[0].layout, 0, assembly::ScaleRole::SecondaryCompression});
    alias_nodes.push_back(
        {nodes[0].layout, 2, assembly::ScaleRole::NamingEquivalent});
    check(
        project_letters(aliased).bind_structure(alias_nodes).nodes.size() == 4,
        "secondary compression and naming preserve conceptual depth at 1:1");
    auto malformed = b;
    malformed.occurrences.push_back({{99, {}}, f.english, 0, {}, {}});
    rejects([&] { warm::validate(malformed); },
            "legacy starts in v2 must reject");
    malformed = b;
    malformed.structural_occurrences.pop_back();
    rejects([&] { warm::validate(malformed); },
            "missing inactive alternative must reject");
    malformed = b;
    malformed.focal_fields.clear();
    rejects([&] { warm::validate(malformed); },
            "missing explicit focus must reject");
    malformed = b;
    malformed.focal_fields.push_back(malformed.focal_fields[0]);
    rejects([&] { warm::validate(malformed); }, "duplicate focus must reject");
    malformed = b;
    malformed.focal_fields = {{warm::FieldKind::SiblingDefinition, f.accented}};
    rejects([&] { warm::validate(malformed); }, "unused focus must reject");
    malformed = b;
    malformed.focal_fields = {{static_cast<warm::FieldKind>(99), f.english}};
    rejects([&] { warm::validate(malformed); },
            "unknown focus kind must reject");
    malformed = b;
    malformed.structural_occurrences[0].node = 99;
    rejects([&] { warm::validate(malformed); },
            "unsupported role binding must reject");
    auto legacy = normalize_letters(
        f.base, {f.request(f.english, 4, warm::Exposure::Aggregate)});
    const auto original = legacy.occurrences[0].position;
    auto adapted = warm::structural_from_v1(
        legacy, {warm::new_uuid(), warm::new_uuid()},
        {{warm::FieldKind::SiblingDefinition, f.english}});
    check(adapted.occurrences.empty() &&
              adapted.structural_occurrences.size() == 2 &&
              legacy.recipe_version == 1 &&
              legacy.occurrences[0].position.x == original.x,
          "explicit legacy adapter leaves original starts unchanged");
    rejects(
        [&] {
          warm::structural_from_v1(legacy, legacy.ref, adapted.focal_fields);
        },
        "adapter must not overwrite an immutable publication identity");
    auto old = project_letters(legacy);
    rejects([&] { old.bind_structure({nodes[0]}); },
            "legacy requires explicit structural adapter");
    auto f2 = f;
    // Group contributions remain distinct per live occurrence, including bytes
    // inheriting their owning letter's groups.
    const auto group = f2.add(object("02.00.00.00.00"), "English", 0);
    f2.base.tools.at(f2.tools.at(f2.english)).member_of = {group};
    f2.base.focal_fields = {{warm::FieldKind::Membership, group.object}};
    auto grouped = normalize_structural_letters(
        f2.base, {{0, f2.ref(f2.english), warm::Exposure::Aggregate},
                  {1, f2.ref(f2.english), warm::Exposure::Parts}});
    auto gp = project_letters(grouped);
    check(membership_fields(gp).contributions().size() == 2,
          "membership retains distinct uses and byte owner inheritance");
    check(
        membership_fields(gp).bind_structure(gp, nodes).memberships.size() == 2,
        "structural membership handoff complete");
    std::cout << "PASS: " << checks << " structural recipe checks\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
