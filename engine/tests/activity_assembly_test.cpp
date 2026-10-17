#include <cmath>
#include <iostream>

#include "assembly/study.h"
#include "harness.h"
#include "membership_fixture.h"

namespace {
int checks = 0;
void check(bool ok, const char *label) {
  ++checks;
  harness::record(ok, label, "focal assembly contract failed");
  if (!ok) throw warm::Error(label);
}
template <class F>
void rejects(F run, const char *label) {
  bool rejected = false;
  try {
    run();
  } catch (const std::exception &) {
    rejected = true;
  }
  check(rejected, label);
}
}  // namespace
int main(int argc, char **argv) {
  try {
    engine::InstanceSettings settings;
    settings.cuda = argc == 2 && std::string(argv[1]) == "--cuda";
    settings.read_environment = false;
    settings.cpu_threads = 1;
    engine::Runtime runtime(settings);
    membership_fixture::Fixture f;
    auto first = f.request(f.english, 0, warm::Exposure::Aggregate);
    auto second = f.request(f.english, 1, warm::Exposure::Aggregate);
    first.aggregate = {{100, 0, 0}, {}};
    second.aggregate = {{102, 0, 0}, {0.1, 0, 0}};
    const auto projection = assembly::project_letters(
        assembly::normalize_letters(f.base, {first, second}));
    const auto fields = assembly::membership_fields(projection);
    using namespace taichi::lang;
    auto root = std::make_unique<SNode>(0, SNodeType::root);
    auto *bound = root.get();
    auto &leaf = root->dense(Axis(0), 1).insert_children(SNodeType::place);
    leaf.dt = PrimitiveType::i32;
    runtime.program().add_snode_tree(std::move(root), false);
    auto input =
        fields.bind(projection, {{bound, -1, assembly::ScaleRole::Root}});
    input.focal_fields = {{assembly::GroupKind::Membership, warm::key(f.en)}};
    field::Parameters p;
    p.gravity = 0;
    p.dt = 0.1f;
    assembly::FixedStudy study(runtime, input, p);
    const auto &prepared = study.prepared();
    check(prepared.focal_groups.size() == 1 &&
              prepared.groups.at(prepared.focal_groups[0]).kind ==
                  assembly::GroupKind::Membership &&
              prepared.groups.at(prepared.focal_groups[0]).value ==
                  warm::key(f.en),
          "typed focal key binds the actual English membership field, without "
          "a group particle");
    study.tick(p);
    study.download();
    check(study.demand().focal_origin[0] == 101 &&
              study.demand().particles_exposed == 2,
          "two retained English letter objects establish the mathematical "
          "focal origin");
    bool carried = false;
    for (const auto &particle : prepared.particles)
      if (particle.occurrence.position.x == 102)
        carried = study.fields().particle(field::kVelX,
                                          particle.occurrence.slot) == 0.1f &&
                  std::abs(study.fields().particle(field::kPosX,
                                                   particle.occurrence.slot) -
                           102.01f) < 1e-5f;
    check(carried,
          "prepared letter object integrates relative to focal field, not "
          "global coordinate zero");
    auto wrong_kind = input;
    wrong_kind.focal_fields[0].kind = assembly::GroupKind::ParentValue;
    rejects([&] { assembly::prepare(wrong_kind); },
            "same text with wrong field kind cannot select focus");
    auto duplicate = input;
    duplicate.focal_fields.push_back(duplicate.focal_fields[0]);
    rejects([&] { assembly::prepare(duplicate); },
            "duplicate typed focal field rejected");
    rejects([&] { study.publish(wrong_kind, p); },
            "invalid focal refresh fails closed");
    check(!study.ready(), "invalid focal generation cannot execute");
    study.publish(input, p);
    study.tick(p);
    check(study.ready() && study.demand().focal_origin[0] == 101,
          "valid supplied focal generation reseeds and recovers readiness");
  } catch (const std::exception &e) {
    harness::record(false, "unexpected exception", e.what());
  }
  std::cout << checks << " focal assembly checks\n";
  return harness::report("activity_assembly_test");
}
