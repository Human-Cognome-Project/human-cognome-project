#include <cmath>
#include <limits>
#include <stdexcept>

#include "assembly/study.h"
#include "harness.h"

namespace {
using namespace assembly;

template <typename Function>
bool refused(Function function) {
  try { function(); } catch (const std::invalid_argument &) { return true; }
  return false;
}

void check_geometry() {
  const Point centre{1, 2, 3};
  const std::vector<Sphere> spheres{{{4, 6, 3}, 2}, {{1, 2, 2}, 0.5}};
  CHECK_TRUE(enclosing_radius(centre, spheres) == 7, "supplied spheres enclosed from supplied centroid");
  auto shifted = spheres;
  for (auto &sphere : shifted) {
    sphere.centre.x += 11; sphere.centre.y -= 7; sphere.centre.z += 5;
  }
  CHECK_TRUE(enclosing_radius({12, -5, 8}, shifted) == 7, "enclosure is translation invariant");
  CHECK_TRUE(enclosing_radius({}, {{{}, 0.25}, {{}, 0.75}}) == 0.75,
             "unequal coincident radii use maximum extent");
  CHECK_TRUE(refused([] { enclosing_radius({}, {}); }), "missing geometry refused");
  CHECK_TRUE(refused([] { enclosing_radius({}, {{{}, -1}}); }), "negative radius refused");
  CHECK_TRUE(refused([] { enclosing_radius({}, {{{}, std::numeric_limits<double>::infinity()}}); }),
             "infinite radius refused");
  CHECK_TRUE(refused([] { enclosing_radius({std::numeric_limits<double>::quiet_NaN(), 0, 0}, {{{}, 1}}); }),
             "non-finite centroid refused");
  CHECK_TRUE(refused([] { enclosing_radius({}, {{{0, 0, std::numeric_limits<double>::infinity()}, 1}}); }),
             "non-finite component centre refused");
}

void check_scale_and_identity() {
  using namespace taichi::lang;
  auto root = std::make_unique<SNode>(0, SNodeType::root);
  auto &primary = root->dense(Axis(0), 1);
  auto &secondary = primary.dense(Axis(1), 1);
  auto &nested = secondary.dense(Axis(2), 1);
  auto &equivalent = nested.dense(Axis(3), 1);
  auto &coarser = equivalent.dense(Axis(4), 1);
  StudyInput input;
  input.capacity = 2;
  input.nodes = {{root.get(), -1, ScaleRole::Root}, {&primary, 0, ScaleRole::PrimaryStep},
                 {&secondary, 1, ScaleRole::SecondaryCompression},
                 {&nested, 2, ScaleRole::SecondaryCompression},
                 {&equivalent, 3, ScaleRole::NamingEquivalent},
                 {&coarser, 4, ScaleRole::PrimaryStep}};
  input.definitions = {{"reused", 3, {}, {{{-0.25, 0, 0}, 0.4}, {{0.25, 0, 0}, 0.4}}, {}}};
  input.occurrences = {{"a", "reused", 0, 1, {1, 2, 3}, {0.5, 0, 0}},
                       {"b", "reused", 1, 4, {2, 3, 4}, {0.5, 0, 0}}};
  auto prepared = prepare(input);
  CHECK_TRUE(prepared.scales[1].units == 64 && prepared.scales[4].units == 64 &&
                 prepared.scales[5].units == 4096,
             "only explicit primary steps multiply scale, independent of depth/id");
  CHECK_TRUE(prepared.particles[0].position.y == 128 && prepared.particles[1].position.y == 192 &&
                 prepared.particles[1].velocity.x == 32,
             "positions and velocities share operative conversion");
  CHECK_TRUE(prepared.particles[0].radius >= 41.6 && prepared.particles[0].radius < 41.60001 &&
                 prepared.particles[1].radius == prepared.particles[0].radius,
             "radii inherit the same 1:1 compression frame with outward rounding");
  CHECK_TRUE(prepared.particles[0].exact_mass == 3 && prepared.particles[0].mass == 3 &&
                 prepared.particles[0].occurrence.definition == "reused" &&
                 prepared.particles[1].occurrence.id == "b",
             "scale preserves structural mass and cold/occurrence identity");
  auto bad = input;
  bad.nodes[4].parent = 1;
  CHECK_TRUE(refused([&] { prepare(bad); }), "unrelated SNode scale attachment refused");
  bad = input; bad.occurrences[1].slot = 0;
  CHECK_TRUE(refused([&] { prepare(bad); }), "repeated definition cannot share mutable slot");
  bad = input; bad.occurrences[1].position.x = std::numeric_limits<double>::max();
  CHECK_TRUE(refused([&] { prepare(bad); }), "operative coordinate overflow refused");
  bad = input; bad.definitions[0].parents = {{"one", 0, 1}};
  CHECK_TRUE(refused([&] { prepare(bad); }), "missing structural count refused rather than inferred");
  bad = input; bad.definitions[0].parents = {{"one", 0, 1}, {"one", 0, 2}};
  CHECK_TRUE(refused([&] { prepare(bad); }), "duplicate ordinal refused");
  bad = input; bad.contacts = {{"a", "b"}, {"b", "a"}};
  CHECK_TRUE(refused([&] { prepare(bad); }), "duplicate unordered contact refused");
}
}  // namespace

int main() {
  check_geometry();
  check_scale_and_identity();
  return harness::report("preparation_test");
}
