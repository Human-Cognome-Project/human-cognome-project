#include <limits>
#include <memory>
#include <set>

#include "assembly/study.h"
#include "harness.h"
#include "letter_projection_fixture.h"

namespace {
using namespace letter_fixture;
template <class F>
bool rejected(F run) {
  try {
    run();
  } catch (const warm::Error &) {
    return true;
  } catch (const std::invalid_argument &) {
    return true;
  }
  return false;
}
struct Layout {
  std::unique_ptr<taichi::lang::SNode> root;
  std::vector<assembly::StudyNode> nodes;
  explicit Layout(bool primary) {
    using namespace taichi::lang;
    root = std::make_unique<SNode>(0, SNodeType::root);
    nodes.push_back({root.get(), -1, assembly::ScaleRole::Root});
    if (primary)
      nodes.push_back(
          {&root->dense(Axis(0), 1), 0, assembly::ScaleRole::PrimaryStep});
  }
};
void profiles() {
  Fixture f;
  for (int mode = 0; mode < 3; ++mode) {
    const auto first =
        mode == 1 ? warm::Exposure::Parts : warm::Exposure::Aggregate;
    const auto second =
        mode == 0 ? warm::Exposure::Aggregate : warm::Exposure::Parts;
    auto recipe = normalize_letters(f.base, {f.request(f.english, 0, first),
                                             f.request(f.english, 1, second)});
    if (mode == 2) recipe.contacts = {{{0, {}}, {1, {0, 0}}}};
    warm::validate(recipe);
    const auto projection = project_letters(recipe);
    Layout layout(mode != 0);
    const auto input = projection.bind(layout.nodes);
    const auto prepared = assembly::prepare(input);
    const std::string name = mode == 0   ? "letter-only"
                             : mode == 1 ? "byte-only"
                                         : "mixed";
    CHECK_TRUE(input.capacity == 4 && input.occurrences.size() == 4 &&
                   prepared.particles.size() == 2,
               name +
                   ": reserve all alternatives; publish only the selected "
                   "occurrences");
    CHECK_TRUE(
        prepared.scales[0].units == 1 &&
            (mode == 0 || prepared.scales[1].units == 64),
        name + ": explicit normalized bindings precede native preparation");
    bool values = true, identity = true;
    for (const auto &particle : prepared.particles) {
      const auto &typed = projection.occurrences().at(particle.occurrence.slot);
      const bool letter = typed.key.source.path.empty();
      const auto units = mode != 0 && letter ? 64.0 : 1.0;
      values &= particle.position.x == 2 * units &&
                particle.radius == 0.5 * units &&
                particle.velocity.x == (letter ? 0.25 : -0.5) * units &&
                particle.exact_mass == 2;
      identity &=
          occurrence_text(typed.key) == particle.occurrence.id &&
          warm::key(typed.definition.object) == particle.occurrence.definition;
    }
    CHECK_TRUE(values, name +
                           ": supplied position/radius/velocity use 1 or 64; "
                           "exact mass unchanged");
    CHECK_TRUE(
        identity,
        name +
            ": native values retain full typed occurrence and object identity");
    CHECK_TRUE(input.definitions.size() == 2 &&
                   input.definitions[0].parents.size() == 2 &&
                   input.definitions[1].parents.size() == 1 &&
                   input.definitions[1].parents[0].value ==
                       warm::key(f.ref(object(floor_address('A' + 17))).object),
               name +
                   ": byte nibble parents and aggregate character parents "
                   "survive handoff");
    CHECK_TRUE(
        prepared.contacts.size() == (mode == 2 ? 1u : 0u),
        name + ": contacts reach only explicitly selected live occurrences");
    auto invalid_nodes = layout.nodes;
    invalid_nodes[0].role = assembly::ScaleRole::NamingEquivalent;
    CHECK_TRUE(rejected([&] { projection.bind(invalid_nodes); }),
               name + ": incompatible native binding refused");
    invalid_nodes = layout.nodes;
    invalid_nodes.push_back(layout.nodes[0]);
    CHECK_TRUE(rejected([&] { projection.bind(invalid_nodes); }),
               name + ": layout shape must match normalized recipe");
    if (mode == 2) {
      auto overflow = input;
      overflow.occurrences[0].position.x = std::numeric_limits<float>::max();
      CHECK_TRUE(rejected([&] { assembly::prepare(overflow); }),
                 "scaled coordinate f32 overflow rejected");
      overflow = input;
      overflow.occurrences[0].velocity.z = std::numeric_limits<float>::max();
      CHECK_TRUE(rejected([&] { assembly::prepare(overflow); }),
                 "scaled velocity f32 overflow rejected");
      overflow = input;
      overflow.definitions[1].components[0].radius =
          std::numeric_limits<float>::max();
      CHECK_TRUE(rejected([&] { assembly::prepare(overflow); }),
                 "scaled supplied radius f32 overflow rejected");
    }
  }
}
void contributions() {
  Fixture f;
  const auto b = normalize_letters(
      f.base, {f.request(f.repeated, 0, warm::Exposure::Aggregate),
               f.request(f.repeated, 1, warm::Exposure::Parts),
               f.request(f.dutch, 2, warm::Exposure::Parts),
               f.request(f.french, 3, warm::Exposure::Aggregate)});
  const auto p = project_letters(b);
  Layout layout(true);
  const auto prepared = assembly::prepare(p.bind(layout.nodes));
  CHECK_TRUE(
      prepared.capacity == 18 && prepared.particles.size() == 11,
      "mixed multi-scalar/repeated fixture reserves all distinct occurrences");
  const auto lower_a = warm::key(f.ref(object(floor_address('a' + 17))).object);
  int repeated = 0;
  std::set<int> groups;
  for (const auto &edge : prepared.participation) {
    if (edge.occurrence == occurrence_text({p.study(), {0, {}}}) &&
        edge.parent_value == lower_a) {
      ++repeated;
      groups.insert(edge.group);
      CHECK_TRUE(edge.exact_count == 2 &&
                     (edge.ordinal == 0 || edge.ordinal == 1) &&
                     edge.share == 0.25f,
                 "repeated a contribution retains source ordinal, exact mass "
                 "and whole-particle share");
    }
  }
  CHECK_TRUE(repeated == 2 && groups.size() == 1,
             "ordered repeated parents share one parent-value field");
  std::uint64_t mass = 0;
  for (const auto &particle : prepared.particles) mass += particle.exact_mass;
  CHECK_TRUE(mass == 28,
             "active masses conserve 8+8+10+2 across aggregate and byte views");
  CHECK_TRUE(prepared.participation.size() ==
                 prepared.particles.size() + p.parents().size(),
             "native contributions match typed active parents plus one sibling "
             "edge per live occurrence");
  bool matches = true;
  for (const auto &typed : p.parents()) {
    int count = 0;
    for (const auto &edge : prepared.participation)
      if (edge.occurrence == occurrence_text(typed.occurrence) &&
          edge.parent_value == warm::key(typed.field.object) &&
          edge.ordinal == typed.ordinal && edge.exact_count == typed.count)
        ++count;
    matches &= count == 1;
  }
  CHECK_TRUE(matches,
             "every typed ordinal/contribution reaches native participation "
             "exactly once");
  // Existing native attachments inherit normalized units without another step.
  auto input = p.bind(layout.nodes);
  auto &primary = *layout.root->ch[0];
  auto &secondary = primary.dense(taichi::lang::Axis(1), 1);
  auto &naming = secondary.dense(taichi::lang::Axis(2), 1);
  input.nodes.push_back(
      {&secondary, 1, assembly::ScaleRole::SecondaryCompression});
  input.nodes.push_back({&naming, 2, assembly::ScaleRole::NamingEquivalent});
  input.occurrences[0].study_node = 3;
  const auto attached = assembly::prepare(input);
  CHECK_TRUE(attached.scales[2].units == 64 && attached.scales[3].units == 64 &&
                 attached.particles[0].radius == prepared.particles[0].radius,
             "compression/naming attachments inherit normalized 1:1 units "
             "independent of depth");
}
void languages() {
  Fixture f;
  const auto p = project_letters(normalize_letters(
      f.base, {f.request(f.english, 0, warm::Exposure::Aggregate),
               f.request(f.french, 1, warm::Exposure::Aggregate)}));
  Layout layout(false);
  const auto prepared = assembly::prepare(p.bind(layout.nodes));
  CHECK_TRUE(prepared.particles.size() == 2 && prepared.groups.size() == 3,
             "English/French A produce two sibling fields and one shared core "
             "parent field");
  CHECK_TRUE(
      prepared.particles[0].occurrence.definition !=
          prepared.particles[1].occurrence.definition,
      "English/French A preserve distinct native linguistic definitions");
  int parent_edges = 0;
  for (const auto &edge : prepared.participation)
    if (prepared.groups[edge.group].kind == assembly::GroupKind::ParentValue &&
        edge.exact_count == 2)
      ++parent_edges;
  CHECK_TRUE(parent_edges == 2,
             "each language A contributes its verified character mass to the "
             "shared parent field");
}
}  // namespace

int main() {
  try {
    profiles();
    contributions();
    languages();
  } catch (const std::exception &e) {
    harness::record(false, "unexpected exception", e.what());
  }
  return harness::report("letter_projection_native_test");
}
