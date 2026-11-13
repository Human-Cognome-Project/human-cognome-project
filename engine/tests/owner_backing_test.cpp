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
void free_id_requests() {
  allocation_lifetime::begin();
  std::size_t attempts = 0;
  {
    ProgramFreeIdStack ids;
    const auto empty = ids.storage();
    const auto accounted = [](const RuntimeContainerStorage &s) {
      return s.allocation_request_bytes + s.counter_object_bytes + s.counter_control_block_bytes;
    };
    require(empty.available && !empty.overflow && !empty.entries &&
                empty.allocation_domain && accounted(empty) == allocation_lifetime::counts.live,
            "empty free-ID deque requests differ from independent allocation witness");
    for (int i = 0; i < 4096; ++i) ids.push(i);
    const auto before_query = allocation_lifetime::counts.attempts;
    const auto grown = ids.storage();
    require(allocation_lifetime::counts.attempts == before_query &&
                grown.entries == 4096 && grown.value_payload_bytes == 4096 * sizeof(int) &&
                grown.allocation_domain == empty.allocation_domain &&
                grown.allocation_request_bytes > empty.allocation_request_bytes &&
                accounted(grown) == allocation_lifetime::counts.live,
            "grown free-ID deque omitted rebound requests or inspection allocated");
    for (int i = 4095; i >= 0; --i) {
      require(ids.top() == i, "free-ID accounting changed LIFO order");
      ids.pop();
    }
    const auto cleared = ids.storage();
    require(!cleared.entries && !cleared.value_payload_bytes &&
                cleared.allocation_request_bytes &&
                cleared.allocation_domain == empty.allocation_domain &&
                accounted(cleared) == allocation_lifetime::counts.live,
            "empty free-ID deque lost retained block/map requests");
    attempts = allocation_lifetime::counts.attempts;
  }
  const auto destroyed = allocation_lifetime::end();
  require(!destroyed.live && !destroyed.blocks, "free-ID allocator domain leaked at teardown");
  for (std::size_t fail = 1; fail <= attempts; ++fail) {
    allocation_lifetime::begin(fail);
    bool refused = false;
    try {
      ProgramFreeIdStack ids;
      for (int i = 0; i < 4096; ++i) ids.push(i);
    } catch (const std::bad_alloc &) {
      refused = true;
    }
    const auto result = allocation_lifetime::end();
    require(refused && !result.live && !result.blocks,
            "free-ID constructor/growth failure leaked requests");
  }
  std::cout << "free_id_failure_controls=" << attempts << '\n';
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
void snode_host_storage() {
  auto root = std::make_unique<SNode>(0, SNodeType::root);
  auto &dense = root->dense(Axis(0), 7);
  auto &leaf = dense.insert_children(SNodeType::place);
  leaf.dt = PrimitiveType::i32;
  root->name.assign(96, 'r');
  leaf.node_type_name.assign(96, 'l');
  leaf.index_offsets = {3};
  SNodeTree tree(0, std::move(root));
  const auto before = tree.root()->host_storage();
  tree.root()->ch.reserve(19);
  leaf.index_offsets.reserve(23);
  allocation_lifetime::begin();
  const auto tree_storage = tree.host_storage();
  const auto root_storage = tree.root()->host_storage();
  const auto dense_storage = dense.host_storage();
  const auto leaf_storage = leaf.host_storage();
  const auto counts = allocation_lifetime::end();
  require(!counts.attempts && tree_storage.available && !tree_storage.complete &&
              tree_storage.owner == &tree && tree_storage.object_bytes == sizeof(SNodeTree) &&
              tree_storage.root_identity == tree.root() &&
              tree_storage.owner != root_storage.owner,
          "tree host query allocated, lost identity or claimed complete coverage");
  require(root_storage.available && !root_storage.overflow && !root_storage.complete &&
              !root_storage.string_storage_available && !root_storage.gradient_storage_available &&
              root_storage.owner == tree.root() && root_storage.object_bytes == sizeof(SNode) &&
              root_storage.child_slots == before.child_slots && root_storage.child_slots == 1 &&
              root_storage.child_capacity_bytes >= 19 * sizeof(std::unique_ptr<SNode>) &&
              root_storage.child_capacity_bytes > before.child_capacity_bytes &&
              !root_storage.index_offset_entries,
          "root host query lost capacity or included recursive child objects");
  require(dense_storage.owner == &dense && dense_storage.owner != root_storage.owner &&
              dense_storage.child_slots == 1 && leaf_storage.owner == &leaf &&
              leaf_storage.available && !leaf_storage.overflow && !leaf_storage.complete &&
              !leaf_storage.string_storage_available && !leaf_storage.gradient_storage_available &&
              !leaf_storage.child_slots && !leaf_storage.child_capacity_bytes &&
              leaf_storage.index_offset_entries == 1 &&
              leaf_storage.index_offset_capacity_bytes >= 23 * sizeof(int),
          "node metadata confused empty children, offset capacity or unknown nested storage");
  leaf.index_offsets.clear();
  const auto cleared = leaf.host_storage();
  require(!cleared.index_offset_entries &&
              cleared.index_offset_capacity_bytes == leaf_storage.index_offset_capacity_bytes,
          "empty offsets lost retained capacity");
  std::cout << "snode_object=" << root_storage.object_bytes
            << " snode_tree_object=" << tree_storage.object_bytes
            << " child_capacity=" << root_storage.child_capacity_bytes
            << " offset_capacity=" << leaf_storage.index_offset_capacity_bytes << '\n';
}
void array_views() {
  cpu::CpuDevice device, foreign;
  auto allocation = allocate(device, 64);
  Ndarray full(allocation, PrimitiveType::i32, {16});
  Ndarray part(allocation, PrimitiveType::i32, {4});
  Ndarray tensor(allocation, PrimitiveType::i32, {2}, {2, 2}, ExternalArrayLayout::kAOS);
  Ndarray scalar(allocation, PrimitiveType::i32, {}, ExternalArrayLayout::kNull,
                 DebugInfo(std::string(96, 'd')));
  const auto full_storage = full.host_storage();
  const auto part_storage = part.host_storage();
  const auto tensor_storage = tensor.host_storage();
  const auto scalar_storage = scalar.host_storage();
  require(scalar_storage.available && !scalar_storage.shape_entries &&
              !scalar_storage.total_shape_entries &&
              !scalar_storage.debug_storage_available && !scalar_storage.complete,
          "zero-rank storage or nonempty debug information falsely claimed complete coverage");
  require(full_storage.available && !full_storage.overflow && !full_storage.complete &&
              !full_storage.debug_storage_available &&
              full_storage.owner == &full && part_storage.owner == &part &&
              full_storage.owner != part_storage.owner &&
              full_storage.object_bytes == sizeof(Ndarray) &&
              full_storage.shape_entries == 1 && full_storage.total_shape_entries == 1 &&
              full_storage.shape_capacity_bytes >= sizeof(int) &&
              full_storage.total_shape_capacity_bytes >= sizeof(int) &&
              tensor_storage.shape_entries == 1 && tensor_storage.total_shape_entries == 3 &&
              tensor_storage.total_shape_capacity_bytes >= 3 * sizeof(int),
          "Ndarray host storage lost owner identity, nested shape capacity or coverage boundary");
  full.shape.reserve(19);
  allocation_lifetime::begin();
  const auto grown_storage = full.host_storage();
  const auto host_query_counts = allocation_lifetime::end();
  require(!host_query_counts.attempts && grown_storage.owner == full_storage.owner &&
              grown_storage.shape_entries == full_storage.shape_entries &&
              grown_storage.shape_capacity_bytes >= 19 * sizeof(int) &&
              grown_storage.total_shape_capacity_bytes == full_storage.total_shape_capacity_bytes &&
              part.host_storage().shape_capacity_bytes == part_storage.shape_capacity_bytes,
          "Ndarray host inspection allocated or conflated separate shape buffers");
  const auto original = full.backing_record(device);
  require(original.status == AllocationRecordStatus::Live &&
              original.requested_bytes == 64 &&
              part.backing_record(device).token == original.token &&
              part.backing_record(device).extent_bytes == 64,
          "view shape replaced actual backing extent or identity");
  require(tensor.backing_record(device).token == original.token,
          "tensor wrapper changed shared backing identity");
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
  require(full.host_storage().owner == grown_storage.owner &&
              full.host_storage().shape_capacity_bytes == grown_storage.shape_capacity_bytes,
          "retired backing incorrectly retired live wrapper metadata");
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
              program_empty.map_storage_available &&
              program_empty.free_id_storage_available &&
              program_empty.free_ids.available && !program_empty.free_ids.entries &&
              !program_empty.pointee_storage_available,
          "Program empty coverage confused unavailable families with zero storage");
  require(impl.launch_registry_storage().status == LaunchMetadataStatus::Absent,
          "Program inspection created a launcher");
  require(program_empty.field_expressions.available && program_empty.accessors.available &&
              !program_empty.field_expressions.entries && !program_empty.accessors.entries,
          "empty field/accessor maps unavailable");
  const void *domains[] = {program_empty.function_registry.allocation_domain,
                          program_empty.ndarray_registry.allocation_domain,
                          program_empty.argpack_registry.allocation_domain,
                          program_empty.field_expressions.allocation_domain,
                          program_empty.accessors.allocation_domain};
  for (int i = 0; i < 5; ++i) {
    require(domains[i], "Program map domain missing");
    for (int j = 0; j < i; ++j)
      require(domains[i] != domains[j], "independent Program maps share a domain");
  }
  const auto &fm = program_empty.function_registry;
  const auto &nm = program_empty.ndarray_registry;
  const auto &am = program_empty.argpack_registry;
  require(fm.available && nm.available && am.available &&
              !fm.entries && !nm.entries && !am.entries &&
              !fm.overflow && !nm.overflow && !am.overflow &&
              fm.allocation_domain && nm.allocation_domain && am.allocation_domain &&
              fm.allocation_domain != nm.allocation_domain &&
              fm.allocation_domain != am.allocation_domain &&
              nm.allocation_domain != am.allocation_domain &&
              fm.counter_object_bytes && fm.counter_control_block_bytes &&
              nm.counter_object_bytes && nm.counter_control_block_bytes &&
              am.counter_object_bytes && am.counter_control_block_bytes,
          "empty Program registries lost independent allocator domains or coverage");
  program.kernel([](Kernel *) {}, "storage_probe");
  program.create_function(FunctionKey("storage_function", 17, 0));
  const auto definitions = program.lifecycle_storage();
  require(definitions.function_registry.entries == 1 &&
              definitions.function_registry.allocation_domain == fm.allocation_domain &&
              definitions.function_registry.allocation_request_bytes > fm.allocation_request_bytes &&
              definitions.function_registry.allocation_request_bytes >=
                  definitions.function_registry.value_payload_bytes,
          "Program function registry omitted node/bucket requests");
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
  const auto first_host = first->host_storage();
  const auto first_root_host = first->root()->host_storage();
  require(first_host.owner != second->host_storage().owner &&
              first_host.root_identity != second->host_storage().root_identity,
          "distinct native trees shared host identities");
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
                queried_program.function_registry.allocation_domain == fm.allocation_domain &&
                queried_program.function_registry.allocation_request_bytes ==
                    definitions.function_registry.allocation_request_bytes &&
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
  require(first->host_storage().owner == first_host.owner &&
              first->host_storage().root_identity == first_host.root_identity &&
              first->root()->host_storage().owner == first_root_host.owner &&
              first->root()->host_storage().child_capacity_bytes == first_root_host.child_capacity_bytes,
          "backing retirement erased retained tree/node host storage");
  require(impl.snode_root_backing_record(first_id).status == AllocationRecordStatus::Unknown &&
              device.allocation_record(a.token).status == AllocationRecordStatus::Retired &&
              impl.snode_root_backing_record(second->id()).token == b.token,
          "root destruction did not retire exactly its backing");
  const auto second_id = second->id();
  program.destroy_snode_tree(second);
  const auto program_retired = program.lifecycle_storage();
  require(program_retired.trees.slots == 2 &&
              program_retired.free_ids.entries == 2 &&
              program_retired.free_ids.value_payload_bytes == 2 * sizeof(int) &&
              program_retired.free_ids.allocation_domain == program_empty.free_ids.allocation_domain &&
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
              reused.free_ids.entries == 1 &&
              reused.free_ids.allocation_domain == program_empty.free_ids.allocation_domain &&
              reused.owner == program_roots.owner && reused.trees.slots == 2 &&
              reused.trees.capacity_bytes == program_roots.trees.capacity_bytes &&
              replacement_backing.status == AllocationRecordStatus::Live &&
              !(replacement_backing.token == a.token) && !(replacement_backing.token == b.token),
          "tree slot reuse duplicated vector storage or reused retired backing identity");
  program.destroy_snode_tree(replacement);
  auto *registered_a = program.create_ndarray(PrimitiveType::i32, {7});
  auto *registered_b = program.create_ndarray(PrimitiveType::i32, {7});
  const auto owned_wrapper = registered_a->host_storage();
  require(owned_wrapper.owner == registered_a && owned_wrapper.available &&
              !owned_wrapper.complete && owned_wrapper.object_bytes == sizeof(Ndarray) &&
              owned_wrapper.shape_entries == 1 && owned_wrapper.total_shape_entries == 1 &&
              owned_wrapper.owner != registered_b->host_storage().owner,
          "Program-owned wrappers lost distinct host identities");
  const auto registered = program.lifecycle_storage();
  require(registered.ndarray_registry.entries == 2 &&
              registered.ndarray_registry.allocation_domain == nm.allocation_domain &&
              registered.ndarray_registry.allocation_request_bytes > nm.allocation_request_bytes &&
              registered.ndarray_registry.allocation_request_bytes >=
                  registered.ndarray_registry.value_payload_bytes &&
              registered_a != registered_b,
          "Program ndarray registry omitted distinct owning rows");
  program.delete_ndarray(registered_a);
  const auto one_array = program.lifecycle_storage();
  program.delete_ndarray(registered_b);
  allocation_lifetime::begin();
  const auto cleared = program.lifecycle_storage();
  const auto registry_query_counts = allocation_lifetime::end();
  require(!registry_query_counts.attempts &&
              one_array.ndarray_registry.entries == 1 &&
              !cleared.ndarray_registry.entries &&
              !cleared.ndarray_registry.value_payload_bytes &&
              cleared.ndarray_registry.allocation_request_bytes > nm.allocation_request_bytes &&
              cleared.ndarray_registry.allocation_request_bytes <
                  one_array.ndarray_registry.allocation_request_bytes &&
              one_array.ndarray_registry.allocation_request_bytes <
                  registered.ndarray_registry.allocation_request_bytes &&
              cleared.ndarray_registry.allocation_domain == nm.allocation_domain &&
              cleared.map_storage_available && !cleared.pointee_storage_available && !cleared.complete,
          "registry erasure lost retained buckets or claimed complete Program ownership");
  std::cout << "function_registry_requests=" << definitions.function_registry.allocation_request_bytes
            << " ndarray_registry_requests=" << registered.ndarray_registry.allocation_request_bytes
            << " empty_ndarray_registry_requests=" << cleared.ndarray_registry.allocation_request_bytes << '\n';
  std::cout << "program_object=" << reused.object_bytes
            << " tree_slots=" << reused.trees.slots
            << " tree_capacity=" << reused.trees.capacity_bytes << '\n';
  auto field_root = std::make_unique<SNode>(0, SNodeType::root);
  auto &field_dense = field_root->dense(Axis(0), 7);
  auto expression = Expr::make<FieldExpression>(PrimitiveType::i32,
      program.get_next_global_id("accounted_field"));
  place_child(&expression, {}, -1, &field_dense, program.get_snode_to_fields());
  auto *field_node = expression.snode();
  auto *owned_field_tree = program.add_snode_tree(std::move(field_root), false);
  auto accessor = program.get_snode_rw_accessors_bank().get(field_node);
  allocation_lifetime::begin();
  const auto field_maps = program.lifecycle_storage();
  const auto direct_accessor = program.get_snode_rw_accessors_bank().storage();
  const auto field_query_counts = allocation_lifetime::end();
  require(!field_query_counts.attempts && field_maps.map_storage_available &&
              field_maps.field_expressions.entries == 1 && field_maps.accessors.entries == 1 &&
              field_maps.field_expressions.allocation_domain == domains[3] &&
              field_maps.accessors.allocation_domain == domains[4] &&
              field_maps.accessors.allocation_domain == direct_accessor.allocation_domain &&
              field_maps.accessors.allocation_request_bytes == direct_accessor.allocation_request_bytes &&
              field_maps.field_expressions.allocation_request_bytes >
                  program_empty.field_expressions.allocation_request_bytes &&
              field_maps.accessors.allocation_request_bytes > program_empty.accessors.allocation_request_bytes &&
              !field_maps.pointee_storage_available && !field_maps.complete,
          "field/accessor map inspection allocated, aliased owners or implied pointee coverage");
  accessor.write_int({2}, 37);
  require(accessor.read_int({2}) == 37, "native accessor execution changed");
  program.destroy_snode_tree(owned_field_tree);
  const auto field_retired = program.lifecycle_storage();
  require(!field_retired.accessors.entries && field_retired.field_expressions.entries == 1 &&
              field_retired.kernels.slots == field_maps.kernels.slots &&
              field_retired.accessors.allocation_request_bytes > program_empty.accessors.allocation_request_bytes &&
              field_retired.accessors.allocation_request_bytes < field_maps.accessors.allocation_request_bytes,
          "tree retirement lost retained field rows/kernels or accessor buckets");
  program.get_snode_to_fields()->erase(field_node);
  const auto field_erased = program.lifecycle_storage();
  require(!field_erased.field_expressions.entries &&
              field_erased.field_expressions.allocation_request_bytes >
                  program_empty.field_expressions.allocation_request_bytes &&
              field_erased.field_expressions.allocation_request_bytes <
                  field_maps.field_expressions.allocation_request_bytes &&
              field_erased.field_expressions.allocation_domain == domains[3],
          "explicit field-map erasure lost retained bucket accounting");
  std::cout << "field_map_requests=" << field_maps.field_expressions.allocation_request_bytes
            << " accessor_map_requests=" << field_maps.accessors.allocation_request_bytes
            << " retained_accessor_requests=" << field_retired.accessors.allocation_request_bytes << '\n';
}
}
int main() {
  try {
    snode_host_storage();
    vector_limits();
    free_id_requests();
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
