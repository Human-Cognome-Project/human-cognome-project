#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

#include "allocation_observation.h"
#include "prepared_fixture.h"
#include "prepared_metrics.h"
#include "assembly/study.h"
#include "membership_fixture.h"

namespace {
using namespace field;
using namespace field::testing;
void require(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
template <class F>
std::array<std::uint64_t, 2> observe(F action) {
  allocation_observation::calls = 0;
  allocation_observation::bytes = 0;
  allocation_observation::enabled = true;
  try {
    action();
  } catch (...) {
    allocation_observation::enabled = false;
    throw;
  }
  allocation_observation::enabled = false;
  return {allocation_observation::calls.load(),
          allocation_observation::bytes.load()};
}
bool near(float a, float b) {
  return std::isfinite(a) && std::isfinite(b) &&
         std::abs(a - b) <= 2e-5f * std::max({1.0f, std::abs(a), std::abs(b)});
}
FixtureSource working(const BudgetDemand &limits) {
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
FixtureSource letters(engine::Runtime &runtime, int mode) {
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
void stage(Harness &h, const FixtureSource &s) {
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
void limits_and_failures(engine::Runtime &runtime, HostPath path) {
  BudgetDemand l;
  l.values = {70, 3, 2, 2, 1, 100, 1, 2, 0};
  l[Budget::ScratchBytes] =
      detail::PreparedStorage::required_scratch(l, runtime.on_cuda());
  auto insufficient = l;
  --insufficient[Budget::ScratchBytes];
  const auto issued = runtime.issued_snode_ids();
  bool rejected = false;
  try {
    Harness invalid(runtime, PreparedCapacity(insufficient), path);
  } catch (const CapacityError &error) {
    rejected = error.failure.budget == Budget::ScratchBytes &&
               error.failure.required == l[Budget::ScratchBytes] &&
               error.failure.capacity == insufficient[Budget::ScratchBytes];
  }
  require(rejected && runtime.issued_snode_ids() == issued,
          "scratch rejection created native layout");
  Harness h(runtime, PreparedCapacity(l), path);
  const auto &s = h.prepared_storage();
  for (int kind = 0; kind < 6; ++kind) {
    detail::PreparedCounts n;
    Budget expected = Budget::Particles;
    switch (kind) {
      case 0:
        n.particles = 4;
        break;
      case 1:
        n.fields = 3;
        expected = Budget::Fields;
        break;
      case 2:
        n.edges = 3;
        expected = Budget::Participation;
        break;
      case 3:
        n.contacts = 2;
        expected = Budget::Contacts;
        break;
      case 4:
        n.focus = 2;
        expected = Budget::Focus;
        break;
      case 5:
        expected = Budget::Batch;
        break;
    }
    const auto failure = s.check(n, kind == 5 ? 3 : 0);
    require(failure.budget == expected && h.ready() && s.generation == 0 &&
                s.particles.size() == 0,
            "independent capacity failure changed ready state");
  }
  // Public legacy operations cannot reset the prepared construct.
  rejected = false;
  try {
    h.publish({});
  } catch (const std::logic_error &) {
    rejected = true;
  }
  require(rejected && h.ready(), "legacy publication reset prepared execution");
  h.particle_data.resize(h.particle_data.size() + 1);
  rejected = false;
  try {
    h.tick({});
  } catch (const std::logic_error &) {
    rejected = true;
  }
  require(rejected && !h.ready(),
          "changed borrowed host shape retained readiness");
  l[Budget::References] = 1;
  Harness refs(runtime, PreparedCapacity(l), path);
  require(refs.prepared_storage().check({2, 0, 0, 0, 0}, 0).budget ==
                  Budget::References &&
              refs.prepared_storage().particles.size() == 0 && refs.ready(),
          "reference limit was inferred from particle slots");
  BudgetDemand zero;
  zero[Budget::Potential] = 1;
  zero[Budget::ScratchBytes] =
      detail::PreparedStorage::required_scratch(zero, runtime.on_cuda());
  Harness empty(runtime, PreparedCapacity(zero), path);
  empty.tick({});
  empty.inspect_prepared();
  require(empty.ready() && empty.present_particles() == 0 &&
              empty.prepared_storage().inspection_counts ==
                  std::array<std::int64_t, 4>{},
          "zero capacity sentinel became live work");
  std::cout << "independent limits, pre-allocation scratch rejection, stale "
               "host shape and zero capacity PASS\n";
}
void one(engine::Runtime &runtime,
         BudgetDemand limits,
         const FixtureSource &source,
         HostPath path,
         const char *label) {
  Parameters parameters;
  parameters.gravity = 0.0001f;
  parameters.dt = 0.015625f;
  parameters.contact_stiffness = 0.001f;
  parameters.reach_strength = 0;
  constexpr int ticks = 32;
  const std::size_t p = source.particles.size(), g = source.kinds.size();
  std::vector<float> golden(ticks *
                            (p * kParticleFieldCount + g * kGroupFieldCount));
  {
    Harness reference(runtime, int(p), int(g), int(source.edges.size()),
                      int(source.contacts.size()), path);
    stage(reference, source);
    reference.publish(parameters);
    // Source starts already use the fixture's local frame. Keep the selected
    // native membership fields without invoking the legacy rebasing adapter.
    if (!source.focus.empty())
      reference.set_focus(source.focus);
    for (int tick = 0; tick < ticks; ++tick) {
      if (tick == 16)
        reference.set_tick_mode(TickMode::AllLoaded);
      reference.tick(parameters);
      reference.download();
      auto *out = golden.data() +
                  tick * (p * kParticleFieldCount + g * kGroupFieldCount);
      for (std::size_t i = 0; i < p; ++i)
        for (int f = 0; f < kParticleFieldCount; ++f)
          *out++ = reference.particle(f, int(i));
      for (std::size_t i = 0; i < g; ++i)
        for (int f = 0; f < kGroupFieldCount; ++f)
          *out++ = reference.group(f, int(i));
    }
  }
  limits[Budget::ScratchBytes] =
      detail::PreparedStorage::required_scratch(limits, runtime.on_cuda());
  Harness h(runtime, PreparedCapacity(limits), path);
  auto &storage = h.prepared_storage();
  require(h.ready() && storage.generation == 0 && storage.live.particles == 0 &&
              storage.inspection_counts == std::array<std::int64_t, 4>{} &&
              h.universal_data[0] == 0,
          "prepared setup created objects, activity or mass");
  prepared_test::Metrics metrics(runtime, PreparedFixtureAccess::nodes(h));
  const auto ready = metrics.sample();
  PreparedFixtureAccess::Input input(
      limits[Budget::Particles], limits[Budget::Fields], limits[Budget::Batch],
      limits[Budget::Focus]);
  const auto allocation = observe([&] {
    PreparedFixtureAccess::load(h, source, input, parameters, true);
    h.inspect_prepared();
  });
  require(allocation[0] == 0,
          "first fixture population allocated C++ control metadata");
  require(ready.same_backing(metrics.sample()),
          "first nonempty field execution acquired native backing or compiled");
  require(storage.live.particles == p && storage.live.fields == g &&
              storage.generation == 1,
          "committed fixture counts wrong");
  require(storage.inspection_counts[0] == std::int64_t(p) &&
              storage.inspection_counts[2] == std::int64_t(g),
          "presence includes holes");
  for (std::size_t i = 0; i < g; ++i)
    require(PreparedFixtureAccess::field_kind(h, input.field_rows[i]) ==
                source.kinds[i],
            "typed field kind was lost");
  for (std::size_t i = 0; i < source.edges.size(); ++i)
    require(storage.edges[i].ordinal == source.edges[i].ordinal &&
                storage.edges[i].share == source.edges[i].share,
            "source contribution order/identity was lost");
  float expected_mass = 0;
  for (const auto &v : source.particles)
    expected_mass += v[kMass];
  require(h.universal_data[0] == expected_mass,
          "universal mass includes holes or omits presence");
  for (std::size_t group = 0; group < g; ++group) {
    double mass = 0;
    for (const auto &edge : source.edges)
      if (edge.group == int(group))
        mass += source.particles[edge.particle][kMass] * edge.share;
    require(near(h.group(kCenM, input.field_rows[group].record), float(mass)),
            "inclusive field mass incorrect");
  }
  std::uint64_t evaluated = 0, forces = 0, contacts = 0;
  for (int tick = 0; tick < ticks; ++tick) {
    const auto recurring = observe([&] {
      if (tick == 16)
        h.set_tick_mode(TickMode::AllLoaded);
      h.tick(parameters);
      h.inspect_prepared();
    });
    require(recurring[0] == 0,
            "prepared tick/inspection allocated C++ metadata");
    require(ready.same_backing(metrics.sample()),
            "prepared tick obtained backing, layout or compilation");
    const auto *expected =
        golden.data() + tick * (p * kParticleFieldCount + g * kGroupFieldCount);
    for (std::size_t i = 0; i < p; ++i)
      for (int f = 0; f < kParticleFieldCount; ++f)
        require(near(h.particle(f, input.particle_rows[i].record), *expected++),
                "prepared particle result differs from existing dense fixture");
    for (std::size_t i = 0; i < g; ++i)
      for (int f = 0; f < kGroupFieldCount; ++f)
        require(
            near(h.group(f, input.field_rows[i].record), *expected++),
            "prepared centroid/history differs from existing dense fixture");
    for (std::size_t i = 0; i < storage.inspection_flags.size(); ++i)
      require(storage.inspection_flags[i] != 2,
              "absent record acquired awake state");
    require(h.demand().particles_loaded == p &&
                h.demand().edges_loaded == source.edges.size(),
            "report confuses capacity and presence");
    evaluated += h.demand().particles_evaluated;
    forces += h.demand().force_evaluations;
    contacts += h.demand().contact_evaluations;
  }
  require(evaluated > 0 && forces > 0, "real field kernel bodies did not run");
  if (!source.contacts.empty())
    require(contacts > 0, "real contact kernel did not run");
  const auto final = metrics.sample();
  std::cout << "profile=" << label
            << ",backend=" << (runtime.on_cuda() ? "cuda" : "cpu")
            << ",path=" << (path == HostPath::CpuDirect ? "direct" : "bulk")
            << ",V=" << limits[Budget::Potential]
            << ",P=" << limits[Budget::Particles]
            << ",G=" << limits[Budget::Fields]
            << ",E=" << limits[Budget::Participation]
            << ",C=" << limits[Budget::Contacts]
            << ",R=" << limits[Budget::References]
            << ",F=" << limits[Budget::Focus] << ",B=" << limits[Budget::Batch]
            << ",scratch_bytes=" << limits[Budget::ScratchBytes]
            << ",binding_bytes=" << storage.binding_bytes
            << ",present_P=" << storage.live.particles
            << ",present_G=" << storage.live.fields
            << ",live_E=" << storage.live.edges
            << ",live_C=" << storage.live.contacts
            << ",selected_F=" << storage.live.focus
            << ",references_used=" << storage.live.references()
            << ",awake_P=" << storage.inspection_counts[1]
            << ",awake_G=" << storage.inspection_counts[3]
            << ",ticks=32,evaluated=" << evaluated << ",forces=" << forces
            << ",contacts=" << contacts << '\n';
  std::cout << "phase,native_requested_bytes,native_chunks,native_elements,"
               "pool_calls,pool_requested,pool_raw_calls,pool_reserved,host_"
               "cursor,trees,snode_ids,compile_seconds\n";
  ready.print("ready_empty");
  final.print("after_32_ticks");
  for (const auto &entry : storage.capacity.numerical_shapes()) {
    const std::string name = entry.name;
    const bool host = name == "particles" || name == "groups" ||
                      name == "edge-floats" || name == "edge-integers" ||
                      name == "contacts" || name == "determiners" ||
                      name == "universal-mass";
    std::cout << "numerical," << entry.name << ',' << entry.rows << ','
              << entry.stride << ',' << entry.element_bytes << ','
              << (host ? 1 : 0) << ','
              << (host && path == HostPath::CpuDirect ? 0 : 1) << ','
              << entry.bytes << '\n';
  }
  std::cout << "native_root,particle-map,"
            << metrics.impl
                   .get_cached_field(storage.particles.bindings()
                                         .presence->get_snode_tree_id())
                   .root_size
            << "\nnative_root,field-map,"
            << metrics.impl
                   .get_cached_field(
                       storage.fields.bindings().presence->get_snode_tree_id())
                   .root_size
            << "\nnative_root,awake,"
            << metrics.impl
                   .get_cached_field(metrics.nodes.back()->get_snode_tree_id())
                   .root_size
            << '\n';
  for (const auto &entry : storage.manifest)
    std::cout << "workspace," << entry.name << ',' << entry.count << ','
              << entry.element_bytes << ',' << entry.copies << ','
              << entry.bytes << '\n';
  std::cout << "C++ new: first population=" << allocation[0] << '/'
            << allocation[1] << ",recurring=0/0; PASS\n";
}
}  // namespace
int main(int argc, char **argv) {
  try {
    bool cuda = false, direct = false, large = false, language = false;
    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--cuda")
        cuda = true;
      else if (arg == "--direct")
        direct = true;
      else if (arg == "--large")
        large = true;
      else if (arg == "--letters")
        language = true;
      else
        throw std::invalid_argument(
            "expected --cuda --direct --large --letters");
    }
    require(!(cuda && direct), "direct path is CPU only");
    engine::InstanceSettings settings;
    settings.read_environment = false;
    settings.cuda = cuda;
    settings.cpu_threads = 2;
    if (cuda)
      settings.device_memory_gb = 0.5;
    engine::Runtime runtime(settings);
    require(!cuda || runtime.on_cuda(),
            "requested CUDA, actual backend is CPU");
    BudgetDemand limits;
    limits.values =
        large ? std::array<std::uint64_t, 9>{16777216, 4096, 257, 16384, 8192,
                                             53762,    257,  256, 0}
              : std::array<std::uint64_t, 9>{1048576, 70, 9,  280, 140,
                                             928,     9,  32, 0};
    const auto path = direct ? HostPath::CpuDirect : HostPath::BulkCopy;
    if (language)
      for (int mode = 0; mode < 3; ++mode)
        one(runtime, limits, letters(runtime, mode), path,
            mode == 0   ? "letters"
            : mode == 1 ? "bytes"
                        : "mixed");
    else
      one(runtime, limits, working(limits), path,
          large ? "working-set" : "padded");
    if (!large && !language)
      limits_and_failures(runtime, path);
  } catch (const std::exception &e) {
    std::cerr << "prepared_execution: " << e.what() << '\n';
    return 1;
  }
}
