#include "field/activity.h"

#include <algorithm>
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
void flag(IRBuilder &b, ArgLoadStmt *a, int row, Stmt *i) {
  b.create_atomic_max(b.create_external_ptr(a, cell(b, row, i)),
                      b.get_int32(1));
}
std::unique_ptr<Ndarray> allocate(engine::Runtime &r, DataType t, int rows,
                                  int n) {
  return std::make_unique<Ndarray>(&r.program(), t,
                                   std::vector<int>{rows, std::max(n, 1)});
}
}  // namespace

Activity::Activity(engine::Runtime &r, int p, int g, int e, int k)
    : runtime_(r), p_(p), g_(g), e_(e), k_(k), focus_mask_(std::max(g, 1), 0) {
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
                                    std::vector<int>{8});
  focus_mask_array_ = std::make_unique<Ndarray>(
      &r.program(), PrimitiveType::i32, std::vector<int>{std::max(g, 1)});
  reset();
  const int threads = r.cpu_threads();
  const auto f = PrimitiveType::f32, i32 = PrimitiveType::i32;
  const auto fa = engine::Param::array(f, 2), ia = engine::Param::array(i32, 2);
  const auto fv = engine::Param::array(f), iv = engine::Param::array(i32);
  const auto cv = engine::Param::array(PrimitiveType::i64);
  const auto integer = engine::Param::scalar(i32);
  {
    IRBuilder b;
    auto *particles = array(b, 0, f), *groups = array(b, 1, f);
    auto *ps = array(b, 2, i32), *pd = array(b, 3, f), *gs = array(b, 4, i32);
    auto *c = array(b, 5, PrimitiveType::i64, 1), *origin = array(b, 6, f, 1),
         *selected = array(b, 7, i32, 1);
    auto *n = scalar(b, 8), *gcount = scalar(b, 9), *all = scalar(b, 10),
         *invalid = scalar(b, 11);
    {
      auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(CounterCount),
                                      false, threads);
      auto guard = b.get_loop_guard(loop);
      write(b, c, {b.get_loop_index(loop)}, b.get_int64(0));
    }
    {
      auto *loop = b.create_range_for(b.get_int32(0), gcount, false, threads);
      auto guard = b.get_loop_guard(loop);
      auto *g = b.get_loop_index(loop);
      write(b, gs, cell(b, Flagged, g), b.get_int32(0));
      auto live = b.get_if_guard(
          b.create_if(b.create_cmp_gt(read(b, groups, cell(b, kCenM, g)),
                                      b.get_float32(0))),
          true);
      write(b, gs, cell(b, Flagged, g), all);
      count(b, c, ExposedFields);
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
          b.create_range_for(b.get_int32(0), gcount, false, 1, 0, true);
      auto guard = b.get_loop_guard(loop);
      auto *g = b.get_loop_index(loop);
      auto active = b.get_if_guard(b.create_if(read(b, selected, {g})), true);
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
        auto *next = b.create_select(
            b.create_cmp_gt(mass, b.get_float32(0)),
            b.create_add(
                read(b, origin, {b.get_int32(axis)}),
                b.create_div(read(b, origin, {b.get_int32(axis + 3)}), denom)),
            b.get_float32(0));
        auto changed =
            b.get_if_guard(b.create_if(b.create_cmp_ne(
                               next, read(b, origin, {b.get_int32(axis)}))),
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
                  b.get_float32(0)));
    }
    {
      auto *loop = b.create_range_for(b.get_int32(0), n, false, threads);
      auto guard = b.get_loop_guard(loop);
      auto *p = b.get_loop_index(loop);
      write(b, ps, cell(b, PriorMoving, p), read(b, ps, cell(b, Moving, p)));
      write(b, ps, cell(b, Moving, p), b.get_int32(0));
      write(b, ps, cell(b, Candidate, p), b.get_int32(0));
      for (int axis = 0; axis < 3; ++axis)
        write(b, pd, cell(b, axis, p), b.get_float32(0));
      auto *trigger = b.create_logical_or(
          b.create_logical_or(all, invalid),
          b.create_logical_or(read(b, ps, cell(b, Continued, p)),
                              b.create_cmp_ne(read(b, origin, {b.get_int32(7)}),
                                              b.get_float32(0))));
      auto *live = b.create_cmp_gt(read(b, particles, cell(b, kMass, p)),
                                   b.get_float32(0));
      auto active = b.get_if_guard(
          b.create_if(b.create_logical_and(trigger, live)), true);
      write(b, ps, cell(b, Candidate, p), b.get_int32(1));
    }
    begin_ = engine::CompiledKernel(
        r, "activity_begin", b.extract_ir(),
        {fa, fa, ia, fa, ia, cv, fv, iv, integer, integer, integer, integer},
        engine::MathMode::Strict);
  }
  {
    IRBuilder b;
    auto *particles = array(b, 0, f), *groups = array(b, 1, f),
         *edges = array(b, 2, i32);
    auto *contacts = array(b, 3, i32), *ps = array(b, 4, i32),
         *gs = array(b, 5, i32);
    auto *gd = array(b, 6, f), *c = array(b, 7, PrimitiveType::i64, 1);
    auto *ecount = scalar(b, 8), *kcount = scalar(b, 9);
    {
      auto *loop = b.create_range_for(b.get_int32(0), ecount, false, threads);
      auto guard = b.get_loop_guard(loop);
      Stmt *edge = b.get_loop_index(loop),
           *p = read(b, edges, cell(b, kEdgeParticle, edge)),
           *g = read(b, edges, cell(b, kEdgeGroup, edge));
      auto active =
          b.get_if_guard(b.create_if(read(b, gs, cell(b, Changed, g))), true);
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
        auto changed = b.get_if_guard(test, true);
        flag(b, ps, Candidate, p);
      }
      {
        auto unchanged = b.get_if_guard(test, false);
        count(b, c, GeometryRejected);
      }
    }
    {
      auto *loop = b.create_range_for(b.get_int32(0), kcount, false, threads);
      auto guard = b.get_loop_guard(loop);
      Stmt *pair = b.get_loop_index(loop),
           *a = read(b, contacts, cell(b, kContactA, pair)),
           *other = read(b, contacts, cell(b, kContactB, pair));
      auto *moved =
          b.create_logical_or(read(b, ps, cell(b, PriorMoving, a)),
                              read(b, ps, cell(b, PriorMoving, other)));
      auto active = b.get_if_guard(b.create_if(moved), true);
      flag(b, ps, Candidate, a);
      flag(b, ps, Candidate, other);
    }
    touch_ = engine::CompiledKernel(
        r, "activity_touch", b.extract_ir(),
        {fa, fa, ia, ia, ia, ia, fa, cv, integer, integer},
        engine::MathMode::Strict);
  }
  {
    IRBuilder b;
    auto *particles = array(b, 0, f), *groups = array(b, 1, f),
         *edges = array(b, 2, i32), *shares = array(b, 3, f),
         *ps = array(b, 4, i32), *pd = array(b, 5, f), *gs = array(b, 6, i32),
         *gd = array(b, 7, f), *c = array(b, 8, PrimitiveType::i64, 1);
    auto *ecount = scalar(b, 9), *gcount = scalar(b, 10);
    auto *offsets = array(b, 11, i32), *order = array(b, 12, i32, 1);
    (void)ecount;
    {
      auto *loop = b.create_range_for(b.get_int32(0), gcount, false, threads);
      auto guard = b.get_loop_guard(loop);
      auto *g = b.get_loop_index(loop);
      auto *total = read(b, groups, cell(b, kCenM, g));
      {
        auto *members = b.create_range_for(read(b, offsets, cell(b, 0, g)),
                                           read(b, offsets, cell(b, 1, g)),
                                           false, 1, 0, true);
        auto member_guard = b.get_loop_guard(members);
        auto *edge = read(b, order, {b.get_loop_index(members)});
        auto *p = read(b, edges, cell(b, kEdgeParticle, edge));
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
      auto active =
          b.get_if_guard(b.create_if(read(b, gs, cell(b, Flagged, g))), true);
      count(b, c, Flags);
    }
    mark_ = engine::CompiledKernel(
        r, "activity_mark_centroids", b.extract_ir(),
        {fa, fa, ia, fa, ia, fa, ia, fa, cv, integer, integer, ia, iv},
        engine::MathMode::Strict);
  }
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
  zero(*focus, 8);
  std::vector<int> ps(ParticleStateCount * std::size_t(std::max(p_, 1)), 0),
      gs(GroupStateCount * std::size_t(std::max(g_, 1)), 0);
  std::fill_n(ps.begin(), p_, 1);
  std::fill_n(gs.begin(), g_, 1);
  engine::upload(*particle_flags, ps.data(), ps.size() * sizeof(int));
  engine::upload(*group_flags, gs.data(), gs.size() * sizeof(int));
  engine::upload(*focus_mask_array_, focus_mask_.data(),
                 focus_mask_.size() * sizeof(int));
}
void Activity::set_focus(const std::vector<int> &groups) {
  std::set<int> unique;
  for (int g : groups)
    if (g < 0 || g >= g_ || !unique.insert(g).second)
      throw std::invalid_argument(
          "focus requires distinct prepared field slots");
  runtime_.synchronize();
  std::fill(focus_mask_.begin(), focus_mask_.end(), 0);
  for (int g : groups) focus_mask_[g] = 1;
  engine::upload(*focus_mask_array_, focus_mask_.data(),
                 focus_mask_.size() * sizeof(int));
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
void Activity::begin(Harness &h, bool all, bool invalid) {
  auto ctx = begin_.make_context();
  h.bind(ctx, 0, h.particle_array_.get(), h.particle_data, {kParticleFieldCount, std::max(p_, 1)});
  h.bind(ctx, 1, h.group_array_.get(), h.group_data, {kGroupFieldCount, std::max(g_, 1)});
  ctx.set_arg_ndarray({2}, *particle_flags);
  ctx.set_arg_ndarray({3}, *particle_delta);
  ctx.set_arg_ndarray({4}, *group_flags);
  ctx.set_arg_ndarray({5}, *counters);
  ctx.set_arg_ndarray({6}, *focus);
  ctx.set_arg_ndarray({7}, *focus_mask_array_);
  ctx.set_arg_int({8}, p_);
  ctx.set_arg_int({9}, g_);
  ctx.set_arg_int({10}, all);
  ctx.set_arg_int({11}, invalid);
  begin_.launch(ctx);
}
void Activity::touch(Harness &h) {
  auto ctx = touch_.make_context();
  h.bind(ctx, 0, h.particle_array_.get(), h.particle_data, {kParticleFieldCount, std::max(p_, 1)});
  h.bind(ctx, 1, h.group_array_.get(), h.group_data, {kGroupFieldCount, std::max(g_, 1)});
  h.bind(ctx, 2, h.edge_int_array_.get(), h.edge_int_data, {kEdgeIntFieldCount, std::max(e_, 1)});
  h.bind(ctx, 3, h.contact_int_array_.get(), h.contact_int_data, {kContactIntFieldCount, std::max(k_, 1)});
  ctx.set_arg_ndarray({4}, *particle_flags);
  ctx.set_arg_ndarray({5}, *group_flags);
  ctx.set_arg_ndarray({6}, *group_delta);
  ctx.set_arg_ndarray({7}, *counters);
  ctx.set_arg_int({8}, e_);
  ctx.set_arg_int({9}, k_);
  touch_.launch(ctx);
}
void Activity::mark(Harness &h) {
  auto ctx = mark_.make_context();
  h.bind(ctx, 0, h.particle_array_.get(), h.particle_data, {kParticleFieldCount, std::max(p_, 1)});
  h.bind(ctx, 1, h.group_array_.get(), h.group_data, {kGroupFieldCount, std::max(g_, 1)});
  h.bind(ctx, 2, h.edge_int_array_.get(), h.edge_int_data, {kEdgeIntFieldCount, std::max(e_, 1)});
  h.bind(ctx, 3, h.edge_float_array_.get(), h.edge_float_data, {kEdgeFloatFieldCount, std::max(e_, 1)});
  ctx.set_arg_ndarray({4}, *particle_flags);
  ctx.set_arg_ndarray({5}, *particle_delta);
  ctx.set_arg_ndarray({6}, *group_flags);
  ctx.set_arg_ndarray({7}, *group_delta);
  ctx.set_arg_ndarray({8}, *counters);
  ctx.set_arg_int({9}, e_);
  ctx.set_arg_int({10}, g_);
  ctx.set_arg_ndarray({11}, *group_offsets);
  ctx.set_arg_ndarray({12}, *group_edges);
  mark_.launch(ctx);
}
Demand Activity::report(std::uint64_t exposed) {
  std::vector<std::int64_t> c(CounterCount);
  std::vector<float> origin(8);
  engine::readback(*counters, c.data(), c.size() * sizeof(std::int64_t));
  engine::readback(*focus, origin.data(), origin.size() * sizeof(float));
  Demand d;
  d.particles_loaded = p_;
  d.particles_exposed = exposed;
  d.fields_loaded = g_;
  d.fields_exposed = c[ExposedFields];
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
  d.touch_adjacency_visits = std::uint64_t(e_) + 2 * std::uint64_t(k_);
  d.force_mask_visits = p_;
  d.centroid_gate_visits = e_;
  d.centroid_mask_visits = 3 * std::uint64_t(g_);
  d.centroid_touch_edges = c[TouchEdges];
  d.centroid_reduction_edges = c[ReducedEdges];
  d.centroids_flagged = c[Flags];
  d.centroids_recomputed = c[Recomputed];
  std::copy_n(origin.begin(), 3, d.focal_origin.begin());
  return d;
}
}  // namespace field::detail
