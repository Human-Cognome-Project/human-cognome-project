#pragma once

#include <algorithm>
#include <cstdint>

#include "engine/runtime.h"
#include "taichi/util/bit.h"

namespace field::detail {
// Native binary traversal control only. Persistent numerical/membership values
// live separately, so consuming a scheduling bit never removes a contributor.
struct NativeSet {
  static constexpr int block = 32;
  taichi::lang::SNode *cells{};

  static NativeSet attach(taichi::lang::SNode &root, int count) {
    using namespace taichi::lang;
    const int blocks =
        int((std::uint64_t(std::max(count, 1)) + block - 1) / block);
    // Keep native sparse traversal containers at power-of-two capacity. The
    // logical count stays separate; padding never activates extra elements.
    auto &parent =
        root.pointer(Axis(0), int(taichi::bit::least_pot_bound(blocks)));
    auto &mask = parent.bitmasked(Axis(0), block);
    auto &leaf = mask.insert_children(SNodeType::place);
    leaf.dt = PrimitiveType::i8;  // Layout leaf only; no scheduling payload.
    return {&mask};
  }
  void activate(taichi::lang::IRBuilder &b, taichi::lang::Stmt *index) const {
    using namespace taichi::lang;
    // Explicit native cell activation does not allocate ancestors in this
    // fork. Activate our pointer block first; both operations are idempotent
    // and use the runtime's own allocation/atomic bit operations.
    operation(b, cells->parent, b.create_div(index, b.get_int32(block)),
              SNodeOpType::activate);
    operation(b, cells, index, SNodeOpType::activate);
  }
  void deactivate(taichi::lang::IRBuilder &b, taichi::lang::Stmt *index) const {
    operation(b, cells, index, taichi::lang::SNodeOpType::deactivate);
  }

 private:
  static void operation(taichi::lang::IRBuilder &b, taichi::lang::SNode *node,
                        taichi::lang::Stmt *index,
                        taichi::lang::SNodeOpType op) {
    using namespace taichi::lang;
    auto *ptr = b.create_global_ptr(node, {index});
    ptr->is_cell_access = true;
    b.insert(Stmt::make_typed<SNodeOpStmt>(op, node, ptr));
  }
};
}  // namespace field::detail
