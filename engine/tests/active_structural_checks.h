#pragma once

#include "active_workbench_checks.h"
#include "letter_projection_fixture.h"

namespace active_checks {
inline void structural_binding() {
    letter_fixture::Fixture f;
    auto base = f.base;
    base.focal_fields = {{warm::FieldKind::ParentValue,
        f.base.tools.at(f.tools.at(f.english)).parents[0].target.object}};
    const auto b = normalize_structural_letters(base,
        {{UINT64_MAX, f.ref(f.english), warm::Exposure::Aggregate},
         {3, f.ref(f.english), warm::Exposure::Parts},
         {7, f.ref(f.dutch), warm::Exposure::Aggregate},
         {8, f.ref(f.repeated), warm::Exposure::Parts}});
    auto source = b;
    std::reverse(source.structural_occurrences.begin(), source.structural_occurrences.end());
    const auto p = project_letters(source);
    std::vector<StudyNode> nodes;
    for (const auto &n : b.nodes)
      nodes.push_back({reinterpret_cast<const taichi::lang::SNode *>(1),
                       n.parent, static_cast<assembly::ScaleRole>(n.role)});
    const auto input = membership_fields(p).bind_structure(p, nodes);
    const auto active = ActiveAssembly::assemble(b, warm::new_uuid(), false);
    const auto owned = active.bind_structure(nodes);
    check(owned.occurrences.size() == input.occurrences.size() &&
              owned.definitions.size() == input.definitions.size() &&
              owned.memberships.size() == input.memberships.size() &&
              owned.contacts.size() == input.contacts.size() &&
              owned.selections.size() == input.selections.size(),
          "active owner exposes the complete structural source");
    // The earlier source-death fixture deliberately reversed source order.
    // Independent projections also have different study UUIDs: compare source paths.
    for (const auto &row : owned.occurrences) {
      const auto found = std::find_if(input.occurrences.begin(), input.occurrences.end(),
          [&](const auto &expected) { return expected.id.substr(expected.id.find('/')) == row.id.substr(row.id.find('/')); });
      check(std::string(row.id).rfind(std::string(active.study().str()) + "/", 0) == 0 &&
                found != input.occurrences.end() && row.definition == found->definition &&
                row.study_node == found->study_node,
            "active structural occurrence identity and conceptual role unchanged");
    }
    check(owned.focal_fields.size() == 1 &&
              owned.focal_fields[0].value == input.focal_fields[0].value &&
              owned.focal_fields[0].kind == input.focal_fields[0].kind,
          "structural active binding inherits recipe focus");
    check(active.focus({}).revise({}).bind_structure(nodes).focal_fields.empty(),
          "explicit empty focus survives revision and clears recipe focus");
    check(active.focus({}).restart().bind_structure(nodes).focal_fields.empty(),
          "explicit empty focus survives logical restart");
    const auto replacement = assembly::FocalField{GroupKind::SiblingDefinition, InputText(warm::key(f.ref(f.english).object))};
    const auto focused = active.focus({replacement}).bind_structure(nodes);
    check(focused.focal_fields.size() == 1 && focused.focal_fields[0].value == replacement.value,
          "explicit focus replaces rather than appends to recipe focus");
    rejects([&] { active.bind({}); }, "v2 active source cannot use legacy binding");
    rejects([&] { active.bind_structure({}); }, "structural active binding validates layout");
    rejects([&] { ActiveAssembly::assemble(b, warm::new_uuid(), true).bind_structure(nodes); },
            "required missing local inputs cannot be bypassed");
    LocalInput local;
    local.source = {letter_fixture::object("00.00.00.00.01", active.instance()), warm::new_uuid(), active.owner()};
    rejects([&] { active.revise(local).bind_structure(nodes); },
            "present local overlays reject rather than disappearing from structural source");
    const auto legacy_active = ActiveAssembly::assemble(normalize_letters(f.base,
        {f.request(f.english, 0, warm::Exposure::Aggregate)}), warm::new_uuid(), false);
    rejects([&] { legacy_active.bind_structure(nodes); }, "legacy recipe cannot silently enter structural binding");
}
}  // namespace active_checks
