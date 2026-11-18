#include "prepared_owner_fixture.h"
#include "allocation_observation.h"
#include "snapshot.h"
#include "field/cascade_source.h"
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>

#include "hierarchy_entry_trace.h"

namespace {
using namespace prepared_owner_fixture;
void check(bool ok, const char *label) { if (!ok) throw std::runtime_error(label); }
template<class F> void rejected(F action, const char *label) {
  bool caught = false;
  try { action(); } catch (const warm::Error &) { caught = true; }
  check(caught,label);
}
template<class F> auto no_new(F action) {
  allocation_observation::calls = 0;
  allocation_observation::enabled = true;
  try {
    auto result = action();
    allocation_observation::enabled = false;
    check(!allocation_observation::calls,"native owner boundary allocated C++ metadata");
    return result;
  } catch (...) { allocation_observation::enabled = false; throw; }
}
std::string suffix(const std::string &id) { return id.substr(id.find('/') + 1); }
void retained_entry(const engine::InstanceSettings &settings, field::HostPath path,
                    const viewport::Json &retained) {
  // Reproduce construction/path/mass using the existing fixture definitions.
  // Its extra byte IDs are synthetic: never turn report IDs into source pins.
  letter_fixture::Fixture fixture;
  std::vector<LetterRequest> requests;
  for (const auto &key : {fixture.english,fixture.french,fixture.dutch,fixture.dutch}) {
    requests.push_back(fixture.request(key,requests.size(),warm::Exposure::Aggregate));
    requests.push_back(fixture.request(key,requests.size(),warm::Exposure::Parts));
  }
  auto legacy = ActiveAssembly::assemble(normalize_letters(fixture.base,requests),warm::new_uuid(),false)
      .focus({{GroupKind::SiblingDefinition,InputText(warm::key(fixture.english))}});
  ActiveSession session(settings);
  check(!session.on_cuda(),"handoff evidence requires actual CPU execution");
  field::Parameters parameters;
  parameters.dt = parameters.gravity = parameters.contact_stiffness = parameters.reach_strength = 0;
  handoff_trace::Calls calls;
  assembly::ExecutionHandle handle;
  {
    handoff_trace::Scope scope(calls);
    handle = session.submit(legacy,parameters,path);
  }
  auto report = session.report(handle,legacy.owner());
  using Construction = std::pair<std::string,std::uint64_t>;
  std::map<std::string,Construction> saved, actual;
  std::map<std::string,std::string> notation, definitions;
  for (const auto &tool : fixture.base.tools)
    if (tool.notation) notation.emplace(warm::key(tool.ref.object),std::string(*tool.notation));
  for (const auto &row : retained.at("particles"))
    saved.emplace(suffix(row.at("id").get<std::string>()),
        Construction{row.at("label").get<std::string>(),std::stoull(row.at("exact_mass").get<std::string>())});
  for (const auto &row : report.prepared.particles) {
    const auto id = suffix(std::string(row.occurrence.id));
    const auto definition = std::string(row.occurrence.definition);
    definitions.emplace(id,definition);
    actual.emplace(id,Construction{notation.at(definition),row.exact_mass});
  }
  check(saved.size() == 16 && saved == actual,"fixture path/notation/mass differs from committed retained study");
  bool synthetic_bytes = false;
  for (const auto &row : retained.at("particles")) {
    const auto &definition = definitions.at(suffix(row.at("id").get<std::string>()));
    if (row.at("exposure") == "aggregate")
      check(definition == row.at("definition"),"retained letter definition identity differs");
    else synthetic_bytes |= definition != row.at("definition").get<std::string>();
  }
  check(synthetic_bytes,"fixture byte IDs were mistaken for reacquired retained source pins");
  check(session.ready(handle) && !report.numerical_profile.wide() &&
            calls.standalone == 0 && calls.combined == 0,"legacy session unexpectedly installed C3");
  const auto observed = viewport::observe(session,legacy.owner(),handle,viewport::IntentRequest{1,UINT64_MAX});
  check(observed.intent.at("status") == "unavailable" && observed.comparisons.empty() &&
            !observed.prepared_generation,"flat retained diagnostic claimed prepared C3 state");

  std::vector<StructuralLetterRequest> structural_requests;
  for (const auto &request : requests)
    structural_requests.push_back({request.entry,request.letter,request.exposure});
  // Same explicit focus choice as the retained capture adapter, expressed at
  // v2's required source boundary rather than silently inferred by a session.
  auto definitions_source = fixture.base;
  definitions_source.focal_fields = {{warm::FieldKind::SiblingDefinition,fixture.english}};
  auto structural = ActiveAssembly::assemble(
      normalize_structural_letters(std::move(definitions_source),structural_requests),legacy.owner(),false);
  rejected([&] { session.submit(structural,parameters,path); },"legacy session silently accepted structural v2");
  check(session.ready(handle) && session.completed_ticks(handle) == 0,
        "unsupported v2 submit displaced the existing flat execution");
  session.tick(handle,parameters);
  check(session.ready(handle) && session.completed_ticks(handle) == 1,
        "flat execution lost healthy use after refused v2 handoff");
  std::cout << "retained_entry,artifact_study=" << retained.at("capture").at("study").get<std::string>()
            << ",construction_rows=16,source=fixture_construction_reproduction,db_reacquired=0,byte_pins=fixture_only"
               ",flat_ready=1,c3_diagnostic=unavailable,v2_session_submit=refused"
               ",standalone_installs=0,combined_commits=0,following_flat_tick=1 PASS\n";
}
void prepared_entry(const engine::InstanceSettings &settings, field::HostPath path, bool parts) {
  engine::Runtime runtime(settings);
  Input input(parts); input.install_layout(runtime);
  field::Harness h(runtime,capacity(),path);
  handoff_trace::Calls positive;
  {
    handoff_trace::Scope scope(positive);
    const auto refusal = h.install_live_hierarchy(h.prepared_storage().generation + 1,{});
    check(refusal.status == field::CascadeStatus::Stale,"standalone counter control changed status");
  }
  check(positive.standalone == 1 && positive.combined == 0 && h.ready(),"standalone linker counter missed real entry");
  auto d = domain();
  auto prepared = prepare(h,input,d);
  check(prepared.status == AdmissionPlanStatus::Ok && !prepared.owner->ready(),"prepared metadata published native state");
  auto &owner = *prepared.owner;
  const auto authorization = input.source->owner();
  const auto handle = owner.handle();
  std::weak_ptr<const ActiveAssembly> pin = input.source;
  input.source.reset();
  check(!pin.expired(),"owner lost immutable source pin");
  handoff_trace::Calls calls;
  {
    handoff_trace::Scope scope(calls);
    const auto refusal = no_new([&] { return owner.bootstrap(authorization,handle,0,UINT64_MAX,UINT64_MAX); });
    check(refusal.status == field::UpdateStatus::CapacityExhausted && h.ready() &&
              !h.live_hierarchy_installed() && calls.combined == 0 && calls.standalone == 0,
          "placement refusal reached hierarchy publication");
    const auto result = no_new([&] { return owner.bootstrap(authorization,handle,UINT64_MAX,UINT64_MAX,UINT64_MAX); });
    check(result.status == field::UpdateStatus::Ok && owner.ready() && h.live_hierarchy_installed() &&
              owner.generation() == h.prepared_storage().generation && calls.combined == 1 && calls.standalone == 0,
          "prepared source did not reach the existing combined C3 publication");
  }
  {
    const auto view = owner.associations(authorization,handle);
    const auto before = viewport::observe(owner,authorization,handle,viewport::IntentRequest{1,UINT64_MAX});
    check(before.report.numerical_profile.wide() && before.body_records.size() == 2 &&
              before.comparisons.size() == (parts ? 3u : 2u) && before.prepared_generation == owner.generation() &&
              before.intent.at("status") == "ok","published prepared observation lacks native C3 result");
    const auto &claims = owner.bodies(authorization,handle);
    for (std::size_t row = 0; row < claims.size(); ++row)
      check(field::detail::same_handle(claims[row],before.body_records[row]),"report substituted a source ordinal for a committed claim");
    rejected([&] { viewport::observe(owner,warm::new_uuid(),handle); },"foreign owner obtained completed C3 state");
    auto stale = handle; stale.generation = warm::new_uuid();
    rejected([&] { viewport::observe(owner,authorization,stale); },"stale logical handle obtained completed C3 state");
    field::Parameters64 parameters; parameters.dt = parameters.gravity = 0;
    const auto tick = no_new([&] { return owner.tick(authorization,handle,parameters); });
    check(tick.status == field::TickStatus::Ok && owner.ready(),"existing prepared tick failed");
    rejected([&] { view.read(); },"pre-tick association rows remained current");
    const auto after = viewport::observe(owner,authorization,handle,viewport::IntentRequest{1,UINT64_MAX});
    check(after.tick == 1 && after.intent.at("status") == "ok" && after.prepared_generation == owner.generation(),
          "completed prepared observation lost tick/generation currentness");
    check(before.tick == 0,"detached observation became mutable current state");
  }
  prepared.owner.reset();
  check(pin.expired() && h.ready() && h.live_hierarchy_installed(),"borrowed owner teardown stole native state or retained its source");
  std::cout << "prepared_entry,exposure=" << (parts ? "parts" : "aggregate")
            << ",source=bounded_fixture,starts_supplied=0,standalone_counter_control=1"
               ",standalone_installs=0,combined_commits=1,claims=2,source_pin=owned"
               ",prepared_tick=1,diagnostic=ok,authorization_currentness=1,owner_boundary_new=0 PASS\n";
}
}
int main(int argc, char **argv) {
  try {
    if (argc != 4) throw std::runtime_error("workers path retained-snapshot required");
    engine::InstanceSettings settings;
    settings.read_environment = false;
    settings.cpu_threads = std::stoi(argv[1]);
    const auto path = std::string(argv[2]) == "direct" ? field::HostPath::CpuDirect : field::HostPath::BulkCopy;
    std::ifstream file(argv[3]); viewport::Json retained; file >> retained;
    check(retained.at("source").at("kind") == "retained_cold_to_warm_to_active" &&
              retained.at("capture").at("tick") == "0","wrong committed retained-study artifact");
    retained_entry(settings,path,retained);
    prepared_entry(settings,path,false);
    prepared_entry(settings,path,true);
    std::cout << "retained_c3_handoff,backend=cpu,workers=" << *settings.cpu_threads
              << ",path=" << argv[2] << ",production_changed=0,retained_numeric_replay=0 PASS\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "retained C3 handoff: " << e.what() << '\n';
    return 1;
  }
}
