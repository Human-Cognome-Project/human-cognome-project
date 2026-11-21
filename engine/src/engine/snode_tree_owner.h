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
  void synchronize() const {
    if (node) program->synchronize();
  }
  // Declare after dependent members. It drains before their destruction while
  // this owner remains alive to retire the tree afterwards. No allocation or
  // additional native ownership is introduced by this borrowed phase guard.
  struct Drain {
    explicit Drain(const SNodeTreeOwner &tree) : tree(tree) {}
    Drain(const Drain &) = delete;
    Drain &operator=(const Drain &) = delete;
    ~Drain() { tree.synchronize(); }
    const SNodeTreeOwner &tree;
  };
  ~SNodeTreeOwner() {
    if (node) {
      synchronize();
      program->destroy_snode_tree(node);
    }
  }
};
}  // namespace engine
