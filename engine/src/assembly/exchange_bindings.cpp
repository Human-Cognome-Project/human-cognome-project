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
}  // namespace

ExchangeBindings::ExchangeBindings(field::Harness &h, const std::shared_ptr<ReservationDomain> &domain)
    : h_(h), occurrences_(domain), fields_(domain), masses_(ReservationAllocator<std::uint64_t>(domain.get())),
      outgoing_(ReservationAllocator<engine::RecordHandle>(domain.get())), edges_(ReservationAllocator<Edge>(domain.get())),
      outgoing_rows_(ReservationAllocator<bool>(domain.get())), edge_rows_(ReservationAllocator<bool>(domain.get())) {
  const auto &s = h.prepared_storage();
  const auto &limits = s.capacity.limits();
  masses_.reserve(limits[field::Budget::Particles]);
  outgoing_.reserve(limits[field::Budget::Particles]);
  edges_.reserve(limits[field::Budget::Participation]);
  outgoing_rows_.reserve(s.particle_headers.size());
  edge_rows_.reserve(limits[field::Budget::Participation]);
}

ExchangeBindings::ExchangeBindings(field::Harness &h, const LetterProjection &projection,
    LetterExchangeContract contract, const std::vector<OccurrenceBinding> &occurrences,
    const std::vector<ExchangeFieldBinding> &fields)
    : ExchangeBindings(h, projection,
        std::make_shared<const LetterExchangeContract>(std::move(contract)),
        std::make_shared<const MembershipFields>(MembershipFields::collect(projection,true)),
        occurrences, fields) {}

ExchangeBindings::ExchangeBindings(field::Harness &h, const LetterProjection &projection,
    std::shared_ptr<const LetterExchangeContract> contract,
    std::shared_ptr<const MembershipFields> memberships, const std::vector<OccurrenceBinding> &occurrences,
    const std::vector<ExchangeFieldBinding> &fields, const std::shared_ptr<ReservationDomain> &domain)
    : ExchangeBindings(h,domain) {
  const auto &limits = h.prepared_storage().capacity.limits();
  const ExchangeMap<ExchangeFieldBinding> trusted(fields,domain);
  reserve_exchange_maps(projection,limits[field::Budget::Particles],limits[field::Budget::Fields],limits[field::Budget::Batch],occurrences_,fields_,&trusted);
  // Adoption is a separate setup operation; its work gate remains outstanding.
  // Preserve the separate setup copies, but attach their outer stores to the
  // same domain instead of making implicit untracked vector-to-map conversions.
  const ExchangeMap<OccurrenceBinding> staged_rows(occurrences,domain);
  const ExchangeMap<ExchangeFieldBinding> staged_fields(fields,domain);
  need(prepare(projection, std::move(contract), std::move(memberships), staged_rows, staged_fields,
               UINT64_MAX).status == CaptureStatus::Ok, "adoption capture work overflow");
}

CaptureResult ExchangeBindings::prepare(const LetterProjection &projection,
    std::shared_ptr<const LetterExchangeContract> contract,
    std::shared_ptr<const MembershipFields> memberships,
    const ExchangeMap<OccurrenceBinding> &occurrences,
    const ExchangeMap<ExchangeFieldBinding> &fields, std::uint64_t capture_work) {
  PreparationWork setup(UINT64_MAX); // Standalone/adoption setup gate remains separate.
  stage(std::move(contract),std::move(memberships),occurrences,fields,setup);
  return capture(projection,capture_work);
}

void ExchangeBindings::stage(std::shared_ptr<const LetterExchangeContract> contract,
    std::shared_ptr<const MembershipFields> memberships,
    const ExchangeMap<OccurrenceBinding> &occurrences,
    const ExchangeMap<ExchangeFieldBinding> &fields, PreparationWork &meter) {
  valid_ = false;
  meter.charge(1);
  contract_ = std::move(contract);
  memberships_ = std::move(memberships);
  occurrences_.clear(); fields_.clear();
  for (const auto &row : occurrences) {
    meter.charge(sizeof(OccurrenceBinding)); meter.copy_key(row.occurrence);
    occurrences_.push_back(row);
  }
  for (const auto &row : fields) {
    meter.charge(sizeof(ExchangeFieldBinding)); meter.copy_key(row.field.object);
    fields_.push_back(row);
  }
}

