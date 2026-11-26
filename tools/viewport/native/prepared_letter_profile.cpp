#include "prepared_letter_profile.h"
#include "field/prepared_storage.h"
#include <array>
#include <algorithm>
#include <climits>
#include <map>
#include <set>

namespace viewport {
namespace {
const warm::Tool &tool(const warm::Bundle &b, const warm::ToolRef &ref) {
  for (const auto &t : b.tools) if (t.ref == ref) return t;
  throw PreparedLetterProfileError("prepared letter profile: missing definition pin");
}
const warm::Tool &letter(const warm::Bundle &b, const warm::ToolRef &ref, bool retained) {
  for (const auto &t : b.tools)
    if (t.ref.object == ref.object && t.ref.revision == ref.revision) {
      // This preset specifies the English A aggregate's direct, single-byte
      // construction. A different source shape needs a separately chosen profile.
      if (!retained && !t.member_of.empty())
        throw PreparedLetterProfileError("prepared English pair: membership comparison choices are unsupported");
      if (t.ref.object.scope.nil() && warm::address_text(t.ref.object.address) == "02.00.00.00.03" &&
          t.mass == 2 && t.notation && *t.notation == "A" &&
          t.parents.size() == 1 && t.parents[0].contribution == 2)
        return t;
      break;
    }
  throw PreparedLetterProfileError("prepared English pair: unsupported source construction");
}
std::array<warm::ToolRef,4> memberships(const warm::Bundle &b, const warm::Tool &a) {
  constexpr std::array<const char *,4> addresses{
    "02.00.00.00.01","02.00.00.00.00","02.00.zz.00.00","02.0z.zz.00.00"};
  constexpr std::array<const char *,4> names{"Alphabet (en)","English","Germanic","Indo-European"};
  std::array<warm::ToolRef,4> refs;
  for (std::size_t i=0; i<refs.size(); ++i) {
    const warm::Tool *found = nullptr;
    for (const auto &t : b.tools)
      if (t.ref.object.scope.nil() && warm::address_text(t.ref.object.address) == addresses[i]) {
        if (found) throw PreparedLetterProfileError("prepared retained English pair: duplicate group definition");
        found = &t;
      }
    if (!found || found->mass || !found->parents.empty() || !found->notation || *found->notation != names[i])
      throw PreparedLetterProfileError("prepared retained English pair: unsupported group definition");
    refs[i] = found->ref;
  }
  if (a.member_of.size() != 2 ||
      !((a.member_of[0] == refs[0] && a.member_of[1] == refs[1]) ||
        (a.member_of[0] == refs[1] && a.member_of[1] == refs[0])))
    throw PreparedLetterProfileError("prepared retained English pair: unsupported direct memberships");
  for (std::size_t i=0; i<refs.size(); ++i) {
    const auto &g = tool(b,refs[i]);
    if (i+1 == refs.size() ? !g.member_of.empty() :
        (g.member_of.size() != 1 || !(g.member_of[0] == refs[i+1])))
      throw PreparedLetterProfileError("prepared retained English pair: unsupported membership closure");
  }
  return refs;
}
void enable_occurrence_edits(assembly::PreparedStudySubmission &request) {
  using field::Budget;
  auto limits = request.capacity.limits();
  limits[Budget::Batch] = 32;
  limits[Budget::Participation] = limits[Budget::Fields] * limits[Budget::Particles];
  limits[Budget::ScratchBytes] = field::detail::PreparedStorage::required_scratch(
      limits,false,request.capacity.profile()) + 256*1024*1024;
  request.capacity = field::PreparedCapacity(limits,request.capacity.profile());
  request.editable_occurrences = true;
}
std::set<warm::ObjectKey> catalogue_memberships(const warm::Bundle &recipe,
                                               const warm::ToolRef &root) {
  std::set<warm::ObjectKey> result;
  const auto visit = [&](auto &&self,const warm::ToolRef &pin) -> void {
    for (const auto &group : tool(recipe,pin).member_of)
      if (result.insert(group.object).second) self(self,group);
  };
  visit(visit,root);
  return result;
}
std::vector<warm::ObjectKey> catalogue_membership_path(const warm::Bundle &recipe,
                                                      const warm::ToolRef &root) {
  const auto closure = catalogue_memberships(recipe,root);
  std::map<warm::ObjectKey,const warm::Tool *> groups;
  std::set<warm::ObjectKey> parents;
  for (const auto &group : recipe.tools) {
    if (!closure.count(group.ref.object)) continue;
    if (group.member_of.size()>1)
      throw PreparedLetterProfileError("byte-focused catalogue requires a membership path for each letter");
    groups.emplace(group.ref.object,&group);
    if (!group.member_of.empty()) parents.insert(group.member_of.front().object);
  }
  const warm::Tool *leaf = nullptr;
  for (const auto &[key,group] : groups) {
    if (parents.count(key)) continue;
    if (leaf) throw PreparedLetterProfileError("byte-focused catalogue membership paths branch within one letter");
    leaf = group;
  }
  std::vector<warm::ObjectKey> path;
  for (auto group=leaf; group && path.size()<groups.size(); ) {
    path.push_back(group->ref.object);
    group = group->member_of.empty() ? nullptr : groups.at(group->member_of.front().object);
  }
  if (path.empty() || path.size()!=closure.size())
    throw PreparedLetterProfileError("byte-focused catalogue requires a complete membership path");
  return path;
}
field::PreparedCapacity capacity(bool retained) {
  using field::Budget;
  field::BudgetDemand l;
  l[Budget::Potential] = 128;
  l[Budget::Particles] = 4; l[Budget::Fields] = 4;
  l[Budget::Participation] = 16; l[Budget::Contacts] = 4;
  l[Budget::References] = 1024; l[Budget::Focus] = 2; l[Budget::Batch] = 1;
  l[Budget::BootstrapCommands] = 32;
  l[Budget::GeometryWork] = l[Budget::CascadeWork] = 1000000;
  l[Budget::CascadeNodes] = l[Budget::CascadeLinks] = l[Budget::CascadeLevels] = 4;
  if (retained) {
    l[Budget::Fields] = l[Budget::CascadeNodes] = l[Budget::CascadeLevels] = 6;
    l[Budget::CascadeLinks] = 5;
  }
  l[Budget::CascadeReferences] = l[Budget::CascadeCoverage] = 32;
  l[Budget::LiveResponses] = 64;
  l[Budget::PlacementWork] = 100000000; l[Budget::PlacementCells] = 65536;
  l[Budget::TickWork] = 1000000000;
  l[Budget::ScratchBytes] = field::detail::PreparedStorage::required_scratch(
      l,false,field::NumericalProfile::trial()) + 256*1024*1024;
  return field::PreparedCapacity(l,field::NumericalProfile::trial());
}
warm::Bundle recipe(warm::Bundle definitions, const warm::ToolRef &english, bool retained) {
  const auto &a = letter(definitions,english,retained);
  if (retained) memberships(definitions,a);
  const auto parent = a.parents[0].target.object;
  // Geometry is a choice of this explicitly selected profile, not a cold value.
  for (auto it = definitions.geometry.begin(); it != definitions.geometry.end();)
    if (it->object == english.object) it = definitions.geometry.erase(it); else ++it;
  definitions.geometry.push_back({english.object,{},{{{},0.125}}});
  definitions.focal_fields = {{warm::FieldKind::ParentValue,parent}};
  return assembly::normalize_structural_letters(std::move(definitions),
      {{10,english,warm::Exposure::Aggregate},{2,english,warm::Exposure::Aggregate}});
}
assembly::PreparedStudySubmission submission(
    std::shared_ptr<const assembly::ActiveAssembly> source, const warm::ToolRef &english,
    field::HostPath path, bool retained) {
  using namespace assembly;
  if (!source || !source->inputs_available() || source->local() || source->shared_recipe().recipe_version != 2)
    throw PreparedLetterProfileError("prepared English pair: shared v2 source required");
  const auto &recipe = source->shared_recipe();
  const auto &a = letter(recipe,english,retained);
  const auto parent = a.parents[0].target.object;
  const auto groups = retained ? memberships(recipe,a) : std::array<warm::ToolRef,4>{};
  PreparedStudySubmission out{source,{},capacity(retained),
      std::make_shared<detail::ReservationDomain>(detail::ReservationBudget{16*1024*1024,16*1024*1024})};
  out.path = path;
  auto &b = out.bindings;
  b.source = {source->owner(),source->study(),source->generation(),recipe.ref};
  b.layout.push_back({{nullptr,-1,ScaleRole::Root},1});
  b.bodies = declared_aggregate_bodies(*source);
  if (b.bodies.size() != 2)
    throw PreparedLetterProfileError("prepared English pair: two aggregate occurrences required");
  for (auto &body : b.bodies) {
    const auto entry = body.occurrence.source.entry;
    if (!(body.definition == english) || body.node != 0 || (entry != 2 && entry != 10))
      throw PreparedLetterProfileError("prepared English pair: unsupported active selection");
    body.address += 40;
    body.mass = 1.5;
    body.geometry = {1,1,true};
  }
  const FieldKey sibling{GroupKind::SiblingDefinition,english.object}, value{GroupKind::ParentValue,parent};
  b.fields = {{sibling,5,0},{value,5,1}};
  b.comparisons = {{sibling,100,0,52,{}},{value,101,1,52,{0}}};
  if (retained) for (const auto &group : groups) {
    const int i = int(b.fields.size());
    const FieldKey field{GroupKind::Membership,group.object};
    b.fields.push_back({field,5,i});
    b.comparisons.push_back({field,std::uint64_t(100+i),1,52,{i-1}});
  }
  b.focus = {value};
  out.plan_work = out.metadata_work = 10000000;
  out.placement_work = 100000000; out.placement_cells = 65536; out.hierarchy_work = 1000000;
  return out;
}
}
std::vector<assembly::detail::AdmissionBody> declared_aggregate_bodies(
    const assembly::ActiveAssembly &source) {
  if (!source.inputs_available() || source.local() ||
      source.shared_recipe().recipe_version != 2)
    throw PreparedLetterProfileError("recipe study: shared v2 source required");
  const auto &recipe = source.shared_recipe();
  std::map<warm::ObjectKey,std::size_t> definitions;
  for (std::size_t i = 0; i < recipe.tools.size(); ++i) {
    if (!recipe.tools[i].ref.object.scope.nil())
      throw PreparedLetterProfileError("recipe study: shared definitions required");
    definitions.emplace(recipe.tools[i].ref.object,i);
  }
  std::vector<assembly::detail::AdmissionBody> bodies;
  bodies.reserve(recipe.selections.size());
  for (const auto &occurrence : recipe.structural_occurrences) {
    if (!occurrence.id.path.empty()) continue;
    if (source.exposure(occurrence.id.entry) != warm::Exposure::Aggregate)
      throw PreparedLetterProfileError("recipe study: aggregate exposure required");
    const auto definition_index = definitions.at(occurrence.definition);
    const auto &definition = recipe.tools[definition_index];
    if (!definition.mass || *definition.mass == 0 || bodies.size() >= INT_MAX)
      throw PreparedLetterProfileError("recipe study: invalid live definition");
    bodies.push_back({{source.study(),occurrence.id},definition.ref,occurrence.node,3,
        -1,double(*definition.mass),{definition_index+1,1,true}});
  }
  std::sort(bodies.begin(),bodies.end(),[](const auto &a, const auto &b) {
    return a.occurrence.source < b.occurrence.source;
  });
  for (std::size_t i=0; i<bodies.size(); ++i)
    bodies[i].address = int(i);
  return bodies;
}
warm::Bundle prepared_english_pair(warm::Bundle b, const warm::ToolRef &english) {
  return recipe(std::move(b),english,false);
}
warm::Bundle whole_english_pair(warm::Bundle b,const warm::ToolRef &english,const warm::ToolRef &focus) {
  const auto &a=tool(b,english);const auto &group=tool(b,focus);
  if(!a.mass || *a.mass!=2 || a.parents.size()!=1 || a.parents[0].contribution!=2 ||
      !a.notation || *a.notation!="A" || group.mass!=0 || !group.parents.empty() ||
      std::find(a.member_of.begin(),a.member_of.end(),focus)==a.member_of.end())
    throw PreparedLetterProfileError("whole English pair: declared one-byte composition and inherited focus required");
  for(auto it=b.geometry.begin();it!=b.geometry.end();) {
    if(it->object==english.object)it=b.geometry.erase(it);else ++it;
  }
  b.geometry.push_back({english.object,{},{{{},0.125}}});
  b.focal_fields={{warm::FieldKind::Membership,focus.object}};
  // Both native roles are declared up front; the actor view below selects the
  // initial uniform exposure without changing this immutable recipe.
  return assembly::normalize_structural_letters(std::move(b),
      {{2,english,warm::Exposure::Aggregate},{10,english,warm::Exposure::Parts}});
}
assembly::PreparedStudySubmission whole_english_pair_submission(
    std::shared_ptr<const assembly::ActiveAssembly> source,const warm::ToolRef &english,
    const warm::ToolRef &focus,field::HostPath path) {
  using namespace assembly;
  if(!source || source->local() || source->exposure(2)!=warm::Exposure::Aggregate ||
      source->exposure(10)!=warm::Exposure::Aggregate)
    throw PreparedLetterProfileError("whole English pair: uniform initial aggregate view required");
  const auto &a=tool(source->shared_recipe(),english);
  if(!a.mass || *a.mass!=2 || a.parents.size()!=1)throw PreparedLetterProfileError("whole English pair: one-byte template required");
  const auto &byte=tool(source->shared_recipe(),a.parents[0].target);
  if(byte.parents.size()!=2)throw PreparedLetterProfileError("whole English pair: ordered nibble parents required");
  auto limits=capacity(false).limits();
  limits[field::Budget::Fields]=6;limits[field::Budget::Batch]=64;
  limits[field::Budget::Participation]=32;limits[field::Budget::Contacts]=6;
  limits[field::Budget::CascadeReferences]=limits[field::Budget::CascadeCoverage]=64;
  limits[field::Budget::ScratchBytes]=field::detail::PreparedStorage::required_scratch(limits,false,field::NumericalProfile::trial())+256*1024*1024;
  PreparedStudySubmission out{source,{},field::PreparedCapacity(limits,field::NumericalProfile::trial()),
      std::make_shared<detail::ReservationDomain>(detail::ReservationBudget{16*1024*1024,16*1024*1024})};
  out.path=path;out.editable_occurrences=true;
  const auto binding=[&](const std::shared_ptr<const ActiveAssembly> &view,bool parts) {
    detail::AdmissionBindings b;
    b.source={view->owner(),view->study(),view->generation(),view->shared_recipe().ref};
    b.layout={{{nullptr,-1,ScaleRole::Root},1},{{nullptr,0,ScaleRole::PrimaryStep},64}};
    for(const auto entry:{std::uint64_t(2),std::uint64_t(10)}) {
      warm::OccurrenceId id{entry,{}};if(parts)id.path={0,0};
      b.bodies.push_back({{view->study(),std::move(id)},parts?byte.ref:english,parts?0:1,3,
          (parts?80:40)+int(b.bodies.size()),2.,{parts?2u:1u,1,true}});
    }
    const FieldKey member{GroupKind::Membership,focus.object};
    if(parts) {
      const FieldKey hi{GroupKind::ParentValue,byte.parents[0].target.object},
                     lo{GroupKind::ParentValue,byte.parents[1].target.object},
                     sibling{GroupKind::SiblingDefinition,byte.ref.object};
      b.fields={{hi,5,3},{lo,5,4},{sibling,5,5},{member,5,2}};
      b.comparisons={{hi,110,0,52,{}},{lo,111,0,52,{}},{sibling,112,1,52,{0,1}},{member,102,2,52,{2}}};
    } else {
      const FieldKey sibling{GroupKind::SiblingDefinition,english.object},parent{GroupKind::ParentValue,byte.ref.object};
      b.fields={{sibling,5,0},{parent,5,1},{member,5,2}};
      b.comparisons={{sibling,100,0,52,{}},{parent,101,1,52,{0}},{member,102,2,52,{1}}};
    }
    b.focus={member};return b;
  };
  out.bindings=binding(source,false);
  auto alternate=std::make_shared<const ActiveAssembly>(source->expose_all(warm::Exposure::Parts));
  out.lod_alternative=PreparedLodAlternative{alternate,binding(alternate,true)};
  out.plan_work=out.metadata_work=10000000;out.placement_work=100000000;out.placement_cells=65536;out.hierarchy_work=1000000;
  return out;
}
assembly::PreparedStudySubmission unicode_i_submission(
    warm::Bundle definitions, const warm::ToolRef &endpoint, warm::Uuid owner,
    field::HostPath path) {
  using namespace assembly;
  using field::Budget;
  const auto &letter = tool(definitions,endpoint);
  if (letter.notation != u8"í" || letter.mass != 4 || letter.parents.size() != 1)
    throw PreparedLetterProfileError("prepared í: one four-count character required");
  const auto character = letter.parents.front().target;
  const auto &scalar = tool(definitions,character);
  if (scalar.mass != 4 || scalar.parents.size() != 2)
    throw PreparedLetterProfileError("prepared í: two ordered bytes required");
  std::array<warm::ToolRef,2> bytes{scalar.parents[0].target,scalar.parents[1].target};
  constexpr std::array<const char *,2> names{"C3","AD"};
  detail::AdmissionBindings fine;
  fine.layout={{{nullptr,-1,ScaleRole::Root},1},{{nullptr,0,ScaleRole::PrimaryStep},64}};
  const auto add=[&](FieldKey key, int level, std::vector<int> children) {
    const int address=int(fine.fields.size());
    fine.fields.push_back({key,5,address});
    fine.comparisons.push_back({std::move(key),std::uint64_t(100+address),level,52,std::move(children)});
  };
  for (std::size_t i=0; i<bytes.size(); ++i) {
    const auto &byte=tool(definitions,bytes[i]);
    if (byte.notation!=names[i] || byte.mass!=2 || byte.parents.size()!=2)
      throw PreparedLetterProfileError("prepared í: the pinned construction must be C3 AD");
    for (const auto &parent:byte.parents)
      add({GroupKind::ParentValue,parent.target.object},0,{});
  }
  for (int i=0; i<int(bytes.size()); ++i)
    add({GroupKind::SiblingDefinition,bytes[i].object},1,{2*i,2*i+1});
  const FieldKey focus{GroupKind::ParentValue,character.object};
  add(focus,2,{4,5});
  fine.focus={focus};

  constexpr std::uint64_t entry=2;
  definitions.focal_fields={{warm::FieldKind::SiblingDefinition,bytes.front().object}};
  auto recipe=normalize_structural_letters(std::move(definitions),
      {{entry,endpoint,warm::Exposure::Parts}});
  auto parts=ActiveAssembly::assemble(std::move(recipe),std::move(owner),false)
      .focus({{focus.kind,InputText(warm::key(focus.object))}});
  auto alternate=std::make_shared<const ActiveAssembly>(std::move(parts));
  auto source=std::make_shared<const ActiveAssembly>(alternate->expose_all(warm::Exposure::Aggregate));
  fine.source={alternate->owner(),alternate->study(),alternate->generation(),alternate->shared_recipe().ref};
  fine.character_rollup=detail::AdmissionCharacterRollup{{alternate->study(),{entry,{}}},character};
  for (std::uint32_t i=0; i<bytes.size(); ++i)
    fine.bodies.push_back({{alternate->study(),{entry,{0,i}}},bytes[i],0,3,int(i)+1,2.,{i+1,1,true}});

  const FieldKey sibling{GroupKind::SiblingDefinition,endpoint.object};
  detail::AdmissionBindings coarse;
  coarse.source={source->owner(),source->study(),source->generation(),source->shared_recipe().ref};
  coarse.layout=fine.layout;
  coarse.bodies=declared_aggregate_bodies(*source);
  coarse.fields={{sibling,5,int(fine.fields.size())},fine.fields.back()};
  coarse.comparisons={{sibling,200,0,52,{}},{focus,fine.comparisons.back().comparison,2,52,{0}}};
  coarse.focus={focus};

  auto limits=capacity(false).limits();
  limits[Budget::Potential]=limits[Budget::Particles]=3;
  limits[Budget::Fields]=limits[Budget::CascadeNodes]=8;
  limits[Budget::Batch]=64;
  limits[Budget::Participation]=32;
  limits[Budget::CascadeLinks]=16;
  limits[Budget::CascadeReferences]=limits[Budget::CascadeCoverage]=64;
  limits[Budget::LiveResponses]=128;
  const auto profile=field::NumericalProfile::trial();
  limits[Budget::ScratchBytes]=field::detail::PreparedStorage::required_scratch(limits,false,profile)
      +256*1024*1024;
  PreparedStudySubmission request{source,std::move(coarse),field::PreparedCapacity(limits,profile),
      std::make_shared<detail::ReservationDomain>(detail::ReservationBudget{16*1024*1024,16*1024*1024})};
  request.path=path;
  request.lod_alternative=PreparedLodAlternative{std::move(alternate),std::move(fine)};
  return request;
}
warm::Bundle prepared_retained_english_pair(warm::Bundle b, const warm::ToolRef &english) {
  return recipe(std::move(b),english,true);
}
assembly::PreparedStudySubmission prepared_english_pair_submission(
    std::shared_ptr<const assembly::ActiveAssembly> source, const warm::ToolRef &english, field::HostPath path) {
  return submission(std::move(source),english,path,false);
}
assembly::PreparedStudySubmission prepared_retained_english_pair_submission(
    std::shared_ptr<const assembly::ActiveAssembly> source, const warm::ToolRef &english, field::HostPath path) {
  return submission(std::move(source),english,path,true);
}
assembly::PreparedStudySubmission editable_english_pair_submission(
    std::shared_ptr<const assembly::ActiveAssembly> source, const warm::ToolRef &english, field::HostPath path) {
  auto out = prepared_english_pair_submission(std::move(source),english,path);
  // The live study starts from the English A baseline mass, without the older
  // demonstration's physical-mass override. Exact source mass is already two.
  for (auto &body : out.bindings.bodies) body.mass=2.;
  enable_occurrence_edits(out);
  return out;
}
assembly::PreparedStudySubmission editable_retained_english_pair_submission(
    std::shared_ptr<const assembly::ActiveAssembly> source, const warm::ToolRef &english, field::HostPath path) {
  auto out = prepared_retained_english_pair_submission(std::move(source),english,path);
  enable_occurrence_edits(out);
  return out;
}
warm::Bundle retained_catalogue_recipe(warm::Bundle definitions,
                                       const std::vector<warm::ToolRef> &roots,
                                       const std::optional<warm::ObjectKey> &focus_byte) {
  if (roots.empty()) throw PreparedLetterProfileError("retained catalogue requires declared roots");
  const auto groups = catalogue_memberships(definitions,roots.front());
  if (groups.empty()) throw PreparedLetterProfileError("retained catalogue requires membership focus");
  std::vector<assembly::StructuralLetterRequest> requests;
  for (const auto &pin : roots) {
    if (focus_byte) {
      const auto &root = tool(definitions,pin);
      if (root.mass!=2 || root.parents.size()!=1 || root.parents.front().contribution!=2 ||
          !(root.parents.front().target.object==*focus_byte))
        throw PreparedLetterProfileError("selected focus byte must be the direct byte of every declared letter");
      catalogue_membership_path(definitions,pin);
    } else if (catalogue_memberships(definitions,pin)!=groups)
      throw PreparedLetterProfileError("different membership cohorts require an explicit shared --focus-byte");
    requests.push_back({requests.size()+1,pin,warm::Exposure::Parts});
  }
  definitions.focal_fields = {focus_byte ? warm::FocalField{warm::FieldKind::SiblingDefinition,*focus_byte}
                                        : warm::FocalField{warm::FieldKind::Membership,*groups.begin()}};
  return assembly::normalize_structural_letters(std::move(definitions),requests);
}

assembly::PreparedStudySubmission editable_retained_catalogue_submission(
    std::shared_ptr<const assembly::ActiveAssembly> aggregate, std::uint64_t initial_entry,
    field::HostPath path) {
  using namespace assembly;
  using field::Budget;
  const auto &declared_focus = aggregate->shared_recipe().focal_fields.at(0);
  const auto focus = declared_focus.object;
  const bool byte_focus = declared_focus.kind==warm::FieldKind::SiblingDefinition;
  if (byte_focus) {
    // The retained recipe names the exposed byte. The prepared owner follows
    // its construction field across both exposure plans.
    aggregate = std::make_shared<const ActiveAssembly>(
        aggregate->focus({{GroupKind::ParentValue,InputText(warm::key(focus))}}));
  }
  auto parts = std::make_shared<const ActiveAssembly>(aggregate->expose_all(warm::Exposure::Parts));
  const auto &recipe = aggregate->shared_recipe();
  using FieldIdentity = std::pair<GroupKind,warm::ObjectKey>;
  std::map<FieldIdentity,int> addresses;
  std::map<warm::ObjectKey,std::size_t> definition_index;
  for (std::size_t i=0; i<recipe.tools.size(); ++i)
    definition_index.emplace(recipe.tools[i].ref.object,i);

  detail::AdmissionBindings coarse, fine;
  coarse.bodies = declared_aggregate_bodies(*aggregate);
  for (const auto &row : recipe.structural_occurrences) {
    if (row.id.path.empty()) continue;
    const auto index = definition_index.at(row.definition);
    const auto &definition = recipe.tools[index];
    fine.bodies.push_back({{parts->study(),row.id},definition.ref,row.node,3,
        int(fine.bodies.size()),double(definition.mass.value()),{index+1,1,true}});
  }
  const auto groups = catalogue_memberships(recipe,coarse.bodies.at(0).definition);
  if (recipe.focal_fields.size()!=1 || (!byte_focus &&
      (recipe.focal_fields[0].kind!=warm::FieldKind::Membership || !groups.count(focus))))
    throw PreparedLetterProfileError("invalid retained catalogue focus");
  std::vector<std::vector<warm::ObjectKey>> membership_paths;
  if (byte_focus)
    for (const auto &body : coarse.bodies)
      membership_paths.push_back(catalogue_membership_path(recipe,body.definition));
  bool shared_parents = false;
  const auto fields = [&](detail::AdmissionBindings &binding,const ActiveAssembly &view,bool expanded) {
    binding.source = {view.owner(),view.study(),view.generation(),recipe.ref};
    binding.layout = {{{nullptr,-1,ScaleRole::Root},1},{{nullptr,0,ScaleRole::PrimaryStep},64}};
    const auto add = [&](FieldKey key,int level,std::vector<int> children) {
      const auto [at,unused] = addresses.try_emplace({key.kind,key.object},int(addresses.size()));
      const int index = int(binding.fields.size());
      binding.fields.push_back({key,5,at->second});
      binding.comparisons.push_back({std::move(key),std::uint64_t(at->second)+1,level,52,std::move(children)});
      return index;
    };
    std::map<warm::ObjectKey,int> siblings, parent_fields;
    std::vector<int> branches;
    for (const auto &body : binding.bodies) {
      const auto [sibling,inserted] = siblings.try_emplace(body.definition.object,-1);
      if (!inserted) continue;
      const auto &definition = tool(recipe,body.definition);
      std::set<warm::ObjectKey> direct;
      std::vector<int> children;
      for (const auto &parent : definition.parents) {
        if (!direct.insert(parent.target.object).second) continue;
        const auto [field,created] = parent_fields.try_emplace(parent.target.object,-1);
        if (created) field->second = add({GroupKind::ParentValue,parent.target.object},0,{});
        else shared_parents = true;
        children.push_back(field->second);
      }
      sibling->second = add({GroupKind::SiblingDefinition,body.definition.object},1,std::move(children));
      branches.push_back(sibling->second);
    }
    if (byte_focus) {
      const FieldKey root{GroupKind::ParentValue,focus};
      int byte = expanded ? add(root,2,std::move(branches)) : parent_fields.at(focus);
      struct MembershipNode {
        int level{};
        std::optional<warm::ObjectKey> parent;
        std::vector<int> children;
      };
      std::map<warm::ObjectKey,MembershipNode> memberships;
      for (std::size_t i=0; i<membership_paths.size(); ++i) {
        const auto &path = membership_paths[i];
        const int child = expanded ? byte : siblings.at(coarse.bodies[i].definition.object);
        auto &inputs = memberships[path.front()].children;
        if (std::find(inputs.begin(),inputs.end(),child)==inputs.end()) inputs.push_back(child);
        for (std::size_t j=0; j<path.size(); ++j) {
          auto &node = memberships[path[j]];
          node.level = std::max(node.level,int(j)+3);
          if (j+1<path.size()) node.parent = path[j+1];
        }
      }
      std::vector<std::pair<int,warm::ObjectKey>> order;
      for (const auto &[key,node] : memberships) order.emplace_back(node.level,key);
      std::sort(order.begin(),order.end());
      for (const auto &[level,key] : order) {
        auto &node = memberships.at(key);
        const auto child = add({GroupKind::Membership,key},level,std::move(node.children));
        if (node.parent) memberships.at(*node.parent).children.push_back(child);
      }
      binding.focus = {root};
    } else {
      int level = 2;
      for (const auto &group : groups)
        if (!(group==focus)) branches = {add({GroupKind::Membership,group},level++,std::move(branches))};
      const FieldKey root{GroupKind::Membership,focus};
      add(root,level,std::move(branches));
      binding.focus = {root};
    }
  };
  fields(coarse,*aggregate,false);
  fields(fine,*parts,true);

  auto limits = capacity(false).limits();
  std::size_t maximum_width = 1;
  for (const auto &selection : recipe.selections)
    maximum_width = std::max(maximum_width,selection.parts.size());
  limits[Budget::Particles] = 2*std::max(coarse.bodies.size(),fine.bodies.size());
  limits[Budget::Potential] = limits[Budget::Particles]*(1+maximum_width);
  limits[Budget::Fields] = limits[Budget::CascadeNodes] = addresses.size();
  limits[Budget::CascadeLevels] = addresses.size();
  limits[Budget::Participation] = limits[Budget::Particles]*limits[Budget::Fields];
  limits[Budget::CascadeLinks] = addresses.size()+((shared_parents || byte_focus) ? limits[Budget::Participation] : 0);
  limits[Budget::CascadeReferences] = limits[Budget::CascadeCoverage] = limits[Budget::Participation];
  limits[Budget::LiveResponses] = 2*limits[Budget::Participation];
  limits[Budget::BootstrapCommands] = limits[Budget::Batch] =
      2*(limits[Budget::Particles]+limits[Budget::Fields]+limits[Budget::Participation]);
  const auto profile = field::NumericalProfile::trial();
  const auto hierarchy = byte_focus ? field::PreparedHierarchyMode::CrossLanguageCatalogue
      : shared_parents ? field::PreparedHierarchyMode::ScopedCatalogue : field::PreparedHierarchyMode::ExpressedTree;
  limits[Budget::ScratchBytes] = field::detail::PreparedStorage::required_scratch(
      limits,false,profile)+256*1024*1024;
  PreparedStudySubmission request{aggregate,std::move(coarse),field::PreparedCapacity(limits,profile,hierarchy),
      std::make_shared<detail::ReservationDomain>(detail::ReservationBudget{16*1024*1024,16*1024*1024})};
  request.path = path;
  request.editable_occurrences = true;
  request.lod_alternative = PreparedLodAlternative{std::move(parts),std::move(fine),{initial_entry}};
  return request;
}
}
