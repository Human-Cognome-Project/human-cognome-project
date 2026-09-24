#include "engine/transfer.h"

#include "taichi/rhi/public_device.h"

namespace engine {

using namespace taichi::lang;

namespace {

void check_size(const Ndarray &array, std::size_t bytes) {
  const std::size_t capacity = array.get_nelement() * array.get_element_size();
  TI_ASSERT_INFO(bytes <= capacity,
                 "transfer of {} bytes exceeds the {}-byte array", bytes,
                 capacity);
}

}  // namespace

void upload(Ndarray &array, const void *src, std::size_t bytes) {
  check_size(array, bytes);
  DeviceAllocation alloc = array.get_device_allocation();
  DevicePtr ptr = alloc.get_ptr(0);
  std::size_t size = bytes;
  TI_ASSERT(alloc.device->upload_data(&ptr, &src, &size, 1) ==
            RhiResult::success);
}

void readback(Ndarray &array, void *dst, std::size_t bytes) {
  check_size(array, bytes);
  DeviceAllocation alloc = array.get_device_allocation();
  DevicePtr ptr = alloc.get_ptr(0);
  std::size_t size = bytes;
  TI_ASSERT(alloc.device->readback_data(&ptr, &dst, &size, 1) ==
            RhiResult::success);
}

}  // namespace engine
