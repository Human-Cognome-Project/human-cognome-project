#include "assembly/letter_projection.h"
#include "assembly/adoption_input.h"
#include <limits>

namespace assembly {
namespace {
void add(std::uint64_t &to, std::uint64_t n) {
  if (n > UINT64_MAX - to) throw warm::Error("projection storage charge overflow");
  to += n;
}
struct Census {
  ProjectionStorage result;
  detail::PreparationWork *meter{};
  bool logical{};
  void count(std::uint64_t &to, std::uint64_t n) {
    if (meter) detail::input_add(to,n,*meter); else add(to,n);
  }
  template<class T> void buffer(const std::vector<T> &v) {
    if (meter) { meter->charge(1); meter->elements(v.size(),1); }
    const auto n = logical ? v.size() : v.capacity();
    if (meter) detail::input_elements(result.vector_bytes,n,sizeof(T),*meter);
    else {
      if (n > UINT64_MAX / sizeof(T)) throw warm::Error("projection capacity overflow");
      add(result.vector_bytes,n*sizeof(T));
    }
  }
  void text(const std::string &s) {
    if (meter) meter->charge(1);
    count(result.text_bytes,logical ? s.size() : s.capacity());
    count(result.text_bytes,1);
  }
  void uuid(const warm::Uuid &u) { text(u.str()); }
  void object(const warm::ObjectKey &o) { uuid(o.scope); buffer(o.address); }
  void tool_ref(const warm::ToolRef &t) { object(t.object); uuid(t.revision); }
  void id(const warm::OccurrenceId &i) { buffer(i.path); }
  void key(const OccurrenceKey &k) { uuid(k.study); id(k.source); }
  void bundle(const warm::Bundle &b) {
    uuid(b.ref.selection); uuid(b.ref.revision); uuid(b.owner); uuid(b.creator);
    buffer(b.tools);
    for (const auto &t : b.tools) {
      tool_ref(t.ref); uuid(t.epoch); text(t.recipe); if (t.notation) text(*t.notation);
      buffer(t.parents); for (const auto &p : t.parents) tool_ref(p.target);
      buffer(t.member_of); for (const auto &m : t.member_of) tool_ref(m);
      uuid(t.author);
    }
    buffer(b.geometry); for (const auto &g : b.geometry) { object(g.object); buffer(g.spheres); }
    buffer(b.nodes);
    buffer(b.occurrences); for (const auto &o : b.occurrences) { id(o.id); object(o.definition); }
    buffer(b.structural_occurrences);
    for (const auto &o : b.structural_occurrences) { id(o.id); object(o.definition); }
    buffer(b.selections);
    for (const auto &s : b.selections) { buffer(s.parts); for (const auto &p : s.parts) id(p); }
    buffer(b.contacts); for (const auto &c : b.contacts) { id(c.a); id(c.b); }
    buffer(b.focal_fields); for (const auto &f : b.focal_fields) object(f.object);
  }
  template<class Input> void input(const Input &in) {
    buffer(in.nodes); buffer(in.definitions);
    for (const auto &d : in.definitions) {
      text(d.id); buffer(d.components); buffer(d.parents);
      for (const auto &p : d.parents) text(p.value);
    }
    buffer(in.occurrences);
    for (const auto &o : in.occurrences) { text(o.id); text(o.definition); }
    buffer(in.selections);
    for (const auto &s : in.selections) {
      text(s.aggregate); buffer(s.parts); for (const auto &p : s.parts) text(p);
    }
    buffer(in.contacts); for (const auto &c : in.contacts) { text(c.a); text(c.b); }
    buffer(in.memberships); for (const auto &m : in.memberships) { text(m.occurrence); text(m.group); }
    buffer(in.focal_fields); for (const auto &f : in.focal_fields) text(f.value);
  }
  void parents(const std::vector<ParentContribution> &v) {
    buffer(v); count(result.parent_slots,v.size());
    for (const auto &p : v) { key(p.occurrence); object(p.field.object); }
  }
};
}

std::uint64_t ProjectionStorage::charged_bytes() const {
  auto total = object_bytes;
  assembly::add(total,vector_bytes); assembly::add(total,text_bytes);
  return total;
}
void ProjectionStorage::add(const ProjectionStorage &other) {
  // Check the complete sum first, so overflow cannot leave a partial report.
  auto total = charged_bytes(); assembly::add(total,other.charged_bytes());
  auto slots = parent_slots; assembly::add(slots,other.parent_slots);
  object_bytes += other.object_bytes;
  vector_bytes += other.vector_bytes;
  text_bytes += other.text_bytes;
  parent_slots = slots;
}
ProjectionStorage LetterProjection::storage() const {
  return storage(nullptr,false);
}
ProjectionStorage LetterProjection::storage(detail::PreparationWork *meter, bool logical) const {
  Census c;
  c.meter = meter; c.logical = logical;
  if (meter) meter->charge(1);
  c.result.object_bytes = sizeof(*this);
  c.uuid(study_); c.bundle(recipe_); c.buffer(exposures_); c.buffer(occurrences_);
  for (const auto &o : occurrences_) {
    c.key(o.key); c.tool_ref(o.definition);
    if (o.byte) { c.key(o.byte->owner); c.tool_ref(o.byte->character); }
    if (o.complete_character) c.tool_ref(*o.complete_character);
  }
  c.parents(parents_); c.parents(spare_parents_);
  c.input(input_); c.input(structural_input_);
  auto total = c.result.object_bytes;
  c.count(total,c.result.vector_bytes); c.count(total,c.result.text_bytes);
  return c.result;
}
}
