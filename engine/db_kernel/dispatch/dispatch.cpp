#include "dispatch.h"

namespace dispatch {

Result dispatch_one(dbk::Controller &ctl, const Command &cmd) {
  if (const auto *node = std::get_if<command::DeclareRecord>(&cmd)) {
    return Result{Verb::kDeclare, declare::execute(ctl, *node)};
  }
  if (const auto *op = std::get_if<command::ReadRecord>(&cmd)) {
    return Result{Verb::kRead, dbread::read(ctl, *op)};
  }
  if (const auto *op = std::get_if<command::MoveRecord>(&cmd)) {
    return Result{Verb::kMove, update::move_record(ctl, *op)};
  }
  if (const auto *op = std::get_if<command::AddConnection>(&cmd)) {
    return Result{Verb::kAddConnection, update::add_connection(ctl, *op)};
  }
  if (const auto *req = std::get_if<DeleteRecordRequest>(&cmd)) {
    return Result{Verb::kDeleteRecord,
                  update::delete_record(ctl, req->op, req->confirm)};
  }
  if (const auto *req = std::get_if<DeleteConnectionRequest>(&cmd)) {
    return Result{Verb::kDeleteConnection,
                  update::delete_connection(ctl, req->op, req->confirm)};
  }
  if (std::holds_alternative<Reconcile>(cmd)) {
    return Result{Verb::kReconcile, std::string("RECONCILE: not yet implemented")};
  }
  if (std::holds_alternative<UpdateCache>(cmd)) {
    return Result{Verb::kUpdateCache, std::string("UPDATE_CACHE: not yet implemented")};
  }
  // The only remaining alternative: RebaseCache.
  return Result{Verb::kRebaseCache, std::string("REBASE_CACHE: not yet implemented")};
}

std::vector<Result> dispatch_stream(dbk::Controller &ctl,
                                     const std::vector<AdditiveCommand> &stream) {
  std::vector<Result> results;
  results.reserve(stream.size());
  for (const AdditiveCommand &item : stream) {
    // Re-wrap the additive alternative into the full Command variant so
    // one dispatch_one implementation serves both surfaces -- no
    // duplicated per-verb switch. Calling dispatch_one once per item, in
    // stream order, against the same controller, IS "executed IN ORDER"
    // (NOTES.md "Arraying is universal"): synchronous, single-threaded,
    // nothing here reorders, batches, or parallelizes.
    Command wrapped = std::visit([](const auto &v) -> Command { return v; }, item);
    results.push_back(dispatch_one(ctl, wrapped));
  }
  return results;
}

}  // namespace dispatch
