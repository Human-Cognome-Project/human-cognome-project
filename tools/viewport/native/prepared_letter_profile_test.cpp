#include "prepared_letter_profile.h"
#include "snapshot.h"
#include "letter_projection_fixture.h"
#include "field/cascade_source.h"
#include "field/prepared_storage.h"
#include <iostream>
#include <map>

namespace {
void check(bool ok, const char *text) { if (!ok) throw std::runtime_error(text); }
template<class F> void rejects(F f) {
  bool refused = false;
  try { f(); } catch (const warm::Error &) { refused = true; }
  check(refused,"unsupported profile input accepted");
}
void run(int workers, field::HostPath path) {
  using namespace assembly;
  letter_fixture::Fixture fixture;
  const auto english = fixture.ref(fixture.english);
  auto invalid = fixture.base;
  invalid.tools[fixture.tools.at(fixture.english)].member_of.push_back(fixture.ref(fixture.french));
  rejects([&] { viewport::prepared_english_pair(invalid,english); });
  rejects([&] { viewport::prepared_english_pair(fixture.base,fixture.ref(fixture.french)); });
  const auto recipe = viewport::prepared_english_pair(fixture.base,english);
  check(recipe.recipe_version == 2 && recipe.occurrences.empty() && recipe.selections.size() == 2 &&
        recipe.focal_fields.size() == 1,"profile inserted starts or changed selection");
  for (const auto &t : fixture.base.tools) {
    bool found = false;
    for (const auto &pin : recipe.tools)
      if (pin.ref == t.ref && pin.epoch == t.epoch && warm::tool_body(pin) == warm::tool_body(t)) found = true;
    check(found,"profile changed supplied exact definition pins");
  }
  const auto owner = warm::new_uuid();
  auto source = std::make_shared<const ActiveAssembly>(ActiveAssembly::assemble(recipe,owner,false));
  auto request = viewport::prepared_english_pair_submission(source,english,path);
  check(request.bindings.bodies.size() == 2 && request.bindings.layout.size() == 1 &&
        !request.bindings.layout[0].node.layout && request.bindings.layout[0].units == 1 &&
        request.bindings.comparisons.size() == 2 && request.bindings.focus.size() == 1,
        "profile introduced a native layout or changed explicit topology");
  std::map<std::string,int> addresses;
  for (const auto &b : request.bindings.bodies) {
    check(b.definition == english && b.mass == 1.5 && b.geometry.complete,"profile body choices changed");
    addresses.emplace(occurrence_text(b.occurrence),b.address);
  }
  engine::InstanceSettings settings;
  settings.read_environment = false; settings.cpu_threads = workers;
  ActiveSession session(settings);
  check(!session.on_cuda(),"actual CPU backend required");
  auto refused = request; refused.plan_work = 0;
  check(!session.submit_prepared(std::move(refused),owner).handle,"profile ignored plan allowance");
  const auto result = session.submit_prepared(std::move(request),owner);
  check(result.handle && session.ready(*result.handle),"profile submission failed");
  const auto handle = *result.handle;
  field::Parameters64 parameters; parameters.dt = 0.1;
  const auto before = viewport::observe(session,owner,handle,viewport::IntentRequest{parameters.gravity,UINT64_MAX});
  check(before.comparisons.size() == 2 && before.tick == 0 && before.body_records.size() == 2,
        "profile did not install native comparisons and bodies");
  for (std::size_t i=0; i<before.body_records.size(); ++i) {
    const auto &h = before.body_records[i];
    const auto &p = before.report.prepared.particles.at(i);
    check(h.context && h.incarnation && h.record == p.occurrence.slot &&
          h.address == addresses.at(std::string(p.occurrence.id)),"profile observation changed actual claims");
  }
  // Exercise the actual nonzero-dt profile defaults, not a fixture-initialized Harness.
  const auto tick = session.tick_prepared(handle,owner,parameters,1000000000,1000000);
  check(tick.status == field::TickStatus::Ok && tick.completed_ticks == 1,"profile native tick failed");
  const auto after = viewport::observe(session,owner,handle,viewport::IntentRequest{parameters.gravity,UINT64_MAX});
  check(after.tick == 1 && after.prepared_generation == tick.generation && after.intent.at("status") == "ok",
        "profile observer did not follow completed native tick");
  for (std::size_t i=0; i<before.body_records.size(); ++i)
    check(field::detail::same_handle(before.body_records[i],after.body_records[i]),"tick changed source claim");
  auto ctx = viewport::context(*source,handle,1,"Synthetic source: prepared English pair profile");
  ctx.source = {{"kind","fixture_only"},{"study_profile","prepared-english-pair"}};
  const auto value = viewport::snapshot(after,ctx);
  check(value.at("execution").at("mode") == "installed_hierarchy" &&
        value.at("execution").at("completed_tick") == "1" && value.at("particles").size() == 2 &&
        value.at("numerics").at("profile") == "trial_f64", "profile capture lost native evidence");
  const auto legacy = normalize_letters(fixture.base,{fixture.request(fixture.english,0,warm::Exposure::Aggregate)});
  const auto legacy_source = ActiveAssembly::assemble(legacy,owner,false);
  field::Parameters legacy_parameters;
  const auto legacy_handle = session.submit(legacy_source,legacy_parameters,path);
  session.tick(legacy_handle,legacy_parameters);
  check(session.completed_ticks(legacy_handle) == 1,"profile prevented legacy capture from ticking");
  std::cout << "prepared_letter_profile,source=fixture_only,retained_acquisition=unqualified"
    ",exact_pins=preserved,actual_claims=2,comparisons=2,completed_tick=1,legacy_tick=1 PASS\n";
}
void run_aggregate_recipes(int workers, field::HostPath path) {
  using namespace assembly;
  letter_fixture::Fixture fixture;
  const auto english = fixture.ref(fixture.english);
  const auto member = fixture.add(letter_fixture::object("02.00.00.00.00"),"Study membership",0);
  fixture.tool(fixture.base,fixture.english).member_of = {member};
  fixture.tool(fixture.base,fixture.accented).member_of = {member};
  fixture.tool(fixture.base,fixture.repeated).member_of = {member};
  const auto owner = warm::new_uuid();
  engine::InstanceSettings settings;
  settings.read_environment = false;
  settings.cpu_threads = workers;
  ActiveSession session(settings);
  check(!session.on_cuda(),"actual CPU backend required");
  for (const auto &object : {fixture.accented,fixture.repeated}) {
    const auto endpoint = fixture.ref(object);
    auto definitions = fixture.base;
    definitions.ref.revision = warm::new_uuid();
    definitions.focal_fields = {{warm::FieldKind::Membership,member.object}};
    auto recipe = normalize_structural_letters(std::move(definitions),
        {{37,endpoint,warm::Exposure::Aggregate},{9,english,warm::Exposure::Aggregate}});
    const auto pin = recipe.ref;
    auto source = std::make_shared<const ActiveAssembly>(
        ActiveAssembly::assemble(std::move(recipe),owner,false));
    const auto parts = source->expose_all(warm::Exposure::Parts);
    rejects([&] { viewport::declared_aggregate_bodies(parts); });
    detail::AdmissionBindings binding;
    binding.source = {owner,source->study(),source->generation(),pin};
    binding.layout = {{{nullptr,-1,ScaleRole::Root},1.}};
    binding.bodies = viewport::declared_aggregate_bodies(*source);
    check(binding.bodies.size() == 2 && binding.bodies[0].definition == endpoint &&
          binding.bodies[1].definition == english,
          "recipe selection changed its definition pins");

    // This study declares a parent-value leaf for each distinct source value,
    // branches by definition, and a common membership as the comparison root.
    const auto compare = [&](const FieldKey &field, int level, std::vector<int> children) {
      const auto index = int(binding.fields.size());
      binding.fields.push_back({field,5,index});
      binding.comparisons.push_back({field,100+std::uint64_t(index),level,52,std::move(children)});
      return index;
    };
    std::vector<int> definitions_in_study;
    for (const auto &key : {fixture.english,object}) {
      std::vector<int> children;
      std::map<warm::ObjectKey,int> leaves;
      for (const auto &parent : fixture.tool(fixture.base,key).parents) {
        if (leaves.count(parent.target.object)) continue;
        const auto index = compare({GroupKind::ParentValue,parent.target.object},0,{});
        leaves.emplace(parent.target.object,index);
        children.push_back(index);
      }
      definitions_in_study.push_back(compare({GroupKind::SiblingDefinition,key},1,std::move(children)));
    }
    binding.focus = {{GroupKind::Membership,member.object}};
    compare(binding.focus.front(),2,std::move(definitions_in_study));

    using field::Budget;
    field::BudgetDemand limits;
    limits[Budget::Potential] = limits[Budget::Particles] = 2;
    limits[Budget::Fields] = limits[Budget::CascadeNodes] = binding.fields.size();
    limits[Budget::Participation] = limits[Budget::CascadeReferences] = limits[Budget::CascadeCoverage] = 16;
    limits[Budget::CascadeLinks] = binding.fields.size()-1;
    limits[Budget::CascadeLevels] = 3;
    limits[Budget::Focus] = 1;
    limits[Budget::References] = 1024;
    limits[Budget::Batch] = limits[Budget::BootstrapCommands] = 32;
    limits[Budget::GeometryWork] = limits[Budget::CascadeWork] = 1000000;
    limits[Budget::LiveResponses] = 64;
    limits[Budget::TickWork] = 1000000000;
    limits[Budget::PlacementWork] = 100000000;
    limits[Budget::PlacementCells] = 65536;
    const auto profile = field::NumericalProfile::trial();
    limits[Budget::ScratchBytes] = field::detail::PreparedStorage::required_scratch(limits,false,profile)
        + 256*1024*1024;
    PreparedStudySubmission request{source,std::move(binding),field::PreparedCapacity(limits,profile),
        std::make_shared<detail::ReservationDomain>(detail::ReservationBudget{16*1024*1024,16*1024*1024}),path};
    const auto expected_mass = *fixture.tool(fixture.base,object).mass;
    const auto reference_count = object==fixture.repeated ? 8u : 6u;
    const auto result = session.submit_prepared(std::move(request),owner);
    if (!result.handle || !session.ready(*result.handle)) {
      const auto &failure = result.failure;
      std::cerr << "declared recipe admission,plan=" << int(result.preparation_status)
                << ",publication=" << int(result.publication.status)
                << ",budget=" << field::budget_name(failure.budget)
                << ",required=" << failure.required << ",capacity=" << failure.capacity << '\n';
      throw std::runtime_error("declared recipe submission failed");
    }
    const auto handle = *result.handle;
    const auto before = viewport::observe(session,owner,handle);
    check(before.tick == 0 && before.body_records.size() == 2 &&
          std::string(before.report.prepared.particles.at(0).occurrence.id) == occurrence_text(source->study(),{9,{}}) &&
          std::string(before.report.prepared.particles.at(1).occurrence.id) == occurrence_text(source->study(),{37,{}}) &&
          before.report.prepared.particles.at(0).exact_mass == 2 &&
          before.report.prepared.particles.at(1).exact_mass == expected_mass,
          "declared recipe lost ordered native bodies or exact masses");
    std::uint64_t native_references = 0;
    for (const auto &comparison : before.comparisons)
      native_references += comparison.cohort_count;
    check(native_references == reference_count,
          "declared recipe lost exact comparison cohort rows");
    field::Parameters64 parameters;
    const auto tick = session.tick_prepared(handle,owner,parameters,1000000000,1000000);
    const auto after = viewport::observe(session,owner,handle);
    check(tick.status == field::TickStatus::Ok && after.tick == 1 &&
          after.report.prepared.particles.at(0).exact_mass == 2 &&
          after.report.prepared.particles.at(1).exact_mass == expected_mass,
          "declared recipe native tick changed source accounting");
    for (std::size_t i=0;i<before.body_records.size();++i)
      check(field::detail::same_handle(before.body_records[i],after.body_records[i]),
            "declared recipe native tick changed a live claim");
    std::cout << "prepared_aggregate_recipe,selection=" << pin.selection.str()
              << ",revision=" << pin.revision.str() << ",body_count=2,source_mass="
              << 2+expected_mass << ",references=" << reference_count << ",completed_tick=1 PASS\n";
  }
}
}
int main(int argc, char **argv) {
  try {
    if (argc != 3) throw std::runtime_error("workers path required");
    const auto workers = std::stoi(argv[1]);
    const auto path = std::string(argv[2]) == "direct" ? field::HostPath::CpuDirect : field::HostPath::BulkCopy;
    run(workers,path);
    run_aggregate_recipes(workers,path);
    return 0;
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
