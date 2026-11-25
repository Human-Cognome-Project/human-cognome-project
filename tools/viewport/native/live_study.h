#pragma once

#include "assembly/prepared_submission.h"
#include "snapshot.h"
#include <iosfwd>

namespace viewport {
struct LiveStudyInput {
  std::istream &commands;
  std::ostream &output;
  engine::InstanceSettings settings;
  field::HostPath path;
  std::optional<ResponseRequest> responses;
};

int run_live_study(const LiveStudyInput &, assembly::PreparedStudySubmission,
                   const std::string &label, const Json &source_metadata,
                   double physical_mass, std::uint64_t source_mass,
                   bool stop_on_signal = false);
int run_retained_live_study(const LiveStudyInput &);
}  // namespace viewport
