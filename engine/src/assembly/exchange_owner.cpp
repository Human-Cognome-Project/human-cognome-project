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
      occurrences_(std::move(rows)), fields_(std::move(fields)), generation_(h.prepared_storage().generation) {
  need(h.ready() && projection_->recipe().recipe_version == 2 &&
       !projection_->recipe().selections.empty(), "owner requires a ready structural execution");
  need(fields_.size() == h.prepared_storage().live.fields, "owner field inventory is incomplete");
  // Validate each active logical occurrence's exact participation at adoption.
  // Initial identity/layout associations still come from trusted admission claims.
  for (const auto &selection : projection_->recipe().selections) {
    const auto target = projection_->exposure(selection.entry) == warm::Exposure::Aggregate
        ? warm::Exposure::Parts : warm::Exposure::Aggregate;
    ExchangeBindings check(h_, *projection_, declare_letter_exchange(*projection_,
        {projection_->study(), {selection.entry,{}}}, target), occurrences_, fields_);
  }
  prospective_ = std::make_unique<LetterProjection>(*projection_);
  projection_->reserve_exposure();
  prospective_->reserve_exposure();
}

void ExchangeOwner::prepare_projection(const OccurrenceKey &owner, warm::Exposure target) {
  projection_->prepare_exposure(*prospective_,owner,target);
}

ProjectionStorage ExchangeOwner::projection_storage() const {
  auto result = projection_->storage();
  result.add(prospective_->storage());
  return result;
}

void ExchangeOwner::prepare(const OccurrenceKey &owner, warm::Exposure target,
    const std::vector<ExchangeIncoming> &incoming, const std::vector<ExchangeNewField> &fields) {
  // Preparation may allocate/free host metadata. Failure discards pending intent,
  // never the committed projection, mappings or last completed report.
  plan_.reset();
  completed_ = false;
  need(h_.ready() && h_.prepared_storage().generation == generation_, "stale or unready exchange owner");
  auto declaration = declare_letter_exchange(*projection_, owner, target);
  prepare_projection(owner,target);
  auto bound = std::unique_ptr<ExchangeBindings>(new ExchangeBindings(
      h_, *projection_, std::move(declaration), occurrences_, fields_));
  auto plan = std::unique_ptr<ExchangeCoordinator>(new ExchangeCoordinator(std::move(bound), incoming, fields));
  plan_ = std::move(plan);
}

void ExchangeOwner::seal(const field::LiveRefreshSpec &hierarchy) {
  need(bool(plan_) && !completed_, "owner has no pending exchange");
  plan_->prepare_hierarchy(hierarchy);
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
