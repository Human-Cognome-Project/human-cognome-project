#pragma once
#include "assembly/projection_access.h"
#include "assembly/exchange_owner.h"
#include "exchange_coordinator_access.h"
namespace assembly::testing {
struct ExchangeOwnerAccess {
  static auto resources(const detail::ExchangeOwner &o) { return o.storage(); }
  static auto reservations(const detail::ExchangeOwner &o) { return o.reservations_->result(); }
  static auto reservation_domain(const detail::ExchangeOwner &o) { return o.reservations_; }
  static auto bootstrap_tracker(const detail::ExchangeOwner &o) { return o.bootstrap_; }
  static const auto &recipe_owner(const LetterProjection &p) { return p.recipe_; }
  template<class Visit> static void projection_table_vectors(const LetterProjection &p, Visit visit, bool recipe = true) {
    const auto input=[&](const auto &value) {
      visit(value.nodes); visit(value.definitions); visit(value.occurrences);
      visit(value.selections); visit(value.contacts); visit(value.memberships);
      visit(value.focal_fields);
      for (const auto &d : value.definitions) { visit(d.components); visit(d.parents); }
      for (const auto &s : value.selections) visit(s.parts);
    };
    input(p.input_); input(p.structural_input_);
    visit(p.exposures_); visit(p.occurrences_); visit(p.parents_); visit(p.spare_parents_);
    if (!recipe) return;
    visit(p.recipe().tools); visit(p.recipe().geometry); visit(p.recipe().nodes);
    visit(p.recipe().occurrences); visit(p.recipe().selections); visit(p.recipe().contacts);
    visit(p.recipe().structural_occurrences); visit(p.recipe().focal_fields);
    for (const auto &tool : p.recipe().tools) { visit(tool.parents); visit(tool.member_of); }
    for (const auto &geometry : p.recipe().geometry) visit(geometry.spheres);
    for (const auto &selection : p.recipe().selections) visit(selection.parts);
  }
  template<class Visit> static void recipe_uuids(const warm::Bundle &b, Visit visit) {
    visit(b.ref.selection); visit(b.ref.revision); visit(b.owner); visit(b.creator);
    const auto reference=[&](const auto &r) { visit(r.object.scope); visit(r.revision); };
    for (const auto &t : b.tools) {
      reference(t.ref); visit(t.epoch); visit(t.author);
      for (const auto &p : t.parents) reference(p.target);
      for (const auto &m : t.member_of) reference(m);
    }
    for (const auto &g : b.geometry) visit(g.object.scope);
    for (const auto &o : b.occurrences) visit(o.definition.scope);
    for (const auto &o : b.structural_occurrences) visit(o.definition.scope);
    for (const auto &f : b.focal_fields) visit(f.object.scope);
  }
  template<class Visit> static void projection_texts(const LetterProjection &p, Visit visit, bool recipe = true) {
    for (const auto *input : {&p.input_.definitions,&p.structural_input_.definitions})
      for (const auto &d : *input) for (const auto &parent : d.parents) visit(parent.value);
    const auto relations=[&](const auto &input) {
      for (const auto &d : input.definitions) visit(d.id);
      for (const auto &o : input.occurrences) { visit(o.id); visit(o.definition); }
      for (const auto &s : input.selections) { visit(s.aggregate); for (const auto &part : s.parts) visit(part); }
      for (const auto &c : input.contacts) { visit(c.a); visit(c.b); }
      for (const auto &m : input.memberships) { visit(m.occurrence); visit(m.group); }
      for (const auto &f : input.focal_fields) visit(f.value);
    };
    relations(p.input_); relations(p.structural_input_);
    if (recipe) recipe_uuids(p.recipe(),[&](const auto &uuid) { visit(uuid.str()); });
    visit(p.study_.str());
    const auto reference=[&](const auto &ref) {
      visit(ref.object.scope.str()); visit(ref.revision.str());
    };
    for (const auto &o : p.occurrences_) {
      if (o.byte) { visit(o.byte->owner.study.str()); reference(o.byte->character); }
      if (o.complete_character) reference(*o.complete_character);
    }
    for (const auto *rows : {&p.parents_,&p.spare_parents_})
      for (const auto &row : *rows) {
        visit(row.occurrence.study.str()); visit(row.field.object.scope.str());
      }
    if (recipe) for (const auto &tool : p.recipe().tools) {
      visit(tool.recipe);
      if (tool.notation) visit(*tool.notation);
    }
  }
  template<class Visit> static void projection_backing(const LetterProjection &p, Visit visit, bool recipe = true) {
    projection_table_vectors(p,[&](const auto &table) {
      visit(std::make_pair(static_cast<const void *>(table.data()),table.capacity()*sizeof(table[0])));
    },recipe);
    projection_texts(p,[&](const auto &text) {
      visit(std::make_pair(static_cast<const void *>(text.data()),warm::recipe_text_backing(text)));
    },recipe);
  }
  template<class Visit> static void projection_paths(const LetterProjection &p, Visit visit, bool recipe = true) {
    if (recipe) for (const auto &o : p.recipe().occurrences) visit(o.id.path);
    if (recipe) for (const auto &o : p.recipe().structural_occurrences) visit(o.id.path);
    if (recipe) for (const auto &s : p.recipe().selections)
      for (const auto &id : s.parts) visit(id.path);
    if (recipe) for (const auto &c : p.recipe().contacts) { visit(c.a.path); visit(c.b.path); }
    for (const auto &o : p.occurrences_) {
      visit(o.source.path);
      if (o.byte) visit(o.byte->owner.source.path);
    }
    for (const auto *rows : {&p.parents_,&p.spare_parents_})
      for (const auto &row : *rows) visit(row.occurrence.source.path);
  }
  template<class Visit> static void projection_addresses(const LetterProjection &p, Visit visit, bool recipe = true) {
    if (recipe) recipe_addresses(p.recipe(),visit);
    for (const auto &o : p.occurrences_) {
      if (o.byte) visit(o.byte->character.object.address);
      if (o.complete_character) visit(o.complete_character->object.address);
    }
    for (const auto *rows : {&p.parents_,&p.spare_parents_})
      for (const auto &row : *rows) visit(row.field.object.address);
  }
  template<class Visit> static void recipe_addresses(const warm::Bundle &b, Visit visit) {
    for (const auto &t : b.tools) {
      visit(t.ref.object.address);
      for (const auto &parent : t.parents) visit(parent.target.object.address);
      for (const auto &ref : t.member_of) visit(ref.object.address);
    }
    for (const auto &g : b.geometry) visit(g.object.address);
    for (const auto &o : b.occurrences) visit(o.definition.address);
    for (const auto &o : b.structural_occurrences) visit(o.definition.address);
    for (const auto &f : b.focal_fields) visit(f.object.address);
  }
  // Recipe family only: no live exposure, occurrence publication or input maps.
  template<class Visit> static void recipe_backing(const warm::Bundle &b, Visit visit) {
    const auto table = [&](const auto &v) {
      visit(static_cast<const void *>(v.data()),v.capacity()*sizeof(v[0]),v.get_allocator().domain.get());
    };
    const auto text = [&](const auto &v) {
      visit(static_cast<const void *>(v.data()),warm::recipe_text_backing(v),v.get_allocator().domain.get());
    };
    table(b.tools); table(b.geometry); table(b.nodes); table(b.occurrences);
    table(b.selections); table(b.contacts); table(b.structural_occurrences); table(b.focal_fields);
    for (const auto &t : b.tools) { table(t.parents); table(t.member_of); text(t.recipe); if (t.notation) text(*t.notation); }
    for (const auto &g : b.geometry) table(g.spheres);
    for (const auto &s : b.selections) { table(s.parts); for (const auto &id : s.parts) table(id.path); }
    for (const auto &o : b.occurrences) table(o.id.path);
    for (const auto &o : b.structural_occurrences) table(o.id.path);
    for (const auto &c : b.contacts) { table(c.a.path); table(c.b.path); }
    recipe_addresses(b,table);
    recipe_uuids(b,[&](const auto &u) { text(u.str()); });
  }
  static bool recipe_equivalent(const warm::Bundle &a, const warm::Bundle &b) {
    const auto rows=[](const auto &x, const auto &y, auto equal) {
      return x.size()==y.size() && std::equal(x.begin(),x.end(),y.begin(),equal);
    };
    const auto point=[](const auto &x, const auto &y) { return x.x==y.x && x.y==y.y && x.z==y.z; };
    return a.ref.selection==b.ref.selection && a.ref.revision==b.ref.revision &&
        a.owner==b.owner && a.creator==b.creator && a.recipe_version==b.recipe_version &&
        rows(a.tools,b.tools,[&](const auto &x,const auto &y) {
          return x.ref==y.ref && x.epoch==y.epoch && x.recipe==y.recipe && x.notation==y.notation &&
              x.mass==y.mass && x.author==y.author && x.member_of==y.member_of &&
              rows(x.parents,y.parents,[](const auto &p,const auto &q) {
                return p.target==q.target && p.ordinal==q.ordinal && p.stored_mass==q.stored_mass &&
                    p.contribution==q.contribution;
              });
        }) &&
        rows(a.geometry,b.geometry,[&](const auto &x,const auto &y) {
          return x.object==y.object && point(x.centroid,y.centroid) &&
              rows(x.spheres,y.spheres,[&](const auto &p,const auto &q) {
                return point(p.centre,q.centre) && p.radius==q.radius;
              });
        }) &&
        rows(a.nodes,b.nodes,[](const auto &x,const auto &y) { return x.parent==y.parent && x.role==y.role; }) &&
        rows(a.occurrences,b.occurrences,[&](const auto &x,const auto &y) {
          return x.id==y.id && x.definition==y.definition && x.node==y.node &&
              point(x.position,y.position) && point(x.velocity,y.velocity);
        }) &&
        rows(a.selections,b.selections,[](const auto &x,const auto &y) {
          return x.entry==y.entry && x.exposure==y.exposure && x.parts==y.parts;
        }) &&
        rows(a.contacts,b.contacts,[](const auto &x,const auto &y) { return x.a==y.a && x.b==y.b; }) &&
        rows(a.structural_occurrences,b.structural_occurrences,[](const auto &x,const auto &y) {
          return x.id==y.id && x.definition==y.definition && x.node==y.node;
        }) &&
        rows(a.focal_fields,b.focal_fields,[](const auto &x,const auto &y) {
          return x.kind==y.kind && x.object==y.object;
        });
  }
  template<class Input> static bool input_equivalent(const Input &a, const Input &b) {
    const auto rows=[](const auto &x,const auto &y,auto same) {
      return x.size()==y.size() && std::equal(x.begin(),x.end(),y.begin(),same);
    };
    const auto point=[](const auto &x,const auto &y) { return x.x==y.x && x.y==y.y && x.z==y.z; };
    if constexpr (std::is_same_v<Input,StudyInput>) if (a.capacity!=b.capacity) return false;
    return rows(a.nodes,b.nodes,[](const auto &x,const auto &y) {
      return x.layout==y.layout && x.parent==y.parent && x.role==y.role;
    }) && rows(a.definitions,b.definitions,[&](const auto &x,const auto &y) {
      return x.id==y.id && x.mass==y.mass && point(x.centroid,y.centroid) &&
          rows(x.components,y.components,[&](const auto &p,const auto &q) {
            return point(p.centre,q.centre) && p.radius==q.radius;
          }) && rows(x.parents,y.parents,[](const auto &p,const auto &q) {
            return p.value==q.value && p.ordinal==q.ordinal && p.count==q.count;
          });
    }) && rows(a.occurrences,b.occurrences,[&](const auto &x,const auto &y) {
      bool same=x.id==y.id && x.definition==y.definition && x.study_node==y.study_node;
      if constexpr (std::is_same_v<Input,StudyInput>)
        same=same && x.slot==y.slot && point(x.position,y.position) && point(x.velocity,y.velocity);
      return same;
    }) && rows(a.selections,b.selections,[](const auto &x,const auto &y) {
      return x.aggregate==y.aggregate && x.parts==y.parts && x.exposure==y.exposure;
    }) && rows(a.contacts,b.contacts,[](const auto &x,const auto &y) { return x.a==y.a && x.b==y.b; }) &&
        rows(a.memberships,b.memberships,[](const auto &x,const auto &y) {
          return x.occurrence==y.occurrence && x.group==y.group;
        }) && rows(a.focal_fields,b.focal_fields,[](const auto &x,const auto &y) {
          return x.kind==y.kind && x.value==y.value;
        });
  }
  // Supplement real v1/v2 projection fixtures with retained tables that are
  // normally filled only by the later public/native handoff.
  static void populate_input_tables(LetterProjection &p) {
    const auto fill=[](auto &input) {
      if (!input.definitions.empty()) {
        auto &parents=input.definitions.front().parents;
        parents.push_back({InputText{},91,1});
        parents.push_back({InputText("short"),92,2});
        parents.push_back({InputText(137,'p'),93,3});
      }
      for (const auto &text : {InputText{},InputText("short"),InputText(139,'r')}) {
        input.definitions.push_back({text,1,{},{},{}});
        using Input = std::decay_t<decltype(input)>;
        if constexpr (std::is_same_v<Input,StudyInput>)
          input.occurrences.push_back({text,text,0,0,{},{}});
        else input.occurrences.push_back({text,text,0});
        input.selections.push_back({text,{text},Exposure::Aggregate});
        input.contacts.push_back({text,text});
        input.memberships.push_back({text,text});
        input.focal_fields.push_back({GroupKind::Membership,text});
      }
      input.nodes.push_back({nullptr,-1,ScaleRole::Root});
      input.memberships.push_back({"retained-occurrence","retained-group"});
      input.focal_fields.push_back({GroupKind::Membership,"retained-group"});
    };
    fill(p.input_); fill(p.structural_input_);
  }
  static bool projection_equivalent(const LetterProjection &a, const LetterProjection &b) {
    if (!input_equivalent(a.input_,b.input_) || !input_equivalent(a.structural_input_,b.structural_input_) ||
        !recipe_equivalent(a.recipe(),b.recipe()) || a.study_ != b.study_ || a.exposures_ != b.exposures_ ||
        a.occurrences_.size() != b.occurrences_.size()) return false;
    for (std::size_t i=0; i<a.occurrences_.size(); ++i) {
      const auto &x=a.occurrences_[i], &y=b.occurrences_[i];
      if (!(x.source == y.source) || x.definition_index != y.definition_index || x.source_index != y.source_index ||
          !(detail::ProjectionAccess::definition(a,i) == detail::ProjectionAccess::definition(b,i)) || x.slot != y.slot ||
          x.node != y.node || x.active != y.active || bool(x.byte) != bool(y.byte) ||
          bool(x.complete_character) != bool(y.complete_character)) return false;
      if (x.byte && (!(x.byte->owner == y.byte->owner) || !(x.byte->character == y.byte->character) ||
          x.byte->character_ordinal != y.byte->character_ordinal || x.byte->byte_ordinal != y.byte->byte_ordinal))
        return false;
      if (x.complete_character && !(*x.complete_character == *y.complete_character)) return false;
    }
    const auto same_parents=[](const auto &x, const auto &y) {
      if (x.size() != y.size()) return false;
      for (std::size_t i=0; i<x.size(); ++i)
        if (!(x[i].occurrence == y[i].occurrence) || x[i].field.kind != y[i].field.kind ||
            !(x[i].field.object == y[i].field.object) ||
            x[i].ordinal != y[i].ordinal || x[i].count != y[i].count) return false;
      return true;
    };
    return same_parents(a.parents_,b.parents_) && same_parents(a.spare_parents_,b.spare_parents_);
  }
  static auto projection_backing_bytes(const LetterProjection &p, bool recipe = true) {
    std::uint64_t bytes = 0;
    projection_backing(p,[&](const auto &entry) { bytes += entry.second; },recipe);
    return bytes;
  }
  template<class Visit> static void projection_domains(const LetterProjection &p, Visit visit) {
    projection_table_vectors(p,[&](const auto &table) { visit(table.get_allocator().domain.get()); });
    projection_texts(p,[&](const auto &text) { visit(text.get_allocator().domain.get()); });
  }
  static auto projection_copy(const LetterProjection &p, const std::shared_ptr<detail::ReservationDomain> &domain) {
    return LetterProjection(p,domain);
  }
  static void reserve_projection(LetterProjection &p) {
    detail::PreparationWork meter(UINT64_MAX); p.reserve_exposure(meter);
  }
  static auto projection_parent_sizes(const LetterProjection &p) {
    return std::make_pair(p.parents_.size(),p.spare_parents_.size());
  }
  static auto unique_objects(const detail::ExchangeOwner &o) {
    return std::array<std::pair<const void *,std::size_t>,5>{{
      {&o,sizeof(o)}, {o.projection_.get(),sizeof(*o.projection_)},
      {o.prospective_.get(),sizeof(*o.prospective_)}, {o.plan_.get(),sizeof(*o.plan_)},
      {ExchangeCoordinatorAccess::binding(*o.plan_),sizeof(detail::ExchangeBindings)}}};
  }
  template<class Visit> static void map_uuids(const detail::ExchangeOwner &o, Visit visit) {
    for (const auto &row : o.occurrences_.storage()) visit(row.occurrence.study);
    for (const auto &row : o.fields_.storage()) visit(row.field.object.scope);
    ExchangeCoordinatorAccess::map_uuids(*o.plan_,visit);
  }
  template<class Visit> static void map_paths(const detail::ExchangeOwner &o, Visit visit) {
    for (const auto &row : o.occurrences_.storage()) visit(row.occurrence.source.path);
    ExchangeCoordinatorAccess::map_paths(*o.plan_,visit);
  }
  template<class Visit> static void map_addresses(const detail::ExchangeOwner &o, Visit visit) {
    for (const auto &row : o.fields_.storage()) visit(row.field.object.address);
    ExchangeCoordinatorAccess::map_addresses(*o.plan_,visit);
  }
  static auto map_outer_bytes(const detail::ExchangeOwner &o) {
    return o.occurrences_.storage().capacity()*sizeof(detail::OccurrenceBinding)+
        o.fields_.storage().capacity()*sizeof(detail::ExchangeFieldBinding)+
        ExchangeCoordinatorAccess::map_outer_bytes(*o.plan_);
  }
  static auto &inactive_path(detail::ExchangeOwner &o) {
    return o.occurrences_[o.occurrences_.storage().size()-1].occurrence.source.path;
  }
  static auto &field_command_buffer(detail::ExchangeOwner &o) {
    return ExchangeCoordinatorAccess::field_command_buffer(*o.plan_);
  }
  static auto map_backing(const detail::ExchangeOwner &o) {
    auto result = ExchangeCoordinatorAccess::map_backing(*o.plan_);
    const auto append = [&](const auto &map) {
      const auto &slots = map.storage();
      result.push_back({slots.data(),slots.capacity()*sizeof(slots[0])});
      for (const auto &row : slots) {
        const auto &key = detail::key_buffer(row);
        result.push_back({key.data(),key.capacity()*sizeof(key[0])});
      }
    };
    append(o.occurrences_); append(o.fields_);
    std::sort(result.begin(),result.end(),[](const auto &a, const auto &b) {
      return std::less<const void *>{}(a.first,b.first);
    });
    return result;
  }
  static const auto &memberships(const detail::ExchangeOwner &o) { return o.memberships_; }
  static const auto &catalogue_rows(const MembershipFields &c) { return c.contributions_; }
  static const auto &catalogue_targets(const MembershipFields &c) { return c.targets_; }
  template<class Visit> static void catalogue_uuids(const MembershipFields &c, Visit visit) {
    visit(c.study_); visit(c.recipe_.selection); visit(c.recipe_.revision);
    for (const auto &target : c.targets_) {
      visit(target.object.scope); visit(target.revision);
    }
  }
  template<class Visit> static void catalogue_addresses(const MembershipFields &c, Visit visit) {
    for (const auto &target : c.targets_) {
      visit(target.object.address);
    }
  }
  static bool catalogue_equivalent(const MembershipFields &a, const MembershipFields &b) {
    if (a.study() != b.study() || a.recipe_.selection != b.recipe_.selection ||
        a.recipe_.revision != b.recipe_.revision || a.alternatives_ != b.alternatives_ ||
        a.contributions_.size() != b.contributions_.size() || a.targets_!=b.targets_) return false;
    for (std::size_t i=0; i<a.contributions_.size(); ++i) {
      const auto &x=a.contributions_[i], &y=b.contributions_[i];
      if (!(x.occurrence == y.occurrence) || x.kind != y.kind ||
          x.target_index!=y.target_index || !(a.target(x)==b.target(y)) || x.count != y.count) return false;
    }
    return true;
  }
  static auto catalogue(const LetterProjection &p, const std::shared_ptr<detail::ReservationDomain> &domain) {
    return MembershipFields::collect(p,true,domain);
  }
  static auto &catalogue_recipe(LetterProjection &p) { return const_cast<warm::Bundle &>(p.recipe()); }
  static auto contract(const LetterProjection &p, const OccurrenceKey &key, warm::Exposure target,
      const std::shared_ptr<detail::ReservationDomain> &domain) {
    return LetterExchangeContract::collect(p,key,target,domain);
  }
  static auto contract_copy(const LetterExchangeContract &c, const std::shared_ptr<detail::ReservationDomain> &domain) {
    return LetterExchangeContract(c,domain);
  }
  static auto pinned_contract(const LetterProjection &p,const OccurrenceKey &key,warm::Exposure target,
      const std::shared_ptr<detail::ReservationDomain> &domain,detail::PreparationWork &work,
      std::shared_ptr<const warm::Bundle> pin) {
    return LetterExchangeContract::collect(p,key,target,domain,&work,std::move(pin));
  }
  static const auto &contract_recipe(const LetterExchangeContract &c) { return c.recipe_pin_; }
  static const auto &contract_parents(const LetterExchangeContract &c,const ExchangeSource &s) { return c.parents(s); }
  static auto source_index(const ExchangeSource &s) { return s.recipe_tool_; }
  static void source_index(ExchangeSource &s,std::size_t i) { s.recipe_tool_=i; }
  template<class Visit> static void contract_uuids(const LetterExchangeContract &c, Visit visit) {
    visit(c.owner().study); visit(c.recipe().selection); visit(c.recipe().revision);
    const auto reference=[&](const auto &r) { visit(r.object.scope); visit(r.revision); };
    for (const auto *side : {&c.outgoing(),&c.incoming()})
      for (const auto &source : *side) {
        visit(source.occurrence.study); reference(source.definition);
        for (const auto &parent : source.parents) reference(parent.target);
      }
  }
  template<class Visit> static void contract_addresses(const LetterExchangeContract &c, Visit visit) {
    for (const auto *side : {&c.outgoing(),&c.incoming()})
      for (const auto &source : *side) {
        visit(source.definition.object.address);
        for (const auto &parent : source.parents) visit(parent.target.object.address);
      }
  }
  static auto contract_tables(const LetterExchangeContract &c) {
    return std::array<std::pair<const void *,std::size_t>,2>{{
      {c.outgoing_.data(),c.outgoing_.capacity()*sizeof(ExchangeSource)},
      {c.incoming_.data(),c.incoming_.capacity()*sizeof(ExchangeSource)}}};
  }
  static bool equivalent(const LetterExchangeContract &a, const LetterExchangeContract &b, bool reverse = false) {
    const auto &outgoing = reverse ? a.incoming() : a.outgoing();
    const auto &incoming = reverse ? a.outgoing() : a.incoming();
    if (!(a.owner() == b.owner()) || a.recipe().selection != b.recipe().selection ||
        a.recipe().revision != b.recipe().revision || (reverse ? a.to() : a.from()) != b.from() ||
        (reverse ? a.from() : a.to()) != b.to() ||
        a.mass() != b.mass() || a.match_work() != b.match_work() ||
        !b.matches(a,reverse,UINT64_MAX))
      return false;
    // Bijections accept caller permutations; immutable declaration copies also
    // retain canonical source/parent order, which now supplies nibble provenance.
    const auto ordered = [&](const auto &x, const auto &y) {
      if (x.size() != y.size()) return false;
      for (std::size_t i=0; i<x.size(); ++i) {
        if (!(x[i].occurrence == y[i].occurrence) || !(x[i].definition == y[i].definition) ||
            x[i].mass != y[i].mass || x[i].node != y[i].node) return false;
        const auto &xp=a.parents(x[i]), &yp=b.parents(y[i]);
        if (xp.size()!=yp.size()) return false;
        for (std::size_t j=0; j<xp.size(); ++j) {
          const auto &p=xp[j], &q=yp[j];
          if (!(p.target == q.target) || p.ordinal != q.ordinal ||
              p.stored_mass != q.stored_mass || p.contribution != q.contribution) return false;
        }
      }
      return true;
    };
    return ordered(outgoing,b.outgoing()) && ordered(incoming,b.incoming());
  }
  static const auto &declarations(const detail::ExchangeOwner &o) { return o.declarations_; }
  static auto declaration(const detail::ExchangeOwner &o, const OccurrenceKey &key, warm::Exposure target) {
    detail::PreparationWork meter(UINT64_MAX); return o.declaration(key,target,meter);
  }
  static auto input_admission(const LetterProjection &p,
      const std::vector<detail::OccurrenceBinding> &rows,
      const std::vector<detail::ExchangeFieldBinding> &fields, detail::AdoptionInputBudget budget) {
    return detail::ExchangeOwner::input_admission(p,rows,fields,budget);
  }
  static auto adopt(field::Harness &h, const LetterProjection &p,
      const std::vector<detail::OccurrenceBinding> &rows,
      const std::vector<detail::ExchangeFieldBinding> &fields,
      detail::AdoptionInputBudget budget, detail::AdoptionInputResult &result) {
    detail::ReservationResult reservation;
    detail::ReservationResult bootstrap;
    detail::ProjectionSetupResult setup;
    return detail::ExchangeOwner::adopt(h,p,rows,fields,budget,result,{UINT64_MAX,UINT64_MAX},reservation,UINT64_MAX,setup,
        {UINT64_MAX,UINT64_MAX},bootstrap);
  }
  static auto adopt(field::Harness &h, const LetterProjection &p,
      const std::vector<detail::OccurrenceBinding> &rows,
      const std::vector<detail::ExchangeFieldBinding> &fields,
      detail::ReservationBudget budget, detail::ReservationResult &result) {
    detail::AdoptionInputResult input;
    detail::ReservationResult bootstrap;
    detail::ProjectionSetupResult setup;
    return detail::ExchangeOwner::adopt(h,p,rows,fields,{UINT64_MAX,UINT64_MAX,UINT64_MAX},input,budget,result,UINT64_MAX,setup,
        {UINT64_MAX,UINT64_MAX},bootstrap);
  }
  static auto adopt(field::Harness &h, const LetterProjection &p,
      const std::vector<detail::OccurrenceBinding> &rows,
      const std::vector<detail::ExchangeFieldBinding> &fields,
      std::uint64_t setup_work, detail::ProjectionSetupResult &setup) {
    detail::AdoptionInputResult input;
    detail::ReservationResult reservation;
    detail::ReservationResult bootstrap;
    return detail::ExchangeOwner::adopt(h,p,rows,fields,{UINT64_MAX,UINT64_MAX,UINT64_MAX},input,
        {UINT64_MAX,UINT64_MAX},reservation,setup_work,setup,{UINT64_MAX,UINT64_MAX},bootstrap);
  }
  static auto adopt_bootstrap(field::Harness &h, const LetterProjection &p,
      const std::vector<detail::OccurrenceBinding> &rows,
      const std::vector<detail::ExchangeFieldBinding> &fields,
      detail::ReservationBudget budget, detail::ReservationResult &bootstrap,
      detail::ReservationResult &selected,
      detail::ReservationBudget selected_budget = {UINT64_MAX,UINT64_MAX},
      std::uint64_t setup_work = UINT64_MAX) {
    detail::AdoptionInputResult input;
    detail::ProjectionSetupResult setup;
    return detail::ExchangeOwner::adopt(h,p,rows,fields,{UINT64_MAX,UINT64_MAX,UINT64_MAX},input,
        selected_budget,selected,setup_work,setup,budget,bootstrap);
  }
  static auto adopt(field::Harness &h, const LetterProjection &p,
      const std::vector<detail::OccurrenceBinding> &rows,
      const std::vector<detail::ExchangeFieldBinding> &fields) {
    detail::AdoptionInputResult result;
    auto owner = adopt(h,p,rows,fields,{UINT64_MAX,UINT64_MAX,UINT64_MAX},result);
    if (!owner) throw warm::Error("test owner input admission failed");
    return owner;
  }
  static void prepare(detail::ExchangeOwner &o, const OccurrenceKey &key, warm::Exposure target,
      const std::vector<detail::ExchangeIncoming> &incoming,
      const std::vector<detail::ExchangeNewField> &fields) {
    if (o.prepare(key,target,incoming,fields,UINT64_MAX,UINT64_MAX,UINT64_MAX).status != detail::CaptureStatus::Ok)
      throw warm::Error("test owner capture work overflow");
  }
  static auto prepare(detail::ExchangeOwner &o, const OccurrenceKey &key, warm::Exposure target,
      const std::vector<detail::ExchangeIncoming> &incoming,
      const std::vector<detail::ExchangeNewField> &fields, std::uint64_t capture_work) {
    const auto result = o.prepare(key,target,incoming,fields,UINT64_MAX,capture_work,UINT64_MAX);
    if (result.phase == detail::PreparationPhase::Commands) throw warm::Error("test command work overflow");
    return result.capture;
  }
  static auto prepare(detail::ExchangeOwner &o, const OccurrenceKey &key, warm::Exposure target,
      const std::vector<detail::ExchangeIncoming> &incoming,
      const std::vector<detail::ExchangeNewField> &fields,
      std::uint64_t capture_work, std::uint64_t command_work) {
    return o.prepare(key,target,incoming,fields,UINT64_MAX,capture_work,command_work);
  }
  static auto prepare(detail::ExchangeOwner &o, const OccurrenceKey &key, warm::Exposure target,
      const std::vector<detail::ExchangeIncoming> &incoming,
      const std::vector<detail::ExchangeNewField> &fields,
      std::uint64_t staging, std::uint64_t capture, std::uint64_t commands) {
    return o.prepare(key,target,incoming,fields,staging,capture,commands);
  }
  static const auto &projection(const detail::ExchangeOwner &o) { return *o.projection_; }
  static auto &mutable_projection(detail::ExchangeOwner &o, bool spare) {
    return spare ? *o.prospective_ : *o.projection_;
  }
  static const auto &spare_projection(const detail::ExchangeOwner &o) { return *o.prospective_; }
  static auto storage(const detail::ExchangeOwner &o) { return o.projection_storage(); }
  static void stage_projection(detail::ExchangeOwner &o, const OccurrenceKey &key, warm::Exposure target) {
    detail::PreparationWork meter(UINT64_MAX); o.prepare_projection(key,target,meter);
  }
  static const auto &mappings(const detail::ExchangeOwner &o) { return o.occurrences_; }
  static const auto &fields(const detail::ExchangeOwner &o) { return o.fields_; }
  static const auto &plan(const detail::ExchangeOwner &o) { return *o.plan_; }
  static auto &plan(detail::ExchangeOwner &o) { return *o.plan_; }
  static auto binding_status(const detail::ExchangeOwner &o) {
    return ExchangeCoordinatorAccess::binding_status(*o.plan_,o.occurrences_,o.fields_);
  }
  static bool pending(const detail::ExchangeOwner &o) { return o.plan_ && !o.completed_; }
  static auto generation(const detail::ExchangeOwner &o) { return o.generation_; }
  static const auto &outgoing_report(const detail::ExchangeOwner &o) { return o.outgoing_report_; }
  static const auto &balance_report(const detail::ExchangeOwner &o) { return o.balance_report_; }
  static auto seal(detail::ExchangeOwner &o, const field::LiveRefreshSpec &spec,
      detail::HierarchySealBudget budget) { return o.seal(spec,budget); }
  static void seal(detail::ExchangeOwner &o, const field::LiveRefreshSpec &spec) {
    const auto charge = ExchangeCoordinatorAccess::hierarchy_charge(*o.plan_,spec);
    if (o.seal(spec,{charge.required_bytes,charge.required_work}).status != detail::HierarchySealStatus::Ok)
      throw warm::Error("test owner hierarchy sealing rejected");
  }
  static auto execute(detail::ExchangeOwner &o, std::uint64_t generation,
      std::uint64_t work = UINT64_MAX) { return o.execute(generation,work,UINT64_MAX,UINT64_MAX); }
};
}
