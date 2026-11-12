#define HCP_ALLOCATION_LIFETIME
#include "allocation_observation.h"
#include "taichi/rhi/cpu/cpu_device.h"
#include "taichi/program/ndarray.h"
#include <iostream>
#include <stdexcept>

using namespace taichi::lang;
void require(bool ok, const char *message) {
  if (!ok) throw std::runtime_error(message);
}
DeviceAllocation allocate(Device &device, std::uint64_t bytes) {
  Device::AllocParams params;
  params.size = bytes;
  DeviceAllocation allocation;
  require(device.allocate_memory(params,&allocation) == RhiResult::success,"allocation failed");
  return allocation;
}
int main() {
  try {
    {
      cpu::CpuDevice native, foreign;
      Device &device = native;
      const auto owned = allocate(device,64);
      const auto owner = device.capture_allocation_record(owned);
      DeviceAllocation alias;
      require(device.import_owned_memory(owner.token,16,24,&alias) == RhiResult::success,
              "proven owned import failed");
      const auto registration = device.capture_allocation_record(alias);
      const auto owner_pool = device.allocation_pool_backing_record(owner.token);
      require(owner_pool.status == PoolBackingStatus::Live &&
                  owner_pool.token == device.allocation_pool_backing_record(registration.token).token &&
                  owner_pool.raw_requested_bytes == 64 && !owner_pool.rounded_extent_available &&
                  device.pool_backing_storage().pool_instance == owner_pool.token.pool_instance &&
                  device.Device::allocation_pool_backing_record(owner.token).status == PoolBackingStatus::Unavailable,
              "canonical owner/import pool backing mismatch");
      require(registration.ownership == AllocationOwnership::Imported &&
                  registration.extent_bytes == 24 && !registration.requested_bytes &&
                  !(registration.token == owner.token), "import registration collapsed into owner");
      allocation_observation::calls = 0;
      allocation_observation::enabled = true;
      const auto backing = device.allocation_backing_record(registration.token);
      allocation_observation::enabled = false;
      require(!allocation_observation::calls && backing.token == owner.token &&
                  backing.extent_bytes == 64 && backing.status == AllocationRecordStatus::Live,
              "backing query allocated or lost full owner extent");
      void *mapped = nullptr;
      require(device.map(alias,&mapped) == RhiResult::success &&
                  mapped == static_cast<char *>(native.get_alloc_info(owned).ptr)+16,
              "owned import mapped wrong offset");
      const auto raw = native.import_memory(mapped,24);
      require(device.allocation_pool_backing_record(device.capture_allocation_record(raw).token).status ==
                  PoolBackingStatus::Unknown, "raw import gained pool provenance");
      require(device.allocation_backing_record(device.capture_allocation_record(raw).token).status ==
                  AllocationRecordStatus::Unknown, "raw pointer match fabricated provenance");
      DeviceAllocation rejected = owned;
      require(device.import_owned_memory(owner.token,65,0,&rejected) == RhiResult::invalid_usage &&
                  !rejected.device &&
                  device.import_owned_memory(owner.token,1,UINT64_MAX,&rejected) == RhiResult::invalid_usage &&
                  device.import_owned_memory(registration.token,0,1,&rejected) == RhiResult::invalid_usage &&
                  foreign.import_owned_memory(owner.token,0,1,&rejected) == RhiResult::invalid_usage &&
                  device.import_owned_memory(owner.token,0,1,nullptr) == RhiResult::invalid_usage,
              "invalid/foreign/nested import accepted");
      require(device.Device::import_owned_memory(owner.token,0,1,&rejected) == RhiResult::not_supported &&
                  device.Device::allocation_backing_record(owner.token).status == AllocationRecordStatus::Unavailable,
              "default backend claimed support");
      DeviceAllocation empty;
      require(device.import_owned_memory(owner.token,64,0,&empty) == RhiResult::success &&
                  device.allocation_backing_record(device.capture_allocation_record(empty).token).token == owner.token,
              "empty end view lost owner");
      native.dealloc_memory(empty);
      native.dealloc_memory(alias);
      require(device.allocation_backing_record(registration.token).status == AllocationRecordStatus::Unknown &&
                  device.allocation_backing_record(owner.token).status == AllocationRecordStatus::Live,
              "import retirement freed owner or remained usable");
      require(device.import_owned_memory(owner.token,0,64,&alias) == RhiResult::success,
              "second owned import failed");
      const auto second = device.capture_allocation_record(alias).token;
      device.dealloc_memory(owned);
      require(device.allocation_pool_backing_record(second).status == PoolBackingStatus::Unknown,
              "retired owner still resolved pool backing");
      require(device.allocation_record(second).status == AllocationRecordStatus::Live &&
                  device.allocation_backing_record(second).status == AllocationRecordStatus::Unknown &&
                  device.import_owned_memory(owner.token,0,0,&rejected) == RhiResult::invalid_usage,
              "retired backing still certified");
      allocate(device,64);
      require(device.allocation_backing_record(second).status == AllocationRecordStatus::Unknown,
              "replacement allocation revived import provenance");
      native.clear();
      const auto zero = allocate(device,0);
      const auto zero_owner = device.capture_allocation_record(zero).token;
      require(device.allocation_pool_backing_record(zero_owner).status == PoolBackingStatus::Absent,
              "zero owner fabricated raw backing");
      require(device.import_owned_memory(zero_owner,0,0,&alias) == RhiResult::success &&
                  device.allocation_backing_record(device.capture_allocation_record(alias).token).token == zero_owner &&
                  device.allocation_backing_record(second).status == AllocationRecordStatus::Unknown,
              "zero backing or reset epoch lost identity");
      native.clear();
    }
    {
      cpu::CpuDevice native;
      auto owned = allocate(native,0);
      const auto token = native.capture_allocation_record(owned).token;
      std::size_t rows = 1;
      const auto capacity = native.allocation_record_storage().record_capacity_bytes /
                            sizeof(cpu::CpuDevice::AllocInfo);
      while (rows < capacity) { allocate(native,0); ++rows; }
      DeviceAllocation output = owned;
      allocation_lifetime::begin(1);
      bool failed = false;
      try { native.import_owned_memory(token,0,0,&output); }
      catch (const std::bad_alloc &) { failed = true; }
      const auto counts = allocation_lifetime::end();
      require(failed && counts.attempts == 1 && !counts.live && !output.device &&
                  native.allocation_backing_record(token).status == AllocationRecordStatus::Live &&
                  native.allocation_record_storage().record_capacity_bytes ==
                      capacity*sizeof(cpu::CpuDevice::AllocInfo),
              "failed import registration changed owner/output/metadata");
      require(native.import_owned_memory(token,0,0,&output) == RhiResult::success &&
                  output.alloc_id == rows, "import registration exact retry failed");
      native.clear();
    }
    // Zero payload isolates the first record-vector request from pool allocation.
    // Refusal cannot publish a live record or leave an allocation handle behind.
    {
      cpu::CpuDevice failure;
      DeviceAllocation unpublished;
      Device::AllocParams zero_params;
      allocation_lifetime::begin(1);
      bool failed = false;
      try { failure.allocate_memory(zero_params,&unpublished); }
      catch (const std::bad_alloc &) { failed = true; }
      const auto counts = allocation_lifetime::end();
      require(failed && counts.attempts == 1 && !counts.live && !unpublished.device &&
                  !failure.allocation_record_storage().record_capacity_bytes &&
                  failure.capture_allocation_record({&failure,0}).status == AllocationRecordStatus::Unknown,
              "failed record registration published identity or retained storage");
      auto retry = allocate(failure,0);
      require(failure.capture_allocation_record(retry).status == AllocationRecordStatus::Live,
              "record registration exact retry failed");
      failure.clear();
    }
    cpu::CpuDevice cpu, foreign;
    Device &device = cpu;
    auto a = allocate(device,64);
    const auto record = device.capture_allocation_record(a);
    require(record.status == AllocationRecordStatus::Live &&
                record.ownership == AllocationOwnership::Owned &&
                record.requested_bytes == 64 && record.extent_bytes == 64,
            "owned request record incorrect");
    {
      Ndarray full(a,PrimitiveType::i32,{16}), part(a,PrimitiveType::i32,{4});
      require(device.capture_allocation_record(full.get_device_allocation()).token ==
                  device.capture_allocation_record(part.get_device_allocation()).token,
              "views lost allocation identity");
    }
    auto distinct = allocate(device,64);
    require(!(device.capture_allocation_record(distinct).token == record.token),
            "equal-sized allocations collapsed");
    auto other = allocate(foreign,64);
    require(other.alloc_id == a.alloc_id &&
                device.capture_allocation_record(other).status == AllocationRecordStatus::Unknown &&
                foreign.allocation_record(record.token).status == AllocationRecordStatus::Unknown,
            "foreign device token or handle accepted");
    auto zero = allocate(device,0);
    const auto zero_token = device.capture_allocation_record(zero).token;
    require(device.allocation_record(zero_token).status == AllocationRecordStatus::Live,
            "zero-size live allocation mistaken for retired");
    device.dealloc_memory(zero);
    require(device.allocation_record(zero_token).status == AllocationRecordStatus::Retired,
            "zero-size release not recorded");
    device.dealloc_memory(a);
    const auto retired = device.allocation_record(record.token);
    require(retired.status == AllocationRecordStatus::Retired && retired.requested_bytes == 64 &&
                retired.extent_bytes == 64,"retirement lost original request extent");

    // Imported aliases must not free the original owner's backing.
    const auto owned_pointer = cpu.get_alloc_info(distinct).ptr;
    const auto imported_alias = cpu.import_memory(owned_pointer,64);
    require(device.capture_allocation_record(imported_alias).ownership == AllocationOwnership::Imported,
            "imported alias claimed backing ownership");
    cpu.dealloc_memory(imported_alias);
    require(cpu.get_alloc_info(distinct).ptr == owned_pointer &&
                device.capture_allocation_record(distinct).status == AllocationRecordStatus::Live,
            "imported alias release retired the backing owner");
    int imported[4]{17,19,23,29};
    const auto imported_handle = cpu.import_memory(imported,sizeof(imported));
    const auto import_record = device.capture_allocation_record(imported_handle);
    require(import_record.ownership == AllocationOwnership::Imported &&
                !import_record.requested_bytes && import_record.extent_bytes == sizeof(imported),
            "import claimed owned request bytes");
    cpu.dealloc_memory(imported_handle);
    require(imported[0] == 17 && device.allocation_record(import_record.token).status == AllocationRecordStatus::Retired,
            "import release freed external storage or failed to retire registration");
    const auto retained = device.allocation_record_storage();
    require(retained.available && retained.device_object_bytes == sizeof(cpu) &&
                retained.record_capacity_bytes >= 4*sizeof(cpu::CpuDevice::AllocInfo),
            "retained record metadata unaccounted");
    allocation_observation::calls.store(0);
    allocation_observation::enabled.store(true);
    const auto observed = device.allocation_record(record.token);
    const auto metadata = device.allocation_record_storage();
    allocation_observation::enabled.store(false);
    require(!allocation_observation::calls.load() && observed.status == AllocationRecordStatus::Retired &&
                metadata.record_capacity_bytes == retained.record_capacity_bytes,"record inspection allocated");

    cpu.clear();
    require(device.allocation_record(record.token).status == AllocationRecordStatus::Unknown &&
                device.allocation_record_storage().record_capacity_bytes == retained.record_capacity_bytes,
            "reset preserved stale identity or hid retained capacity");
    auto reused = allocate(device,13);
    const auto current = device.capture_allocation_record(reused);
    require(reused.alloc_id == a.alloc_id && !(current.token == record.token) &&
                current.requested_bytes == 13 && current.extent_bytes == 13 &&
                device.allocation_record(record.token).status == AllocationRecordStatus::Unknown,
            "reset/reuse revived an old token");
    auto invalid = current.token;
    invalid.allocation = UINT64_MAX;
    const auto unknown = device.allocation_record(invalid);
    require(unknown.status == AllocationRecordStatus::Unknown && !unknown.requested_bytes &&
                !unknown.extent_bytes && unknown.ownership == AllocationOwnership::Unknown,
            "invalid token supplied a usable extent");
    require(device.Device::allocation_record(current.token).status == AllocationRecordStatus::Unavailable &&
                !device.Device::allocation_record_storage().available,
            "unsupported backend default fabricated zero coverage");
    LlvmDevice::LlvmRuntimeAllocParams runtime_params;
    runtime_params.size = 7;
    auto runtime_allocation = cpu.allocate_memory_runtime(runtime_params);
    require(device.capture_allocation_record(runtime_allocation).requested_bytes == 7,
            "runtime allocation bypassed records");
    cpu.import_memory(imported,sizeof(imported));
    cpu.clear();
    require(imported[3] == 29,"reset released imported backing");
    foreign.clear();

    // Reuse exactly the same device address, querying only its new live owner.
    alignas(cpu::CpuDevice) unsigned char storage[sizeof(cpu::CpuDevice)];
    auto *first = ::new (static_cast<void *>(storage)) cpu::CpuDevice;
    auto first_handle = allocate(*first,1);
    const auto expired = first->capture_allocation_record(first_handle).token;
    first->clear(); first->~CpuDevice();
    auto *second = ::new (static_cast<void *>(storage)) cpu::CpuDevice;
    auto second_handle = allocate(*second,1);
    require(second->allocation_record(expired).status == AllocationRecordStatus::Unknown &&
                !(second->capture_allocation_record(second_handle).token == expired),
            "device-address reuse revived expired token");
    second->clear(); second->~CpuDevice();
    std::cout << "allocation_record_test PASS device_bytes=" << retained.device_object_bytes
              << " record_capacity_bytes=" << retained.record_capacity_bytes << '\n';
    return 0;
  } catch (const std::exception &e) {
    allocation_observation::enabled.store(false);
    std::cerr << e.what() << '\n'; return 1;
  }
}
