#include "field/activity.h"

#include <algorithm>
#include <chrono>
#include <set>
#include <stdexcept>

#include "engine/transfer.h"

namespace field::detail {
namespace {
using namespace taichi::lang;
ArgLoadStmt *array(IRBuilder &b, int id, DataType type, int rank = 2) {
  return b.create_ndarray_arg_load({id}, type, rank, 0);
}
Stmt *scalar(IRBuilder &b, int id) {
  return b.create_arg_load({id}, PrimitiveType::i32, false, 0);
}
std::vector<Stmt *> cell(IRBuilder &b, int row, Stmt *i) {
  return {b.get_int32(row), i};
}
Stmt *read(IRBuilder &b, ArgLoadStmt *a, const std::vector<Stmt *> &at) {
  return b.create_global_load(b.create_external_ptr(a, at));
}
void write(IRBuilder &b, ArgLoadStmt *a, const std::vector<Stmt *> &at,
           Stmt *v) {
  b.create_global_store(b.create_external_ptr(a, at), v);
}
void count(IRBuilder &b, ArgLoadStmt *c, int which) {
  b.create_atomic_add(b.create_external_ptr(c, {b.get_int32(which)}),
                      b.get_int64(1));
}
std::unique_ptr<Ndarray> allocate(engine::Runtime &r, DataType t, int rows,
                                  int n) {
  return std::make_unique<Ndarray>(&r.program(), t,
                                   std::vector<int>{rows, std::max(n, 1)});
}
}  // namespace

Activity::Activity(engine::Runtime &r, int p, int g, int e, int k)
    : runtime_(r),
      p_(p),
      g_(g),
      e_(e),
      k_(k),
      selected_slots_(std::max(g, 1), 0) {
  auto root = std::make_unique<SNode>(0, SNodeType::root);
  current = NativeSet::attach(*root, p);
  next = NativeSet::attach(*root, p);
  centroids = NativeSet::attach(*root, g);
  r.program().add_snode_tree(std::move(root), false);
  particle_flags = allocate(r, PrimitiveType::i32, ParticleStateCount, p);
  particle_delta = allocate(r, PrimitiveType::f32, 3, p);
  group_flags = allocate(r, PrimitiveType::i32, GroupStateCount, g);
  group_delta = allocate(r, PrimitiveType::f32, 6, g);
  particle_offsets = allocate(r, PrimitiveType::i32, 2, p);
  group_offsets = allocate(r, PrimitiveType::i32, 2, g);
  contact_a_offsets = allocate(r, PrimitiveType::i32, 2, p);
  contact_b_offsets = allocate(r, PrimitiveType::i32, 2, p);
  const auto links = [&](int n) {
    return std::make_unique<Ndarray>(&r.program(), PrimitiveType::i32,
                                     std::vector<int>{std::max(n, 1)});
  };
  particle_edges = links(e);
  group_edges = links(e);
  contact_a_edges = links(k);
  contact_b_edges = links(k);
  counters = std::make_unique<Ndarray>(&r.program(), PrimitiveType::i64,
                                       std::vector<int>{CounterCount});
  focus = std::make_unique<Ndarray>(&r.program(), PrimitiveType::f32,
                                    std::vector<int>{11});
  selected_slots_array_ = std::make_unique<Ndarray>(
      &r.program(), PrimitiveType::i32, std::vector<int>{std::max(g, 1)});
  // The fork caches CHI IR by name, not an IR hash. Each kernel below embeds
  // this tree's SNode ids, so its name must distinguish the execution layout.
  // CHI IR is memory-cached only, never restored from the offline disk cache.
  const auto compile_start = std::chrono::steady_clock::now();
  const int threads = r.cpu_threads();
  const auto f = PrimitiveType::f32, i32 = PrimitiveType::i32;
  const auto fa = engine::Param::array(f, 2), ia = engine::Param::array(i32, 2);
  const auto fv = engine::Param::array(f), iv = engine::Param::array(i32);
  const auto cv = engine::Param::array(PrimitiveType::i64);
  const auto integer = engine::Param::scalar(i32);
  {
    IRBuilder b;
    auto *particles = array(b, 0, f), *groups = array(b, 1, f);
    auto *ps = array(b, 2, i32), *pd = array(b, 3, f);
    auto *c = array(b, 5, PrimitiveType::i64, 1), *origin = array(b, 6, f, 1),
         *selected = array(b, 7, i32, 1);
    auto *n = scalar(b, 8), *gcount = scalar(b, 9), *all = scalar(b, 10),
         *invalid = scalar(b, 11), *fcount = scalar(b, 12),
         *radial = scalar(b, 13);
    {
      auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(CounterCount),
                                      false, threads);
      auto guard = b.get_loop_guard(loop);
      write(b, c, {b.get_loop_index(loop)}, b.get_int64(0));
    }
    // Consume only the previous next-phase set. Current and next have separate
    // native masks, so no parallel next-tick touch is consumed in this phase.
    {
      auto *loop = b.create_struct_for(next.cells, false, threads);
      auto guard = b.get_loop_guard(loop);
      auto *p = b.get_loop_index(loop);
      current.activate(b, p);
      next.deactivate(b, p);
    }
    {
      auto *loop =
          b.create_range_for(b.get_int32(0), b.get_int32(1), false, threads);
      auto guard = b.get_loop_guard(loop);
      for (int axis = 3; axis < 8; ++axis)
        write(b, origin, {b.get_int32(axis)}, b.get_float32(0));
    }
    {
      auto *loop =
          b.create_range_for(b.get_int32(0), fcount, false, 1, 0, true);
      auto guard = b.get_loop_guard(loop);
      auto *g = read(b, selected, {b.get_loop_index(loop)});
      auto *mass = read(b, groups, cell(b, kCenM, g));
      for (int axis = 0; axis < 3; ++axis)
        write(b, origin, {b.get_int32(axis + 3)},
              b.create_add(
                  read(b, origin, {b.get_int32(axis + 3)}),
                  b.create_mul(
                      mass,
                      b.create_sub(read(b, groups, cell(b, kCenX + axis, g)),
                                   read(b, origin, {b.get_int32(axis)})))));
      write(b, origin, {b.get_int32(6)},
            b.create_add(read(b, origin, {b.get_int32(6)}), mass));
    }
    {
      auto *loop =
          b.create_range_for(b.get_int32(0), b.get_int32(1), false, threads);
      auto guard = b.get_loop_guard(loop);
      auto *mass = read(b, origin, {b.get_int32(6)});
      auto *denom = b.create_add(
          mass, b.create_sub(b.get_float32(1), b.create_sgn(mass)));
      for (int axis = 0; axis < 3; ++axis) {
        auto *value = b.create_select(
            b.create_cmp_gt(mass, b.get_float32(0)),
            b.create_add(
                read(b, origin, {b.get_int32(axis)}),
                b.create_div(read(b, origin, {b.get_int32(axis + 3)}), denom)),
            read(b, origin, {b.get_int32(8 + axis)}));
        auto changed =
            b.get_if_guard(b.create_if(b.create_cmp_ne(
                               value, read(b, origin, {b.get_int32(axis)}))),
                           true);
        write(b, origin, {b.get_int32(7)}, b.get_float32(1));
      }
      for (int axis = 0; axis < 3; ++axis)
        write(b, origin, {b.get_int32(axis)},
              b.create_select(
                  b.create_cmp_gt(mass, b.get_float32(0)),
                  b.create_add(
                      read(b, origin, {b.get_int32(axis)}),
                      b.create_div(read(b, origin, {b.get_int32(axis + 3)}),
                                   denom)),
                  read(b, origin, {b.get_int32(8 + axis)})));
    }
    // A whole-view input change can invalidate every live calculation. The
    // ordinary stable tick has range end zero, not a scan with flag branches.
    {
      auto *trigger = b.create_logical_or(
          b.create_logical_or(all, invalid),
          b.create_logical_and(
              radial, b.create_cmp_ne(read(b, origin, {b.get_int32(7)}),
                                      b.get_float32(0))));
      auto *loop = b.create_range_for(
          b.get_int32(0), b.create_select(trigger, n, b.get_int32(0)), false,
          threads);
      auto guard = b.get_loop_guard(loop);
      auto *p = b.get_loop_index(loop);
      count(b, c, InvalidatedSlots);
      auto live = b.get_if_guard(
          b.create_if(b.create_cmp_gt(read(b, particles, cell(b, kMass, p)),
                                      b.get_float32(0))),
          true);
      current.activate(b, p);
    }
    {
      auto *loop = b.create_range_for(
          b.get_int32(0), b.create_select(all, gcount, b.get_int32(0)), false,
          threads);
      auto guard = b.get_loop_guard(loop);
      auto *g = b.get_loop_index(loop);
      auto live = b.get_if_guard(
          b.create_if(b.create_cmp_gt(read(b, groups, cell(b, kCenM, g)),
                                      b.get_float32(0))),
          true);
      centroids.activate(b, g);
    }
    // Motion history is maintained only within the native active traversal.
    {
      auto *loop = b.create_struct_for(current.cells, false, threads);
      auto guard = b.get_loop_guard(loop);
      auto *p = b.get_loop_index(loop);
      write(b, ps, cell(b, PriorMoving, p), read(b, ps, cell(b, Moving, p)));
      write(b, ps, cell(b, Moving, p), b.get_int32(0));
      for (int axis = 0; axis < 3; ++axis)
        write(b, pd, cell(b, axis, p), b.get_float32(0));
    }
    begin_ = engine::CompiledKernel(
        r, "activity_begin_" + std::to_string(current.cells->id),
        b.extract_ir(),
        {fa, fa, ia, fa, ia, cv, fv, iv, integer, integer, integer, integer,
         integer, integer},
        engine::MathMode::Strict);
  }
  {
    IRBuilder b;
    auto *particles = array(b, 0, f), *groups = array(b, 1, f),
         *edges = array(b, 2, i32), *shares = array(b, 3, f),
         *gs = array(b, 4, i32), *gd = array(b, 5, f),
         *c = array(b, 6, PrimitiveType::i64, 1), *offsets = array(b, 7, i32),
         *order = array(b, 8, i32, 1);
    auto *gravity = b.create_arg_load({9}, f, false, 0);
    auto *loop = b.create_struct_for(centroids.cells, false, threads);
    auto guard = b.get_loop_guard(loop);
    auto *g = b.get_loop_index(loop);
    auto changed =
        b.get_if_guard(b.create_if(read(b, gs, cell(b, Changed, g))), true);
    auto *members =
        b.create_range_for(read(b, offsets, cell(b, 0, g)),
                           read(b, offsets, cell(b, 1, g)), false, 1, 0, true);
    auto member_guard = b.get_loop_guard(members);
    auto *edge = read(b, order, {b.get_loop_index(members)});
    auto *p = read(b, edges, cell(b, kEdgeParticle, edge));
    count(b, c, AdjacencyVisits);
    auto *mass = b.create_mul(read(b, particles, cell(b, kMass, p)),
                              read(b, shares, cell(b, kShare, edge)));
    auto *response = b.create_if(b.create_logical_and(
        b.create_cmp_ne(gravity, b.get_float32(0)),
        b.create_cmp_ne(read(b, groups, cell(b, kCenM, g)), mass)));
    {
      auto eligible = b.get_if_guard(response, true);
      count(b, c, GeometryChecks);
      Stmt *different = b.get_int32(0);
      for (int axis = 0; axis < 3; ++axis) {
        auto *position = read(b, particles, cell(b, kPosX + axis, p));
        auto *now =
            b.create_sub(read(b, groups, cell(b, kCenX + axis, g)), position);
        auto *before =
            b.create_sub(read(b, gd, cell(b, 3 + axis, g)), position);
        different =
            b.create_logical_or(different, b.create_cmp_ne(now, before));
      }
      auto *test = b.create_if(different);
      {
        auto qualifies = b.get_if_guard(test, true);
        next.activate(b, p);
      }
      {
        auto unchanged = b.get_if_guard(test, false);
        count(b, c, GeometryRejected);
      }
    }
    {
      auto zero = b.get_if_guard(response, false);
      count(b, c, ZeroTouches);
    }
    touch_ = engine::CompiledKernel(
        r, "activity_propagate_" + std::to_string(current.cells->id),
        b.extract_ir(),
        {fa, fa, ia, fa, ia, fa, cv, ia, iv, engine::Param::scalar(f)},
        engine::MathMode::Strict);
  }
  {
    IRBuilder b;
    auto *particles = array(b, 0, f), *groups = array(b, 1, f),
         *edges = array(b, 2, i32), *shares = array(b, 3, f),
         *ps = array(b, 4, i32), *pd = array(b, 5, f), *gs = array(b, 6, i32),
         *gd = array(b, 7, f), *c = array(b, 8, PrimitiveType::i64, 1);
    auto *all = scalar(b, 9);
    auto *offsets = array(b, 11, i32), *order = array(b, 12, i32, 1);
    {
      auto *loop = b.create_struct_for(centroids.cells, false, threads);
      auto guard = b.get_loop_guard(loop);
      auto *g = b.get_loop_index(loop);
      auto *total = read(b, groups, cell(b, kCenM, g));
      count(b, c, GroupGateEntries);
      write(b, gs, cell(b, Flagged, g),
            b.create_select(b.create_cmp_gt(total, b.get_float32(0)), all,
                            b.get_int32(0)));
      {
        auto *members = b.create_range_for(read(b, offsets, cell(b, 0, g)),
                                           read(b, offsets, cell(b, 1, g)),
                                           false, 1, 0, true);
        auto member_guard = b.get_loop_guard(members);
        auto *edge = read(b, order, {b.get_loop_index(members)});
        auto *p = read(b, edges, cell(b, kEdgeParticle, edge));
        count(b, c, GateEdges);
        auto active =
            b.get_if_guard(b.create_if(b.create_logical_and(
                               read(b, ps, cell(b, Moving, p)),
                               b.create_cmp_gt(total, b.get_float32(0)))),
                           true);
        count(b, c, TouchEdges);
        auto *mass = b.create_mul(read(b, particles, cell(b, kMass, p)),
                                  read(b, shares, cell(b, kShare, edge)));
        for (int axis = 0; axis < 3; ++axis) {
          auto *weighted = b.create_mul(mass, read(b, pd, cell(b, axis, p)));
          write(b, gd, cell(b, axis, g),
                b.create_add(read(b, gd, cell(b, axis, g)), weighted));
          auto *centre = read(b, groups, cell(b, kCenX + axis, g));
          auto *effect = b.create_add(centre, b.create_div(weighted, total));
          auto changed = b.get_if_guard(
              b.create_if(b.create_cmp_ne(effect, centre)), true);
          write(b, gs, cell(b, Flagged, g), b.get_int32(1));
        }
      }
      auto live = b.get_if_guard(
          b.create_if(b.create_cmp_gt(total, b.get_float32(0))), true);
      for (int axis = 0; axis < 3; ++axis) {
        auto *centre = read(b, groups, cell(b, kCenX + axis, g));
        auto *effect = b.create_add(
            centre, b.create_div(read(b, gd, cell(b, axis, g)), total));
        auto changed =
            b.get_if_guard(b.create_if(b.create_cmp_ne(effect, centre)), true);
        write(b, gs, cell(b, Flagged, g), b.get_int32(1));
      }
      auto *qualifies = b.create_if(read(b, gs, cell(b, Flagged, g)));
      {
        auto active = b.get_if_guard(qualifies, true);
        count(b, c, Flags);
      }
      {
        auto inactive = b.get_if_guard(qualifies, false);
        centroids.deactivate(b, g);
      }
    }
    mark_ = engine::CompiledKernel(
        r, "activity_mark_centroids_" + std::to_string(current.cells->id),
        b.extract_ir(),
        {fa, fa, ia, fa, ia, fa, ia, fa, cv, integer, integer, ia, iv},
        engine::MathMode::Strict);
  }
  {
    IRBuilder b;
    for (const auto &set : {current, next, centroids}) {
      auto *loop = b.create_struct_for(set.cells, false, threads);
      auto guard = b.get_loop_guard(loop);
      set.deactivate(b, b.get_loop_index(loop));
    }
    for (const auto &entry : std::vector<std::pair<NativeSet, int>>{
             {current, p_}, {centroids, g_}}) {
      auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(entry.second),
                                      false, threads);
      auto guard = b.get_loop_guard(loop);
      entry.first.activate(b, b.get_loop_index(loop));
    }
    reset_ = engine::CompiledKernel(
        r, "activity_reset_" + std::to_string(current.cells->id),
        b.extract_ir(), {}, engine::MathMode::Strict);
  }
  {
    IRBuilder b;
    for (const auto &set : {current, centroids}) {
      auto *loop = b.create_struct_for(set.cells, false, threads);
      auto guard = b.get_loop_guard(loop);
      set.deactivate(b, b.get_loop_index(loop));
    }
    finish_ = engine::CompiledKernel(
        r, "activity_finish_" + std::to_string(current.cells->id),
        b.extract_ir(), {}, engine::MathMode::Strict);
  }
  compilation_ms = std::chrono::duration<double, std::milli>(
                       std::chrono::steady_clock::now() - compile_start)
                       .count();
}

