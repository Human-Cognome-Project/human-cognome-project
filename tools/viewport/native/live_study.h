#pragma once

#include "assembly/prepared_submission.h"
#include "live_study_io.h"
#include "snapshot.h"
#include <iosfwd>

namespace viewport {
struct LiveStudyInput {
  LiveCommandInput &commands;
  std::ostream &output;
  engine::InstanceSettings settings;
  field::HostPath path;
  std::optional<ResponseRequest> responses;
};

void write_live_event(const LiveStudyInput &, const Json &);
int run_live_study(const LiveStudyInput &, assembly::PreparedStudySubmission,
                   const std::string &label, const Json &source_metadata,
                   double physical_mass, std::uint64_t source_mass);
int run_retained_live_study(const LiveStudyInput &);
}  // namespace viewport
