#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <set>

#include "assembly/study.h"
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
  } catch (const std::invalid_argument &) {
    refused = true;
  }
  check(refused, label);
}
struct Layout {
  std::unique_ptr<taichi::lang::SNode> root;
  std::vector<assembly::StudyNode> nodes;
  explicit Layout(bool primary, engine::Runtime *runtime = nullptr) {
    using namespace taichi::lang;
    root = std::make_unique<SNode>(0, SNodeType::root);
    nodes.push_back({root.get(), -1, assembly::ScaleRole::Root});
    auto *leaf_parent = root.get();
    if (primary) {
      leaf_parent = &root->dense(Axis(0), 1);
      nodes.push_back({leaf_parent, 0, assembly::ScaleRole::PrimaryStep});
    }
    if (runtime) {
      auto &leaf = leaf_parent->insert_children(SNodeType::place);
      leaf.dt = PrimitiveType::i32;
      runtime->program().add_snode_tree(std::move(root), false);
    }
  }
};

void native_contract() {
  membership_fixture::Fixture f;
  const auto recipe = normalize_letters(
      f.base, {f.request(f.repeated, 0, warm::Exposure::Aggregate),
               f.request(f.repeated, 1, warm::Exposure::Parts),
               f.request(f.english, 2, warm::Exposure::Aggregate)});
  const auto p = project_letters(recipe);
  const auto fields = membership_fields(p);
  Layout layout(true);
  const auto base = assembly::prepare(p.bind(layout.nodes));
  const auto input = fields.bind(p, layout.nodes);
  const auto prepared = assembly::prepare(input);
  check(prepared.capacity == base.capacity &&
            prepared.particles.size() == base.particles.size() &&
            input.definitions.size() == p.bind(layout.nodes).definitions.size(),
        "NULL naming anchors add no definitions, slots or particles");
  check(prepared.groups.size() == base.groups.size() + 5 &&
            prepared.participation.size() ==
                base.participation.size() + fields.contributions().size(),
        "native handoff adds only reached membership fields and active "
        "membership edges");
  bool exact = true;
  for (const auto &typed : fields.contributions()) {
    int found = 0;
    for (const auto &edge : prepared.participation) {
      const auto &group = prepared.groups[edge.group];
      if (group.kind == assembly::GroupKind::Membership &&
          group.value == warm::key(typed.field.object) &&
          edge.occurrence == occurrence_text(typed.occurrence)) {
        ++found;
        exact &= edge.exact_count == typed.count && edge.share == 1 &&
                 edge.parent_value.empty() &&
                 p.occurrences().at(edge.slot).key == typed.occurrence;
      }
    }
    exact &= found == 1;
  }
  check(exact,
        "typed field/occurrence keys and exact whole-mass counts survive "
        "native handoff once each");
  bool unchanged = true;
  for (std::size_t i = 0; i < base.participation.size(); ++i) {
    const auto &a = base.participation[i], &b = prepared.participation[i];
    unchanged &= a.occurrence == b.occurrence && a.slot == b.slot &&
                 a.group == b.group && a.parent_value == b.parent_value &&
                 a.ordinal == b.ordinal && a.exact_count == b.exact_count &&
                 a.share == b.share;
  }
  check(unchanged,
        "all existing ordered parent and sibling edges remain unchanged");
  const auto a_value = warm::key(f.ref(object(floor_address('a' + 17))).object);
  int repeated = 0;
  std::set<int> repeated_fields;
  for (const auto &edge : prepared.participation)
    if (edge.occurrence == occurrence_text({p.study(), {0, {}}}) &&
        edge.parent_value == a_value && edge.exact_count == 2) {
      ++repeated;
      repeated_fields.insert(edge.group);
    }
  check(repeated == 2 && repeated_fields.size() == 1,
        "repeated a source ordinals still create separate contributions to one "
        "parent field");
  auto duplicate = input;
  duplicate.memberships.push_back(duplicate.memberships[0]);
  check(assembly::prepare(duplicate).participation.size() ==
            prepared.participation.size(),
        "duplicate explicit requests collapse only for the same membership "
        "field/live occurrence");

  auto bad = input;
  bad.memberships[0].occurrence = occurrence_text({p.study(), {1, {}}});
  rejects([&] { assembly::prepare(bad); },
          "membership on an inactive aggregate rejected");
  bad = input;
  bad.memberships[0].occurrence = occurrence_text({p.study(), {0, {0, 0}}});
  rejects([&] { assembly::prepare(bad); },
          "membership on an inactive byte rejected");
  bad = input;
  bad.memberships[0].occurrence = "missing";
  rejects([&] { assembly::prepare(bad); },
          "membership on an unknown occurrence rejected");
  bad = input;
  bad.memberships[0].occurrence.clear();
  rejects([&] { assembly::prepare(bad); },
          "empty membership occurrence rejected");
  bad = input;
  bad.memberships[0].group.clear();
  rejects([&] { assembly::prepare(bad); }, "empty membership field rejected");
  const auto other = project_letters(recipe);
  rejects([&] { fields.bind(other, layout.nodes); },
          "membership fields cannot bind a different study with identical "
          "counts/definitions");

  // A fixture explicitly reaches the byte key through the letter graph.
  // That same opaque value is now a sibling, parent and membership field.
  const auto byte = f.ref(object(floor_address('A' + 17)));
  f.tool(f.base, f.english).member_of.push_back(byte);
  const auto collision = project_letters(normalize_letters(
      f.base, {f.request(f.english, 0, warm::Exposure::Aggregate),
               f.request(f.english, 1, warm::Exposure::Parts)}));
  const auto cp = assembly::prepare(
      membership_fields(collision).bind(collision, layout.nodes));
  std::set<assembly::GroupKind> kinds;
  for (const auto &g : cp.groups)
    if (g.value == warm::key(byte.object)) kinds.insert(g.kind);
  check(kinds.size() == 3,
        "identical object keys cannot collide across sibling, parent-value and "
        "membership field kinds");
}

