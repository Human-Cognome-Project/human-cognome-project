#pragma once

#include <cstddef>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>

#include "taichi/program/compiler_jit_counts.h"

namespace taichi::lang {
class CompiledKernelData;
class KernelCompilationManager;

struct CompiledBorrowObservation {
  OwnershipCountStatus status{OwnershipCountStatus::Unavailable};
  const void *owner{nullptr}, *record{nullptr};
  std::size_t tracked_borrows{0};
  bool untracked_clients{true};
  // A borrow observation alone never establishes removal eligibility. The
  // manager's separate bounded operation checks storage/registration/provenance.
  bool reclamation_supported{false};
};

// Existing native cache owns the data. The guard owns only lifetime metadata.
// Keep Program/context dependencies alive and serialize cache mutation/acquire/
// release/observation. This does not stop in-flight launches or enable eviction.
class CompiledKernelBorrow {
 public:
  CompiledKernelBorrow() = default;
  CompiledKernelBorrow(const CompiledKernelBorrow &) = delete;
  CompiledKernelBorrow &operator=(const CompiledKernelBorrow &) = delete;
  CompiledKernelBorrow(CompiledKernelBorrow &&other) noexcept = default;
  CompiledKernelBorrow &operator=(CompiledKernelBorrow &&other) noexcept {
    if (this != &other) {
      release();
      state_ = std::move(other.state_);
    }
    return *this;
  }
  ~CompiledKernelBorrow() { release(); }

  bool has_record() const noexcept { return bool(state_); }
  // Validate metadata before dereferencing the native object, including after
  // its owner has finalized. Derived references may not outlive this guard.
  const CompiledKernelData &get() const & {
    if (!state_ || !state_->data)
      throw std::logic_error("compiled-data borrow is empty or expired");
    return *state_->data;
  }
  const CompiledKernelData &get() const && = delete;
  CompiledBorrowObservation observation() const noexcept {
    return observe(state_);
  }

 private:
  friend class CompiledKernelData;
  friend class KernelCompilationManager;
  struct State {
    const CompiledKernelData *data;
    const void *owner;
    std::size_t borrows{0};
    bool untracked{false};
    State(const CompiledKernelData *data, const void *owner) : data(data), owner(owner) {}
  };
  explicit CompiledKernelBorrow(std::shared_ptr<State> state) : state_(std::move(state)) {
    if (state_->borrows == std::numeric_limits<std::size_t>::max())
      throw std::overflow_error("compiled-data borrow count overflow");
    ++state_->borrows;
  }
  static CompiledBorrowObservation observe(const std::shared_ptr<State> &state) noexcept {
    CompiledBorrowObservation result;
    if (state) {
      result.status = state->data ? OwnershipCountStatus::Present : OwnershipCountStatus::Absent;
      result.owner = state->owner; result.record = state.get();
      result.tracked_borrows = state->borrows; result.untracked_clients = state->untracked;
    }
    return result;
  }
  void release() noexcept {
    if (state_) { --state_->borrows; state_.reset(); }
  }
  std::shared_ptr<State> state_;
};
}  // namespace taichi::lang
