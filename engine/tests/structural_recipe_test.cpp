#include <algorithm>
#include <iostream>
#include <limits>
#include <set>

#include "assembly/membership_fields.h"
#include "assembly/exchange_contract.h"
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
    // Published identities must outlive the source of borrowed expansion refs.
    auto p = [&] {
      auto source = b;
      std::reverse(source.structural_occurrences.begin(), source.structural_occurrences.end());
      return project_letters(source);
    }();
    for (const auto &published : p.occurrences()) {
      const auto selection = std::find_if(b.selections.begin(), b.selections.end(),
          [&](const auto &s) { return s.entry == published.key.source.entry; });
      check(selection != b.selections.end(), "published occurrence has a selection");
      const bool aggregate = published.key.source.path.empty();
      check(published.active == (aggregate ==
                (selection->exposure == warm::Exposure::Aggregate)),
            "source-death publication retains exact aggregate/parts activity");
      const auto &definition = b.tools.at(f.tools.at(published.definition.object));
      const bool complete = aggregate && definition.parents.size() == 1;
      check(bool(published.complete_character) == complete &&
                (!complete || *published.complete_character == definition.parents.front().target),
            "complete character retains pinned reference even for inactive aggregates");
    }
    std::size_t parent_index = 0;
    for (const auto &published : p.occurrences()) {
      if (!published.active) continue;
      const auto &definition = b.tools.at(f.tools.at(published.definition.object));
      for (const auto &expected : definition.parents) {
        check(parent_index < p.parents().size(), "all active parent rows are published");
        const auto &parent = p.parents()[parent_index++];
        check(parent.occurrence == published.key &&
                  parent.field.kind == GroupKind::ParentValue &&
                  parent.field.object == expected.target.object &&
                  parent.ordinal == expected.ordinal && parent.count == expected.contribution,
              "parent publication retains full occurrence/field/ordinal/count after source death");
      }
    }
    check(parent_index == p.parents().size(), "no inactive or duplicate parent rows published");
    std::vector<warm::OccurrenceId> expected_ids;
    for (const auto &row : b.structural_occurrences) expected_ids.push_back(row.id);
    std::sort(expected_ids.begin(), expected_ids.end());
    std::vector<warm::OccurrenceId> published_ids;
    for (const auto &row : p.occurrences()) published_ids.push_back(row.key.source);
    check(published_ids == expected_ids,
          "published occurrence order is by value after shuffled source death");
    for (const auto &published : p.occurrences()) {
      const auto source = std::find_if(b.structural_occurrences.begin(), b.structural_occurrences.end(),
          [&](const auto &row) { return row.id == published.key.source; });
      check(source != b.structural_occurrences.end() &&
                published.definition == f.ref(source->definition) && published.node == source->node,
            "published occurrence retains exact source definition and node after source death");
    }
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
    std::set<std::string> expected_definitions;
    for (const auto &row : b.structural_occurrences)
      expected_definitions.insert(warm::key(row.definition));
    std::vector<std::string> published_definitions;
    for (const auto &definition : input.definitions)
      published_definitions.emplace_back(definition.id);
    check(published_definitions == std::vector<std::string>(
              expected_definitions.begin(), expected_definitions.end()),
          "definitions survive source death with exact value deduplication and encoded order");
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
          warm::structural_from_v1(legacy, legacy.ref,
              {adapted.focal_fields.begin(),adapted.focal_fields.end()});
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
    // Declarations are pinned to one study/occurrence, not just a definition.
    for (const auto entry : {max_entry, std::uint64_t(3), std::uint64_t(7), std::uint64_t(8)}) {
      const bool aggregate = entry == max_entry || entry == 7;
      const auto target = aggregate ? warm::Exposure::Parts : warm::Exposure::Aggregate;
      const OccurrenceKey owner{p.study(), {entry, {}}};
      const auto contract = declare_letter_exchange(p, owner, target);
      auto outgoing = contract.outgoing(), incoming = contract.incoming();
      const auto matches = [&] {
        return contract.matches(outgoing.data(), outgoing.size(), incoming.data(), incoming.size(),
                                contract.match_work());
      };
      check(contract.owner() == owner && contract.to() == target &&
                contract.recipe().revision == b.ref.revision && matches(),
            "exact aggregate/byte exchange declaration");
      check(!contract.matches(outgoing.data(), outgoing.size(), incoming.data(), incoming.size(),
                               contract.match_work() - 1), "comparison budget rejects before matching");
      std::uint64_t out_mass = 0, in_mass = 0, leaf_mass = 0;
      for (const auto &s : outgoing) out_mass += s.mass;
      for (const auto &s : incoming) in_mass += s.mass;
      for (const auto &s : contract.leaves()) leaf_mass += s.count;
      check(out_mass == contract.mass() && in_mass == out_mass && leaf_mass == out_mass,
            "both alternatives retain exact ordered leaf mass");
      if (entry == 8)
        check(outgoing.size() == 4 && outgoing[0].definition == outgoing[1].definition &&
                  !(outgoing[0].occurrence == outgoing[1].occurrence) && contract.leaves().size() == 8 &&
                  contract.leaves()[4].character == 2 && contract.leaves()[4].byte == 0 &&
                  contract.leaves()[6].character == 2 && contract.leaves()[6].byte == 1,
              "repeated bytes retain distinct occurrence paths and contributions");
      std::reverse(outgoing.begin(), outgoing.end());
      std::reverse(incoming.begin(), incoming.end());
      check(matches(), "adapter permutation preserves exchange identity");
      const auto original = outgoing;
      outgoing[0].occurrence.study = warm::new_uuid();
      check(!matches(), "foreign study rejected");
      outgoing = original;
      outgoing[0].occurrence.source.entry = entry == max_entry ? 3 : max_entry;
      check(!matches(), "another copy of the same definition is not outgoing authority");
      outgoing = original;
      outgoing[0].definition.revision = warm::new_uuid();
      check(!matches(), "stale definition revision rejected");
      outgoing = original;
      ++outgoing[0].mass;
      check(!matches(), "changed exact body mass rejected");
      outgoing = original;
      ++outgoing[0].node;
      check(!matches(), "changed scale binding rejected");
      outgoing = original;
      ++outgoing[0].parents[0].contribution;
      check(!matches(), "changed exact source contribution rejected");
      outgoing = original;
      ++outgoing[0].parents[0].ordinal;
      check(!matches(), "changed parent order rejected");
      outgoing = original;
      outgoing[0].parents[0].stored_mass = 999;
      check(!matches(), "changed source mass provenance rejected");
      outgoing = original;
      check(!contract.matches(nullptr, outgoing.size(), incoming.data(), incoming.size(), contract.match_work()) &&
                !contract.matches(outgoing.data(), 0, incoming.data(), incoming.size(), contract.match_work()),
            "missing outgoing binding rejected");
      incoming[0].definition.revision = warm::new_uuid();
      check(!matches(), "undeclared replacement rejected");
      incoming = contract.incoming();
      auto &parts = outgoing.size() > 1 ? outgoing : incoming;
      if (parts.size() > 1) {
        parts[1] = parts[0];
        check(!matches(), "duplicate part cannot substitute for a missing part");
      }
      rejects([&] { declare_letter_exchange(p, owner, contract.from()); }, "same alternative rejected");
      rejects([&] { declare_letter_exchange(p, owner, static_cast<warm::Exposure>(99)); },
              "unknown alternative rejected");
    }
    rejects([&] { declare_letter_exchange(p, {warm::new_uuid(), {3, {}}}, warm::Exposure::Aggregate); },
            "foreign declaration owner rejected");
    rejects([&] { declare_letter_exchange(p, {p.study(), {3, {0, 0}}}, warm::Exposure::Aggregate); },
            "byte path is not an aggregate exchange owner");
    rejects([&] { declare_letter_exchange(p, {p.study(), {99, {}}}, warm::Exposure::Parts); },
            "undeclared occurrence rejected");
    rejects([&] { declare_letter_exchange(old, {old.study(), {4, {}}}, warm::Exposure::Parts); },
            "legacy numerical recipe cannot authorize structural exchange");
    const auto expanded_recipe = normalize_structural_letters(base,
        {{11, f.ref(f.accented), warm::Exposure::Parts},
         {12, f.ref(f.english), warm::Exposure::Aggregate}});
    const auto unicode = project_letters(expanded_recipe);
    const auto unicode_exchange = declare_letter_exchange(unicode, {unicode.study(), {11, {}}},
                                                         warm::Exposure::Aggregate);
    check(unicode_exchange.outgoing().size() == 2 &&
              unicode_exchange.leaves().size() == unicode_exchange.mass(),
          "multibyte UTF-8 alternative retains every ordered nibble contribution");
    check(p.parents().size() == 14 && p.recipe().selections[0].exposure == b.selections[0].exposure &&
              p.recipe().ref.revision == b.ref.revision,
          "declaration and rejection leave projection unchanged");
    std::cout << "PASS: " << checks << " structural recipe checks\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
