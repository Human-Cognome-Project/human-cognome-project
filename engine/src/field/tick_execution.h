#pragma once

#include <array>
#include <memory>
#include <vector>
#include "field/field.h"
#include "field/native_selection.h"

namespace field::detail {
enum class TickKernel {
  Clear,
  Force,
  Contact,
  Integrate,
  Determine,
  Centroid,
  Finalize,
  Begin,
  Touch,
  ContactTouch,
  Mark
};
// Exclusively owned candidate execution. Every array binding is prepared once;
// only scalar parameters change during attempts. Presence and source ledgers
// remain borrowed from the owning Harness in the same Runtime.
class TickExecution {
 public:
  static void append_layout(const BudgetDemand &, std::vector<StorageExtent> &);
  explicit TickExecution(Harness &);
  ~TickExecution();
  TickResult tick(std::uint64_t,
                  const Parameters64 &,
                  std::uint64_t,
                  std::uint64_t);
  engine::CompiledKernel &kernel(TickKernel);
  void redirect(TickKernel, taichi::lang::LaunchContextBuilder &);
  std::size_t binding_bytes() const;
  std::uint64_t native_bytes() const {
    return native_bytes_;
  }

 private:
  enum Surface {
    Bodies,
    Groups,
    Universal,
    Speed,
    ParticleFlags,
    ParticleDelta,
    GroupFlags,
    GroupDelta,
    Counters,
    Focus,
    Packets,
    Selectors,
    Previous,
    HierarchyCounters,
    Visits,
    SurfaceCount
  };
  struct CopySlot {
    taichi::lang::Ndarray *source{}, *destination{};
    RealBuffer *host{};
    taichi::lang::DataType type;
    int rank{}, rows{}, stride{};
    bool staged{};
  };
  struct Geometry;
  Harness &h_;
  std::uint64_t native_before_{};
  NativeSelectionWorkspace selection_;
  std::array<std::unique_ptr<taichi::lang::Ndarray>, SurfaceCount> values_;
  std::unique_ptr<Geometry> current_, candidate_, installed_;
  std::array<std::unique_ptr<taichi::lang::Ndarray>, 4> contact_csr_;
  std::unique_ptr<taichi::lang::Ndarray> status_;
  std::vector<CopySlot> copies_;
  std::array<std::int64_t, 2> status_values_{};
  std::array<engine::CompiledKernel, 7> numerical_;
  std::array<engine::CompiledKernel, 6> activity_;
  std::array<engine::CompiledKernel, 3> hierarchy_;
  engine::CompiledKernel stage_, publish_, before_, after_, compare_, validate_,
      split_;
  std::uint64_t copy_values_{}, native_bytes_{};
  void add(Surface,
           taichi::lang::Ndarray *,
           RealBuffer *,
           taichi::lang::DataType,
           int,
           int,
           int);
  void copy(bool publication, bool enabled = true);
  void run_hierarchy(int, const Parameters64 &);
  void run_physics(const Parameters64 &);
  void coverage(Geometry &, Geometry &, engine::CompiledKernel &, bool);
  void split_contacts();
  void check_numerics();
  void compile_copies();
  void compile_coverage();
  std::uint64_t tick_work_bound(TickResult &) const;
};
}  // namespace field::detail
