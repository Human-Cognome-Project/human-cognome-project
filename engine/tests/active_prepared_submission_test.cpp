#include "prepared_owner_fixture.h"
#include "assembly/prepared_submission.h"
#include "snapshot.h"
#include "hierarchy_entry_trace.h"
#include "field/cascade_source.h"
#include <cstring>
#include <iostream>
#include <map>

namespace {
using namespace prepared_owner_fixture;
void check(bool ok, const char *text) { if (!ok) throw std::runtime_error(text); }
template<class F> void rejects(F f, const char *text) {
  bool refused = false;
  try { f(); } catch (const warm::Error &) { refused = true; }
  check(refused,text);
}
template<class V> bool bits(const V &a, const V &b) {
  return a.size() == b.size() && !std::memcmp(a.data(),b.data(),a.size()*sizeof(a[0]));
}
bool same_claims(const std::vector<engine::RecordHandle> &a,
                 const std::vector<engine::RecordHandle> &b) {
  if (a.size() != b.size()) return false;
  for (std::size_t i=0; i<a.size(); ++i)
    if (!field::detail::same_handle(a[i],b[i])) return false;
  return true;
}
PreparedStudySubmission request(const Input &input, field::HostPath path) {
  PreparedStudySubmission out{input.source,input.bindings,capacity(),domain()};
  out.path = path;
  for (auto &v : out.bindings.layout) v.node.layout = nullptr;
  return out;
}
PreparedSubmissionResult submit(ActiveSession &session, PreparedStudySubmission input,
                               const warm::Uuid &owner, handoff_trace::Calls &calls) {
  handoff_trace::Scope scope(calls);
  return session.submit_prepared(std::move(input),owner);
}
void claims(const viewport::CompletedState &state, const PreparedStudySubmission &request) {
  std::map<std::string,int> expected;
  for (const auto &body : request.bindings.bodies)
    expected.emplace(occurrence_text(request.bindings.source.study,body.occurrence.source),body.address);
  check(state.body_records.size() == expected.size(),"incomplete committed claim count");
  for (std::size_t i=0; i<state.body_records.size(); ++i) {
    const auto &body = state.body_records[i];
    const auto &metadata = state.report.prepared.particles.at(i).occurrence;
    check(body.context && body.incarnation && body.record == metadata.slot &&
              body.address == expected.at(std::string(metadata.id)),
          "submission report does not expose actual native claims for source occurrences");
  }
  for (std::size_t i=0; i<state.report.prepared.scales.size(); ++i) {
    const auto &scale = state.report.prepared.scales[i];
    check(!scale.node.layout && scale.units == request.bindings.layout[i].units,
          "report leaked a native layout or changed caller units");
  }
}
void run(const engine::InstanceSettings &settings, field::HostPath path) {
  std::weak_ptr<const ActiveAssembly> aggregate_pin, parts_pin;
  {
    ActiveSession session(settings);
    check(!session.on_cuda(),"actual CPU backend required");
    letter_fixture::Fixture fixture;
    auto legacy_source = ActiveAssembly::assemble(normalize_letters(fixture.base,
        {fixture.request(fixture.english,0,warm::Exposure::Aggregate)}),warm::new_uuid(),false);
    field::Parameters legacy_parameters;
    legacy_parameters.dt = legacy_parameters.gravity = legacy_parameters.contact_stiffness = legacy_parameters.reach_strength = 0;
    const auto legacy = session.submit(legacy_source,legacy_parameters,path);
    const auto legacy_before = session.report(legacy,legacy_source.owner());
    const auto legacy_healthy = [&] {
      const auto current = session.report(legacy,legacy_source.owner());
      check(session.ready(legacy) && session.completed_ticks(legacy) == 0 &&
                bits(current.particles,legacy_before.particles) && bits(current.groups,legacy_before.groups) &&
                current.frame_anchor == legacy_before.frame_anchor,
            "failed prepared submission changed a prior legacy execution");
    };
    Input aggregate;
    auto aggregate_request = request(aggregate,path);
    const auto authorization = aggregate.source->owner();
    aggregate_pin = aggregate.source;
    handoff_trace::Calls calls;
    rejects([&] { session.submit_prepared(aggregate_request,warm::new_uuid()); },"wrong owner submitted a prepared source");
    auto invalid = aggregate_request;
    invalid.bindings.layout[0].node.layout = aggregate.bindings.layout[0].node.layout;
    auto result = submit(session,std::move(invalid),authorization,calls);
    check(!result.handle && result.preparation_status == AdmissionPlanStatus::Unsupported &&
              !calls.standalone && !calls.combined,"caller native pointer entered the session layout");
    invalid = aggregate_request; invalid.source = std::make_shared<const ActiveAssembly>(legacy_source);
    result = submit(session,std::move(invalid),legacy_source.owner(),calls);
    check(!result.handle && result.preparation_status == AdmissionPlanStatus::Unsupported,
          "prepared entry silently accepted legacy source");
    invalid = aggregate_request;
    auto insufficient = invalid.capacity.limits();
    insufficient[field::Budget::ScratchBytes] = 0;
    invalid.capacity = field::PreparedCapacity(insufficient,invalid.capacity.profile());
    result = submit(session,std::move(invalid),authorization,calls);
    check(!result.handle && result.preparation_status == AdmissionPlanStatus::CapacityExhausted &&
              result.failure.budget == field::Budget::ScratchBytes && !calls.combined && !calls.standalone,
          "insufficient native scratch reached preparation/publication");
    legacy_healthy();

    auto short_request = aggregate_request;
    short_request.placement_work = 0;
    result = submit(session,std::move(short_request),authorization,calls);
    check(!result.handle && result.preparation_status == AdmissionPlanStatus::Ok &&
              result.publication.status == field::UpdateStatus::CapacityExhausted &&
              result.failure.budget == field::Budget::PlacementWork &&
              aggregate_request.resources->result().live_bytes == 0 && !calls.combined && !calls.standalone,
          "work refusal published a handle, retained owner metadata or reached native hierarchy commit");
    legacy_healthy();
    result = submit(session,aggregate_request,authorization,calls);
    check(result.handle && session.ready(*result.handle) && calls.combined == 1 && !calls.standalone,
          "retry did not publish exactly one existing combined C3 bootstrap");
    const auto aggregate_handle = *result.handle;
    const auto aggregate_before = viewport::observe(session,authorization,aggregate_handle,viewport::IntentRequest{1,UINT64_MAX});
    claims(aggregate_before,aggregate_request);
    check(aggregate_before.comparisons.size() == 2 && aggregate_before.intent.at("status") == "ok" &&
              aggregate_before.prepared_generation && aggregate_before.tick == 0,
          "new session entry failed to expose prepared observation");
    const auto report = session.report(aggregate_handle,authorization);
    check(report.numerical_profile.wide() && bits(report.particles64,aggregate_before.report.particles64) &&
              report.particles.empty(),"core prepared report differs from native viewport observation");
    session.accept(report,authorization);
    rejects([&] { session.report(aggregate_handle,warm::new_uuid()); },"foreign owner read prepared report");
    rejects([&] { viewport::observe(session,warm::new_uuid(),aggregate_handle); },"foreign owner read prepared observation");
    auto stale = aggregate_handle; stale.generation = warm::new_uuid();
    rejects([&] { session.report(stale,authorization); },"stale logical generation read prepared report");
    stale = aggregate_handle; stale.session = warm::new_uuid();
    rejects([&] { session.report(stale,authorization); },"foreign session read prepared report");
    rejects([&] { session.tick(aggregate_handle,legacy_parameters); },"legacy tick implicitly converted prepared numerics");
    rejects([&] { session.refresh(aggregate_handle,legacy_source,legacy_parameters); },"legacy refresh mutated a prepared execution");

    Input parts(true);
    auto parts_request = request(parts,path);
    const auto parts_authorization = parts.source->owner();
    parts_pin = parts.source;
    auto short_plan = parts_request; short_plan.plan_work = 0;
    result = submit(session,std::move(short_plan),parts_authorization,calls);
    check(!result.handle && result.preparation_status == AdmissionPlanStatus::WorkExhausted &&
              parts_request.resources->result().live_bytes == 0 && calls.combined == 1 && !calls.standalone,
          "plan refusal exposed a new execution or retained plan resources");
    const auto aggregate_after_refusal = viewport::observe(session,authorization,aggregate_handle,viewport::IntentRequest{1,UINT64_MAX});
    check(session.ready(aggregate_handle) && session.completed_ticks(aggregate_handle) == 0 &&
              same_claims(aggregate_after_refusal.body_records,aggregate_before.body_records) &&
              bits(aggregate_after_refusal.report.particles64,aggregate_before.report.particles64) &&
              bits(aggregate_after_refusal.report.groups64,aggregate_before.report.groups64),
          "failed new submission changed prior prepared claims/numerics");
    legacy_healthy();
    result = submit(session,parts_request,parts_authorization,calls);
    check(result.handle && session.ready(*result.handle) && calls.combined == 2 && !calls.standalone,
          "parts submission did not reuse existing bootstrap authority");
    const auto parts_handle = *result.handle;
    const auto parts_before = viewport::observe(session,parts_authorization,parts_handle,viewport::IntentRequest{1,UINT64_MAX});
    claims(parts_before,parts_request);
    check(parts_before.comparisons.size() == 3,"parts topology changed from caller choices");
    const auto duplicate = submit(session,parts_request,parts_authorization,calls);
    check(!duplicate.handle && session.ready(parts_handle) && calls.combined == 2,
          "duplicate study replaced its current execution");
    aggregate.source.reset(); aggregate_request.source.reset();
    parts.source.reset(); parts_request.source.reset();
    check(!aggregate_pin.expired() && !parts_pin.expired(),"published session lost pinned source lifetime");
    field::Parameters64 parameters; parameters.dt = parameters.gravity = 0;
    const auto refused_tick = session.tick_prepared(aggregate_handle,authorization,parameters,0);
    check(refused_tick.status == field::TickStatus::CapacityExhausted && session.ready(aggregate_handle) &&
              session.completed_ticks(aggregate_handle) == 0,"tick work refusal changed prepared currentness");
    for (const auto &entry : {std::pair{aggregate_handle,authorization},std::pair{parts_handle,parts_authorization}}) {
      const auto tick = session.tick_prepared(entry.first,entry.second,parameters);
      const auto after = viewport::observe(session,entry.second,entry.first,viewport::IntentRequest{1,UINT64_MAX});
      check(tick.status == field::TickStatus::Ok && session.ready(entry.first) &&
                session.completed_ticks(entry.first) == 1 && after.tick == 1 &&
                after.prepared_generation == tick.generation && after.intent.at("status") == "ok",
            "prepared session did not follow native completed tick/generation");
    }
    check(aggregate_before.tick == 0 && parts_before.tick == 0,"detached report became mutable execution state");
    session.tick(legacy,legacy_parameters);
    check(session.ready(legacy) && session.completed_ticks(legacy) == 1,
          "prepared submissions broke legacy following tick");
    std::cout << "active_prepared_submission,combined_commits=" << calls.combined
              << ",standalone_installs=" << calls.standalone
              << ",exposures=2,plan_refusal_retry=1,placement_refusal_retry=1,prior_legacy_healthy=1"
                 ",prior_prepared_healthy=1,actual_claims=1,source_pins=owned,following_ticks=3 PASS\n";
  }
  check(aggregate_pin.expired() && parts_pin.expired(),"session teardown retained source pins");
  std::cout << "prepared_submission_lifetime,owner_before_harness_before_runtime=1,source_pins_released=1 PASS\n";
}
}
int main(int argc, char **argv) {
  try {
    if (argc != 3) throw std::runtime_error("workers path required");
    engine::InstanceSettings settings; settings.read_environment = false;
    settings.cpu_threads = std::stoi(argv[1]); settings.snode_capacity = 2048;
    const auto path = std::string(argv[2]) == "direct" ? field::HostPath::CpuDirect : field::HostPath::BulkCopy;
    run(settings,path);
    std::cout << "prepared_submission_case,actual_backend=cpu,workers=" << *settings.cpu_threads << ",path=" << argv[2] << " PASS\n";
    return 0;
  } catch (const std::exception &e) { std::cerr << "active prepared submission: " << e.what() << '\n'; return 1; }
}
