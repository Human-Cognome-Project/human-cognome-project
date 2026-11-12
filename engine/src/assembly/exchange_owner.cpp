#include "assembly/exchange_owner.h"
#include <algorithm>
#include <stdexcept>

namespace assembly::detail {
namespace {
void need(bool ok, const char *message) { if (!ok) throw warm::Error(message); }
}
AdoptionInputResult ExchangeOwner::input_admission(const LetterProjection &projection,
    const std::vector<OccurrenceBinding> &rows, const std::vector<ExchangeFieldBinding> &fields,
    AdoptionInputBudget budget) {
  AdoptionInputResult result;
  PreparationWork inspection(budget.inspection_work), copy(budget.copy_work);
  try {
    inspection.charge(1);
    const auto map = [&](const auto &values, bool logical, const auto &uuid) {
      inspection.charge(1); inspection.elements(values.size(),1);
      std::uint64_t bytes = sizeof(values);
      input_elements(bytes,logical ? values.size() : values.capacity(),sizeof(values[0]),inspection);
      for (const auto &row : values) {
        inspection.charge(1);
        const auto &text = uuid(row).str();
        input_add(bytes,logical ? text.size() : text.capacity(),inspection);
        input_add(bytes,1,inspection);
        inspection.charge(1);
        const auto &key = key_buffer(row);
        input_elements(bytes,logical ? key.size() : key.capacity(),sizeof(key[0]),inspection);
      }
      return bytes;
    };
    const auto occurrence_uuid = [](const auto &row) -> const auto & { return row.occurrence.study; };
    const auto field_uuid = [](const auto &row) -> const auto & { return row.field.object.scope; };
    auto caller = projection.storage(&inspection,false).charged_bytes();
    input_add(caller,map(rows,false,occurrence_uuid),inspection);
    input_add(caller,map(fields,false,field_uuid),inspection);
    auto snapshot = projection.storage(&inspection,true).charged_bytes();
    input_add(snapshot,map(rows,true,occurrence_uuid),inspection);
    input_add(snapshot,map(fields,true,field_uuid),inspection);
    auto total = caller; input_add(total,snapshot,inspection);
    result.inspection = inspection.result();
    result.caller_bytes = caller; result.snapshot_bytes = snapshot; result.required_bytes = total;
    result.phase = AdoptionInputPhase::Copy;
    if (total > budget.bytes) { result.status = AdoptionInputStatus::ByteExhausted; return result; }
    copy.charge(snapshot);
    result.copy = copy.result();
    result.phase = AdoptionInputPhase::Complete;
    return result;
  } catch (const PreparationStop &stop) {
    result.status = stop.result.status == PreparationStatus::Overflow
        ? AdoptionInputStatus::Overflow : AdoptionInputStatus::WorkExhausted;
    if (result.phase == AdoptionInputPhase::Inspection) result.inspection = stop.result;
    else result.copy = stop.result;
    return result;
  }
}

ReservedUnique<ExchangeOwner> ExchangeOwner::adopt(field::Harness &h,
    const LetterProjection &projection, const std::vector<OccurrenceBinding> &rows,
    const std::vector<ExchangeFieldBinding> &fields, AdoptionInputBudget budget,
    AdoptionInputResult &result, ReservationBudget budgeted_buffers, ReservationResult &reservation,
    std::uint64_t projection_setup_work, ProjectionSetupResult &setup) {
  // References cross this gate. No by-value argument copy or owner allocation
  // may precede it. Later constructor setup remains a separately scoped domain.
  reservation = {};
  setup = {};
  result = input_admission(projection,rows,fields,budget);
  if (result.status != AdoptionInputStatus::Ok) return {};
  PreparationWork meter(projection_setup_work);
  try { meter.charge(1); }
  catch (const PreparationStop &stop) { setup.work = stop.result; return {}; }
  // The domain and its own control block are separate overhead. Shared data
  // allocators keep it alive through their final strong/weak reference release.
  auto domain = std::make_shared<ReservationDomain>(budgeted_buffers);
  try {
    auto owner = reserve_unique<ExchangeOwner>(domain,[&](auto *p) {
      ::new (static_cast<void *>(p)) ExchangeOwner(h,projection,rows,fields,domain,meter,setup);
    });
    reservation = domain->result();
    owner->reservations_ = std::move(domain);
    setup.phase = ProjectionSetupPhase::Complete;
    setup.work = meter.result();
    return owner;
  } catch (const PreparationStop &stop) {
    setup.work = stop.result;
    reservation = domain->result();
    return {};
  } catch (const ReservationStop &) {
    // Constructor members/temporaries have unwound while the domain stays alive.
    reservation = domain->result();
    setup.work = meter.result();
    return {};
  }
}

ExchangeOwner::ExchangeOwner(field::Harness &h, const LetterProjection &projection,
    const std::vector<OccurrenceBinding> &rows, const std::vector<ExchangeFieldBinding> &fields,
    const std::shared_ptr<ReservationDomain> &domain,
    PreparationWork &meter, ProjectionSetupResult &setup)
    : h_(h), projection_(reserve_unique<LetterProjection>(domain,[&](auto *p) {
        ::new (static_cast<void *>(p)) LetterProjection(projection,domain);
      })),
      occurrences_(rows,domain), fields_(fields,domain),
      declarations_(SharedReservationAllocator<std::shared_ptr<const LetterExchangeContract>>(domain)),
      generation_(h.prepared_storage().generation) {
  need(h.ready() && projection_->recipe().recipe_version == 2 &&
       !projection_->recipe().selections.empty(), "owner requires a ready structural execution");
  need(fields_.size() == h.prepared_storage().live.fields, "owner field inventory is incomplete");
  setup.phase = ProjectionSetupPhase::Membership;
  memberships_ = std::allocate_shared<const MembershipFields>(
      SharedReservationAllocator<MembershipFields>(domain),MembershipFields::collect(*projection_,true,domain,&meter));
  // Validate each active logical occurrence's exact participation at adoption.
  // Initial identity/layout associations still come from trusted admission claims.
  for (const auto &selection : projection_->recipe().selections) {
    const auto target = projection_->exposure(selection.entry) == warm::Exposure::Aggregate
        ? warm::Exposure::Parts : warm::Exposure::Aggregate;
    auto forward = LetterExchangeContract::collect(*projection_,
        {warm::Uuid(projection_->study(),domain), {selection.entry,{}}}, target,domain);
    auto reverse = LetterExchangeContract(forward,domain);
    std::swap(reverse.from_, reverse.to_);
    reverse.outgoing_.swap(reverse.incoming_);
    auto retained = std::allocate_shared<const LetterExchangeContract>(
        SharedReservationAllocator<LetterExchangeContract>(domain),std::move(forward));
    ExchangeBindings check(h_, *projection_, retained, memberships_, rows, fields,domain);
    declarations_.push_back(std::move(retained));
    declarations_.push_back(std::allocate_shared<const LetterExchangeContract>(
        SharedReservationAllocator<LetterExchangeContract>(domain),std::move(reverse)));
  }
  setup.phase = ProjectionSetupPhase::SpareInspection;
  const auto copy_bytes = projection_->storage(&meter,true).charged_bytes();
  setup.phase = ProjectionSetupPhase::SpareCopy;
  meter.charge(copy_bytes);
  prospective_ = reserve_unique<LetterProjection>(domain,[&](auto *p) {
    ::new (static_cast<void *>(p)) LetterProjection(*projection_,domain);
  });
  setup.phase = ProjectionSetupPhase::CommittedParents;
  projection_->reserve_exposure(meter);
  setup.phase = ProjectionSetupPhase::SpareParents;
  prospective_->reserve_exposure(meter);
  plan_ = reserve_unique<ExchangeCoordinator>(domain,[&](auto *p) {
    ::new (static_cast<void *>(p)) ExchangeCoordinator(h_,domain);
  });
  const auto &limits = h_.prepared_storage().capacity.limits();
  reserve_exchange_maps(*projection_,limits[field::Budget::Particles],limits[field::Budget::Fields],limits[field::Budget::Batch],occurrences_,fields_);
  reserve_exchange_maps(*projection_,limits[field::Budget::Particles],limits[field::Budget::Fields],limits[field::Budget::Batch],plan_->bound_->occurrences_,plan_->bound_->fields_,&fields_);
  reserve_exchange_maps(*projection_,limits[field::Budget::Particles],limits[field::Budget::Fields],limits[field::Budget::Batch],plan_->next_,plan_->next_fields_,&fields_);
  completed_ = true;
}

void ExchangeOwner::prepare_projection(const OccurrenceKey &owner, warm::Exposure target, PreparationWork &meter) {
  projection_->prepare_exposure(*prospective_,owner,target,meter);
}

std::shared_ptr<const LetterExchangeContract> ExchangeOwner::declaration(
    const OccurrenceKey &owner, warm::Exposure target, PreparationWork &meter) const {
  for (const auto &entry : declarations_)
    if (meter.equal(entry->owner(),owner) && entry->to() == target) {
      bool current = false;
      for (std::size_t i = 0; i < projection_->recipe_.selections.size(); ++i) {
        meter.charge(1);
        if (projection_->recipe_.selections[i].entry == owner.source.entry) {
          current = entry->from() == projection_->exposures_[i]; break;
        }
      }
      need(current,
           "exchange requires the other declared alternative");
      return entry;
    }
  throw warm::Error("exchange occurrence or alternative is not declared by this owner");
}

ProjectionStorage ExchangeOwner::projection_storage() const {
  auto result = projection_->storage();
  result.add(prospective_->storage());
  return result;
}

OwnerPreparationResult ExchangeOwner::prepare(const OccurrenceKey &owner, warm::Exposure target,
    const std::vector<ExchangeIncoming> &incoming, const std::vector<ExchangeNewField> &fields,
    std::uint64_t staging_work, std::uint64_t capture_work, std::uint64_t command_work) {
  // Preparation reuses reserved host metadata. Failure discards pending intent,
  // never the committed projection, mappings or last completed report.
  completed_ = true;
  plan_->consumed_ = true;
  plan_->sealed_ = false;
  plan_->bound_->valid_ = false;
  PreparationWork meter(staging_work);
  try {
  meter.charge(1);
  need(h_.ready() && h_.prepared_storage().generation == generation_, "stale or unready exchange owner");
  auto selected = declaration(owner, target,meter);
  prepare_projection(owner,target,meter);
  plan_->bound_->stage(std::move(selected),memberships_,occurrences_,fields_,meter);
  const auto staged = meter.result();
  const auto captured = plan_->bound_->capture(*projection_,capture_work);
  if (captured.status != CaptureStatus::Ok)
    return {captured.status,PreparationPhase::Capture,staged,captured,{}};
  const auto commands = plan_->prepare(incoming, fields,command_work);
  if (commands.status != PreparationStatus::Ok)
    return {commands.status,PreparationPhase::Commands,staged,captured,commands};
  completed_ = false;
  return {PreparationStatus::Ok,PreparationPhase::Complete,staged,captured,commands};
  } catch (const PreparationStop &stop) {
    return {stop.result.status,PreparationPhase::Staging,stop.result,{},{}};
  }
}

HierarchySealResult ExchangeOwner::seal(const field::LiveRefreshSpec &hierarchy,
    HierarchySealBudget budget) {
  need(bool(plan_) && !completed_, "owner has no pending exchange");
  return plan_->prepare_hierarchy(hierarchy,budget);
}

field::UpdateResult ExchangeOwner::execute(std::uint64_t expected, std::uint64_t work,
    std::uint64_t cells, std::uint64_t hierarchy_work) {
  field::UpdateResult result;
  result.generation = h_.prepared_storage().generation;
  if (!h_.ready()) { result.status = field::UpdateStatus::Unready; return result; }
  if (expected != generation_ || result.generation != generation_) {
    result.status = field::UpdateStatus::Stale; return result;
  }
  if (!plan_ || !prospective_ || completed_) return result;
  work = std::min(work, h_.prepared_storage().capacity.limits()[field::Budget::PlacementWork]);
  if (work < 64) {
    result.status = field::UpdateStatus::CapacityExhausted;
    result.failure = {field::Budget::PlacementWork,64,work}; return result;
  }
  try {
    result = plan_->execute(expected, work - 64, cells, hierarchy_work);
    result.structural_visits += 64;
    if (result.status != field::UpdateStatus::Ok) return result;
    if (h_.prepared_storage().failure_point == 8)
      throw std::runtime_error("injected failure before owner exposure publication");
    occurrences_.swap(plan_->bound_->occurrences_);
    fields_.swap(plan_->bound_->fields_);
    projection_.swap(prospective_);
    outgoing_report_ = plan_->outgoing_report_;
    balance_report_ = plan_->balance_report_;
    generation_ = result.generation;
    completed_ = true;
    return result;
  } catch (...) {
    h_.ready_ = false;
    throw;
  }
}
}
