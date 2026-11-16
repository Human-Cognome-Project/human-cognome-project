#include "snapshot.h"
#include "prepared_owner_fixture.h"
#define HCP_ALLOCATION_LIFETIME
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
const allocation_lifetime::Entry *tracked(const void *pointer) {
  if (!pointer) return nullptr;
  auto *p = const_cast<void *>(pointer);
  for (std::size_t n=0, i=allocation_lifetime::bucket(p); n<65536; ++n, i=(i+1)%65536) {
    const auto &entry = allocation_lifetime::table[i];
    if (!entry.used) return nullptr;
    if (entry.pointer == p) return &entry;
  }
  return nullptr;
}
void origin_association_cost(PreparedStudyOwner &owner, const warm::Uuid &authorization,
                             const ExecutionHandle &handle) {
  const auto view = owner.associations(authorization,handle);
  const auto rows = view.read();
  const auto *plan = &rows.plan;
  std::vector<Origin> origins;
  allocation_lifetime::begin();
  allocation_observation::calls = allocation_observation::bytes = 0;
  allocation_observation::enabled = true;
  try { origins = rows.source.origins(); }
  catch (...) {
    allocation_observation::enabled = false;
    origins.clear(); std::vector<Origin>().swap(origins);
    allocation_lifetime::end(); throw;
  }
  allocation_observation::enabled = false;
  const auto calls = allocation_observation::calls.load(), requested = allocation_observation::bytes.load();
  const auto count = origins.size(), capacity = origins.capacity();
  const auto retained = allocation_lifetime::counts;
  std::size_t duplicate_bytes = 0, pin_bytes = 0, duplicate_blocks = 0;
  bool exact = true, distinct = true, attributed = true;
  // Inspect actual producer-owned requests, rather than estimating a key copy.
  // This fixture has heap-backed UUID text and nonempty full addresses.
  for (const auto &origin : origins) {
    if (!origin.shared || origin.local) { exact = false; continue; }
    const auto &pin = origin.shared->object;
    exact &= origin.definition == pin;
    const auto *text = tracked(origin.definition.scope.str().data());
    const auto *address = tracked(origin.definition.address.data());
    const auto *pin_text = tracked(pin.scope.str().data());
    const auto *pin_address = tracked(pin.address.data());
    attributed &= text && address && pin_text && pin_address;
    distinct &= origin.definition.scope.str().data() != pin.scope.str().data() &&
                origin.definition.address.data() != pin.address.data();
    if (text && address) { duplicate_bytes += text->bytes + address->bytes; duplicate_blocks += 2; }
    if (pin_text && pin_address) pin_bytes += pin_text->bytes + pin_address->bytes;
  }
  for (std::size_t i=0; i<rows.bodies.size(); ++i) {
    const auto body = rows.body(i);
    const auto found = std::find_if(origins.begin(),origins.end(),
        [&](const auto &v) { return v.occurrence == body.source.occurrence; });
    exact &= found != origins.end() && found->shared && *found->shared == body.source.definition;
  }
  const auto *outer = tracked(origins.data());
  attributed &= outer && outer->bytes == capacity*sizeof(Origin);
  const auto outer_bytes = outer ? outer->bytes : 0;
  // Borrowing the existing pin member is a comparison, not a new production
  // accessor or a removal of Origin::definition.
  allocation_observation::calls = allocation_observation::bytes = 0;
  allocation_observation::enabled = true;
  for (const auto &origin : origins)
    if (origin.shared) exact &= origin.definition == origin.shared->object;
  allocation_observation::enabled = false;
  const auto borrow_calls = allocation_observation::calls.load(), borrow_bytes = allocation_observation::bytes.load();
  origins.clear();
  const auto after_clear = allocation_lifetime::counts;
  std::vector<Origin>().swap(origins);
  const auto released = allocation_lifetime::end();
  check(exact && distinct && attributed,"origin definition and exact pin own equal keys with distinct tracked backing");
  check(count == 4 && rows.bodies.size() == 2 && duplicate_blocks == 2*count &&
        duplicate_bytes && duplicate_bytes == pin_bytes,
        "duplicate cost covers all source alternatives independently of exposed bodies");
  check(retained.acquired == calls && after_clear.live == outer_bytes && !released.live && !released.blocks &&
        released.acquired == released.released,
        "row destruction releases both key copies while only vector capacity remains");
  check(!borrow_calls && !borrow_bytes && &view.read().plan == plan,
        "pin-member comparison allocates nothing and preserves checked execution associations");
  std::cout << "origin_association_cost,rows=" << count << ",exposed=" << rows.bodies.size()
            << ",construction_calls=" << calls << ",construction_requested=" << requested
            << ",retained_requested=" << retained.live << ",peak_requested=" << retained.peak
            << ",duplicate_blocks=" << duplicate_blocks << ",duplicate_requested=" << duplicate_bytes
            << ",pin_object_requested=" << pin_bytes << ",key_object_bytes=" << sizeof(warm::ObjectKey)
            << ",origin_object_bytes=" << sizeof(Origin) << ",outer_capacity_bytes=" << outer_bytes
            << ",after_clear_requested=" << after_clear.live << ",after_destroy_requested=" << released.live
            << ",borrow_calls=" << borrow_calls << ",borrow_requested=" << borrow_bytes << '\n';
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
    check(a.occurrence == b.occurrence && a.definition == b.definition && a.shared == b.shared &&
          !a.local && !b.local,"consuming report retains exact detached source pins");
  }
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
  origin_association_cost(owner,authorization,handle);
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
