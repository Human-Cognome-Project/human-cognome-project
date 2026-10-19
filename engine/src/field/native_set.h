#pragma once

#include <algorithm>
#include <cstdint>

#include "engine/runtime.h"

namespace field::detail {
// Native binary traversal control only. Persistent numerical/membership values
// live separately, so consuming a scheduling bit never removes a contributor.
struct NativeSet {
  taichi::lang::SNode *cells{}, *value{};

  static NativeSet attach(taichi::lang::SNode &root, int count) {
    using namespace taichi::lang;
    constexpr int block = 32;
    const int blocks =
        int((std::uint64_t(std::max(count, 1)) + block - 1) / block);
    auto &parent = root.pointer(Axis(0), blocks);
    auto &mask = parent.bitmasked(Axis(0), block);
    auto &leaf = mask.insert_children(SNodeType::place);
    leaf.dt = PrimitiveType::i32;
    return {&mask, &leaf};
  }
  void activate(taichi::lang::IRBuilder &b, taichi::lang::Stmt *index) const {
    // Global atomic access activates the native bit and safely tolerates
    // parallel duplicate touches of the same marker payload.
    b.create_atomic_max(b.create_global_ptr(value, {index}), b.get_int32(1));
  }
  void deactivate(taichi::lang::IRBuilder &b, taichi::lang::Stmt *index) const {
    using namespace taichi::lang;
    auto *ptr = b.create_global_ptr(cells, {index});
    ptr->is_cell_access = true;
    b.insert(
        Stmt::make_typed<SNodeOpStmt>(SNodeOpType::deactivate, cells, ptr));
  }
};
}  // namespace field::detail
