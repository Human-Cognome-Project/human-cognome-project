#include "prepared_owner_fixture.h"
#include "prepared_fixture.h"
#include "prepared_metrics.h"
#include "allocation_observation.h"
#include "assembly/prepared_occurrence_edit.h"
#include <iostream>

namespace {
using namespace assembly;
using field::UpdateStatus;
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
template<class F> auto no_new(F action) {
  allocation_observation::calls=0;
  allocation_observation::enabled=true;
  try {
    auto result=action(); allocation_observation::enabled=false;
    check(!allocation_observation::calls,"edit allocated recurring C++ storage"); return result;
  } catch (...) { allocation_observation::enabled=false; throw; }
}
template<class F> void rejects(F action) {
  bool rejected=false;
  try { action(); } catch (const warm::Error &) { rejected=true; }
  check(rejected,"foreign or stale edit was authorized");
}
void owner_case(int workers, field::HostPath path) {
  using namespace prepared_owner_fixture;
  engine::InstanceSettings settings; settings.read_environment=false; settings.cpu_threads=workers;
  engine::Runtime runtime(settings);
  Input input; input.install_layout(runtime);
  auto limits=capacity().limits(); limits[field::Budget::Batch]=32; limits[field::Budget::Particles]=2;
  limits[field::Budget::ScratchBytes]=field::detail::PreparedStorage::required_scratch(
      limits,false,field::NumericalProfile::trial())+256*1024*1024;
  field::Harness harness(runtime,field::PreparedCapacity(limits,field::NumericalProfile::trial()),path);
  auto resources=domain();
  auto built=detail::PreparedStudyOwner::prepare(harness,input.source,input.session,input.bindings,resources,
      UINT64_MAX,UINT64_MAX,true);
  check(bool(built.owner),"direct editable preparation failed");
  const auto peak=resources->result().peak_bytes, allocation_work=resources->result().charged_work;
  const auto metadata_work=built.metadata_work.charged;
  built.owner.reset();
  check(resources->result().live_bytes == 0,"editable preparation retained metadata after destruction");
  auto byte_short=domain({peak-1,UINT64_MAX});
  auto refused=detail::PreparedStudyOwner::prepare(harness,input.source,input.session,input.bindings,byte_short,
      UINT64_MAX,UINT64_MAX,true);
  check(!refused.owner && refused.status == detail::AdmissionPlanStatus::AllocationExhausted &&
        byte_short->result().live_bytes == 0 && harness.ready(),"one-short editable bytes did not unwind");
  auto work_short=domain({UINT64_MAX,allocation_work-1});
  refused=detail::PreparedStudyOwner::prepare(harness,input.source,input.session,input.bindings,work_short,
      UINT64_MAX,UINT64_MAX,true);
  check(!refused.owner && refused.status == detail::AdmissionPlanStatus::AllocationExhausted &&
        work_short->result().live_bytes == 0 && harness.ready(),"one-short editable allocation work did not unwind");
  auto metadata_short=domain();
  refused=detail::PreparedStudyOwner::prepare(harness,input.source,input.session,input.bindings,metadata_short,
      UINT64_MAX,metadata_work-1,true);
  check(!refused.owner && refused.metadata_work.status == detail::PreparationStatus::WorkExhausted &&
        metadata_short->result().live_bytes == 0 && harness.ready(),"one-short editable metadata work did not unwind");
  resources=domain({peak,allocation_work});
  built=detail::PreparedStudyOwner::prepare(harness,input.source,input.session,input.bindings,resources,
      UINT64_MAX,metadata_work,true);
  check(bool(built.owner),"exact editable bytes/work retry failed");
  auto &owner=*built.owner; const auto handle=owner.handle();
  check(owner.bootstrap(input.source->owner(),handle,UINT64_MAX,UINT64_MAX,UINT64_MAX).status == UpdateStatus::Ok,
        "direct editable bootstrap failed");
  prepared_test::Metrics metrics(runtime,field::testing::PreparedFixtureAccess::nodes(harness));
  const auto backing=metrics.sample();
  std::cout << "editable_owner_setup,body_slots=2,peak_bytes=" << peak
            << ",allocation_work=" << allocation_work << ",metadata_work=" << metadata_work << '\n';
  const auto view=owner.associations(input.source->owner(),handle);
  PreparedOccurrenceEdit full{{input.source->study(),{999,{}}},true,owner.generation()};
  full.clone_from=input.bindings.bodies[0].occurrence;
  const auto bounded=no_new([&] { return owner.edit(input.source->owner(),handle,full); });
  check(bounded.publication.status == UpdateStatus::CapacityExhausted &&
        bounded.publication.failure.budget == field::Budget::Particles && owner.ready() && view.read().occurrences.size() == 2,
        "full instance capacity changed the healthy catalogue");
  PreparedOccurrenceEdit edit{input.bindings.bodies[0].occurrence,false,owner.generation()};
  const auto ticks=harness.prepared_storage().completed_ticks;
  auto result=no_new([&] { return owner.edit(input.source->owner(),handle,edit); });
  check(result.publication.status == UpdateStatus::Ok && owner.ready(),"direct removal failed");
  rejects([&] { view.read(); });
  full.expected_generation=owner.generation();
  check(no_new([&] { return owner.edit(input.source->owner(),handle,full); }).publication.status == UpdateStatus::Ok,
        "free native/metadata slot was not reusable for a new identity");
  full.present=false; full.clone_from.reset(); full.expected_generation=owner.generation();
  check(no_new([&] { return owner.edit(input.source->owner(),handle,full); }).publication.status == UpdateStatus::Ok,
        "recycled identity removal failed");
  check(backing.same_backing(metrics.sample()),"edits changed native backing, tree or compilation counters");
  edit.present=true; edit.expected_generation=owner.generation();
  field::testing::PreparedFixtureAccess::inject_failure(harness,2);
  bool failed=false;
  try { owner.edit(input.source->owner(),handle,edit); } catch (const std::exception &) { failed=true; }
  check(failed && !harness.ready() && owner.phase() == detail::PreparedOwnerPhase::Failed &&
        harness.prepared_storage().completed_ticks == ticks,"postclaim failure remained healthy or advanced time");
  check(owner.edit(input.source->owner(),handle,edit).publication.status == UpdateStatus::Unready,
        "failed execution resumed editing");
  std::cout << "prepared_occurrence_edit_failure,postclaim=unready,stale_view=rejected,"
               "exact_short_setup=1,capacity_and_reuse=1 PASS\n";
}
}
int main(int argc, char **argv) {
  try {
    const int workers=argc>1 ? std::stoi(argv[1]) : 1;
    const auto path=argc>2 && std::string(argv[2]) == "direct" ? field::HostPath::CpuDirect : field::HostPath::BulkCopy;
    owner_case(workers,path); return 0;
  } catch (const std::exception &e) { std::cerr << "prepared edit core: " << e.what() << '\n'; return 1; }
}