struct Centroid {
  assembly::Point position;
  double mass{0};
};
std::vector<Centroid> centroids(const assembly::PreparedStudy &study) {
  std::vector<assembly::Point> positions(study.capacity);
  for (const auto &p : study.particles)
    positions[p.occurrence.slot] = p.position;
  std::vector<Centroid> out(study.groups.size());
  for (const auto &edge : study.participation) {
    auto &c = out[edge.group];
    const auto p = positions[edge.slot];
    const double mass = double(edge.exact_count);
    c.mass += mass;
    c.position.x += p.x * mass;
    c.position.y += p.y * mass;
    c.position.z += p.z * mass;
  }
  for (auto &c : out) {
    c.position.x /= c.mass;
    c.position.y /= c.mass;
    c.position.z /= c.mass;
  }
  return out;
}
std::vector<assembly::Point> expected_forces(
    const assembly::PreparedStudy &study, const field::Parameters &parameters,
    bool membership_only) {
  const auto centres = centroids(study);
  std::vector<assembly::Point> positions(study.capacity), force(study.capacity);
  for (const auto &p : study.particles)
    positions[p.occurrence.slot] = p.position;
  for (const auto &edge : study.participation) {
    if (membership_only &&
        study.groups[edge.group].kind != assembly::GroupKind::Membership)
      continue;
    const auto &c = centres[edge.group];
    const auto p = positions[edge.slot];
    const assembly::Point d{c.position.x - p.x, c.position.y - p.y,
                            c.position.z - p.z};
    const double distance = std::hypot(d.x, d.y, d.z);
    if (distance == 0 || c.mass == double(edge.exact_count)) continue;
    const double factor = parameters.gravity * double(edge.exact_count) *
                          c.mass / (distance * distance * distance);
    force[edge.slot].x += factor * d.x;
    force[edge.slot].y += factor * d.y;
    force[edge.slot].z += factor * d.z;
  }
  return force;
}

