#pragma once

#include <deque>
#include <stack>
#include "taichi/program/runtime_metadata.h"
#include "taichi/rhi/common/shared_request_counting_allocator.h"

namespace taichi::lang {

// Preserve std::stack's deque/LIFO behavior; expose only its allocator metadata.
class ProgramFreeIdStack
    : public std::stack<int, std::deque<int, SharedRequestCountingAllocator<int>>> {
 public:
  RuntimeContainerStorage storage() const {
    return runtime_container_storage(c);
  }
};

}  // namespace taichi::lang
