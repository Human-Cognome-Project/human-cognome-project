#include "field/activity.h"

#include <algorithm>
#include <chrono>
#include <set>
#include <stdexcept>

#include "engine/transfer.h"
#include "field/prepared_storage.h"

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
void write(IRBuilder &b,
           ArgLoadStmt *a,
           const std::vector<Stmt *> &at,
           Stmt *v) {
  b.create_global_store(b.create_external_ptr(a, at), v);
}
void count(IRBuilder &b, ArgLoadStmt *c, int which) {
  b.create_atomic_add(b.create_external_ptr(c, {b.get_int32(which)}),
                      b.get_int64(1));
}
std::unique_ptr<Ndarray> allocate(engine::Runtime &r,
                                  DataType t,
                                  int rows,
                                  int n) {
  return std::make_unique<Ndarray>(&r.program(), t,
                                   std::vector<int>{rows, std::max(n, 1)});
}
}  // namespace

bool Activity::wide() const {
  return prepared_ && prepared_->capacity.profile().wide();
}

Activity::Activity(engine::Runtime &r,
                   int p,
                   int g,
                   int e,
                   int k,
                   PreparedStorage *prepared)
    : runtime_(r),
      p_(p),
      g_(g),
      e_(e),
      k_(k),
      prepared_(prepared),
      zero_workspace_(std::max({3 * std::size_t(std::max(p, 1)),
                                6 * std::size_t(std::max(g, 1))}),
                      0),
      selected_slots_(
          std::max(
              prepared ? int(prepared->capacity.limits()[Budget::Focus]) : g,
              1),
          0) {
  if (wide()) zero_workspace_.resize(2 * zero_workspace_.size());
  selected_groups_.reserve(selected_slots_.size());
  auto root = std::make_unique<SNode>(0, SNodeType::root);
  particle_activity = NativeSet::attach(*root, p);
  centroids = NativeSet::attach(*root, g);
  r.program().add_snode_tree(std::move(root), false);
  particle_flags = allocate(r, PrimitiveType::i32, ParticleStateCount, p);
  particle_delta = allocate(r, real_type(wide()), 3, p);
  group_flags = allocate(r, PrimitiveType::i32, GroupStateCount, g);
  group_delta = allocate(r, real_type(wide()), 6, g);
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
  counters = std::make_unique<Ndarray>(
      &r.program(), PrimitiveType::i64,
      std::vector<int>{wide() ? TrialCounterCount : CounterCount});
  focus = std::make_unique<Ndarray>(&r.program(), real_type(wide()),
                                    std::vector<int>{11});
  selected_slots_array_ =
      std::make_unique<Ndarray>(&r.program(), PrimitiveType::i32,
                                std::vector<int>{int(selected_slots_.size())});
  // The fork caches CHI IR by name, not an IR hash. Each kernel below embeds
  // this tree's SNode ids, so its name must distinguish the execution layout.
  // CHI IR is memory-cached only, never restored from the offline disk cache.
  const auto compile_start = std::chrono::steady_clock::now();
  const int threads = r.cpu_threads();
  const auto f = real_type(wide()), i32 = PrimitiveType::i32;
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
      auto *loop = b.create_range_for(
          b.get_int32(0),
          b.get_int32(wide() ? TrialCounterCount : CounterCount), false,
          threads);
      auto guard = b.get_loop_guard(loop);
      write(b, c, {b.get_loop_index(loop)}, b.get_int64(0));
    }
    {
      auto *loop =
          b.create_range_for(b.get_int32(0), b.get_int32(1), false, threads);
      auto guard = b.get_loop_guard(loop);
      for (int axis = 3; axis < 8; ++axis)
        write(b, origin, {b.get_int32(axis)}, real(b, 0));
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
      auto *denom =
          b.create_add(mass, b.create_sub(real(b, 1), b.create_sgn(mass)));
      for (int axis = 0; axis < 3; ++axis) {
        auto *value = b.create_select(
            b.create_cmp_gt(mass, real(b, 0)),
            b.create_add(
                read(b, origin, {b.get_int32(axis)}),
                b.create_div(read(b, origin, {b.get_int32(axis + 3)}), denom)),
            read(b, origin, {b.get_int32(8 + axis)}));
        auto changed =
            b.get_if_guard(b.create_if(b.create_cmp_ne(
                               value, read(b, origin, {b.get_int32(axis)}))),
                           true);
        write(b, origin, {b.get_int32(7)}, real(b, 1));
      }
      for (int axis = 0; axis < 3; ++axis)
        write(b, origin, {b.get_int32(axis)},
              b.create_select(
                  b.create_cmp_gt(mass, real(b, 0)),
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
              radial,
              b.create_cmp_ne(read(b, origin, {b.get_int32(7)}), real(b, 0))));
      auto *loop = b.create_range_for(
          b.get_int32(0), b.create_select(trigger, n, b.get_int32(0)), false,
          threads);
      auto guard = b.get_loop_guard(loop);
      auto *p = b.get_loop_index(loop);
      std::optional<IRBuilder::IfGuard> presence_guard;
      if (prepared_)
        presence_guard.emplace(b.get_if_guard(
            b.create_if(
                NativeSet{prepared_->particles.bindings().presence}.active(b,
                                                                           p)),
            true));
      count(b, c, InvalidatedSlots);
      auto live = b.get_if_guard(
          b.create_if(b.create_cmp_gt(read(b, particles, cell(b, kMass, p)),
                                      real(b, 0))),
          true);
      particle_activity.activate(b, p);
    }
    {
      auto *loop = b.create_range_for(
          b.get_int32(0), b.create_select(all, gcount, b.get_int32(0)), false,
          threads);
      auto guard = b.get_loop_guard(loop);
      auto *g = b.get_loop_index(loop);
      std::optional<IRBuilder::IfGuard> presence_guard;
      if (prepared_)
        presence_guard.emplace(b.get_if_guard(
            b.create_if(
                NativeSet{prepared_->fields.bindings().presence}.active(b, g)),
            true));
      auto live =
          b.get_if_guard(b.create_if(b.create_cmp_gt(
                             read(b, groups, cell(b, kCenM, g)), real(b, 0))),
                         true);
      centroids.activate(b, g);
    }
    // Motion history is maintained only within the native active traversal.
    {
      auto *loop = b.create_struct_for(particle_activity.cells, false, threads);
      auto guard = b.get_loop_guard(loop);
      auto *p = b.get_loop_index(loop);
      write(b, ps, cell(b, PriorMoving, p), read(b, ps, cell(b, Moving, p)));
      write(b, ps, cell(b, Moving, p), b.get_int32(0));
      for (int axis = 0; axis < 3; ++axis)
        write(b, pd, cell(b, axis, p), real(b, 0));
    }
    begin_ = engine::CompiledKernel(
        r, "activity_begin_" + std::to_string(particle_activity.cells->id),
        b.extract_ir(),
        {fa, fa, ia, fa, ia, cv, fv, iv, integer, integer, integer, integer,
         integer, integer},
        engine::MathMode::Strict);
  }
  {
    IRBuilder b;
    auto *ps = array(b, 0, i32), *c = array(b, 1, PrimitiveType::i64, 1),
         *a_offsets = array(b, 2, i32), *a_order = array(b, 3, i32, 1),
         *b_offsets = array(b, 4, i32), *b_order = array(b, 5, i32, 1),
         *pairs = array(b, 6, i32);
    // All numerical particle work and bit consumption has finished. Movers
    // already retained their own bits; newly touched sleepers have Moving=0.
    // Therefore dynamic peer activation can neither enter a calculation early
    // nor extend propagation through a newly touched stationary particle.
    auto *loop = b.create_struct_for(particle_activity.cells, false, threads);
    auto guard = b.get_loop_guard(loop);
    auto *p = b.get_loop_index(loop);
    auto moving =
        b.get_if_guard(b.create_if(read(b, ps, cell(b, Moving, p))), true);
    for (int side = 0; side < 2; ++side) {
      auto *offsets = side == 0 ? a_offsets : b_offsets;
      auto *order = side == 0 ? a_order : b_order;
      auto *members = b.create_range_for(read(b, offsets, cell(b, 0, p)),
                                         read(b, offsets, cell(b, 1, p)), false,
                                         1, 0, true);
      auto member_guard = b.get_loop_guard(members);
      auto *contact = read(b, order, {b.get_loop_index(members)});
      auto *other =
          read(b, pairs, cell(b, side == 0 ? kContactB : kContactA, contact));
      particle_activity.activate(b, other);
      count(b, c, AdjacencyVisits);
    }
    contact_touch_ = engine::CompiledKernel(
        r,
        "activity_contact_touch_" + std::to_string(particle_activity.cells->id),
        b.extract_ir(), {ia, cv, ia, iv, ia, iv, ia}, engine::MathMode::Strict);
  }
  {
    IRBuilder b;
    auto *ratios = wide() ? array(b, 10, PrimitiveType::u64, 1) : nullptr;
    auto *particles = array(b, 0, f), *groups = array(b, 1, f),
         *edges = array(b, 2, i32), *shares = array(b, 3, PrimitiveType::f32),
         *gs = array(b, 4, i32), *gd = array(b, 5, f),
         *c = array(b, 6, PrimitiveType::i64, 1), *offsets = array(b, 7, i32),
         *order = array(b, 8, i32, 1);
    auto *gravity = b.create_arg_load({9}, f, false, 0);
    auto *loop = b.create_struct_for(centroids.cells, false, threads);
    auto guard = b.get_loop_guard(loop);
    auto *g = b.get_loop_index(loop);
    // No centroid writes occur in this type's phase. Consume before touching
    // particles; their phase has already finished its numerical calculation.
    centroids.deactivate(b, g);
    auto changed =
        b.get_if_guard(b.create_if(read(b, gs, cell(b, Changed, g))), true);
    auto *members =
        b.create_range_for(read(b, offsets, cell(b, 0, g)),
                           read(b, offsets, cell(b, 1, g)), false, 1, 0, true);
    auto member_guard = b.get_loop_guard(members);
    auto *edge = read(b, order, {b.get_loop_index(members)});
    auto *p = read(b, edges, cell(b, kEdgeParticle, edge));
    count(b, c, AdjacencyVisits);
    auto *mass =
        b.create_mul(read(b, particles, cell(b, kMass, p)),
                     wide() ? source_ratio(b, ratios, edge, std::max(e_, 1))
                            : read(b, shares, cell(b, kShare, edge)));
    auto *response = b.create_if(b.create_logical_and(
        b.create_cmp_ne(gravity, real(b, 0)),
        wide() ? b.create_cmp_ne(
                     read(b, ratios,
                          {b.create_add(g, b.get_int32(2 * std::max(e_, 1)))}),
                     b.get_uint64(0))
               : b.create_cmp_ne(read(b, groups, cell(b, kCenM, g)), mass)));
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
        particle_activity.activate(b, p);
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
        r, "activity_propagate_" + std::to_string(particle_activity.cells->id),
        b.extract_ir(),
        ratio_parameters(
            {fa, fa, ia, engine::Param::array(PrimitiveType::f32, 2), ia, fa,
             cv, ia, iv, engine::Param::scalar(f)},
            wide()),
        engine::MathMode::Strict);
  }
  {
    IRBuilder b;
    auto *particles = array(b, 0, f), *groups = array(b, 1, f),
         *edges = array(b, 2, i32), *shares = array(b, 3, PrimitiveType::f32),
         *ps = array(b, 4, i32), *pd = array(b, 5, f), *gs = array(b, 6, i32),
         *gd = array(b, 7, f), *c = array(b, 8, PrimitiveType::i64, 1);
    auto *all = scalar(b, 9);
    auto *offsets = array(b, 11, i32), *order = array(b, 12, i32, 1);
    if (!wide()) {
      {
        auto *loop = b.create_struct_for(centroids.cells, false, threads);
        auto guard = b.get_loop_guard(loop);
        auto *g = b.get_loop_index(loop);
        auto *total = read(b, groups, cell(b, kCenM, g));
        count(b, c, GroupGateEntries);
        write(b, gs, cell(b, Flagged, g),
              b.create_select(b.create_cmp_gt(total, real(b, 0)), all,
                              b.get_int32(0)));
        {
          auto *members = b.create_range_for(read(b, offsets, cell(b, 0, g)),
                                             read(b, offsets, cell(b, 1, g)),
                                             false, 1, 0, true);
          auto member_guard = b.get_loop_guard(members);
          auto *edge = read(b, order, {b.get_loop_index(members)});
          auto *p = read(b, edges, cell(b, kEdgeParticle, edge));
          count(b, c, GateEdges);
          auto active = b.get_if_guard(b.create_if(b.create_logical_and(
                                           read(b, ps, cell(b, Moving, p)),
                                           b.create_cmp_gt(total, real(b, 0)))),
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
            b.create_if(b.create_cmp_gt(total, real(b, 0))), true);
        for (int axis = 0; axis < 3; ++axis) {
          auto *centre = read(b, groups, cell(b, kCenX + axis, g));
          auto *effect = b.create_add(
              centre, b.create_div(read(b, gd, cell(b, axis, g)), total));
          auto changed = b.get_if_guard(
              b.create_if(b.create_cmp_ne(effect, centre)), true);
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
    } else {
      auto *loop = b.create_struct_for(centroids.cells, false, threads);
      auto guard = b.get_loop_guard(loop);
      auto *g = b.get_loop_index(loop);
      count(b, c, GroupGateEntries);
      write(b, gs, cell(b, Flagged, g), b.get_int32(1));
      count(b, c, Flags);
    }
    mark_ = engine::CompiledKernel(
        r,
        "activity_mark_centroids_" +
            std::to_string(particle_activity.cells->id),
        b.extract_ir(),
        {fa, fa, ia, engine::Param::array(PrimitiveType::f32, 2), ia, fa, ia,
         fa, cv, integer, integer, ia, iv},
        engine::MathMode::Strict);
  }
  {
    IRBuilder b;
    auto *flags = array(b, 0, i32);
    for (const auto &set : {particle_activity, centroids}) {
      auto *loop = b.create_struct_for(set.cells, false, threads);
      auto guard = b.get_loop_guard(loop);
      set.deactivate(b, b.get_loop_index(loop));
    }
    for (const auto &entry : std::vector<std::pair<NativeSet, int>>{
             {particle_activity, p_}, {centroids, g_}}) {
      auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(entry.second),
                                      false, threads);
      auto guard = b.get_loop_guard(loop);
      auto *row = b.get_loop_index(loop);
      const bool group = entry.first.cells == centroids.cells;
      std::optional<IRBuilder::IfGuard> presence_guard;
      if (prepared_)
        presence_guard.emplace(b.get_if_guard(
            b.create_if(
                NativeSet{group ? prepared_->fields.bindings().presence
                                : prepared_->particles.bindings().presence}
                    .active(b, row)),
            true));
      entry.first.activate(b, row);
      if (group)
        write(b, flags, cell(b, Flagged, row), b.get_int32(1));
    }
    reset_ = engine::CompiledKernel(
        r, "activity_reset_" + std::to_string(particle_activity.cells->id),
        b.extract_ir(), {ia}, engine::MathMode::Strict);
  }
  {
    IRBuilder b;
    for (const auto &set : {particle_activity, centroids}) {
      auto *loop = b.create_struct_for(set.cells, false, threads);
      auto guard = b.get_loop_guard(loop);
      set.deactivate(b, b.get_loop_index(loop));
    }
    clear_ = engine::CompiledKernel(
        r, "activity_clear_" + std::to_string(particle_activity.cells->id),
        b.extract_ir(), {}, engine::MathMode::Strict);
  }
  compilation_ms = std::chrono::duration<double, std::milli>(
                       std::chrono::steady_clock::now() - compile_start)
                       .count();
}

void Activity::reset() {
  const auto zero = [&](Ndarray &a, std::size_t n) {
    engine::upload(a, zero_workspace_.data(), n * sizeof(int));
  };
  zero(*particle_flags, ParticleStateCount * std::size_t(std::max(p_, 1)));
  zero(*particle_delta, 3 * std::size_t(std::max(p_, 1)) * (wide() ? 2 : 1));
  zero(*group_flags, GroupStateCount * std::size_t(std::max(g_, 1)));
  zero(*group_delta, 6 * std::size_t(std::max(g_, 1)) * (wide() ? 2 : 1));
  const std::array<std::int64_t, TrialCounterCount> count_zero{};
  engine::upload(
      *counters, count_zero.data(),
      (wide() ? TrialCounterCount : CounterCount) * sizeof(std::int64_t));
  std::array<float, 11> origin{};
  std::copy(fixed_origin_.begin(), fixed_origin_.end(), origin.begin() + 8);
  if (wide()) {
    std::array<double, 11> origin64{};
    std::copy(fixed_origin64_.begin(), fixed_origin64_.end(),
              origin64.begin() + 8);
    engine::upload(*focus, origin64.data(), sizeof(origin64));
  } else
    engine::upload(*focus, origin.data(), sizeof(origin));
  engine::upload(*selected_slots_array_, selected_slots_.data(),
                 selected_slots_.size() * sizeof(int));
  auto call = reset_.invocation(bool(prepared_));
  auto &context = call.get();
  if (!context.prepared)
    context.set_arg_ndarray({0}, *group_flags);
  reset_.launch(context);
}
void Activity::set_focus(const std::vector<int> &groups) {
  if (groups.size() > selected_slots_.size())
    throw std::invalid_argument("focus capacity exhausted");
  for (std::size_t i = 0; i < groups.size(); ++i) {
    const int g = groups[i];
    if (g < 0 || g >= g_ ||
        std::find(groups.begin(), groups.begin() + i, g) != groups.begin() + i)
      throw std::invalid_argument(
          "focus requires distinct prepared field slots");
    if (prepared_ && prepared_->field_headers[g].record < 0)
      throw std::invalid_argument("focus requires present field");
  }
  runtime_.synchronize();
  std::fill(selected_slots_.begin(), selected_slots_.end(), 0);
  selected_groups_.assign(groups.begin(), groups.end());
  if (!prepared_)
    std::sort(selected_groups_.begin(), selected_groups_.end());
  std::copy(selected_groups_.begin(), selected_groups_.end(),
            selected_slots_.begin());
  engine::upload(*selected_slots_array_, selected_slots_.data(),
                 selected_slots_.size() * sizeof(int));
}
void Activity::set_frame_anchor(const FrameAnchor &anchor) {
  for (int axis = 0; axis < 3; ++axis) {
    fixed_origin64_[axis] = -anchor[axis];
    fixed_origin_[axis] = static_cast<float>(-anchor[axis]);
  }
}
void Activity::index(const std::vector<int> &edges,
                     const std::vector<int> &contacts) {
  const auto build = [&](int nodes, int capacity, int links,
                         const std::vector<int> &source, int row,
                         Ndarray &offset_array, Ndarray &order_array,
                         int *offsets, int *order, int *counts, int *next) {
    const auto stride = std::size_t(std::max(nodes, 1));
    std::fill_n(offsets, 2 * stride, 0);
    std::fill_n(order, std::max(capacity, 1), 0);
    std::fill_n(counts, nodes, 0);
    for (int e = 0; e < links; ++e)
      ++counts[source[std::size_t(row) * capacity + e]];
    int cursor = 0;
    for (int n = 0; n < nodes; ++n) {
      offsets[n] = cursor;
      next[n] = cursor;
      cursor += counts[n];
      offsets[stride + n] = cursor;
    }
    for (int e = 0; e < links; ++e)
      order[next[source[std::size_t(row) * capacity + e]]++] = e;
    engine::upload(offset_array, offsets, 2 * stride * sizeof(int));
    engine::upload(order_array, order, std::max(capacity, 1) * sizeof(int));
  };
  std::vector<int> owned;
  if (!prepared_)
    owned.resize(
        6 * std::size_t(std::max(p_, 1)) + 2 * std::size_t(std::max(g_, 1)) +
        2 * std::size_t(std::max(e_, 1)) + 2 * std::size_t(std::max(k_, 1)) +
        2 * std::size_t(std::max({p_, g_, 1})));
  int *scratch = prepared_ ? prepared_->prospective_csr.data() : owned.data();
  int *counts = prepared_
                    ? prepared_->counts.data()
                    : owned.data() + owned.size() - 2 * std::max({p_, g_, 1});
  int *next =
      prepared_ ? prepared_->cursors.data() : counts + std::max({p_, g_, 1});
  const int elive = prepared_ ? int(prepared_->live.edges) : e_;
  const int klive = prepared_ ? int(prepared_->live.contacts) : k_;
  const auto one = [&](int nodes, int capacity, int live,
                       const std::vector<int> &source, int row,
                       Ndarray &offset_array, Ndarray &order_array) {
    int *offsets = scratch;
    int *order = offsets + 2 * std::max(nodes, 1);
    build(nodes, capacity, live, source, row, offset_array, order_array,
          offsets, order, counts, next);
    scratch = order + std::max(capacity, 1);
  };
  one(p_, e_, elive, edges, kEdgeParticle, *particle_offsets, *particle_edges);
  one(g_, e_, elive, edges, kEdgeGroup, *group_offsets, *group_edges);
  one(p_, k_, klive, contacts, kContactA, *contact_a_offsets, *contact_a_edges);
  one(p_, k_, klive, contacts, kContactB, *contact_b_offsets, *contact_b_edges);
}

void Activity::begin(Harness &h, bool all, bool invalid, bool radial) {
  auto call = begin_.invocation(bool(prepared_));
  auto &ctx = call.get();
  if (!ctx.prepared) {
    h.bind(ctx, 0, h.particle_array_.get(), h.particle_values_,
           {kParticleFieldCount, std::max(p_, 1)});
  }
  if (!ctx.prepared) {
    h.bind(ctx, 1, h.group_array_.get(), h.group_values_,
           {kGroupFieldCount, std::max(g_, 1)});
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({2}, *particle_flags);
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({3}, *particle_delta);
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({4}, *group_flags);
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({5}, *counters);
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({6}, *focus);
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({7}, *selected_slots_array_);
  }
  engine::argument_int(ctx, 8, p_);
  engine::argument_int(ctx, 9, g_);
  engine::argument_int(ctx, 10, all);
  engine::argument_int(ctx, 11, invalid);
  engine::argument_int(ctx, 12, int(selected_groups_.size()));
  engine::argument_int(ctx, 13, radial);
  begin_.launch(ctx);
}
void Activity::touch(Harness &h, double gravity) {
  auto call = touch_.invocation(bool(prepared_));
  auto &ctx = call.get();
  if (!ctx.prepared) {
    h.bind(ctx, 0, h.particle_array_.get(), h.particle_values_,
           {kParticleFieldCount, std::max(p_, 1)});
  }
  if (!ctx.prepared) {
    h.bind(ctx, 1, h.group_array_.get(), h.group_values_,
           {kGroupFieldCount, std::max(g_, 1)});
  }
  if (!ctx.prepared) {
    h.bind(ctx, 2, h.edge_int_array_.get(), h.edge_int_data,
           {kEdgeIntFieldCount, std::max(e_, 1)});
  }
  if (!ctx.prepared) {
    h.bind(ctx, 3, h.edge_float_array_.get(), h.edge_float_data,
           {kEdgeFloatFieldCount, std::max(e_, 1)});
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({4}, *group_flags);
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({5}, *group_delta);
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({6}, *counters);
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({7}, *group_offsets);
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({8}, *group_edges);
  }
  engine::argument_float(ctx, 9, gravity);
  if (wide() && !ctx.prepared)
    ctx.set_arg_ndarray({10}, *prepared_->live_ratios_array);
  touch_.launch(ctx);
}
void Activity::touch_contacts(Harness &h, double stiffness) {
  if ((!prepared_ && k_ == 0) || stiffness == 0)
    return;
  auto call = contact_touch_.invocation(bool(prepared_));
  auto &ctx = call.get();
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({0}, *particle_flags);
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({1}, *counters);
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({2}, *contact_a_offsets);
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({3}, *contact_a_edges);
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({4}, *contact_b_offsets);
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({5}, *contact_b_edges);
  }
  if (!ctx.prepared) {
    h.bind(ctx, 6, h.contact_int_array_.get(), h.contact_int_data,
           {kContactIntFieldCount, std::max(k_, 1)});
  }
  contact_touch_.launch(ctx);
}
void Activity::mark(Harness &h, bool all) {
  auto call = mark_.invocation(bool(prepared_));
  auto &ctx = call.get();
  if (!ctx.prepared) {
    h.bind(ctx, 0, h.particle_array_.get(), h.particle_values_,
           {kParticleFieldCount, std::max(p_, 1)});
  }
  if (!ctx.prepared) {
    h.bind(ctx, 1, h.group_array_.get(), h.group_values_,
           {kGroupFieldCount, std::max(g_, 1)});
  }
  if (!ctx.prepared) {
    h.bind(ctx, 2, h.edge_int_array_.get(), h.edge_int_data,
           {kEdgeIntFieldCount, std::max(e_, 1)});
  }
  if (!ctx.prepared) {
    h.bind(ctx, 3, h.edge_float_array_.get(), h.edge_float_data,
           {kEdgeFloatFieldCount, std::max(e_, 1)});
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({4}, *particle_flags);
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({5}, *particle_delta);
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({6}, *group_flags);
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({7}, *group_delta);
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({8}, *counters);
  }
  engine::argument_int(ctx, 9, all);
  engine::argument_int(ctx, 10, g_);
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({11}, *group_offsets);
  }
  if (!ctx.prepared) {
    ctx.set_arg_ndarray({12}, *group_edges);
  }
  mark_.launch(ctx);
}
void Activity::clear() {
  auto call = clear_.invocation(bool(prepared_));
  auto &ctx = call.get();
  clear_.launch(ctx);
}

