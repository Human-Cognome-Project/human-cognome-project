#pragma once
#include <cstdint>
#include <iostream>
#include <vector>
#include "engine/runtime.h"
#include "taichi/jit/jit_module.h"
#include "taichi/rhi/common/host_memory_pool.h"
#include "taichi/rhi/llvm/device_memory_pool.h"
#include "taichi/runtime/program_impls/llvm/llvm_program.h"

namespace prepared_test {
using namespace taichi::lang;
struct Sample {
  MemoryPoolStatistics pool{};
  std::uint64_t requested{}, chunks{}, elements{};
  std::size_t cursor{};
  int trees{}, snodes{};
  double compilation{};
  bool same_backing(const Sample &b) const {
    return requested == b.requested && chunks == b.chunks &&
           elements == b.elements && cursor == b.cursor &&
           pool.allocation_calls == b.pool.allocation_calls &&
           pool.requested_bytes == b.pool.requested_bytes &&
           pool.raw_allocation_calls == b.pool.raw_allocation_calls &&
           pool.reserved_bytes == b.pool.reserved_bytes && trees == b.trees &&
           snodes == b.snodes && compilation == b.compilation;
  }
  void print(const char *phase) const {
    std::cout << phase << ',' << requested << ',' << chunks << ',' << elements
              << ',' << pool.allocation_calls << ',' << pool.requested_bytes
              << ',' << pool.raw_allocation_calls << ',' << pool.reserved_bytes
              << ',' << cursor << ',' << trees << ',' << snodes << ','
              << compilation << '\n';
  }
};
struct Metrics {
  engine::Runtime &runtime;
  Program &program;
  LlvmProgramImpl &impl;
  std::vector<SNode *> nodes;
  Metrics(engine::Runtime &r, std::vector<SNode *> n)
      : runtime(r),
        program(r.program()),
        impl(*static_cast<LlvmProgramImpl *>(program.get_program_impl())),
        nodes(std::move(n)) {
  }
  template <class T, class... Args>
  T query(const std::string &key, Args... args) {
    auto *exec = impl.get_runtime_executor();
    exec->get_runtime_jit_module()->call<void *>(
        "runtime_" + key, exec->get_llvm_runtime(), args...);
    return impl.fetch_result<T>(taichi_result_buffer_runtime_query_id,
                                program.result_buffer);
  }
  std::uint64_t list_bytes(void *list) {
    if (!list)
      return 0;
    return query<int>("ListManager_get_num_active_chunks", list) *
           query<std::uint64_t>("ListManager_get_element_size", list) *
           query<std::uint64_t>("ListManager_get_max_num_elements_per_chunk",
                                list);
  }
  Sample sample() {
    runtime.synchronize();
    Sample s;
    const auto host = HostMemoryPool::get_instance().statistics();
    s.pool = runtime.on_cuda() ? DeviceMemoryPool::get_instance().statistics()
                               : static_cast<MemoryPoolStatistics>(host);
    s.cursor = host.cursor_bytes;
    auto *rt = impl.get_llvm_runtime();
    s.requested =
        query<std::uint64_t>("LLVMRuntime_get_total_requested_memory", rt);
    for (auto *node : nodes) {
      s.chunks += list_bytes(
          query<void *>("LLVMRuntime_get_element_lists", rt, node->id));
      auto *allocator =
          query<void *>("LLVMRuntime_get_node_allocators", rt, node->id);
      if (allocator)
        for (const auto *list :
             {"NodeManager_get_data_list", "NodeManager_get_free_list",
              "NodeManager_get_recycled_list"}) {
          auto *ptr = query<void *>(list, allocator);
          s.chunks += list_bytes(ptr);
          if (std::string(list) == "NodeManager_get_data_list")
            s.elements += query<int>("ListManager_get_num_elements", ptr);
        }
    }
    s.trees = program.get_snode_tree_size();
    s.snodes = runtime.issued_snode_ids();
    s.compilation = program.get_total_compilation_time();
    return s;
  }
};
}  // namespace prepared_test
