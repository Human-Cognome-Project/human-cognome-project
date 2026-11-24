#pragma once

#include <map>
#include <optional>

#include "assembly/active_assembly.h"
#include "assembly/study.h"

namespace viewport { struct ActiveSessionAccess; }
namespace assembly {
struct PreparedOccurrenceEdit;
struct PreparedOccurrenceEditResult;
namespace detail { class PreparedStudyOwner; }
struct PreparedStudySubmission;
struct PreparedSubmissionResult;
struct ExecutionHandle {
  warm::Uuid session, study, generation;
};
struct ActiveReport {
  ExecutionHandle handle;
  warm::Uuid owner, instance;
  std::vector<Origin> origins;
  std::optional<LocalInput> local_provenance;
  PreparedStudy prepared;
  std::vector<float> particles, groups, determined;
  field::FrameAnchor frame_anchor;
  field::Demand demand;
  field::NumericalProfile numerical_profile{};
  std::vector<double> particles64, groups64, determined64;
  // Prepared observations are historical values after either an edit or tick.
  // Legacy reports retain their existing logical-generation contract.
  std::optional<std::uint64_t> prepared_generation{}, prepared_completed_tick{};
};
// Owns all native bindings. Destruction drains work, destroys studies/kernels
// while their Program is alive, then finalizes it. Logical assemblies/pins are
// outside this owner. No process-failure supervisor is implied.
class ActiveSession {
 public:
  explicit ActiveSession(const engine::InstanceSettings &settings);
  ~ActiveSession();
  ExecutionHandle submit(const ActiveAssembly &assembly,
                         const field::Parameters &parameters,
                         field::HostPath path = field::HostPath::BulkCopy);
  PreparedSubmissionResult submit_prepared(PreparedStudySubmission,
                                           const warm::Uuid &owner);
  PreparedOccurrenceEditResult edit_prepared(const ExecutionHandle &, const warm::Uuid &owner,
                                             const PreparedOccurrenceEdit &);
  field::TickResult tick_prepared(const ExecutionHandle &, const warm::Uuid &owner,
      const field::Parameters64 &, std::uint64_t tick_work = UINT64_MAX,
      std::uint64_t geometry_work = UINT64_MAX);
  ExecutionHandle refresh(const ExecutionHandle &old,
                          const ActiveAssembly &assembly,
                          const field::Parameters &parameters);
  bool ready(const ExecutionHandle &handle) const;
  std::uint64_t completed_ticks(const ExecutionHandle &handle) const;
  void tick(const ExecutionHandle &handle, const field::Parameters &parameters);
  ActiveReport report(const ExecutionHandle &handle, const warm::Uuid &owner);
  void accept(const ActiveReport &report, const warm::Uuid &owner) const;
  field::PhaseTimes timings(const ExecutionHandle &handle) const;
  field::Demand demand(const ExecutionHandle &handle, const warm::Uuid &owner) const;
  double assembly_ms() const { return assembly_ms_; }
  // The backend the session actually runs on, which can differ from the request
  // (a CUDA-enabled build without a driver or device falls back to the CPU).
  bool on_cuda() const { return runtime_->on_cuda(); }

 private:
  friend struct viewport::ActiveSessionAccess;
  detail::PreparedStudyOwner *prepared_owner(const ExecutionHandle &, const warm::Uuid &) const;
  struct Execution;
  Execution &get(const ExecutionHandle &handle) const;
  warm::Uuid session_{warm::new_uuid()};
  std::unique_ptr<engine::Runtime> runtime_;
  std::map<warm::Uuid, std::unique_ptr<Execution>> executions_;
  double assembly_ms_{0};
};
}  // namespace assembly
