#pragma once

#include "field/placement_math.h"
#include "field/contact_math.h"

namespace field::detail::placement {
// Attempt-local bounded spatial search. Its heap contains interval boxes, not
// particle work; authoritative activity remains in the existing native sets.
class Search {
 public:
  using V = taichi::lang::Stmt;
  using L = taichi::lang::AllocaStmt;
  using A = taichi::lang::ArgLoadStmt;
  using Vector = std::array<V *, 3>;
  using Box = std::array<V *, 7>;
  enum Count {
    Status,
    Work,
    RequiredWork,
    RequiredCells,
    Completed,
    Occupants,
    Peak,
    Visits,
    CandidateTests,
    Moves,
    Proposals,
    Reserved11,
    Reserved12,
    Reserved13,
    Reserved14,
    Reserved15
  };
  taichi::lang::IRBuilder &b;
  L *status;
  Math m;
  A *occupancy, *requests, *focal, *heap, *output, *counts;
  V *new_count, *work_limit, *cell_limit, *base_work;
  L *used, *required_work, *required_cells, *occupied, *heap_size, *peak,
      *visits, *tests, *moves, *proposals, *completed, *best_upper, *have_best,
      *global_lower;
  std::array<L *, 3> best;
  Vector focus;
  V *radius{};
  int bits;

