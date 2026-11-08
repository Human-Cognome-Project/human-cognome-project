#include "assembly/exchange_owner.h"
#include <algorithm>
#include <stdexcept>

namespace assembly::detail {
namespace {
void need(bool ok, const char *message) { if (!ok) throw warm::Error(message); }
}
ExchangeOwner::ExchangeOwner(field::Harness &h, LetterProjection projection,
    std::vector<OccurrenceBinding> rows, std::vector<ExchangeFieldBinding> fields)
    : h_(h), projection_(std::make_unique<LetterProjection>(std::move(projection))),
      occurrences_(rows), fields_(fields), generation_(h.prepared_storage().generation) {
  need(h.ready() && projection_->recipe().recipe_version == 2 &&
       !projection_->recipe().selections.empty(), "owner requires a ready structural execution");
  need(fields_.size() == h.prepared_storage().live.fields, "owner field inventory is incomplete");
  memberships_ = std::make_shared<const MembershipFields>(MembershipFields::collect(*projection_,true));
  // Validate each active logical occurrence's exact participation at adoption.
  // Initial identity/layout associations still come from trusted admission claims.
  for (const auto &selection : projection_->recipe().selections) {
    const auto target = projection_->exposure(selection.entry) == warm::Exposure::Aggregate
        ? warm::Exposure::Parts : warm::Exposure::Aggregate;
    auto forward = declare_letter_exchange(*projection_,
        {projection_->study(), {selection.entry,{}}}, target);
    auto reverse = forward;
    std::swap(reverse.from_, reverse.to_);
    reverse.outgoing_.swap(reverse.incoming_);
    auto retained = std::make_shared<const LetterExchangeContract>(std::move(forward));
    ExchangeBindings check(h_, *projection_, retained, memberships_, rows, fields);
    declarations_.push_back(std::move(retained));
    declarations_.push_back(std::make_shared<const LetterExchangeContract>(std::move(reverse)));
  }
  prospective_ = std::make_unique<LetterProjection>(*projection_);
  projection_->reserve_exposure();
  prospective_->reserve_exposure();
  plan_ = std::unique_ptr<ExchangeCoordinator>(new ExchangeCoordinator(h_));
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
