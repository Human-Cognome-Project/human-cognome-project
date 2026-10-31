#pragma once
#include <array>
#include <memory>
#include <vector>
#include "field/cascade.h"
#include "field/prepared_update.h"
#include "taichi/program/ndarray.h"
namespace field {
struct LiveHierarchyResult {
  CascadeStatus status{CascadeStatus::Unsupported};
  BudgetFailure failure{};
  std::uint64_t generation{}, nodes{}, responses{}, preparation_visits{},
      reference_work{}, upload_bytes{}, readback_bytes{}, synchronizations{};
};
struct LiveHierarchyInspection {
  // Borrowed until the next synchronized operation. Use order[0..node_count)
  // to address live packet slots; precision and identity are per comparison.
  std::vector<CascadeNode> nodes;
  std::vector<double> slots, previous;
  std::vector<int> selectors, fields;
  std::vector<std::int64_t> visits;
  std::array<std::int64_t, 10> counters{};
  // Logical topological order -> stable packet slot. Slots can have holes
  // after refresh; numerical arrays/nodes remain indexed by packet slot.
  std::vector<int> order;
  std::size_t node_count{};
};
// Existing fields use CascadeNode::field. A pending owner instead names a
// DefineField command and leaves field empty. No future native handle is
// guessed.
struct LiveRefreshNode : CascadeNode {
  int field_command{-1};
};
struct LiveRefreshContribution {
  UpdateRef occurrence;
  CascadeSourceRow source;
  // Insert/ReplaceParticipation command, or -1 for an untouched committed row.
  int source_command{-1};
};
struct LiveRefreshSpec {
  const LiveRefreshNode *nodes{};
  std::size_t node_count{};
  const int *children{};
  std::size_t child_count{};
  const LiveRefreshContribution *contributions{};
  std::size_t contribution_count{};
};
class Harness;
namespace testing {
struct LiveHierarchyAccess;
}
namespace detail {
struct StorageExtent;
struct PreparedCounts;
struct NativeSet;
class LiveHierarchy {
 public:
  static void append_layout(const BudgetDemand &, std::vector<StorageExtent> &);
  explicit LiveHierarchy(Harness &);
  ~LiveHierarchy();
  bool installed() const;
  std::uint64_t response_count() const;
  void validate() const;
  LiveHierarchyResult install(std::uint64_t,
                              const WeightedCascadeSpec &,
                              std::uint64_t);
  void force(double);
  void poll(double, double contact_stiffness);
  const LiveHierarchyInspection &inspect();
  std::size_t binding_bytes() const;
  const taichi::lang::Ndarray &leaf_fields() const;
  UpdateStatus prepare_refresh(const LiveRefreshSpec &,
                               const PreparedCounts &,
                               std::size_t commands,
                               bool source_changed,
                               UpdateResult &,
                               std::uint64_t work_limit);
  bool stage_refresh(UpdateResult &);
  void commit_refresh(UpdateResult &);
  void finish_refresh(std::uint64_t generation);

 private:
  friend class TickExecution;
  friend struct field::testing::LiveHierarchyAccess;
  std::uint64_t node_count() const;
  void retag_generation(std::uint64_t);
  std::array<const engine::CompiledKernel *, 3> kernels() const;
  std::array<const engine::CompiledKernel *, 2> refresh_kernels() const;
  const std::vector<int> &child_links() const;
  void poll_target(double, int, double contact_stiffness = 0);
  std::size_t fixed_object_bytes() const;
  // Build the same numerical kernels for a qualified transaction-local native
  // selection surface. Returned kernels have no bindings until their owner
  // supplies complete, fixed-lifetime inputs.
  std::array<engine::CompiledKernel, 3> build_kernels(NativeSet, NativeSet);
  void bind_input(taichi::lang::LaunchContextBuilder &,
                  int argument,
                  int input) const;
  taichi::lang::Ndarray *input_array(int input) const;
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}  // namespace detail
}  // namespace field
