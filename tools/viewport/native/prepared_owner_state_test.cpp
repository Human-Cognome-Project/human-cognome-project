#include "snapshot.h"
#include "prepared_owner_fixture.h"
#include "allocation_observation.h"
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
struct Measured {
  viewport::CompletedState state;
  std::uint64_t calls{}, bytes{};
};
template<class F> Measured measured(F action) {
  allocation_observation::calls = allocation_observation::bytes = 0;
  allocation_observation::enabled = true;
  try {
    auto state = action();
    allocation_observation::enabled = false;
    return {std::move(state),allocation_observation::calls.load(),allocation_observation::bytes.load()};
  } catch (...) { allocation_observation::enabled = false; throw; }
}
void transfer_equivalence(field::Harness &h, const viewport::CompletedState &completed,
                          const viewport::CaptureContext &context) {
  // Reconstitute only identity/scale metadata, as in the production binding.
  // Numerical state is always obtained again by the real native observer.
  auto metadata = completed.report;
  metadata.particles.clear(); metadata.groups.clear(); metadata.determined.clear();
  metadata.particles64.clear(); metadata.groups64.clear(); metadata.determined64.clear();
  auto retained = viewport::bind(h,std::move(metadata));
  auto consumed = retained; // Fixture preparation is outside the measurements.
  const auto request = viewport::IntentRequest{1,UINT64_MAX};
  const auto copied = measured([&] { return viewport::observe(h,retained,request); });
  const auto moved = measured([&] { return viewport::observe(h,std::move(consumed),request); });
  std::cout << "observer_metadata_transfer,tick=" << completed.tick
            << ",copy_calls=" << copied.calls << ",copy_bytes=" << copied.bytes
            << ",move_calls=" << moved.calls << ",move_bytes=" << moved.bytes << '\n';
  check(copied.calls > moved.calls && copied.bytes > moved.bytes,
        "consuming observer avoids measured detached metadata copies");
  check(viewport::snapshot(copied.state,context) == viewport::snapshot(moved.state,context) &&
        viewport::snapshot(moved.state,context) == viewport::snapshot(completed,context),
        "retained and consuming bindings produce exactly the same complete snapshot");
  check(copied.state.numeric_copy_bytes == moved.state.numeric_copy_bytes &&
        copied.state.requested_readback_bytes == moved.state.requested_readback_bytes &&
        retained.mapping.origins.size() == completed.report.origins.size(),
        "transfer savings preserve native readback/numerical copy scope and retained binding");
  for (std::size_t i=0; i<completed.report.origins.size(); ++i) {
    const auto &a = moved.state.report.origins[i], &b = completed.report.origins[i];
    check(a.occurrence == b.occurrence && a.definition() == b.definition() && a.shared == b.shared &&
          !a.local && !b.local,"consuming report retains exact detached source pins");
  }
  auto response_consumed=retained;
  const auto both_copied=viewport::observe(h,retained,request,viewport::ResponseRequest{UINT64_MAX});
  const auto both_moved=viewport::observe(h,std::move(response_consumed),request,viewport::ResponseRequest{UINT64_MAX});
  check(viewport::snapshot(both_copied,context) == viewport::snapshot(both_moved,context) &&
        both_copied.intent == completed.intent && both_copied.response_trace.at("status") == "ok",
        "both binding overloads copy response operands before later intent inspection invalidates the borrowed native view");
  // Both overloads must reject invalid/stale mappings before a completed read.
  auto stale = retained; ++stale.generation;
  for (bool consume : {false,true}) {
    bool refused = false;
    try {
      if (consume) viewport::observe(h,std::move(stale),request);
      else viewport::observe(h,stale,request);
    } catch (const std::invalid_argument &) { refused = true; }
    check(refused,"both observer overloads reject the same stale generation");
  }
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
    const auto associations = owner.associations(authorization,handle).read();
    for (std::size_t i=0; i<map.size(); ++i) {
      const auto &actual = observed.body_records[i];
      check(actual.context == map[i].context && actual.incarnation == map[i].incarnation &&
            actual.address == map[i].address && actual.record == map[i].record &&
            observed.report.prepared.particles[i].occurrence.slot == actual.record &&
            std::string(observed.report.prepared.particles[i].occurrence.id) ==
                occurrence_text(associations.source.study(),associations.body(i).source.occurrence),
            "completed report uses committed occurrence/full-handle associations");
    }
    for (const auto &scale : observed.report.prepared.scales)
      check(!scale.node.layout,"report metadata retains no native layout pointer");
    const auto context = viewport::context(*input.source,handle,tick,"private prepared owner");
    const auto snapshot = viewport::snapshot(observed,context);
    check(snapshot.at("capture").at("tick") == std::to_string(tick) &&
          snapshot.at("particles").size() == 2 && snapshot.at("numerics").at("profile") == "trial_f64",
          "existing snapshot serialization consumes the completed owner state");
    const auto native = h.inspect_live_responses(UINT64_MAX); // Test-owned copy.
    check(native.status == field::CascadeStatus::Ok,"native response reference available");
    const auto traced = viewport::observe(owner,authorization,handle,std::nullopt,
        viewport::ResponseRequest{native.work_required});
    const auto &trace = traced.response_trace;
    check(traced.intent.is_null() && omitted.response_trace.is_null() &&
          trace.at("status") == "ok" && trace.at("count") == std::to_string(native.responses) &&
          trace.at("capacity") == std::to_string(native.capacity) &&
          trace.at("work_used") == std::to_string(native.work_required) &&
          trace.at("readback_bytes") == std::to_string(native.readback_bytes),
          "response-only exact-budget observation is independent of intent");
    const auto hierarchy = h.inspect_live_hierarchy(); // Also test-owned.
    check(trace.at("packets").size() == hierarchy.node_count,
          "response packet table covers every actual live comparison");
    for (const auto &packet : trace.at("packets")) {
      const int slot=packet.at("slot");
      check(packet.at("comparison") == std::to_string(hierarchy.nodes.at(slot).comparison),
            "response packet slot resolves to actual native comparison identity");
    }
    bool local=false, inherited=false;
    for (std::size_t row=0; row<native.responses; ++row) {
      const auto &copied=trace.at("rows").at(row);
      const auto value=[&](int col) { return native.values[col*native.capacity+row]; };
      const auto identity=[&](int col) { return native.identities[col*native.capacity+row]; };
      const bool is_local=identity(3) == 0;
      local |= is_local; inherited |= !is_local;
      check(copied.at("body").at("record") == identity(0) &&
            copied.at("child") == identity(1) && copied.at("parent") == identity(2) &&
            copied.at("kind") == (is_local ? "local" : "inherited") &&
            copied.at("numerator") == std::to_string(native.fractions[row]) &&
            copied.at("denominator") == std::to_string(native.fractions[native.capacity+row]),
            "response preserves actual native row order/path and exact participating ratio");
      bool full_body=false;
      for (const auto &body : traced.body_records)
        if (body.record == identity(0)) full_body = copied.at("body").at("context") == std::to_string(body.context) &&
            copied.at("body").at("incarnation") == std::to_string(body.incarnation) &&
            copied.at("body").at("address") == body.address;
      check(full_body,"response body uses the committed full claim, including sparse records");
      for (int k=0; k<3; ++k)
        check(copied.at("delta").at(k) == value(k) && copied.at("recipient").at(k) == value(3+k),
              "response coordinates are copied without a second solve");
      check(copied.at("body_mass") == value(6) && copied.at("source_mass") == value(7) &&
            copied.at("production_eligible") == (value(8) == 1) &&
            copied.at("extraction_valid") == (value(9) == 1),"response masses and distinct native flags survive");
    }
    check(local && inherited,"fixture exposes both native response path kinds");
    check(traced.report.particles64 == omitted.report.particles64 &&
          traced.report.groups64 == omitted.report.groups64 && traced.tick == tick &&
          traced.prepared_generation == omitted.prepared_generation &&
          traced.requested_readback_bytes == omitted.requested_readback_bytes+native.readback_bytes &&
          traced.numeric_copy_bytes == omitted.numeric_copy_bytes+8*native.responses*sizeof(double),
          "response observation adds only declared copy/readback scope and preserves state");
    check(viewport::snapshot(traced,context).at("response_trace") == trace,
          "completed snapshot retains the response payload");
    for (const auto budget : {std::uint64_t(0),native.work_required-1}) {
      const auto refused_trace=viewport::observe(owner,authorization,handle,std::nullopt,
          viewport::ResponseRequest{budget});
      const auto &q=refused_trace.response_trace;
      check(q.at("status") == "budget_refused" && q.at("rows").empty() && q.at("packets").empty() &&
            q.at("work_used") == "0" && q.at("readback_bytes") == "0" &&
            refused_trace.report.particles64 == traced.report.particles64 && owner.ready(),
            "zero/one-short response request refuses without stale payload or state change");
    }
    auto stale_trace=traced;
    stale_trace.response_trace["tick"]=std::to_string(tick+1);
    bool stale_refused=false;
    try { viewport::snapshot(stale_trace,context); } catch (const std::invalid_argument &) { stale_refused=true; }
    check(stale_refused,"snapshot rejects response data from another tick");
    transfer_equivalence(h,observed,context);
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
  const auto generation = owner.generation();
  p.gravity = 0.025;
  const auto moving = owner.tick(authorization,handle,p);
  check(moving.status == field::TickStatus::Ok && moving.coverage_changed &&
        owner.generation() == generation+1 && owner.ready(),"native contact publication updates the owner's completed generation");
  verify(2);
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
