#include "taichi/rhi/cpu/cpu_device.h"
#include "taichi/rhi/impl_support.h"
#include "taichi/rhi/common/host_memory_pool.h"

#include "taichi/jit/jit_module.h"

namespace taichi::lang {

namespace cpu {

CpuDevice::AllocInfo CpuDevice::get_alloc_info(const DeviceAllocation handle) {
  validate_device_alloc(handle);
  return allocations_[handle.alloc_id];
}

CpuDevice::CpuDevice() {
  // Materialize shared bookkeeping during setup, never during metadata queries.
  (void)HostMemoryPool::get_instance();
}

AllocationRecord CpuDevice::capture_allocation_record(DeviceAllocation handle) const {
  if (handle.device != this) {
    AllocationRecord result;
    result.status = AllocationRecordStatus::Unknown;
    return result;
  }
  return allocation_record({allocation_instance(), allocation_epoch_, handle.alloc_id});
}

AllocationRecord CpuDevice::allocation_record(AllocationRecordToken token) const {
  AllocationRecord result;
  if (!allocation_instance() || !allocation_epoch_) return result;
  result.status = AllocationRecordStatus::Unknown;
  if (token.device_instance != allocation_instance() || token.epoch != allocation_epoch_ ||
      token.allocation >= allocations_.size()) return result;
  const auto &info = allocations_[token.allocation];
  result.token = token;
  result.status = info.live ? AllocationRecordStatus::Live : AllocationRecordStatus::Retired;
  result.ownership = info.imported ? AllocationOwnership::Imported : AllocationOwnership::Owned;
  result.requested_bytes = info.imported ? 0 : info.size;
  result.extent_bytes = info.size;  // CPU exclusive request, not OS page residency.
  return result;
}

AllocationRecordStorage CpuDevice::allocation_record_storage() const {
  return {true, sizeof(*this), allocations_.capacity() * sizeof(AllocInfo)};
}

AllocationRecord CpuDevice::allocation_backing_record(AllocationRecordToken token) const {
  auto record = allocation_record(token);
  if (record.status == AllocationRecordStatus::Unavailable) return record;
  if (record.status == AllocationRecordStatus::Live) {
    if (record.ownership == AllocationOwnership::Owned) return record;
    const auto owner = allocation_record(allocations_[token.allocation].backing_owner);
    if (owner.status == AllocationRecordStatus::Live &&
        owner.ownership == AllocationOwnership::Owned) return owner;
  }
  AllocationRecord unknown;
  unknown.status = AllocationRecordStatus::Unknown;
  return unknown;
}

RhiResult CpuDevice::import_owned_memory(AllocationRecordToken token,
                                        uint64_t offset, uint64_t size,
                                        DeviceAllocation *out) {
  if (!out) return RhiResult::invalid_usage;
  *out = kDeviceNullAllocation;
  const auto owner = allocation_record(token);
  if (owner.status != AllocationRecordStatus::Live ||
      owner.ownership != AllocationOwnership::Owned ||
      offset > owner.extent_bytes || size > owner.extent_bytes - offset)
    return RhiResult::invalid_usage;
  // Copy before push_back: growth may invalidate the source row.
  const auto source = allocations_[token.allocation];
  AllocInfo info;
  info.ptr = source.ptr ? static_cast<char *>(source.ptr) + offset : nullptr;
  info.size = size;
  info.live = true;
  info.imported = true;
  info.backing_owner = token;
  const auto id = allocations_.size();
  allocations_.push_back(info);  // Failure leaves the owner and output untouched.
  *out = DeviceAllocation{this, id};
  return RhiResult::success;
}

PoolBackingRecord CpuDevice::allocation_pool_backing_record(AllocationRecordToken token) const {
  const auto owner = allocation_backing_record(token);
  if (owner.status == AllocationRecordStatus::Unavailable) return {};
  PoolBackingRecord result;
  result.status = PoolBackingStatus::Unknown;
  if (owner.status != AllocationRecordStatus::Live) return result;
  if (!owner.extent_bytes) {
    result.status = PoolBackingStatus::Absent;  // Live zero-byte CPU owner has no raw mapping.
    return result;
  }
  return HostMemoryPool::get_instance().capture_backing_record(
      allocations_[owner.token.allocation].ptr, owner.extent_bytes);
}

PoolBackingStorage CpuDevice::pool_backing_storage() const {
  return HostMemoryPool::get_instance().backing_storage();
}

void CpuDevice::clear() {
  for (std::size_t i = 0; i < allocations_.size(); ++i)
    if (allocations_[i].live) dealloc_memory(DeviceAllocation{this, i});
  allocations_.clear();  // Retained capacity is still reported by storage().
  if (allocation_epoch_)
    allocation_epoch_ = allocation_epoch_ == UINT64_MAX ? 0 : allocation_epoch_ + 1;
}

RhiResult CpuDevice::allocate_memory(const AllocParams &params,
                                     DeviceAllocation *out_devalloc) {
  AllocInfo info;
  info.size = params.size;
  info.use_cached = false;
  info.live = true;

  if (info.size == 0) {
    info.ptr = nullptr;
  } else {
    info.ptr = HostMemoryPool::get_instance().allocate(
        params.size, HostMemoryPool::page_size, true /*exclusive*/);

    if (info.ptr == nullptr) {
      return RhiResult::out_of_memory;
    }
  }
  *out_devalloc = DeviceAllocation{};
  out_devalloc->alloc_id = allocations_.size();
  out_devalloc->device = this;

  try {
    allocations_.push_back(info);
  } catch (...) {
    if (info.ptr) HostMemoryPool::get_instance().release(info.size, info.ptr);
    *out_devalloc = kDeviceNullAllocation;
    throw;
  }

  return RhiResult::success;
}

DeviceAllocation CpuDevice::allocate_memory_runtime(
    const LlvmRuntimeAllocParams &params) {
  DeviceAllocation alloc;
  RhiResult res = allocate_memory(params, &alloc);
  RHI_ASSERT(res == RhiResult::success &&
             "Failed to allocate memory for runtime");
  return alloc;
}

uint64_t *CpuDevice::allocate_llvm_runtime_memory_jit(
    const LlvmRuntimeAllocParams &params) {
  params.runtime_jit->call<void *, std::size_t, std::size_t>(
      "runtime_memory_allocate_aligned", params.runtime, params.size,
      taichi_page_size, params.result_buffer);
  return reinterpret_cast<uint64_t *>(params.result_buffer[0]);
}

void CpuDevice::dealloc_memory(DeviceAllocation handle) {
  validate_device_alloc(handle);
  AllocInfo &info = allocations_[handle.alloc_id];
  if (!info.live) {
    TI_ERROR("the DeviceAllocation is already deallocated");
  }
  if (!info.imported && !info.use_cached && info.size) {
    HostMemoryPool::get_instance().release(info.size, info.ptr);
  }
  info.ptr = nullptr;
  info.live = false;
}

RhiResult CpuDevice::upload_data(DevicePtr *device_ptr,
                                 const void **data,
                                 size_t *size,
                                 int num_alloc) noexcept {
  if (!device_ptr || !data || !size) {
    return RhiResult::invalid_usage;
  }

  for (int i = 0; i < num_alloc; i++) {
    if (device_ptr[i].device != this || !data[i]) {
      return RhiResult::invalid_usage;
    }

    AllocInfo &info = allocations_[device_ptr[i].alloc_id];
    memcpy((uint8_t *)info.ptr + device_ptr[i].offset, data[i], size[i]);
  }

  return RhiResult::success;
}

RhiResult CpuDevice::readback_data(
    DevicePtr *device_ptr,
    void **data,
    size_t *size,
    int num_alloc,
    const std::vector<StreamSemaphore> &wait_sema) noexcept {
  if (!device_ptr || !data || !size) {
    return RhiResult::invalid_usage;
  }

  for (int i = 0; i < num_alloc; i++) {
    if (device_ptr[i].device != this || !data[i]) {
      return RhiResult::invalid_usage;
    }

    AllocInfo &info = allocations_[device_ptr[i].alloc_id];
    memcpy(data[i], (uint8_t *)info.ptr + device_ptr[i].offset, size[i]);
  }

  return RhiResult::success;
}

RhiResult CpuDevice::map_range(DevicePtr ptr,
                               uint64_t size,
                               void **mapped_ptr) {
  AllocInfo &info = allocations_[ptr.alloc_id];
  if (info.ptr == nullptr) {
    return RhiResult::error;
  }
  *mapped_ptr = (uint8_t *)info.ptr + ptr.offset;
  return RhiResult::success;
}

RhiResult CpuDevice::map(DeviceAllocation alloc, void **mapped_ptr) {
  AllocInfo &info = allocations_[alloc.alloc_id];
  if (info.ptr == nullptr) {
    return RhiResult::error;
  }
  *mapped_ptr = info.ptr;
  return RhiResult::success;
}

void CpuDevice::unmap(DeviceAllocation alloc) {
  return;
}

void CpuDevice::memcpy_internal(DevicePtr dst, DevicePtr src, uint64_t size) {
  void *dst_ptr =
      static_cast<char *>(allocations_[dst.alloc_id].ptr) + dst.offset;
  void *src_ptr =
      static_cast<char *>(allocations_[src.alloc_id].ptr) + src.offset;
  std::memcpy(dst_ptr, src_ptr, size);
}

DeviceAllocation CpuDevice::import_memory(void *ptr, size_t size) {
  AllocInfo info;
  info.ptr = ptr;
  info.size = size;
  info.live = true;
  info.imported = true;

  DeviceAllocation alloc;
  alloc.alloc_id = allocations_.size();
  alloc.device = this;

  allocations_.push_back(info);
  return alloc;
}

}  // namespace cpu
}  // namespace taichi::lang