void Activity::reset() {
  const auto zero = [](Ndarray &a, std::size_t n) {
    std::vector<int> values(n,
                            0);  // f32 zero and i32 zero have identical bits.
    engine::upload(a, values.data(), n * sizeof(int));
  };
  zero(*particle_flags, ParticleStateCount * std::size_t(std::max(p_, 1)));
  zero(*particle_delta, 3 * std::size_t(std::max(p_, 1)));
  zero(*group_flags, GroupStateCount * std::size_t(std::max(g_, 1)));
  zero(*group_delta, 6 * std::size_t(std::max(g_, 1)));
  const std::vector<std::int64_t> count_zero(CounterCount, 0);
  engine::upload(*counters, count_zero.data(),
                 count_zero.size() * sizeof(std::int64_t));
  std::vector<float> origin(11, 0);
  std::copy(fixed_origin_.begin(), fixed_origin_.end(), origin.begin() + 8);
  engine::upload(*focus, origin.data(), origin.size() * sizeof(float));
  std::vector<int> ps(ParticleStateCount * std::size_t(std::max(p_, 1)), 0),
      gs(GroupStateCount * std::size_t(std::max(g_, 1)), 0);
  std::fill_n(gs.begin(), g_, 1);
  engine::upload(*particle_flags, ps.data(), ps.size() * sizeof(int));
  engine::upload(*group_flags, gs.data(), gs.size() * sizeof(int));
  engine::upload(*selected_slots_array_, selected_slots_.data(),
                 selected_slots_.size() * sizeof(int));
  auto context = reset_.make_context();
  reset_.launch(context);
}
void Activity::set_focus(const std::vector<int> &groups) {
  std::set<int> unique;
  for (int g : groups)
    if (g < 0 || g >= g_ || !unique.insert(g).second)
      throw std::invalid_argument(
          "focus requires distinct prepared field slots");
  runtime_.synchronize();
  std::fill(selected_slots_.begin(), selected_slots_.end(), 0);
  selected_groups_.assign(unique.begin(), unique.end());
  std::copy(selected_groups_.begin(), selected_groups_.end(),
            selected_slots_.begin());
  engine::upload(*selected_slots_array_, selected_slots_.data(),
                 selected_slots_.size() * sizeof(int));
}
void Activity::set_frame_anchor(const FrameAnchor &anchor) {
  for (int axis = 0; axis < 3; ++axis)
    fixed_origin_[axis] = static_cast<float>(-anchor[axis]);
}
void Activity::index(const std::vector<int> &edges,
                     const std::vector<int> &contacts) {
  const auto build = [](int nodes, int links, const std::vector<int> &source,
                        int row, Ndarray &offset_array, Ndarray &order_array) {
    const auto stride = std::size_t(std::max(nodes, 1));
    std::vector<int> offsets(2 * stride, 0), order(std::max(links, 1), 0),
        counts(nodes, 0);
    for (int e = 0; e < links; ++e)
      ++counts[source[std::size_t(row) * links + e]];
    int cursor = 0;
    for (int n = 0; n < nodes; ++n) {
      offsets[n] = cursor;
      cursor += counts[n];
      offsets[stride + n] = cursor;
    }
    std::vector<int> next(offsets.begin(), offsets.begin() + nodes);
    for (int e = 0; e < links; ++e)
      order[next[source[std::size_t(row) * links + e]]++] = e;
    engine::upload(offset_array, offsets.data(), offsets.size() * sizeof(int));
    engine::upload(order_array, order.data(), order.size() * sizeof(int));
  };
  build(p_, e_, edges, kEdgeParticle, *particle_offsets, *particle_edges);
  build(g_, e_, edges, kEdgeGroup, *group_offsets, *group_edges);
  build(p_, k_, contacts, kContactA, *contact_a_offsets, *contact_a_edges);
  build(p_, k_, contacts, kContactB, *contact_b_offsets, *contact_b_edges);
}
void Activity::begin(Harness &h, bool all, bool invalid, bool radial) {
  auto ctx = begin_.make_context();
  h.bind(ctx, 0, h.particle_array_.get(), h.particle_data,
         {kParticleFieldCount, std::max(p_, 1)});
  h.bind(ctx, 1, h.group_array_.get(), h.group_data,
         {kGroupFieldCount, std::max(g_, 1)});
  ctx.set_arg_ndarray({2}, *particle_flags);
  ctx.set_arg_ndarray({3}, *particle_delta);
  ctx.set_arg_ndarray({4}, *group_flags);
  ctx.set_arg_ndarray({5}, *counters);
  ctx.set_arg_ndarray({6}, *focus);
  ctx.set_arg_ndarray({7}, *selected_slots_array_);
  ctx.set_arg_int({8}, p_);
  ctx.set_arg_int({9}, g_);
  ctx.set_arg_int({10}, all);
  ctx.set_arg_int({11}, invalid);
  ctx.set_arg_int({12}, int(selected_groups_.size()));
  ctx.set_arg_int({13}, radial);
  begin_.launch(ctx);
}
void Activity::touch(Harness &h, float gravity) {
  auto ctx = touch_.make_context();
  h.bind(ctx, 0, h.particle_array_.get(), h.particle_data,
         {kParticleFieldCount, std::max(p_, 1)});
  h.bind(ctx, 1, h.group_array_.get(), h.group_data,
         {kGroupFieldCount, std::max(g_, 1)});
  h.bind(ctx, 2, h.edge_int_array_.get(), h.edge_int_data,
         {kEdgeIntFieldCount, std::max(e_, 1)});
  h.bind(ctx, 3, h.edge_float_array_.get(), h.edge_float_data,
         {kEdgeFloatFieldCount, std::max(e_, 1)});
  ctx.set_arg_ndarray({4}, *group_flags);
  ctx.set_arg_ndarray({5}, *group_delta);
  ctx.set_arg_ndarray({6}, *counters);
  ctx.set_arg_ndarray({7}, *group_offsets);
  ctx.set_arg_ndarray({8}, *group_edges);
  ctx.set_arg_float({9}, gravity);
  touch_.launch(ctx);
}
void Activity::mark(Harness &h, bool all) {
  auto ctx = mark_.make_context();
  h.bind(ctx, 0, h.particle_array_.get(), h.particle_data,
         {kParticleFieldCount, std::max(p_, 1)});
  h.bind(ctx, 1, h.group_array_.get(), h.group_data,
         {kGroupFieldCount, std::max(g_, 1)});
  h.bind(ctx, 2, h.edge_int_array_.get(), h.edge_int_data,
         {kEdgeIntFieldCount, std::max(e_, 1)});
  h.bind(ctx, 3, h.edge_float_array_.get(), h.edge_float_data,
         {kEdgeFloatFieldCount, std::max(e_, 1)});
  ctx.set_arg_ndarray({4}, *particle_flags);
  ctx.set_arg_ndarray({5}, *particle_delta);
  ctx.set_arg_ndarray({6}, *group_flags);
  ctx.set_arg_ndarray({7}, *group_delta);
  ctx.set_arg_ndarray({8}, *counters);
  ctx.set_arg_int({9}, all);
  ctx.set_arg_int({10}, g_);
  ctx.set_arg_ndarray({11}, *group_offsets);
  ctx.set_arg_ndarray({12}, *group_edges);
  mark_.launch(ctx);
}
void Activity::finish() {
  auto ctx = finish_.make_context();
  finish_.launch(ctx);
}
Demand Activity::report(std::uint64_t exposed, std::uint64_t fields_exposed) {
  std::vector<std::int64_t> c(CounterCount);
  std::vector<float> origin(8);
  engine::readback(*counters, c.data(), c.size() * sizeof(std::int64_t));
  engine::readback(*focus, origin.data(), origin.size() * sizeof(float));
  Demand d;
  d.particles_loaded = p_;
  d.particles_exposed = exposed;
  d.fields_loaded = g_;
  d.fields_exposed = fields_exposed;
  d.edges_loaded = e_;
  d.edges_exposed =
      e_;  // Validated edges reference exposed positive-mass particles.
  d.particles_evaluated = c[Evaluated];
  d.particles_skipped = exposed - c[Evaluated];
  d.particles_moved = c[Moved];
  d.particles_woken = c[Woken];
  d.force_requests = c[ForceRequests];
  d.force_evaluations = c[Forces];
  d.force_zero_response = c[ForceRequests] - c[Forces];
  d.force_skipped = e_ - c[ForceRequests];
  d.contact_evaluations = c[Contacts];
  d.gate_checks =
      c[Evaluated] + std::uint64_t(c[GeometryChecks]) + c[ForceRequests];
  d.gate_rejected = c[Rejected];
  d.geometry_rejected = c[GeometryRejected];
  d.touch_adjacency_visits = c[AdjacencyVisits];
  d.force_mask_visits = 0;
  d.native_particle_entries = c[Evaluated];
  d.native_centroid_entries = c[GroupGateEntries];
  d.zero_response_touches = c[ZeroTouches];
  d.invalidation_particle_visits = c[InvalidatedSlots];
  d.centroid_gate_visits = c[GateEdges];
  d.centroid_mask_visits = 0;
  d.centroid_touch_edges = c[TouchEdges];
  d.centroid_reduction_edges = c[ReducedEdges];
  d.centroids_flagged = c[Flags];
  d.centroids_recomputed = c[Recomputed];
  std::copy_n(origin.begin(), 3, d.focal_origin.begin());
  return d;
}
}  // namespace field::detail
