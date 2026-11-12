#define HCP_ALLOCATION_LIFETIME
#include "allocation_observation.h"
#include "engine/runtime.h"
#include "taichi/rhi/cpu/cpu_device.h"
#include <iostream>
#include <stdexcept>

using namespace taichi::lang;
namespace {
void require(bool ok, const char *message) {
  if (!ok) throw std::runtime_error(message);
}
DeviceAllocation allocate(Device &device, std::size_t bytes) {
  Device::AllocParams params;
  params.size = bytes;
  DeviceAllocation allocation;
  require(device.allocate_memory(params, &allocation) == RhiResult::success,
          "allocation failed");
  return allocation;
}
SNodeTree *tree(Program &program) {
  auto root = std::make_unique<SNode>(0, SNodeType::root);
  auto &leaf = root->dense(Axis(0), 7).insert_children(SNodeType::place);
  leaf.dt = PrimitiveType::i32;
  return program.add_snode_tree(std::move(root), false);
}
void array_views() {
  cpu::CpuDevice device, foreign;
  auto allocation = allocate(device, 64);
  Ndarray full(allocation, PrimitiveType::i32, {16});
  Ndarray part(allocation, PrimitiveType::i32, {4});
  const auto original = full.backing_record(device);
  require(original.status == AllocationRecordStatus::Live &&
              original.requested_bytes == 64 &&
              part.backing_record(device).token == original.token &&
              part.backing_record(device).extent_bytes == 64,
          "view shape replaced actual backing extent or identity");
  require(full.backing_record(foreign).status == AllocationRecordStatus::Unknown,
          "foreign owner accepted");
  auto imported = device.import_memory(device.get_alloc_info(allocation).ptr, 64);
  {
    Ndarray alias(imported, PrimitiveType::i32, {16});
    const auto record = alias.backing_record(device);
    require(record.ownership == AllocationOwnership::Imported &&
                record.requested_bytes == 0 && !(record.token == original.token),
            "import registration falsely resolved to owned backing");
    device.dealloc_memory(imported);
    require(alias.backing_record(device).status == AllocationRecordStatus::Retired &&
                full.backing_record(device).status == AllocationRecordStatus::Live,
            "import retirement affected owner backing");
  }
  device.dealloc_memory(allocation);
  require(full.backing_record(device).status == AllocationRecordStatus::Retired,
          "array did not expose retirement");
  device.clear();
  auto reused = allocate(device, 64);
  require(reused.alloc_id == allocation.alloc_id &&
              full.backing_record(device).status == AllocationRecordStatus::Unknown,
          "array recaptured reused ID after reset");
  Ndarray fresh(reused, PrimitiveType::i32, {16});
  require(fresh.backing_record(device).status == AllocationRecordStatus::Live &&
              !(fresh.backing_record(device).token == original.token),
          "new array did not capture new incarnation");
  device.clear();
}
void native_owners() {
  engine::InstanceSettings settings;
  settings.cpu_threads = 1;
  engine::Runtime runtime(settings);
  auto &program = runtime.program();
  auto &impl = *program.prog_impl;
  auto &device = *impl.get_compute_device();
  require(impl.snode_root_backing_record(-1).status == AllocationRecordStatus::Unknown,
          "missing root reported as zero backing");
  auto *first = tree(program);
  auto *second = tree(program);
  const auto a = impl.snode_root_backing_record(first->id());
  const auto b = impl.snode_root_backing_record(second->id());
  require(a.status == AllocationRecordStatus::Live &&
              a.ownership == AllocationOwnership::Owned && a.requested_bytes >= 7 * sizeof(int) &&
              a.extent_bytes == a.requested_bytes && b.requested_bytes == a.requested_bytes &&
              !(a.token == b.token), "native roots lost backing identity or extent");
  require(impl.ProgramImpl::snode_root_backing_record(first->id()).status ==
              AllocationRecordStatus::Unavailable, "base coverage default is not unavailable");
  AllocationRecord array_record;
  {
    Ndarray owned(&program, PrimitiveType::i32, {7});
    array_record = owned.backing_record(device);
    require(array_record.status == AllocationRecordStatus::Live &&
                array_record.requested_bytes == 7 * sizeof(int), "owned array request missing");
    allocation_lifetime::begin();
    const auto queried_array = owned.backing_record(device);
    const auto queried_root = impl.snode_root_backing_record(first->id());
    const auto counts = allocation_lifetime::end();
    require(!counts.attempts && queried_array.token == array_record.token &&
                queried_root.token == a.token, "owner inspection allocated or changed identity");
  }
  require(device.allocation_record(array_record.token).status == AllocationRecordStatus::Retired,
          "owned array destruction left live record");
  const auto first_id = first->id();
  program.destroy_snode_tree(first);
  require(impl.snode_root_backing_record(first_id).status == AllocationRecordStatus::Unknown &&
              device.allocation_record(a.token).status == AllocationRecordStatus::Retired &&
              impl.snode_root_backing_record(second->id()).token == b.token,
          "root destruction did not retire exactly its backing");
  program.destroy_snode_tree(second);
  std::cout << "root_request=" << a.requested_bytes << " array_request="
            << array_record.requested_bytes << " ndarray_object=" << sizeof(Ndarray) << '\n';
}
}
int main() {
  try {
    array_views();
    native_owners();
    std::cout << "owner_backing_test PASS\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
