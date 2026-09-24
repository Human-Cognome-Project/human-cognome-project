#include "engine/device.h"

#include <cstdlib>
#include <thread>
#include <cstdint>
#include <unistd.h>

#include "taichi/platform/cuda/detect_cuda.h"
#include "taichi/rhi/cuda/cuda_context.h"
#include "taichi/rhi/cuda/cuda_driver.h"

namespace engine {

namespace {

// Set once anything here initializes the CUDA driver, because the visibility
// list is only read at that point.
bool cuda_touched = false;

taichi::lang::CUDADriver &driver() {
  auto &instance = taichi::lang::CUDADriver::get_instance_without_context();
  if (!cuda_touched) {
    instance.init(0);
    cuda_touched = true;
  }
  return instance;
}

}  // namespace

HostInfo host_info() {
  HostInfo info;
  info.cpu_threads = static_cast<int>(std::thread::hardware_concurrency());
  const long pages = sysconf(_SC_PHYS_PAGES);
  const long page_size = sysconf(_SC_PAGE_SIZE);
  if (pages > 0 && page_size > 0) {
    info.total_memory_bytes =
        static_cast<std::size_t>(pages) * static_cast<std::size_t>(page_size);
  }
  return info;
}

bool cuda_available() {
  return taichi::is_cuda_api_available();
}

std::vector<CudaDevice> visible_cuda_devices() {
  std::vector<CudaDevice> devices;
  if (!cuda_available()) {
    return devices;
  }
  auto &cuda = driver();
  int count = 0;
  cuda.device_get_count(&count);
  for (int index = 0; index < count; ++index) {
    void *handle = nullptr;
    cuda.device_get(&handle, reinterpret_cast<void *>(
                                 static_cast<std::intptr_t>(index)));
    char name[128] = {0};
    cuda.device_get_name(name, sizeof(name), handle);
    CudaDevice device;
    device.visible_index = index;
    device.name = name;
    cuda.device_get_attribute(&device.compute_capability_major,
                              taichi::lang::CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR,
                              handle);
    cuda.device_get_attribute(&device.compute_capability_minor,
                              taichi::lang::CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR,
                              handle);
    // The engine binds ordinal zero of whatever this process can see.
    device.bound = index == 0;
    devices.push_back(device);
  }
  return devices;
}

CudaDevice bound_cuda_device() {
  CudaDevice device;
  if (!cuda_available()) {
    return device;
  }
  cuda_touched = true;
  auto &context = taichi::lang::CUDAContext::get_instance();
  device.visible_index = 0;
  device.name = context.get_device_name();
  const int capability = context.get_compute_capability();
  device.compute_capability_major = capability / 10;
  device.compute_capability_minor = capability % 10;
  device.total_bytes = context.get_total_memory();
  device.free_bytes = context.get_free_memory();
  device.memory_pool_path = context.supports_mem_pool();
  device.bound = true;
  return device;
}

std::size_t cuda_free_memory() {
  if (!cuda_available()) {
    return 0;
  }
  cuda_touched = true;
  return taichi::lang::CUDAContext::get_instance().get_free_memory();
}

bool set_visible_cuda_devices(const std::string &list) {
  if (cuda_touched) {
    return false;
  }
  setenv("CUDA_VISIBLE_DEVICES", list.c_str(), 1);
  return true;
}

}  // namespace engine
