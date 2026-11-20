#include "assembly/projection_access.h"
#pragma once
#include "prepared_owner_fixture.h"

// Shared synthetic full-closure input; never a retained acquisition.
namespace retained_membership_fixture {
using namespace assembly;
using namespace assembly::detail;
using field::Budget;
inline field::PreparedCapacity capacity() {
  auto limits = prepared_owner_fixture::capacity().limits();
  limits[Budget::Fields] = limits[Budget::CascadeNodes] = limits[Budget::CascadeLevels] = 6;
  limits[Budget::CascadeLinks] = 5;
  limits[Budget::ScratchBytes] = field::detail::PreparedStorage::required_scratch(
      limits,false,field::NumericalProfile::trial()) + 256*1024*1024;
  return field::PreparedCapacity(limits,field::NumericalProfile::trial());
}
inline auto resources() { return std::make_shared<ReservationDomain>(ReservationBudget{16*1024*1024,16*1024*1024}); }
struct Input {
  letter_fixture::Fixture fixture;
  std::vector<warm::ObjectKey> groups{
    letter_fixture::object("02.00.00.00.01"),letter_fixture::object("02.00.00.00.00"),
    letter_fixture::object("02.00.zz.00.00"),letter_fixture::object("02.0z.zz.00.00")};
  std::shared_ptr<const ActiveAssembly> source;
  std::unique_ptr<taichi::lang::SNode> root;
  AdmissionBindings bindings;
  Input() {
    const std::vector<std::string> names{"Alphabet (en)","English","Germanic","Indo-European"};
    std::vector<warm::ToolRef> refs;
    for (std::size_t i=0; i<groups.size(); ++i) {
      warm::Tool t{{groups[i],warm::new_uuid()},fixture.epoch,"synthetic-membership-decision",
        warm::RecipeText(names[i]),std::nullopt,{}, {},fixture.actor};
      fixture.tools.emplace(groups[i],fixture.base.tools.size());
      refs.push_back(t.ref); fixture.base.tools.push_back(std::move(t));
    }
    // Full retained shape, with a diamond: A -> English and Alphabet -> English.
    // Tool revisions/byte addresses remain synthetic; no cold record is mutated.
    fixture.base.tools[fixture.tools.at(fixture.english)].member_of = {refs[1],refs[0]};
    for (std::size_t i=0; i+1<refs.size(); ++i)
      fixture.base.tools[fixture.tools.at(groups[i])].member_of = {refs[i+1]};
    for (auto &g : fixture.base.geometry) if (g.object == fixture.english) g.spheres[0].radius = 0.125;
    const auto english = fixture.ref(fixture.english);
    const auto parent = fixture.base.tools[fixture.tools.at(fixture.english)].parents[0].target.object;
    fixture.base.focal_fields = {{warm::FieldKind::ParentValue,parent}};
    auto recipe = normalize_structural_letters(fixture.base,
      {{10,english,warm::Exposure::Aggregate},{2,english,warm::Exposure::Aggregate}});
    source = std::make_shared<const ActiveAssembly>(ActiveAssembly::assemble(recipe,warm::new_uuid(),false));
    root = std::make_unique<taichi::lang::SNode>(0,taichi::lang::SNodeType::root);
    root->insert_children(taichi::lang::SNodeType::place).dt = taichi::lang::PrimitiveType::i32;
    bindings.layout = {{{root.get(),-1,ScaleRole::Root},1}};
    bindings.source = {source->owner(),source->study(),source->generation(),recipe.ref};
    const auto projection = project_letters(recipe);
    for (const auto &o : assembly::detail::ProjectionAccess::rows(projection)) if (o.active) {
      auto occurrence = o.key; occurrence.study = source->study();
      bindings.bodies.push_back({std::move(occurrence),assembly::detail::ProjectionAccess::definition(projection,o),0,3,40+int(bindings.bodies.size()),1.5,{1,1,true}});
    }
    const auto add = [&](FieldKey field) {
      const int i = int(bindings.fields.size());
      bindings.fields.push_back({field,5,i});
      bindings.comparisons.push_back({field,std::uint64_t(100+i),i ? 1 : 0,52,i ? std::vector<int>{i-1} : std::vector<int>{}});
    };
    add({GroupKind::SiblingDefinition,fixture.english});
    add({GroupKind::ParentValue,parent});
    for (const auto &group : groups) add({GroupKind::Membership,group});
    bindings.focus = {bindings.fields[1].field};
  }
  auto build(const AdmissionBindings &b, const field::PreparedCapacity &cap) const {
    return StructuralAdmissionPlan::build(*source,b,cap,resources(),10000000);
  }
};
}
