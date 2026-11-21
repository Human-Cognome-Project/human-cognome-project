#pragma once

#include "engine/runtime.h"

namespace engine {
// Runtime/Program outlives this guard. Declare before dependent members so
// native retirement follows their destruction, including constructor unwind.
struct SNodeTreeOwner {
  taichi::lang::Program *program{};
  taichi::lang::SNodeTree *node{};
  SNodeTreeOwner() = default;
  SNodeTreeOwner(const SNodeTreeOwner &) = delete;
  SNodeTreeOwner &operator=(const SNodeTreeOwner &) = delete;
  ~SNodeTreeOwner() {
    if (node) {
      program->synchronize();
      program->destroy_snode_tree(node);
    }
  }
};
}  // namespace engine