CaptureResult ExchangeBindings::capture(const LetterProjection &projection, std::uint64_t allowance) {
  valid_ = false;
  CaptureWork meter(allowance);
  try {
  meter.charge(1); // Fixed capture admission; vector headers only before this.
  masses_.clear(); outgoing_.clear(); edges_.clear();
  const auto &s = h_.prepared_storage();
  need(h_.ready(), "cannot bind an unready exchange owner");
  need(bool(contract_), "missing exchange declaration");
  need(memberships_ && memberships_->alternatives_ && meter.equal(memberships_->study_,projection.study()) &&
       meter.equal(memberships_->recipe_.selection,projection.recipe().ref.selection) &&
       meter.equal(memberships_->recipe_.revision,projection.recipe().ref.revision),
       "exchange membership catalogue belongs to another projection");
  need(meter.equal(contract_->owner().study,projection.study()) &&
       meter.equal(contract_->recipe().selection,projection.recipe().ref.selection) &&
       meter.equal(contract_->recipe().revision,projection.recipe().ref.revision),
       "exchange contract belongs to another projection");
  need(occurrences_.size() == s.live.particles &&fields_.size() <= s.live.fields,
       "exchange binding inventory differs from committed execution");
  need(occurrences_.size() <= masses_.capacity() && s.live.edges <= edges_.capacity() &&
       s.live.edges <= edge_rows_.capacity() && s.particle_headers.size() <= outgoing_rows_.capacity(),
       "exchange binding reservation exceeded");
  std::size_t active = 0;
  for (const auto &o : projection.occurrences()) { meter.charge(1); active += o.active; }
  need(active == occurrences_.size(), "exchange owner exposure is incomplete");
  for (std::size_t i = 0; i < occurrences_.size(); ++i) {
    meter.charge(1);
    const auto &binding = occurrences_[i];
    const auto o = std::find_if(projection.occurrences().begin(), projection.occurrences().end(),
        [&](const auto &v) { return meter.equal(v.key,binding.occurrence); });
    need(o != projection.occurrences().end() && o->active && current(binding.handle, s.particle_headers),
         "exchange occurrence is inactive, foreign or stale");
    for (std::size_t j = 0; j < i; ++j)
      need(!meter.equal(occurrences_[j].occurrence,binding.occurrence) &&
           !same(occurrences_[j].handle, binding.handle), "duplicate exchange occurrence binding");
    const auto tool = std::find_if(projection.recipe().tools.begin(), projection.recipe().tools.end(),
        [&](const auto &v) { return meter.equal(v.ref,o->definition); });
    need(tool != projection.recipe().tools.end() && tool->mass &&
         *tool->mass == s.source_masses[binding.handle.record], "bound exact source mass differs");
    meter.charge(1); masses_.push_back(*tool->mass);
  }
  for (std::size_t i = 0; i < fields_.size(); ++i) {
    meter.charge(1);
    need(current(fields_[i].handle, s.field_headers), "stale exchange field binding");
    for (std::size_t j = 0; j < i; ++j) {
      meter.charge(1);
      need(!(fields_[i].field.kind == fields_[j].field.kind &&
             meter.equal(fields_[i].field.object,fields_[j].field.object)) &&
           !same(fields_[i].handle, fields_[j].handle),
           "aliased exchange field binding");
    }
  }
  meter.charge(s.live.edges); meter.charge(s.particle_headers.size());
  edge_rows_.assign(s.live.edges, false);
  outgoing_rows_.assign(s.particle_headers.size(), false);
  for (const auto &source : contract_->outgoing()) {
    meter.charge(1);
    const auto binding = std::find_if(occurrences_.begin(), occurrences_.end(),
        [&](const auto &v) { return meter.equal(v.occurrence,source.occurrence); });
    need(binding != occurrences_.end(), "outgoing alternative is not currently exposed");
    need(outgoing_.size() < outgoing_.capacity(), "exchange outgoing reservation exceeded");
    outgoing_.push_back(binding->handle);
    outgoing_rows_[binding->handle.record] = true;
    const auto add = [&](GroupKind kind, const warm::ObjectKey &object, std::uint64_t count) {
      meter.charge(1);
      const auto field = std::find_if(fields_.begin(), fields_.end(),
          [&](const auto &v) { meter.charge(1); return v.field.kind == kind && meter.equal(v.field.object,object); });
      need(field != fields_.end(), "missing declared exchange field binding");
      std::size_t row = 0;
      for (; row < s.live.edges; ++row) {
        meter.charge(1);
        if (!edge_rows_[row] && same(s.edges[row].first, binding->handle) &&
            same(s.edges[row].second, field->handle) && s.source_counts[row] == count) break;
      }
      need(row < s.live.edges, "missing exact committed exchange participation");
      need(std::isfinite(s.edges[row].share) && s.edges[row].share > 0,
           "invalid committed exchange share");
      edge_rows_[row] = true;
      meter.charge(1); edges_.push_back({row, s.edges[row], count});
    };
    add(GroupKind::SiblingDefinition, source.definition.object, source.mass);
    for (const auto &parent : source.parents) {
      meter.charge(1);
      add(GroupKind::ParentValue, parent.target.object, parent.contribution);
    }
    for (const auto &membership : memberships_->contributions())
      if (meter.equal(membership.occurrence,source.occurrence))
        add(membership.field.kind, membership.field.object, membership.count);
  }
  need(!outgoing_.empty(), "empty outgoing alternative");
  for (std::size_t row = 0; row < s.live.edges; ++row)
    for (const auto &body : outgoing_) {
      meter.charge(1);
      need(!same(s.edges[row].first, body) || edge_rows_[row], "undeclared outgoing participation");
    }
  generation_ = s.generation;
  source_edges_ = s.live.edges;
  // Bounded read-only validation; reserve before scanning live source metadata.
  const auto terms = occurrences_.size() + fields_.size() + edges_.size() + source_edges_ + 1;
  need(terms <= UINT64_MAX / 16, "exchange binding work overflow");
  work_ = 16 * terms;
  meter.charge(work_); // Reserve the existing complete validation charge before its scan.
  valid_ = true;
  valid_ = validate(generation_, work_).status == field::UpdateStatus::Ok;
  need(valid_,
       "exchange binding changed during preparation");
  return meter.result();
  } catch (const CaptureStop &stop) {
    valid_ = false;
    return stop.result;
  }
}

field::UpdateResult ExchangeBindings::validate(std::uint64_t expected, std::uint64_t work) const {
  field::UpdateResult out;
  const auto &s = h_.prepared_storage();
  out.generation = s.generation;
  if (!h_.ready()) { out.status = field::UpdateStatus::Unready; return out; }
  if (!valid_) return out;
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
