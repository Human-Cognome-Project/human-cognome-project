#include "prepared_letter_profile.h"
#include "snapshot.h"
#include "letter_projection_fixture.h"
#include "field/cascade_source.h"
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
}
int main(int argc, char **argv) {
  try {
    if (argc != 3) throw std::runtime_error("workers path required");
    run(std::stoi(argv[1]),std::string(argv[2]) == "direct" ? field::HostPath::CpuDirect : field::HostPath::BulkCopy);
    return 0;
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
