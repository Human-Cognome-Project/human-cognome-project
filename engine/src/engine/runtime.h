#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "taichi/ir/ir_builder.h"
#include "taichi/ir/statements.h"
#include "taichi/program/program.h"

namespace engine {

// Instance settings, supplied by the caller. Anything left unset keeps the
// engine's own default: this workspace declares no policy of its own.
// Capacities are read when the Program is constructed and are fixed for the
// life of the instance.
struct InstanceSettings {
  bool cuda{false};
  // Read the engine own startup environment variables first, so a caller
  // that sets nothing gets the engine documented behaviour. Explicit
  // settings below still win.
  bool read_environment{true};
  std::optional<int> snode_capacity;
  std::optional<int> snode_tree_capacity;
  std::optional<double> device_memory_gb;
  std::optional<int> cpu_threads;
};

// Owns one engine instance. Device allocations belong to it: destroying it
// finalizes the engine and releases them, and nothing built against it stays
// valid afterwards.
class Runtime {
 public:
  explicit Runtime(const InstanceSettings &settings);
  Runtime(const Runtime &) = delete;
  Runtime &operator=(const Runtime &) = delete;
  ~Runtime();

  taichi::lang::Program &program() {
    return *program_;
  }
  bool on_cuda() const {
    return settings_.cuda;
  }
  const InstanceSettings &settings() const {
    return settings_;
  }
  void synchronize();

  // Threads an offloaded task should be launched with. A top-level loop built
  // with IRBuilder must carry this; zero aborts the launch.
  int cpu_threads() const;

  // SNode identifiers issued so far, including those of destroyed layouts.
  int issued_snode_ids() const;

 private:
  InstanceSettings settings_;
  std::unique_ptr<taichi::lang::Program> program_;
};

// One kernel parameter. Scalars are passed by value; arrays are passed as
// engine ndarrays the host can read and write, dense over total_dim axes
// (1 by default; a caller with a multi-axis array passes its rank so the
// registered kernel-argument dimensionality matches what the kernel body's
// IR addresses it with).
struct Param {
  bool is_array{false};
  taichi::lang::DataType dt;
  int total_dim{1};

  static Param scalar(taichi::lang::DataType dt) {
    return Param{false, dt, 1};
  }
  static Param array(taichi::lang::DataType dt, int total_dim = 1) {
    return Param{true, dt, total_dim};
  }
};

// A kernel built from engine IR, compiled once and launched many times.
// Argument ids follow the declaration order of |params|.
class CompiledKernel {
 public:
  CompiledKernel() = default;
  CompiledKernel(Runtime &runtime,
                 const std::string &name,
                 std::unique_ptr<taichi::lang::Block> body,
                 const std::vector<Param> &params);

  taichi::lang::LaunchContextBuilder make_context();
  void launch(taichi::lang::LaunchContextBuilder &ctx);

 private:
  Runtime *runtime_{nullptr};
  std::unique_ptr<taichi::lang::Kernel> kernel_;
  const taichi::lang::CompiledKernelData *compiled_{nullptr};
};

}  // namespace engine
