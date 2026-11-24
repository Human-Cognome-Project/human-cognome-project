#include "letter_projection_fixture.h"
#include "allocation_observation.h"
#include "assembly/prepared_occurrence_edit.h"
#include "prepared_letter_profile.h"
#include "snapshot.h"
#include "field/cascade_source.h"
#include <cstring>
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
bool same(const engine::RecordHandle &a, const engine::RecordHandle &b) {
  return field::detail::same_handle(a,b);
}
std::size_t find(const viewport::CompletedState &state, const OccurrenceKey &key) {
  const auto name=occurrence_text(key);
  for (std::size_t i=0; i<state.body_records.size(); ++i)
    if (std::string(state.report.prepared.particles[i].occurrence.id) == name) return i;
  throw std::runtime_error("missing full live occurrence identity");
}
void survivor(const viewport::CompletedState &before, const viewport::CompletedState &after,
              const OccurrenceKey &key) {
  const auto a=before.body_records[find(before,key)], b=after.body_records[find(after,key)];
  check(same(a,b),"edit replaced survivor claim");
  const auto capacity=before.report.prepared.capacity;
  for (int k=0; k<field::kParticleFieldCount; ++k) {
    const auto x=before.report.particles64[k*capacity+a.record];
    const auto y=after.report.particles64[k*capacity+b.record];
    check(!std::memcmp(&x,&y,sizeof(x)),"edit changed survivor physical state");
  }
  check(before.tick == after.tick,"edit advanced physical time");
}
void unchanged(const viewport::CompletedState &before, const viewport::CompletedState &after) {
  check(before.prepared_generation == after.prepared_generation && before.tick == after.tick &&
        before.body_records.size() == after.body_records.size() &&
        before.report.particles64 == after.report.particles64 &&
        before.report.groups64 == after.report.groups64,"ordinary refusal changed committed state");
  for (std::size_t i=0; i<before.body_records.size(); ++i)
    check(same(before.body_records[i],after.body_records[i]),"ordinary refusal changed claims");
}
viewport::Json snapshot(const viewport::CompletedState &state) {
  // This fixture declares aggregate root copies. Display metadata follows the
  // completed live mapping, including newly minted occurrence entries.
  viewport::CaptureContext context{state.report.handle,state.tick,"editable control",{},viewport::Json::object()};
  for (const auto &body : state.report.prepared.particles) {
    const auto id=std::string(body.occurrence.id);
    context.occurrences.emplace(id,viewport::DisplayContext{"aggregate",id,"A"});
  }
  return viewport::snapshot(state,context);
}
void session_case(int workers, field::HostPath path) {
  letter_fixture::Fixture fixture;
  const auto english=fixture.ref(fixture.english);
  const auto source=std::make_shared<const ActiveAssembly>(ActiveAssembly::assemble(
      viewport::prepared_english_pair(fixture.base,english),warm::new_uuid(),false));
  auto request=viewport::editable_english_pair_submission(source,english,path);
  for (const auto &body : request.bindings.bodies) check(body.mass == 2.,"operator preset changed baseline mass");
  const auto first=request.bindings.bodies[0].occurrence, second=request.bindings.bodies[1].occurrence;
  engine::InstanceSettings settings; settings.read_environment=false; settings.cpu_threads=workers;
  ActiveSession session(settings);
  auto submitted=session.submit_prepared(std::move(request),source->owner());
  check(bool(submitted.handle) && !session.on_cuda(),"editable CPU submission failed");
  const auto handle=*submitted.handle;
  const auto observe=[&] {
    auto state=viewport::observe(session,source->owner(),handle);
    check(state.report.prepared_generation == state.prepared_generation &&
          state.report.prepared_completed_tick == std::optional<std::uint64_t>(state.tick),
          "completed observation lost its report stamp");
    return state;
  };
  auto state=observe();
  field::Parameters64 parameters;
  check(session.tick_prepared(handle,source->owner(),parameters).status == field::TickStatus::Ok,
        "initial tick failed");
  state=observe();
  session.accept(state.report,source->owner());
  const auto physical=state.body_records[find(state,first)];
  check(state.report.particles64[field::kVelX*state.report.prepared.capacity+physical.record] != 0,
        "survivor was not moving before edit");
  const auto original=state.body_records[find(state,second)];
  PreparedOccurrenceEdit edit{second,false,*state.prepared_generation};
  rejects([&] { session.edit_prepared(handle,warm::new_uuid(),edit); });
  auto wrong=handle; wrong.generation=warm::new_uuid();
  rejects([&] { session.edit_prepared(wrong,source->owner(),edit); });
  auto invalid=edit; invalid.occurrence.study=warm::new_uuid();
  check(session.edit_prepared(handle,source->owner(),invalid).publication.status == UpdateStatus::InvalidInput,
        "foreign study occurrence accepted");
  invalid=edit; invalid.occurrence.source.path.push_back(0);
  check(session.edit_prepared(handle,source->owner(),invalid).publication.status == UpdateStatus::InvalidInput,
        "different occurrence path accepted");
  invalid=edit; invalid.expected_generation--;
  check(no_new([&] { return session.edit_prepared(handle,source->owner(),invalid); }).publication.status == UpdateStatus::Stale,
        "stale native generation accepted");
  invalid=edit; invalid.preparation_work=0;
  check(no_new([&] { return session.edit_prepared(handle,source->owner(),invalid); }).preparation.status ==
        detail::PreparationStatus::WorkExhausted,"short preparation work accepted");
  unchanged(state,observe());
  {
    PreparedOccurrenceEdit withdraw{first,false,*state.prepared_generation};
    check(no_new([&] { return session.edit_prepared(handle,source->owner(),withdraw); }).publication.status == UpdateStatus::Ok,
          "declared destination withdrawal failed");
    auto absent=observe(); survivor(state,absent,second);
    OccurrenceKey replacement{source->study(),{30,{}}};
    PreparedOccurrenceEdit recycle{replacement,true,*absent.prepared_generation}; recycle.clone_from=second;
    check(no_new([&] { return session.edit_prepared(handle,source->owner(),recycle); }).publication.status == UpdateStatus::Ok,
          "declared destination metadata slot was not recycled");
    auto recycled=observe(); survivor(absent,recycled,second);
    PreparedOccurrenceEdit restore{first,true,*recycled.prepared_generation}; restore.clone_from=second;
    const auto mismatch=no_new([&] { return session.edit_prepared(handle,source->owner(),restore); });
    check(mismatch.publication.status == UpdateStatus::InvalidInput &&
          !mismatch.publication.claims.sampled &&
          mismatch.publication.generation == *recycled.prepared_generation,
          "evicted declared destination accepted a different template");
    const auto rejected=observe(); unchanged(recycled,rejected);
    survivor(recycled,rejected,second); survivor(recycled,rejected,replacement);
    session.accept(recycled.report,source->owner());
    restore.clone_from=first;
    check(no_new([&] { return session.edit_prepared(handle,source->owner(),restore); }).publication.status == UpdateStatus::Ok,
          "evicted declared destination could not restore its original template");
    auto restored=observe(); survivor(recycled,restored,second); survivor(recycled,restored,replacement);
    check(restored.body_records.size() == 3 &&
          *restored.prepared_generation == *recycled.prepared_generation+1,
          "declared destination restoration lost coherent identity publication");
    restore.expected_generation=*restored.prepared_generation;
    check(no_new([&] { return session.edit_prepared(handle,source->owner(),restore); }).publication.status == UpdateStatus::NoChange,
          "restored declared destination lost its original template mapping");
    restore.clone_from=second;
    check(no_new([&] { return session.edit_prepared(handle,source->owner(),restore); }).publication.status == UpdateStatus::InvalidInput,
          "resident declared destination accepted a different template");
    unchanged(restored,observe());
    restore.clone_from.reset();
    check(no_new([&] { return session.edit_prepared(handle,source->owner(),restore); }).publication.status == UpdateStatus::NoChange,
          "plain add rejected the restored declared destination");
    recycle.present=false; recycle.clone_from.reset(); recycle.expected_generation=*restored.prepared_generation;
    check(no_new([&] { return session.edit_prepared(handle,source->owner(),recycle); }).publication.status == UpdateStatus::Ok,
          "recycled destination regression cleanup failed");
    state=observe(); survivor(restored,state,first); survivor(restored,state,second);
  }
  for (int round=0; round<3; ++round) {
    edit.preparation_work=UINT64_MAX;
    edit.present=false; edit.expected_generation=*state.prepared_generation;
    auto result=no_new([&] { return session.edit_prepared(handle,source->owner(),edit); });
    check(result.publication.status == UpdateStatus::Ok,"removal failed");
    auto removed=observe();
    check(removed.body_records.size() == 1 && *removed.prepared_generation == *state.prepared_generation+1,
          "removal not coherently observable");
    survivor(state,removed,first);
    rejects([&] { session.accept(state.report,source->owner()); });
    check(snapshot(removed).is_object(),"removed occurrence remained in display coverage");
    edit.expected_generation=*removed.prepared_generation;
    check(no_new([&] { return session.edit_prepared(handle,source->owner(),edit); }).publication.status == UpdateStatus::NoChange,
          "repeated removal not a no-op");
    auto last=edit; last.occurrence=first;
    check(session.edit_prepared(handle,source->owner(),last).publication.status == UpdateStatus::Unsupported,
          "last supported cohort was removed");
    unchanged(removed,observe());
    session.accept(removed.report,source->owner());
    edit.present=true;
    auto short_placement=edit; short_placement.placement_work=0;
    const auto measured=no_new([&] { return session.edit_prepared(handle,source->owner(),short_placement); });
    check(measured.publication.status == UpdateStatus::CapacityExhausted,"short placement work accepted");
    auto short_preparation=edit; short_preparation.preparation_work=measured.preparation.charged-1;
    check(no_new([&] { return session.edit_prepared(handle,source->owner(),short_preparation); }).preparation.status ==
          detail::PreparationStatus::WorkExhausted,"one-short edit preparation accepted");
    unchanged(removed,observe());
    edit.preparation_work=measured.preparation.charged;
    result=no_new([&] { return session.edit_prepared(handle,source->owner(),edit); });
    check(result.publication.status == UpdateStatus::Ok,"addition failed");
    auto added=observe();
    check(added.body_records.size() == 2 && !same(original,added.body_records[find(added,second)]) &&
          *added.prepared_generation == *removed.prepared_generation+1,"addition reused stale claim or publication");
    survivor(removed,added,first);
    check(source->shared_recipe().tools.size() == fixture.base.tools.size() &&
          source->shared_recipe().selections.size() == 2,"edit destroyed reusable source definitions");
    check(snapshot(added).is_object(),"existing completed snapshot failed");
    check(session.tick_prepared(handle,source->owner(),parameters).status == field::TickStatus::Ok,
          "tick after addition failed");
    state=observe();
    rejects([&] { session.accept(added.report,source->owner()); });
    check(state.tick == added.tick+1 && state.body_records.size() == 2,"added occurrence lost during tick");
  }
  OccurrenceKey created{source->study(),{20,{}}};
  PreparedOccurrenceEdit add{created,true,*state.prepared_generation}; add.clone_from=first;
  const auto clone=no_new([&] { return session.edit_prepared(handle,source->owner(),add); });
  check(clone.publication.status == UpdateStatus::Ok,"new composed occurrence entry was not admitted");
  check(clone.publication.claims.sampled && clone.publication.claims.particles_before == 2 &&
        clone.publication.claims.particles_peak == 3 && clone.publication.claims.particles_after == 3 &&
        clone.publication.claims.fields_before == 2 && clone.publication.claims.fields_peak == 2 &&
        clone.publication.claims.fields_after == 2,"admission lost native claim observation");
  auto three=observe();
  check(three.body_records.size() == 3 && snapshot(three).is_object(),"new live occurrence not observable");
  survivor(state,three,first); survivor(state,three,second);
  const auto fresh=three.body_records[find(three,created)];
  check(!same(fresh,three.body_records[find(three,first)]) && !same(fresh,three.body_records[find(three,second)]),
        "new occurrence aliased an existing claim");
  bool pinned=false;
  for (const auto &origin : three.report.origins)
    if (origin.occurrence == created && origin.shared == std::optional<warm::ToolRef>(english)) pinned=true;
  check(pinned,"new occurrence lost its exact declared definition pin");
  check(session.tick_prepared(handle,source->owner(),parameters).status == field::TickStatus::Ok,
        "tick with new occurrence failed");
  const auto after_tick=observe();
  PreparedOccurrenceEdit remove{created,false,*after_tick.prepared_generation};
  check(no_new([&] { return session.edit_prepared(handle,source->owner(),remove); }).publication.status == UpdateStatus::Ok,
        "new occurrence removal failed");
  const auto two=observe(); survivor(after_tick,two,first); survivor(after_tick,two,second);
  check(two.body_records.size() == 2 && snapshot(two).is_object(),"removed new entry remained visible");
  check(session.tick_prepared(handle,source->owner(),parameters).status == field::TickStatus::Ok,
        "tick after new entry removal failed");
  state=observe();
  add.expected_generation=*state.prepared_generation;
  check(no_new([&] { return session.edit_prepared(handle,source->owner(),add); }).publication.status == UpdateStatus::Ok,
        "definition was not reusable after new entry removal");
  three=observe();
  check(!same(fresh,three.body_records[find(three,created)]),"re-added entry reused a stale incarnation");
  remove.expected_generation=*three.prepared_generation;
  check(session.edit_prepared(handle,source->owner(),remove).publication.status == UpdateStatus::Ok,"clone cleanup failed");
  state=observe();
  edit.present=false; edit.expected_generation=*state.prepared_generation;
  check(session.edit_prepared(handle,source->owner(),edit).publication.status == UpdateStatus::Ok,"final removal failed");
  const auto before_tick=observe();
  check(session.tick_prepared(handle,source->owner(),parameters).status == field::TickStatus::Ok,
        "survivor tick after removal failed");
  check(observe().tick == before_tick.tick+1 && observe().body_records.size() == 1,
        "same study did not continue after removal");
  std::cout << "prepared_occurrence_edit,workers=" << workers << ",path="
      << (path == field::HostPath::BulkCopy ? "bulk" : "direct")
      << ",rounds=3,new_root_entry=20,add_tick_remove_tick=1,source_pins=preserved,"
         "survivor_bits=preserved,stale_reports=rejected,declared_destination_after_reuse=preserved,recurring_new=0 PASS\n";
}

}
int main(int argc, char **argv) {
  try {
    const int workers=argc>1 ? std::stoi(argv[1]) : 1;
    const auto path=argc>2 && std::string(argv[2]) == "direct" ? field::HostPath::CpuDirect : field::HostPath::BulkCopy;
    session_case(workers,path); return 0;
  } catch (const std::exception &e) { std::cerr << "prepared edit: " << e.what() << '\n'; return 1; }
}
