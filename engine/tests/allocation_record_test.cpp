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
