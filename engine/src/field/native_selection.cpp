#include "field/native_selection.h"

#include <stdexcept>

namespace field::detail {
namespace {
using namespace taichi::lang;
void clear(IRBuilder &b,
           const std::array<NativeSet, 2> &sets,
           int threads,
           Stmt *enabled) {
  for (const auto &set : sets) {
    auto *loop = b.create_struct_for(set.cells, false, threads);
    auto guard = b.get_loop_guard(loop);
    auto allowed = b.get_if_guard(b.create_if(enabled), true);
    set.deactivate(b, b.get_loop_index(loop));
  }
}
void copy(IRBuilder &b,
          const std::array<NativeSet, 2> &from,
          const std::array<NativeSet, 2> &to,
          int threads,
          Stmt *enabled) {
  for (int kind = 0; kind < 2; ++kind) {
    auto *loop = b.create_struct_for(from[kind].cells, false, threads);
    auto guard = b.get_loop_guard(loop);
    auto allowed = b.get_if_guard(b.create_if(enabled), true);
    to[kind].activate(b, b.get_loop_index(loop));
  }
}
}  // namespace

NativeSelectionWorkspace::NativeSelectionWorkspace(engine::Runtime &r,
                                                   NativeSet particles,
                                                   int p,
                                                   NativeSet centroids,
                                                   int g)
    : committed_{particles, centroids} {
  if (p < 0 || g < 0 || !particles.cells || !centroids.cells ||
      particles.cells == centroids.cells)
    throw std::invalid_argument("native selection requires distinct type sets");
  const auto compatible = [](NativeSet set, int count) {
    const auto blocks =
        (std::uint64_t(std::max(count, 1)) + NativeSet::block - 1) /
        NativeSet::block;
    return set.cells->type == SNodeType::bitmasked && set.cells->parent &&
           set.cells->parent->type == SNodeType::pointer &&
           set.cells->num_cells_per_container == NativeSet::block &&
           set.cells->parent->num_cells_per_container ==
               std::int64_t(taichi::bit::least_pot_bound(blocks));
  };
  if (!compatible(particles, p) || !compatible(centroids, g))
    throw std::invalid_argument("native selection capacity/layout mismatch");
  // B1 qualification is CPU-only. No CUDA transaction claim follows from the
  // existing fixed-hierarchy qualification path.
  if (r.arch() != taichi::Arch::x64)
    throw std::invalid_argument(
        "native selection workspace currently requires x64");
  auto root = std::make_unique<SNode>(0, SNodeType::root);
  candidate_[0] = NativeSet::attach(*root, p);
  candidate_[1] = NativeSet::attach(*root, g);
  r.program().add_snode_tree(std::move(root), false);
  const auto suffix = std::to_string(candidate_[0].cells->id);
  const int threads = r.cpu_threads();
  const auto build = [&](const std::string &name, bool publication,
                         bool discard) {
    IRBuilder b;
    auto *enabled = b.create_arg_load({0}, PrimitiveType::i32, false, 0);
    const auto &to = publication ? committed_ : candidate_;
    const auto &from = publication ? candidate_ : committed_;
    clear(b, to, threads, enabled);
    if (!discard)
      copy(b, from, to, threads, enabled);
    return engine::CompiledKernel(r, name + suffix, b.extract_ir(),
                                  {engine::Param::scalar(PrimitiveType::i32)},
                                  engine::MathMode::Strict);
  };
  stage_ = build("selection_stage_", false, false);
  abandon_ = build("selection_abandon_", false, true);
  publish_ = build("selection_publish_", true, false);
  // Provision every scratch pointer block and native traversal high-water.
  // Committed type sets are already provisioned by the execution owner.
  {
    IRBuilder b;
    for (int kind = 0; kind < 2; ++kind) {
      auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(kind ? g : p),
                                      false, threads);
      auto guard = b.get_loop_guard(loop);
      candidate_[kind].activate(b, b.get_loop_index(loop));
    }
    engine::CompiledKernel provision(r, "selection_provision_" + suffix,
                                     b.extract_ir(), {});
    auto context = provision.make_context();
    provision.launch(context);
  }
  abandon();
  stage();
  // Prepare the real publication binding/list traversals with all committed
  // writes disabled. Preparation never clears/reinstalls committed membership.
  launch(publish_, 0);
  r.synchronize();
}

void NativeSelectionWorkspace::launch(engine::CompiledKernel &kernel,
                                      int enabled) {
  auto call = kernel.invocation(true);
  auto &context = call.get();
  engine::argument_int(context, 0, enabled);
  kernel.launch(context);
}
void NativeSelectionWorkspace::stage() {
  launch(stage_);
}
void NativeSelectionWorkspace::abandon() {
  launch(abandon_);
}
void NativeSelectionWorkspace::publish() {
  launch(publish_);
}
std::size_t NativeSelectionWorkspace::binding_bytes() const {
  return stage_.prepared_bytes() + abandon_.prepared_bytes() +
         publish_.prepared_bytes();
}
}  // namespace field::detail
