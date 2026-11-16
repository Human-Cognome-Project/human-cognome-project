#include "snapshot.h"
#include "prepared_owner_fixture.h"
#include <iostream>

namespace {
using namespace prepared_owner_fixture;
int checks = 0;
void check(bool ok, const char *label) {
  if (!ok) throw std::runtime_error(label);
  ++checks;
}
template<class F> void rejects(F action, const char *label) {
  bool rejected = false;
  try { action(); } catch (const warm::Error &) { rejected = true; }
  check(rejected,label);
}
void run(engine::Runtime &runtime, field::HostPath path) {
  Input input(true); input.install_layout(runtime);
  field::Harness h(runtime,capacity(),path);
  auto d = domain(); auto result = prepare(h,input,d);
  check(result.status == AdmissionPlanStatus::Ok,"observer owner preparation");
  auto &owner = *result.owner;
  const auto handle = owner.handle(); const auto authorization = input.source->owner();
  rejects([&] { viewport::observe(owner,authorization,handle); },"unpublished owner observation is unavailable");
  const auto committed = owner.bootstrap(authorization,handle,UINT64_MAX,UINT64_MAX,UINT64_MAX);
  check(committed.status == field::UpdateStatus::Ok,"observer uses production generated bootstrap");
  const auto verify = [&](std::uint64_t tick) {
    const auto omitted = viewport::observe(owner,authorization,handle);
    const auto observed = viewport::observe(owner,authorization,handle,viewport::IntentRequest{1,UINT64_MAX});
    check(observed.backend == "native_cpu" && observed.tick == tick &&
          observed.prepared_generation == owner.generation() &&
          observed.body_records.size() == 2 && observed.comparisons.size() == 3,
          "completed state carries actual backend/tick/generation/comparison identity");
    check(observed.intent.at("status") == "ok" && observed.intent.at("tick") == std::to_string(tick) &&
          observed.intent.at("generation") == std::to_string(owner.generation()) &&
          omitted.intent.is_null(),"optional intent uses the completed native hierarchy");
    check(omitted.report.particles64 == observed.report.particles64 &&
          omitted.report.groups64 == observed.report.groups64 && owner.ready() &&
          h.prepared_storage().completed_ticks == tick,"observation preserves physical state and completed boundary");
    const auto &map = owner.bodies(authorization,handle);
    const auto &plan = owner.plan(authorization,handle);
    for (std::size_t i=0; i<map.size(); ++i) {
      const auto &actual = observed.body_records[i];
      check(actual.context == map[i].context && actual.incarnation == map[i].incarnation &&
            actual.address == map[i].address && actual.record == map[i].record &&
            observed.report.prepared.particles[i].occurrence.slot == actual.record &&
            std::string(observed.report.prepared.particles[i].occurrence.id) == occurrence_text(plan.occurrences()[i].occurrence),
            "completed report uses committed occurrence/full-handle associations");
    }
    for (const auto &scale : observed.report.prepared.scales)
      check(!scale.node.layout,"report metadata retains no native layout pointer");
    const auto context = viewport::context(*input.source,handle,tick,"private prepared owner");
    const auto snapshot = viewport::snapshot(observed,context);
    check(snapshot.at("capture").at("tick") == std::to_string(tick) &&
          snapshot.at("particles").size() == 2 && snapshot.at("numerics").at("profile") == "trial_f64",
          "existing snapshot serialization consumes the completed owner state");
    const auto refused = viewport::observe(owner,authorization,handle,viewport::IntentRequest{1,0});
    check(refused.intent.at("status") == "budget_refused" && owner.ready(),"diagnostic refusal remains read-only");
  };
  verify(0);
  auto stale = handle; stale.session = warm::new_uuid();
  rejects([&] { viewport::observe(owner,authorization,stale); },"foreign session cannot observe copied native state");
  rejects([&] { viewport::observe(owner,warm::new_uuid(),handle); },"wrong owner cannot observe private state");
  field::Parameters64 p; p.gravity = p.contact_stiffness = p.reach_strength = 0; p.dt = 0.125;
  check(owner.tick(authorization,handle,p).status == field::TickStatus::Ok,"observer follows an actual native tick");
  verify(1);
}
}
int main(int argc, char **argv) {
  try {
    const int workers = argc > 1 ? std::stoi(argv[1]) : 1;
    const auto path = argc > 2 && std::string(argv[2]) == "direct" ? field::HostPath::CpuDirect : field::HostPath::BulkCopy;
    engine::InstanceSettings settings; settings.read_environment = false; settings.cuda = false; settings.cpu_threads = workers;
    engine::Runtime runtime(settings);
    run(runtime,path);
    std::cout << "prepared_owner_observer,checks=" << checks << ",workers=" << workers << " PASS\n";
    return 0;
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
