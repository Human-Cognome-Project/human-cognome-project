// Check raw reservation boundaries without writing/committing the large spans.
#define HCP_ALLOCATION_LIFETIME
#include "allocation_observation.h"
#include <cstdint>
#include <iostream>
#include <stdexcept>

#include "taichi/rhi/common/host_memory_pool.h"

int main() {
  try {
    using taichi::lang::HostMemoryPool;
    auto &pool = HostMemoryPool::get_instance();
    constexpr std::size_t span = std::size_t{1} << 30;
    constexpr std::size_t page = 4096;
    const auto baseline = pool.statistics();
    const auto first = reinterpret_cast<std::uintptr_t>(pool.allocate(span, page));
    const auto full = pool.statistics();
    const auto second = reinterpret_cast<std::uintptr_t>(pool.allocate(page, page));
    const auto next = pool.statistics();
    using taichi::lang::PoolBackingStatus;
    const auto shared = pool.allocate(32,128);
    allocation_observation::calls = 0;
    allocation_observation::enabled = true;
    const auto first_record = pool.capture_backing_record(reinterpret_cast<void *>(first),span);
    const auto second_record = pool.capture_backing_record(reinterpret_cast<void *>(second),page);
    const auto shared_record = pool.capture_backing_record(shared,32);
    const auto validated = pool.backing_record(second_record.token);
    const auto storage = pool.backing_storage();
    allocation_observation::enabled = false;
    if (allocation_observation::calls || first_record.status != PoolBackingStatus::Live ||
        second_record.status != PoolBackingStatus::Live ||
        shared_record.token == first_record.token || !(shared_record.token == second_record.token) ||
        !(validated.token == second_record.token) || shared_record.raw_requested_bytes != span ||
        shared_record.rounded_extent_available || shared_record.rounded_extent_bytes ||
        !storage.available || storage.metadata_complete || !storage.incremental_record_payload_bytes ||
        storage.incremental_record_payload_bytes >= storage.record_payload_bytes ||
        pool.capture_backing_record(reinterpret_cast<void *>(first+span-1),2).status != PoolBackingStatus::Unknown ||
        pool.capture_backing_record(shared,SIZE_MAX).status != PoolBackingStatus::Unknown ||
        pool.capture_backing_record(nullptr,1).status != PoolBackingStatus::Unknown ||
        pool.capture_backing_record(shared,0).status != PoolBackingStatus::Unknown) {
      throw std::runtime_error("pool backing identity/range/metadata/query allocation incorrect");
    }
    auto wrong = shared_record.token;
    ++wrong.pool_instance;
    if (pool.backing_record(wrong).status != PoolBackingStatus::Unknown)
      throw std::runtime_error("foreign pool token accepted");
    void *exclusive = pool.allocate(17,page,true);
    const auto exclusive_record = pool.capture_backing_record(exclusive,17);
    void *other_exclusive = pool.allocate(19,page,true);
    const auto other_record = pool.capture_backing_record(other_exclusive,19);
    pool.release(17,exclusive);
    exclusive = pool.allocate(17,page,true);
    if (pool.backing_record(exclusive_record.token).status != PoolBackingStatus::Unknown ||
        pool.capture_backing_record(exclusive,17).token == exclusive_record.token ||
        !(pool.backing_record(second_record.token).token == second_record.token) ||
        !(pool.backing_record(other_record.token).token == other_record.token))
      throw std::runtime_error("release/reuse or chunk movement corrupted backing identity");
    pool.release(17,exclusive);
    pool.release(19,other_exclusive);
    if (!(second + page <= first || second >= first + span) ||
        next.raw_allocation_calls != baseline.raw_allocation_calls + 2 ||
        next.reserved_bytes != baseline.reserved_bytes + 2 * span ||
        full.cursor_bytes != baseline.cursor_bytes + span ||
        next.cursor_bytes != baseline.cursor_bytes + span + page ||
        next.allocation_calls != baseline.allocation_calls + 2 ||
        next.requested_bytes != baseline.requested_bytes + span + page) {
      throw std::runtime_error("native pool reused bytes outside a raw reservation");
    }
    const auto before_reset = pool.statistics();
    pool.reset();
    const auto reset = pool.statistics();
    if (reset.reserved_bytes != 0 || reset.cursor_bytes != 0 ||
        reset.allocation_calls != before_reset.allocation_calls ||
        reset.raw_allocation_calls != before_reset.raw_allocation_calls ||
        pool.backing_record(second_record.token).status != PoolBackingStatus::Unknown ||
        pool.backing_storage().record_payload_bytes) {
      throw std::runtime_error("native pool reset lost cumulative accounting");
    }
    allocation_lifetime::begin(1);
    bool failed = false;
    try { pool.allocate(17,page,true); }
    catch (const std::bad_alloc &) { failed = true; }
    const auto counts = allocation_lifetime::end();
    if (!failed || counts.attempts != 1 || counts.live || pool.statistics().reserved_bytes ||
        pool.backing_storage().record_payload_bytes)
      throw std::runtime_error("failed raw metadata registration retained a mapping/record");
    exclusive = pool.allocate(17,page,true);
    if (pool.capture_backing_record(exclusive,17).status != PoolBackingStatus::Live ||
        pool.backing_record(second_record.token).status != PoolBackingStatus::Unknown)
      throw std::runtime_error("retry failed or reset revived old backing");
    pool.release(17,exclusive);
    std::cout << "PASS: native raw bounds, cursor/request accounting and reset\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "native_pool_test: " << e.what() << '\n';
    return 1;
  }
}
