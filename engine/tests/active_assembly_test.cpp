#include "assembly/active_assembly.h"
#include "harness.h"
#include "letter_projection_fixture.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace {
using namespace assembly;

template<class Exception, class F>
void refuses(F &&f, const char *message) {
  try {
    f();
    harness::record(false, message, "request accepted");
  } catch (const Exception &e) {
    harness::record(std::string(e.what()) == message, message, e.what());
  }
}

std::vector<StudyNode> bindings(const warm::Bundle &recipe) {
  // The logical boundary copies opaque tokens; it never dereferences them.
  // Only the native owner may supply executable bindings in production.
  static int token;
  std::vector<StudyNode> nodes;
  for (const auto &node : recipe.nodes)
    nodes.push_back({reinterpret_cast<const taichi::lang::SNode *>(&token),
                     node.parent, static_cast<ScaleRole>(node.role)});
  return nodes;
}

void logical_inputs(letter_fixture::Fixture &f) {
  const auto recipe = normalize_letters(f.base,
      {f.request(f.english, 0, warm::Exposure::Aggregate)});
  const auto nodes = bindings(recipe);
  const auto owner = warm::new_uuid();
  const auto shared = ActiveAssembly::assemble(recipe, owner, false);
  const auto missing = ActiveAssembly::assemble(recipe, owner, true);
  const auto shared_input = shared.bind(nodes);
  CHECK_TRUE(shared.inputs_available() && !missing.inputs_available(),
             "shared availability and missing private input remain distinct");
  refuses<warm::Error>([&] { missing.bind(nodes); },
                      "invalid or incomplete active input");
  refuses<warm::Error>([&] { ActiveAssembly::assemble(recipe, {}, false); },
                      "invalid or incomplete active input");

  LocalInput local;
  const auto version = warm::new_uuid();
  local.source = {letter_fixture::object("y0.00.00.00.00", shared.instance()), version, owner};
  const LocalRef value{letter_fixture::object("02.00.00.00.03", shared.instance()), version, owner};
  local.values.push_back({value, 99, 2, {}, {{{3, 4, 0}, 0.5}}, {{7, 8, 9}, {1, 2, 3}}});
  local.context.push_back({{0, {}}, value});
  local.placements.push_back({{0, {}}, {{-2, 0, 0}, {}}});
  local.geometry.push_back({f.english, {}, {{{}, 0.25}}});
  auto joint = missing.revise(local);
  CHECK_TRUE(joint.inputs_available() && joint.study() == missing.study() &&
                 joint.generation() != missing.generation() && joint.owner() == owner &&
                 joint.instance() == shared.instance(),
             "revision preserves study and owner while replacing logical generation");
  auto bound = joint.bind(nodes);
  const auto english = std::find_if(bound.definitions.begin(), bound.definitions.end(),
      [&](const auto &v) { return std::string(v.id) == warm::key(f.english); });
  CHECK_TRUE(bound.occurrences.size() == shared_input.occurrences.size() + 1 &&
                 bound.definitions.size() == shared_input.definitions.size() + 1 &&
                 bound.occurrences[0].position.x == -2 &&
                 bound.occurrences.back().position.x == 7 &&
                 bound.occurrences.back().velocity.z == 3 &&
                 english != bound.definitions.end() && english->components[0].radius == 0.25 &&
                 bound.definitions.back().components[0].centre.y == 4 &&
                 bound.definitions.back().components[0].radius == 0.5,
             "linked logical binding preserves private starts and geometry overrides");
  const auto origins = joint.origins();
  CHECK_TRUE(origins.size() == shared.origins().size() + 1 &&
                 origins[0].shared && !origins[0].local &&
                 !origins.back().shared && origins.back().local &&
                 origins[0].definition().scope.nil() &&
                 origins.back().definition().scope == joint.instance() &&
                 origins.back().local->version == version &&
                 origins[0].definition().address == origins.back().definition().address,
             "equal addresses retain distinct shared and private provenance");

  const FocalField focus{GroupKind::Membership, InputText(warm::key(value.object))};
  joint = joint.focus({focus});
  const auto restarted = joint.restart();
  CHECK_TRUE(restarted.study() != joint.study() &&
                 restarted.generation() != joint.generation() &&
                 restarted.owner() == joint.owner() &&
                 restarted.instance() == joint.instance() &&
                 restarted.local()->source.version == version &&
                 restarted.bind(nodes).focal_fields.at(0).value == focus.value &&
                 joint.revise(local).bind(nodes).focal_fields.at(0).value == focus.value,
             "restart creates a study while revision and restart retain private focus");
  CHECK_TRUE(shared.bind(nodes).occurrences.size() == shared_input.occurrences.size() &&
                 shared.origins().at(0).definition().scope.nil(),
             "private revision leaves shared source independently usable");

  const auto invalid = [&](auto mutate, const char *message = "invalid or incomplete active input") {
    auto changed = local;
    mutate(changed);
    refuses<warm::Error>([&] { missing.revise(changed); }, message);
  };
  invalid([](auto &v) { v.source.owner = warm::new_uuid(); });
  invalid([](auto &v) { v.source.object.scope = warm::new_uuid(); });
  invalid([](auto &v) { v.source.version = {}; });
  invalid([](auto &v) { v.source.object.address[0].partial = true; },
          "invalid, empty or wildcard object address");
  invalid([](auto &v) { v.context[0].target.version = warm::new_uuid(); });
  invalid([](auto &v) { v.values[0].entry = 0; });
  invalid([](auto &v) { v.values[0].mass = 0; });

  const auto geometry = [&](auto mutate, const char *message) {
    auto changed = local;
    mutate(changed.values[0]);
    refuses<std::invalid_argument>([&] { missing.revise(changed); }, message);
  };
  geometry([](auto &v) { v.centroid.x = std::numeric_limits<double>::quiet_NaN(); },
           "assembly: non-finite coordinates");
  geometry([](auto &v) { v.components.clear(); },
           "assembly: supplied component geometry required");
  geometry([](auto &v) { v.components[0].centre.y = std::numeric_limits<double>::infinity(); },
           "assembly: non-finite coordinates");
  geometry([](auto &v) { v.components[0].radius = -1; },
           "assembly: invalid component radius");
  geometry([](auto &v) { v.components[0].radius = std::numeric_limits<double>::infinity(); },
           "assembly: invalid component radius");
  geometry([](auto &v) {
    v.centroid.x = -std::numeric_limits<double>::max();
    v.components[0].centre.x = std::numeric_limits<double>::max();
  }, "assembly: enclosure overflow");
}

