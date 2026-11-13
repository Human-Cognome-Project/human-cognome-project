#define HCP_ALLOCATION_LIFETIME
#include "allocation_observation.h"
#include "engine/runtime.h"
#include "taichi/rhi/cpu/cpu_device.h"
#include "taichi/system/threading.h"
#include <iostream>
#include <stdexcept>

using namespace taichi::lang;
namespace {
void require(bool ok, const char *message) {
  if (!ok) throw std::runtime_error(message);
}
void vector_limits() {
  struct Oversized {
    using value_type = std::uint64_t;
    std::size_t size() const { return 0; }
    std::size_t capacity() const { return std::numeric_limits<std::size_t>::max(); }
  };
  const auto overflow = program_vector_storage(Oversized{});
  require(overflow.overflow && !overflow.available && !overflow.capacity_bytes,
          "vector capacity overflow became available zero storage");
  std::vector<int> retained;
  retained.reserve(7);
  retained.push_back(1);
  const auto populated = program_vector_storage(retained);
  retained.clear();
  const auto empty = program_vector_storage(retained);
  require(empty.available && !empty.overflow && !empty.slots &&
              empty.capacity_bytes == populated.capacity_bytes &&
              empty.capacity_bytes >= 7 * sizeof(int),
          "empty vector lost retained capacity");
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
void native_owners(int workers) {
  engine::InstanceSettings settings;
  settings.cpu_threads = workers;
  engine::Runtime runtime(settings);
  auto &program = runtime.program();
  auto &impl = *program.get_program_impl();
  auto &device = *impl.get_compute_device();
  const auto program_empty = program.lifecycle_storage();
  require(program_empty.available && !program_empty.complete &&
              program_empty.owner == &program &&
              program_empty.object_bytes == sizeof(Program) &&
              program_empty.kernels.available && !program_empty.kernels.slots &&
              program_empty.functions.available && !program_empty.functions.slots &&
              program_empty.trees.available && !program_empty.trees.slots &&
              program_empty.textures.available && !program_empty.textures.slots &&
              !program_empty.map_storage_available &&
              !program_empty.free_id_storage_available &&
              !program_empty.pointee_storage_available,
          "Program empty coverage confused unavailable families with zero storage");
  require(impl.launch_registry_storage().status == LaunchMetadataStatus::Absent,
          "Program inspection created a launcher");
  program.kernel([](Kernel *) {}, "storage_probe");
  program.create_function(FunctionKey("storage_function", 17, 0));
  const auto definitions = program.lifecycle_storage();
  require(definitions.kernels.slots == 1 && definitions.functions.slots == 1 &&
              definitions.kernels.capacity_bytes >= sizeof(std::unique_ptr<Kernel>) &&
              definitions.functions.capacity_bytes >= sizeof(std::unique_ptr<Function>) &&
              !definitions.kernels.overflow && !definitions.functions.overflow,
          "Program definition vectors omitted retained capacity");
  const auto empty = impl.runtime_bookkeeping();
  require(empty.thread_pool.available && empty.thread_pool.present &&
              !empty.thread_pool.overflow && !empty.thread_pool.complete &&
              empty.thread_pool.owner && empty.thread_pool.owner != empty.executor_identity &&
              empty.thread_pool.owner != empty.root_manager_identity &&
              empty.thread_pool.object_bytes == sizeof(taichi::ThreadPool) &&
              empty.thread_pool.thread_handles == static_cast<std::size_t>(workers) &&
              empty.thread_pool.handle_capacity_bytes >= workers * sizeof(std::thread) &&
              !impl.ProgramImpl::runtime_bookkeeping().thread_pool.available,
          "native thread-pool storage coverage incorrect");
  require(empty.available && !empty.complete && empty.executor_identity &&
              empty.executor_base_object_bytes && empty.root_manager_identity &&
              empty.root_manager_object_bytes && empty.root_records.available &&
              !empty.root_records.entries && empty.tree_allocations.available &&
              empty.runtime_allocations.available &&
              !impl.ProgramImpl::runtime_bookkeeping().available,
          "runtime bookkeeping confused empty supported storage with unavailable");
  require(empty.root_records.allocation_domain != empty.tree_allocations.allocation_domain &&
              empty.tree_allocations.allocation_domain != empty.runtime_allocations.allocation_domain &&
              empty.root_records.counter_object_bytes &&
              empty.root_records.counter_control_block_bytes,
          "runtime map domains or counter requests missing");
  require(impl.snode_root_record_storage_bytes() == 0 &&
              !impl.ProgramImpl::snode_root_record_storage_bytes(),
          "empty supported metadata confused with unavailable");
  require(impl.snode_root_backing_record(-1).status == AllocationRecordStatus::Unknown,
          "missing root reported as zero backing");
  auto *first = tree(program);
  auto *second = tree(program);
  const auto program_roots = program.lifecycle_storage();
  require(program_roots.trees.available && !program_roots.trees.overflow &&
              program_roots.trees.slots == 2 &&
              program_roots.trees.capacity_bytes >= 2 * sizeof(std::unique_ptr<SNodeTree>),
          "Program tree-vector storage missing");
  const auto a = impl.snode_root_backing_record(first->id());
  const auto b = impl.snode_root_backing_record(second->id());
  const auto roots = impl.runtime_bookkeeping();
  require(roots.executor_identity == empty.executor_identity &&
              roots.root_manager_identity == empty.root_manager_identity &&
              roots.root_records.entries == 2 &&
              roots.tree_allocations.entries == empty.tree_allocations.entries + 2 &&
              roots.runtime_allocations.entries == empty.runtime_allocations.entries + 2 &&
              roots.root_records.allocation_request_bytes > empty.root_records.allocation_request_bytes &&
              roots.root_records.allocation_request_bytes >= roots.root_records.value_payload_bytes &&
              roots.root_records.value_payload_bytes >= 2*sizeof(AllocationRecord),
          "runtime maps did not report retained root bookkeeping");
  require(impl.snode_root_record_storage_bytes() == 2 * sizeof(AllocationRecord),
          "root record storage missing or counted per query");
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
    const auto queried_metadata = impl.runtime_bookkeeping();
    const auto queried_program = program.lifecycle_storage();
    const auto counts = allocation_lifetime::end();
    require(!counts.attempts && queried_array.token == array_record.token &&
                queried_program.owner == &program &&
                queried_program.trees.capacity_bytes == program_roots.trees.capacity_bytes &&
                queried_root.token == a.token && queried_metadata.available &&
                queried_metadata.runtime_allocations.entries == roots.runtime_allocations.entries + 1 &&
                queried_metadata.root_records.allocation_domain == roots.root_records.allocation_domain,
            "owner inspection allocated or changed identity");
    require(queried_metadata.thread_pool.owner == empty.thread_pool.owner &&
                queried_metadata.thread_pool.thread_handles == empty.thread_pool.thread_handles &&
                queried_metadata.thread_pool.handle_capacity_bytes == empty.thread_pool.handle_capacity_bytes,
            "runtime ownership work changed thread-pool storage");
  }
  require(device.allocation_record(array_record.token).status == AllocationRecordStatus::Retired,
          "owned array destruction left live record");
  require(impl.runtime_bookkeeping().runtime_allocations.entries == roots.runtime_allocations.entries + 1,
          "retained ndarray descriptor was mistaken for live allocation storage");
  const auto first_id = first->id();
  program.destroy_snode_tree(first);
  require(impl.snode_root_backing_record(first_id).status == AllocationRecordStatus::Unknown &&
              device.allocation_record(a.token).status == AllocationRecordStatus::Retired &&
              impl.snode_root_backing_record(second->id()).token == b.token,
          "root destruction did not retire exactly its backing");
  const auto second_id = second->id();
  program.destroy_snode_tree(second);
  const auto program_retired = program.lifecycle_storage();
  require(program_retired.trees.slots == 2 &&
              program_retired.trees.capacity_bytes == program_roots.trees.capacity_bytes &&
              !program_retired.complete && !program_retired.pointee_storage_available,
          "retired backing erased retained Program slots or implied pointee coverage");
  require(impl.snode_root_record_storage_bytes() == 0,
          "root record metadata retained after destruction");
  const auto retired = impl.runtime_bookkeeping();
  require(retired.thread_pool.owner == empty.thread_pool.owner &&
              retired.thread_pool.object_bytes == empty.thread_pool.object_bytes &&
              retired.thread_pool.handle_capacity_bytes == empty.thread_pool.handle_capacity_bytes &&
              !retired.thread_pool.complete,
          "tree destruction changed retained pool storage or claimed complete thread coverage");
  std::cout << "thread_handles=" << retired.thread_pool.thread_handles
            << " thread_pool_object=" << retired.thread_pool.object_bytes
            << " thread_handle_capacity=" << retired.thread_pool.handle_capacity_bytes << '\n';
  require(!retired.root_records.entries &&
              retired.root_records.allocation_request_bytes == empty.root_records.allocation_request_bytes &&
              retired.runtime_allocations.entries == empty.runtime_allocations.entries + 1 &&
              retired.tree_allocations.entries == roots.tree_allocations.entries &&
              retired.tree_allocations.allocation_request_bytes == roots.tree_allocations.allocation_request_bytes &&
              !retired.complete,
          "root release lost retained descriptors or claimed full runtime coverage");
  std::cout << "root_map_requests=" << roots.root_records.allocation_request_bytes
            << " tree_map_requests=" << roots.tree_allocations.allocation_request_bytes
            << " retained_runtime_entries=" << retired.runtime_allocations.entries << '\n';
  std::cout << "root_request=" << a.requested_bytes << " array_request="
            << array_record.requested_bytes << " ndarray_object=" << sizeof(Ndarray) << '\n';
  auto *replacement = tree(program);
  const auto reused = program.lifecycle_storage();
  const auto replacement_backing = impl.snode_root_backing_record(replacement->id());
  require(replacement->id() == second_id &&
              reused.owner == program_roots.owner && reused.trees.slots == 2 &&
              reused.trees.capacity_bytes == program_roots.trees.capacity_bytes &&
              replacement_backing.status == AllocationRecordStatus::Live &&
              !(replacement_backing.token == a.token) && !(replacement_backing.token == b.token),
          "tree slot reuse duplicated vector storage or reused retired backing identity");
  program.destroy_snode_tree(replacement);
  std::cout << "program_object=" << reused.object_bytes
            << " tree_slots=" << reused.trees.slots
            << " tree_capacity=" << reused.trees.capacity_bytes << '\n';
}
}
int main() {
  try {
    vector_limits();
    array_views();
    native_owners(1);
    native_owners(2);
    std::cout << "owner_backing_test PASS\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
