// Fixture-owned worked trace. Numerical references below are test assertions,
// never an operational solver or a source of native starts.
#include "snapshot.h"
#include "capture_options.h"
#include "prepared_owner_fixture.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <set>

namespace {
using namespace assembly;
using namespace assembly::detail;
using viewport::Json;
int checks=0;
void check(bool ok,const char *why) { if(!ok) throw std::runtime_error(why); ++checks; }
Json pin(const warm::ToolRef &p) { return {{"object",warm::key(p.object)},{"revision",p.revision.str()}}; }
Json key(const FieldKey &f) { return {{"kind",int(f.kind)},{"object",warm::key(f.object)}}; }
std::string name(const FieldKey &f) { return std::to_string(int(f.kind))+":"+warm::key(f.object); }
Json claim(const engine::RecordHandle &h) {
  return {{"context",std::to_string(h.context)},{"incarnation",std::to_string(h.incarnation)},
          {"address",h.address},{"record",h.record}};
}
const warm::Tool &tool(const warm::Bundle &b,const warm::ToolRef &ref) {
  for(const auto &t:b.tools) if(t.ref==ref) return t;
  throw std::runtime_error("missing exact source pin");
}
struct Input {
  std::shared_ptr<const ActiveAssembly> source;
  AdmissionBindings bindings;
  Json trace=Json::object();
  bool pair,parts,context;
  Input(engine::Runtime &runtime,const letter_fixture::Fixture &fixture,
        const std::string &profile,bool fine,bool imposed):
      pair(profile=="ascii"),parts(fine),context(imposed) {
    auto base=fixture.base;
    const auto object=pair?fixture.english:profile=="accented"?fixture.accented:fixture.repeated;
    const auto first=fixture.ref(object);
    const auto &endpoint=tool(base,first);
    auto initial_focus=first;
    if(fine) {
      initial_focus=endpoint.parents[0].target;
      if(*tool(base,initial_focus).mass!=2) initial_focus=tool(base,initial_focus).parents[0].target;
    }
    base.focal_fields={{warm::FieldKind::SiblingDefinition,initial_focus.object}};
    std::optional<warm::ToolRef> group;
    if(context) {
      // Context changes get new endpoint revision pins. Structural parents,
      // exact masses and geometry remain byte-for-byte unchanged.
      warm::Tool g{{letter_fixture::object("02.0z.00.00.00"),warm::new_uuid()},fixture.epoch,
        "semantic-trace-fixture",warm::RecipeText("Imposed cross-language A"),0,{}, {},fixture.actor};
      group=g.ref;base.tools.push_back(g);
      for(auto &t:base.tools) if(t.ref.object==fixture.english || t.ref.object==fixture.french) {
        t.ref.revision=warm::new_uuid();t.member_of.push_back(*group);
      }
    }
    const auto ref=[&](const warm::ObjectKey &id) {
      for(const auto &t:base.tools) if(t.ref.object==id) return t.ref;
      throw std::runtime_error("missing endpoint");
    };
    std::vector<StructuralLetterRequest> requests{{2,ref(first.object),fine?warm::Exposure::Parts:warm::Exposure::Aggregate}};
    if(pair) requests.push_back({10,ref(fixture.french),fine?warm::Exposure::Parts:warm::Exposure::Aggregate});
    auto recipe=normalize_structural_letters(base,requests);
    auto projection=project_letters(recipe);
    // These are explicit fixture comparison choices. No production topology
    // is inferred from notation, parent names, or native storage depth.
    const auto add=[&](FieldKey f) {
      for(std::size_t i=0;i<bindings.fields.size();++i) if(name(bindings.fields[i].field)==name(f)) return int(i);
      const int i=int(bindings.fields.size());
      bindings.fields.push_back({f,5,i});bindings.comparisons.push_back({f,std::uint64_t(100+i),0,52,{}});
      return i;
    };
    if(parts || profile=="repeated") {
      for(std::size_t i=0;i<projection.parent_count();++i) add(projection.parent(i).field);
      for(std::size_t i=0;i<projection.occurrence_count();++i) {
        const auto o=projection.occurrence(i);if(!o.active) continue;
        const int whole=add({GroupKind::SiblingDefinition,o.definition.object});
        std::set<int> children;
        for(const auto &p:tool(recipe,o.definition).parents) children.insert(add({GroupKind::ParentValue,p.target.object}));
        bindings.comparisons[whole].children.assign(children.begin(),children.end());
        bindings.comparisons[whole].level=1;
      }
    } else {
      for(std::size_t i=0;i<projection.occurrence_count();++i) {
        const auto o=projection.occurrence(i);if(o.active) add({GroupKind::SiblingDefinition,o.definition.object});
      }
      for(std::size_t i=0;i<projection.parent_count();++i) {
        const auto p=projection.parent(i);const int parent=add(p.field);
        for(std::size_t j=0;j<projection.occurrence_count();++j) {
          const auto o=projection.occurrence(j);if(!o.active || !(o.key==p.occurrence)) continue;
          const int child=add({GroupKind::SiblingDefinition,o.definition.object});
          auto &v=bindings.comparisons[parent].children;
          if(std::find(v.begin(),v.end(),child)==v.end()) v.push_back(child);
        }
        bindings.comparisons[parent].level=1;
      }
    }
    if(group) {const int prior=int(bindings.fields.size())-1;const int i=add({GroupKind::Membership,group->object});
      bindings.comparisons[i].children={prior};bindings.comparisons[i].level=2;}
    const auto focus=bindings.fields[context?bindings.fields.size()-2:bindings.fields.size()-1].field;
    recipe.focal_fields={{static_cast<warm::FieldKind>(focus.kind),focus.object}};
    source=std::make_shared<const ActiveAssembly>(ActiveAssembly::assemble(recipe,warm::new_uuid(),false));
    bindings.source={source->owner(),source->study(),source->generation(),recipe.ref};bindings.focus={focus};
    auto root=std::make_unique<taichi::lang::SNode>(0,taichi::lang::SNodeType::root);
    auto *leaf=root.get();bindings.layout.push_back({{leaf,-1,ScaleRole::Root},1});
    if(parts) {leaf=&root->dense(taichi::lang::Axis(0),1);bindings.layout.push_back({{leaf,0,ScaleRole::PrimaryStep},64});}
    leaf->insert_children(taichi::lang::SNodeType::place).dt=taichi::lang::PrimitiveType::i32;
    trace={{"schema","hcp-semantic-native-trace"},{"version",1},{"profile",profile},
      {"exposure",parts?"parts":"aggregate"},{"imposed_context",context},
      {"definitions",Json::array()},{"occurrences",Json::array()},{"parents",Json::array()},
      {"layouts",Json::array()},{"fields",Json::array()}};
    std::set<std::string> reached;
    const auto describe=[&](const auto &self,const warm::ToolRef &r)->void {
      if(!reached.insert(warm::key(r.object)).second) return;
      const auto &t=tool(recipe,r);Json parents=Json::array(),memberships=Json::array();
      for(const auto &p:t.parents) {parents.push_back({{"target",pin(p.target)},{"ordinal",p.ordinal},
        {"count",std::to_string(p.contribution)}});self(self,p.target);}
      for(const auto &m:t.member_of) {memberships.push_back(pin(m));self(self,m);}
      trace["definitions"].push_back({{"pin",pin(r)},{"mass",std::to_string(*t.mass)},
        {"notation",t.notation?std::string(*t.notation):""},{"parents",parents},{"memberships",memberships}});
    };
    for(const auto &r:requests) describe(describe,r.letter);
    std::map<std::string,std::uint64_t> geometry_ids;
    for(std::size_t i=0;i<projection.occurrence_count();++i) {
      const auto o=projection.occurrence(i);Json byte=nullptr;
      if(o.byte) byte={{"owner_entry",o.byte->owner.source.entry},{"character",pin(o.byte->character)},
        {"character_ordinal",o.byte->character_ordinal},{"byte_ordinal",o.byte->byte_ordinal}};
      trace["occurrences"].push_back({{"entry",o.key.source.entry},{"path",o.key.source.path},
        {"definition",pin(o.definition)},{"active",o.active},{"node",o.node},{"byte",byte}});
      if(o.active) {auto occurrence=o.key;occurrence.study=source->study();
        const auto identity=warm::key(o.definition.object);
        if(!geometry_ids.count(identity))geometry_ids.emplace(identity,geometry_ids.size()+1);
        bindings.bodies.push_back({std::move(occurrence),o.definition,o.node,3,40+int(bindings.bodies.size()),
          double(*tool(recipe,o.definition).mass),{geometry_ids.at(identity),1,true}});}
    }
    for(std::size_t i=0;i<projection.parent_count();++i) {const auto p=projection.parent(i);
      trace["parents"].push_back({{"entry",p.occurrence.source.entry},{"path",p.occurrence.source.path},
        {"field",key(p.field)},{"ordinal",p.ordinal},{"count",std::to_string(p.count)}});}
    check(bindings.bodies.size()==(parts?(pair?2:profile=="accented"?2:4):(pair?2:1)),
          "active exposure has the exact source-derived occurrence count");
    if(pair) {
      const auto &en=tool(recipe,ref(fixture.english)), &fr=tool(recipe,ref(fixture.french));
      check(!(en.ref==fr.ref) && en.parents.size()==1 && fr.parents.size()==1 &&
            en.parents[0].target==fr.parents[0].target && en.parents[0].ordinal==0 && fr.parents[0].ordinal==0,
            "distinct language endpoints retain one shared pinned ASCII character parent");
      check(!parts || (bindings.bodies[0].definition==bindings.bodies[1].definition &&
            bindings.bodies[0].occurrence.source.entry!=bindings.bodies[1].occurrence.source.entry),
            "byte exposure shares a definition without merging occurrence ownership");
    } else if(profile=="repeated") {
      const auto &t=tool(recipe,first);
      check(t.parents.size()==3 && t.parents[0].target==t.parents[1].target &&
            t.parents[0].ordinal==0 && t.parents[1].ordinal==1 && t.parents[2].ordinal==2 &&
            t.parents[0].contribution==2 && t.parents[1].contribution==2 && t.parents[2].contribution==4,
            "ordered repeated ASCII and multibyte scalar parent contributions remain distinct");
    } else {
      const auto &t=tool(recipe,tool(recipe,first).parents[0].target);
      check(t.parents.size()==2 && t.parents[0].ordinal==0 && t.parents[1].ordinal==1 &&
            tool(recipe,t.parents[0].target).notation==warm::RecipeText("C3") &&
            tool(recipe,t.parents[1].target).notation==warm::RecipeText("AD"),
            "multibyte scalar preserves the exact ordered UTF-8 byte construction");
    }
    for(std::size_t i=0;i<bindings.layout.size();++i) {const auto &b=bindings.layout[i];
      trace["layouts"].push_back({{"index",i},{"parent",b.node.parent},{"role",int(b.node.role)},
        {"units",b.units},{"native_snode_id",b.node.layout->id},{"native_snode_type",int(b.node.layout->type)}});}
    for(std::size_t i=0;i<bindings.fields.size();++i) {const auto &f=bindings.comparisons[i];
      trace["fields"].push_back({{"index",i},{"source",key(f.field)},
        {"comparison",std::to_string(f.comparison)},{"level",f.level},{"children",f.children}});}
    std::vector<int> parents(bindings.fields.size());
    for(const auto &c:bindings.comparisons)for(int child:c.children)++parents.at(child);
    Json roots=Json::array(),shared_children=Json::array(),full_fields=Json::array();
    for(std::size_t i=0;i<parents.size();++i) {
      if(parents[i]==0)roots.push_back(i);
      if(parents[i]>1)shared_children.push_back(i);
      const auto &f=bindings.fields[i].field;bool full=true;
      for(const auto &body:bindings.bodies) {
        const auto mass=*tool(recipe,body.definition).mass;std::uint64_t count=0;
        if(f.kind==GroupKind::SiblingDefinition)count=f.object==body.definition.object?mass:0;
        else if(f.kind==GroupKind::Membership)count=mass;
        else for(const auto &p:trace["parents"])if(p.at("entry")==body.occurrence.source.entry &&
          p.at("path")==Json(body.occurrence.source.path) && p.at("field")==key(f))
          count+=std::stoull(p.at("count").get<std::string>());
        full &= count==mass;
      }
      if(full)full_fields.push_back(i);
    }
    trace["tree_witness"]={{"roots",roots},{"shared_children",shared_children},{"full_coverage_fields",full_fields}};
    if(parts && !pair)check(full_fields.empty(),"multibyte/repeated byte ledger has no field able to serve as a complete rooted rollup");
    if(parts && profile=="repeated")check(!shared_children.empty(),"repeated-byte literal tree exposes a shared parent-value field across branches");
    runtime.program().add_snode_tree(std::move(root),false);
  }
};
Json run(engine::Runtime &runtime,const letter_fixture::Fixture &fixture,const std::string &profile,
         bool parts,bool context,field::HostPath path,bool project_only) {
  Input in(runtime,fixture,profile,parts,context);
  auto limits=prepared_owner_fixture::capacity().limits();
  limits[field::Budget::Particles]=8;limits[field::Budget::Fields]=16;
  limits[field::Budget::CascadeNodes]=limits[field::Budget::CascadeLevels]=16;
  limits[field::Budget::CascadeLinks]=32;limits[field::Budget::Participation]=64;
  limits[field::Budget::BootstrapCommands]=128;limits[field::Budget::CascadeReferences]=128;
  limits[field::Budget::CascadeCoverage]=128;limits[field::Budget::LiveResponses]=256;
  limits[field::Budget::ScratchBytes]=field::detail::PreparedStorage::required_scratch(limits,false,field::NumericalProfile::trial())+256*1024*1024;
  const field::PreparedCapacity capacity(limits,field::NumericalProfile::trial());
  auto domain=prepared_owner_fixture::domain();
  const auto plan=StructuralAdmissionPlan::build(*in.source,in.bindings,capacity,domain,UINT64_MAX);
  const bool supported=profile=="ascii" || !parts;
  if(plan.status!=(supported?AdmissionPlanStatus::Ok:AdmissionPlanStatus::InvalidInput))
    std::cerr<<"plan_result,profile="<<profile<<",parts="<<parts<<",context="<<context
      <<",status="<<int(plan.status)<<",work="<<plan.work.charged<<'\n';
  check(plan.status==(supported?AdmissionPlanStatus::Ok:AdmissionPlanStatus::InvalidInput),"unexpected semantic plan admission result");
  in.trace["plan_status"]=supported?"ok":"invalid_input";
  in.trace["plan_work"]=std::to_string(plan.work.charged);
  in.trace["native"]=nullptr;
  if(!supported || project_only) return in.trace;
  in.trace["participation_commands"]=Json::array();
  std::size_t parent_rows=0;
  for(const auto &c:plan.plan->commands()) if(c.operation==field::UpdateOp::InsertParticipation) {
    const AdmissionFieldCommand *f=nullptr;const AdmissionOccurrenceCommand *o=nullptr;
    for(const auto &v:plan.plan->fields())if(v.command==c.second.command)f=&v;
    for(const auto &v:plan.plan->occurrences())if(v.command==c.first.command)o=&v;
    check(f && o && c.exact_count,"source command resolves to actual plan occurrence/field");
    in.trace["participation_commands"].push_back({{"entry",o->occurrence.entry},{"path",o->occurrence.path},
      {"field",key(f->field)},{"ordinal",std::to_string(c.ordinal)},{"count",std::to_string(*c.exact_count)}});
    if(f->field.kind==GroupKind::ParentValue) {
      bool match=false;
      for(const auto &p:in.trace["parents"])if(p["entry"]==o->occurrence.entry && p["path"]==Json(o->occurrence.path) &&
        p["field"]==key(f->field) && p["ordinal"]==c.ordinal && p["count"]==std::to_string(*c.exact_count))match=true;
      check(match,"plan preserves each projected parent ordinal and exact count");++parent_rows;
    }
  }
  check(parent_rows==in.trace["parents"].size(),"plan neither drops nor duplicates parent participation rows");
  field::Harness harness(runtime,capacity,path);
  auto prepared=PreparedStudyOwner::prepare(harness,in.source,warm::new_uuid(),in.bindings,domain,UINT64_MAX,UINT64_MAX);
  check(prepared.status==AdmissionPlanStatus::Ok,"native owner preparation failed");
  auto &owner=*prepared.owner;const auto handle=owner.handle();const auto authorization=in.source->owner();
  check(owner.bootstrap(authorization,handle,UINT64_MAX,UINT64_MAX,UINT64_MAX).status==field::UpdateStatus::Ok,"native bootstrap failed");
  const auto capture=[&] {
    auto state=viewport::observe(owner,authorization,handle,viewport::IntentRequest{1,UINT64_MAX},viewport::ResponseRequest{UINT64_MAX});
    auto context=viewport::context(*in.source,handle,state.tick,"Semantic native fixture trace");
    context.source={{"kind","fixture_only"},{"profile","semantic-parent-native-trace"}};
    Json result={{"snapshot",viewport::snapshot(state,context)}};
    const auto rows=owner.associations(authorization,handle).read();
    result["field_sources"]=Json::array();
    for(std::size_t i=0;i<rows.fields.size();++i) {
      const auto f=rows.field(i);const int slot=f.claim.record;
      check(state.report.prepared.groups.at(slot).value==warm::key(f.source.field.object),"published field lost semantic source");
      result["field_sources"].push_back({{"source",key(f.source.field)},{"claim",claim(f.claim)},
        {"position",{state.report.groups64.at(slot),state.report.groups64.at(capacity.limits()[field::Budget::Fields]+slot),
          state.report.groups64.at(2*capacity.limits()[field::Budget::Fields]+slot)}},
        {"mass",state.report.groups64.at(3*capacity.limits()[field::Budget::Fields]+slot)}});
    }
    // Independent scalar reference for the recorded diagnostic only. Contact
    // and integration are outside this comparison; native state is never fed
    // from this host calculation.
    const auto native=harness.inspect_live_intent(1,UINT64_MAX);
    check(native.status==field::CascadeStatus::Ok,"native arithmetic reference query available");
    result["arithmetic"]=Json::array();
    for(const auto &body:state.body_records) {
      const auto slot=std::size_t(body.record),P=native.capacity;
      double weight=0;std::array<double,3> force{};int coincidence=0;
      for(const auto &r:state.response_trace.at("rows")) {
        if(r.at("body").at("record")!=body.record || !r.at("production_eligible").get<bool>())continue;
        const double numerator=double(std::stoull(r.at("numerator").get<std::string>()));
        const double denominator=double(std::stoull(r.at("denominator").get<std::string>()));
        const double coupling=(r.at("body_mass").get<double>()*numerator/denominator)*r.at("source_mass").get<double>();
        if(coupling==0)continue;
        const auto d=r.at("delta").get<std::array<double,3>>();
        const double d2=d[0]*d[0]+d[1]*d[1]+d[2]*d[2];
        if(d2==0){++coincidence;continue;}
        const double alpha=coupling/(d2*std::sqrt(d2));weight+=alpha;
        for(int k=0;k<3;++k)force[k]+=alpha*d[k];
      }
      const auto near=[](double a,double b) {return std::abs(a-b)<=64*std::numeric_limits<double>::epsilon()*std::max({1.,std::abs(a),std::abs(b)});};
      const int status=coincidence?2:weight>0?1:0;
      check(native.values[slot]==status && near(native.values[2*P+slot],weight) &&
            native.values[3*P+slot]==coincidence,"native status/weight/coincidence match independent response arithmetic");
      for(int k=0;k<3;++k)check(near(native.values[(4+k)*P+slot],force[k]),"native field force matches independent response arithmetic");
      Json target=nullptr;
      if(status==1) {
        target=Json::array();
        for(int k=0;k<3;++k){const double position=state.report.particles64[(field::kPosX+k)*state.report.prepared.capacity+slot];
          check(near(native.values[(10+k)*P+slot],position+force[k]/weight),"native candidate follows the worked weighted-displacement calculation");
          target.push_back(native.values[(10+k)*P+slot]);}
      }
      result["arithmetic"].push_back({{"slot",slot},{"status",status},{"coincidences",coincidence},
        {"coefficient_sum",native.values[2*P+slot]},
        {"force",{native.values[4*P+slot],native.values[5*P+slot],native.values[6*P+slot]}},{"candidate",target}});
    }
    return result;
  };
  in.trace["native"]={{"before",capture()}};
  field::Parameters64 parameters;parameters.gravity=1;parameters.contact_stiffness=parameters.reach_strength=0;parameters.dt=0.01;
  check(owner.tick(authorization,handle,parameters).status==field::TickStatus::Ok,"following native tick failed");
  in.trace["native"]["after"]=capture();
  return in.trace;
}
Json signature(const Json &trace) {
  const auto &before=trace.at("native").at("before"), &s=before.at("snapshot");
  Json bodies=Json::array(),fields=Json::array(),definitions=Json::array();
  for(const auto &p:s.at("particles")) {auto id=p.at("id").get<std::string>();
    bodies.push_back({{"occurrence",id.substr(id.find('/')+1)},{"definition",p.at("definition")},
      {"position",p.at("position")},{"radius",p.at("radius")},{"mass",p.at("mass")},{"exact_mass",p.at("exact_mass")}});}
  for(const auto &f:before.at("field_sources"))if(f.at("source").at("kind")!=int(GroupKind::Membership))
    fields.push_back({{"source",f.at("source")},{"position",f.at("position")},{"mass",f.at("mass")}});
  for(auto d:trace.at("definitions")) {
    if(d.at("mass")=="0")continue; // The imposed naming object is contextual.
    // Endpoint versions differ only in contextual attachments. Compare their
    // complete structural bodies and exact parent pins rather than erasing
    // revision identity from the emitted trace.
    const auto object=d.at("pin").at("object").get<std::string>();
    if(object==warm::key(letter_fixture::object("02.00.00.00.03")) ||
       object==warm::key(letter_fixture::object("02.01.00.00.03")))d["pin"]["revision"]=nullptr;
    d.erase("memberships");definitions.push_back(d);
  }
  return {{"bodies",bodies},{"fields",fields},{"definitions",definitions},
    {"frame",s.at("frame").at("reference")},{"tick",s.at("capture").at("tick")},
    {"arithmetic",before.at("arithmetic")}};
}
void compare_context(const Json &off,const Json &on,const letter_fixture::Fixture &fixture) {
  check(signature(off)==signature(on),"imposed off/on controls must have identical pre-tick structural inputs, native positions and diagnostic arithmetic");
  const auto &snapshot=on.at("native").at("before").at("snapshot");
  std::string imposed;
  for(const auto &f:on.at("fields"))if(f.at("source").at("kind")==int(GroupKind::Membership))imposed=f.at("comparison");
  int packet=-1;for(const auto &p:snapshot.at("response_trace").at("packets"))if(p.at("comparison")==imposed)packet=p.at("slot");
  check(packet>=0,"imposed comparison resolves to an actual live packet");
  int extra=0;
  for(const auto &r:snapshot.at("response_trace").at("rows"))if(r.at("parent")==packet) {
    check(r.at("kind")=="inherited" && !r.at("production_eligible").get<bool>() &&
          r.at("delta")==Json::array({0.,0.,0.}),"equal-cohort imposed parent adds no complementary native response");++extra;
  }
  check(extra==2,"both endpoint paths retain the zero imposed response rather than omitting it");
  const auto parent=fixture.base.tools.at(fixture.tools.at(fixture.english)).parents[0].target;
  Json centre;
  for(const auto &f:off.at("native").at("before").at("field_sources"))
    if(f.at("source")==key({GroupKind::ParentValue,parent.object}))centre=f.at("position");
  check(centre.is_array(),"shared character parent resolves through the native field association");
  for(const auto &r:off.at("native").at("before").at("arithmetic")) {
    check(r.at("status")==1 && r.at("candidate")==centre && r.at("coefficient_sum").get<double>()>0,
          "one-path native candidate equals the shared parent centre with an actual nonzero transmitted response");
    const auto force=r.at("force").get<std::array<double,3>>();
    check(force[0]!=0 || force[1]!=0 || force[2]!=0,"shared parent path actually transmits a field response");
  }
}
}
int main(int argc,char **argv) {
  try {
    engine::InstanceSettings settings;settings.read_environment=false;settings.cpu_threads=1;
    field::HostPath path=field::HostPath::BulkCopy;bool project_only=false;std::string selected="all";
    for(int i=1;i<argc;++i) {
      const std::string arg=argv[i];
      if(arg=="--threads" && i+1<argc)settings.cpu_threads=int(viewport::parse_count(arg,argv[++i],1,1024));
      else if(arg=="--direct")path=field::HostPath::CpuDirect;
      else if(arg=="--projection-only")project_only=true;
      else if(arg=="--case" && i+1<argc)selected=argv[++i];
      else throw std::invalid_argument("semantic_native_trace_test [--threads N] [--direct] [--projection-only] [--case all|ascii|unicode]");
    }
    check(selected=="all" || selected=="ascii" || selected=="unicode","unknown trace case");
    engine::Runtime runtime(settings);letter_fixture::Fixture fixture;
    for(const auto &profile:{std::string("ascii"),std::string("accented"),std::string("repeated")}) {
      if(selected!="all" && ((selected=="ascii")!=(profile=="ascii")))continue;
      const auto aggregate=run(runtime,fixture,profile,false,false,path,project_only);
      std::cout<<aggregate.dump()<<'\n';
      std::cout<<run(runtime,fixture,profile,true,false,path,project_only).dump()<<'\n';
      if(profile=="ascii") {
        auto imposed=run(runtime,fixture,profile,false,true,path,project_only);
        if(!project_only){compare_context(aggregate,imposed,fixture);imposed["fixed_pre_tick_control"]="passed";}
        std::cout<<imposed.dump()<<'\n';
      }
    }
    std::cerr<<"semantic_native_trace,checks="<<checks<<" PASS\n";return 0;
  }catch(const std::exception &e){std::cerr<<"semantic_native_trace FAIL: "<<e.what()<<'\n';return 1;}
}
