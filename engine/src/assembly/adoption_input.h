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
  // Work components replace map snapshot work; byte census remains independent.
  std::uint64_t projection_copy_work{}, occurrence_copy_work{}, field_copy_work{}, required_copy_work{};
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
// Fresh ExchangeMap only: resize admission, reserve, nil-slot initialization and
// assignment. Each canonical UUID contributes 36 characters plus terminator.
inline std::uint64_t fresh_map_copy_work(std::uint64_t count, std::uint64_t row_bytes,
    std::uint64_t payload_bytes, PreparationWork &inspection) {
  std::uint64_t result=1;
  for (int i=0; i<3; ++i) input_elements(result,count,row_bytes,inspection);
  input_elements(result,count,74,inspection);
  input_add(result,payload_bytes,inspection);
  return result;
}

}