void cpu_response() {
  engine::InstanceSettings settings;
  settings.cuda = false;
  settings.read_environment = false;
  settings.snode_capacity = 4096;
  settings.snode_tree_capacity = 128;
  settings.cpu_threads = 2;
  engine::Runtime runtime(settings);
  check(!runtime.on_cuda(),
        "membership response executes the existing native CPU backend");
  for (int profile = 0; profile < 3; ++profile) {
    membership_fixture::Fixture f;
    const auto first =
        profile == 1 ? warm::Exposure::Parts : warm::Exposure::Aggregate;
    const auto second =
        profile == 0 ? warm::Exposure::Aggregate : warm::Exposure::Parts;
    auto a = f.request(f.english, 0, first),
         b = f.request(f.english, 1, second),
         c = f.request(f.french, 2, second);
    // Exactly representable centroids keep this check focused on the added
    // field response, including the field kernel's exact coincidence gate.
    a.aggregate = {{-4, 0, 0}, {}};
    a.bytes[0].initial = {{-4, 0, 0}, {}};
    b.aggregate = {{4, 0, 0}, {}};
    b.bytes[0].initial = {{4, 0, 0}, {}};
    c.aggregate = {{12, 0, 0}, {}};
    c.bytes[0].initial = {{12, 0, 0}, {}};
    const auto p = project_letters(normalize_letters(f.base, {a, b, c}));
    const auto fields = membership_fields(p);
    Layout layout(profile != 0, &runtime);
    const auto input = fields.bind(p, layout.nodes);
    field::Parameters parameters;
    parameters.gravity = 0.125f;
    assembly::FixedStudy baseline(runtime, p.bind(layout.nodes), parameters);
    assembly::FixedStudy study(runtime, input, parameters);
    check(
        baseline.ready() && study.ready(),
        "CPU: baseline and contextual fields are ready after centroid seeding");
    study.download();
    baseline.download();
    const auto &prepared = study.prepared();
    const auto expected = centroids(prepared);
    bool seeds = true;
    std::map<std::string, double> membership_mass;
    for (std::size_t i = 0; i < expected.size(); ++i) {
      seeds &= std::abs(study.fields().group(field::kCenM, int(i)) -
                        expected[i].mass) < 1e-5 &&
               std::abs(study.fields().group(field::kCenX, int(i)) -
                        expected[i].position.x) < 1e-5;
      if (prepared.groups[i].kind == assembly::GroupKind::Membership)
        membership_mass.emplace(prepared.groups[i].value,
                                study.fields().group(field::kCenM, int(i)));
    }
    check(seeds,
          "CPU: every native field centroid agrees with exact participating "
          "counts before first force");
    check(membership_mass.at(warm::key(f.en)) == 4 &&
              membership_mass.at(warm::key(f.en_alphabet)) == 4 &&
              membership_mass.at(warm::key(f.family)) == 4 &&
              membership_mass.at(warm::key(f.fr)) == 2 &&
              membership_mass.at(warm::key(f.fr_alphabet)) == 2,
          "CPU: aggregate/parts/mixed profiles conserve each contextual "
          "membership mass");
    bool staged = true;
    for (std::size_t i = 0; i < prepared.participation.size(); ++i) {
      const auto &edge = prepared.participation[i];
      if (prepared.groups[edge.group].kind != assembly::GroupKind::Membership)
        continue;
      staged &=
          study.fields().edge_int_data.at(std::size_t(field::kEdgeParticle) *
                                              prepared.participation.size() +
                                          i) == edge.slot &&
          study.fields().edge_int_data.at(std::size_t(field::kEdgeGroup) *
                                              prepared.participation.size() +
                                          i) == edge.group &&
          study.fields().edge_float_data.at(
              std::size_t(field::kShare) * prepared.participation.size() + i) ==
              1;
    }
    check(staged,
          "CPU: membership fields use existing slot/group/share arrays without "
          "a new kernel");
    const auto all_force = expected_forces(prepared, parameters, false);
    const auto member_force = expected_forces(prepared, parameters, true);
    study.tick(parameters);
    baseline.tick(parameters);
    study.download();
    baseline.download();
    bool force = true, delta = true, active_response = false,
         inactive_zero = true;
    for (const auto &o : p.occurrences()) {
      const int slot = o.slot;
      const double actual = study.fields().particle(field::kForceX, slot);
      const double base = baseline.fields().particle(field::kForceX, slot);
      force &= std::abs(actual - all_force[slot].x) < 1e-6;
      delta &= std::abs((actual - base) - member_force[slot].x) < 1e-6;
      if (!o.active) {
        for (int f = 0; f < field::kParticleFieldCount; ++f)
          inactive_zero &= study.fields().particle(f, slot) == 0;
      } else if (o.key.source.entry < 2) {
        active_response |= std::abs(actual - base) > 1e-7 &&
                           (profile == 0 || o.byte.has_value());
      }
    }
    check(force,
          "CPU: seeded first force matches independent inclusive-centroid "
          "calculation");
    check(delta,
          "CPU: force added by memberships matches the isolated "
          "membership-edge calculation");
    check(active_response,
          "CPU: contextual response acts on selected live letters/byte parts");
    check(inactive_zero,
          "CPU: inactive alternatives remain inert despite shared definitions");
    const auto fr_slot = profile == 0 ? 4 : 5;
    check(std::abs(study.fields().particle(field::kForceX, fr_slot) -
                   baseline.fields().particle(field::kForceX, fr_slot)) < 1e-6,
          "CPU: French's shared byte/character receives no English membership "
          "response");
    bool immutable = true;
    for (const auto &t : f.base.tools)
      immutable &=
          warm::tool_body(t) ==
          warm::tool_body(p.recipe().tools.at(f.tools.at(t.ref.object)));
    check(immutable,
          "CPU: ticking leaves all pinned source memberships and definitions "
          "unchanged");
  }
}
}  // namespace

int main() {
  try {
    native_contract();
    cpu_response();
  } catch (const std::exception &e) {
    harness::record(false, "unexpected exception", e.what());
  }
  std::cout << checks << " native membership checks\n";
  return harness::report("membership_fields_native_test");
}
