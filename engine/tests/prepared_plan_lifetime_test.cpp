#define HCP_ALLOCATION_LIFETIME
#include "allocation_observation.h"
#include "prepared_owner_fixture.h"
#include "field/cascade_source.h"
#include <iostream>

namespace {
using namespace prepared_owner_fixture;
namespace A = allocation_lifetime;
void check(bool ok, const char *why) { if (!ok) throw std::runtime_error(why); }
std::size_t request(const void *pointer) {
  for (const auto &entry : A::table) if (entry.pointer == pointer) return entry.bytes;
  throw std::runtime_error("plan backing has no independently observed allocation");
}
struct Backing {
  const void *data;
  std::size_t bytes;
};
void run(engine::Runtime &runtime, field::HostPath path, bool parts) {
  Input input(parts); input.install_layout(runtime);
  field::Harness h(runtime,capacity(),path);
  const auto resources = domain();
  // Observe only preparation's current-thread C++ requests. Native setup/tick
  // and detached reports are outside this census; no RSS/driver claim is made.
  A::begin();
  auto prepared = prepare(h,input,resources);
  A::active = false;
  check(prepared.owner && prepared.status == AdmissionPlanStatus::Ok,"preparation failed");
  const auto retained = resources->result().live_bytes;
  check(A::counts.live == retained,"domain and independent retained requests disagree");
  auto &owner = *prepared.owner;
  const auto handle = owner.handle(); const auto authorization = input.source->owner();
  const auto original = A::counts;
  check(owner.bootstrap(authorization,handle,0,65536,1000000).status != field::UpdateStatus::Ok &&
        resources->result().live_bytes == retained && owner.phase() == PreparedOwnerPhase::Prepared,
        "healthy bootstrap refusal changed plan ownership");
  check(owner.bootstrap(authorization,handle,100000000,65536,1000000).status == field::UpdateStatus::Ok,
        "native bootstrap failed");
  const auto &plan = owner.plan(authorization,handle);
  const auto spec = plan.hierarchy();
  const auto vector = [&](const auto &v) {
    using T = typename std::decay_t<decltype(v)>::value_type;
    const auto bytes = v.capacity()*sizeof(T);
    check(bytes && request(v.data()) == bytes,"vector extent differs from actual C++ request");
    return Backing{v.data(),bytes};
  };
  const Backing buffers[]{vector(plan.commands()),vector(plan.intrinsics()),vector(plan.focus()),
    {spec.nodes,request(spec.nodes)},{spec.children,request(spec.children)},
    {spec.contributions,request(spec.contributions)}};
  check(buffers[3].bytes == spec.node_count*sizeof(field::LiveRefreshNode) &&
        buffers[4].bytes == spec.child_count*sizeof(int) &&
        buffers[5].bytes == spec.contribution_count*sizeof(field::LiveRefreshContribution),
        "hierarchy logical rows differ from actual reserved requests");
  std::size_t candidate = 0;
  for (std::size_t i=0; i<6; ++i) {
    candidate += buffers[i].bytes;
    for (std::size_t j=0; j<i; ++j) check(buffers[i].data != buffers[j].data,"candidate backing counted twice");
  }
  check(request(&plan) == sizeof(plan) && candidate < retained,"plan request attribution incomplete");
  const auto commands = plan.commands().size();
  const auto verify = [&] {
    const auto associations = owner.associations(authorization,handle).read();
    const auto &storage = h.prepared_storage();
    const auto report = owner.report(authorization,handle);
    const auto &native = h.inspect_live_hierarchy();
    check(native.node_count == spec.node_count && report.prepared.particles.size() == associations.bodies.size(),
          "native installed topology or detached mapping incomplete");
    for (std::size_t i=0; i<associations.bodies.size(); ++i) {
      const auto row = associations.body(i);
      const auto &claim = row.claim;
      check(claim.context && claim.incarnation && claim.record >= 0 &&
            field::detail::same_handle(claim,storage.particle_headers.at(claim.record)) &&
            row.exact_mass == storage.source_masses.at(claim.record) && row.exact_mass == 2 &&
            report.prepared.particles.at(i).exact_mass == row.exact_mass &&
            plan.commands().at(row.source.command).exact_mass == row.exact_mass,
            "plan/claim/native ledger/report exact masses disagree");
      const auto origin = std::find_if(report.origins.begin(),report.origins.end(),[&](const auto &o) {
        return o.occurrence.study == plan.source().study && o.occurrence.source == row.source.occurrence;
      });
      check(origin != report.origins.end() && origin->shared && *origin->shared == row.source.definition,
            "native numeric source cannot replace full definition provenance");
    }
    for (std::size_t i=0; i<associations.fields.size(); ++i) {
      const auto row = associations.field(i);
      const auto &node = native.nodes.at(native.order.at(i));
      check(field::detail::same_handle(row.claim,storage.field_headers.at(row.claim.record)) &&
            field::detail::same_handle(row.claim,node.field) &&
            node.comparison == spec.nodes[i].comparison && node.level == spec.nodes[i].level &&
            node.retained_fraction_bits == spec.nodes[i].retained_fraction_bits,
            "installed native descriptor differs from sealed plan identity");
    }
    check(resources->result().live_bytes == retained && A::counts.live == original.live,
          "bootstrap/tick changed selected owner-domain retained backing");
    for (const auto &buffer : buffers) check(request(buffer.data) == buffer.bytes,"plan buffer lifetime changed");
  };
  verify();
  const auto previous = owner.associations(authorization,handle);
  field::Parameters64 parameters; parameters.dt = 0.125;
  check(owner.tick(authorization,handle,parameters).status == field::TickStatus::Ok,"following native tick failed");
  bool stale = false;
  try { previous.read(); } catch (const warm::Stale &) { stale = true; }
  check(stale,"retained backing must not make an old association view current");
  verify();
  check(owner.bootstrap(authorization,handle,0,0,0).status == field::UpdateStatus::NoChange &&
        h.prepared_storage().completed_ticks == 1,"published bootstrap replay is not a no-op");
  // Preparation's allocations remain live until owner teardown. Tracking is
  // resumed only for their destruction; native/compiler allocations were never
  // admitted to this observer, and the source/domain were created beforehand.
  A::active = true;
  prepared.owner.reset();
  const auto released = A::end();
  check(!resources->result().live_bytes && !released.live && !released.blocks,
        "owner teardown did not release all measured preparation requests");
  std::cout << "prepared_plan_lifetime,source=fixture_only,exposure=" << (parts ? "parts" : "aggregate")
    << ",commands=" << commands << ",nodes=" << spec.node_count << ",contributions=" << spec.contribution_count
    << ",owner_domain_retained=" << retained << ",bootstrap_buffer_candidate=" << candidate
    << ",commands_bytes=" << buffers[0].bytes << ",intrinsics_bytes=" << buffers[1].bytes
    << ",focus_bytes=" << buffers[2].bytes << ",nodes_bytes=" << buffers[3].bytes
    << ",children_bytes=" << buffers[4].bytes << ",contributions_bytes=" << buffers[5].bytes
    << ",achieved_saving=0,completed_tick=1,source_mass_crosschecks=2,release_live=0 PASS\n";
}
}
int main(int argc, char **argv) {
  try {
    check(argc == 3,"workers path required");
    engine::InstanceSettings settings; settings.read_environment = false; settings.cuda = false;
    settings.cpu_threads = std::stoi(argv[1]);
    engine::Runtime runtime(settings);
    const auto path = std::string(argv[2]) == "direct" ? field::HostPath::CpuDirect : field::HostPath::BulkCopy;
    run(runtime,path,false); run(runtime,path,true);
    return 0;
  } catch (const std::exception &e) { A::active = false; std::cerr << e.what() << '\n'; return 1; }
}
