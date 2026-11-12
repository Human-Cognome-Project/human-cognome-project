#include "allocation_observation.h"
#include "engine/runtime.h"
#include "taichi/rhi/common/host_memory_pool.h"
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace taichi::lang;
namespace {
void require(bool ok, const char *message) {
  if (!ok) throw std::runtime_error(message);
}
std::uint64_t exercise() {
  engine::InstanceSettings settings;
  settings.cpu_threads = 1;
  settings.snode_capacity = 128;
  settings.snode_tree_capacity = 32;
  engine::Runtime runtime(settings);
  auto &program = runtime.program();
  auto &owner = *program.get_program_impl();
  const auto limits = owner.reservation_limits();
  require(limits.available && limits.node_capacity == 128 && limits.query_work_units == 1 &&
              limits.list_pages == reservation_max_pages && limits.list_chunks == reservation_max_chunks &&
              limits.incremental_owner_bytes == 2 * sizeof(void *), "reservation limits/metadata incorrect");
  require(!owner.ProgramImpl::reservation_limits().available &&
              owner.ProgramImpl::reservation_request(ReservationKind::Ambient, 0).status ==
                  ReservationStatus::Unavailable, "unsupported default claimed zero coverage");
  allocation_observation::calls = 0;
  allocation_observation::enabled = true;
  const auto metadata = owner.reservation_request(ReservationKind::AmbientMetadata, -1);
  allocation_observation::enabled = false;
  require(!allocation_observation::calls && metadata.status == ReservationStatus::Present &&
              metadata.requested_bytes == 128 * sizeof(std::uint64_t) && metadata.alignment == 4096 &&
              !metadata.backing_identity_complete && !metadata.rounded_extent_available && !metadata.extent_bytes,
          "ambient request metadata missing, allocated during query, or certified rounded backing");
  allocation_observation::calls = 0;
  allocation_observation::enabled = true;
  const auto metadata_pool = owner.reservation_pool_backing(ReservationKind::AmbientMetadata,-1);
  allocation_observation::enabled = false;
  require(!allocation_observation::calls && metadata_pool.status == PoolBackingStatus::Live &&
              metadata_pool.raw_requested_bytes >= metadata.requested_bytes &&
              metadata_pool.rounded_extent_available &&
              metadata_pool.rounded_extent_bytes >= metadata_pool.raw_requested_bytes &&
              owner.reservation_pool_backing(ReservationKind::Ambient,127).status == PoolBackingStatus::Absent &&
              owner.reservation_pool_backing(ReservationKind::Ambient,-1).status == PoolBackingStatus::Unknown &&
              owner.ProgramImpl::reservation_pool_backing(ReservationKind::Ambient,-1).status == PoolBackingStatus::Unavailable,
          "native request pool bridge incorrect or allocated");
  require(owner.reservation_request(ReservationKind::Ambient, 127).status == ReservationStatus::Absent &&
              owner.reservation_request(ReservationKind::Ambient, -1).status == ReservationStatus::Invalid &&
              owner.reservation_request(ReservationKind::Ambient, 128).status == ReservationStatus::Invalid &&
              owner.reservation_request(static_cast<ReservationKind>(99), 0).status == ReservationStatus::Invalid,
          "absent and invalid requests conflated");

  auto root = std::make_unique<SNode>(0, SNodeType::root);
  const int root_id = root->id;
  auto &pointer = root->pointer(Axis(0), 4);
  const int node = pointer.id;
  auto &leaf = pointer.dense(Axis(0), 8).insert_children(SNodeType::place);
  leaf.dt = PrimitiveType::i32;
  auto *tree = program.add_snode_tree(std::move(root), false);
  const auto query = [&](ReservationKind kind, ReservationList list = ReservationList::Elements,
                         std::uint64_t index = 0) {
    return owner.reservation_request(kind, node, list, index);
  };
  const auto manager = query(ReservationKind::NodeManager);
  const auto ambient = query(ReservationKind::Ambient);
  require(manager.status == ReservationStatus::Present && ambient.status == ReservationStatus::Present &&
              ambient.requested_bytes && ambient.alignment == 128 && manager.alignment == 4096 &&
              manager.address != ambient.address && manager.runtime_instance == metadata.runtime_instance,
          "node/ambient requests missing or conflated");
  for (const auto role : {ReservationList::Elements, ReservationList::Free,
                         ReservationList::Recycled, ReservationList::Data}) {
    require(query(ReservationKind::ListObject, role).status == ReservationStatus::Present,
            "list owner request missing");
  }
  const auto free = query(ReservationKind::ListObject, ReservationList::Free);
  const auto recycled = query(ReservationKind::ListObject, ReservationList::Recycled);
  require(free.requested_bytes == recycled.requested_bytes && free.address != recycled.address,
          "equal-sized list objects collapsed");
  require(query(ReservationKind::ListChunk, ReservationList::Data).status == ReservationStatus::Absent &&
              query(ReservationKind::ListPage, ReservationList::Data).status == ReservationStatus::Absent,
          "untouched sparse capacity fabricated a request");
  require(query(ReservationKind::ListPage, ReservationList::Data, limits.list_pages).status == ReservationStatus::Invalid &&
              query(ReservationKind::ListChunk, ReservationList::Data, std::numeric_limits<std::uint64_t>::max()).status == ReservationStatus::Invalid &&
              query(ReservationKind::ListObject, static_cast<ReservationList>(99)).status == ReservationStatus::Invalid,
          "invalid sparse directory selector accepted");
  require(owner.reservation_request(ReservationKind::ListChunk, root_id).status == ReservationStatus::Present,
          "root element-list chunk not exported");
  IRBuilder ir;
  ir.create_global_store(ir.create_global_ptr(&leaf, {ir.get_int32(0)}), ir.get_int32(7));
  engine::CompiledKernel activate(runtime, "reservation_activate", ir.extract_ir(), {});
  auto context = activate.make_context();
  const auto before = HostMemoryPool::get_instance().statistics();
  activate.launch(context);
  runtime.synchronize();
  const auto after = HostMemoryPool::get_instance().statistics();
  const auto page = query(ReservationKind::ListPage, ReservationList::Data);
  const auto chunk = query(ReservationKind::ListChunk, ReservationList::Data);
  require(page.status == ReservationStatus::Present && chunk.status == ReservationStatus::Present &&
              page.requested_bytes == reservation_chunks_per_page * sizeof(void *) &&
              page.address != chunk.address && chunk.requested_bytes == ambient.requested_bytes * 16384 &&
              after.requested_bytes - before.requested_bytes == page.requested_bytes + chunk.requested_bytes &&
              after.allocation_calls - before.allocation_calls == 2,
          "exported page/chunk requests disagree with actual allocator requests");
  allocation_observation::calls = 0;
  allocation_observation::enabled = true;
  const auto repeat = query(ReservationKind::ListChunk, ReservationList::Data);
  const auto empty = query(ReservationKind::ListChunk, ReservationList::Data, limits.list_chunks - 1);
  allocation_observation::enabled = false;
  require(!allocation_observation::calls && repeat.address == chunk.address &&
              empty.status == ReservationStatus::Absent && !empty.requested_bytes,
          "query allocated, mutated backing, or treated absent capacity as present");
  activate.launch(context);
  runtime.synchronize();
  require(HostMemoryPool::get_instance().statistics().allocation_calls == after.allocation_calls &&
              query(ReservationKind::ListChunk, ReservationList::Data).address == chunk.address,
          "repeated activation changed retained reservation");
  program.destroy_snode_tree(tree);
  require(query(ReservationKind::Ambient).address == ambient.address &&
              query(ReservationKind::ListChunk, ReservationList::Data).address == chunk.address &&
              owner.reservation_request(ReservationKind::AmbientMetadata, -1).address == metadata.address,
          "tree removal hid still-retained runtime requests");
  std::cout << "ambient_request=" << ambient.requested_bytes << " page_request=" << page.requested_bytes
            << " chunk_request=" << chunk.requested_bytes << " metadata_request=" << metadata.requested_bytes << '\n';
  return metadata.runtime_instance;
}
}
int main() {
  try {
    const auto former = exercise();
    engine::InstanceSettings settings;
    settings.cpu_threads = 1;
    engine::Runtime next(settings);
    const auto now = next.program().get_program_impl()->reservation_request(ReservationKind::AmbientMetadata, -1);
    require(now.status == ReservationStatus::Present && now.runtime_instance != former,
            "new runtime reused former owner identity");
    std::cout << "reservation_request_test PASS\n";
    return 0;
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
