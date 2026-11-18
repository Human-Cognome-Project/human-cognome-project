#include "prepared_letter_profile.h"
#include "field/prepared_storage.h"

namespace viewport {
namespace {
const warm::Tool &letter(const warm::Bundle &b, const warm::ToolRef &ref) {
  for (const auto &t : b.tools)
    if (t.ref.object == ref.object && t.ref.revision == ref.revision) {
      // This preset specifies the English A aggregate's direct, single-byte
      // construction. A different source shape needs a separately chosen profile.
      if (!t.member_of.empty())
        throw PreparedLetterProfileError("prepared English pair: membership comparison choices are unsupported");
      if (t.ref.object.scope.nil() && warm::address_text(t.ref.object.address) == "02.00.00.00.03" &&
          t.mass == 2 && t.notation && *t.notation == "A" &&
          t.parents.size() == 1 && t.parents[0].contribution == 2)
        return t;
      break;
    }
  throw PreparedLetterProfileError("prepared English pair: unsupported source construction");
}
field::PreparedCapacity capacity() {
  using field::Budget;
  field::BudgetDemand l;
  l[Budget::Potential] = 128;
  l[Budget::Particles] = 4; l[Budget::Fields] = 4;
  l[Budget::Participation] = 16; l[Budget::Contacts] = 4;
  l[Budget::References] = 1024; l[Budget::Focus] = 2; l[Budget::Batch] = 1;
  l[Budget::BootstrapCommands] = 32;
  l[Budget::GeometryWork] = l[Budget::CascadeWork] = 1000000;
  l[Budget::CascadeNodes] = l[Budget::CascadeLinks] = l[Budget::CascadeLevels] = 4;
  l[Budget::CascadeReferences] = l[Budget::CascadeCoverage] = 32;
  l[Budget::LiveResponses] = 64;
  l[Budget::PlacementWork] = 100000000; l[Budget::PlacementCells] = 65536;
  l[Budget::TickWork] = 1000000000;
  l[Budget::ScratchBytes] = field::detail::PreparedStorage::required_scratch(
      l,false,field::NumericalProfile::trial()) + 256*1024*1024;
  return field::PreparedCapacity(l,field::NumericalProfile::trial());
}
}
warm::Bundle prepared_english_pair(warm::Bundle definitions, const warm::ToolRef &english) {
  const auto parent = letter(definitions,english).parents[0].target.object;
  // Geometry is a choice of this explicitly selected profile, not a cold value.
  for (auto it = definitions.geometry.begin(); it != definitions.geometry.end();)
    if (it->object == english.object) it = definitions.geometry.erase(it); else ++it;
  definitions.geometry.push_back({english.object,{},{{{},0.125}}});
  definitions.focal_fields = {{warm::FieldKind::ParentValue,parent}};
  return assembly::normalize_structural_letters(std::move(definitions),
      {{10,english,warm::Exposure::Aggregate},{2,english,warm::Exposure::Aggregate}});
}
assembly::PreparedStudySubmission prepared_english_pair_submission(
    std::shared_ptr<const assembly::ActiveAssembly> source, const warm::ToolRef &english,
    field::HostPath path) {
  using namespace assembly;
  if (!source || !source->inputs_available() || source->local() || source->shared_recipe().recipe_version != 2)
    throw PreparedLetterProfileError("prepared English pair: shared v2 source required");
  const auto &recipe = source->shared_recipe();
  const auto parent = letter(recipe,english).parents[0].target.object;
  PreparedStudySubmission out{source,{},capacity(),
      std::make_shared<detail::ReservationDomain>(detail::ReservationBudget{16*1024*1024,16*1024*1024})};
  out.path = path;
  auto &b = out.bindings;
  b.source = {source->owner(),source->study(),source->generation(),recipe.ref};
  b.layout.push_back({{nullptr,-1,ScaleRole::Root},1});
  const auto projected = project_letters(recipe);
  for (const auto &o : projected.occurrences()) if (o.active) {
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
  b.focus = {value};
  out.plan_work = out.metadata_work = 10000000;
  out.placement_work = 100000000; out.placement_cells = 65536; out.hierarchy_work = 1000000;
  return out;
}
}