void structural_inputs(letter_fixture::Fixture &f) {
  auto base = f.base;
  base.focal_fields = {{warm::FieldKind::ParentValue,
      f.base.tools.at(f.tools.at(f.english)).parents.at(0).target.object}};
  const auto recipe = normalize_structural_letters(base,
      {{0, f.ref(f.english), warm::Exposure::Aggregate},
       {1, f.ref(f.repeated), warm::Exposure::Parts}});
  const auto nodes = bindings(recipe);
  const auto source = ActiveAssembly::assemble(recipe, warm::new_uuid(), false);
  const auto input = source.bind_structure(nodes);
  // Both exposure alternatives remain: two English and five repeated rows.
  CHECK_TRUE(input.occurrences.size() == 7 && input.selections.size() == 2 &&
                 input.selections[0].exposure == Exposure::Aggregate &&
                 input.selections[1].exposure == Exposure::Parts &&
                 input.selections[1].parts.size() == 4 && input.focal_fields.size() == 1 &&
                 input.focal_fields[0].kind == GroupKind::ParentValue,
             "standalone v2 binding retains repeated parts and inherited focus");
  CHECK_TRUE(std::all_of(input.occurrences.begin(), input.occurrences.end(),
                 [&](const auto &v) { return std::string(v.id).rfind(source.study().str() + "/", 0) == 0; }),
             "structural occurrence identities remain in the active study");
  const auto cleared = source.focus({});
  CHECK_TRUE(cleared.revise({}).bind_structure(nodes).focal_fields.empty() &&
                 cleared.restart().bind_structure(nodes).focal_fields.empty() &&
                 source.bind_structure(nodes).focal_fields.size() == 1,
             "explicit empty focus survives revision and restart without changing source");
  const FocalField focus{GroupKind::SiblingDefinition, InputText(warm::key(f.english))};
  const auto replaced = source.focus({focus}).bind_structure(nodes);
  CHECK_TRUE(replaced.focal_fields.size() == 1 && replaced.focal_fields[0].value == focus.value &&
                 replaced.focal_fields[0].kind == focus.kind,
             "explicit structural focus replaces inherited focus");
  const auto selected=source.focus({focus});
  const auto all_parts=selected.expose_all(warm::Exposure::Parts);
  const auto all_aggregate=all_parts.expose_all(warm::Exposure::Aggregate);
  const auto parts=all_parts.bind_structure(nodes), aggregate=all_aggregate.bind_structure(nodes);
  CHECK_TRUE(all_parts.study()==selected.study() && all_parts.owner()==selected.owner() &&
                 all_parts.generation()!=selected.generation() &&
                 &all_parts.shared_recipe()==&selected.shared_recipe() &&
                 &all_aggregate.shared_recipe()==&selected.shared_recipe() &&
                 all_aggregate.generation()!=all_parts.generation(),
             "explicit exposure views share immutable recipe and logical owner with fresh generations");
  CHECK_TRUE(parts.selections.size()==2 && aggregate.selections.size()==2 &&
                 all_parts.exposure(0)==warm::Exposure::Parts && all_parts.exposure(1)==warm::Exposure::Parts &&
                 all_aggregate.exposure(0)==warm::Exposure::Aggregate && all_aggregate.exposure(1)==warm::Exposure::Aggregate &&
                 selected.exposure(0)==warm::Exposure::Aggregate && selected.exposure(1)==warm::Exposure::Parts &&
                 parts.focal_fields.at(0).value==focus.value && aggregate.focal_fields.at(0).value==focus.value,
             "whole-source exposure views preserve explicit focus and leave original selection unchanged");
  CHECK_TRUE(cleared.expose_all(warm::Exposure::Parts).bind_structure(nodes).focal_fields.empty(),
             "explicit empty focus survives exposure view selection");
  refuses<warm::Error>([&] { source.bind(nodes); }, "legacy numerical binding requires recipe v1");
  refuses<warm::Error>([&] { source.bind_structure({}); }, "native layout differs from normalized recipe");
  refuses<warm::Error>([&] {
    ActiveAssembly::assemble(recipe, source.owner(), true).bind_structure(nodes);
  }, "invalid or incomplete active input");
  LocalInput local;
  local.source = {letter_fixture::object("y0.00.00.00.00", source.instance()),
                  warm::new_uuid(), source.owner()};
  refuses<warm::Error>([&] { source.revise(local).bind_structure(nodes); },
                      "structural active binding does not yet support local overlays");
}
}  // namespace

int main() {
  try {
    letter_fixture::Fixture f;
    logical_inputs(f);
    structural_inputs(f);
  } catch (const std::exception &e) {
    harness::record(false, "unexpected exception", e.what());
  }
  return harness::report("active_assembly_test");
}
