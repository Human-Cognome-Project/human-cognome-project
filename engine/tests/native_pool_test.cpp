// Check raw reservation boundaries without writing/committing the large spans.
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
    if (!(second + page <= first || second >= first + span) ||
        next.raw_allocation_calls != baseline.raw_allocation_calls + 2 ||
        next.reserved_bytes != baseline.reserved_bytes + 2 * span ||
        full.cursor_bytes != baseline.cursor_bytes + span ||
        next.cursor_bytes != baseline.cursor_bytes + span + page ||
        next.allocation_calls != baseline.allocation_calls + 2 ||
        next.requested_bytes != baseline.requested_bytes + span + page) {
      throw std::runtime_error("native pool reused bytes outside a raw reservation");
    }
    pool.reset();
    const auto reset = pool.statistics();
    if (reset.reserved_bytes != 0 || reset.cursor_bytes != 0 ||
        reset.allocation_calls != next.allocation_calls ||
        reset.raw_allocation_calls != next.raw_allocation_calls) {
      throw std::runtime_error("native pool reset lost cumulative accounting");
    }
    std::cout << "PASS: native raw bounds, cursor/request accounting and reset\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "native_pool_test: " << e.what() << '\n';
    return 1;
  }
}