  Search(taichi::lang::IRBuilder &builder,
         A *occupants,
         A *input,
         A *target,
         A *boxes,
         A *result,
         A *counters,
         V *number,
         V *current,
         V *limit,
         V *prefix,
         V *cells,
         int fraction_bits)
      : b(builder),
        status(b.create_local_var(taichi::lang::PrimitiveType::i32)),
        m(b, status),
        occupancy(occupants),
        requests(input),
        focal(target),
        heap(boxes),
        output(result),
        counts(counters),
        new_count(number),
        work_limit(limit),
        cell_limit(cells),
        base_work(prefix),
        bits(fraction_bits) {
    using namespace taichi::lang;
    b.create_local_store(status, b.get_int32(0));
    const auto u = [&](V *x) {
      auto *v = b.create_local_var(PrimitiveType::u64);
      set(v, x);
      return v;
    };
    const auto j = [&](V *x) {
      auto *v = b.create_local_var(PrimitiveType::i32);
      set(v, x);
      return v;
    };
    used = u(base_work);
    required_work = u(z());
    required_cells = u(z());
    occupied = j(current);
    heap_size = j(m.i(0));
    peak = u(z());
    visits = u(z());
    tests = u(z());
    moves = u(z());
    proposals = u(z());
    completed = u(z());
    have_best = j(m.i(0));
    best_upper = real_local(m.f(0));
    global_lower = real_local(m.f(0));
    for (int k = 0; k < 3; ++k) {
      best[k] = real_local(m.f(0));
      focus[k] = read(focal, {m.i(k)});
    }
  }
  V *v(L *x) {
    return b.create_local_load(x);
  }
  void set(L *x, V *a) {
    b.create_local_store(x, a);
  }
  V *z() {
    return b.get_uint64(0);
  }
  V *u(std::uint64_t n) {
    return b.get_uint64(n);
  }
  V *as_u(V *x) {
    return b.create_cast(x, taichi::lang::PrimitiveType::u64);
  }
  L *real_local(V *x) {
    auto *a = b.create_local_var(taichi::lang::PrimitiveType::f64);
    set(a, x);
    return a;
  }
  L *int_local(V *x) {
    auto *a = b.create_local_var(taichi::lang::PrimitiveType::i32);
    set(a, x);
    return a;
  }
  V *read(A *a, std::vector<V *> index) {
    return b.create_global_load(b.create_external_ptr(a, index));
  }
  void write(A *a, std::vector<V *> index, V *value) {
    b.create_global_store(b.create_external_ptr(a, index), value);
  }
  V *at(A *a, int field, V *row) {
    return read(a, {m.i(field), row});
  }
  void at(A *a, int field, V *row, V *value) {
    write(a, {m.i(field), row}, value);
  }
  void increment(L *x) {
    set(x, b.create_add(v(x), u(1)));
  }
  template <class F>
  void range(V *end, F body) {
    auto *index = int_local(m.i(0));
    loop([&] { return m.both(m.healthy(), b.create_cmp_lt(v(index), end)); },
         [&] {
           body(v(index));
           set(index, b.create_add(v(index), m.i(1)));
         });
  }
  template <class C, class F>
  void loop(C condition, F body) {
    auto *w = b.create_while_true();
    auto guard = b.get_loop_guard(w);
    {
      auto stop =
          b.get_if_guard(b.create_if(b.create_logical_not(condition())), true);
      b.create_break();
    }
    body();
  }
  void spend(V *amount) {
    auto active = b.get_if_guard(b.create_if(m.healthy()), true);
    auto *fits = b.create_cmp_le(amount, b.create_sub(work_limit, v(used)));
    auto *choice = b.create_if(fits);
    {
      auto yes = b.get_if_guard(choice, true);
      set(used, b.create_add(v(used), amount));
    }
    {
      auto no = b.get_if_guard(choice, false);
      set(required_work, b.create_add(v(used), amount));
      set(status, m.i(4));
    }
  }
  Vector centre(V *row) {
    return {at(occupancy, 0, row), at(occupancy, 1, row),
            at(occupancy, 2, row)};
  }
  Interval extent(V *row) {
    return m.add(m.point(radius), m.point(at(occupancy, 3, row)));
  }
  V *lex_less(const Vector &a, const Vector &c) {
    V *result = m.i(0), *equal = m.i(1);
    for (int k = 0; k < 3; ++k) {
      result = m.either(result, m.both(equal, b.create_cmp_lt(a[k], c[k])));
      equal = m.both(equal, b.create_cmp_eq(a[k], c[k]));
    }
    return result;
  }
  Vector best_point() {
    return {v(best[0]), v(best[1]), v(best[2])};
  }
  Vector published(const Vector &a) {
    Vector out;
    for (int k = 0; k < 3; ++k)
      out[k] = publish_position(b, a[k], bits);
    return out;
  }
  void consider(const Vector &raw) {
    spend(u(1));
    auto active = b.get_if_guard(b.create_if(m.healthy()), true);
    increment(proposals);
    V *domain = m.i(1);
    for (auto *x : raw)
      domain = m.both(domain, m.supported(x, -256, 300));
    // An optional analytic proposal outside the proved coordinate window is
    // skipped. It cannot affect the certificate or replace the global search.
    auto supported = b.get_if_guard(b.create_if(domain), true);
    const auto point = published(raw);
    auto *valid = int_local(m.i(1));
    range(v(occupied), [&](V *row) {
      spend(u(1));
      auto alive = b.get_if_guard(b.create_if(m.healthy()), true);
      increment(tests);
      const auto d = m.distance2(point, centre(row)), r = m.square(extent(row));
      const auto c = centre(row);
      Vector delta;
      for (int k = 0; k < 3; ++k)
        delta[k] = b.create_sub(point[k], c[k]);
      const auto contact =
          contact_geometry(b, delta, radius, at(occupancy, 3, row), true);
      set(valid, m.both(v(valid),
                        m.both(b.create_cmp_ge(d.lo, r.hi),
                               b.create_cmp_eq(contact.penetration, m.f(0)))));
    });
    auto feasible =
        b.get_if_guard(b.create_if(m.both(m.healthy(), v(valid))), true);
    const auto distance = m.root(m.distance2(point, focus));
    auto *better =
        m.either(b.create_cmp_eq(v(have_best), m.i(0)),
                 m.either(b.create_cmp_lt(distance.hi, v(best_upper)),
                          m.both(b.create_cmp_eq(distance.hi, v(best_upper)),
                                 lex_less(point, best_point()))));
    auto improve =
        b.get_if_guard(b.create_if(m.both(m.healthy(), better)), true);
    for (int k = 0; k < 3; ++k)
      set(best[k], point[k]);
    set(best_upper, distance.hi);
    set(have_best, m.i(1));
  }
  V *coordinate_bound() {
    V *bound = m.f(0);
    for (int k = 0; k < 3; ++k)
      bound = b.create_max(
          bound,
          m.add(m.point(b.create_abs(focus[k])), m.point(v(best_upper))).hi);
    return bound;
  }
  V *tolerance() {
    return m.tolerance(coordinate_bound(), v(best_upper), bits);
  }
  V *next_stored(V *x, bool positive, V *scale) {
    auto *word = b.create_bit_cast(x, taichi::lang::PrimitiveType::u64);
    auto *step = b.get_uint64(std::uint64_t{1} << (52 - bits));
    V *increase = b.create_cmp_gt(x, m.f(0));
    if (!positive)
      increase = b.create_logical_not(increase);
    auto *out =
        b.create_bit_cast(b.create_select(increase, b.create_add(word, step),
                                          b.create_sub(word, step)),
                          taichi::lang::PrimitiveType::f64);
    auto *zero_step = m.quantum(scale, bits);
    return b.create_select(b.create_cmp_eq(x, m.f(0)),
                           positive ? zero_step : b.create_neg(zero_step), out);
  }
  void neighbours(const Vector &point, V *scale) {
    // These are upper-bound proposals around an analytic surface point. The
    // interval heap, not this finite proposal set, supplies global coverage.
    range(m.i(27), [&](V *code) {
      Vector candidate;
      V *digits = code;
      for (int k = 0; k < 3; ++k) {
        auto *digit = b.create_mod(digits, m.i(3));
        digits = b.create_div(digits, m.i(3));
        auto *q = publish_position(b, point[k], bits);
        candidate[k] = b.create_select(
            b.create_cmp_eq(digit, m.i(0)), next_stored(q, false, scale),
            b.create_select(b.create_cmp_eq(digit, m.i(2)),
                            next_stored(q, true, scale), q));
      }
      consider(candidate);
    });
  }
  void seed() {
    consider(focus);
    // A feasible published focus is already within its derived quantization
    // bound of the global zero lower bound. No obstacle seeding is necessary.
    auto needed = b.get_if_guard(
        b.create_if(m.both(m.healthy(), b.create_cmp_eq(v(have_best), m.i(0)))),
        true);
    // A monotone axis sweep exits every blocking sphere after at most N
    // advances. Outward stored endpoints are checked against all occupants.
    for (int axis = 0; axis < 3; ++axis)
      for (bool positive : {false, true}) {
        std::array<L *, 3> position;
        for (int k = 0; k < 3; ++k)
          position[k] = real_local(focus[k]);
        range(b.create_add(v(occupied), m.i(1)), [&](V *) {
          range(v(occupied), [&](V *row) {
            spend(u(1));
            auto alive = b.get_if_guard(b.create_if(m.healthy()), true);
            Vector point{v(position[0]), v(position[1]), v(position[2])};
            const auto r = extent(row), d = m.distance2(point, centre(row)),
                       r2 = m.square(r);
            auto blocked =
                b.get_if_guard(b.create_if(b.create_cmp_lt(d.lo, r2.hi)), true);
            Vector edge = point;
            std::array<Interval, 3> transverse;
            for (int k = 0; k < 3; ++k)
              transverse[k] = k == axis ? m.point(m.f(0))
                                        : m.sub(m.point(point[k]),
                                                m.point(at(occupancy, k, row)));
            const auto chord2 = m.sub(r2, m.norm2(transverse));
            const auto chord = m.root(m.point(b.create_max(chord2.hi, m.f(0))));
            const auto bound =
                positive ? m.add(m.point(at(occupancy, axis, row)), chord)
                         : m.sub(m.point(at(occupancy, axis, row)), chord);
            edge[axis] = positive ? bound.hi : bound.lo;
            consider(edge);
            V *scale = b.create_max(b.create_abs(edge[axis]), r.hi);
            auto *next = next_stored(publish_position(b, edge[axis], bits),
                                     positive, scale);
            set(position[axis], positive
                                    ? b.create_max(v(position[axis]), next)
                                    : b.create_min(v(position[axis]), next));
          });
        });
        consider({v(position[0]), v(position[1]), v(position[2])});
      }
    {
      auto none = b.get_if_guard(
          b.create_if(
              m.both(m.healthy(), b.create_cmp_eq(v(have_best), m.i(0)))),
          true);
      set(status, m.i(3));
    }
    // Single-sphere distance bounds and radial proposals.
    range(v(occupied), [&](V *row) {
      spend(u(1));
      auto alive = b.get_if_guard(b.create_if(m.healthy()), true);
      const auto c = centre(row);
      const auto r = extent(row), d = m.root(m.distance2(focus, c));
      auto lower = m.sub(r, d);
      set(global_lower,
          b.create_max(v(global_lower), b.create_max(lower.lo, m.f(0))));
      auto *denom =
          b.create_select(b.create_cmp_gt(d.hi, m.f(0)), d.hi, m.f(1));
      Vector point;
      for (int k = 0; k < 3; ++k)
        point[k] =
            b.create_add(c[k], b.create_mul(b.create_div(r.hi, denom),
                                            b.create_sub(focus[k], c[k])));
      auto nonzero =
          b.get_if_guard(b.create_if(b.create_cmp_gt(d.hi, m.f(0))), true);
      neighbours(point, coordinate_bound());
    });
    // Nonnegative weighted exclusion constraints give a rigorous global lower
    // bound. A centre-segment projection captures symmetric intersection rings
    // without forcing an interval tiling to resolve their whole circumference.
    range(v(occupied), [&](V *a) {
      range(a, [&](V *c) {
        spend(u(1));
        auto alive = b.get_if_guard(b.create_if(m.healthy()), true);
        auto positive = b.get_if_guard(
            b.create_if(m.both(b.create_cmp_gt(extent(a).hi, m.f(0)),
                               b.create_cmp_gt(extent(c).hi, m.f(0)))),
            true);
        const auto ca = centre(a), cc = centre(c);
        Vector da, dc, delta;
        V *dot = m.f(0), *norm = m.f(0);
        for (int k = 0; k < 3; ++k) {
          da[k] = b.create_sub(ca[k], focus[k]);
          dc[k] = b.create_sub(cc[k], focus[k]);
          delta[k] = b.create_sub(da[k], dc[k]);
          dot = b.create_add(dot, b.create_mul(dc[k], delta[k]));
          norm = b.create_add(norm, b.create_mul(delta[k], delta[k]));
        }
        auto distinct =
            b.get_if_guard(b.create_if(b.create_cmp_gt(norm, m.f(0))), true);
        auto *w = b.create_min(
                 m.f(1),
                 b.create_max(m.f(0), b.create_div(b.create_neg(dot), norm))),
             *other = b.create_sub(m.f(1), w);
        auto weights =
            b.get_if_guard(b.create_if(m.both(m.supported(w, -128, 1),
                                              m.supported(other, -128, 1))),
                           true);
        std::array<Interval, 3> centre_interval;
        for (int k = 0; k < 3; ++k)
          centre_interval[k] = m.add(
              m.mul(m.point(w), m.sub(m.point(ca[k]), m.point(focus[k]))),
              m.mul(m.point(other), m.sub(m.point(cc[k]), m.point(focus[k]))));
        const auto wa = m.mul(
            m.point(w), m.sub(m.square(extent(a)), m.distance2(ca, focus)));
        const auto wc = m.mul(
            m.point(other), m.sub(m.square(extent(c)), m.distance2(cc, focus)));
        const auto power = m.add(wa, wc);
        V *supported = m.both(b.create_cmp_gt(power.lo, m.f(0)),
                              m.supported(power.lo, -384, 384));
        for (const auto &component : centre_interval)
          supported =
              m.both(supported, m.both(m.supported(component.lo, -384, 384),
                                       m.supported(component.hi, -384, 384)));
        auto useful = b.get_if_guard(b.create_if(supported), true);
        const auto centre_norm = m.root(m.norm2(centre_interval));
        const auto total = m.add(m.point(w), m.point(other));
        const auto denominator =
            m.add(m.root(m.add(m.square(centre_norm),
                               m.mul(total, m.point(power.lo)))),
                  centre_norm);
        const auto lower = m.divide_positive(m.point(power.lo), denominator);
        set(global_lower, b.create_max(v(global_lower), lower.lo));
      });
    });
  }
  V *box_less(const Box &a, const Box &c) {
    V *answer = b.create_cmp_lt(a[6], c[6]),
      *equal = b.create_cmp_eq(a[6], c[6]);
    for (int k = 0; k < 6; ++k) {
      answer = m.either(answer, m.both(equal, b.create_cmp_lt(a[k], c[k])));
      equal = m.both(equal, b.create_cmp_eq(a[k], c[k]));
    }
    return answer;
  }
  Box load_box(V *index) {
    Box out;
    for (int k = 0; k < 7; ++k)
      out[k] = at(heap, k, index);
    return out;
  }
  void store_box(V *index, const Box &box) {
    for (int k = 0; k < 7; ++k)
      at(heap, k, index, box[k]);
  }
  void push(const Box &box) {
    spend(u(1));
    auto alive = b.get_if_guard(b.create_if(m.healthy()), true);
    auto *fits = b.create_if(b.create_cmp_lt(v(heap_size), cell_limit));
    {
      auto no = b.get_if_guard(fits, false);
      set(required_cells, b.create_add(as_u(v(heap_size)), u(1)));
      set(status, m.i(4));
    }
    {
      auto yes = b.get_if_guard(fits, true);
      auto *index = int_local(v(heap_size));
      set(heap_size, b.create_add(v(heap_size), m.i(1)));
      set(peak, b.create_max(v(peak), as_u(v(heap_size))));
      auto *moving = int_local(m.i(1));
      loop(
          [&] {
            return m.both(m.healthy(),
                          m.both(v(moving), b.create_cmp_gt(v(index), m.i(0))));
          },
          [&] {
            spend(u(1));
            auto active = b.get_if_guard(b.create_if(m.healthy()), true);
            increment(moves);
            auto *parent = b.create_div(b.create_sub(v(index), m.i(1)), m.i(2));
            const auto old = load_box(parent);
            auto *order = b.create_if(box_less(box, old));
            {
              auto earlier = b.get_if_guard(order, true);
              store_box(v(index), old);
              set(index, parent);
            }
            {
              auto later = b.get_if_guard(order, false);
              set(moving, m.i(0));
            }
          });
      store_box(v(index), box);
    }
  }
  Box pop() {
    spend(u(1));
    std::array<L *, 7> result;
    const auto first = load_box(m.i(0));
    for (int k = 0; k < 7; ++k)
      result[k] = real_local(first[k]);
    set(heap_size, b.create_sub(v(heap_size), m.i(1)));
    const auto last = load_box(v(heap_size));
    auto *index = int_local(m.i(0)), *moving = int_local(m.i(1));
    loop(
        [&] {
          return m.both(
              m.healthy(),
              m.both(v(moving),
                     b.create_cmp_lt(
                         b.create_add(b.create_mul(v(index), m.i(2)), m.i(1)),
                         v(heap_size))));
        },
        [&] {
          spend(u(1));
          auto active = b.get_if_guard(b.create_if(m.healthy()), true);
          increment(moves);
          auto *left = b.create_add(b.create_mul(v(index), m.i(2)), m.i(1)),
               *right = b.create_add(left, m.i(1));
          auto *child = int_local(left);
          {
            auto both = b.get_if_guard(
                b.create_if(b.create_cmp_lt(right, v(heap_size))), true);
            auto *choose =
                b.create_if(box_less(load_box(right), load_box(left)));
            auto choose_right = b.get_if_guard(choose, true);
            set(child, right);
          }
          const auto next = load_box(v(child));
          auto *order = b.create_if(box_less(next, last));
          {
            auto earlier = b.get_if_guard(order, true);
            store_box(v(index), next);
            set(index, v(child));
          }
          {
            auto later = b.get_if_guard(order, false);
            set(moving, m.i(0));
          }
        });
    store_box(v(index), last);
    Box out;
    for (int k = 0; k < 7; ++k)
      out[k] = v(result[k]);
    return out;
  }
  void examine(Box box) {
    spend(u(1));
    auto alive = b.get_if_guard(b.create_if(m.healthy()), true);
    increment(visits);
    std::array<Interval, 3> relative;
    for (int k = 0; k < 3; ++k)
      relative[k] = {box[k], box[3 + k]};
    box[6] = b.create_max(v(global_lower), m.root(m.norm2(relative)).lo);
    auto *excluded = int_local(m.i(0));
    range(v(occupied), [&](V *row) {
      spend(u(1));
      auto active = b.get_if_guard(b.create_if(m.healthy()), true);
      const auto c = centre(row);
      std::array<Interval, 3> d;
      for (int k = 0; k < 3; ++k)
        d[k] = m.sub(relative[k], m.sub(m.point(c[k]), m.point(focus[k])));
      const auto maximum = m.norm2(d), r = m.square(extent(row));
      set(excluded, m.either(v(excluded), b.create_cmp_lt(maximum.hi, r.lo)));
    });
    auto relevant = b.get_if_guard(
        b.create_if(m.both(m.healthy(),
                           m.both(b.create_cmp_eq(v(excluded), m.i(0)),
                                  b.create_cmp_lt(box[6], v(best_upper))))),
        true);
    Vector near, mid;
    for (int k = 0; k < 3; ++k) {
      auto *closest = b.create_min(box[3 + k], b.create_max(box[k], m.f(0)));
      near[k] = b.create_add(focus[k], closest);
      mid[k] = b.create_add(focus[k],
                            b.create_add(b.create_mul(box[k], m.f(.5)),
                                         b.create_mul(box[3 + k], m.f(.5))));
    }
    consider(near);
    consider(mid);
    auto retain = b.get_if_guard(
        b.create_if(
            m.both(m.healthy(), b.create_cmp_lt(box[6], v(best_upper)))),
        true);
    push(box);
  }
  void find() {
    seed();
    auto alive = b.get_if_guard(b.create_if(m.healthy()), true);
    m.invalid(b.create_cmp_gt(v(global_lower), v(best_upper)));
    auto *done = int_local(m.i(0));
    {
      auto certified = b.get_if_guard(
          b.create_if(b.create_cmp_le(
              m.sub(m.point(v(best_upper)), m.point(v(global_lower))).hi,
              tolerance())),
          true);
      set(done, m.i(1));
    }
    {
      auto needed =
          b.get_if_guard(b.create_if(b.create_cmp_eq(v(done), m.i(0))), true);
      Box root;
      for (int k = 0; k < 3; ++k) {
        root[k] = b.create_neg(v(best_upper));
        root[3 + k] = v(best_upper);
      }
      root[6] = m.f(0);
      examine(root);
    }
    loop(
        [&] { return m.both(m.healthy(), b.create_cmp_eq(v(done), m.i(0))); },
        [&] {
          auto *choice = b.create_if(b.create_cmp_eq(v(heap_size), m.i(0)));
          {
            auto empty = b.get_if_guard(choice, true);
            set(global_lower, v(best_upper));
            set(done, m.i(1));
          }
          {
            auto pending = b.get_if_guard(choice, false);
            auto *lower = b.create_min(
                v(best_upper),
                b.create_max(v(global_lower), at(heap, 6, m.i(0))));
            auto *certified = b.create_if(b.create_cmp_le(
                m.sub(m.point(v(best_upper)), m.point(lower)).hi, tolerance()));
            {
              auto success = b.get_if_guard(certified, true);
              set(global_lower, lower);
              set(done, m.i(1));
            }
            {
              auto refine = b.get_if_guard(certified, false);
              const auto box = pop();
              auto *axis = int_local(m.i(0));
              auto *width = real_local(b.create_sub(box[3], box[0]));
              for (int k = 1; k < 3; ++k) {
                auto *w = b.create_sub(box[3 + k], box[k]);
                auto larger = b.get_if_guard(
                    b.create_if(b.create_cmp_gt(w, v(width))), true);
                set(axis, m.i(k));
                set(width, w);
              }
              for (int k = 0; k < 3; ++k) {
                auto selected = b.get_if_guard(
                    b.create_if(b.create_cmp_eq(v(axis), m.i(k))), true);
                auto *middle = b.create_add(b.create_mul(box[k], m.f(.5)),
                                            b.create_mul(box[3 + k], m.f(.5)));
                m.invalid(m.either(b.create_cmp_le(middle, box[k]),
                                   b.create_cmp_ge(middle, box[3 + k])));
                Box left = box, right = box;
                left[3 + k] = middle;
                right[k] = middle;
                examine(left);
                examine(right);
              }
            }
          }
        });
  }
  void emit() {
    // Per-attempt output tails are deterministic, including after a rejected
    // larger proposal. The heap itself is private and reset by its live count.
    range(b.create_arg_load({11}, taichi::lang::PrimitiveType::i32, false, 0),
          [&](V *row) {
            for (int k = 0; k < 7; ++k)
              at(output, k, row, m.f(0));
          });
    range(new_count, [&](V *rank) {
      auto active = b.get_if_guard(b.create_if(m.healthy()), true);
      radius = at(requests, 1, rank);
      set(heap_size, m.i(0));
      set(have_best, m.i(0));
      set(best_upper, m.f(0));
      set(global_lower, m.f(0));
      find();
      auto certified = b.get_if_guard(b.create_if(m.healthy()), true);
      for (int k = 0; k < 3; ++k) {
        at(output, k, rank, v(best[k]));
        at(occupancy, k, v(occupied), v(best[k]));
      }
      at(occupancy, 3, v(occupied), radius);
      set(occupied, b.create_add(v(occupied), m.i(1)));
      at(output, 3, rank, v(global_lower));
      at(output, 4, rank, v(best_upper));
      at(output, 5, rank, tolerance());
      at(output, 6, rank, coordinate_bound());
      increment(completed);
    });
    const std::array<V *, 16> result{as_u(v(status)),
                                     v(used),
                                     v(required_work),
                                     v(required_cells),
                                     v(completed),
                                     as_u(v(occupied)),
                                     v(peak),
                                     v(visits),
                                     v(tests),
                                     v(moves),
                                     v(proposals),
                                     z(),
                                     z(),
                                     z(),
                                     z(),
                                     z()};
    for (int k = 0; k < 16; ++k)
      write(counts, {m.i(k)}, result[k]);
  }
};
}  // namespace field::detail::placement
