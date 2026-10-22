#pragma once
#include "prepared_fixture.h"
#include "assembly/study.h"
#include "membership_fixture.h"
namespace prepared_sources {
using namespace field;
using namespace field::testing;
inline FixtureSource working(const BudgetDemand &limits) {
  FixtureSource s;
  const int p = int(limits[Budget::Particles] * 4 / 5),
            g = int(limits[Budget::Fields]) - 2;
  s.particles.resize(p);
  for (int i = 0; i < p; ++i) {
    auto &v = s.particles[i];
    v[kPosX] = i == 1 ? 0.05f : (i % 19) * 0.125f;
    v[kPosY] = (i / 19 % 19) * 0.125f;
    v[kPosZ] = (i / 361) * 0.125f;
    v[kMass] = 1.0f + (i % 2);
    v[kRadius] = 0.04f;
    v[kVelX] = (i % 3 - 1) * 0.015625f;
  }
  for (int i = 0; i < g; ++i) {
    s.kinds.push_back(i % 3 + 1);
    s.focus.push_back(i);
  }
  for (std::size_t i = 0; i < limits[Budget::Participation] - 3; ++i)
    s.edges.push_back({int(i % p), int(i % g), i % 2 ? 0.5f : 1.0f, i});
  for (std::size_t i = 0; i < limits[Budget::Contacts] - 3; ++i) {
    const int a = int((17 * i + 1) % p), b = (a + 1 + int(i % (p - 1))) % p;
    s.contacts.push_back({a, b});
  }
  return s;
}
inline FixtureSource letters(engine::Runtime &runtime, int mode) {
  membership_fixture::Fixture f;
  auto first =
      f.request(f.english, 0,
                mode == 1 ? warm::Exposure::Parts : warm::Exposure::Aggregate);
  auto second =
      f.request(f.english, 1,
                mode == 0 ? warm::Exposure::Aggregate : warm::Exposure::Parts);
  first.aggregate = {{0, 0, 0}, {0.015625, 0, 0}};
  second.aggregate = {{2, 0, 0}, {-0.015625, 0, 0}};
  for (auto &byte : first.bytes)
    byte.initial = {{0, 0, 0}, {0.015625, 0, 0}};
  for (auto &byte : second.bytes)
    byte.initial = {{2, 0, 0}, {-0.015625, 0, 0}};
  auto recipe = assembly::normalize_letters(f.base, {first, second});
  recipe.contacts = mode == 0 ? std::vector<warm::Contact>{{{0, {}}, {1, {}}}}
                    : mode == 1
                        ? std::vector<warm::Contact>{{{0, {0, 0}}, {1, {0, 0}}}}
                        : std::vector<warm::Contact>{{{0, {}}, {1, {0, 0}}}};
  const auto projection = assembly::project_letters(recipe);
  const auto memberships = assembly::membership_fields(projection);
  using namespace taichi::lang;
  auto root = std::make_unique<SNode>(0, SNodeType::root);
  auto *node = root.get();
  auto &primary = root->dense(Axis(0), 1);
  primary.insert_children(SNodeType::place).dt = PrimitiveType::i32;
  std::vector<assembly::StudyNode> nodes{{node, -1, assembly::ScaleRole::Root}};
  if (mode != 0)
    nodes.push_back({&primary, 0, assembly::ScaleRole::PrimaryStep});
  runtime.program().add_snode_tree(std::move(root), false);
  auto input = memberships.bind(projection, nodes);
  input.focal_fields = {{assembly::GroupKind::Membership, warm::key(f.en)}};
  const auto prepared = assembly::prepare(input);
  FixtureSource s;
  std::vector<int> mapping(prepared.capacity, -1);
  for (const auto &particle : prepared.particles) {
    mapping[particle.occurrence.slot] = int(s.particles.size());
    std::array<float, kParticleFieldCount> v{};
    v[kPosX] = float(particle.position.x);
    v[kPosY] = float(particle.position.y);
    v[kPosZ] = float(particle.position.z);
    v[kVelX] = float(particle.velocity.x);
    v[kVelY] = float(particle.velocity.y);
    v[kVelZ] = float(particle.velocity.z);
    v[kMass] = particle.mass;
    v[kRadius] = particle.radius;
    s.particles.push_back(v);
  }
  for (const auto &group : prepared.groups)
    s.kinds.push_back(int(group.kind) + 1);
  for (const auto &edge : prepared.participation)
    s.edges.push_back(
        {mapping.at(edge.slot), edge.group, edge.share, edge.ordinal});
  for (const auto &contact : prepared.contacts)
    s.contacts.push_back({mapping.at(contact.a), mapping.at(contact.b)});
  s.focus = prepared.focal_groups;
  return s;
}
inline void stage(Harness &h, const FixtureSource &s) {
  for (std::size_t i = 0; i < s.particles.size(); ++i)
    for (int f = 0; f < kParticleFieldCount; ++f)
      h.particle(f, int(i)) = s.particles[i][f];
  for (std::size_t i = 0; i < s.edges.size(); ++i) {
    h.edge_int(kEdgeParticle, int(i)) = s.edges[i].particle;
    h.edge_int(kEdgeGroup, int(i)) = s.edges[i].group;
    h.edge_float(kShare, int(i)) = s.edges[i].share;
  }
  for (std::size_t i = 0; i < s.contacts.size(); ++i) {
    h.contact_int(kContactA, int(i)) = s.contacts[i][0];
    h.contact_int(kContactB, int(i)) = s.contacts[i][1];
  }
}
}  // namespace prepared_sources
