#include "assembly/projection_access.h"
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
warm::OccurrenceId copy_occurrence(const warm::OccurrenceId &key,
    const std::shared_ptr<ReservationDomain> &domain, PreparationWork &w) {
  w.copy_key(key);
  warm::OccurrencePath path(key.path.begin(),key.path.end(),
      SharedReservationAllocator<std::uint32_t>(domain));
  return {key.entry,std::move(path)};
}
AdmissionSource empty_source(const std::shared_ptr<ReservationDomain> &domain) {
  // GCC 11 does not fully unwind allocating nested aggregate temporaries.
  // Construct independent owners first; only nonallocating moves form the row.
  auto owner = warm::Uuid(domain);
  auto study = warm::Uuid(domain);
  auto generation = warm::Uuid(domain);
  auto selection = warm::Uuid(domain);
  auto revision = warm::Uuid(domain);
  warm::BundleRef recipe{std::move(selection),std::move(revision)};
  return {std::move(owner),std::move(study),std::move(generation),std::move(recipe)};
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
    : domain_(domain), source_(empty_source(domain)),
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
  need(w.equal(assembly.memberships_.study_,p.study()));
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
  const bool separate_memberships = capacity.separate_membership_roots();
  if (separate_memberships)
    need(!b.character_rollup && !b.endpoint_rollup && b.focus.size()==1 &&
         b.focus.front().kind==GroupKind::ParentValue);
  ReservedVector<std::size_t> ordered{ReservationAllocator<std::size_t>(domain_.get())};
  reserve(ordered,b.bodies.size(),w);
  std::uint64_t edges = 0;
  for (std::size_t index=0;index<p.occurrence_count();++index) {
    const auto &o=ProjectionAccess::row(p,index);
    w.charge(1);
    if (!o.active) continue;
    need(ordered.size() < b.bodies.size());
    ordered.push_back(index);
    const auto *definition=ProjectionAccess::resolve(p,index,&w); need(definition);
    edges = sum(edges,sum(1,definition->parents.size()));
    if (separate_memberships) {
      const auto *letter = o.byte ? ProjectionAccess::resolve(p,o.byte->owner_index,&w) : definition;
      need(letter && letter->mass==2 && letter->parents.size()==1);
      const auto &byte = letter->parents.front();
      need(byte.ordinal==0 && byte.contribution==2 && w.equal(byte.target.object,b.focus.front().object));
      if (o.byte) {
        need(o.byte->character_ordinal==0 && o.byte->byte_ordinal==0 &&
             w.equal(o.byte->character,byte.target) && w.equal(definition->ref,byte.target));
        edges = sum(edges,1);
      }
    }
  }
  need(ordered.size() == b.bodies.size());
  const warm::Tool *endpoint_rollup = nullptr;
  if (b.endpoint_rollup) {
    w.charge(1);
    need(!b.character_rollup);
    const auto owner_index = ProjectionAccess::find(p,b.endpoint_rollup->owner,w);
    need(owner_index<p.occurrence_count());
    const auto &owner = ProjectionAccess::row(p,owner_index);
    endpoint_rollup = ProjectionAccess::resolve(p,owner_index,&w);
    need(endpoint_rollup && !owner.active && !owner.byte &&
         w.equal(endpoint_rollup->ref,b.endpoint_rollup->endpoint));
    const auto &parents = endpoint_rollup->parents;
    need(!parents.empty() && parents.size() <= ordered.size());
    ReservedVector<std::uint64_t> counts{ReservationAllocator<std::uint64_t>(domain_.get())};
    reserve(counts,parents.size(),w);
    counts.resize(parents.size());
    for (const auto index : ordered) {
      w.charge(1);
      const auto &part = ProjectionAccess::row(p,index);
      const auto *definition = ProjectionAccess::resolve(p,index,&w);
      // The sealed projection binds each byte to the full parent pin and both
      // source ordinals. Check the active cohort's exact counts per parent.
      need(definition && part.byte && part.byte->owner_index == owner_index &&
           part.byte->character_ordinal < counts.size());
      auto &count = counts[part.byte->character_ordinal];
      count = sum(count,*definition->mass);
    }
    std::uint64_t total = 0;
    for (std::size_t i=0; i<counts.size(); ++i) {
      w.charge(1);
      need(counts[i] && counts[i] == parents[i].contribution);
      total = sum(total,counts[i]);
    }
    need(total == *endpoint_rollup->mass);
    edges = sum(edges,sum(ordered.size(),ordered.size()));
  }
  if (b.character_rollup) {
    w.charge(1);
    const auto &rollup = *b.character_rollup;
    const auto owner_index = ProjectionAccess::find(p,rollup.owner,w);
    need(owner_index < p.occurrence_count());
    const auto &owner = ProjectionAccess::row(p,owner_index);
    const auto *endpoint = ProjectionAccess::resolve(p,owner_index,&w);
    need(endpoint && !owner.active && !owner.byte && owner.complete_character &&
         w.equal(*owner.complete_character,rollup.character) && endpoint->parents.size() == 1);
    const auto &character = tool(rollup.character);
    need(character.mass && *character.mass == *endpoint->mass &&
         character.parents.size() >= 2 && character.parents.size() == ordered.size());
    // The existing projection checks each byte's complete source relationship.
    // Match the whole active cohort to the ordered scalar construction here.
    std::uint64_t total = 0;
    for (std::size_t i=0; i<ordered.size(); ++i) {
      w.charge(1);
      const auto &part = ProjectionAccess::row(p,ordered[i]);
      const auto *definition = ProjectionAccess::resolve(p,ordered[i],&w);
      const auto &edge = character.parents[i];
      need(definition && part.byte && part.byte->owner_index == owner_index &&
           part.byte->character_ordinal == 0 && part.byte->byte_ordinal == i &&
           edge.ordinal == i && w.equal(definition->ref,edge.target) &&
           *definition->mass == edge.contribution);
      total = sum(total,edge.contribution);
    }
    need(total == *character.mass);
    edges = sum(edges,ordered.size());
  }
  edges = sum(edges,assembly.memberships_.contributions_.size());
  cap(Budget::Participation,edges); cap(Budget::Contacts,recipe.contacts.size());
  const auto commands = sum(sum(ordered.size(),b.fields.size()),sum(edges,recipe.contacts.size()));
  cap(Budget::BootstrapCommands,commands);
  need(commands <= INT_MAX,AdmissionPlanStatus::Overflow);
  std::sort(ordered.begin(),ordered.end(),[&](std::size_t a, std::size_t c) {
    const auto &left=ProjectionAccess::row(p,a).source,&right=ProjectionAccess::row(p,c).source;
    w.copy_key(left); w.copy_key(right);
    return left < right;
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
    const auto &o = ProjectionAccess::row(p,ordered[i]);
    const auto *resolved=ProjectionAccess::resolve(p,ordered[i],&w); need(resolved);
    const auto &pin=resolved->ref;
    need(w.equal(p.study(),source_.study));
    const AdmissionBody *input = nullptr;
    for (const auto &v : b.bodies) {
      w.charge(1);
      if (ProjectionAccess::matches(p,ordered[i],v.occurrence,w)) { need(!input); input = &v; }
    }
    need(input && w.equal(input->definition,pin) && input->node == o.node &&
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
    const auto &definition = tool(pin);
    need(definition.mass && *definition.mass);
    const warm::Geometry *geometry = nullptr;
    for (const auto &g : recipe.geometry) {
      w.charge(1);
      if (w.equal(g.object,pin.object)) { need(!geometry); geometry = &g; }
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
    auto key = copy_occurrence(o.source,domain_,w);
    auto ref = copy_ref(pin,domain_,w);
    w.charge(sizeof(AdmissionOccurrenceCommand));
    occurrences_.push_back({std::move(key),std::move(ref),command,o.node,units[o.node]});
  }
  for (std::size_t i=0; i<b.fields.size(); ++i) {
    w.charge(1); const auto &f = b.fields[i];
    need(f.kind > 0 && f.kind <= 255 && f.address >= 0 &&
         f.field.kind >= GroupKind::SiblingDefinition && f.field.kind <= GroupKind::Membership);
    cap(Budget::Fields,std::uint64_t(f.address)+1);
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
    const auto &o = ProjectionAccess::row(p,ordered[i]);
    const auto *resolved=ProjectionAccess::resolve(p,ordered[i],&w); need(resolved);
    const auto &t = *resolved;
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
    if (separate_memberships && o.byte)
      edge(GroupKind::ParentValue,b.focus.front().object,*t.mass,o.byte->character_ordinal);
    if (b.character_rollup) {
      w.charge(1);
      edge(GroupKind::ParentValue,b.character_rollup->character.object,*t.mass,o.byte->byte_ordinal);
    }
    if (endpoint_rollup) {
      w.charge(1);
      edge(GroupKind::ParentValue,o.byte->character.object,*t.mass,o.byte->character_ordinal);
      edge(GroupKind::SiblingDefinition,endpoint_rollup->ref.object,*t.mass,o.byte->character_ordinal);
    }
    for (const auto &m : assembly.memberships_.contributions_) {
      w.charge(1);
      w.charge(1);
      need(m.target_index<assembly.memberships_.targets_.size());
      const auto &target=assembly.memberships_.targets_[m.target_index];
      if (w.equal(m.occurrence,o.source)) {
        need(m.count == *t.mass);
        // Retain the prior object-association validation charge. The pin now
        // owns the sole object, while explicit kind and full revision still validate.
        w.charge(1);
        w.elements(2,target.object.scope.str().size());
        w.elements(target.object.address.size(),2*sizeof(codec::AddressElement));
        need(m.kind == GroupKind::Membership);
        tool(target); edge(m.kind,target.object,m.count,0);
      }
    }
  }
  need(position == edges);
  const auto occurrence_index = [&](const warm::OccurrenceId &key) {
    for (std::size_t i=0; i<occurrences_.size(); ++i) {
      // Preserve the prior full-key validation charge for this scan even
      // though the common study is now borrowed from the sealed header.
      w.charge(1); w.charge(source_.study.str().size()); w.copy_key(occurrences_[i].occurrence);
      w.elements(key.path.size(),sizeof(std::uint32_t));
      if (occurrences_[i].occurrence == key) return int(i);
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
  const auto rollup_children = [&](std::size_t node, GroupKind kind,
                                   const warm::ObjectKey *character) {
    const auto &children = b.comparisons[node].children;
    std::size_t next = 0;
    for (std::size_t i=0; i<ordered.size(); ++i) {
      w.charge(1);
      const auto &pin = ProjectionAccess::row(p,ordered[i]).byte->character.object;
      if (character && !w.equal(*character,pin)) continue;
      const auto &object = kind == GroupKind::ParentValue ? pin : occurrences_[i].definition.object;
      bool seen = false;
      for (std::size_t j=0; j<next; ++j) {
        w.charge(1);
        if (w.equal(b.comparisons[children[j]].field.object,object)) {
          seen = true;
          break;
        }
      }
      if (seen) continue;
      need(next < children.size());
      const auto child = children[next++];
      need(child >= 0 && std::size_t(child) < node);
      const auto &field = b.comparisons[child].field;
      need(field.kind == kind && w.equal(field.object,object));
    }
    need(next && next == children.size());
  };
  if (b.character_rollup) {
    w.charge(1);
    const auto &root = b.comparisons.back();
    need(root.field.kind == GroupKind::ParentValue &&
         w.equal(root.field.object,b.character_rollup->character.object));
    rollup_children(b.comparisons.size()-1,GroupKind::SiblingDefinition,nullptr);
  }
  if (endpoint_rollup) {
    w.charge(1);
    const auto &root = b.comparisons.back();
    need(root.field.kind==GroupKind::SiblingDefinition &&
         w.equal(root.field.object,endpoint_rollup->ref.object));
    rollup_children(b.comparisons.size()-1,GroupKind::ParentValue,nullptr);
    for (const int child : root.children) {
      w.charge(1);
      rollup_children(child,GroupKind::SiblingDefinition,&b.comparisons[child].field.object);
    }
  }
  std::uint64_t links = 0;
  struct Coverage { std::size_t node, body; field::CascadeCoverage value; };
  ReservedVector<Coverage> coverage{ReservationAllocator<Coverage>(domain_.get())};
  // At most one coverage entry per participation; this bounds temporary space.
  reserve(coverage,edges,w);
  ReservedVector<int> parents{ReservationAllocator<int>(domain_.get())},
      ranks{ReservationAllocator<int>(domain_.get())},
      child_uses{ReservationAllocator<int>(domain_.get())};
  reserve(parents,b.comparisons.size(),w); reserve(ranks,b.comparisons.size(),w);
  reserve(child_uses,b.comparisons.size(),w);
  w.elements(b.comparisons.size(),3*sizeof(int));
  parents.assign(b.comparisons.size(),-1); ranks.assign(b.comparisons.size(),0);
  child_uses.assign(b.comparisons.size(),0);
  const auto coverage_at = [&](std::size_t node, std::size_t body) -> const field::CascadeCoverage * {
    for (const auto &v : coverage) { w.charge(1); if (v.node == node && v.body == body) return &v.value; }
    return nullptr;
  };
  for (std::size_t i=0; i<b.comparisons.size(); ++i) {
    w.charge(1);
    for (int child : b.comparisons[i].children) {
      w.charge(1);
      need(child >= 0 && std::size_t(child) < i && parents[child] != int(i));
      parents[child] = int(i);
      ++child_uses[child];
      links = sum(links,1);
    }
  }
  cap(Budget::CascadeLinks,links);
  const auto edge_begin = occurrences_.size()+fields_.size();
  struct ScopedLink { int branch, child, command; bool confirmed{}; };
  ReservedVector<ScopedLink> scoped{ReservationAllocator<ScopedLink>(domain_.get())};
  const bool editable_scopes = capacity.editable_scopes();
  for (std::size_t child=0; child<b.comparisons.size(); ++child) {
    w.charge(1);
    if (child_uses[child] < 2) continue;
    need(editable_scopes ||
         (b.character_rollup && capacity.hierarchy_mode()==field::PreparedHierarchyMode::ScopedNibbleBootstrap) ||
         (endpoint_rollup && capacity.hierarchy_mode()==field::PreparedHierarchyMode::ScopedEndpointBootstrap));
    const auto &publisher = b.comparisons[child];
    const bool split_memberships = separate_memberships && !publisher.children.empty();
    need(publisher.field.kind==GroupKind::ParentValue && publisher.retained_fraction_bits==52 &&
         (publisher.children.empty() || (split_memberships && w.equal(publisher.field.object,b.focus.front().object))));
    if (scoped.capacity() == 0)
      reserve(scoped,std::min(edges,limits[Budget::CascadeLinks]),w);
    const auto field = fields_[field_index(publisher.field.kind,publisher.field.object)].command;
    const auto begin = scoped.size();
    for (std::size_t j=edge_begin; j<edge_begin+edges; ++j) {
      w.charge(1);
      const auto &edge = commands_[j];
      if (edge.second.command != field) continue;
      const auto body = edge.first.command;
      const auto &part = ProjectionAccess::row(p,ordered[body]);
      const auto *definition = ProjectionAccess::resolve(p,ordered[body],&w);
      need(definition && (part.byte || separate_memberships));
      if (split_memberships)
        need(part.byte && w.equal(definition->ref.object,publisher.field.object) && *edge.exact_count==*definition->mass);
      else {
        need(edge.ordinal < definition->parents.size());
        const auto &parent = definition->parents[edge.ordinal];
        need(parent.ordinal == edge.ordinal && parent.contribution == *edge.exact_count &&
             w.equal(parent.target.object,publisher.field.object));
      }
      int branch = -1;
      for (std::size_t i=child+1; i<b.comparisons.size(); ++i) {
        w.charge(1);
        const auto &candidate = b.comparisons[i];
        if (split_memberships ? candidate.field.kind!=GroupKind::Membership
            : candidate.field.kind!=GroupKind::SiblingDefinition ||
                  !w.equal(candidate.field.object,definition->ref.object)) continue;
        for (int link : candidate.children) {
          w.charge(1);
          if (link != int(child)) continue;
          if (split_memberships) {
            const auto member = fields_[field_index(GroupKind::Membership,candidate.field.object)].command;
            bool included = false;
            for (std::size_t row=edge_begin; row<edge_begin+edges; ++row) {
              w.charge(1);
              const auto &participation = commands_[row];
              if (participation.first.command==body && participation.second.command==member) {
                included = true;
                break;
              }
            }
            if (!included) continue;
          }
          need(branch < 0 && candidate.retained_fraction_bits == 52);
          branch = int(i);
        }
      }
      need(branch >= 0);
      cap(Budget::CascadeLinks,sum(scoped.size(),1));
      w.charge(sizeof(ScopedLink));
      scoped.push_back({branch,int(child),int(j)});
    }
    const auto count = scoped.size()-begin;
    need(count >= std::size_t(child_uses[child]));
    links = sum(links,count-child_uses[child]);
    scoped_nibble_rows_ = true;
  }
  need(editable_scopes || scoped_nibble_rows_ == capacity.scoped_rows());
  // A caller may additionally assert every derived selection by full byte key
  // and ordinal. The immutable ledger above determines the selections.
  if (!b.nibble_scopes.empty()) {
    need(b.nibble_scopes.size() == scoped.size());
    for (const auto &assertion : b.nibble_scopes) {
      w.charge(1);
      bool found = false;
      for (auto &link : scoped) {
        w.charge(1);
        const auto &edge = commands_[link.command];
        if (b.comparisons[link.branch].comparison != assertion.branch_comparison ||
            edge.ordinal != assertion.ordinal ||
            !ProjectionAccess::matches(p,ordered[edge.first.command],assertion.byte,w)) continue;
        need(!link.confirmed);
        link.confirmed = found = true;
      }
      need(found);
    }
  }
  w.elements(parents.size(),sizeof(int));
  std::fill(parents.begin(),parents.end(),-1);
  cap(Budget::CascadeLinks,links); cap(Budget::CascadeReferences,edges);
  reserve(children_,links,w);
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
      if (child_uses[child]>1) {
        w.charge(1);
        const auto begin = children_.size();
        for (const auto &scope : scoped) {
          w.charge(1);
          if (scope.branch!=int(i) || scope.child!=child) continue;
          int link = child;
          for (std::size_t j=nodes_[child].cohort_begin;
               j<nodes_[child].cohort_begin+nodes_[child].cohort_count; ++j) {
            w.charge(1);
            if (contributions_[j].source_command==scope.command) link = -1-int(j);
          }
          need(link<0 && children_.size()<links);
          w.charge(sizeof(int)); children_.push_back(link);
        }
        need(children_.size()>begin);
      } else {
        parents[child] = int(i);
        need(children_.size()<links);
        w.charge(sizeof(int)); children_.push_back(child);
      }
      ranks[i] = std::max(ranks[i],ranks[child]+1);
    }
    node.child_count = children_.size()-node.child_begin;
    if (node.child_count) for (std::size_t body=0; body<occurrences_.size(); ++body) {
      const auto *parent = coverage_at(i,body);
      std::uint64_t accounted = 0;
      for (std::size_t j=node.child_begin; j<children_.size(); ++j) {
        w.charge(1);
        const int child = children_[j];
        if (child<0) {
          const auto &edge = commands_[contributions_[-1-child].source_command];
          if (edge.first.command==int(body)) {
            const field::CascadeCoverage part{{},*edge.exact_count,*commands_[body].exact_mass};
            need(parent && field::detail::account_child(*parent,part,accounted));
          }
          continue;
        }
        const auto *part = coverage_at(child,body);
        if (part) need(parent && field::detail::account_child(*parent,*part,accounted));
      }
      need(accounted == (parent ? parent->count : 0));
    }
    cap(Budget::CascadeLevels,std::uint64_t(ranks[i])+1);
    w.charge(sizeof(node)); nodes_.push_back(node);
  }
  need(children_.size()==links);
  for (std::size_t i=0; i<parents.size(); ++i) {
    w.charge(1);
    if (separate_memberships) {
      if (parents[i]<0 && child_uses[i]<2) need(b.comparisons[i].field.kind==GroupKind::Membership);
    } else need((parents[i] < 0) == (i+1 == parents.size() || child_uses[i]>1));
  }
  for (std::size_t body=0; body<occurrences_.size(); ++body) {
    bool leaf = false;
    std::size_t roots = 0;
    for (std::size_t i=0; i<nodes_.size(); ++i) {
      w.charge(1); if (!nodes_[i].child_count && coverage_at(i,body)) leaf = true;
      if (separate_memberships && parents[i]<0 && child_uses[i]<2) {
        const auto *cohort = coverage_at(i,body);
        if (cohort) {
          need(cohort->count==cohort->denominator);
          ++roots;
        }
      }
    }
    need(leaf && (!separate_memberships || roots==1));
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
