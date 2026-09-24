#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace engine {

// What the host offers. Read from the machine, not configured.
struct HostInfo {
  int cpu_threads{0};
  std::size_t total_memory_bytes{0};
};

HostInfo host_info();

// One CUDA device as the process can see it. |total_bytes| and |free_bytes|
// are filled in only for the device the engine has bound, because the sizes
// come from the bound context.
struct CudaDevice {
  int visible_index{0};
  std::string name;
  int compute_capability_major{0};
  int compute_capability_minor{0};
  std::size_t total_bytes{0};
  std::size_t free_bytes{0};
  bool bound{false};
  // Whether the engine will use the asynchronous memory-pool allocation path
  // on this device. Filled in for the bound device only.
  bool memory_pool_path{false};
};

bool cuda_available();

// Enumerates the devices this process can see, without binding one.
std::vector<CudaDevice> visible_cuda_devices();

// The device the engine uses. The engine binds visible ordinal zero
// (taichi/rhi/cuda/cuda_context.cpp), so this is always index 0 of the list
// above, and calling it creates the engine's process-wide CUDA context.
CudaDevice bound_cuda_device();

// Free memory on the bound device, for measuring what a runtime costs.
std::size_t cuda_free_memory();

// Chooses which physical devices the process can see, by setting
// CUDA_VISIBLE_DEVICES. The CUDA driver reads it when it initializes, which
// happens the first time the engine touches CUDA, so this only works before
// any other call in this header and before a CUDA Runtime is constructed.
// Returns false, changing nothing, once CUDA has been touched.
bool set_visible_cuda_devices(const std::string &list);

}  // namespace engine
