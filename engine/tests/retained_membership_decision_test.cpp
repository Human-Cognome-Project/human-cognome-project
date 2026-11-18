#include "retained_membership_fixture.h"
#include "assembly/prepared_submission.h"
#include "snapshot.h"
#include "hierarchy_entry_trace.h"
#include "field/cascade_source.h"
#include <algorithm>
#include <iostream>
#include <map>

namespace {
using namespace assembly;
using namespace assembly::detail;
using field::Budget;
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
using namespace retained_membership_fixture;
void plan_checks(const Input &input) {
  const auto cap = capacity();
  auto plan = input.build(input.bindings,cap);
  check(plan.status == AdmissionPlanStatus::Ok && plan.plan,"six-comparison chain refused");
  const auto hierarchy = plan.plan->hierarchy();
  check(plan.plan->commands().size() == 20 && hierarchy.node_count == 6 &&
        hierarchy.child_count == 5 && hierarchy.contribution_count == 12,"decision demand differs from exact source");
  std::map<int,int> rows;
  for (const auto &c : plan.plan->commands()) if (c.operation == field::UpdateOp::InsertParticipation) {
    check(c.exact_count == 2 && c.share == 1,"membership diamond was double-counted or NULL used as a denominator");
    ++rows[c.second.command];
  }
  check(rows.size() == 6,"missing source field");
  for (const auto &row : rows) check(row.second == 2,"occurrences merged or inherited memberships duplicated");
  auto invalid = input.bindings;
  invalid.comparisons[4].children.clear();
  invalid.comparisons[5].children = {3,4}; // Both children cover the whole cohort.
  auto refused = input.build(invalid,cap);
  check(refused.status == AdmissionPlanStatus::InvalidInput && !refused.plan,"overlapping fork accepted");
  invalid = input.bindings; invalid.comparisons[3].children.clear();
  refused = input.build(invalid,cap);
  check(refused.status == AdmissionPlanStatus::InvalidInput && !refused.plan,"independent roots accepted");
  invalid = input.bindings; invalid.comparisons.resize(2);
  refused = input.build(invalid,cap);
  check(refused.status == AdmissionPlanStatus::InvalidInput && !refused.plan,"membership comparisons silently omitted");
  auto reordered = input.bindings;
  std::swap(reordered.comparisons[2].field,reordered.comparisons[3].field);
  std::swap(reordered.comparisons[2].comparison,reordered.comparisons[3].comparison);
  auto reordered_plan = input.build(reordered,cap);
  check(reordered_plan.status == AdmissionPlanStatus::Ok && reordered_plan.plan,
        "equal-cohort ordering was incorrectly inferred from membership ancestry");
  for (auto budget : {Budget::Fields,Budget::CascadeNodes,Budget::CascadeLinks,Budget::CascadeLevels}) {
    auto short_limits = cap.limits(); --short_limits[budget];
    refused = input.build(input.bindings,field::PreparedCapacity(short_limits,cap.profile()));
    check(refused.status == AdmissionPlanStatus::CapacityExhausted && refused.failure.budget == budget &&
          !refused.plan,"one-short decision capacity did not identify its exhausted resource");
  }
  // An English-membership focus is supported with NULL naming mass; retaining
  // the existing ParentValue focus is an explicit proposal, not an engine limit.
  auto alternate = std::make_shared<const ActiveAssembly>(input.source->focus(
    {{GroupKind::Membership,InputText(warm::key(input.groups[1]))}}));
  auto alternative = input.bindings;
  alternative.source = {alternate->owner(),alternate->study(),alternate->generation(),alternate->shared_recipe().ref};
  alternative.focus = {{GroupKind::Membership,input.groups[1]}};
  auto other = StructuralAdmissionPlan::build(*alternate,alternative,cap,resources(),10000000);
  check(other.status == AdmissionPlanStatus::Ok && other.plan,"positive membership focus demanded a naming mass");
  std::cout << "membership_decision_plan,source=fixture_only,fields=6,participations=12,commands=20,nodes=6,links=5,rounds=6"
    << ",plan_work=" << plan.work.charged << ",metadata_peak=" << plan.reservation.peak_bytes
    << ",focus_alternative=valid,order_alternative=valid,overlap_forest_omission=refused,short_capacities=4 PASS\n";
}
void native_check(const Input &input, int workers, field::HostPath path) {
  engine::InstanceSettings settings; settings.read_environment = false; settings.cpu_threads = workers;
  ActiveSession session(settings);
  check(!session.on_cuda(),"actual CPU backend required");
  PreparedStudySubmission request{input.source,input.bindings,capacity(),resources()};
  for (auto &layout : request.bindings.layout) layout.node.layout = nullptr;
  request.path = path; request.plan_work = request.metadata_work = 10000000;
  request.placement_work = 100000000; request.placement_cells = 65536; request.hierarchy_work = 1000000;
  handoff_trace::Calls calls;
  PreparedSubmissionResult result;
  { handoff_trace::Scope trace(calls); result = session.submit_prepared(request,input.source->owner()); }
  check(result.handle && calls.combined == 1 && !calls.standalone,"native decision did not use one existing combined bootstrap");
  const auto handle = *result.handle;
  const auto before = viewport::observe(session,input.source->owner(),handle);
  check(before.body_records.size() == 2 && before.comparisons.size() == 6 && before.tick == 0,
        "incomplete decision observation");
  std::map<std::string,int> addresses;
  for (const auto &body : request.bindings.bodies) addresses.emplace(occurrence_text(body.occurrence),body.address);
  for (std::size_t i=0; i<before.body_records.size(); ++i) {
    const auto &claim = before.body_records[i]; const auto &p = before.report.prepared.particles[i];
    check(claim.context && claim.incarnation && claim.record == p.occurrence.slot &&
          claim.address == addresses.at(std::string(p.occurrence.id)) && p.exact_mass == 2,"source occurrence lost its actual full claim");
    const auto stride = std::size_t(before.report.prepared.capacity);
    check(before.report.particles64[field::kMass*stride+claim.record] == 1.5 &&
          before.report.particles64[field::kRadius*stride+claim.record] == 0.125,
          "native body changed supplied physical mass or intrinsic geometry");
  }
  const auto &report = before.report;
  for (std::size_t i=0; i<before.comparisons.size(); ++i) {
    const auto &node = before.comparisons[i];
    const auto &field = report.prepared.groups.at(node.field.record);
    check(node.field.context && node.field.incarnation && node.field.address == int(i) &&
          node.comparison == 100+i && node.level == (i ? 1 : 0) && node.retained_fraction_bits == 52 &&
          node.child_count == (i ? 1U : 0U) && field.kind == input.bindings.fields[i].field.kind &&
          field.value == warm::key(input.bindings.fields[i].field.object),
          "comparison identity, actual claim, precision or explicit dependency choice changed");
  }
  check(report.prepared.scales.size() == 1 && report.prepared.scales[0].units == 1 &&
        !report.prepared.scales[0].node.layout,"comparison rounds became conceptual scale or leaked native layout");
  check(std::count_if(report.prepared.groups.begin(),report.prepared.groups.end(),
    [](const Group &g) { return g.kind == GroupKind::Membership; }) == 4,
    "missing inherited membership field in native mapping");
  check(report.prepared.focal_groups.size() == 1,"decision focus count changed");
  const auto &focus = report.prepared.groups.at(report.prepared.focal_groups[0]);
  check(focus.kind == GroupKind::ParentValue && focus.value == warm::key(input.bindings.fields[1].field.object),
        "proposal changed existing structural focus");
  for (std::size_t i=0; i<report.prepared.groups.size(); ++i)
    check(report.groups64[field::kCenM*report.prepared.groups.size()+i] == 3,
          "selected cohort physical mass differs from two supplied masses");
  for (const auto &g : input.groups)
    check(!input.source->shared_recipe().tools.at(input.fixture.tools.at(g)).mass,
          "native observation filled an unknown source naming mass");
  field::Parameters64 parameters; parameters.dt = 0.1;
  const auto tick = session.tick_prepared(handle,input.source->owner(),parameters,1000000000,1000000);
  const auto after = viewport::observe(session,input.source->owner(),handle);
  check(tick.status == field::TickStatus::Ok && after.tick == 1 && after.prepared_generation == tick.generation,
        "decision fixture did not complete a native tick");
  for (std::size_t i=0; i<before.body_records.size(); ++i)
    check(field::detail::same_handle(before.body_records[i],after.body_records[i]),"tick changed a published source claim");
  const auto demand = session.demand(handle,input.source->owner());
  check(demand.fields_loaded == 6 && demand.edges_loaded == 12,"native ledger did not retain deduplicated complete participation");
  check(demand.force_requests == 12 && demand.force_evaluations == 2 && demand.force_zero_response == 10,
        "equal-coverage unary ancestors added independent force responses");
  std::cout << "membership_decision_native,source=fixture_only,retained_tick=unqualified,workers=" << workers
    << ",claims=2,comparisons=6,combined_commits=" << calls.combined << ",standalone_installs=" << calls.standalone
    << ",completed_tick=" << after.tick << ",fields_loaded=" << demand.fields_loaded
    << ",edges_loaded=" << demand.edges_loaded << ",metadata_peak=" << request.resources->result().peak_bytes
    << ",force_requests=" << demand.force_requests << ",force_evaluations=" << demand.force_evaluations
    << ",zero_responses=" << demand.force_zero_response
    << ",scratch_capacity=" << request.capacity.limits()[Budget::ScratchBytes]
    << ",prior_scratch_capacity=" << prepared_owner_fixture::capacity().limits()[Budget::ScratchBytes] << " PASS\n";
}
}
int main(int argc, char **argv) {
  try {
    if (argc != 3) throw std::runtime_error("workers path required");
    Input input; plan_checks(input);
    native_check(input,std::stoi(argv[1]),std::string(argv[2]) == "direct" ? field::HostPath::CpuDirect : field::HostPath::BulkCopy);
    return 0;
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
