#pragma once
#include "assembly/preparation_work.h"

namespace assembly::detail {
// Input snapshot admission only, not the complete constructor allocation peak.
struct AdoptionInputBudget {
  std::uint64_t bytes, inspection_work, copy_work;
};
enum class AdoptionInputStatus { Ok, WorkExhausted, ByteExhausted, Overflow };
enum class AdoptionInputPhase { Inspection, Copy, Complete };
struct AdoptionInputResult {
  AdoptionInputStatus status{AdoptionInputStatus::Ok};
  AdoptionInputPhase phase{AdoptionInputPhase::Inspection};
  PreparationResult inspection, copy;
  std::uint64_t caller_bytes{}, snapshot_bytes{}, required_bytes{};
};
// A logical snapshot uses live sizes; the caller census includes capacity slack.
// Inline strings are conservatively charged again including a terminator.
inline void input_add(std::uint64_t &to, std::uint64_t n, PreparationWork &meter) {
  if (n > UINT64_MAX-to)
    throw PreparationStop{{PreparationStatus::Overflow,meter.result().charged,0}};
  to += n;
}
inline void input_elements(std::uint64_t &to, std::uint64_t n, std::uint64_t size,
                           PreparationWork &meter) {
  if (n > UINT64_MAX/size)
    throw PreparationStop{{PreparationStatus::Overflow,meter.result().charged,0}};
  input_add(to,n*size,meter);
}
}
