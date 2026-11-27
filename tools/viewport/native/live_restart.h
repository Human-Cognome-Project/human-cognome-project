#pragma once

#include "live_study.h"

namespace viewport {
struct RetainedRestart {
  warm::Uuid study;
  warm::BundleRef catalogue, recipe;
  std::uint64_t tick{};
  double dt{field::Parameters64{}.dt};
  warm::Exposure exposure{};
  std::vector<std::string> letters;
  std::optional<std::string> focus_byte;
  Json structure, revisions;
  assembly::PreparedRestart state;
};

RetainedRestart read_retained_restart(const std::string &path);
void verify_retained_restart(const RetainedRestart &, const warm::Bundle &);
void rebind_retained_revisions(const RetainedRestart &, warm::Bundle &);
void save_retained_restart(const std::string &path, const CompletedState &,
                           const warm::Bundle &, const warm::BundleRef &catalogue, double dt);
assembly::PreparedStudySubmission retained_restart_submission(
    std::shared_ptr<const assembly::ActiveAssembly>, const RetainedRestart &, field::HostPath);
}
