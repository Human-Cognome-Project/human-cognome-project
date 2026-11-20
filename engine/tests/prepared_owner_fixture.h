#include "assembly/projection_access.h"
#pragma once
#include "assembly/prepared_study_owner.h"
#include "letter_projection_fixture.h"
#include "field/prepared_storage.h"
#include "taichi/ir/snode.h"

// Explicit preparing-owner bindings for a tiny pinned source. This fixture
// supplies no numerical starts and performs no fixture admission.
namespace prepared_owner_fixture {
using namespace assembly;
using namespace assembly::detail;
struct Input {
  std::shared_ptr<const ActiveAssembly> source;
  warm::Uuid session{warm::new_uuid()};
  AdmissionBindings bindings;
  std::unique_ptr<taichi::lang::SNode> root;
  explicit Input(bool parts = false) {
    letter_fixture::Fixture fixture;
    const auto letter = fixture.ref(fixture.english);
    const auto byte = fixture.base.tools[fixture.tools.at(fixture.english)].parents[0].target;
    const auto &byte_tool = fixture.base.tools[fixture.tools.at(byte.object)];
    fixture.base.focal_fields = {{parts ? warm::FieldKind::SiblingDefinition : warm::FieldKind::ParentValue,byte.object}};
    auto recipe = normalize_structural_letters(fixture.base,
        {{10,letter,parts ? warm::Exposure::Parts : warm::Exposure::Aggregate},
         {2,letter,parts ? warm::Exposure::Parts : warm::Exposure::Aggregate}});
    source = std::make_shared<const ActiveAssembly>(ActiveAssembly::assemble(recipe,warm::new_uuid(),false));
    root = std::make_unique<taichi::lang::SNode>(0,taichi::lang::SNodeType::root);
    auto *parent = root.get();
    bindings.layout.push_back({{parent,-1,ScaleRole::Root},1});
    if (parts) {
      parent = &parent->dense(taichi::lang::Axis(0),1);
      bindings.layout.push_back({{parent,0,ScaleRole::PrimaryStep},64});
    }
    parent->insert_children(taichi::lang::SNodeType::place).dt = taichi::lang::PrimitiveType::i32;
    bindings.source = {source->owner(),source->study(),source->generation(),recipe.ref};
    const auto projected = project_letters(recipe);
    int address = 40;
    for (const auto &o : assembly::detail::ProjectionAccess::rows(projected)) if (o.active) {
      auto key = o.key; key.study = source->study();
      bindings.bodies.push_back({std::move(key),assembly::detail::ProjectionAccess::definition(projected,o),o.node,3,address++,1.5,{1,7,true}});
    }
    const auto add = [&](GroupKind kind, const warm::ObjectKey &key, std::vector<int> children) {
      const int i = int(bindings.fields.size()); const FieldKey field{kind,key};
      bindings.fields.push_back({field,5,i});
      bindings.comparisons.push_back({field,std::uint64_t(i+100),i,52,std::move(children)});
    };
    if (parts) {
      add(GroupKind::ParentValue,byte_tool.parents[0].target.object,{});
      add(GroupKind::ParentValue,byte_tool.parents[1].target.object,{});
      add(GroupKind::SiblingDefinition,byte.object,{0,1});
    } else {
      add(GroupKind::SiblingDefinition,letter.object,{});
      add(GroupKind::ParentValue,byte.object,{0});
    }
    bindings.focus.push_back(bindings.fields.back().field);
    std::reverse(bindings.bodies.begin(),bindings.bodies.end());
  }
  void install_layout(engine::Runtime &runtime) {
    runtime.program().add_snode_tree(std::move(root),false);
  }
};
inline field::PreparedCapacity capacity() {
  using field::Budget;
  field::BudgetDemand l;
  l[Budget::Potential] = 128;
  l[Budget::Particles] = 4; l[Budget::Fields] = 4;
  l[Budget::Participation] = 16; l[Budget::Contacts] = 4;
  l[Budget::References] = 1024; l[Budget::Focus] = 2; l[Budget::Batch] = 1;
  l[Budget::BootstrapCommands] = 32;
  l[Budget::GeometryWork] = l[Budget::CascadeWork] = 1000000;
  l[Budget::CascadeNodes] = 4; l[Budget::CascadeLinks] = 4;
  l[Budget::CascadeReferences] = 32; l[Budget::CascadeLevels] = 4;
  l[Budget::CascadeCoverage] = 32; l[Budget::LiveResponses] = 64;
  l[Budget::PlacementWork] = 100000000; l[Budget::PlacementCells] = 65536;
  l[Budget::TickWork] = 1000000000;
  l[Budget::ScratchBytes] = field::detail::PreparedStorage::required_scratch(
      l,false,field::NumericalProfile::trial()) + 256*1024*1024;
  return field::PreparedCapacity(l,field::NumericalProfile::trial());
}
inline auto domain(ReservationBudget budget = {UINT64_MAX,UINT64_MAX}) {
  return std::make_shared<ReservationDomain>(budget);
}
inline auto prepare(field::Harness &h, const Input &input,
    const std::shared_ptr<ReservationDomain> &d,
    std::uint64_t plan_work = UINT64_MAX, std::uint64_t metadata_work = UINT64_MAX) {
  return PreparedStudyOwner::prepare(h,input.source,input.session,input.bindings,d,plan_work,metadata_work);
}
}
