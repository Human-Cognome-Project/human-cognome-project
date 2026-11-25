// Fixture-owned worked trace. Numerical references below are test assertions,
// never an operational solver or a source of native starts.
#include "snapshot.h"
#include "capture_options.h"
#include "prepared_owner_fixture.h"
#include "field/cascade_source.h"
#include "scoped_hierarchy_audit.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <initializer_list>
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
        const std::string &profile,bool fine,bool imposed,
        const warm::ObjectKey *rollup_endpoint = nullptr, bool repeated_rollup = false):
      pair(profile=="ascii"),parts(fine),context(imposed) {
    auto base=fixture.base;
    const bool rollup = rollup_endpoint != nullptr;
    const auto &object = rollup ? *rollup_endpoint :
        pair ? fixture.english : profile=="accented" ? fixture.accented : fixture.repeated;
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
    std::vector<int> byte_fields;
    if(parts || profile=="repeated") {
      for(std::size_t i=0;i<projection.parent_count();++i) add(projection.parent(i).field);
      for(std::size_t i=0;i<projection.occurrence_count();++i) {
        const auto o=projection.occurrence(i);if(!o.active) continue;
        const int whole=add({GroupKind::SiblingDefinition,o.definition.object});
        if ((rollup || repeated_rollup) &&
            std::find(byte_fields.begin(),byte_fields.end(),whole)==byte_fields.end())
          byte_fields.push_back(whole);
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
    if (rollup) {
      const auto character = tool(recipe,first).parents[0].target;
      const int root = add({GroupKind::ParentValue,character.object});
      bindings.comparisons[root].children = byte_fields;
      bindings.comparisons[root].level = 2;
    }
    if (repeated_rollup) {
      check(byte_fields.size()==3 && endpoint.parents.size()==3,"repeated endpoint has three distinct byte fields");
      const int a = add({GroupKind::ParentValue,endpoint.parents[0].target.object});
      bindings.comparisons[a].children={byte_fields[0]};
      bindings.comparisons[a].level=2;
      const int accent = add({GroupKind::ParentValue,endpoint.parents[2].target.object});
      bindings.comparisons[accent].children={byte_fields[1],byte_fields[2]};
      bindings.comparisons[accent].level=2;
      const int endpoint_root = add({GroupKind::SiblingDefinition,first.object});
      bindings.comparisons[endpoint_root].children={a,accent};
      bindings.comparisons[endpoint_root].level=3;
    }
    const auto focus=bindings.fields[context?bindings.fields.size()-2:bindings.fields.size()-1].field;
    // The warm recipe selects an existing byte field. The optional character
    // field belongs to prepared admission, so select it through active focus.
    const auto &recipe_focus = (rollup || repeated_rollup) ? bindings.fields[byte_fields.front()].field : focus;
    recipe.focal_fields={{static_cast<warm::FieldKind>(recipe_focus.kind),recipe_focus.object}};
    auto selection = ActiveAssembly::assemble(recipe,warm::new_uuid(),false);
    if (rollup || repeated_rollup) selection = selection.focus({{focus.kind,InputText(warm::key(focus.object))}});
    source=std::make_shared<const ActiveAssembly>(std::move(selection));
    bindings.source={source->owner(),source->study(),source->generation(),recipe.ref};bindings.focus={focus};
    if (rollup) {
      bindings.character_rollup = AdmissionCharacterRollup{
          {source->study(),{requests[0].entry,{}}},tool(recipe,first).parents[0].target};
    }
    if (repeated_rollup)
      bindings.endpoint_rollup = AdmissionEndpointRollup{{source->study(),{requests[0].entry,{}}},first};
    auto root=std::make_unique<taichi::lang::SNode>(0,taichi::lang::SNodeType::root);
    auto *leaf=root.get();bindings.layout.push_back({{leaf,-1,ScaleRole::Root},1});
    if(parts) {leaf=&root->dense(taichi::lang::Axis(0),1);bindings.layout.push_back({{leaf,0,ScaleRole::PrimaryStep},64});}
    leaf->insert_children(taichi::lang::SNodeType::place).dt=taichi::lang::PrimitiveType::i32;
    trace={{"schema","hcp-semantic-native-trace"},{"version",1},{"profile",profile},
      {"exposure",parts?"parts":"aggregate"},{"imposed_context",context},
      {"definitions",Json::array()},{"occurrences",Json::array()},{"parents",Json::array()},
      {"layouts",Json::array()},{"fields",Json::array()}};
    if (rollup) trace["character_rollup"] = {{"character",pin(bindings.character_rollup->character)},
        {"owner_entry",requests[0].entry},{"byte_paths",Json::array({{0,0},{0,1}})}};
    if (repeated_rollup) trace["endpoint_rollup"] = pin(first);
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
    check(bindings.bodies.size()==(parts?(profile=="repeated"?4:2):(pair?2:1)),
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
            tool(recipe,t.parents[1].target).notation==
                warm::RecipeText(profile=="scalar-overlap" ? "B3" : "AD"),
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
        if(f.kind==GroupKind::SiblingDefinition)
          count=(f.object==body.definition.object || (repeated_rollup && f.object==first.object))?mass:0;
        else if(f.kind==GroupKind::Membership)count=mass;
        else if (rollup && f.object == bindings.character_rollup->character.object) count=mass;
        else for(const auto &p:trace["parents"])if(p.at("entry")==body.occurrence.source.entry &&
          p.at("path")==Json(body.occurrence.source.path) && p.at("field")==key(f))
          count+=std::stoull(p.at("count").get<std::string>());
        full &= count==mass;
      }
      if(full)full_fields.push_back(i);
    }
    trace["tree_witness"]={{"roots",roots},{"shared_children",shared_children},{"full_coverage_fields",full_fields}};
    if(parts && !pair && !rollup && !repeated_rollup)check(full_fields.empty(),"multibyte/repeated byte ledger has no field able to serve as a complete rooted rollup");
    if (rollup) {
      check(roots.size()==1 && full_fields==roots,
            "whole-byte branches have one complete character root");
      check(shared_children.empty()==(profile!="scalar-overlap"),
            "the single-scalar overlap control shares one leaf across branches");
    }
    if (repeated_rollup)
      check(roots.size()==1 && full_fields==roots,"repeated byte topology has a complete endpoint root");
    if(parts && profile=="repeated")check(!shared_children.empty(),"repeated-byte literal tree exposes a shared parent-value field across branches");
    runtime.program().add_snode_tree(std::move(root),false);
  }
};

