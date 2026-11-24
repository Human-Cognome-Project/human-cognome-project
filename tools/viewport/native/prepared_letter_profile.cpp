#include "prepared_letter_profile.h"
#include "field/prepared_storage.h"
#include <array>

namespace viewport {
namespace {
const warm::Tool &tool(const warm::Bundle &b, const warm::ToolRef &ref) {
  for (const auto &t : b.tools) if (t.ref == ref) return t;
  throw PreparedLetterProfileError("prepared English pair: missing definition pin");
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
  const auto projected = project_letters(recipe);
  for (std::size_t i=0;i<projected.occurrence_count();++i) {
    const auto o=projected.occurrence(i);
    if (!o.active) continue;
    if (!(o.definition.object == english.object) || o.definition.revision != english.revision ||
        o.node != 0 || !o.key.source.path.empty() ||
        (o.key.source.entry != 2 && o.key.source.entry != 10) || b.bodies.size() >= 2)
      throw PreparedLetterProfileError("prepared English pair: unsupported active selection");
    auto occurrence = o.key; occurrence.study = source->study();
    b.bodies.push_back({std::move(occurrence),o.definition,0,3,40+int(b.bodies.size()),1.5,{1,1,true}});
  }
  if (b.bodies.size() != 2) throw PreparedLetterProfileError("prepared English pair: two aggregate occurrences required");
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
warm::Bundle prepared_english_pair(warm::Bundle b, const warm::ToolRef &english) {
  return recipe(std::move(b),english,false);
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
  auto limits = out.capacity.limits();
  limits[field::Budget::Batch] = 32;
  limits[field::Budget::ScratchBytes] = field::detail::PreparedStorage::required_scratch(
      limits,false,out.capacity.profile()) + 256*1024*1024;
  out.capacity = field::PreparedCapacity(limits,out.capacity.profile());
  out.editable_occurrences = true;
  return out;
}
}
