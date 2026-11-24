#include "letter_projection_fixture.h"
#include "allocation_observation.h"
#include "prepared_letter_profile.h"
#include "snapshot.h"
#include "assembly/prepared_occurrence_edit.h"
#include "assembly/prepared_study_owner.h"
#include "prepared_fixture.h"
#include "prepared_metrics.h"
#include "field/cascade_source.h"
#include <cstring>
#include <iostream>

namespace {
using namespace assembly;
using field::UpdateStatus;
void check(bool v,const char *message){if(!v)throw std::runtime_error(message);}
bool same(const engine::RecordHandle &a,const engine::RecordHandle &b){return field::detail::same_handle(a,b);}
template<class F>auto no_new(F f){
  allocation_observation::calls=0;allocation_observation::enabled=true;
  try{auto r=f();allocation_observation::enabled=false;check(!allocation_observation::calls,"whole-field edit allocated recurring C++ storage");return r;}
  catch(...){allocation_observation::enabled=false;throw;}
}
template<class F>void rejects(F f){bool rejected=false;try{f();}catch(const warm::Error &){rejected=true;}check(rejected,"foreign/stale whole-field access accepted");}
struct Input {
  letter_fixture::Fixture fixture;
  warm::ToolRef english,focus;
  std::shared_ptr<const ActiveAssembly> source;
  Input():english(fixture.ref(fixture.english)),focus(fixture.add(letter_fixture::object("02.00.00.00.00"),"English",0)) {
    fixture.base.tools.at(fixture.tools.at(fixture.english)).member_of={focus};
    const auto mixed=ActiveAssembly::assemble(viewport::whole_english_pair(fixture.base,english,focus),warm::new_uuid(),false);
    source=std::make_shared<const ActiveAssembly>(mixed.expose_all(warm::Exposure::Aggregate));
  }
  auto request(field::HostPath path)const{return viewport::whole_english_pair_submission(source,english,focus,path);}
};
engine::RecordHandle body(const viewport::CompletedState &s,std::uint64_t entry) {
  warm::OccurrenceId id{entry,{}};
  if(s.report.prepared_exposure==warm::Exposure::Parts)id.path={0,0};
  const auto text=occurrence_text(s.report.handle.study,id);
  for(std::size_t i=0;i<s.body_records.size();++i)
    if(std::string(s.report.prepared.particles[i].occurrence.id)==text)return s.body_records[i];
  throw std::runtime_error("missing full logical occurrence at selected exposure");
}
double value(const viewport::CompletedState &s,const engine::RecordHandle &h,int field) {
  return s.report.particles64.at(field*s.report.prepared.capacity+h.record);
}
engine::RecordHandle focused(const viewport::CompletedState &s) {
  for(const auto &node:s.comparisons)if(node.comparison==102)return node.field;
  throw std::runtime_error("membership focus comparison missing");
}
void unchanged(const viewport::CompletedState &a,const viewport::CompletedState &b) {
  check(a.tick==b.tick && a.prepared_generation==b.prepared_generation &&
    a.report.prepared_exposure==b.report.prepared_exposure && a.body_records.size()==b.body_records.size() &&
    a.report.particles64.size()==b.report.particles64.size() &&
    !std::memcmp(a.report.particles64.data(),b.report.particles64.data(),a.report.particles64.size()*sizeof(double)) &&
    a.report.groups64==b.report.groups64,"refused whole-field request changed completed state");
  for(std::size_t i=0;i<a.body_records.size();++i)check(same(a.body_records[i],b.body_records[i]),"refusal changed native claim");
  check(a.comparisons.size()==b.comparisons.size(),"refusal changed comparison count");
  for(std::size_t i=0;i<a.comparisons.size();++i)
    check(a.comparisons[i].comparison==b.comparisons[i].comparison && same(a.comparisons[i].field,b.comparisons[i].field),
          "refusal changed full field claim");
  check(same(focused(a),focused(b)),"refusal changed inherited focus");
}
void survivor(const viewport::CompletedState &a,const viewport::CompletedState &b,std::uint64_t entry) {
  auto x=body(a,entry),y=body(b,entry);check(same(x,y) && a.tick==b.tick,"ordinary edit replaced a survivor");
  for(int k=0;k<field::kParticleFieldCount;++k){const auto p=value(a,x,k),q=value(b,y,k);check(!std::memcmp(&p,&q,sizeof(p)),"ordinary edit changed survivor bits");}
}
void transferred(const viewport::CompletedState &a,const viewport::CompletedState &b) {
  check(a.tick==b.tick && *b.prepared_generation==*a.prepared_generation+1 &&
        same(focused(a),focused(b)),"whole-field generation/tick/focus publication was not coherent");
  for(auto entry:{std::uint64_t(2),std::uint64_t(10)}) {
    const auto old=body(a,entry),fresh=body(b,entry);check(!same(old,fresh),"LoD exchange reused outgoing incarnation");
    for(int k=0;k<3;++k) {
      const auto x=value(a,old,field::kVelX+k),y=value(b,fresh,field::kVelX+k);
      check(!std::memcmp(&x,&y,sizeof(x)),"logical occurrence received averaged or reset velocity");
      if(*b.report.prepared_exposure==warm::Exposure::Parts) {
        const auto p=value(a,old,field::kPosX+k),q=value(b,fresh,field::kPosX+k);
        check(!std::memcmp(&p,&q,sizeof(p)),"unblocked expansion did not target its own outgoing centre");
      }
    }
    check(value(b,fresh,field::kMass)==2,"whole-field exchange changed physical mass");
  }
}
void snapshot(const viewport::CompletedState &s) {
  viewport::CaptureContext c{s.report.handle,s.tick,"whole-field",{},viewport::Json::object()};
  for(const auto &p:s.report.prepared.particles) {
    const auto id=std::string(p.occurrence.id);std::string root=id;
    if(*s.report.prepared_exposure==warm::Exposure::Parts)root=root.substr(0,root.size()-4);
    c.occurrences.emplace(id,viewport::DisplayContext{*s.report.prepared_exposure==warm::Exposure::Parts?"parts":"aggregate",root,"A"});
  }
  check(viewport::snapshot(s,c).is_object(),"completed whole-field snapshot rejected");
}
void run(int workers,field::HostPath path,bool short_peak) {
  Input input;auto request=input.request(path);
  if(short_peak) {
    auto limits=request.capacity.limits();limits[field::Budget::Particles]=3;
    limits[field::Budget::ScratchBytes]=field::detail::PreparedStorage::required_scratch(limits,false,request.capacity.profile())+256*1024*1024;
    request.capacity=field::PreparedCapacity(limits,request.capacity.profile());
  }
  const auto resources=request.resources;
  engine::InstanceSettings settings;settings.read_environment=false;settings.cpu_threads=workers;
  ActiveSession session(settings);auto submitted=session.submit_prepared(std::move(request),input.source->owner());
  check(bool(submitted.handle) && !session.on_cuda(),"whole-field native CPU setup failed");
  const auto h=*submitted.handle;
  const auto observe=[&]{return viewport::observe(session,input.source->owner(),h);};
  field::Parameters64 parameters;parameters.contact_stiffness=0;
  check(session.tick_prepared(h,input.source->owner(),parameters).status==field::TickStatus::Ok,"initial moving tick failed");
  auto state=observe();const auto focus=focused(state);
  check(value(state,body(state,2),field::kVelX)!=value(state,body(state,10),field::kVelX) &&
        value(state,body(state,2),field::kVelX)!=0 && value(state,body(state,10),field::kVelX)!=0,
        "two-occurrence witness lacks distinct outgoing motion");
  PreparedLodEdit edit{warm::Exposure::Parts,*state.prepared_generation};
  if(short_peak) {
    const auto refused=no_new([&]{return session.lod_prepared(h,input.source->owner(),edit);});
    check(refused.publication.status==UpdateStatus::CapacityExhausted &&
      refused.publication.failure.budget==field::Budget::Particles && refused.publication.failure.required==4 &&
      !refused.publication.claims.sampled,"final-fit/old-plus-incoming peak did not refuse");
    unchanged(state,observe());session.accept(state.report,input.source->owner());
    check(session.tick_prepared(h,input.source->owner(),parameters).status==field::TickStatus::Ok,"peak refusal prevented following tick");
    std::cout<<"whole_field_peak_refusal,capacity=3,final=2,required=4,unchanged=1,following_tick=1 PASS\n";return;
  }
  rejects([&]{session.lod_prepared(h,warm::new_uuid(),edit);});
  auto bad=h;bad.generation=warm::new_uuid();rejects([&]{session.lod_prepared(bad,input.source->owner(),edit);});
  auto stale=edit;--stale.expected_generation;
  check(no_new([&]{return session.lod_prepared(h,input.source->owner(),stale);}).publication.status==UpdateStatus::Stale,"stale LoD generation accepted");
  const auto live_bytes=resources->result().live_bytes;
  for(int round=0;round<3;++round)for(const auto exposure:{warm::Exposure::Parts,warm::Exposure::Aggregate}) {
    edit={exposure,*state.prepared_generation};edit.preparation_work=0;
    check(no_new([&]{return session.lod_prepared(h,input.source->owner(),edit);}).preparation.status==detail::PreparationStatus::WorkExhausted,
          "zero whole-field preparation work accepted");
    unchanged(state,observe());edit.preparation_work=UINT64_MAX;edit.placement_work=0;
    const auto measured=no_new([&]{return session.lod_prepared(h,input.source->owner(),edit);});
    check(measured.publication.status==UpdateStatus::CapacityExhausted,"zero whole-field placement work accepted");
    unchanged(state,observe());edit.preparation_work=measured.preparation.charged-1;
    check(no_new([&]{return session.lod_prepared(h,input.source->owner(),edit);}).preparation.status==detail::PreparationStatus::WorkExhausted,
          "one-short whole-field metadata allowance accepted");
    unchanged(state,observe());edit.preparation_work=measured.preparation.charged;edit.placement_work=UINT64_MAX;
    const auto result=no_new([&]{return session.lod_prepared(h,input.source->owner(),edit);});
    check(result.publication.status==UpdateStatus::Ok,"whole-field exchange failed");
    auto after=observe();check(after.report.prepared_exposure==exposure && same(focus,focused(after)),"LoD exposure or focus lost");
    transferred(state,after);snapshot(after);rejects([&]{session.accept(state.report,input.source->owner());});
    const auto &c=result.publication.claims;
    check(c.sampled && c.particles_before==2 && c.particles_peak==4 && c.particles_after==2 && c.fields_peak==6 &&
      c.fields_before==(exposure==warm::Exposure::Parts?3:4) && c.fields_after==(exposure==warm::Exposure::Parts?4:3),
      "whole-field receipt omitted simultaneous old/incoming claims");
    edit.expected_generation=*after.prepared_generation;
    check(no_new([&]{return session.lod_prepared(h,input.source->owner(),edit);}).publication.status==UpdateStatus::NoChange,"same whole-field exposure was not a no-op");
    unchanged(after,observe());
    check(resources->result().live_bytes==live_bytes,"whole-field commit grew retained metadata");
    if(exposure==warm::Exposure::Parts) {
      PreparedOccurrenceEdit add{{h.study,{0,{}}},true,*after.prepared_generation};add.clone_from=OccurrenceKey{h.study,{2,{}}};
      check(no_new([&]{return session.edit_prepared(h,input.source->owner(),add);}).publication.status==UpdateStatus::Ok,"fine-exposure admission failed");
      auto three=observe();survivor(after,three,2);survivor(after,three,10);snapshot(three);
      PreparedLodEdit over{warm::Exposure::Aggregate,*three.prepared_generation};
      const auto refused=no_new([&]{return session.lod_prepared(h,input.source->owner(),over);});
      check(refused.publication.status==UpdateStatus::CapacityExhausted && refused.publication.failure.budget==field::Budget::Particles &&
            refused.publication.failure.required==6,"three-occurrence peak did not refuse");
      unchanged(three,observe());add.present=false;add.clone_from.reset();add.expected_generation=*three.prepared_generation;
      check(no_new([&]{return session.edit_prepared(h,input.source->owner(),add);}).publication.status==UpdateStatus::Ok,"fine-exposure removal failed");
      after=observe();survivor(three,after,2);survivor(three,after,10);
    }
    check(session.tick_prepared(h,input.source->owner(),parameters).status==field::TickStatus::Ok,"tick after whole-field exchange failed");
    state=observe();
  }
  std::cout<<"prepared_whole_field,workers="<<workers<<",path="<<(path==field::HostPath::BulkCopy?"bulk":"direct")
    <<",round_trips=3,distinct_motion=1,own_targets=1,one_generation=1,inherited_focus=same_claim,peak_particles=4,peak_fields=6,"
      "ordinary_fine_edits=1,refusal_unchanged=1,recurring_new=0 PASS\n";
}
void probe_budgets(int workers,field::HostPath path) {
  Input input;auto request=input.request(path);
  const auto slots=request.capacity.limits()[field::Budget::Particles];
  const auto declarations=request.bindings.bodies.size();
  const auto resources=request.resources;
  engine::InstanceSettings settings;settings.read_environment=false;settings.cpu_threads=workers;
  ActiveSession session(settings);auto submitted=session.submit_prepared(std::move(request),input.source->owner());
  check(bool(submitted.handle),"probe-budget study setup failed");
  const auto h=*submitted.handle;
  const auto observe=[&]{return viewport::observe(session,input.source->owner(),h);};
  auto state=observe();
  check(no_new([&]{return session.lod_prepared(h,input.source->owner(),{warm::Exposure::Parts,*state.prepared_generation});}).publication.status==UpdateStatus::Ok,
        "probe-budget fine exposure failed");
  state=observe();
  for(const auto entry:{std::uint64_t(20),std::uint64_t(30)}) {
    PreparedOccurrenceEdit add{{h.study,{entry,{}}},true,*state.prepared_generation};add.clone_from=OccurrenceKey{h.study,{2,{}}};
    check(no_new([&]{return session.edit_prepared(h,input.source->owner(),add);}).publication.status==UpdateStatus::Ok,
          "probe-budget full catalogue admission failed");
    state=observe();
  }
  check(slots==4 && state.body_records.size()==slots,"probe-budget fixture is not full");
  const auto live_bytes=resources->result().live_bytes;
  // Two study-key equality gates, two immutable declaration scans, and the
  // catalogue/free-slot outer visits. This was the reviewed unmetered bound.
  const auto outer_work=2*(1+2*h.study.str().size())+2*declarations+2*slots;
  const auto full_probes=slots*(slots+1)/2;
  PreparedOccurrenceEdit fresh{{h.study,{40,{}}},true,*state.prepared_generation};fresh.clone_from=OccurrenceKey{h.study,{2,{}}};
  fresh.preparation_work=outer_work;
  const auto short_full=no_new([&]{return session.edit_prepared(h,input.source->owner(),fresh);});
  check(short_full.preparation.status==detail::PreparationStatus::WorkExhausted &&
        !short_full.publication.claims.sampled,"full catalogue accepted outer-only work without charging live probes");
  unchanged(state,observe());session.accept(state.report,input.source->owner());
  fresh.preparation_work=outer_work+full_probes;
  const auto full=no_new([&]{return session.edit_prepared(h,input.source->owner(),fresh);});
  check(full.preparation.status==detail::PreparationStatus::Ok && full.preparation.charged==fresh.preparation_work &&
        full.publication.status==UpdateStatus::CapacityExhausted && full.publication.failure.budget==field::Budget::Particles &&
        full.publication.failure.required==slots+1 && !full.publication.claims.sampled,"exact full-probe work did not reach the healthy slot refusal");
  unchanged(state,observe());
  PreparedOccurrenceEdit remove{{h.study,{30,{}}},false,*state.prepared_generation};
  check(no_new([&]{return session.edit_prepared(h,input.source->owner(),remove);}).publication.status==UpdateStatus::Ok,
        "last metadata slot removal failed");
  state=observe();fresh.expected_generation=*state.prepared_generation;
  // Slots 0..N-2 are found after 1..N-1 comparisons; the late free slot and
  // its separate activity lookup each inspect all N-1 live entries.
  const auto late_probes=(slots-1)*slots/2+2*(slots-1);
  fresh.preparation_work=outer_work+late_probes;
  const auto short_late=no_new([&]{return session.edit_prepared(h,input.source->owner(),fresh);});
  check(short_late.preparation.status==detail::PreparationStatus::WorkExhausted &&
        short_late.preparation.charged==fresh.preparation_work && short_late.preparation.next_required==fresh.preparation_work+1 &&
        !short_late.publication.claims.sampled,"late-free work did not include every free-slot/activity probe");
  unchanged(state,observe());session.accept(state.report,input.source->owner());
  fresh.preparation_work=UINT64_MAX;
  const auto accepted=no_new([&]{return session.edit_prepared(h,input.source->owner(),fresh);});
  check(accepted.preparation.status==detail::PreparationStatus::Ok && accepted.publication.status==UpdateStatus::Ok,
        "late-free sufficient-work retry failed");
  const auto after=observe();
  for(const auto entry:{std::uint64_t(2),std::uint64_t(10),std::uint64_t(20)})survivor(state,after,entry);
  check(resources->result().live_bytes==live_bytes,"probe-budget retries grew metadata storage");
  field::Parameters64 parameters;parameters.contact_stiffness=0;
  check(session.tick_prepared(h,input.source->owner(),parameters).status==field::TickStatus::Ok,"tick after probe-budget retry failed");
  std::cout<<"whole_field_probe_budget,slots="<<slots<<",outer_work="<<outer_work<<",full_probes="<<full_probes
    <<",full_required="<<full.preparation.charged<<",late_probes="<<late_probes<<",late_prefix="<<short_late.preparation.charged
    <<",late_next="<<short_late.preparation.next_required<<",retry_work="<<accepted.preparation.charged
    <<",unchanged_refusals=1,sufficient_retry=1,following_tick=1,recurring_new=0 PASS\n";
}
void setup_and_failure(int workers,field::HostPath path) {
  using namespace assembly::detail;
  using field::testing::PreparedFixtureAccess;
  Input input;auto request=input.request(path);
  engine::InstanceSettings settings;settings.read_environment=false;settings.cpu_threads=workers;
  engine::Runtime runtime(settings);
  auto root=std::make_unique<taichi::lang::SNode>(0,taichi::lang::SNodeType::root);
  auto *primary=&root->dense(taichi::lang::Axis(0),1);
  primary->insert_children(taichi::lang::SNodeType::place).dt=taichi::lang::PrimitiveType::i32;
  for(auto *bindings:{&request.bindings,&request.lod_alternative->bindings}) {
    bindings->layout[0].node.layout=root.get();bindings->layout[1].node.layout=primary;
  }
  runtime.program().add_snode_tree(std::move(root),false);
  field::Harness h(runtime,request.capacity,path);
  const auto session=warm::new_uuid();
  const auto domain=[](ReservationBudget budget= {UINT64_MAX,UINT64_MAX}) {return std::make_shared<ReservationDomain>(budget);};
  const auto prepare=[&](const auto &d,std::uint64_t plan=UINT64_MAX,std::uint64_t metadata=UINT64_MAX) {
    return PreparedStudyOwner::prepare(h,input.source,session,request.bindings,d,plan,metadata,true,&*request.lod_alternative);
  };
  auto full=domain();auto witness=prepare(full);
  check(witness.status==AdmissionPlanStatus::Ok && !witness.owner->ready(),"whole-field setup witness failed");
  const auto bytes=full->result().peak_bytes,allocations=full->result().charged_work;
  const auto plan=witness.plan_work.charged,metadata=witness.metadata_work.charged;
  witness.owner.reset();check(!full->result().live_bytes,"whole-field setup teardown retained allocations");
  // Published reviewed fixture metadata allowance, before address scans were
  // metered. The same caller limit must now refuse and release all metadata.
  auto reviewed_domain=domain();auto reviewed_bound=prepare(reviewed_domain,plan,28541);
  check(!reviewed_bound.owner && reviewed_bound.status==AdmissionPlanStatus::WorkExhausted &&
        !reviewed_domain->result().live_bytes && h.ready() && !h.prepared_storage().populated,
        "reviewed setup allowance still omits address probes");
  for(int budget=0;budget<4;++budget) {
    auto d=domain({budget==0?bytes-1:UINT64_MAX,budget==1?allocations-1:UINT64_MAX});
    auto refused=prepare(d,budget==2?plan-1:plan,budget==3?metadata-1:metadata);
    check(!refused.owner && !d->result().live_bytes &&
      refused.status==(budget<2?AdmissionPlanStatus::AllocationExhausted:AdmissionPlanStatus::WorkExhausted) &&
      h.ready() && !h.prepared_storage().populated && !h.prepared_storage().generation,
      "one-short whole-field setup failed to unwind before claims");
    if(budget==2)check(refused.plan_work.next_required==plan,"combined alternate plan did not report total next work");
    auto exact=domain({budget==0?bytes:UINT64_MAX,budget==1?allocations:UINT64_MAX});
    auto accepted=prepare(exact,plan,metadata);
    check(accepted.status==AdmissionPlanStatus::Ok,"exact whole-field setup budget failed");
    accepted.owner.reset();check(!exact->result().live_bytes,"exact whole-field setup teardown retained allocations");
  }
  auto d=domain({bytes,allocations});auto prepared=prepare(d,plan,metadata);
  check(prepared.status==AdmissionPlanStatus::Ok,"whole-field exact combined setup failed");
  auto &owner=*prepared.owner;const auto handle=owner.handle();
  prepared_test::Metrics metrics(runtime,PreparedFixtureAccess::nodes(h));const auto backing=metrics.sample();
  const auto boot=no_new([&]{return owner.bootstrap(input.source->owner(),handle,UINT64_MAX,UINT64_MAX,UINT64_MAX);});
  check(boot.status==UpdateStatus::Ok && owner.ready(),"whole-field direct owner bootstrap failed");
  const auto loan=owner.associations(input.source->owner(),handle);
  const auto edit=PreparedLodEdit{warm::Exposure::Parts,owner.generation()};
  check(no_new([&]{return owner.lod(input.source->owner(),handle,edit);}).publication.status==UpdateStatus::Ok &&
        backing.same_backing(metrics.sample()),"whole-field native backing or compilation grew");
  rejects([&]{loan.read();});
  const auto before_failure=owner.associations(input.source->owner(),handle);
  PreparedFixtureAccess::inject_failure(h,7);bool threw=false;
  try{owner.lod(input.source->owner(),handle,{warm::Exposure::Aggregate,owner.generation()});}
  catch(const std::runtime_error &){threw=true;}
  check(threw && !h.ready() && !owner.ready() && owner.phase()==PreparedOwnerPhase::Failed,
        "postclaim whole-field owner publication failure remained healthy");
  rejects([&]{before_failure.read();});
  check(owner.lod(input.source->owner(),handle,{warm::Exposure::Aggregate,owner.generation()}).publication.status==UpdateStatus::Unready,
        "failed whole-field owner allowed a healthy retry");
  prepared.owner.reset();check(!d->result().live_bytes,"failed whole-field owner retained domain allocations");
  std::cout<<"whole_field_setup,peak_bytes="<<bytes<<",allocation_work="<<allocations<<",plan_work="<<plan
    <<",metadata_work="<<metadata<<",one_short_refusals=4,exact_retries=4,unwind=1,backing=stable,postclaim_unready=1 PASS\n";
}
}
int main(int argc,char **argv){try{
  const int workers=argc>1?std::stoi(argv[1]):1;
  const auto path=argc>2&&std::string(argv[2])=="direct"?field::HostPath::CpuDirect:field::HostPath::BulkCopy;
  if(argc>3 && std::string(argv[3])=="probes"){probe_budgets(workers,path);return 0;}
  if(argc>3 && std::string(argv[3])=="setup"){setup_and_failure(workers,path);return 0;}
  run(workers,path,false);run(workers,path,true);setup_and_failure(workers,path);probe_budgets(workers,path);return 0;
}catch(const std::exception &e){std::cerr<<"whole-field test: "<<e.what()<<'\n';return 1;}}
