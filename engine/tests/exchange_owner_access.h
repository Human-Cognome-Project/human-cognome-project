#pragma once
#include "assembly/exchange_owner.h"
#include "exchange_coordinator_access.h"
namespace assembly::testing {
struct ExchangeOwnerAccess {
  static auto resources(const detail::ExchangeOwner &o) { return o.storage(); }
  static auto reservations(const detail::ExchangeOwner &o) { return o.reservations_->result(); }
  static auto reservation_domain(const detail::ExchangeOwner &o) { return o.reservations_; }
  template<class Visit> static void projection_table_vectors(const LetterProjection &p, Visit visit) {
    visit(p.exposures_); visit(p.occurrences_); visit(p.parents_); visit(p.spare_parents_);
    visit(p.recipe_.tools); visit(p.recipe_.geometry); visit(p.recipe_.nodes);
    visit(p.recipe_.occurrences); visit(p.recipe_.selections); visit(p.recipe_.contacts);
    visit(p.recipe_.structural_occurrences); visit(p.recipe_.focal_fields);
    for (const auto &tool : p.recipe_.tools) { visit(tool.parents); visit(tool.member_of); }
    for (const auto &geometry : p.recipe_.geometry) visit(geometry.spheres);
    for (const auto &selection : p.recipe_.selections) visit(selection.parts);
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
  template<class Visit> static void projection_texts(const LetterProjection &p, Visit visit) {
    recipe_uuids(p.recipe_,[&](const auto &uuid) { visit(uuid.str()); });
    visit(p.study_.str());
    const auto reference=[&](const auto &ref) {
      visit(ref.object.scope.str()); visit(ref.revision.str());
    };
    for (const auto &o : p.occurrences_) {
      visit(o.key.study.str()); reference(o.definition);
      if (o.byte) { visit(o.byte->owner.study.str()); reference(o.byte->character); }
      if (o.complete_character) reference(*o.complete_character);
    }
    for (const auto *rows : {&p.parents_,&p.spare_parents_})
      for (const auto &row : *rows) {
        visit(row.occurrence.study.str()); visit(row.field.object.scope.str());
      }
    for (const auto &tool : p.recipe_.tools) {
      visit(tool.recipe);
      if (tool.notation) visit(*tool.notation);
    }
  }
  template<class Visit> static void projection_backing(const LetterProjection &p, Visit visit) {
    projection_table_vectors(p,[&](const auto &table) {
      visit(std::make_pair(static_cast<const void *>(table.data()),table.capacity()*sizeof(table[0])));
    });
    projection_texts(p,[&](const auto &text) {
      visit(std::make_pair(static_cast<const void *>(text.data()),warm::recipe_text_backing(text)));
    });
  }
  template<class Visit> static void projection_paths(const LetterProjection &p, Visit visit) {
    for (const auto &o : p.recipe_.occurrences) visit(o.id.path);
    for (const auto &o : p.recipe_.structural_occurrences) visit(o.id.path);
    for (const auto &s : p.recipe_.selections)
      for (const auto &id : s.parts) visit(id.path);
    for (const auto &c : p.recipe_.contacts) { visit(c.a.path); visit(c.b.path); }
    for (const auto &o : p.occurrences_) {
      visit(o.key.source.path);
      if (o.byte) visit(o.byte->owner.source.path);
    }
    for (const auto *rows : {&p.parents_,&p.spare_parents_})
      for (const auto &row : *rows) visit(row.occurrence.source.path);
  }
  template<class Visit> static void projection_addresses(const LetterProjection &p, Visit visit) {
    recipe_addresses(p.recipe_,visit);
    for (const auto &o : p.occurrences_) {
      visit(o.definition.object.address);
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
  static bool projection_equivalent(const LetterProjection &a, const LetterProjection &b) {
    if (!recipe_equivalent(a.recipe_,b.recipe_) || a.study_ != b.study_ || a.exposures_ != b.exposures_ ||
        a.occurrences_.size() != b.occurrences_.size()) return false;
    for (std::size_t i=0; i<a.occurrences_.size(); ++i) {
      const auto &x=a.occurrences_[i], &y=b.occurrences_[i];
      if (!(x.key == y.key) || !(x.definition == y.definition) || x.slot != y.slot ||
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
  static auto projection_backing_bytes(const LetterProjection &p) {
    std::uint64_t bytes = 0;
    projection_backing(p,[&](const auto &entry) { bytes += entry.second; });
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
  template<class Visit> static void catalogue_addresses(const MembershipFields &c, Visit visit) {
    for (const auto &row : c.contributions()) {
      visit(row.field.object.address);
      visit(row.target.object.address);
    }
  }
  static bool catalogue_equivalent(const MembershipFields &a, const MembershipFields &b) {
    if (a.study() != b.study() || a.contributions().size() != b.contributions().size()) return false;
    for (std::size_t i=0; i<a.contributions().size(); ++i) {
      const auto &x=a.contributions()[i], &y=b.contributions()[i];
      if (!(x.occurrence == y.occurrence) || x.field.kind != y.field.kind ||
          !(x.field.object == y.field.object) || !(x.target == y.target) || x.count != y.count) return false;
    }
    return true;
  }
  static auto catalogue(const LetterProjection &p, const std::shared_ptr<detail::ReservationDomain> &domain) {
    return MembershipFields::collect(p,true,domain);
  }
  static auto &catalogue_recipe(LetterProjection &p) { return p.recipe_; }
  static auto contract(const LetterProjection &p, const OccurrenceKey &key, warm::Exposure target,
      const std::shared_ptr<detail::ReservationDomain> &domain) {
    return LetterExchangeContract::collect(p,key,target,domain);
  }
  static auto contract_copy(const LetterExchangeContract &c, const std::shared_ptr<detail::ReservationDomain> &domain) {
    return LetterExchangeContract(c,domain);
  }
  template<class Visit> static void contract_addresses(const LetterExchangeContract &c, Visit visit) {
    for (const auto *side : {&c.outgoing(),&c.incoming()})
      for (const auto &source : *side) {
        visit(source.definition.object.address);
        for (const auto &parent : source.parents) visit(parent.target.object.address);
      }
    for (const auto &leaf : c.leaves()) visit(leaf.definition.object.address);
  }
  static auto contract_tables(const LetterExchangeContract &c) {
    return std::array<std::pair<const void *,std::size_t>,3>{{
      {c.outgoing_.data(),c.outgoing_.capacity()*sizeof(ExchangeSource)},
      {c.incoming_.data(),c.incoming_.capacity()*sizeof(ExchangeSource)},
      {c.leaves_.data(),c.leaves_.capacity()*sizeof(ExchangeLeaf)}}};
  }
  static bool equivalent(const LetterExchangeContract &a, const LetterExchangeContract &b) {
    if (!(a.owner() == b.owner()) || a.recipe().selection != b.recipe().selection ||
        a.recipe().revision != b.recipe().revision || a.from() != b.from() || a.to() != b.to() ||
        a.mass() != b.mass() || a.match_work() != b.match_work() || a.leaves().size() != b.leaves().size() ||
        !a.matches(b.outgoing().data(),b.outgoing().size(),b.incoming().data(),b.incoming().size(),UINT64_MAX))
      return false;
    for (std::size_t i = 0; i < a.leaves().size(); ++i) {
      const auto &x = a.leaves()[i], &y = b.leaves()[i];
      if (x.character != y.character || x.byte != y.byte || x.nibble != y.nibble ||
          !(x.definition == y.definition) || x.count != y.count) return false;
    }
    return true;
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
    detail::ProjectionSetupResult setup;
    return detail::ExchangeOwner::adopt(h,p,rows,fields,budget,result,{UINT64_MAX,UINT64_MAX},reservation,UINT64_MAX,setup);
  }
  static auto adopt(field::Harness &h, const LetterProjection &p,
      const std::vector<detail::OccurrenceBinding> &rows,
      const std::vector<detail::ExchangeFieldBinding> &fields,
      detail::ReservationBudget budget, detail::ReservationResult &result) {
    detail::AdoptionInputResult input;
    detail::ProjectionSetupResult setup;
    return detail::ExchangeOwner::adopt(h,p,rows,fields,{UINT64_MAX,UINT64_MAX,UINT64_MAX},input,budget,result,UINT64_MAX,setup);
  }
  static auto adopt(field::Harness &h, const LetterProjection &p,
      const std::vector<detail::OccurrenceBinding> &rows,
      const std::vector<detail::ExchangeFieldBinding> &fields,
      std::uint64_t setup_work, detail::ProjectionSetupResult &setup) {
    detail::AdoptionInputResult input;
    detail::ReservationResult reservation;
    return detail::ExchangeOwner::adopt(h,p,rows,fields,{UINT64_MAX,UINT64_MAX,UINT64_MAX},input,
        {UINT64_MAX,UINT64_MAX},reservation,setup_work,setup);
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
  static const auto &spare_projection(const detail::ExchangeOwner &o) { return *o.prospective_; }
  static auto storage(const detail::ExchangeOwner &o) { return o.projection_storage(); }
  static void stage_projection(detail::ExchangeOwner &o, const OccurrenceKey &key, warm::Exposure target) {
    detail::PreparationWork meter(UINT64_MAX); o.prepare_projection(key,target,meter);
  }
  static const auto &mappings(const detail::ExchangeOwner &o) { return o.occurrences_; }
  static const auto &fields(const detail::ExchangeOwner &o) { return o.fields_; }
  static const auto &plan(const detail::ExchangeOwner &o) { return *o.plan_; }
  static auto &plan(detail::ExchangeOwner &o) { return *o.plan_; }
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