Json refuse_on_empty_owner(field::Harness &harness, const Input &input,
                           const AdmissionBindings &bindings) {
  const auto &storage = harness.prepared_storage();
  const auto state = [&] {
    return Json{{"generation",storage.generation},{"tick",storage.completed_ticks},
        {"references",storage.live.references()},{"populated",storage.populated},
        {"hierarchy",harness.live_hierarchy_installed()},{"ready",harness.ready()}};
  };
  const auto before = state();
  check(!storage.populated && storage.live.references()==0 && !harness.live_hierarchy_installed(),
        "overlap refusal control starts with an empty prepared Harness");
  auto domain = prepared_owner_fixture::domain();
  const auto refused = PreparedStudyOwner::prepare(harness,input.source,warm::new_uuid(),bindings,
      domain,UINT64_MAX,UINT64_MAX);
  check(refused.status==AdmissionPlanStatus::InvalidInput && !refused.owner &&
        refused.reservation.live_bytes==0 && state()==before,
        "overlap refuses without publishing or retaining owner allocations");
  return {{"status","invalid_input"},{"preparation_work",refused.plan_work.charged},
      {"retained_bytes",refused.reservation.live_bytes},{"before",before},{"after",state()}};
}

Json run_input(engine::Runtime &runtime,Input in,field::HostPath path,bool project_only,
               bool supported,std::initializer_list<Input *> refused={}) {
  const bool rollup = bool(in.bindings.character_rollup);
  const bool repeated = bool(in.bindings.endpoint_rollup);
  const bool scoped = !in.bindings.nibble_scopes.empty();
  auto limits=prepared_owner_fixture::capacity().limits();
  limits[field::Budget::Particles]=8;limits[field::Budget::Fields]=16;
  if (repeated) {
    const auto bodies = in.bindings.bodies.size();
    // This fixture can encounter all unordered pairs during the candidate tick.
    limits[field::Budget::Contacts]=bodies*(bodies-1)/2;
  }
  limits[field::Budget::CascadeNodes]=limits[field::Budget::CascadeLevels]=16;
  limits[field::Budget::CascadeLinks]=32;limits[field::Budget::Participation]=64;
  limits[field::Budget::BootstrapCommands]=128;limits[field::Budget::CascadeReferences]=128;
  limits[field::Budget::CascadeCoverage]=128;limits[field::Budget::LiveResponses]=256;
  limits[field::Budget::ScratchBytes]=field::detail::PreparedStorage::required_scratch(limits,false,field::NumericalProfile::trial())+256*1024*1024;
  const auto mode = repeated ? field::PreparedHierarchyMode::ScopedEndpointBootstrap
                   : scoped ? field::PreparedHierarchyMode::ScopedNibbleBootstrap
                           : field::PreparedHierarchyMode::ExpressedTree;
  const field::PreparedCapacity capacity(limits,field::NumericalProfile::trial(),mode);
  auto domain=prepared_owner_fixture::domain();
  const auto plan=StructuralAdmissionPlan::build(*in.source,in.bindings,capacity,domain,UINT64_MAX);
  if(plan.status!=(supported?AdmissionPlanStatus::Ok:AdmissionPlanStatus::InvalidInput))
    std::cerr<<"plan_result,profile="<<in.trace.at("profile")<<",parts="<<in.parts<<",context="<<in.context
      <<",status="<<int(plan.status)<<",work="<<plan.work.charged<<'\n';
  check(plan.status==(supported?AdmissionPlanStatus::Ok:AdmissionPlanStatus::InvalidInput),"unexpected semantic plan admission result");
  in.trace["plan_status"]=supported?"ok":"invalid_input";
  in.trace["plan_work"]=std::to_string(plan.work.charged);
  in.trace["native"]=nullptr;
  std::optional<FieldKey> rollup_field;
  if (rollup) {
    const auto &character = in.bindings.character_rollup->character;
    rollup_field = FieldKey{GroupKind::ParentValue,character.object};
    auto invalid = in.bindings;
    const auto require_refusal = [&](const AdmissionBindings &input, const char *reason) {
      auto rejected_domain = prepared_owner_fixture::domain();
      const auto rejected = StructuralAdmissionPlan::build(*in.source,input,capacity,rejected_domain,UINT64_MAX);
      check(rejected.status==AdmissionPlanStatus::InvalidInput && !rejected.plan &&
            rejected_domain->result().live_bytes==0,reason);
    };
    invalid.character_rollup->character.revision = warm::new_uuid();
    require_refusal(invalid,"a stale character revision refuses without a plan or retained allocation");
    invalid = in.bindings;
    invalid.character_rollup->owner.source.entry++;
    require_refusal(invalid,"a foreign occurrence refuses without a plan or retained allocation");
    invalid = in.bindings;
    invalid.comparisons.back().children.pop_back();
    require_refusal(invalid,"incomplete byte coverage refuses without a plan or retained allocation");
    invalid = in.bindings;
    std::reverse(invalid.comparisons.back().children.begin(),invalid.comparisons.back().children.end());
    require_refusal(invalid,"reordered byte children refuse without a plan or retained allocation");
    invalid = in.bindings;
    invalid.character_rollup.reset();
    require_refusal(invalid,"a declared field alone does not create a literal rollup");
  }
  if(!supported || project_only) return in.trace;
  in.trace["participation_commands"]=Json::array();
  if (repeated) in.trace["endpoint_rows"] = Json::array();
  const auto repeated_root = in.bindings.comparisons.back().field;
  std::size_t parent_rows=0;
  std::set<std::uint64_t> rollup_ordinals;
  for(const auto &c:plan.plan->commands()) if(c.operation==field::UpdateOp::InsertParticipation) {
    const AdmissionFieldCommand *f=nullptr;const AdmissionOccurrenceCommand *o=nullptr;
    for(const auto &v:plan.plan->fields())if(v.command==c.second.command)f=&v;
    for(const auto &v:plan.plan->occurrences())if(v.command==c.first.command)o=&v;
    check(f && o && c.exact_count,"source command resolves to actual plan occurrence/field");
    in.trace["participation_commands"].push_back({{"entry",o->occurrence.entry},{"path",o->occurrence.path},
      {"field",key(f->field)},{"ordinal",std::to_string(c.ordinal)},{"count",std::to_string(*c.exact_count)}});
    const auto declared = repeated ? std::find_if(in.trace["occurrences"].begin(),in.trace["occurrences"].end(),
        [&](const Json &row) { return row.at("active")==true && row.at("entry")==o->occurrence.entry &&
            row.at("path")==Json(o->occurrence.path); }) : in.trace["occurrences"].end();
    const bool endpoint_row = repeated && key(f->field)==key(repeated_root);
    const bool literal_row = repeated && declared!=in.trace["occurrences"].end() &&
        f->field.kind==GroupKind::ParentValue && key(f->field).at("object")==declared->at("byte").at("character").at("object");
    if (endpoint_row || literal_row) {
      check(declared!=in.trace["occurrences"].end() && c.ordinal==declared->at("byte").at("character_ordinal") &&
            *c.exact_count==2 && c.share==1,"endpoint and character rows retain literal ordinal and full byte count");
      if (endpoint_row) in.trace["endpoint_rows"].push_back(in.trace["participation_commands"].back());
    } else if (rollup_field && key(f->field)==key(*rollup_field)) {
      check(o->occurrence.entry==in.bindings.character_rollup->owner.source.entry &&
            c.ordinal<2 && o->occurrence.path==warm::OccurrencePath{0,std::uint32_t(c.ordinal)} &&
            *c.exact_count==2 && c.share==1 && rollup_ordinals.insert(c.ordinal).second,
            "each ordered byte path contributes its exact mass once at the character root");
    } else if(f->field.kind==GroupKind::ParentValue) {
      bool match=false;
      for(const auto &p:in.trace["parents"])if(p["entry"]==o->occurrence.entry && p["path"]==Json(o->occurrence.path) &&
        p["field"]==key(f->field) && p["ordinal"]==c.ordinal && p["count"]==std::to_string(*c.exact_count))match=true;
      check(match,"plan preserves each projected parent ordinal and exact count");++parent_rows;
    }
  }
  check(parent_rows==in.trace["parents"].size(),"plan neither drops nor duplicates parent participation rows");
  if (rollup) check(rollup_ordinals.size()==2,"both exact byte contributions reach the rollup");
  std::optional<FieldKey> shared_nibble_field;
  if (scoped) {
    const auto spec = plan.plan->hierarchy();
    check(spec.scoped_nibble_rows,"the native hierarchy receives explicit scoped links");
    in.trace["scoped_child_rows"] = Json::array();
    const auto leaf = in.trace.at("tree_witness").at("shared_children").at(0).get<int>();
    shared_nibble_field = in.bindings.comparisons[leaf].field;
    const auto &publisher = spec.nodes[leaf];
    check(publisher.cohort_count==(repeated ? 3u : 2u),"one nibble publisher retains its complete global ledger");
    std::set<int> used;
    for (std::size_t i=0; i<spec.node_count; ++i) {
      const auto &node = spec.nodes[i];
      for (std::size_t j=node.child_begin; j<node.child_begin+node.child_count; ++j) {
        const int child = spec.children[j];
        if (child>=0) continue;
        const auto index = std::size_t(-1-child);
        check(index>=publisher.cohort_begin && index<publisher.cohort_begin+publisher.cohort_count,
              "scoped child selects a row in the same publisher's cohort");
        const auto &source = spec.contributions[index];
        check(used.insert(source.source_command).second,"a global source command is scoped once");
        const auto &edge = plan.plan->commands().at(source.source_command);
        const auto &body = plan.plan->occurrences().at(edge.first.command);
        check(edge.exact_count==1 && edge.ordinal==1 && body.occurrence.path.size()==2 &&
              plan.plan->commands().at(edge.first.command).exact_mass==2,
              "scoped child retains the actual exact numerator, denominator and ordinal");
        in.trace["scoped_child_rows"].push_back({{"branch",std::to_string(node.comparison)},
            {"publisher",std::to_string(publisher.comparison)},{"path",body.occurrence.path},
            {"ordinal",edge.ordinal},{"count",*edge.exact_count},{"denominator",2}});
      }
    }
    check(used.size()==publisher.cohort_count,"scoped rows partition the global nibble ledger");
  }
  field::Harness harness(runtime,capacity,path);
  for (auto *input : refused) input->trace["owner_refusal"] = refuse_on_empty_owner(harness,*input,input->bindings);
  if (scoped) {
    auto invalid = in.bindings;
    invalid.nibble_scopes.pop_back();
    in.trace["scope_refusals"]["missing"] = refuse_on_empty_owner(harness,in,invalid);
    invalid = in.bindings;
    invalid.nibble_scopes[1] = invalid.nibble_scopes[0];
    in.trace["scope_refusals"]["duplicate"] = refuse_on_empty_owner(harness,in,invalid);
    invalid = in.bindings;
    std::swap(invalid.nibble_scopes.front().byte,invalid.nibble_scopes.back().byte);
    in.trace["scope_refusals"]["foreign_branch"] = refuse_on_empty_owner(harness,in,invalid);
    invalid = in.bindings;
    invalid.nibble_scopes[0].ordinal = 2;
    in.trace["scope_refusals"]["wrong_ordinal"] = refuse_on_empty_owner(harness,in,invalid);
    if (repeated) {
      invalid = in.bindings;
      invalid.comparisons.back().children.pop_back();
      in.trace["scope_refusals"]["incomplete_endpoint"] = refuse_on_empty_owner(harness,in,invalid);
      invalid = in.bindings;
      invalid.endpoint_rollup->endpoint.revision = warm::new_uuid();
      in.trace["scope_refusals"]["stale_endpoint"] = refuse_on_empty_owner(harness,in,invalid);
    }
  }
  if (rollup || repeated) {
    auto rejected_domain = prepared_owner_fixture::domain();
    const auto editable = PreparedStudyOwner::prepare(harness,in.source,warm::new_uuid(),in.bindings,
        rejected_domain,UINT64_MAX,UINT64_MAX,true);
    check(editable.status==AdmissionPlanStatus::Unsupported && !editable.owner &&
          rejected_domain->result().live_bytes==0 && !harness.live_hierarchy_installed(),
          "an editing workspace cannot prepare the fixed character rollup");
  }
  auto prepared=PreparedStudyOwner::prepare(harness,in.source,warm::new_uuid(),in.bindings,domain,UINT64_MAX,UINT64_MAX);
  check(prepared.status==AdmissionPlanStatus::Ok,"native owner preparation failed");
  auto &owner=*prepared.owner;const auto handle=owner.handle();const auto authorization=in.source->owner();
  check(owner.bootstrap(authorization,handle,UINT64_MAX,UINT64_MAX,UINT64_MAX).status==field::UpdateStatus::Ok,"native bootstrap failed");
  if (repeated) audit_scoped_hierarchy(harness);
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
    if (rollup || repeated) {
      const auto &particles = result["snapshot"].at("particles");
      result["body_sources"] = Json::array();
      std::set<int> slots;
      for (std::size_t i=0; i<rows.bodies.size(); ++i) {
        const auto body = rows.body(i);
        check(body.source.occurrence.entry==2 &&
              body.source.occurrence.path==in.bindings.bodies.at(i).occurrence.source.path &&
              body.source.definition==in.bindings.bodies.at(i).definition &&
              body.exact_mass==2 && slots.insert(body.claim.record).second,
              "native exact ledger preserves each byte occurrence and distinct claim");
        result["body_sources"].push_back({{"path",body.source.occurrence.path},
            {"definition",pin(body.source.definition)},{"claim",claim(body.claim)}});
      }
      check(particles.size()==in.bindings.bodies.size(),"publication retains one body per byte occurrence");
      for (const auto &particle : particles)
        check(particle.at("exact_mass")=="2" && particle.at("mass")==2 &&
              slots.erase(particle.at("slot").get<int>())==1,"published byte retains its mass and distinct slot");
      int whole_fields = 0;
      int root_fields = 0;
      const auto expected_root = repeated ? repeated_root : *rollup_field;
      for (const auto &f : result["field_sources"]) {
        if (f.at("source")==key(expected_root)) {
          check(f.at("mass")== (repeated ? 8 : 4),"the native root counts byte masses once");
          ++root_fields;
        } else if (f.at("source").at("kind")==int(GroupKind::SiblingDefinition)) {
          std::size_t cohort = 0;
          for (const auto &body : in.bindings.bodies)
            cohort += f.at("source").at("object")==warm::key(body.definition.object);
          check(f.at("mass")==2*cohort,"each native whole-byte child carries its occurrence mass");
          ++whole_fields;
        }
      }
      check(root_fields==1 && whole_fields==(repeated ? 3 : 2),"the root and all whole-byte fields publish");
    }
    if (scoped) {
      const auto &storage = harness.prepared_storage();
      int shared_fields = 0;
      for (std::size_t i=0; i<rows.fields.size(); ++i) {
        const auto f = rows.field(i);
        if (key(f.source.field)!=key(*shared_nibble_field)) continue;
        ++shared_fields;
        check(result["field_sources"][i]["mass"]==(repeated ? 3 : 2),"the shared nibble publishes its full mass");
        result["global_nibble"] = {{"field",key(f.source.field)},{"claim",claim(f.claim)},
            {"rows",Json::array()}};
        for (std::size_t j=0; j<storage.live.edges; ++j) {
          const auto &edge = storage.edges[j];
          if (edge.second.record!=f.claim.record) continue;
          check(field::detail::same_handle(edge.second,f.claim),"global nibble row retains the complete field handle");
          const auto body = std::find_if(rows.bodies.begin(),rows.bodies.end(),
              [&](const auto &h) { return field::detail::same_handle(h,edge.first); });
          check(body!=rows.bodies.end() && edge.ordinal==1 && storage.source_counts[j]==1,
                "native global nibble retains each original exact row once");
          result["global_nibble"]["rows"].push_back({{"path",rows.occurrences[body-rows.bodies.begin()].occurrence.path},
               {"ordinal",edge.ordinal},{"count",storage.source_counts[j]}});
        }
        check(result["global_nibble"]["rows"].size()==in.bindings.nibble_scopes.size(),
              "global nibble publishes all original body rows");
      }
      check(shared_fields==1 && storage.live.particles==in.bindings.bodies.size() && storage.live.fields==in.bindings.fields.size() &&
            storage.live.edges==in.trace["participation_commands"].size(),
            "scoping adds no body, field or physical participation row");
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
  const auto tick = owner.tick(authorization,handle,parameters);
  if (tick.status!=field::TickStatus::Ok) {
    std::cerr<<"semantic_tick_refusal,profile="<<in.trace.at("profile")<<",status="<<int(tick.status)
        <<",budget="<<field::budget_name(tick.failure.budget)<<",required="<<tick.failure.required
        <<",capacity="<<tick.failure.capacity<<",current="<<int(tick.current.status)
        <<",candidate="<<int(tick.candidate.status)<<'\n';
    std::cout<<in.trace.dump()<<'\n';
  }
  check(tick.status==field::TickStatus::Ok,"following native tick failed");
  in.trace["native"]["after"]=capture();
  if (rollup || repeated) {
    const auto &before=in.trace["native"]["before"], &after=in.trace["native"]["after"];
    check(before["snapshot"]["capture"]["tick"]=="0" && after["snapshot"]["capture"]["tick"]=="1" &&
          before["snapshot"]["capture"]["generation"]==after["snapshot"]["capture"]["generation"],
          "character rollup publishes a following completed tick");
    for (std::size_t i=0; i<before["field_sources"].size(); ++i)
      check(before["field_sources"][i]["claim"]==after["field_sources"][i]["claim"],
            "the rollup and all child field claims persist through the tick");
    check(before["body_sources"]==after["body_sources"],"both byte claims and source paths persist through the tick");
  }
  return in.trace;
}

Json run(engine::Runtime &runtime,const letter_fixture::Fixture &fixture,const std::string &profile,
         bool parts,bool context,field::HostPath path,bool project_only,bool rollup=false,
         std::initializer_list<Input *> refused={}) {
  return run_input(runtime,Input(runtime,fixture,profile,parts,context,rollup ? &fixture.accented : nullptr),
      path,project_only,profile=="ascii" || !parts || rollup,refused);
}

Json shared_nibble(const Input &input, const char *notation) {
  const auto &trace = input.trace;
  const auto &shared = trace.at("tree_witness").at("shared_children");
  check(shared.size()==1,"expected one shared nibble field across byte branches");
  const auto leaf = shared.at(0).get<int>();
  const auto &field = trace.at("fields").at(leaf).at("source");
  const auto found = std::find_if(trace.at("definitions").begin(),trace.at("definitions").end(),
      [&](const Json &d) { return d.at("pin").at("object")==field.at("object"); });
  check(found!=trace.at("definitions").end() && found->at("notation")==notation,
        "shared field resolves to the pinned nibble definition");
  Json rows = Json::array(), branches = Json::array();
  for (const auto &row : trace.at("parents")) {
    if (row.at("field")!=field) continue;
    check(row.at("ordinal")==1 && row.at("count")=="1",
          "each shared low nibble retains its byte ordinal and one exact count");
    rows.push_back(row);
  }
  for (const auto &comparison : input.bindings.comparisons) {
    if (std::find(comparison.children.begin(),comparison.children.end(),leaf)==comparison.children.end()) continue;
    check(comparison.field.kind==GroupKind::SiblingDefinition,"shared nibble is used by whole-byte branches");
    Json included = Json::array(), foreign = Json::array();
    for (const auto &row : rows) {
      const auto body = std::find_if(input.bindings.bodies.begin(),input.bindings.bodies.end(),
          [&](const AdmissionBody &b) { return row.at("entry")==b.occurrence.source.entry &&
              row.at("path")==Json(b.occurrence.source.path); });
      check(body!=input.bindings.bodies.end(),"shared nibble row resolves to one active byte path");
      (body->definition.object==comparison.field.object ? included : foreign).push_back(row.at("path"));
    }
    check(!included.empty() && !foreign.empty(),"global nibble cohort spills outside each whole-byte branch");
    branches.push_back({{"byte",key(comparison.field)},{"included_paths",included},{"outside_paths",foreign}});
  }
  check(branches.size()==2,"one global nibble comparison has two prospective parents");
  return {{"definition",found->at("pin")},{"notation",notation},{"rows",rows},{"branches",branches}};
}

warm::ObjectKey add_scalar_overlap(letter_fixture::Fixture &fixture) {
  // Fixture additions keep the first byte C3, replacing AD with B3 to isolate
  // a shared nibble under the same single-scalar rollup contract.
  const auto scalar = fixture.character(letter_fixture::object("00.00.00.01.1m"),u8"ó");
  const auto endpoint = fixture.add(letter_fixture::object("02.00.20.00.1Z"),u8"ó",4,{scalar});
  const auto &definition = tool(fixture.base,scalar);
  fixture.base.geometry.push_back({definition.parents.at(1).target.object,{},{{{},0.5}}});
  fixture.base.geometry.push_back({endpoint.object,{},{{{},0.5}}});
  return endpoint.object;
}

Json repeated_parent_overlap(engine::Runtime &runtime, letter_fixture::Fixture &fixture, field::HostPath path) {
  const auto endpoint = add_scalar_overlap(fixture);

  Input repeated(runtime,fixture,"repeated",true,false);
  Input overlap(runtime,fixture,"scalar-overlap",true,false,&endpoint);
  check(repeated.bindings.bodies[0].definition==repeated.bindings.bodies[1].definition,
        "the two a paths share the exact byte definition pin");
  const auto repeated_nibble = shared_nibble(repeated,"1");
  const auto overlap_nibble = shared_nibble(overlap,"3");
  const auto paths = [](const Json &nibble) {
    Json out = Json::array();
    for (const auto &row : nibble.at("rows")) out.push_back(row.at("path"));
    return out;
  };
  check(paths(repeated_nibble)==Json::array({{0,0},{1,0},{2,1}}) &&
        paths(overlap_nibble)==Json::array({{0,0},{0,1}}),
        "each shared nibble retains the exact source paths across the two branches");
  repeated.trace["shared_nibble"] = repeated_nibble;
  overlap.trace["shared_nibble"] = overlap_nibble;

  // The first source admits at aggregate exposure in its own native study.
  // Both fine refusals then precede a successful disjoint rollup on one Harness.
  auto aggregate = run(runtime,fixture,"repeated",false,false,path,false);
  const auto &commands = aggregate.at("participation_commands");
  const auto &a = commands.at(1), &again = commands.at(2);
  check(a.at("field")==again.at("field") && a.at("ordinal")=="0" && again.at("ordinal")=="1" &&
        a.at("count")=="2" && again.at("count")=="2" && a.at("path").empty() && again.at("path").empty(),
        "aggregate admission retains both a contributions in the same definition field");
  for (const auto &observation : aggregate.at("native")) {
    const auto &particles = observation.at("snapshot").at("particles");
    const auto &fields = observation.at("field_sources");
    const auto shared = std::find_if(fields.begin(),fields.end(),
        [&](const Json &field) { return field.at("source")==a.at("field"); });
    check(particles.size()==1 && particles.at(0).at("mass")==8 &&
          shared!=fields.end() && shared->at("mass")==4,
          "both a contributions publish through one field while aggregate body mass stays eight");
  }
  auto retry = run(runtime,fixture,"accented",true,false,path,false,true,{&repeated,&overlap});
  return {{"schema","hcp-repeated-parent-overlap"},{"version",1},{"repeated_aggregate",std::move(aggregate)},
      {"repeated_bytes",std::move(repeated.trace)},{"single_scalar_overlap",std::move(overlap.trace)},
      {"same_harness_retry",std::move(retry)}};
}

Json scoped_nibble_rollup(engine::Runtime &runtime, letter_fixture::Fixture &fixture, field::HostPath path) {
  const auto endpoint = add_scalar_overlap(fixture);
  Input overlap(runtime,fixture,"scalar-overlap",true,false,&endpoint);
  const auto &root = overlap.bindings.comparisons.back();
  for (std::size_t i=0; i<root.children.size(); ++i)
    overlap.bindings.nibble_scopes.push_back({overlap.bindings.comparisons[root.children[i]].comparison,
        overlap.bindings.bodies[i].occurrence,1});
  auto experiment = run_input(runtime,std::move(overlap),path,false,true);
  auto control = run(runtime,fixture,"accented",true,false,path,false,true);
  return {{"schema","hcp-scoped-nibble-rollup"},{"version",1},
      {"overlap",std::move(experiment)},{"disjoint",std::move(control)}};
}

Json repeated_endpoint_rollup(engine::Runtime &runtime, const letter_fixture::Fixture &fixture,
                             field::HostPath path) {
  Input input(runtime,fixture,"repeated",true,false,nullptr,true);
  const auto shared = shared_nibble(input,"1");
  for (const auto &body : input.bindings.bodies) {
    const auto matching = std::find_if(shared.at("rows").begin(),shared.at("rows").end(),
        [&](const Json &row) { return row.at("path")==Json(body.occurrence.source.path); });
    if (matching==shared.at("rows").end()) continue;
    const auto branch = std::find_if(input.bindings.comparisons.begin(),input.bindings.comparisons.end(),
        [&](const AdmissionComparison &comparison) { return comparison.field.kind==GroupKind::SiblingDefinition &&
            comparison.field.object==body.definition.object; });
    check(branch!=input.bindings.comparisons.end(),"scoped nibble input resolves its declared byte branch");
    input.bindings.nibble_scopes.push_back({branch->comparison,body.occurrence,1});
  }
  auto aggregate = run(runtime,fixture,"repeated",false,false,path,false);
  auto fine = run_input(runtime,std::move(input),path,false,true);
  std::array<unsigned long long,3> coarse_counts{}, fine_counts{};
  for (const auto &row : aggregate.at("participation_commands"))
    if (row.at("field").at("kind")==int(GroupKind::ParentValue))
      coarse_counts.at(std::stoull(row.at("ordinal").get<std::string>())) +=
          std::stoull(row.at("count").get<std::string>());
  for (const auto &row : fine.at("endpoint_rows"))
    fine_counts.at(std::stoull(row.at("ordinal").get<std::string>())) +=
        std::stoull(row.at("count").get<std::string>());
  check(coarse_counts==std::array<unsigned long long,3>{2,2,4} && fine_counts==coarse_counts,
        "aggregate and byte endpoints agree by ordered character contribution");
  for (const auto &observation : fine.at("native")) {
    const auto &q = observation.at("snapshot").at("response_trace");
    const auto packet = [&](const Json &comparison) {
      for (const auto &entry : q.at("packets"))
        if (entry.at("comparison")==comparison) return entry.at("slot").get<int>();
      throw std::runtime_error("missing scoped response packet");
    };
    std::size_t matched = 0;
    for (const auto &link : fine.at("scoped_child_rows")) {
      const auto &bodies = observation.at("body_sources");
      const auto body = std::find_if(bodies.begin(),bodies.end(),
          [&](const Json &entry) { return entry.at("path")==link.at("path"); });
      check(body!=bodies.end(),"scoped response resolves to an actual byte claim");
      const int parent = packet(link.at("branch")), child = packet(link.at("publisher"));
      const auto &fields = fine.at("fields");
      const auto field = std::find_if(fields.begin(),fields.end(),
          [&](const Json &f) { return f.at("comparison")==link.at("branch"); });
      check(field!=fields.end(),"scoped response owns a declared branch field");
      const auto &actual = observation.at("field_sources");
      const auto branch = std::find_if(actual.begin(),actual.end(),
          [&](const Json &f) { return f.at("source")==field->at("source"); });
      check(branch!=actual.end(),"scoped response owns a published branch field");
      for (const auto &row : q.at("rows")) {
        if (row.at("kind")!="scoped_inherited" || row.at("body")!=body->at("claim")) continue;
        const bool eligible = link.at("path")!=Json::array({2,1});
        check(row.at("child")==child && row.at("parent")==parent && row.at("numerator")=="1" &&
              row.at("denominator")=="2" && row.at("source_mass")==branch->at("mass") &&
              row.at("production_eligible")==eligible,"scope drives only its owning byte comparison");
        for (int axis=0; axis<3; ++axis)
          check(row.at("delta").at(axis).get<double>()==branch->at("position").at(axis).get<double>()-
                  row.at("recipient").at(axis).get<double>(),
                "scoped inherited displacement starts at the singleton byte centroid");
        ++matched;
      }
    }
    check(matched==3,"exactly three scoped inherited rows resolve through their branch and byte claim");
  }
  return {{"schema","hcp-repeated-endpoint-rollup"},{"version",1},
      {"aggregate_counts",coarse_counts},{"fine_counts",fine_counts},
      {"aggregate",std::move(aggregate)},{"parts",std::move(fine)}};
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
      else if(arg=="--compiler-no-unroll") scoped_audit_no_unroll();
      else if(arg=="--projection-only")project_only=true;
      else if(arg=="--case" && i+1<argc)selected=argv[++i];
      else throw std::invalid_argument("semantic_native_trace_test [--threads N] [--direct] [--compiler-no-unroll] [--projection-only] [--case all|ascii|unicode|character-rollup|parent-overlap|scoped-nibble|repeated-rollup]");
    }
    check(selected=="all" || selected=="ascii" || selected=="unicode" ||
          selected=="character-rollup" || selected=="parent-overlap" || selected=="scoped-nibble" ||
          selected=="repeated-rollup","unknown trace case");
    engine::Runtime runtime(settings);letter_fixture::Fixture fixture;
    if (selected=="repeated-rollup") {
      check(!project_only,"repeated-rollup includes native publication and refusal");
      std::cout<<repeated_endpoint_rollup(runtime,fixture,path).dump()<<'\n';
      std::cerr<<"semantic_native_trace,checks="<<checks<<" PASS\n";
      return 0;
    }
    if (selected=="parent-overlap" || selected=="scoped-nibble") {
      check(!project_only,"the overlap controls include native publication and refusal");
      const auto result = selected=="parent-overlap" ? repeated_parent_overlap(runtime,fixture,path)
                                                       : scoped_nibble_rollup(runtime,fixture,path);
      std::cout<<result.dump()<<'\n';
      std::cerr<<"semantic_native_trace,checks="<<checks<<" PASS\n";
      return 0;
    }
    if (selected=="character-rollup") {
      std::cout<<run(runtime,fixture,"accented",true,false,path,project_only,true).dump()<<'\n';
      std::cerr<<"semantic_native_trace,checks="<<checks<<" PASS\n";
      return 0;
    }
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
