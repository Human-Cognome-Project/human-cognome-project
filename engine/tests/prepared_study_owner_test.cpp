#include "prepared_owner_fixture.h"
#include "allocation_observation.h"
#include "prepared_fixture.h" // Failure injection/backing observation only.
#include "prepared_metrics.h"
#include <cstring>
#include <iostream>
#include <type_traits>

namespace {
using namespace prepared_owner_fixture;
using field::testing::PreparedFixtureAccess;
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
template<class F> auto no_new(F action) {
  allocation_observation::calls = 0;
  allocation_observation::enabled = true;
  try {
    auto result = action();
    allocation_observation::enabled = false;
    check(!allocation_observation::calls,"prepared operation allocated C++ storage");
    return result;
  } catch (...) { allocation_observation::enabled = false; throw; }
}
std::uint64_t state(field::Harness &h) {
  h.inspect_prepared();
  const auto &s = h.prepared_storage();
  std::uint64_t hash = 1469598103934665603ULL;
  const auto bytes = [&](const auto &value) {
    const auto *data = reinterpret_cast<const unsigned char *>(&value);
    for (std::size_t i=0; i<sizeof(value); ++i) { hash ^= data[i]; hash *= 1099511628211ULL; }
  };
  const auto rows = [&](const auto &values) { for (const auto &value : values) bytes(value); };
  const auto handles = [&](const auto &values) {
    for (const auto &v : values) { bytes(v.context); bytes(v.incarnation); bytes(v.address); bytes(v.record); }
  };
  bytes(s.generation); bytes(s.completed_ticks); bytes(s.populated);
  bytes(s.live.particles); bytes(s.live.fields); bytes(s.live.edges); bytes(s.live.contacts); bytes(s.live.focus);
  rows(h.particles64()); rows(h.groups64()); rows(h.determined64());
  rows(s.inspection_flags); rows(s.inspection_state); rows(s.motion64()); rows(s.inspection_focus64);
  rows(s.source_masses); rows(s.source_counts); handles(s.particle_headers); handles(s.field_headers); handles(s.focal);
  for (const auto &e : s.edges) {
    bytes(e.first.context); bytes(e.first.incarnation); bytes(e.first.address); bytes(e.first.record);
    bytes(e.second.context); bytes(e.second.incarnation); bytes(e.second.address); bytes(e.second.record);
    bytes(e.ordinal); bytes(e.share);
  }
  return hash;
}
void setup_refusals(field::Harness &h, const Input &input) {
  const auto before = state(h);
  auto full = domain(); auto successful = prepare(h,input,full);
  check(successful.status == AdmissionPlanStatus::Ok,"owner preparation witness");
  const auto metadata = successful.metadata_work.charged;
  const auto plan = successful.plan_work.charged;
  const auto bytes = full->result().peak_bytes, allocations = full->result().charged_work;
  successful.owner.reset();
  check(!full->result().live_bytes && before == state(h),"unpublished teardown preserves empty native execution");
  for (bool byte_limit : {true,false}) {
    std::uint64_t allowance = 0, ceiling = byte_limit ? bytes : allocations;
    while (allowance < ceiling) {
      auto d = domain({byte_limit ? allowance : UINT64_MAX,byte_limit ? UINT64_MAX : allowance});
      auto r = prepare(h,input,d);
      const auto next = byte_limit ? d->result().required_bytes : d->result().required_work;
      check(r.status == AdmissionPlanStatus::AllocationExhausted && !r.owner && !d->result().live_bytes &&
            next > allowance && next <= ceiling,"owner allocation refusal unwinds plan/pins/maps");
      allowance = next;
    }
    auto d = domain({byte_limit ? ceiling : UINT64_MAX,byte_limit ? UINT64_MAX : ceiling});
    auto r = prepare(h,input,d);
    check(r.status == AdmissionPlanStatus::Ok,"exact owner allocation bound");
    r.owner.reset(); check(!d->result().live_bytes,"exact-bound owner teardown");
  }
  std::uint64_t allowance = 0;
  while (allowance < metadata) {
    auto d = domain(); auto r = prepare(h,input,d,plan,allowance);
    check(r.status == AdmissionPlanStatus::WorkExhausted && !r.owner && !d->result().live_bytes &&
          r.metadata_work.next_required > allowance && r.metadata_work.next_required <= metadata,
          "every owner metadata work refusal unwinds before claims");
    allowance = r.metadata_work.next_required;
  }
  for (auto limit : {plan-1,plan}) {
    auto d = domain(); auto r = prepare(h,input,d,limit,metadata);
    check(r.status == (limit == plan ? AdmissionPlanStatus::Ok : AdmissionPlanStatus::WorkExhausted),
          "exact nested plan work and one-short refusal");
  }
  check(before == state(h) && h.ready(),"all setup refusals preserve native state for retry");
}
void lifecycle(engine::Runtime &runtime, field::HostPath path, bool parts) {
  Input input(parts); input.install_layout(runtime);
  field::Harness h(runtime,capacity(),path);
  if (!parts) setup_refusals(h,input);
  auto d = domain(); auto prepared = prepare(h,input,d);
  check(prepared.status == AdmissionPlanStatus::Ok,"prepared owner");
  auto &owner = *prepared.owner;
  const auto handle = owner.handle();
  const auto authorization = input.source->owner();
  auto competitor_domain = domain();
  auto competitor = prepare(h,input,competitor_domain);
  check(competitor.status == AdmissionPlanStatus::Ok,"second pending view for stale-generation control");
  check(!owner.ready() && owner.phase() == PreparedOwnerPhase::Prepared,"metadata preparation publishes no model");
  rejects([&] { owner.bodies(authorization,handle); },"unpublished maps unavailable");
  rejects([&] { owner.associations(authorization,handle); },"unpublished association view unavailable");
  const auto call = [&](std::uint64_t placement = UINT64_MAX, std::uint64_t hierarchy = UINT64_MAX) {
    return owner.bootstrap(authorization,handle,placement,UINT64_MAX,hierarchy);
  };
  const auto before = state(h);
  auto r = no_new([&] { return call(0); });
  check(r.status == field::UpdateStatus::CapacityExhausted && r.failure.budget == field::Budget::PlacementWork &&
        h.ready() && !owner.ready() && before == state(h),"placement refusal keeps healthy empty owner");
  r = no_new([&] { return call(UINT64_MAX,0); });
  if (r.status != field::UpdateStatus::CapacityExhausted || r.failure.budget != field::Budget::CascadeWork ||
      !h.ready() || before != state(h))
    std::cerr << "hierarchy refusal status=" << int(r.status) << " budget=" << field::budget_name(r.failure.budget)
              << " ready=" << h.ready() << " unchanged=" << (before == state(h)) << '\n';
  check(r.status == field::UpdateStatus::CapacityExhausted && r.failure.budget == field::Budget::CascadeWork &&
        h.ready() && before == state(h),"hierarchy refusal keeps exact native state");
  PreparedFixtureAccess::inject_failure(h,1);
  r = call();
  check(r.status != field::UpdateStatus::Ok && h.ready() && before == state(h),
        "late preclaim rejection preserves source/physics/activity/history");
  PreparedFixtureAccess::inject_failure(h,0);
  rejects([&] { owner.bootstrap(warm::new_uuid(),handle,UINT64_MAX,UINT64_MAX,UINT64_MAX); },
          "wrong owner refused before native commit");
  auto stale = handle; stale.generation = warm::new_uuid();
  rejects([&] { owner.bootstrap(authorization,stale,UINT64_MAX,UINT64_MAX,UINT64_MAX); },"stale logical generation refused");
  prepared_test::Metrics metrics(runtime,PreparedFixtureAccess::nodes(h));
  const auto backing = metrics.sample();
  r = no_new([&] { return call(); });
  check(r.status == field::UpdateStatus::Ok && owner.ready() && h.live_hierarchy_installed() &&
        owner.generation() == h.prepared_storage().generation && owner.generation() == 1,
        "existing combined bootstrap publishes the owner only after commit");
  check(backing.same_backing(metrics.sample()),"bootstrap and map publication reuse prepared native backing/compilation");
  const auto committed_before_stale = state(h);
  const PreparedOccurrenceEdit disabled{input.bindings.bodies[0].occurrence,false,owner.generation()};
  check(no_new([&] { return owner.edit(authorization,handle,disabled); }).publication.status ==
            field::UpdateStatus::Unsupported && committed_before_stale == state(h),
        "default prepared owner cannot silently enable occurrence editing");
  const auto stale_attempt = no_new([&] {
    return competitor.owner->bootstrap(authorization,handle,UINT64_MAX,UINT64_MAX,UINT64_MAX);
  });
  check(stale_attempt.status == field::UpdateStatus::Stale && !competitor.owner->ready() &&
        owner.ready() && committed_before_stale == state(h),"stale pending owner cannot replace the committed generation");
  competitor.owner.reset();
  check(!competitor_domain->result().live_bytes && owner.ready(),"stale borrower teardown leaves the current execution intact");
  std::weak_ptr<const ActiveAssembly> source_lifetime = input.source;
  {
  const auto &bodies = owner.bodies(authorization,handle);
  const auto &fields = owner.fields(authorization,handle);
  const auto &s = h.prepared_storage();
  static_assert(std::is_trivially_copyable_v<PreparedStudyOwner::AssociationView>);
  const auto source_owners = input.source.use_count();
  const auto view = no_new([&] { return owner.associations(authorization,handle); });
  const auto associations = no_new([&] { return view.read(); });
  check(input.source.use_count() == source_owners && &associations.source == input.source.get() &&
        &associations.bodies == &bodies && &associations.fields == &fields &&
        &associations.exact_masses == &s.source_masses &&
        associations.source.shared_recipe().ref.selection == input.bindings.source.recipe.selection &&
        associations.source.shared_recipe().ref.revision == input.bindings.source.recipe.revision,
        "checked view shares pinned source, consumed rows and native exact ledger without another owner");
  rejects([&] { owner.associations(warm::new_uuid(),handle); },"association view requires owner authorization");
  for (int part=0; part<3; ++part) {
    auto foreign = handle;
    if (part == 0) foreign.session = warm::new_uuid();
    if (part == 1) foreign.study = warm::new_uuid();
    if (part == 2) foreign.generation = warm::new_uuid();
    rejects([&] { owner.associations(authorization,foreign); },"association view rejects foreign logical identity");
  }
  for (std::size_t i=0; i<bodies.size(); ++i) {
    const auto association = no_new([&] { return associations.body(i); });
    const auto expected = std::find_if(input.bindings.bodies.begin(),input.bindings.bodies.end(),
        [&](const auto &v) { return v.address == bodies[i].address; });
    check(expected != input.bindings.bodies.end(),"actual claim has an independently supplied source binding");
    const auto &layout = input.bindings.layout.at(expected->node);
    check(&association.source == &associations.occurrences[i] && &association.claim == &bodies[i] &&
          associations.source.study() == expected->occurrence.study &&
          association.source.occurrence == expected->occurrence.source && association.source.definition == expected->definition &&
          association.source.node == expected->node &&
          association.scale.node.layout == nullptr && association.scale.node.parent == layout.node.parent &&
          association.scale.node.role == layout.node.role && association.scale.units == layout.units &&
          association.source.units == layout.units && association.exact_mass == 2,
          "body ordinal preserves exact definition, source mass, semantic role and units");
    const auto &v = bodies[i];
    check(v.context == s.particles.context() && v.incarnation && v.address == 40+int(i) &&
          v.record >= 0 && v.record < h.particle_capacity() && v.record != v.address &&
          s.particle_headers[v.record].incarnation == v.incarnation &&
          associations.occurrences[i].occurrence.entry == (i ? 10U : 2U),
          "typed occurrence map uses actual claims rather than addresses or future slots");
  }
  for (std::size_t i=0; i<fields.size(); ++i) {
    const auto association = no_new([&] { return associations.field(i); });
    const auto expected = std::find_if(input.bindings.fields.begin(),input.bindings.fields.end(),
        [&](const auto &v) { return v.address == fields[i].address; });
    check(expected != input.bindings.fields.end(),"actual field claim has an independently supplied typed binding");
    check(&association.source == &associations.field_sources[i] && &association.claim == &fields[i] &&
          association.source.field.kind == expected->field.kind &&
          association.source.field.object == expected->field.object,
          "field ordinal preserves complete typed key and actual claim");
    check(fields[i].context == s.fields.context() && fields[i].incarnation && fields[i].address == int(i),
          "complete typed field map uses actual claims");
  }
  for (bool body : {true,false}) {
    bool refused = false;
    try { if (body) associations.body(bodies.size()); else associations.field(fields.size()); }
    catch (const std::out_of_range &) { refused = true; }
    check(refused,"association ordinal bounds reject before exposing a row");
  }
  // Test-only committed-header fault injection, not an alternate mutation API.
  for (bool body : {true,false}) {
    auto &storage = const_cast<field::detail::PreparedStorage &>(s);
    auto &header = body ? storage.particle_headers[bodies.front().record]
                        : storage.field_headers[fields.front().record];
    const auto saved = header;
    for (int part=0; part<4; ++part) {
      if (part == 0) ++header.context;
      if (part == 1) ++header.incarnation;
      if (part == 2) ++header.address;
      if (part == 3) ++header.record;
      rejects([&] { view.read(); },"association read rejects every mismatched full-handle component");
      header = saved;
    }
  }
  check(&view.read().occurrences.catalogue == &associations.occurrences.catalogue,
        "restored headers retain the same immutable association");
  std::cout << "association_storage,view_bytes=" << sizeof(view)
            << ",loan_bytes=" << sizeof(associations) << ",extra_shared_owners=0,owning_allocations=0\n";
  h.inspect_prepared();
  const auto anchor = bodies.front().record;
  check(h.particle64(field::kPosX,anchor) == 0 && h.particle64(field::kPosY,anchor) == 0 &&
        h.particle64(field::kPosZ,anchor) == 0,"canonical first focal participant is the native D1 anchor");
  for (const auto &v : bodies) {
    check(h.particle64(field::kMass,v.record) == 1.5 && s.source_masses[v.record] == 2,
          "physical mass and exact denominator survive native publication separately");
    for (int f=field::kVelX; f<=field::kVelZ; ++f)
      check(h.particle64(f,v.record) == 0,"new motion is native zero initialization");
  }
  const auto committed = state(h);
  field::UpdateCommand proposal; proposal.operation = field::UpdateOp::DefineParticle;
  proposal.address = 100; proposal.exact_mass = 1;
  const field::PlacementIntrinsic intrinsic{0,0,1,0,{2,1,true}};
  const auto query = no_new([&] {
    return h.query_prepared_placement(owner.generation(),&proposal,1,&intrinsic,1);
  });
  check(query.status == field::PlacementStatus::Ok && !s.resolved[0].context &&
        bodies[0].context == s.particles.context() && owner.ready() && committed == state(h),
        "later scratch reuse cannot change the published actual-claim map");
  r = no_new([&] { return call(); });
  check(r.status == field::UpdateStatus::NoChange && committed == state(h) && owner.ready(),"authorized repeat bootstrap is a healthy no-op");
  check(&view.read().bodies == &bodies,"scratch reuse and no-op retain current association view");
  auto denied = prepare(h,input,d);
  check(denied.status == AdmissionPlanStatus::InvalidInput && !denied.owner && committed == state(h),
        "a populated execution cannot be adopted as an empty bootstrap target");
  field::Parameters64 p; p.gravity = p.contact_stiffness = p.reach_strength = 0; p.dt = 0.125;
  auto tick = no_new([&] { return owner.tick(authorization,handle,p,0); });
  check(tick.status == field::TickStatus::CapacityExhausted && committed == state(h) && owner.ready(),
        "tick resource rejection preserves published owner and maps");
  auto invalid = p; invalid.dt = -1;
  tick = owner.tick(authorization,handle,invalid);
  check(tick.status == field::TickStatus::InvalidInput && owner.ready() && committed == state(h),
        "invalid tick parameters remain a healthy native refusal");
  check(&view.read().occurrences.catalogue == &associations.occurrences.catalogue,
        "healthy tick refusals preserve current association view");
  tick = no_new([&] { return owner.tick(authorization,handle,p); });
  check(tick.status == field::TickStatus::Ok && tick.completed_ticks == 1 && owner.ready(),"native owner tick after generated bootstrap");
  rejects([&] { view.read(); },"completed quiet tick invalidates an old association stamp");
  const auto after_tick = owner.associations(authorization,handle);
  const auto generation = owner.generation();
  p.gravity = 0.025;
  tick = no_new([&] { return owner.tick(authorization,handle,p); });
  check(tick.status == field::TickStatus::Ok && tick.coverage_changed && tick.completed_ticks == 2 &&
        tick.generation == generation+1 && owner.generation() == tick.generation && owner.ready(),
        "owner follows a native contact-changing tick without replacing identity maps");
  rejects([&] { after_tick.read(); },"contact-changing tick invalidates an old native generation stamp");
  const auto current = owner.associations(authorization,handle);
  input.source.reset();
  check(!source_lifetime.expired() && current.read().source.study() == handle.study &&
        &current.read().occurrences.catalogue == &associations.occurrences.catalogue,
        "owner retains immutable pins after caller release and view reacquisition");
  } // All association views/loans end before owner teardown.
  prepared.owner.reset();
  check(source_lifetime.expired() && !d->result().live_bytes && h.ready() && h.present_particles() == 2,
        "borrower teardown releases metadata without retiring retained native state");
}
void failed_publication(engine::Runtime &runtime, field::HostPath path, int point) {
  Input input; input.install_layout(runtime);
  field::Harness h(runtime,capacity(),path);
  auto d = domain(); auto prepared = prepare(h,input,d);
  check(prepared.status == AdmissionPlanStatus::Ok,"failure fixture owner preparation");
  auto &owner = *prepared.owner; const auto handle = owner.handle();
  PreparedFixtureAccess::inject_failure(h,point);
  bool failed = false;
  try { owner.bootstrap(input.source->owner(),handle,UINT64_MAX,UINT64_MAX,UINT64_MAX); }
  catch (const std::runtime_error &) { failed = true; }
  check(failed && !h.ready() && !owner.ready() && owner.phase() == PreparedOwnerPhase::Failed,
        "postclaim native/publication failure makes execution unready");
  rejects([&] { owner.bodies(input.source->owner(),handle); },"half-published claims remain unavailable");
  rejects([&] { owner.associations(input.source->owner(),handle); },"failed publication cannot expose an association view");
  check(owner.bootstrap(input.source->owner(),handle,UINT64_MAX,UINT64_MAX,UINT64_MAX).status == field::UpdateStatus::Unready,
        "failed publication cannot be retried as a healthy empty owner");
  prepared.owner.reset(); check(!d->result().live_bytes,"failed publication metadata teardown");
}
}
int main(int argc, char **argv) {
  try {
    const int workers = argc > 1 ? std::stoi(argv[1]) : 1;
    const auto path = argc > 2 && std::string(argv[2]) == "direct" ? field::HostPath::CpuDirect : field::HostPath::BulkCopy;
    engine::InstanceSettings settings; settings.read_environment = false; settings.cpu_threads = workers;
    settings.cuda = false;
    engine::Runtime runtime(settings);
    lifecycle(runtime,path,false); lifecycle(runtime,path,true);
    if (workers == 1 && path == field::HostPath::BulkCopy) {
      failed_publication(runtime,path,2); failed_publication(runtime,path,7);
    }
    std::cout << "prepared_study_owner,checks=" << checks << ",workers=" << workers
              << ",path=" << (path == field::HostPath::BulkCopy ? "bulk" : "direct") << " PASS\n";
    return 0;
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