void Activity::provision() {
  if (!prepared_)
    return;
  IRBuilder b;
  for (const auto &entry :
       {std::make_pair(particle_activity, p_), std::make_pair(centroids, g_)}) {
    auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(entry.second),
                                    false, runtime_.cpu_threads());
    auto guard = b.get_loop_guard(loop);
    entry.first.activate(b, b.get_loop_index(loop));
  }
  // Exercise native traversal high-water marks, then clear leaves. Pointer
  // backing remains prepared. No physical kernel runs with these setup bits.
  for (const auto &set : {particle_activity, centroids}) {
    auto *loop = b.create_struct_for(set.cells, false, runtime_.cpu_threads());
    auto guard = b.get_loop_guard(loop);
    set.deactivate(b, b.get_loop_index(loop));
  }
  engine::CompiledKernel provision(
      runtime_,
      "activity_provision_" + std::to_string(particle_activity.cells->id),
      b.extract_ir(), {});
  auto context = provision.make_context();
  provision.launch(context);
  runtime_.synchronize();
}

std::size_t Activity::prepared_binding_bytes() const {
  return begin_.prepared_bytes() + touch_.prepared_bytes() +
         contact_touch_.prepared_bytes() + mark_.prepared_bytes() +
         reset_.prepared_bytes() + clear_.prepared_bytes();
}
Demand Activity::report(std::uint64_t exposed, std::uint64_t fields_exposed) {
  std::array<std::int64_t, TrialCounterCount> c{};
  std::array<float, 8> origin{};
  engine::readback(
      *counters, c.data(),
      (wide() ? TrialCounterCount : CounterCount) * sizeof(std::int64_t));
  std::array<double, 8> origin64{};
  if (wide())
    engine::readback(*focus, origin64.data(), sizeof(origin64));
  else {
    engine::readback(*focus, origin.data(), origin.size() * sizeof(float));
    std::copy(origin.begin(), origin.end(), origin64.begin());
  }
  Demand d;
  if (prepared_) d.numerical_profile = prepared_->capacity.profile();
  if (wide()) {
    d.motion_attempts = c[TrialAttempts];
    d.numerical_errors = c[TrialErrors];
  }
  std::copy_n(origin64.begin(), 3, d.focal_origin.begin());
  d.particles_loaded = prepared_ ? prepared_->live.particles : p_;
  d.particles_exposed = exposed;
  d.fields_loaded = prepared_ ? prepared_->live.fields : g_;
  d.fields_exposed = fields_exposed;
  const auto live_edges = prepared_ ? prepared_->live.edges : std::size_t(e_);
  d.edges_loaded = live_edges;
  d.edges_exposed =
      live_edges;  // Validated edges reference exposed positive-mass particles.
  d.particles_evaluated = c[Evaluated];
  d.particles_skipped = exposed - c[Evaluated];
  d.particles_moved = c[Moved];
  d.particles_woken = c[Woken];
  d.force_requests = c[ForceRequests];
  d.force_evaluations = c[Forces];
  d.force_zero_response = c[ForceRequests] - c[Forces];
  d.force_skipped = live_edges - c[ForceRequests];
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

  return d;
}
}  // namespace field::detail
