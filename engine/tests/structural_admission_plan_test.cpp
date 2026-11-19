#include "assembly/structural_admission_plan.h"
#include "assembly/intrinsic_geometry.h"
#include "letter_projection_fixture.h"
#include "membership_fields_access.h"
#include "taichi/ir/snode.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#define HCP_ALLOCATION_LIFETIME
#include "allocation_observation.h"

namespace assembly::testing {
struct ActiveAssemblyAccess {
  static auto &memberships(ActiveAssembly &value) { return value.memberships_; }
};
}

namespace {
using namespace assembly;
using namespace assembly::detail;
using field::Budget;
using Status = AdmissionPlanStatus;
int checks = 0;
void check(bool ok, const char *label) {
  if (!ok) throw std::runtime_error(label);
  ++checks;
}
auto domain(ReservationBudget b = {UINT64_MAX,UINT64_MAX}) {
  return std::make_shared<ReservationDomain>(b);
}
field::BudgetDemand limits() {
  field::BudgetDemand l;
  for (auto &v : l.values) v = 10000;
  l[Budget::Potential] = 256;
  l[Budget::Particles] = 128;
  l[Budget::Fields] = 64;
  l[Budget::Focus] = 64;
  l[Budget::Batch] = 1;
  l[Budget::ScratchBytes] = UINT64_C(1)<<30;
  return l;
}
// The test owner explicitly declares this known tree. Production never derives
// comparison topology from memberships, field names or SNode depth.
struct Example {
  letter_fixture::Fixture fixture;
  warm::ObjectKey left = letter_fixture::object("02.00.00.00.04");
  warm::ObjectKey right = letter_fixture::object("02.00.00.00.05");
  warm::ObjectKey group = letter_fixture::object("02.00.00.00.06");
  warm::ObjectKey letters = letter_fixture::object("02.00.20.00.1Z");
  std::unique_ptr<taichi::lang::SNode> root;
  std::unique_ptr<ActiveAssembly> assembly;
  AdmissionBindings bindings;
  bool parts;
  explicit Example(bool expose_parts = false, bool compression = false) : parts(expose_parts) {
    using namespace warm;
    const auto root_group = fixture.add(group,"group",99);
    const auto l = fixture.add(left,"left",71), r = fixture.add(right,"right",53);
    auto &tools = fixture.base.tools;
    tools[fixture.tools.at(left)].member_of.push_back(root_group);
    tools[fixture.tools.at(right)].member_of.push_back(root_group);
    std::vector<ToolRef> repeated(12,fixture.byte('A'));
    const auto word = fixture.add(letters,std::string(12,'A'),24,repeated);
    tools[fixture.tools.at(letters)].member_of = {l,r};
    fixture.base.geometry.push_back({letters,{},{{{},1+std::ldexp(1.0,-40)}}});
    for (auto &g : fixture.base.geometry) if (g.object == fixture.byte('A').object)
      g.spheres[0].radius = 0.5+std::ldexp(1.0,-40);
    fixture.base.focal_fields = {{warm::FieldKind::Membership,group}};
    auto recipe = normalize_structural_letters(fixture.base,
        {{10,word,parts ? warm::Exposure::Parts : warm::Exposure::Aggregate},
         {2,word,parts ? warm::Exposure::Parts : warm::Exposure::Aggregate}});
    if (compression) {
      recipe.nodes.push_back({0,warm::ScaleRole::SecondaryCompression});
      recipe.nodes.push_back({int(recipe.nodes.size()-1),warm::ScaleRole::NamingEquivalent});
      for (auto &o : recipe.structural_occurrences)
        if (parts && !o.id.path.empty()) o.node = int(recipe.nodes.size()-1);
    }
    const warm::OccurrenceId first{2,parts ? warm::OccurrencePath{0,0} : warm::OccurrencePath{}};
    const warm::OccurrenceId second{10,parts ? warm::OccurrencePath{0,0} : warm::OccurrencePath{}};
    const warm::OccurrenceId third{10,parts ? warm::OccurrencePath{1,0} : warm::OccurrencePath{}};
    recipe.contacts.push_back({first,second});
    if (parts) recipe.contacts.push_back({first,third});
    assembly = std::make_unique<ActiveAssembly>(ActiveAssembly::assemble(recipe,new_uuid(),false));
    root = std::make_unique<taichi::lang::SNode>(0,taichi::lang::SNodeType::root);
    bindings.source = {assembly->owner(),assembly->study(),assembly->generation(),recipe.ref};
    for (std::size_t i=0; i<recipe.nodes.size(); ++i) {
      const auto &n = recipe.nodes[i];
      const auto *node = !i ? root.get() :
          &const_cast<taichi::lang::SNode *>(bindings.layout[n.parent].node.layout)->insert_children(
              taichi::lang::SNodeType::dense);
      const double units = !i ? 1 : bindings.layout[n.parent].units *
          (n.role == warm::ScaleRole::PrimaryStep ? 64 : 1);
      bindings.layout.push_back({{node,n.parent,static_cast<assembly::ScaleRole>(n.role)},units});
    }
    const auto projection = project_letters(recipe);
    int address = 40;
    for (const auto &o : projection.occurrences()) if (o.active) {
      const bool aggregate = o.key.source.path.empty();
      auto key = o.key;
      key.study = assembly->study();
      bindings.bodies.push_back({std::move(key),o.definition,o.node,3,address++,1.25,
                                {aggregate ? 900U : 901U,7,true}});
    }
    const auto byte = fixture.byte('A');
    const auto &byte_tool = fixture.base.tools[fixture.tools.at(byte.object)];
    const auto add_field = [&](GroupKind kind, const ObjectKey &key) {
      const FieldKey field{kind,key};
      const int i = int(bindings.fields.size());
      bindings.fields.push_back({field,5,i});
      bindings.comparisons.push_back({field,std::uint64_t(100+i),0,52,{}});
    };
    if (parts) {
      add_field(GroupKind::ParentValue,byte_tool.parents[0].target.object);
      add_field(GroupKind::ParentValue,byte_tool.parents[1].target.object);
      add_field(GroupKind::SiblingDefinition,byte.object);
      bindings.comparisons.back().children = {0,1};
    } else {
      add_field(GroupKind::SiblingDefinition,letters);
      add_field(GroupKind::ParentValue,byte.object);
      bindings.comparisons.back().children = {0};
    }
    for (const auto &key : {left,right,group}) {
      const auto prior = int(bindings.comparisons.size()-1);
      add_field(GroupKind::Membership,key);
      bindings.comparisons.back().children = {prior};
    }
    for (std::size_t i=0; i<bindings.comparisons.size(); ++i)
      bindings.comparisons[i].level = int(i); // Explicit comparison labels, not depth.
    bindings.focus.push_back({GroupKind::Membership,group});
    std::reverse(bindings.bodies.begin(),bindings.bodies.end());
  }
  AdmissionPlanResult build(const AdmissionBindings &b, const std::shared_ptr<ReservationDomain> &d,
      std::uint64_t work = UINT64_MAX, const field::BudgetDemand &l = limits()) const {
    return StructuralAdmissionPlan::build(*assembly,b,field::PreparedCapacity(l,field::NumericalProfile::trial()),d,work);
  }
  AdmissionPlanResult build(const std::shared_ptr<ReservationDomain> &d) const { return build(bindings,d); }
  void restamp() {
    bindings.source = {assembly->owner(),assembly->study(),assembly->generation(),assembly->shared_recipe().ref};
  }
};

void source_and_symbols(bool parts) {
  Example e(parts,true);
  auto d = domain();
  auto result = e.build(d);
  if (result.status != Status::Ok)
    std::cerr << "plan status=" << int(result.status) << " work=" << result.work.charged
              << " budget=" << field::budget_name(result.failure.budget) << '\n';
  check(result.status == Status::Ok && result.plan, "supported plan");
  const auto &p = *result.plan;
  const auto &commands = p.commands();
  const auto h = p.hierarchy();
  check(p.occurrences().size() == (parts ? 24 : 2), "only selected exposure is emitted");
  check(p.commands().size() > 1, "bootstrap is independent of ordinary Batch=1");
  check(p.source().generation == e.assembly->generation(), "exact logical generation retained");
  check(p.source().study.str().get_allocator().domain == d &&
        p.source().recipe.revision.str().get_allocator().domain == d,
        "source stamps retain selected-domain ownership");
  for (std::size_t i=0; i<p.occurrences().size(); ++i) {
    const auto &o = p.occurrences()[i];
    if (i) check(p.occurrences()[i-1].occurrence < o.occurrence,
                 "numeric occurrence entry/path order");
    check(o.command == int(i) && commands[i].operation == field::UpdateOp::DefineParticle &&
              commands[i].exact_mass == (parts ? 2U : 24U), "exact denominator and command index");
    check(p.intrinsics()[i].command == i && p.intrinsics()[i].order == i &&
              p.intrinsics()[i].mass == 1.25, "physical mass is not the exact denominator");
    check(p.intrinsics()[i].radius == (parts ? 0.5+std::ldexp(1.0,-40) : 1+std::ldexp(1.0,-40)),
          "original double geometry survives before any f32 staging");
    check(p.source().study.str().get_allocator().domain == d &&
          o.occurrence.path.get_allocator().domain == d &&
          o.definition.object.address.get_allocator().domain == d &&
          o.definition.revision.str().get_allocator().domain == d, "nested identities use the selected domain");
  }
  if (parts) check(p.occurrences()[2].occurrence.path[0] == 2 &&
                   p.occurrences()[10].occurrence.path[0] == 10,
                   "path 2 precedes path 10 despite lexical spelling");
  check(p.occurrences().front().occurrence.entry == 2 &&
        p.occurrences().back().occurrence.entry == 10, "entry 2 precedes entry 10");
  std::size_t edges = 0, contacts = 0;
  for (const auto &c : commands) {
    check(!c.first.handle.context && !c.second.handle.context, "no guessed native handle");
    if (c.operation == field::UpdateOp::InsertParticipation) {
      const auto &body = commands[c.first.command];
      check(c.position == edges++ && c.second.command >= int(p.occurrences().size()) &&
            commands[c.second.command].operation == field::UpdateOp::DefineField &&
            c.exact_count && *c.exact_count <= *body.exact_mass &&
            c.share == float(double(*c.exact_count)/double(*body.exact_mass)),
            "ordered symbolic source with derived share");
    }
    if (c.operation == field::UpdateOp::InsertContact) {
      check(c.position == contacts++ && c.first.command < int(p.occurrences().size()) &&
            c.second.command < int(p.occurrences().size()), "explicit source contacts retain symbolic bodies");
    }
  }
  check(contacts == (parts ? 2U : 1U), "all explicit contacts retained");
  check(edges == p.occurrences().size()*(parts ? 6 : 16), "diamond membership is deduplicated per occurrence");
  check(h.contribution_count == edges && h.node_count == p.fields().size(), "complete field/cohort inventory");
  for (std::size_t n=0; n<h.node_count; ++n) {
    const auto &node = h.nodes[n];
    check(node.field_command == p.fields()[n].command && !node.field.context &&
          node.comparison == e.bindings.comparisons[n].comparison, "explicit comparison/field binding");
    int previous = -1;
    for (std::size_t i=node.cohort_begin; i<node.cohort_begin+node.cohort_count; ++i) {
      const auto &v = h.contributions[i];
      const auto &c = commands[v.source_command];
      check(v.source_command > previous && c.second.command == node.field_command &&
            c.first.command == v.occurrence.command && !v.occurrence.handle.context &&
            !v.source.generation && !v.source.row && !v.source.ordinal,
            "cohorts reference precisely the emitted participation commands");
      previous = v.source_command;
    }
  }
  check(p.focus().size() == 1 && p.focus()[0].command == p.fields().back().command,
        "ordered focus names the same prospective field inventory");
  if (!parts) {
    const auto parent_command = p.fields()[1].command;
    std::uint64_t ordinal = 0;
    for (const auto &c : commands) if (c.operation == field::UpdateOp::InsertParticipation &&
        c.first.command == 0 && c.second.command == parent_command)
      check(c.ordinal == ordinal++ && c.exact_count == 2, "repeated parent ordinals retain exact counts");
    check(ordinal == 12, "every repeated ordered parent retained");
  }
  check(d->result().live_bytes > 0, "plan owns admitted backing");
  result.plan.reset();
  check(!d->result().live_bytes, "successful plan teardown returns every selected request");
}

void refusals() {
  Example e;
  const auto reject = [&](auto change, Status expected = Status::InvalidInput) {
    auto b = e.bindings; change(b); auto d = domain();
    const auto r = e.build(b,d);
    check(r.status == expected && !r.plan && !d->result().live_bytes,
          "invalid binding refuses without a partial plan or retained allocation");
  };
  reject([](auto &b) { b.source.study = warm::new_uuid(); });
  reject([](auto &b) { b.bodies[0].occurrence.study = warm::new_uuid(); });
  reject([](auto &b) { b.source.owner = warm::new_uuid(); });
  reject([](auto &b) { b.source.generation = warm::new_uuid(); });
  reject([](auto &b) { b.source.recipe.revision = warm::new_uuid(); });
  reject([](auto &b) { b.bodies[0].definition.revision = warm::new_uuid(); });
  reject([](auto &b) { b.bodies[0].occurrence.source.path = {0,0}; }); // Inactive alternative.
  reject([](auto &b) { b.bodies[0] = b.bodies[1]; });
  reject([](auto &b) { b.bodies.pop_back(); });
  reject([](auto &b) { b.bodies[0].address = b.bodies[1].address; });
  reject([](auto &b) { b.bodies[0].kind = 0; });
  reject([](auto &b) { b.bodies[0].geometry.complete = false; });
  reject([](auto &b) { b.bodies[0].geometry.revision = 8; });
  reject([](auto &b) { b.bodies[0].mass = std::numeric_limits<double>::infinity(); },Status::InvalidNumerics);
  reject([](auto &b) { b.bodies[0].mass = std::numeric_limits<double>::denorm_min(); },Status::InvalidNumerics);
  reject([](auto &b) { b.layout[0].node.layout = nullptr; });
  reject([](auto &b) { b.layout[0].node.role = ScaleRole::PrimaryStep; });
  reject([](auto &b) { b.layout[0].units = 64; },Status::Unsupported);
  reject([](auto &b) { b.frame_anchor[0] = 1; },Status::Unsupported);
  reject([](auto &b) { b.fields.pop_back(); });
  reject([](auto &b) { b.fields[0].address = b.fields[1].address; });
  reject([](auto &b) { b.fields[0].field.kind = GroupKind::Membership; });
  {
    auto b = e.bindings;
    b.fields[0].address = int(limits()[Budget::Fields]);
    auto d = domain(); auto denied = e.build(b,d);
    check(denied.status == Status::CapacityExhausted && denied.failure.budget == Budget::Fields &&
          !denied.plan && !d->result().live_bytes,
          "field declaration addresses obey native Fields extent rather than Potential");
  }
  reject([](auto &b) { b.comparisons.pop_back(); });
  reject([](auto &b) { b.comparisons[1].comparison = b.comparisons[0].comparison; });
  reject([](auto &b) { b.comparisons[1].children = {1}; });
  reject([](auto &b) { b.comparisons[1].children = {0,0}; });
  reject([](auto &b) { b.comparisons[1].children.clear(); });
  reject([](auto &b) { b.comparisons[1].retained_fraction_bits = 53; },Status::Unsupported);
  reject([](auto &b) { b.focus.clear(); });
  reject([](auto &b) { b.focus[0] = b.fields.front().field; });
  auto empty = e.assembly->focus({});
  auto b = e.bindings; b.source.generation = empty.generation(); b.focus.clear();
  auto d = domain();
  auto r = StructuralAdmissionPlan::build(empty,b,field::PreparedCapacity(limits(),field::NumericalProfile::trial()),d,UINT64_MAX);
  check(r.status == Status::InvalidInput && !r.plan && !d->result().live_bytes,
        "explicit empty override never falls back to recipe focus");
  auto focused = e.assembly->focus({{GroupKind::Membership,InputText(warm::key(e.right))},
                                  {GroupKind::Membership,InputText(warm::key(e.left))}});
  b = e.bindings; b.source.generation = focused.generation();
  b.focus = {{GroupKind::Membership,e.right},{GroupKind::Membership,e.left}};
  r = StructuralAdmissionPlan::build(focused,b,field::PreparedCapacity(limits(),field::NumericalProfile::trial()),d,UINT64_MAX);
  check(r.status == Status::Ok && r.plan->focus()[0].command != r.plan->focus()[1].command,
        "explicit nonempty focus override preserves requested order");
  r.plan.reset();
  std::reverse(b.focus.begin(),b.focus.end());
  r = StructuralAdmissionPlan::build(focused,b,field::PreparedCapacity(limits(),field::NumericalProfile::trial()),d,UINT64_MAX);
  check(r.status == Status::InvalidInput && !r.plan && !d->result().live_bytes,
        "focus permutation cannot silently change the owner's requested order");
  // Fractional leaves must sum to the parent's exact coverage for each body.
  Example fractional(true);
  b = fractional.bindings; b.comparisons[2].children.pop_back();
  r = fractional.build(b,d);
  check(r.status == Status::InvalidInput && !r.plan && !d->result().live_bytes,
        "incomplete fractional child coverage refuses before a usable plan");
  Example scaled(true);
  b = scaled.bindings; b.layout[1].units = 1;
  r = scaled.build(b,d);
  check(r.status == Status::Unsupported && !r.plan, "PrimaryStep requires 64 units");
  b = scaled.bindings; b.layout[1].node.layout = b.layout[0].node.layout;
  r = scaled.build(b,d);
  check(r.status == Status::InvalidInput && !r.plan && !d->result().live_bytes,
        "layout aliases cannot impersonate a real scale child");
  auto foreign = std::make_unique<taichi::lang::SNode>(1,taichi::lang::SNodeType::dense);
  b = scaled.bindings; b.layout[1].node.layout = foreign.get();
  r = scaled.build(b,d);
  check(r.status == Status::InvalidInput && !r.plan && !d->result().live_bytes,
        "a nonnull foreign node cannot impersonate native parentage");
  auto missing = ActiveAssembly::assemble(e.assembly->shared_recipe(),warm::new_uuid(),true);
  r = StructuralAdmissionPlan::build(missing,e.bindings,
      field::PreparedCapacity(limits(),field::NumericalProfile::trial()),d,UINT64_MAX);
  check(r.status == Status::Unsupported && !r.plan && !d->result().live_bytes,
        "unavailable private inputs cannot cross structural preparation");
}

void mixed_scale() {
  Example e(true);
  auto recipe = e.assembly->shared_recipe();
  recipe.selections[0].exposure = warm::Exposure::Aggregate;
  recipe.contacts.clear();
  e.assembly = std::make_unique<ActiveAssembly>(ActiveAssembly::assemble(recipe,warm::new_uuid(),false));
  e.restamp();
  e.bindings.bodies.clear(); e.bindings.fields.clear(); e.bindings.comparisons.clear();
  const auto projection = project_letters(recipe);
  int address = 40;
  for (const auto &o : projection.occurrences()) if (o.active) {
    auto key = o.key; key.study = e.assembly->study();
    const bool aggregate = o.key.source.path.empty();
    e.bindings.bodies.push_back({std::move(key),o.definition,o.node,3,address++,2.5,
                               {aggregate ? 900U : 901U,7,true}});
  }
  const auto byte = e.fixture.byte('A');
  const auto &tool = e.fixture.base.tools[e.fixture.tools.at(byte.object)];
  const auto add = [&](GroupKind kind, const warm::ObjectKey &key, std::vector<int> children) {
    const FieldKey field{kind,key}; const int i = int(e.bindings.fields.size());
    e.bindings.fields.push_back({field,5,i});
    e.bindings.comparisons.push_back({field,std::uint64_t(100+i),i,8,std::move(children)});
  };
  add(GroupKind::ParentValue,tool.parents[0].target.object,{});
  add(GroupKind::ParentValue,tool.parents[1].target.object,{});
  add(GroupKind::SiblingDefinition,byte.object,{0,1});
  add(GroupKind::SiblingDefinition,e.letters,{});
  add(GroupKind::ParentValue,byte.object,{3});
  add(GroupKind::Membership,e.left,{2,4});
  add(GroupKind::Membership,e.right,{5});
  add(GroupKind::Membership,e.group,{6});
  auto d = domain(); auto r = e.build(d);
  check(r.status == Status::Ok && r.plan->occurrences().size() == 13,
        "mixed selected exposure admits one explicit comparison tree");
  const auto &aggregate = r.plan->intrinsics().back();
  check(r.plan->occurrences().back().units == 64 && aggregate.radius ==
        (1+std::ldexp(1.0,-40))*64 && aggregate.mass == 2.5,
        "PrimaryStep converts original geometry without rewriting physical mass");
  check(r.plan->hierarchy().nodes[7].retained_fraction_bits == 8,
        "explicit per-comparison retained precision is preserved");
  auto incompatible = e.bindings;
  incompatible.comparisons[5].children = {2,3}; // Orphans field 4.
  auto rejected = e.build(incompatible,d);
  check(rejected.status == Status::InvalidInput && !rejected.plan,
        "all prospective fields require one explicit complete tree");
}

void bounded_setup() {
  Example e;
  auto full = domain();
  auto reference = e.build(full);
  check(reference.status == Status::Ok, "unrestricted work witness");
  const auto required = reference.work.charged;
  const auto bytes = full->result().peak_bytes, allocation_work = full->result().charged_work;
  reference.plan.reset();
  for (auto allowance : {UINT64_C(0),required-1,required}) {
    auto d = domain(); auto r = e.build(e.bindings,d,allowance);
    check((allowance == required ? r.status == Status::Ok : r.status == Status::WorkExhausted) &&
          r.work.charged <= allowance, "exact work bound and one-short refusal");
    if (allowance < required) check(!r.plan && !d->result().live_bytes &&
          r.work.next_required > allowance, "work refusal unwinds without a partial plan");
  }
  std::uint64_t allowance = 0;
  while (allowance < required) {
    auto d = domain(); auto r = e.build(e.bindings,d,allowance);
    check(r.status == Status::WorkExhausted && !r.plan && !d->result().live_bytes &&
          r.work.next_required > allowance && r.work.next_required <= required,
          "each preparation work boundary refuses and fully unwinds");
    allowance = r.work.next_required;
  }
  // Visit every actual allocator refusal boundary, including nested keys and
  // scratch. Each failed attempt gets a fresh domain; retries use the same source.
  for (bool byte_limit : {true,false}) {
    std::uint64_t allowance = 0, ceiling = byte_limit ? bytes : allocation_work;
    while (allowance < ceiling) {
      auto d = domain({byte_limit ? allowance : UINT64_MAX,byte_limit ? UINT64_MAX : allowance});
      auto r = e.build(d);
      const auto next = byte_limit ? d->result().required_bytes : d->result().required_work;
      if (r.status != Status::AllocationExhausted || r.plan || d->result().live_bytes ||
          next <= allowance || next > ceiling)
        std::cerr << "refusal mode=" << (byte_limit ? "bytes" : "work")
                  << " allowance=" << allowance << " next=" << next << " ceiling=" << ceiling
                  << " status=" << int(r.status) << " live=" << d->result().live_bytes << '\n';
      check(r.status == Status::AllocationExhausted && !r.plan && !d->result().live_bytes &&
            next > allowance && next <= ceiling, "every request refusal fully unwinds");
      allowance = next;
    }
    auto d = domain({byte_limit ? ceiling : UINT64_MAX,byte_limit ? UINT64_MAX : ceiling});
    auto r = e.build(d);
    check(r.status == Status::Ok, "exact selected request bound admits retry");
    r.plan.reset(); check(!d->result().live_bytes, "exact-bound teardown");
  }
  const auto h = e.build(full);
  auto exact = limits();
  exact[Budget::Particles] = h.plan->occurrences().size();
  exact[Budget::Fields] = h.plan->fields().size();
  exact[Budget::BootstrapCommands] = h.plan->commands().size();
  exact[Budget::Focus] = h.plan->focus().size();
  const auto hierarchy = h.plan->hierarchy();
  exact[Budget::CascadeNodes] = hierarchy.node_count;
  exact[Budget::CascadeLinks] = hierarchy.child_count;
  exact[Budget::CascadeReferences] = hierarchy.contribution_count;
  exact[Budget::Participation] = hierarchy.contribution_count;
  exact[Budget::CascadeCoverage] = hierarchy.node_count*h.plan->occurrences().size();
  exact[Budget::CascadeLevels] = hierarchy.node_count;
  exact[Budget::Contacts] = 1;
  exact[Budget::Potential] = 42;
  for (const auto budget : {Budget::Particles,Budget::Fields,Budget::BootstrapCommands,Budget::Focus,
       Budget::CascadeNodes,Budget::CascadeLinks,Budget::CascadeReferences,Budget::Participation,
       Budget::CascadeCoverage,Budget::CascadeLevels,Budget::Contacts,Budget::Potential}) {
    auto d = domain(); auto r = e.build(e.bindings,d,required,exact);
    check(r.status == Status::Ok, "exact logical capacities pass");
    r.plan.reset();
    auto short_limit = exact; --short_limit[budget];
    r = e.build(e.bindings,d,required,short_limit);
    check(r.status == Status::CapacityExhausted && r.failure.budget == budget &&
          !r.plan && !d->result().live_bytes, "named one-short logical capacity refuses and unwinds");
  }
}

void foreign_membership_header() {
  for (const bool parts:{false,true}) {
    Example e(parts);
    auto &catalogue=assembly::testing::ActiveAssemblyAccess::memberships(*e.assembly);
    const auto original=catalogue.study();
    const auto numeric=assembly::testing::MembershipFieldsAccess::rows(catalogue).front().occurrence;
    assembly::testing::MembershipFieldsAccess::study(catalogue,warm::new_uuid());
    auto d=domain();
    auto refused=e.build(d);
    check(refused.status==Status::InvalidInput && !refused.plan && !d->result().live_bytes &&
              assembly::testing::MembershipFieldsAccess::rows(catalogue).front().occurrence==numeric,
          "foreign membership header refuses identical numeric plan rows and unwinds");
    assembly::testing::MembershipFieldsAccess::study(catalogue,original);
    auto retried=e.build(d);
    check(retried.status==Status::Ok,"healthy plan retries after foreign catalogue refusal");
  }
}

const allocation_lifetime::Entry *tracked(const void *pointer) {
  if (!pointer) return nullptr;
  auto *p = const_cast<void *>(pointer);
  for (std::size_t n=0, i=allocation_lifetime::bucket(p); n<65536; ++n, i=(i+1)%65536) {
    const auto &entry = allocation_lifetime::table[i];
    if (!entry.used) return nullptr;
    if (entry.pointer == p) return &entry;
  }
  return nullptr;
}
// Measure the actual changed producer with the same fixtures as #355. The
// source-pinned old binaries/evidence retain the complete before measurement.
std::size_t study_identity_cost(bool parts) {
  auto input = std::make_unique<Example>(parts,true);
  const auto stamps = input->bindings.source;
  auto expected = input->bindings.bodies;
  std::sort(expected.begin(),expected.end(),[](const auto &a, const auto &b) {
    return a.occurrence.source < b.occurrence.source;
  });
  auto d = domain();
  allocation_lifetime::begin();
  allocation_observation::calls = allocation_observation::bytes = 0;
  allocation_observation::enabled = true;
  auto result = input->build(d);
  allocation_observation::enabled = false;
  const auto calls = allocation_observation::calls.load(), requested = allocation_observation::bytes.load();
  if (!result.plan) {
    allocation_lifetime::end();
    throw std::runtime_error("study identity measurement requires a supported plan");
  }
  const auto &plan = *result.plan;
  const auto count = plan.occurrences().size(), capacity = plan.occurrences().capacity();
  const auto retained = allocation_lifetime::counts;
  const auto domain_retained = d->result().live_bytes;
  const auto *header = tracked(plan.source().study.str().data());
  const auto *outer = tracked(plan.occurrences().data());
  const auto header_bytes = header ? header->bytes : 0;
  const auto outer_bytes = outer ? outer->bytes : 0;
  bool exact = count == expected.size();
  bool attributed = header && outer && plan.source().study.str().get_allocator().domain == d;
  // The existing header is the comparison accessor. Keep numeric entry/path and
  // full ToolRefs independent; no temporary owning OccurrenceKey is constructed.
  allocation_observation::calls = allocation_observation::bytes = 0;
  allocation_observation::enabled = true;
  for (std::size_t i=0; i<count && i<expected.size(); ++i) {
    const auto &row = plan.occurrences()[i];
    exact &= plan.source().study == expected[i].occurrence.study &&
             row.occurrence == expected[i].occurrence.source && row.definition == expected[i].definition;
  }
  allocation_observation::enabled = false;
  const auto borrow_calls = allocation_observation::calls.load(), borrow_bytes = allocation_observation::bytes.load();
  const auto *header_address = &plan.source().study;
  const auto *row_address = plan.occurrences().data();
  auto owned = std::move(result.plan);
  input.reset(); // Caller assembly, bindings, source stamps and real SNodes die.
  const bool independent = &owned->source().study == header_address && owned->occurrences().data() == row_address &&
      owned->source().study == stamps.study && owned->source().owner == stamps.owner &&
      owned->source().generation == stamps.generation &&
      owned->source().recipe.selection == stamps.recipe.selection && owned->source().recipe.revision == stamps.recipe.revision &&
      allocation_lifetime::counts.live == retained.live;
  for (std::size_t i=0; i<count && i<expected.size(); ++i)
    exact &= owned->source().study == expected[i].occurrence.study &&
             owned->occurrences()[i].occurrence == expected[i].occurrence.source &&
             owned->occurrences()[i].definition == expected[i].definition;
  owned.reset();
  const auto released = allocation_lifetime::end();
  check(exact && independent,"existing owned header preserves full study identity after source death and plan-handle move");
  check(count == (parts ? 24U : 2U) && attributed && header_bytes,
        "one domain-owned plan header supplies study identity to every numeric row");
  check(outer_bytes == capacity*sizeof(AdmissionOccurrenceCommand) && capacity >= count,
        "numeric-only rows occupy the actual reserved occurrence vector");
  check(!borrow_calls && !borrow_bytes,"existing source header and numeric occurrence comparison allocate nothing");
  check(retained.acquired == calls && !released.live && !released.blocks &&
        released.acquired == released.released && !d->result().live_bytes,
        "actual build scratch and retained plan allocations release completely");
  std::cout << "study_identity_cost,parts=" << parts << ",rows=" << count << ",capacity=" << capacity
            << ",construction_calls=" << calls << ",construction_requested=" << requested
            << ",retained_requested=" << retained.live << ",peak_requested=" << retained.peak
            << ",domain_retained=" << domain_retained << ",header_requested=" << header_bytes
            << ",uuid_object_bytes=" << sizeof(warm::Uuid) << ",row_object_bytes=" << sizeof(AdmissionOccurrenceCommand)
            << ",outer_capacity_bytes=" << outer_bytes << ",borrow_calls=" << borrow_calls
            << ",after_destroy_requested=" << released.live << ",preparation_work=" << result.work.charged << '\n';
  return calls;
}

void allocation_failures(bool parts, std::size_t requests) {
  Example input(parts,true);
  for (std::size_t fail=1; fail<=requests; ++fail) {
    auto d = domain();
    allocation_lifetime::begin(fail);
    auto refused = input.build(d);
    const bool unavailable = refused.status == Status::AllocationExhausted && !refused.plan;
    refused.plan.reset();
    const auto released = allocation_lifetime::end();
    check(unavailable && released.attempts == fail && !released.live && !released.blocks &&
          released.acquired == released.released && !d->result().live_bytes,
          "every actual C++ allocation failure unwinds the numeric-only plan");
  }
}

void study_scoping() {
  for (bool parts : {false,true}) {
    Example a(parts,true), b(parts,true);
    auto da = domain(), db = domain();
    auto first = a.build(da), second = b.build(db);
    check(first.status == Status::Ok && second.status == Status::Ok &&
          first.plan->source().study != second.plan->source().study,"separate plans own distinct study scopes");
    for (std::size_t i=0; i<first.plan->occurrences().size(); ++i) {
      const auto &id = first.plan->occurrences()[i].occurrence;
      check(id == second.plan->occurrences()[i].occurrence,"equal numeric paths remain reusable across studies");
      const OccurrenceKey full{first.plan->source().study,id}; // Expected owner prepared outside counters.
      allocation_observation::calls = allocation_observation::bytes = 0;
      allocation_observation::enabled = true;
      const auto expected = occurrence_text(full);
      allocation_observation::enabled = false;
      const auto full_calls = allocation_observation::calls.load(), full_bytes = allocation_observation::bytes.load();
      allocation_observation::calls = allocation_observation::bytes = 0;
      allocation_observation::enabled = true;
      const auto actual = occurrence_text(first.plan->source().study,id);
      allocation_observation::enabled = false;
      check(actual == expected && allocation_observation::calls == full_calls && allocation_observation::bytes == full_bytes,
            "split rendering preserves full text without allocating a temporary owning key");
      check(actual != occurrence_text(second.plan->source().study,id),"rendered identities retain separate-study distinction");
    }
  }
}

void independent_lifetimes() {
  std::weak_ptr<ReservationDomain> weak;
  auto result = [&] {
    Example e(true);
    auto d = domain(); weak = d;
    auto r = e.build(d);
    check(r.status == Status::Ok, "source lifetime setup");
    return r;
  }(); // ActiveAssembly, caller vectors, real SNodes and caller domain handle die.
  check(!weak.expired() && result.plan->occurrences().size() == 24 &&
        result.plan->occurrences()[10].occurrence.path[0] == 10 &&
        result.plan->hierarchy().contributions[0].source_command >= 0,
        "owned plan values survive all borrowed source/layout inputs");
  result.plan.reset();
  check(weak.expired(), "plan releases the final selected domain lifetime");
  const Point centre{};
  const std::vector<Sphere> geometry{{{3,4,0},2}};
  check(enclosing_radius(centre,geometry) == 7 &&
        intrinsic_enclosing_radius(centre,geometry) == 7,
        "legacy and structural preparation reuse the same complete enclosure");
}
}
int main() {
  try {
    source_and_symbols(false); source_and_symbols(true);
    refusals(); mixed_scale(); bounded_setup(); foreign_membership_header(); independent_lifetimes();
    allocation_failures(false,study_identity_cost(false));
    allocation_failures(true,study_identity_cost(true));
    study_scoping();
    std::cout << "structural_admission_plan,checks=" << checks
              << ",native_claims=0,retained_execution=0 PASS\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n'; return 1;
  }
}
