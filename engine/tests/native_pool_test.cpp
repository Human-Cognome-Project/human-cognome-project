// Check raw reservation boundaries without writing/committing the large spans.
#define HCP_ALLOCATION_LIFETIME
#include "allocation_observation.h"
#include <cstdint>
#include <iostream>
#include <stdexcept>

#include "taichi/rhi/common/host_memory_pool.h"
#if defined(TI_PLATFORM_UNIX)
#include <unistd.h>
#else
#include "taichi/platform/windows/windows.h"
#endif

int main() {
  try {
    using taichi::lang::HostMemoryPool;
    std::size_t refused_bytes = SIZE_MAX;
    taichi::lang::RequestCountingAllocator<std::uint64_t> refused(refused_bytes);
    bool addition_refused = false, product_refused = false;
    try { (void)refused.allocate(1); }
    catch (const std::bad_alloc &) { addition_refused = true; }
    try { (void)refused.allocate(SIZE_MAX); }
    catch (const std::bad_alloc &) { product_refused = true; }
    if (!addition_refused || !product_refused || refused_bytes != SIZE_MAX)
      throw std::runtime_error("metadata allocator overflow changed accounting");
    auto &pool = HostMemoryPool::get_instance();
#if defined(TI_PLATFORM_UNIX)
    const auto os_page = static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
#else
    SYSTEM_INFO info{};
    GetSystemInfo(&info);
    const auto os_page = static_cast<std::size_t>(info.dwPageSize);
#endif
    if (os_page <= 1 || os_page == SIZE_MAX)
      throw std::runtime_error("invalid test OS page size");
    constexpr std::size_t span = std::size_t{1} << 30;
    constexpr std::size_t page = 4096;
    const auto baseline = pool.statistics();
    const auto empty_storage = pool.backing_storage();
    allocation_observation::calls = 0;
    allocation_observation::bytes = 0;
    allocation_observation::enabled = true;
    const auto first = reinterpret_cast<std::uintptr_t>(pool.allocate(span, page));
    allocation_observation::enabled = false;
    const auto first_new_bytes = allocation_observation::bytes.load();
    const auto first_storage = pool.backing_storage();
    std::cout << "first_observed_new_bytes=" << first_new_bytes
              << " first_map_bytes=" << first_storage.record_allocation_bytes
              << " empty_map_bytes=" << empty_storage.record_allocation_bytes
              << " first_vector_bytes=" << first_storage.allocator_chunk_capacity_bytes
              << " empty_vector_bytes=" << empty_storage.allocator_chunk_capacity_bytes
              << " allocator_object_bytes=" << first_storage.allocator_object_bytes
              << " payload_bytes=" << first_storage.record_payload_bytes << '\n';
    if (!empty_storage.record_storage_available || !empty_storage.allocator_storage_available ||
        empty_storage.allocator_chunk_capacity_bytes ||
        first_storage.allocator_object_bytes != sizeof(taichi::lang::UnifiedAllocator) ||
        first_storage.record_allocation_bytes < first_storage.record_payload_bytes ||
        first_storage.pool_instance != empty_storage.pool_instance)
      throw std::runtime_error("pool container storage disagrees with actual allocation requests");
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
        !shared_record.rounded_extent_available || shared_record.rounded_extent_bytes < span ||
        shared_record.rounded_extent_bytes % os_page ||
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
    const auto before_other = pool.backing_storage();
    void *other_exclusive = pool.allocate(19,page,true);
    const auto with_other = pool.backing_storage();
    const auto other_record = pool.capture_backing_record(other_exclusive,19);
    pool.release(17,exclusive);
    const auto after_release = pool.backing_storage();
    if (after_release.record_allocation_bytes >= with_other.record_allocation_bytes ||
        after_release.allocator_chunk_capacity_bytes != with_other.allocator_chunk_capacity_bytes ||
        with_other.record_allocation_bytes <= before_other.record_allocation_bytes)
      throw std::runtime_error("map release or retained vector capacity accounting incorrect");
    for (const auto request : {std::size_t{1}, os_page-1, os_page, os_page+1}) {
      void *mapping = pool.allocate(request,page,true);
      const auto record = pool.capture_backing_record(mapping,request);
      const auto expected = request <= os_page ? os_page : 2*os_page;
      if (record.status != PoolBackingStatus::Live || record.raw_requested_bytes != request ||
          !record.rounded_extent_available || record.rounded_extent_bytes != expected ||
          pool.backing_record(record.token).rounded_extent_bytes != expected ||
          (request < expected && pool.capture_backing_record(
              static_cast<char *>(mapping)+request,1).status != PoolBackingStatus::Unknown))
        throw std::runtime_error("rounded backing changed request containment or extent");
      // Isolated native mapping test: prove the final reported page is mapped.
      static_cast<volatile char *>(mapping)[expected-1] = 23;
      pool.release(request,mapping);
      if (pool.backing_record(record.token).status != PoolBackingStatus::Unknown)
        throw std::runtime_error("rounded mapping release retained identity");
    }
    const auto before_invalid = pool.statistics();
    bool overflow = false, zero = false;
    try { pool.allocate(SIZE_MAX,page,true); }
    catch (const std::overflow_error &) { overflow = true; }
    try { pool.allocate(0,page,true); }
    catch (const std::invalid_argument &) { zero = true; }
    if (!overflow || !zero || pool.statistics().allocation_calls != before_invalid.allocation_calls ||
        pool.statistics().raw_allocation_calls != before_invalid.raw_allocation_calls ||
        pool.statistics().reserved_bytes != before_invalid.reserved_bytes)
      throw std::runtime_error("invalid rounded request reached allocator accounting");
    std::cout << "os_page_bytes=" << os_page << " rounded_boundary_cases=4\n";
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
        pool.backing_storage().record_payload_bytes ||
        pool.backing_storage().record_allocation_bytes != empty_storage.record_allocation_bytes ||
        pool.backing_storage().allocator_chunk_capacity_bytes ||
        pool.backing_storage().allocator_object_bytes != empty_storage.allocator_object_bytes) {
      throw std::runtime_error("native pool reset lost cumulative accounting");
    }
    allocation_lifetime::begin(1);
    bool failed = false;
    try { pool.allocate(17,page,true); }
    catch (const std::bad_alloc &) { failed = true; }
    const auto counts = allocation_lifetime::end();
    if (!failed || counts.attempts != 1 || counts.live || pool.statistics().reserved_bytes ||
        pool.backing_storage().record_payload_bytes ||
        pool.backing_storage().record_allocation_bytes != empty_storage.record_allocation_bytes)
      throw std::runtime_error("failed raw metadata registration retained a mapping/record");
    exclusive = pool.allocate(17,page,true);
    if (pool.capture_backing_record(exclusive,17).status != PoolBackingStatus::Live ||
        pool.backing_record(second_record.token).status != PoolBackingStatus::Unknown)
      throw std::runtime_error("retry failed or reset revived old backing");
    pool.release(17,exclusive);
    const auto retained_storage = pool.backing_storage();
    if (retained_storage.record_allocation_bytes != empty_storage.record_allocation_bytes ||
        !retained_storage.allocator_chunk_capacity_bytes ||
        retained_storage.metadata_complete)
      throw std::runtime_error("empty pool lost retained allocator storage or claimed complete metadata");
    pool.reset();
    const auto before_witness = pool.backing_storage();
    allocation_observation::bytes = 0;
    allocation_observation::enabled = true;
    allocation_lifetime::begin();
    void *witness_mapping = pool.allocate(17,page,true);
    const auto witness = allocation_lifetime::end();
    allocation_observation::enabled = false;
    const auto after_witness = pool.backing_storage();
    const auto retained_requests = after_witness.record_allocation_bytes - before_witness.record_allocation_bytes +
                                   after_witness.allocator_chunk_capacity_bytes;
    if (before_witness.allocator_chunk_capacity_bytes || witness.live != retained_requests)
      throw std::runtime_error("retained pool metadata disagrees with live allocation witness");
    std::cout << "witness_cumulative_bytes=" << allocation_observation::bytes.load()
              << " witness_live_bytes=" << witness.live
              << " retained_metadata_requests=" << retained_requests << '\n';
    pool.release(17,witness_mapping);
    std::cout << "first_map_request_bytes=" << first_storage.record_allocation_bytes
              << " first_chunk_capacity_bytes=" << first_storage.allocator_chunk_capacity_bytes
              << " allocator_object_bytes=" << first_storage.allocator_object_bytes << '\n';
    std::cout << "PASS: native raw bounds, cursor/request accounting and reset\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "native_pool_test: " << e.what() << '\n';
    return 1;
  }
}
