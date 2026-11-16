#include "assembly/structural_admission_plan.h"
#include "assembly/intrinsic_geometry.h"
#include "field/cascade_source.h"
#include "taichi/ir/snode.h"
#include <algorithm>
#include <climits>
#include <cmath>
#include <new>

namespace assembly::detail {
namespace {
struct Refusal {
  AdmissionPlanStatus status;
  field::BudgetFailure failure{};
};
void need(bool ok, AdmissionPlanStatus status = AdmissionPlanStatus::InvalidInput) {
  if (!ok) throw Refusal{status};
}
std::uint64_t sum(std::uint64_t a, std::uint64_t b) {
  need(b <= UINT64_MAX-a, AdmissionPlanStatus::Overflow);
  return a+b;
}
bool same_field(const FieldKey &a, const FieldKey &b, PreparationWork &w) {
  w.charge(1);
  return a.kind == b.kind && w.equal(a.object,b.object);
}
warm::ObjectKey copy_object(const warm::ObjectKey &key,
    const std::shared_ptr<ReservationDomain> &domain, PreparationWork &w) {
  w.copy_key(key);
  auto scope = warm::Uuid(key.scope,domain);
  codec::Address address(key.address.begin(),key.address.end(),
      SharedReservationAllocator<codec::AddressElement>(domain));
  return {std::move(scope),std::move(address)};
}
warm::ToolRef copy_ref(const warm::ToolRef &ref,
    const std::shared_ptr<ReservationDomain> &domain, PreparationWork &w) {
  auto object = copy_object(ref.object,domain,w);
  w.copy_key(ref.revision);
  auto revision = warm::Uuid(ref.revision,domain);
  return {std::move(object),std::move(revision)};
}
OccurrenceKey copy_occurrence(const OccurrenceKey &key,
    const std::shared_ptr<ReservationDomain> &domain, PreparationWork &w) {
  w.copy_key(key);
  auto study = warm::Uuid(key.study,domain);
  warm::OccurrencePath path(key.source.path.begin(),key.source.path.end(),
      SharedReservationAllocator<std::uint32_t>(domain));
  return {std::move(study),{key.source.entry,std::move(path)}};
}
// Compare the existing scope/address encoding without an unaccounted temporary
// std::string from warm::key. Both spelling and field kind must match.
bool same_focus(const FocalField &request, const FieldKey &key, PreparationWork &w) {
  w.charge(1); w.elements(request.value.size(),1); w.copy_key(key.object);
  if (request.kind != key.kind || key.object.address.size() != 5) return false;
  const auto &text = request.value;
  const auto &scope = key.object.scope.str();
  if (text.size() != scope.size()+15 ||
      !std::equal(scope.begin(),scope.end(),text.begin()) || text[scope.size()] != '/')
    return false;
  std::size_t at = scope.size()+1;
  for (std::size_t i=0; i<5; ++i) {
    const auto &e = key.object.address[i];
    if (e.partial || !codec::is_valid_element(e)) return false;
    if (i && text[at++] != codec::kDelimiter) return false;
    if (text[at++] != codec::kAlphabet[e.first] ||
        text[at++] != codec::kAlphabet[e.second]) return false;
  }
  return true;
}
template<class T> void reserve(ReservedVector<T> &v, std::uint64_t count, PreparationWork &w) {
  need(count <= v.max_size(),AdmissionPlanStatus::Overflow);
  w.elements(count,sizeof(T));
  v.reserve(count);
}
}

StructuralAdmissionPlan::StructuralAdmissionPlan(const std::shared_ptr<ReservationDomain> &domain)
    : domain_(domain), source_{warm::Uuid(domain),warm::Uuid(domain),warm::Uuid(domain),
                              {warm::Uuid(domain),warm::Uuid(domain)}},
      commands_(ReservationAllocator<field::UpdateCommand>(domain.get())),
      intrinsics_(ReservationAllocator<field::PlacementIntrinsic>(domain.get())),
      occurrences_(ReservationAllocator<AdmissionOccurrenceCommand>(domain.get())),
      fields_(ReservationAllocator<AdmissionFieldCommand>(domain.get())),
      focus_(ReservationAllocator<field::UpdateRef>(domain.get())),
      nodes_(ReservationAllocator<field::LiveRefreshNode>(domain.get())),
      children_(ReservationAllocator<int>(domain.get())),
      contributions_(ReservationAllocator<field::LiveRefreshContribution>(domain.get())) {}

AdmissionPlanResult StructuralAdmissionPlan::build(const ActiveAssembly &assembly,
    const AdmissionBindings &bindings, const field::PreparedCapacity &capacity,
    const std::shared_ptr<ReservationDomain> &domain, std::uint64_t allowance) {
  AdmissionPlanResult result;
  PreparationWork w(allowance);
  try {
    w.charge(1); // Fixed header/control setup; sized work is charged separately.
    need(bool(domain));
    auto plan = reserve_unique<StructuralAdmissionPlan>(domain,[&](auto *p) {
      ::new (static_cast<void *>(p)) StructuralAdmissionPlan(domain);
    });
    plan->prepare(assembly,bindings,capacity,w);
    result.plan = std::move(plan);
    result.status = AdmissionPlanStatus::Ok;
  } catch (const Refusal &r) {
    result.status = r.status; result.failure = r.failure;
  } catch (const PreparationStop &stop) {
    result.status = stop.result.status == PreparationStatus::Overflow
        ? AdmissionPlanStatus::Overflow : AdmissionPlanStatus::WorkExhausted;
    result.work = stop.result;
  } catch (const ReservationStop &) {
    result.status = domain->result().status == ReservationStatus::Overflow
        ? AdmissionPlanStatus::Overflow : AdmissionPlanStatus::AllocationExhausted;
  } catch (const std::bad_alloc &) {
    result.status = AdmissionPlanStatus::AllocationExhausted;
  } catch (const std::invalid_argument &) {
    result.status = AdmissionPlanStatus::InvalidNumerics;
  }
  if (result.work.status == PreparationStatus::Ok) result.work = w.result();
  if (domain) result.reservation = domain->result();
  return result;
}

void StructuralAdmissionPlan::prepare(const ActiveAssembly &assembly,
    const AdmissionBindings &b, const field::PreparedCapacity &capacity, PreparationWork &w) {
  using field::Budget;
  using field::UpdateCommand;
  using field::UpdateOp;
  using field::UpdateRef;
  const auto &p = assembly.projection_;
  const auto &recipe = p.recipe();
  const auto &limits = capacity.limits();
  const auto cap = [&](Budget kind, std::uint64_t required) {
    w.charge(1);
    if (required > limits[kind])
      throw Refusal{AdmissionPlanStatus::CapacityExhausted,{kind,required,limits[kind]}};
  };
  need(capacity.profile().wide() && recipe.recipe_version == 2 &&
       assembly.inputs_available() && !assembly.local_,AdmissionPlanStatus::Unsupported);
  need(w.equal(b.source.owner,assembly.owner()) && w.equal(b.source.study,assembly.study()) &&
       w.equal(b.source.generation,assembly.generation()) &&
       w.equal(b.source.recipe.selection,recipe.ref.selection) &&
       w.equal(b.source.recipe.revision,recipe.ref.revision));
  // No implicit coordinate rebase or restart hints in the initial plan subset.
  w.elements(b.frame_anchor.size(),1);
  for (double coordinate : b.frame_anchor)
    need(coordinate == 0,AdmissionPlanStatus::Unsupported);
  cap(Budget::Particles,b.bodies.size()); cap(Budget::Fields,b.fields.size());
  cap(Budget::CascadeNodes,b.comparisons.size()); cap(Budget::Focus,b.focus.size());
  need(!b.bodies.empty() && !b.fields.empty() && !b.focus.empty());
  need(b.layout.size() == recipe.nodes.size() && !b.layout.empty());
  ReservedVector<double> units{ReservationAllocator<double>(domain_.get())};
  reserve(units,b.layout.size(),w);
  for (std::size_t i=0; i<b.layout.size(); ++i) {
    w.charge(1);
    const auto &v = b.layout[i];
    const auto &node = v.node;
    need(node.layout && node.parent == recipe.nodes[i].parent &&
         int(node.role) == int(recipe.nodes[i].role));
    for (std::size_t j=0; j<i; ++j) { w.charge(1); need(node.layout != b.layout[j].node.layout); }
    double expected = 1;
    if (!i) need(node.parent == -1 && node.role == ScaleRole::Root &&
                 node.layout->type == taichi::lang::SNodeType::root);
    else {
      need(node.parent >= 0 && std::size_t(node.parent) < i);
      bool child = false;
      for (const auto &c : b.layout[node.parent].node.layout->ch) {
        w.charge(1); child |= c.get() == node.layout;
      }
      need(child);
      expected = units[node.parent];
      switch (node.role) {
        case ScaleRole::PrimaryStep: expected *= 64; break;
        case ScaleRole::SecondaryCompression:
        case ScaleRole::NamingEquivalent: break;
        default: need(false);
      }
    }
    need(field::NumericalProfile::supported_real(expected) && expected > 0 &&
         v.units == expected,AdmissionPlanStatus::Unsupported);
    units.push_back(expected);
  }
  const auto tool = [&](const warm::ToolRef &ref) -> const warm::Tool & {
    const warm::Tool *found = nullptr;
    for (const auto &t : recipe.tools) {
      w.charge(1);
      if (w.equal(t.ref.object,ref.object)) { need(!found && w.equal(t.ref,ref)); found = &t; }
    }
    need(found); return *found;
  };
  ReservedVector<const ProjectedOccurrence *> ordered{
      ReservationAllocator<const ProjectedOccurrence *>(domain_.get())};
  reserve(ordered,b.bodies.size(),w);
  std::uint64_t edges = 0;
  for (const auto &o : p.occurrences()) {
    w.charge(1);
    if (!o.active) continue;
    need(ordered.size() < b.bodies.size());
    ordered.push_back(&o);
    edges = sum(edges,sum(1,tool(o.definition).parents.size()));
  }
  need(ordered.size() == b.bodies.size());
  edges = sum(edges,assembly.memberships_.contributions().size());
  cap(Budget::Participation,edges); cap(Budget::Contacts,recipe.contacts.size());
  const auto commands = sum(sum(ordered.size(),b.fields.size()),sum(edges,recipe.contacts.size()));
  cap(Budget::BootstrapCommands,commands);
  need(commands <= INT_MAX,AdmissionPlanStatus::Overflow);
  std::sort(ordered.begin(),ordered.end(),[&](const auto *a, const auto *c) {
    w.copy_key(a->key); w.copy_key(c->key);
    return a->key.source < c->key.source;
  });
  reserve(commands_,commands,w); reserve(intrinsics_,ordered.size(),w);
  reserve(occurrences_,ordered.size(),w); reserve(fields_,b.fields.size(),w);
  reserve(focus_,b.focus.size(),w); reserve(nodes_,b.comparisons.size(),w);
  reserve(contributions_,edges,w);
  for (const auto *id : {&b.source.owner,&b.source.study,&b.source.generation,
                        &b.source.recipe.selection,&b.source.recipe.revision}) w.copy_key(*id);
  source_ = b.source; // All destinations already have domain-attached allocators.
  const auto append = [&](const UpdateCommand &command) {
    w.charge(sizeof(command)); need(commands_.size() < commands);
    const int index = int(commands_.size()); commands_.push_back(command); return index;
  };
  for (std::size_t i=0; i<ordered.size(); ++i) {
    const auto &o = *ordered[i];
    const AdmissionBody *input = nullptr;
    for (const auto &v : b.bodies) {
      w.charge(1);
      if (w.equal(v.occurrence,o.key)) { need(!input); input = &v; }
    }
    need(input && w.equal(input->definition,o.definition) && input->node == o.node &&
         o.node >= 0 && std::size_t(o.node) < units.size());
    need(input->kind > 0 && input->kind <= 255 && input->address >= 0);
    cap(Budget::Potential,std::uint64_t(input->address)+1);
    need(input->geometry.complete && input->geometry.definition && input->geometry.revision);
    need(field::NumericalProfile::supported_real(input->mass) && input->mass > 0,
         AdmissionPlanStatus::InvalidNumerics);
    for (std::size_t j=0; j<i; ++j) {
      w.charge(1); need(commands_[j].address != input->address);
      const auto &g = intrinsics_[j].geometry;
      const bool same = w.equal(occurrences_[j].definition,input->definition);
      need(same ? (g.definition == input->geometry.definition && g.revision == input->geometry.revision)
                : (g.definition != input->geometry.definition || g.revision != input->geometry.revision));
    }
    const auto &definition = tool(o.definition);
    need(definition.mass && *definition.mass);
    const warm::Geometry *geometry = nullptr;
    for (const auto &g : recipe.geometry) {
      w.charge(1);
      if (w.equal(g.object,o.definition.object)) { need(!geometry); geometry = &g; }
    }
    need(geometry);
    w.elements(geometry->spheres.size(),sizeof(warm::Sphere));
    const double radius = intrinsic_enclosing_radius(geometry->centroid,geometry->spheres) * units[o.node];
    need(field::NumericalProfile::supported_real(radius) && radius >= 0,
         AdmissionPlanStatus::InvalidNumerics);
    UpdateCommand c; c.operation = UpdateOp::DefineParticle;
    c.kind = input->kind; c.address = input->address; c.exact_mass = *definition.mass;
    const int command = append(c);
    w.charge(sizeof(field::PlacementIntrinsic));
    intrinsics_.push_back({std::uint64_t(command),i,input->mass,radius,input->geometry});
    auto key = copy_occurrence(o.key,domain_,w);
    auto ref = copy_ref(o.definition,domain_,w);
    w.charge(sizeof(AdmissionOccurrenceCommand));
    occurrences_.push_back({std::move(key),std::move(ref),command,o.node,units[o.node]});
  }
  for (std::size_t i=0; i<b.fields.size(); ++i) {
    w.charge(1); const auto &f = b.fields[i];
    need(f.kind > 0 && f.kind <= 255 && f.address >= 0 &&
         f.field.kind >= GroupKind::SiblingDefinition && f.field.kind <= GroupKind::Membership);
    cap(Budget::Potential,std::uint64_t(f.address)+1);
    for (std::size_t j=0; j<i; ++j) {
      w.charge(1); need(b.fields[j].address != f.address && !same_field(b.fields[j].field,f.field,w));
    }
    UpdateCommand c; c.operation = UpdateOp::DefineField; c.kind = f.kind; c.address = f.address;
    const int command = append(c);
    auto object = copy_object(f.field.object,domain_,w);
    w.charge(sizeof(AdmissionFieldCommand));
    fields_.push_back({{f.field.kind,std::move(object)},command});
  }
  const auto field_index = [&](GroupKind kind, const warm::ObjectKey &object) {
    for (std::size_t i=0; i<fields_.size(); ++i) {
      w.charge(1);
      if (fields_[i].field.kind == kind && w.equal(fields_[i].field.object,object)) return i;
    }
    throw Refusal{AdmissionPlanStatus::InvalidInput};
  };
  std::uint64_t position = 0;
  for (std::size_t i=0; i<ordered.size(); ++i) {
    const auto &o = *ordered[i]; const auto &t = tool(o.definition);
    const auto edge = [&](GroupKind kind, const warm::ObjectKey &object,
                          std::uint64_t count, std::uint64_t ordinal) {
      const auto f = field_index(kind,object);
      need(count && count <= *t.mass);
      UpdateCommand c; c.operation = UpdateOp::InsertParticipation;
      c.first = UpdateRef::pending(int(i)); c.second = UpdateRef::pending(fields_[f].command);
      c.position = position++; c.ordinal = ordinal; c.exact_count = count;
      c.share = float(double(count)/double(*t.mass));
      append(c);
    };
    edge(GroupKind::SiblingDefinition,t.ref.object,*t.mass,0);
    std::uint64_t parent_mass = 0;
    for (const auto &parent : t.parents) {
      w.charge(1); parent_mass = sum(parent_mass,parent.contribution);
      edge(GroupKind::ParentValue,parent.target.object,parent.contribution,parent.ordinal);
    }
    need(t.parents.empty() || parent_mass == *t.mass);
    for (const auto &m : assembly.memberships_.contributions()) {
      w.charge(1);
      if (w.equal(m.occurrence,o.key)) {
        need(m.count == *t.mass && w.equal(m.field.object,m.target.object));
        tool(m.target); edge(GroupKind::Membership,m.field.object,m.count,0);
      }
    }
  }
  need(position == edges);
  const auto occurrence_index = [&](const warm::OccurrenceId &key) {
    for (std::size_t i=0; i<occurrences_.size(); ++i) {
      w.charge(1); w.copy_key(occurrences_[i].occurrence);
      w.elements(key.path.size(),sizeof(std::uint32_t));
      if (occurrences_[i].occurrence.source == key) return int(i);
    }
    throw Refusal{AdmissionPlanStatus::InvalidInput};
  };
  const auto contact_begin = commands_.size();
  for (std::size_t i=0; i<recipe.contacts.size(); ++i) {
    w.charge(1); const auto &contact = recipe.contacts[i];
    const int a = occurrence_index(contact.a), c = occurrence_index(contact.b);
    need(a != c);
    for (std::size_t j=0; j<i; ++j) {
      w.charge(1); const auto &old = commands_[contact_begin+j];
      need(!((old.first.command == a && old.second.command == c) ||
             (old.first.command == c && old.second.command == a)));
    }
    UpdateCommand command; command.operation = UpdateOp::InsertContact;
    command.first = UpdateRef::pending(a); command.second = UpdateRef::pending(c);
    command.position = i; append(command);
  }
  need(commands_.size() == commands);
  // Every declared field owns all and only its emitted source rows. The caller
  // chooses the tree, while cohort order comes from this one canonical ledger.
  need(b.comparisons.size() == fields_.size());
  std::uint64_t links = 0;
  for (const auto &node : b.comparisons) { w.charge(1); links = sum(links,node.children.size()); }
  cap(Budget::CascadeLinks,links); cap(Budget::CascadeReferences,edges);
  reserve(children_,links,w);
  struct Coverage { std::size_t node, body; field::CascadeCoverage value; };
  ReservedVector<Coverage> coverage{ReservationAllocator<Coverage>(domain_.get())};
  // At most one coverage entry per participation; this bounds temporary space.
  reserve(coverage,edges,w);
  ReservedVector<int> parents{ReservationAllocator<int>(domain_.get())}, ranks{ReservationAllocator<int>(domain_.get())};
  reserve(parents,b.comparisons.size(),w); reserve(ranks,b.comparisons.size(),w);
  w.elements(b.comparisons.size(),2*sizeof(int));
  parents.assign(b.comparisons.size(),-1); ranks.assign(b.comparisons.size(),0);
  const auto coverage_at = [&](std::size_t node, std::size_t body) -> const field::CascadeCoverage * {
    for (const auto &v : coverage) { w.charge(1); if (v.node == node && v.body == body) return &v.value; }
    return nullptr;
  };
  const auto edge_begin = occurrences_.size()+fields_.size();
  for (std::size_t i=0; i<b.comparisons.size(); ++i) {
    w.charge(1); const auto &input = b.comparisons[i];
    need(input.comparison && input.level >= 0);
    need(input.retained_fraction_bits >= 0 && input.retained_fraction_bits <= 52,
         AdmissionPlanStatus::Unsupported);
    const auto f = field_index(input.field.kind,input.field.object);
    for (std::size_t j=0; j<i; ++j) {
      w.charge(1); need(input.comparison != b.comparisons[j].comparison &&
                       !same_field(input.field,b.comparisons[j].field,w));
    }
    field::LiveRefreshNode node;
    node.field_command = fields_[f].command; node.comparison = input.comparison;
    node.level = input.level; node.retained_fraction_bits = input.retained_fraction_bits;
    node.cohort_begin = contributions_.size(); node.child_begin = children_.size();
    for (std::size_t row=0; row<edges; ++row) {
      w.charge(1); const auto &e = commands_[edge_begin+row];
      if (e.second.command != node.field_command) continue;
      w.charge(sizeof(field::LiveRefreshContribution));
      contributions_.push_back({e.first,{},int(edge_begin+row)});
    }
    node.cohort_count = contributions_.size()-node.cohort_begin;
    need(node.cohort_count);
    for (std::size_t body=0; body<occurrences_.size(); ++body) {
      field::CascadeCoverage value{{},0,*commands_[body].exact_mass};
      for (std::size_t row=0; row<edges; ++row) {
        w.charge(1); const auto &e = commands_[edge_begin+row];
        if (e.first.command == int(body) && e.second.command == node.field_command)
          need(field::detail::add_coverage(value,*e.exact_count,value.denominator));
      }
      if (value.count) {
        cap(Budget::CascadeCoverage,sum(coverage.size(),1));
        w.charge(sizeof(Coverage)); coverage.push_back({i,body,value});
      }
    }
    for (int child : input.children) {
      w.charge(1);
      need(child >= 0 && std::size_t(child) < i && parents[child] < 0 &&
           b.comparisons[child].level <= input.level);
      parents[child] = int(i); ranks[i] = std::max(ranks[i],ranks[child]+1);
      w.charge(sizeof(int)); children_.push_back(child);
    }
    node.child_count = children_.size()-node.child_begin;
    if (node.child_count) for (std::size_t body=0; body<occurrences_.size(); ++body) {
      const auto *parent = coverage_at(i,body);
      std::uint64_t accounted = 0;
      for (int child : input.children) {
        w.charge(1); const auto *part = coverage_at(child,body);
        if (part) need(parent && field::detail::account_child(*parent,*part,accounted));
      }
      need(accounted == (parent ? parent->count : 0));
    }
    cap(Budget::CascadeLevels,std::uint64_t(ranks[i])+1);
    w.charge(sizeof(node)); nodes_.push_back(node);
  }
  for (std::size_t i=0; i<parents.size(); ++i) {
    w.charge(1); need((parents[i] < 0) == (i+1 == parents.size()));
  }
  for (std::size_t body=0; body<occurrences_.size(); ++body) {
    bool leaf = false;
    for (std::size_t i=0; i<nodes_.size(); ++i) {
      w.charge(1); if (!nodes_[i].child_count && coverage_at(i,body)) leaf = true;
    }
    need(leaf);
  }
  // Inherited recipe focus is typed; explicit ActiveAssembly override retains
  // its existing lossless text representation, including an explicit empty list.
  const auto requested = assembly.focus_override_ ? assembly.focal_fields_.size() : recipe.focal_fields.size();
  need(requested == b.focus.size());
  for (std::size_t i=0; i<b.focus.size(); ++i) {
    w.charge(1); const auto &key = b.focus[i];
    if (assembly.focus_override_) need(same_focus(assembly.focal_fields_[i],key,w));
    else need(int(recipe.focal_fields[i].kind) == int(key.kind) &&
              w.equal(recipe.focal_fields[i].object,key.object));
    const auto f = field_index(key.kind,key.object);
    for (std::size_t j=0; j<i; ++j) need(!same_field(key,b.focus[j],w));
    w.charge(sizeof(UpdateRef)); focus_.push_back(UpdateRef::pending(fields_[f].command));
  }
}
}
