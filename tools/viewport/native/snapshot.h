#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

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
// A value copied from one completed, exclusively owned execution boundary.
// No backend selection, engine construction, replay or numerical solve occurs.
struct CompletedState {
  assembly::ActiveReport report;
  std::string backend;
  std::uint64_t tick{};
  std::optional<std::uint64_t> prepared_generation;
  std::vector<engine::RecordHandle> body_records;
  std::vector<field::CascadeNode> comparisons;
  double observation_ms{};
  std::size_t numeric_copy_bytes{};
  // Requested readback API payloads, not measured PCIe/driver traffic.
  std::size_t requested_readback_bytes{};
};
struct HarnessBinding {
  assembly::ActiveReport mapping;
  std::uint64_t generation{};
  bool installed{};
  std::vector<engine::RecordHandle> bodies, fields;
};
// The run owner supplies identity/source/scale metadata matching present native
// slots. Binding rejects partial coverage; later reads reject recycled/stale maps.
HarnessBinding bind(field::Harness &execution, assembly::ActiveReport mapping);
CompletedState observe(field::Harness &execution, const HarnessBinding &binding);
CompletedState observe(assembly::ActiveSession &session, const warm::Uuid &owner,
                       const assembly::ExecutionHandle &handle);
Json snapshot(const CompletedState &state, const CaptureContext &capture);
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
