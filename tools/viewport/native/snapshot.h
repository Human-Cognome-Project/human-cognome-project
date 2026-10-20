#pragma once

#include <map>
#include <string>

#include "assembly/active_workbench.h"
#include "json.hpp"

namespace viewport {
using Json = nlohmann::json;
struct DisplayContext {
  std::string exposure, aggregate, label;
};
// Supplied by the synchronous native capture owner. No source lookup here.
struct CaptureContext {
  assembly::ExecutionHandle handle;
  std::uint64_t tick{0};
  std::string label;
  std::map<std::string, DisplayContext> occurrences;
  Json source = Json::object();
};
CaptureContext context(const assembly::ActiveAssembly &assembly,
                       const assembly::ExecutionHandle &handle,
                       std::uint64_t completed_ticks, std::string label);
// Pure value conversion, also used for fidelity checks. The embedding owner
// must obtain the report through ActiveSession::report, not manager reporting.
Json snapshot(const assembly::ActiveReport &report, const CaptureContext &capture);
// The operational entry point always obtains the owner-checked current report.
Json capture(assembly::ActiveSession &session, const warm::Uuid &owner,
             const CaptureContext &context);
void write(const std::string &path, const Json &snapshot);
// Readers of a watched path never see a partial file: the content is written to a
// hidden, uniquely named temporary in the same directory, closed, then renamed over the target.
void write_atomic(const std::string &path, const Json &snapshot);
}  // namespace viewport
