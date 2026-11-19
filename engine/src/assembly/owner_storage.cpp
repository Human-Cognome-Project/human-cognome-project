#include "assembly/exchange_owner.h"

namespace assembly::detail {
namespace {
// Same conservative capacity convention as ProjectionStorage. Embedded object
// sizes are charged by their containing object/vector, never a second time.
struct Census {
  ProjectionStorage result;
  void add(std::uint64_t &to, std::uint64_t n) {
    if (n > UINT64_MAX - to) throw warm::Error("owner storage charge overflow");
    to += n;
  }
  template<class T, class A> void buffer(const std::vector<T,A> &v) {
    if (v.capacity() > UINT64_MAX / sizeof(T)) throw warm::Error("owner capacity overflow");
    add(result.vector_bytes, v.capacity() * sizeof(T));
  }
  template<class A> void buffer(const std::vector<bool,A> &v) {
    // vector<bool>::capacity is in bits. Round without an overflowing +7.
    add(result.vector_bytes, v.capacity()/8 + (v.capacity()%8 != 0));
  }
  void uuid(const warm::Uuid &u) {
    add(result.text_bytes,u.str().capacity()); add(result.text_bytes,1);
  }
  void object(const warm::ObjectKey &o) { uuid(o.scope); buffer(o.address); }
  void ref(const warm::ToolRef &r) { object(r.object); uuid(r.revision); }
  void key(const OccurrenceKey &k) { uuid(k.study); buffer(k.source.path); }
  void row(const OccurrenceBinding &r) { key(r.occurrence); }
  void row(const ExchangeFieldBinding &r) { object(r.field.object); }
  template<class T> void map(const ExchangeMap<T> &m) {
    buffer(m.storage());
    // Includes constructed inactive slots; begin()/end() expose only live rows.
    for (const auto &r : m.storage()) row(r);
  }
  template<class A> void sources(const std::vector<ExchangeSource,A> &v) {
    buffer(v);
    for (const auto &s : v) {
      key(s.occurrence); ref(s.definition); buffer(s.parents);
      for (const auto &p : s.parents) ref(p.target);
    }
  }
  void contract(const LetterExchangeContract &c) {
    add(result.object_bytes,sizeof(c)); key(c.owner());
    uuid(c.recipe().selection); uuid(c.recipe().revision);
    sources(c.outgoing()); sources(c.incoming());
  }
};
}

OwnerStorage ExchangeOwner::storage() const {
  OwnerStorage out;
  out.owner.object_bytes = sizeof(*this);
  if (reservations_) out.owner.object_bytes += sizeof(*reservations_);
  out.projections = projection_storage();
  const auto &p = *plan_;
  const auto &b = *p.bound_;
  Census maps;
  maps.map(occurrences_); maps.map(fields_);
  // The persistent binding's owned maps stay empty. Their headers are already
  // in sizeof(b); only current/candidate backing is retained on this path.
  maps.map(b.occurrences_); maps.map(b.fields_);
  maps.map(p.next_); maps.map(p.next_fields_);
  out.maps = maps.result;

  Census binding;
  binding.result.object_bytes = sizeof(b);
  binding.buffer(b.masses_); binding.buffer(b.outgoing_); binding.buffer(b.edges_);
  binding.buffer(b.outgoing_rows_); binding.buffer(b.edge_rows_);
  out.binding = binding.result;

  Census coordinator;
  coordinator.result.object_bytes = sizeof(p);
  coordinator.buffer(p.commands_); coordinator.buffer(p.intrinsics_);
  coordinator.buffer(p.field_commands_); coordinator.buffer(p.nodes_);
  coordinator.buffer(p.children_); coordinator.buffer(p.contributions_);
  out.coordinator = coordinator.result;

  Census declarations;
  declarations.buffer(declarations_);
  // One payload per exact selected owner; the binding's directional alias does
  // not own a second graph or another shared allocation identity.
  for (const auto &c : declarations_) {
    if (c->recipe_pin_!=projection_->recipe_)
      throw warm::Error("declaration does not pin its owner's exact recipe");
    // Validate each recipe-local index; aliased parent backing is already
    // counted once with the projection recipe, never with the declaration.
    for (const auto *side : {&c->outgoing(),&c->incoming()})
      for (const auto &source : *side) (void)c->parents(source);
    declarations.contract(*c);
  }
  out.declarations = declarations.result;
  Census memberships;
  memberships.result.object_bytes = sizeof(*memberships_);
  memberships.uuid(memberships_->study_);
  memberships.uuid(memberships_->recipe_.selection);
  memberships.uuid(memberships_->recipe_.revision);
  memberships.buffer(memberships_->contributions_);
  for (const auto &row : memberships_->contributions_) {
    memberships.buffer(row.occurrence.path);
    memberships.ref(row.target);
  }
  out.memberships = memberships.result;
  // Binding references alias the retained declarations/catalogue. Their pointer
  // objects are in sizeof(b); count each pointee and control block identity once,
  // including the immutable recipe shared by the two mutable projection views.
  const std::uint64_t shared_recipes = projection_->recipe_ == prospective_->recipe_ ? 1 : 2;
  if (declarations_.size() > UINT64_MAX-1-shared_recipes) throw warm::Error("owner shared count overflow");
  out.shared_allocations = declarations_.size() + 1 + shared_recipes;
  out.total(); // Reject aggregate overflow before returning any report.
  return out;
}
}
