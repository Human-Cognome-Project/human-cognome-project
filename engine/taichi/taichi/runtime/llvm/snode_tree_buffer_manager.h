#pragma once
#include "taichi/inc/constants.h"
#include "taichi/struct/snode_tree.h"
#include "taichi/rhi/public_device.h"
#define TI_RUNTIME_HOST

#include <set>

using Ptr = uint8_t *;

namespace taichi::lang {

class JITModule;
class LlvmRuntimeExecutor;

class SNodeTreeBufferManager {
 public:
  explicit SNodeTreeBufferManager(LlvmRuntimeExecutor *runtime_exec);

  Ptr allocate(std::size_t size,
               const int snode_tree_id,
               uint64 *result_buffer);

  void destroy(SNodeTree *snode_tree);

  // Root only: node/list reservations are separate, still uncovered families.
  // The manager/runtime must be alive and quiescent for inspection.
  AllocationRecord root_backing_record(int snode_tree_id) const;

 private:
  LlvmRuntimeExecutor *runtime_exec_;
  struct RootBacking {
    DeviceAllocation allocation;
    AllocationRecord record;
  };
  std::map<int, RootBacking> snode_tree_id_to_device_alloc_;
};

}  // namespace taichi::lang
