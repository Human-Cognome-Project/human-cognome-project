#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

#include "assembly/study.h"
#include "harness.h"
#include "taichi/platform/cuda/detect_cuda.h"

namespace {
using namespace assembly;

engine::InstanceSettings settings(bool cuda) {
  engine::InstanceSettings value;
  value.cuda = cuda;
  value.read_environment = false;
  value.snode_capacity = 4096;
  value.snode_tree_capacity = 128;
  value.cpu_threads = 2;
  return value;
}

// Real native study schema. Scale roles are explicit host bindings, not a
// rule inferred from dense axes, structural depth or SNode identifier.
std::vector<StudyNode> layout(engine::Runtime &runtime) {
  using namespace taichi::lang;
  auto root = std::make_unique<SNode>(0, SNodeType::root);
  auto &primary = root->dense(Axis(0), 1);
  auto &secondary = primary.dense(Axis(1), 1);
  auto &nested = secondary.dense(Axis(2), 1);
  auto &equivalent = nested.dense(Axis(3), 1);
  auto &leaf = equivalent.insert_children(SNodeType::place);
  leaf.dt = PrimitiveType::i32;
  std::vector<StudyNode> nodes{{root.get(), -1, ScaleRole::Root},
      {&primary, 0, ScaleRole::PrimaryStep},
      {&secondary, 1, ScaleRole::SecondaryCompression},
      {&nested, 2, ScaleRole::SecondaryCompression},
      {&equivalent, 3, ScaleRole::NamingEquivalent}};
  runtime.program().add_snode_tree(std::move(root), false);
  return nodes;
}

StudyInput couplets(const std::vector<StudyNode> &nodes) {
  StudyInput input;
  input.capacity = 6;
  input.nodes = nodes;
  const std::vector<Sphere> geometry{{{-0.25, 0, 0}, 0.4}, {{0.25, 0, 0}, 0.4}};
  input.definitions = {{"01", 2, {}, geometry, {{"0", 0, 1}, {"1", 1, 1}}},
                       {"10", 2, {}, geometry, {{"1", 0, 1}, {"0", 1, 1}}},
                       {"11", 2, {}, geometry, {{"1", 0, 1}, {"1", 1, 1}}}};
  input.occurrences = {{"01-a", "01", 0, 1, {-3, 0, 0}, {}},
                       {"10-a", "10", 1, 2, {3, 0, 0}, {}},
                       {"11-a", "11", 2, 3, {-1, 3, 0}, {}},
                       {"11-b", "11", 3, 4, {1, -3, 0}, {}}};
  for (int a = 0; a < 4; ++a) {
    for (int b = a + 1; b < 4; ++b) {
      input.contacts.push_back({input.occurrences[a].id, input.occurrences[b].id});
    }
  }
  return input;
}

int parent_group(const PreparedStudy &study, const std::string &value) {
  for (std::size_t g = 0; g < study.groups.size(); ++g) {
    if (study.groups[g].kind == GroupKind::ParentValue && study.groups[g].value == value)
      return int(g);
  }
  throw std::logic_error("fixture parent field missing");
}

struct Centroid { Point position; double mass{0}; };
std::vector<Centroid> centroids(const PreparedStudy &study) {
  std::vector<Point> positions(study.capacity);
  std::vector<double> masses(study.capacity);
  for (const auto &particle : study.particles) {
    positions[particle.occurrence.slot] = particle.position;
    masses[particle.occurrence.slot] = particle.mass;
  }
  std::vector<Centroid> groups(study.groups.size());
  for (const auto &edge : study.participation) {
    auto &group = groups[edge.group];
    const double mass = masses[edge.slot] * edge.share;
    group.mass += mass;
    group.position.x += positions[edge.slot].x * mass;
    group.position.y += positions[edge.slot].y * mass;
    group.position.z += positions[edge.slot].z * mass;
  }
  for (auto &group : groups) {
    group.position.x /= group.mass;
    group.position.y /= group.mass;
    group.position.z /= group.mass;
  }
  return groups;
}

void check_first_force(FixedStudy &study, const field::Parameters &parameters,
                       const std::string &label) {
  const auto &target = study.prepared();
  const auto expected_groups = centroids(target);
  study.download();
  const auto &fields = study.fields();
  double centroid_error = 0;
  for (std::size_t g = 0; g < expected_groups.size(); ++g) {
    const auto &expected = expected_groups[g];
    centroid_error = std::max({centroid_error,
        std::abs(fields.group(field::kCenX, int(g)) - expected.position.x),
        std::abs(fields.group(field::kCenY, int(g)) - expected.position.y),
        std::abs(fields.group(field::kCenM, int(g)) - expected.mass)});
  }
  harness::record(centroid_error < 1e-5, label + ": centroids seeded from target before first force", "");
  std::vector<Point> expected_force(target.capacity), positions(target.capacity);
  std::vector<double> masses(target.capacity);
  for (const auto &particle : target.particles) {
    positions[particle.occurrence.slot] = particle.position;
    masses[particle.occurrence.slot] = particle.mass;
  }
  for (const auto &edge : target.participation) {
    const auto &group = expected_groups[edge.group];
    const auto &position = positions[edge.slot];
    const Point delta{group.position.x - position.x, group.position.y - position.y,
                      group.position.z - position.z};
    const double d = std::hypot(delta.x, delta.y, delta.z);
    const double mass = masses[edge.slot] * edge.share;
    if (d == 0 || mass == group.mass) continue;
    const double factor = parameters.gravity * mass * group.mass / (d * d * d);
    expected_force[edge.slot].x += factor * delta.x;
    expected_force[edge.slot].y += factor * delta.y;
    expected_force[edge.slot].z += factor * delta.z;
  }
  // Declared fixture separations all exceed the supplied summed radii.
  study.tick(parameters);
  study.download();
  double error = 0;
  for (int slot = 0; slot < target.capacity; ++slot) {
    error = std::max({error,
        std::abs(fields.particle(field::kForceX, slot) - expected_force[slot].x),
        std::abs(fields.particle(field::kForceY, slot) - expected_force[slot].y),
        std::abs(fields.particle(field::kForceZ, slot) - expected_force[slot].z)});
  }
  harness::record(error < 1e-7, label + ": first force agrees with independent inclusive-centroid calculation",
                  std::to_string(error));
}

void check_study(bool cuda, const std::string &arch) {
  engine::Runtime runtime(settings(cuda));
  const int before = runtime.program().get_snode_tree_size();
  const auto nodes = layout(runtime);
  CHECK_EQ_INT(runtime.program().get_snode_tree_size(), before + 1, arch + ": native study SNode tree registered");
  auto input = couplets(nodes);
  field::Parameters parameters;
  parameters.contact_stiffness = 40;
  FixedStudy study(runtime, input, parameters);
  CHECK_TRUE(study.ready(), arch + ": initial study ready after publication");
  const auto &prepared = study.prepared();
  CHECK_EQ_INT(prepared.groups.size(), 5, arch + ": parent-value and sibling fields remain distinct");
  int one_occurrences = 0;
  bool saw_zero = false, saw_one = false;
  for (const auto &edge : prepared.participation) {
    if (edge.parent_value == "1") {
      ++one_occurrences;
      saw_zero |= edge.ordinal == 0; saw_one |= edge.ordinal == 1;
    }
  }
  CHECK_TRUE(one_occurrences == 6 && saw_zero && saw_one,
             arch + ": parent value spans ordinals and preserves repeated 11 contributions");
  study.download();
  CHECK_TRUE(study.fields().group(field::kCenM, parent_group(prepared, "0")) == 2 &&
                 study.fields().group(field::kCenM, parent_group(prepared, "1")) == 6,
             arch + ": exact parent counts reach numerical participating mass");
  check_first_force(study, parameters, arch + " initial");
  CHECK_TRUE(study.fields().particle(field::kPosX, 2) != study.fields().particle(field::kPosX, 3) &&
                 study.fields().particle(field::kForceY, 2) != study.fields().particle(field::kForceY, 3),
             arch + ": repeated definitions have independent mutable positions and force");
  bool all_finite = true;
  for (int step = 0; step < 400; ++step) {
    study.tick(parameters);
    if (step % 50 == 0 || step == 399) {
      study.download();
      all_finite &= std::all_of(study.fields().particle_data.begin(), study.fields().particle_data.end(),
                               [](float value) { return std::isfinite(value); });
    }
  }
  double farthest = 0, minimum_gap = std::numeric_limits<double>::infinity(), movement = 0;
  for (const auto &particle : prepared.particles) {
    const int slot = particle.occurrence.slot;
    const auto &f = study.fields();
    farthest = std::max<double>(farthest, std::hypot(f.particle(field::kPosX, slot), f.particle(field::kPosY, slot)));
    movement = std::max(movement, std::hypot(f.particle(field::kPosX, slot) - particle.position.x,
                                            f.particle(field::kPosY, slot) - particle.position.y));
  }
  for (const auto &pair : prepared.contacts) {
    const auto &f = study.fields();
    minimum_gap = std::min<double>(minimum_gap, std::hypot(f.particle(field::kPosX, pair.a) - f.particle(field::kPosX, pair.b),
                                                  f.particle(field::kPosY, pair.a) - f.particle(field::kPosY, pair.b)));
  }
  harness::note(arch + ": fixed study 401 ticks; farthest=" + std::to_string(farthest) +
                 " min separation=" + std::to_string(minimum_gap) + " max movement=" + std::to_string(movement));
  CHECK_TRUE(all_finite && farthest < 300 && movement > 0,
             arch + ": prepared objects interact and remain finite within fixture bound");
  bool free_zero = true;
  for (int slot : {4, 5}) {
    for (int f = 0; f < field::kParticleFieldCount; ++f) free_zero &= study.fields().particle(f, slot) == 0;
    free_zero &= study.fields().determined()[slot] == 0;
  }
  CHECK_TRUE(free_zero, arch + ": unreferenced free slots remain finite and inert");

  // Supported equal-count relationship refresh. No old centroid may be read.
  input.definitions[1].parents = {{"0", 0, 1}, {"0", 1, 1}};
  input.occurrences[1].position = {5, -1, 0};
  study.publish(input, parameters);
  CHECK_TRUE(study.ready() && study.error().empty(), arch + ": relationship refresh publishes successfully");
  check_first_force(study, parameters, arch + " refreshed");

  auto bad = input;
  bad.definitions[0].components[0].radius = -1;
  bool failed = false;
  try { study.publish(bad, parameters); } catch (const std::invalid_argument &) { failed = true; }
  CHECK_TRUE(failed && !study.ready() && !study.error().empty(), arch + ": malformed publication reports error and is non-runnable");
  bool tick_refused = false;
  try { study.tick(parameters); } catch (const std::logic_error &) { tick_refused = true; }
  CHECK_TRUE(tick_refused, arch + ": failed publication refuses tick");
  study.publish(input, parameters);
  CHECK_TRUE(study.ready(), arch + ": explicit valid republication restores readiness");
  bad = input; bad.capacity = 7;
  failed = false;
  try { study.publish(bad, parameters); } catch (const std::invalid_argument &) { failed = true; }
  CHECK_TRUE(failed && !study.ready(), arch + ": fixed-study count change refused without implicit recomposition");
}

void check_exposure_and_empty(bool cuda, const std::string &arch) {
  engine::Runtime runtime(settings(cuda));
  StudyInput input;
  input.capacity = 3; input.nodes = layout(runtime);
  input.definitions = {{"pair", 2, {}, {{{-0.5, 0, 0}, 0.25}, {{0.5, 0, 0}, 0.25}}, {}},
                       {"unit", 1, {}, {{{}, 0.25}}, {}}};
  input.occurrences = {{"whole", "pair", 0, 1, {}, {}},
                       {"left", "unit", 1, 2, {-0.5, 0, 0}, {}},
                       {"right", "unit", 2, 4, {0.5, 0, 0}, {}}};
  input.selections = {{"whole", {"left", "right"}, Exposure::Aggregate}};
  field::Parameters parameters;
  {
    FixedStudy aggregate(runtime, input, parameters);
    aggregate.tick(parameters); aggregate.download();
    CHECK_TRUE(aggregate.prepared().particles.size() == 1 && aggregate.prepared().participation.size() == 1 &&
                   aggregate.fields().particle(field::kMass, 0) == 2 && aggregate.fields().particle(field::kMass, 1) == 0,
               arch + ": aggregate selection excludes exposed parts from mass and fields");
  }
  input.selections[0].exposure = Exposure::Parts;
  {
    FixedStudy parts(runtime, input, parameters);
    parts.tick(parameters); parts.download();
    CHECK_TRUE(parts.prepared().particles.size() == 2 && parts.prepared().participation.size() == 2 &&
                   parts.fields().particle(field::kMass, 0) == 0 && parts.fields().particle(field::kMass, 1) == 1 &&
                   parts.fields().particle(field::kMass, 2) == 1,
               arch + ": exposed parts exclude aggregate, preserve total structural mass");
  }
  for (int capacity : {0, 3}) {
    StudyInput empty; empty.capacity = capacity;
    FixedStudy study(runtime, empty, parameters);
    for (int tick = 0; tick < 8; ++tick) study.tick(parameters);
    study.download();
    CHECK_TRUE(study.ready() && std::all_of(study.fields().particle_data.begin(), study.fields().particle_data.end(),
                                            [](float value) { return value == 0 && std::isfinite(value); }) &&
                   study.fields().edge_count() == 0 && study.fields().group_count() == 0 &&
                   study.fields().contact_count() == 0 && study.fields().determined().size() == std::size_t(capacity),
               arch + ": empty/all-free study with capacity " + std::to_string(capacity) + " stays inert");
  }
}
}  // namespace

int main() {
  check_study(false, "cpu");
  check_exposure_and_empty(false, "cpu");
  if (taichi::is_cuda_api_available()) {
    check_study(true, "cuda"); check_exposure_and_empty(true, "cuda");
  } else {
    harness::note("no cuda on this host, cpu only");
  }
  return harness::report("assembly_test");
}
