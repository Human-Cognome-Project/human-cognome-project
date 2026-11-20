#include "assembly/projection_access.h"
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

ExchangeBindings::ExchangeBindings(field::Harness &h, const std::shared_ptr<ReservationDomain> &domain, PreparationWork *meter)
    : h_(h), occurrences_(domain), fields_(domain), masses_(ReservationAllocator<std::uint64_t>(domain.get())),
      outgoing_(ReservationAllocator<engine::RecordHandle>(domain.get())), edges_(ReservationAllocator<Edge>(domain.get())),
      outgoing_rows_(ReservationAllocator<bool>(domain.get())), edge_rows_(ReservationAllocator<bool>(domain.get())) {
  const auto &s = h.prepared_storage();
  const auto &limits = s.capacity.limits();
  if (meter) meter->elements(limits[field::Budget::Particles],sizeof(masses_[0]));
  masses_.reserve(limits[field::Budget::Particles]);
  if (meter) meter->elements(limits[field::Budget::Particles],sizeof(outgoing_[0]));
  outgoing_.reserve(limits[field::Budget::Particles]);
  if (meter) meter->elements(limits[field::Budget::Participation],sizeof(edges_[0]));
  edges_.reserve(limits[field::Budget::Participation]);
  // Conservatively admit one byte per bit plus a rounding word before packed
  // mask backing is allocated/initialized; do not depend on vector<bool> layout.
  if (meter) { meter->elements(s.particle_headers.size(),1); meter->charge(sizeof(std::uintmax_t)); }
  outgoing_rows_.reserve(s.particle_headers.size());
  if (meter) { meter->elements(limits[field::Budget::Participation],1); meter->charge(sizeof(std::uintmax_t)); }
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
    const std::vector<ExchangeFieldBinding> &fields, const std::shared_ptr<ReservationDomain> &domain, PreparationWork *meter)
    : ExchangeBindings(h,domain,meter) {
  const auto &limits = h.prepared_storage().capacity.limits();
  PreparationWork standalone(UINT64_MAX); // Standalone adoption remains explicitly unrestricted.
  auto &setup = meter ? *meter : standalone;
  const ExchangeMap<ExchangeFieldBinding> trusted(fields,domain,&setup);
  reserve_exchange_maps(projection,limits[field::Budget::Particles],limits[field::Budget::Fields],limits[field::Budget::Batch],occurrences_,fields_,&trusted,&setup);
  const ExchangeMap<OccurrenceBinding> staged_rows(occurrences,domain,&setup);
  const ExchangeMap<ExchangeFieldBinding> staged_fields(fields,domain,&setup);
  const auto target = contract ? contract->to() : warm::Exposure{};
  stage(std::move(contract),target,std::move(memberships),staged_rows,staged_fields,setup);
  // Setup refusal must unwind the constructor to the owner boundary, not become
  // a fresh operational capture allowance or a swallowed partial result.
  capture(projection,setup);
}

CaptureResult ExchangeBindings::prepare(const LetterProjection &projection,
    std::shared_ptr<const LetterExchangeContract> contract,
    std::shared_ptr<const MembershipFields> memberships,
    const ExchangeMap<OccurrenceBinding> &occurrences,
    const ExchangeMap<ExchangeFieldBinding> &fields, std::uint64_t capture_work) {
  PreparationWork setup(UINT64_MAX); // Standalone wrapper; owner adoption uses its shared meter.
  const auto target = contract ? contract->to() : warm::Exposure{};
  stage(std::move(contract),target,std::move(memberships),occurrences,fields,setup);
  return capture(projection,capture_work);
}

ExchangeBindings::ExchangeBindings(field::Harness &h, const LetterProjection &projection,
    std::shared_ptr<const LetterExchangeContract> contract,
    std::shared_ptr<const MembershipFields> memberships,
    const ExchangeMap<OccurrenceBinding> &occurrences, const ExchangeMap<ExchangeFieldBinding> &fields,
    const std::shared_ptr<ReservationDomain> &domain, PreparationWork &setup)
    : ExchangeBindings(h,domain,&setup) {
  const auto target=contract ? contract->to() : warm::Exposure{};
  capture_adoption(projection,std::move(contract),target,std::move(memberships),occurrences,fields,setup);
}

