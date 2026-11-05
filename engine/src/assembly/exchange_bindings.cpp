#include "assembly/exchange_bindings.h"
#include "assembly/membership_fields.h"

#include <algorithm>
#include <cmath>

namespace assembly::detail {
namespace {
void need(bool ok, const char *message) {
  if (!ok) throw warm::Error(message);
}
bool same(engine::RecordHandle a, engine::RecordHandle b) {
  return a.context == b.context && a.record == b.record &&
         a.address == b.address && a.incarnation == b.incarnation;
}
bool current(engine::RecordHandle h, const std::vector<engine::RecordHandle> &rows) {
  return h.context && h.incarnation && h.record >= 0 &&
         std::size_t(h.record) < rows.size() && same(h, rows[h.record]);
}
bool same(const FieldKey &a, const FieldKey &b) {
  return a.kind == b.kind && a.object == b.object;
}
}  // namespace

ExchangeBindings::ExchangeBindings(field::Harness &h, const LetterProjection &projection,
    LetterExchangeContract contract, std::vector<OccurrenceBinding> occurrences,
    std::vector<ExchangeFieldBinding> fields)
    : h_(h), contract_(std::move(contract)), occurrences_(std::move(occurrences)),
      fields_(std::move(fields)) {
  const auto &s = h_.prepared_storage();
  need(h_.ready(), "cannot bind an unready exchange owner");
  need(contract_.owner().study == projection.study() &&
       contract_.recipe().selection == projection.recipe().ref.selection &&
       contract_.recipe().revision == projection.recipe().ref.revision,
       "exchange contract belongs to another projection");
  need(occurrences_.size() == s.live.particles && fields_.size() <= s.live.fields,
       "exchange binding inventory differs from committed execution");
  std::size_t active = 0;
  for (const auto &o : projection.occurrences()) active += o.active;
  need(active == occurrences_.size(), "exchange owner exposure is incomplete");
  for (std::size_t i = 0; i < occurrences_.size(); ++i) {
    const auto &binding = occurrences_[i];
    const auto o = std::find_if(projection.occurrences().begin(), projection.occurrences().end(),
        [&](const auto &v) { return v.key == binding.occurrence; });
    need(o != projection.occurrences().end() && o->active && current(binding.handle, s.particle_headers),
         "exchange occurrence is inactive, foreign or stale");
    for (std::size_t j = 0; j < i; ++j)
      need(!(occurrences_[j].occurrence == binding.occurrence) &&
           !same(occurrences_[j].handle, binding.handle), "duplicate exchange occurrence binding");
    const auto tool = std::find_if(projection.recipe().tools.begin(), projection.recipe().tools.end(),
        [&](const auto &v) { return v.ref == o->definition; });
    need(tool != projection.recipe().tools.end() && tool->mass &&
         *tool->mass == s.source_masses[binding.handle.record], "bound exact source mass differs");
    masses_.push_back(*tool->mass);
  }
  for (std::size_t i = 0; i < fields_.size(); ++i) {
    need(current(fields_[i].handle, s.field_headers), "stale exchange field binding");
    for (std::size_t j = 0; j < i; ++j)
      need(!same(fields_[i].field, fields_[j].field) && !same(fields_[i].handle, fields_[j].handle),
           "aliased exchange field binding");
  }
  std::vector<bool> used(s.live.edges, false);
  outgoing_rows_.assign(s.particle_headers.size(), false);
  const auto memberships = membership_fields(projection);
  for (const auto &source : contract_.outgoing()) {
    const auto binding = std::find_if(occurrences_.begin(), occurrences_.end(),
        [&](const auto &v) { return v.occurrence == source.occurrence; });
    need(binding != occurrences_.end(), "outgoing alternative is not currently exposed");
    outgoing_.push_back(binding->handle);
    outgoing_rows_[binding->handle.record] = true;
    const auto add = [&](const FieldKey &key, std::uint64_t count) {
      const auto field = std::find_if(fields_.begin(), fields_.end(),
          [&](const auto &v) { return same(v.field, key); });
      need(field != fields_.end(), "missing declared exchange field binding");
      std::size_t row = 0;
      for (; row < s.live.edges; ++row)
        if (!used[row] && same(s.edges[row].first, binding->handle) &&
            same(s.edges[row].second, field->handle) && s.source_counts[row] == count) break;
      need(row < s.live.edges, "missing exact committed exchange participation");
      need(std::isfinite(s.edges[row].share) && s.edges[row].share > 0,
           "invalid committed exchange share");
      used[row] = true;
      edges_.push_back({row, s.edges[row], count});
    };
    add({GroupKind::SiblingDefinition, source.definition.object}, source.mass);
    for (const auto &parent : source.parents)
      add({GroupKind::ParentValue, parent.target.object}, parent.contribution);
    for (const auto &membership : memberships.contributions())
      if (membership.occurrence == source.occurrence) add(membership.field, membership.count);
  }
  need(!outgoing_.empty(), "empty outgoing alternative");
  for (std::size_t row = 0; row < s.live.edges; ++row)
    for (const auto &body : outgoing_)
      need(!same(s.edges[row].first, body) || used[row], "undeclared outgoing participation");
  generation_ = s.generation;
  source_edges_ = s.live.edges;
  edge_rows_ = std::move(used);
  // Bounded read-only validation; reserve before scanning live source metadata.
  const auto terms = occurrences_.size() + fields_.size() + edges_.size() + source_edges_ + 1;
  need(terms <= UINT64_MAX / 16, "exchange binding work overflow");
  work_ = 16 * terms;
  need(validate(generation_, work_).status == field::UpdateStatus::Ok,
       "exchange binding changed during preparation");
}

field::UpdateResult ExchangeBindings::validate(std::uint64_t expected, std::uint64_t work) const {
  field::UpdateResult out;
  const auto &s = h_.prepared_storage();
  out.generation = s.generation;
  if (!h_.ready()) { out.status = field::UpdateStatus::Unready; return out; }
  if (expected != generation_ || s.generation != generation_) {
    out.status = field::UpdateStatus::Stale; return out;
  }
  work = std::min(work, s.capacity.limits()[field::Budget::PlacementWork]);
  if (work < work_) {
    out.status = field::UpdateStatus::CapacityExhausted;
    out.failure = {field::Budget::PlacementWork, work_, work};
    return out;
  }
  out.structural_visits = work_;
  if (s.live.particles != occurrences_.size() || s.live.edges != source_edges_) return out;
  for (std::size_t i = 0; i < occurrences_.size(); ++i) {
    const auto handle = occurrences_[i].handle;
    if (!current(handle, s.particle_headers)) { out.status = field::UpdateStatus::Stale; return out; }
    if (s.source_masses[handle.record] != masses_[i]) return out;
  }
  for (const auto &f : fields_)
    if (!current(f.handle, s.field_headers)) { out.status = field::UpdateStatus::Stale; return out; }
  for (const auto &edge : edges_) {
    const auto &live = s.edges[edge.row];
    if (!same(live.first, edge.header.first) || !same(live.second, edge.header.second) ||
        live.ordinal != edge.header.ordinal || live.share != edge.header.share ||
        s.source_counts[edge.row] != edge.count) return out;
  }
  for (std::size_t row = 0; row < s.live.edges; ++row) {
    const auto record = s.edges[row].first.record;
    if (record >= 0 && std::size_t(record) < outgoing_rows_.size() &&
        outgoing_rows_[record] && !edge_rows_[row]) return out;
  }
  out.status = field::UpdateStatus::Ok;
  return out;
}
}  // namespace assembly::detail
