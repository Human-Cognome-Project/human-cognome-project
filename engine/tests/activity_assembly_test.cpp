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
    const auto path = argc == 2 && std::string(argv[1]) == "--direct"
                          ? field::HostPath::CpuDirect
                          : field::HostPath::BulkCopy;
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
    assembly::FixedStudy study(runtime, input, p, path);
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
    check(study.demand().frame_anchor[0] == 100 &&
              study.demand().focal_origin[0] == 1 &&
              study.demand().particles_exposed == 2,
          "two retained English letter objects establish the mathematical "
          "focal origin");
    bool carried = false;
    for (const auto &particle : prepared.particles)
      if (particle.occurrence.position.x == 102)
        carried = study.fields().particle(field::kVelX,
                                          particle.occurrence.slot) == 0.1f &&
                  std::abs(study.fields().supplied_position(
                               0, particle.occurrence.slot) -
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
    check(study.ready() && study.demand().focal_origin[0] == 1,
          "valid supplied focal generation reseeds and recovers readiness");
    for (const double offset : {0.0, 134217728.0}) {
      auto translated = input;
      const auto first_id = prepared.particles.at(0).occurrence.id;
      const auto second_id = prepared.particles.at(1).occurrence.id;
      for (auto &occurrence : translated.occurrences) {
        if (occurrence.id == first_id) occurrence.position = {offset, 0, 0};
        if (occurrence.id == second_id) {
          occurrence.position = {offset + 1, 0, 0};
          occurrence.velocity = {1, 0, 0};
        }
      }
      field::Parameters motion;
      motion.gravity = 0;
      motion.dt = 1;
      motion.reach_strength = 0;
      assembly::FixedStudy local(runtime, translated, motion, path);
      check(local.prepared().frame_anchor.x == offset &&
                local.prepared().particles.at(1).position.x == 1,
            "preparation subtracts supplied double reference before narrowing "
            "to f32");
      const int group = local.prepared().focal_groups.at(0);
      const int moving_slot = local.prepared().particles.at(1).occurrence.slot;
      for (int tick = 1; tick <= 3; ++tick) {
        local.tick(motion);
        local.download();
        check(local.fields().particle(field::kPosX, moving_slot) == 1 + tick &&
                  local.fields().group(field::kCenX, group) ==
                      (1 + tick) * 0.5f &&
                  local.demand().particles_moved == 1 &&
                  local.demand().centroids_recomputed ==
                      local.prepared().groups.size(),
              "translated language objects preserve independently expected "
              "local motion and centroid propagation");
      }
    }
  } catch (const std::exception &e) {
    harness::record(false, "unexpected exception", e.what());
  }
  std::cout << checks << " focal assembly checks\n";
  return harness::report("activity_assembly_test");
}