CaptureResult ExchangeBindings::capture_adoption(const LetterProjection &projection,
    std::shared_ptr<const LetterExchangeContract> contract, warm::Exposure target,
    std::shared_ptr<const MembershipFields> memberships,
    const ExchangeMap<OccurrenceBinding> &occurrences, const ExchangeMap<ExchangeFieldBinding> &fields,
    PreparationWork &setup) {
  valid_ = false; // Includes checked arithmetic/inventory refusals before stage.
  const auto &limits=h_.prepared_storage().capacity.limits();
  // Preserve the removed reservation's checked sums and inventory bounds.
  const auto rows=exchange_map_capacity(limits[field::Budget::Particles],limits[field::Budget::Batch]);
  const auto groups=exchange_map_capacity(limits[field::Budget::Fields],
      std::min(limits[field::Budget::Fields],limits[field::Budget::Batch]));
  need(occurrences.size()<=rows && fields.size()<=groups,"exchange adoption inventory exceeds map bounds");
  stage(std::move(contract),target,std::move(memberships),setup);
  return capture(projection,occurrences,fields,setup);
}

void ExchangeBindings::stage(std::shared_ptr<const LetterExchangeContract> contract,
    warm::Exposure target, std::shared_ptr<const MembershipFields> memberships,
    const ExchangeMap<OccurrenceBinding> &occurrences,
    const ExchangeMap<ExchangeFieldBinding> &fields, PreparationWork &meter) {
  stage(std::move(contract),target,std::move(memberships),meter);
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

void ExchangeBindings::stage(std::shared_ptr<const LetterExchangeContract> contract,
    warm::Exposure target, std::shared_ptr<const MembershipFields> memberships, PreparationWork &meter) {
  valid_ = false;
  meter.charge(1);
  need(contract && (target == contract->from() || target == contract->to()) &&
       contract->from() != contract->to(), "invalid exchange operation direction");
  contract_ = std::move(contract);
  target_ = target;
  memberships_ = std::move(memberships);
}

CaptureResult ExchangeBindings::capture(const LetterProjection &projection, std::uint64_t allowance) {
  return capture(projection,occurrences_,fields_,allowance);
}

CaptureResult ExchangeBindings::capture(const LetterProjection &projection, PreparationWork &meter) {
  return capture(projection,occurrences_,fields_,meter);
}

CaptureResult ExchangeBindings::capture(const LetterProjection &projection,
    const ExchangeMap<OccurrenceBinding> &current_occurrences,
    const ExchangeMap<ExchangeFieldBinding> &current_fields, std::uint64_t allowance) {
  CaptureWork meter(allowance);
  try { return capture(projection,current_occurrences,current_fields,meter); }
  catch (const CaptureStop &stop) {
    valid_ = false;
    return stop.result;
  }
}

CaptureResult ExchangeBindings::capture(const LetterProjection &projection,
    const ExchangeMap<OccurrenceBinding> &current_occurrences,
    const ExchangeMap<ExchangeFieldBinding> &current_fields, PreparationWork &meter) {
  valid_ = false;
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
  meter.elements(projection.recipe().selections.size(),1);
  meter.charge(1);
  const auto source_exposure = target_ == contract_->to() ? contract_->from() : contract_->to();
  need(projection.exposure(contract_->owner().source.entry) == source_exposure,
       "exchange source exposure differs from committed projection");
  need(current_occurrences.size() == s.live.particles && current_fields.size() <= s.live.fields,
       "exchange binding inventory differs from committed execution");
  need(current_occurrences.size() <= masses_.capacity() && s.live.edges <= edges_.capacity() &&
       s.live.edges <= edge_rows_.capacity() && s.particle_headers.size() <= outgoing_rows_.capacity(),
       "exchange binding reservation exceeded");
  std::size_t active = 0;
  for (const auto &o : ProjectionAccess::rows(projection)) { meter.charge(1); active += o.active; }
  need(active == current_occurrences.size(), "exchange owner exposure is incomplete");
  for (std::size_t i = 0; i < current_occurrences.size(); ++i) {
    meter.charge(1);
    const auto &binding = current_occurrences[i];
    const auto o = std::find_if(ProjectionAccess::rows(projection).begin(), ProjectionAccess::rows(projection).end(),
        [&](const auto &v) { return meter.equal(v.key,binding.occurrence); });
    need(o != ProjectionAccess::rows(projection).end() && o->active && current(binding.handle, s.particle_headers),
         "exchange occurrence is inactive, foreign or stale");
    for (std::size_t j = 0; j < i; ++j)
      need(!meter.equal(current_occurrences[j].occurrence,binding.occurrence) &&
           !same(current_occurrences[j].handle, binding.handle), "duplicate exchange occurrence binding");
    const auto *tool = &ProjectionAccess::tool(projection,*o,&meter);
    need(tool->mass &&
         *tool->mass == s.source_masses[binding.handle.record], "bound exact source mass differs");
    meter.charge(1); masses_.push_back(*tool->mass);
  }
  for (std::size_t i = 0; i < current_fields.size(); ++i) {
    meter.charge(1);
    need(current(current_fields[i].handle, s.field_headers), "stale exchange field binding");
    for (std::size_t j = 0; j < i; ++j) {
      meter.charge(1);
      need(!(current_fields[i].field.kind == current_fields[j].field.kind &&
             meter.equal(current_fields[i].field.object,current_fields[j].field.object)) &&
           !same(current_fields[i].handle, current_fields[j].handle),
           "aliased exchange field binding");
    }
  }
  meter.charge(s.live.edges); meter.charge(s.particle_headers.size());
  edge_rows_.assign(s.live.edges, false);
  outgoing_rows_.assign(s.particle_headers.size(), false);
  for (const auto &source : outgoing_sources()) {
    meter.charge(1);
    const auto binding = std::find_if(current_occurrences.begin(), current_occurrences.end(),
        [&](const auto &v) { return meter.equal(v.occurrence,source.occurrence); });
    need(binding != current_occurrences.end(), "outgoing alternative is not currently exposed");
    need(outgoing_.size() < outgoing_.capacity(), "exchange outgoing reservation exceeded");
    outgoing_.push_back(binding->handle);
    outgoing_rows_[binding->handle.record] = true;
    const auto add = [&](GroupKind kind, const warm::ObjectKey &object, std::uint64_t count) {
      meter.charge(1);
      const auto field = std::find_if(current_fields.begin(), current_fields.end(),
          [&](const auto &v) { meter.charge(1); return v.field.kind == kind && meter.equal(v.field.object,object); });
      need(field != current_fields.end(), "missing declared exchange field binding");
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
    for (const auto &parent : contract_->parents(source,&meter)) {
      meter.charge(1);
      add(GroupKind::ParentValue, parent.target.object, parent.contribution);
    }
    for (const auto &membership : memberships_->contributions_) {
      const auto &target=memberships_->target(membership,&meter);
      if (meter.equal(membership.occurrence,source.occurrence.source)) {
        meter.charge(1);
        need(membership.kind == GroupKind::Membership,"invalid membership field kind");
        add(membership.kind, target.object, membership.count);
      }
    }
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
  const auto terms = current_occurrences.size() + current_fields.size() + edges_.size() + source_edges_ + 1;
  need(terms <= UINT64_MAX / 16, "exchange binding work overflow");
  work_ = 16 * terms;
  meter.charge(work_); // Reserve the existing complete validation charge before its scan.
  valid_ = true;
  valid_ = validate(generation_,work_,current_occurrences,current_fields).status == field::UpdateStatus::Ok;
  need(valid_,
       "exchange binding changed during preparation");
  return meter.result();
}

field::UpdateResult ExchangeBindings::validate(std::uint64_t expected, std::uint64_t work) const {
  return validate(expected,work,occurrences_,fields_);
}

field::UpdateResult ExchangeBindings::validate(std::uint64_t expected, std::uint64_t work,
    const ExchangeMap<OccurrenceBinding> &current_occurrences,
    const ExchangeMap<ExchangeFieldBinding> &current_fields) const {
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
  if (s.live.particles != current_occurrences.size() || masses_.size() != current_occurrences.size() ||
      s.live.edges != source_edges_) return out;
  for (std::size_t i = 0; i < current_occurrences.size(); ++i) {
    const auto handle = current_occurrences[i].handle;
    if (!current(handle, s.particle_headers)) { out.status = field::UpdateStatus::Stale; return out; }
    if (s.source_masses[handle.record] != masses_[i]) return out;
  }
  for (const auto &f : current_fields)
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
