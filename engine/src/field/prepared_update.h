#pragma once

#include <optional>

#include "engine/record_map.h"
#include "field/prepared_capacity.h"

namespace field {
// Internal, exclusively owned, synchronized execution boundary. Definition
// identities belong to the source adapter; these identify live occurrences.
enum class UpdateOp : std::int32_t {
  DefineParticle,
  DefineField,
  RetireParticle,
  RetireField,
  InsertParticipation,
  ReplaceParticipation,
  RemoveParticipation,
  InsertContact,
  RemoveContact,
  ReplaceFocus
};
struct UpdateRef {
  engine::RecordHandle handle;
  std::int32_t command{-1};  // >=0 refers to a definition in this batch
  static UpdateRef existing(engine::RecordHandle h) {
    return {h, -1};
  }
  static UpdateRef pending(int command) {
    return {{}, command};
  }
};
struct UpdateCommand {
  UpdateOp operation{};
  std::int32_t kind{1}, address{-1};
  UpdateRef first, second;
  std::uint64_t row{}, generation{}, ordinal{};
  // Insert positions are indexes in the final ordered ledger. Existing rows
  // retain their relative order; two insertions cannot occupy one position.
  std::uint64_t position{};
  float share{1};
  // Source provenance only; never a numerical mass override. Nullopt means
  // unavailable, while an explicitly supplied zero is invalid.
  std::optional<std::uint64_t> exact_mass;   // DefineParticle only
  std::optional<std::uint64_t> exact_count;  // Insert/ReplaceParticipation only
};
enum class UpdateStatus {
  Ok,
  NoChange,
  InvalidInput,
  Stale,
  CapacityExhausted,
  Unsupported,
  InvalidNumerics,
  Unready
};
struct UpdateResult {
  UpdateStatus status{UpdateStatus::InvalidInput};
  BudgetFailure failure{};
  std::uint64_t generation{};
  std::uint64_t structural_visits{}, index_visits{}, upload_bytes{},
      readback_bytes{}, synchronizations{}, collections{};
};
}  // namespace field
