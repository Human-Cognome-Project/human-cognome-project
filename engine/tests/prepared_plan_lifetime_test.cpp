#define HCP_ALLOCATION_LIFETIME
#include "allocation_observation.h"
#include "prepared_owner_fixture.h"
#include "field/cascade_source.h"
#include <iostream>

namespace {
using namespace prepared_owner_fixture;
namespace A = allocation_lifetime;
void check(bool ok, const char *why) { if (!ok) throw std::runtime_error(why); }
void observer_control() {
  A::begin();
  auto *tracked = ::operator new(17);
  A::capture_acquisitions = false;
  auto *outside = ::operator new(29);
  ::operator delete(tracked);
  check(A::counts.acquired == 1 && A::counts.released == 1 && !A::counts.live,
        "frozen observer lost tracked destruction or counted new requests");
  ::operator delete(outside);
  check(!A::end().blocks,"untracked destruction changed the frozen census");
}
void run(engine::Runtime &runtime, field::HostPath path, bool parts, bool baseline) {
  Input input(parts); input.install_layout(runtime);
  field::Harness h(runtime,capacity(),path);
  const auto resources = domain(), separate = domain();
  auto standalone = StructuralAdmissionPlan::build(*input.source,input.bindings,capacity(),separate,UINT64_MAX);
  check(standalone.plan && standalone.status == AdmissionPlanStatus::Ok,"standalone plan failed");
  const auto *standalone_commands = standalone.plan->commands().data();
  const auto standalone_spec = standalone.plan->hierarchy();
  const auto standalone_bytes = separate->result().live_bytes;
  A::begin();
  auto prepared = prepare(h,input,resources);
  A::capture_acquisitions = false; // Keep frees; exclude native/compiler/report allocations.
  check(prepared.owner && prepared.status == AdmissionPlanStatus::Ok,"preparation failed");
  const auto prepared_bytes = resources->result().live_bytes, peak = resources->result().peak_bytes;
  check(A::counts.live == prepared_bytes && A::counts.peak == peak,"domain and independent preparation census disagree");
  auto &owner = *prepared.owner;
  const auto handle = owner.handle(); const auto authorization = input.source->owner();
  std::weak_ptr<const ActiveAssembly> source_lifetime = input.source;
  input.source.reset();
  check(!source_lifetime.expired(),"preparing owner lost its composed source pin");
  check(owner.bootstrap(authorization,handle,0,65536,1000000).status != field::UpdateStatus::Ok &&
        resources->result().live_bytes == prepared_bytes && A::counts.live == prepared_bytes &&
        owner.phase() == PreparedOwnerPhase::Prepared,"healthy refusal retired retry data");
  check(owner.bootstrap(authorization,handle,100000000,65536,1000000).status == field::UpdateStatus::Ok,
        "native bootstrap retry failed");
  const auto published = resources->result().live_bytes;
  check(A::counts.live == published && (baseline ? published == prepared_bytes : published < prepared_bytes),
        "publication retention does not match selected source profile");
  const auto verify = [&] {
    const auto rows = owner.associations(authorization,handle).read();
    const auto &storage = h.prepared_storage();
    const auto report = owner.report(authorization,handle);
    const auto &native = h.inspect_live_hierarchy();
    check(native.node_count == input.bindings.comparisons.size() &&
          report.prepared.particles.size() == rows.bodies.size(),"published mapping/topology incomplete");
    for (std::size_t i=0; i<rows.bodies.size(); ++i) {
      const auto row = rows.body(i);
      check(row.claim.context && row.claim.incarnation && row.claim.record >= 0 &&
            field::detail::same_handle(row.claim,storage.particle_headers.at(row.claim.record)) &&
            row.exact_mass == storage.source_masses.at(row.claim.record) && row.exact_mass == 2 &&
            report.prepared.particles.at(i).exact_mass == row.exact_mass,"exact source/native claim mismatch");
      const auto origin = std::find_if(report.origins.begin(),report.origins.end(),[&](const auto &o) {
        return o.occurrence.study == rows.source.study() && o.occurrence.source == row.source.occurrence;
      });
      check(origin != report.origins.end() && origin->shared && *origin->shared == row.source.definition,
            "retirement changed complete detached definition provenance");
    }
    for (std::size_t i=0; i<rows.fields.size(); ++i) {
      const auto row = rows.field(i);
      const auto &node = native.nodes.at(native.order.at(i));
      const auto &expected = input.bindings.comparisons.at(i);
      check(field::detail::same_handle(row.claim,storage.field_headers.at(row.claim.record)) &&
            field::detail::same_handle(row.claim,node.field) && node.comparison == expected.comparison &&
            node.level == expected.level && node.retained_fraction_bits == expected.retained_fraction_bits,
            "retirement changed actual comparison identity/precision");
    }
    check(resources->result().live_bytes == published && A::counts.live == published &&
          resources->result().peak_bytes == peak,"native tick/report changed owner metadata requests");
    check(standalone.plan->commands().data() == standalone_commands &&
          standalone.plan->commands().size() == (parts ? 11U : 8U) &&
          standalone.plan->hierarchy().nodes == standalone_spec.nodes &&
          standalone.plan->hierarchy().contributions == standalone_spec.contributions &&
          separate->result().live_bytes == standalone_bytes,"private consumption changed standalone plan lifetime");
  };
  verify();
  const auto previous = owner.associations(authorization,handle);
  field::Parameters64 parameters; parameters.dt = 0.125;
  check(owner.tick(authorization,handle,parameters).status == field::TickStatus::Ok,"following native tick failed");
  bool stale = false;
  try { previous.read(); } catch (const warm::Stale &) { stale = true; }
  check(stale,"old association view stayed current across tick");
  verify();
  check(owner.bootstrap(authorization,handle,0,0,0).status == field::UpdateStatus::NoChange &&
        h.prepared_storage().completed_ticks == 1,"published bootstrap replay is not a no-op");
  const auto detached = owner.report(authorization,handle);
  prepared.owner.reset();
  const auto released = A::end();
  check(!resources->result().live_bytes && !released.live && !released.blocks && source_lifetime.expired(),
        "owner teardown retained measured requests or source pin");
  check(detached.prepared.particles.size() == 2 && !detached.origins.empty() &&
        standalone.plan->commands().data() == standalone_commands && standalone_spec.nodes[0].comparison == 100,
        "detached report/standalone plan did not survive owner and source death");
  std::cout << "prepared_plan_lifetime,source=fixture_only,profile=" << (baseline ? "baseline" : "retired")
    << ",exposure=" << (parts ? "parts" : "aggregate") << ",owner_header=" << sizeof(PreparedStudyOwner)
    << ",prepared_retained=" << prepared_bytes << ",published_retained=" << published << ",setup_peak=" << peak
    << ",retired_at_publication=" << prepared_bytes-published
    << ",completed_tick=1,source_mass_bodies=2,release_live=0 PASS\n";
}
}
int main(int argc, char **argv) {
  try {
    check(argc == 3 || (argc == 4 && std::string(argv[3]) == "baseline"),"workers path [baseline] required");
    observer_control();
    engine::InstanceSettings settings; settings.read_environment = false; settings.cuda = false;
    settings.cpu_threads = std::stoi(argv[1]);
    engine::Runtime runtime(settings);
    const auto path = std::string(argv[2]) == "direct" ? field::HostPath::CpuDirect : field::HostPath::BulkCopy;
    run(runtime,path,false,argc == 4); run(runtime,path,true,argc == 4);
    return 0;
  } catch (const std::exception &e) { A::active = false; std::cerr << e.what() << '\n'; return 1; }
}
