#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <type_traits>

#include "allocation_observation.h"
#include "engine/packed_view.h"
#include "engine/precision.h"
#include "engine/transfer.h"
#include "prepared_metrics.h"

namespace {
using namespace taichi::lang;
using engine::numeric::native_type;
using engine::numeric::Precision;
using engine::numeric::Real;
using engine::numeric::truncate_position;
void require(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
template <class F>
void rejects(F f) {
  try {
    f();
  } catch (const std::exception &) {
    return;
  }
  throw std::runtime_error("invalid numerical setting/view accepted");
}
template <class F>
void no_new(F f) {
  allocation_observation::calls = 0;
  allocation_observation::enabled = true;
  try {
    f();
  } catch (...) {
    allocation_observation::enabled = false;
    throw;
  }
  allocation_observation::enabled = false;
  require(!allocation_observation::calls,
          "prepared numerical probe allocated C++ metadata");
}
template <class T>
constexpr Real format() {
  return sizeof(T) == 4 ? Real::F32 : Real::F64;
}
template <class T>
bool same(T a, T b) {
  return std::memcmp(&a, &b, sizeof(T)) == 0;
}
template <class T>
using Word = std::conditional_t<sizeof(T) == 4, std::uint32_t, std::uint64_t>;
template <class T>
Word<T> bits(T value) {
  Word<T> result;
  std::memcpy(&result, &value, sizeof(T));
  return result;
}

// Independent arithmetic oracle: binade scaling and truncation, never the
// implementation's integer mask. long double has enough range/significand on
// the qualified native CPU build to represent both tested IEEE containers.
template <class T>
T oracle(T x, int b) {
  if (x == 0)
    return x;
  int exponent = 0;
  const long double magnitude = std::fabs(static_cast<long double>(x));
  std::frexp(magnitude, &exponent);
  const int e =
      std::max(exponent - 1, std::numeric_limits<T>::min_exponent - 1);
  const auto coarse =
      std::ldexp(std::trunc(std::ldexp(magnitude, b - e)), e - b);
  return std::copysign(static_cast<T>(coarse), x);
}
template <class T>
std::vector<T> cases() {
  std::vector<T> v;
  auto add = [&](T x) {
    v.push_back(x);
    v.push_back(-x);
  };
  add(0);
  add(1);
  add(std::numeric_limits<T>::denorm_min());
  add(std::nextafter(std::numeric_limits<T>::min(), T(0)));
  add(std::numeric_limits<T>::min());
  add(std::numeric_limits<T>::max());
  for (int e : {std::numeric_limits<T>::min_exponent - 1, -50, -20, -1, 0, 1,
                24, 25, 50, std::numeric_limits<T>::max_exponent - 2}) {
    const T x = std::ldexp(T(1), e);
    if (!std::isfinite(x) || x == 0)
      continue;
    for (T p : {x, std::nextafter(x, T(0)),
                std::nextafter(x, std::numeric_limits<T>::infinity()),
                T(x * T(1.25)), T(x * T(1.5)), T(x * T(1.75))})
      add(p);
  }
  std::mt19937_64 random(172);
  for (int i = 0; i < 128; ++i) {
    const Word<T> raw = Word<T>(random());
    T x;
    std::memcpy(&x, &raw, sizeof(T));
    v.push_back(x);
  }
  v.push_back(std::numeric_limits<T>::infinity());
  v.push_back(-std::numeric_limits<T>::infinity());
  v.push_back(std::numeric_limits<T>::quiet_NaN());
  v.push_back(std::numeric_limits<T>::signaling_NaN());
  // Values just above/below f32 conversion ties, exponent/underflow boundaries.
  for (double x :
       {std::ldexp(1.0, -149), std::ldexp(8388607.0, -149),
        std::ldexp(1.0, -126), 1.0 + std::ldexp(1.0, -24),
        1.0 + 3 * std::ldexp(1.0, -24), std::ldexp(1.0, -150),
        3 * std::ldexp(1.0, -150), double(std::numeric_limits<float>::max()),
        double(std::numeric_limits<float>::max()) + std::ldexp(1.0, 103),
        std::ldexp(1.0, 128)}) {
    add(T(x));
    add(T(std::nextafter(x, 0.0)));
    add(T(std::nextafter(x, std::numeric_limits<double>::infinity())));
  }
  return v;
}

// Test-owned typed payloads, fixed before binding. Kernel/context variables are
// declared after these owners so prepared borrows finish before their storage.
template <class T>
struct Buffer {
  std::vector<T> host;
  std::vector<int> shape;
  std::unique_ptr<Ndarray> array;
  Buffer(engine::Runtime &r,
         bool direct,
         std::vector<int> dimensions,
         DataType dt)
      : shape(std::move(dimensions)) {
    std::size_t n = 1;
    for (int a : shape)
      n *= a;
    host.resize(n);
    if (!direct) {
      array = std::make_unique<Ndarray>(&r.program(), dt, shape);
      r.program().fill_ndarray_fast_u32(array.get(), 0);
    }
  }
  std::size_t bytes() const {
    return host.size() * sizeof(T);
  }
  void bind(LaunchContextBuilder &ctx, int arg) {
    engine::PackedView<T> view(host, shape, 1);
    if (array)
      ctx.set_arg_ndarray({arg}, *array);
    else
      ctx.set_arg_external_array_with_shape(
          {arg}, reinterpret_cast<std::uintptr_t>(view.data()), view.bytes(),
          std::vector<taichi::int64>(shape.begin(), shape.end()));
  }
  void upload() {
    if (array)
      engine::upload(*array, host.data(), bytes());
  }
  void download() {
    if (array)
      engine::readback(*array, host.data(), bytes());
  }
};
Stmt *load(IRBuilder &b, ArgLoadStmt *a, std::vector<Stmt *> index) {
  return b.create_global_load(b.create_external_ptr(a, index));
}
void store(IRBuilder &b, ArgLoadStmt *a, std::vector<Stmt *> index, Stmt *v) {
  b.create_global_store(b.create_external_ptr(a, index), v);
}

void settings() {
  std::vector<double> values{1, 2, 3, 4};
  engine::PackedView<double> view(values, {2, 2}, 7);
  require(view.bytes() == 32 && view.data() == values.data() &&
              view.at(1, 1) == 4 && view.generation() == 7,
          "double packed element width/shape/generation wrong");
  require(reinterpret_cast<std::uintptr_t>(view.data()) % alignof(double) == 0,
          "misaligned double vector");
  rejects([&] { engine::PackedView<double> bad(values, {2, 2}, 0); });
  rejects([&] { engine::PackedView<double> bad(values, {}, 1); });
  rejects([&] { engine::PackedView<double> bad(values, {0, 4}, 1); });
  rejects([&] { engine::PackedView<double> bad(values, {3}, 1); });
  rejects([&] {
    engine::PackedView<double> bad(values, {INT32_MAX, INT32_MAX, INT32_MAX},
                                   1);
  });
  rejects([&] { view.at(2, 0); });
  engine::PackedView<double> one(values, {4}, 1);
  rejects([&] { one.at(0, 0); });
  for (Real calc : {Real::F32, Real::F64})
    for (Real stored : {Real::F32, Real::F64}) {
      rejects([&] { Precision p(calc, stored, -1); });
      rejects([&] {
        Precision p(calc, stored, engine::numeric::fraction_bits(stored) + 1);
      });
    }
  rejects([] { Precision p(static_cast<Real>(99), Real::F64, 52); });
  rejects([] { Precision p(Real::F64, static_cast<Real>(99), 0); });
}

template <class C, class S>
void boundaries(engine::Runtime &r, bool direct) {
  const auto source = cases<C>();
  const int n = int(source.size());
  constexpr int m = std::numeric_limits<S>::digits - 1;
  Precision(format<C>(), format<S>(), m).require_backend(r);
  Buffer<C> input(r, direct, {n}, native_type(format<C>()));
  input.host = source;
  Buffer<S> output(r, direct, {m + 1, n}, native_type(format<S>()));
  Buffer<int> status(r, direct, {2 * (m + 1), n}, PrimitiveType::i32);
  IRBuilder b;
  auto *in = b.create_ndarray_arg_load({0}, native_type(format<C>()), 1, 0);
  auto *out = b.create_ndarray_arg_load({1}, native_type(format<S>()), 2, 0);
  auto *valid = b.create_ndarray_arg_load({2}, PrimitiveType::i32, 2, 0);
  auto *count = b.create_arg_load({3}, PrimitiveType::i32, false, 0);
  auto *loop =
      b.create_range_for(b.get_int32(0), count, false, r.cpu_threads());
  {
    auto guard = b.get_loop_guard(loop);
    auto *i = b.get_loop_index(loop);
    auto *x = load(b, in, {i});
    for (int retained = 0; retained <= m; ++retained) {
      const Precision p(format<C>(), format<S>(), retained);
      const auto q = truncate_position(b, x, p),
                 again = truncate_position(
                     b, q.value, Precision(format<S>(), format<S>(), retained));
      store(b, out, {b.get_int32(retained), i}, q.value);
      store(b, valid, {b.get_int32(retained), i}, q.valid);
      store(b, valid, {b.get_int32(m + 1 + retained), i},
            b.create_cmp_eq(q.value, again.value));
    }
  }
  const auto name = std::to_string(sizeof(C) * 8) + "_" +
                    std::to_string(sizeof(S) * 8) +
                    (direct ? "_direct" : "_bulk");
  engine::CompiledKernel kernel(
      r, "precision_boundaries_" + name, b.extract_ir(),
      {engine::Param::array(native_type(format<C>())),
       engine::Param::array(native_type(format<S>()), 2),
       engine::Param::array(PrimitiveType::i32, 2),
       engine::Param::scalar(PrimitiveType::i32)},
      engine::MathMode::Strict);
  auto call = kernel.invocation(true);
  auto &ctx = call.get();
  input.bind(ctx, 0);
  output.bind(ctx, 1);
  status.bind(ctx, 2);
  input.upload();
  engine::argument_int(ctx, 3, 0);
  kernel.launch(ctx);
  r.synchronize();
  prepared_test::Metrics metrics(r, {});
  const auto before = metrics.sample();
  const auto start = std::chrono::steady_clock::now();
  no_new([&] {
    engine::argument_int(ctx, 3, n);
    kernel.launch(ctx);
    r.synchronize();
    output.download();
    status.download();
  });
  const auto first_us = std::chrono::duration<double, std::micro>(
                            std::chrono::steady_clock::now() - start)
                            .count();
  std::size_t tested = 0, invalid = 0;
  for (int retained = 0; retained <= m; ++retained)
    for (int i = 0; i < n; ++i) {
      const S converted = static_cast<S>(source[i]);
      bool finite = std::isfinite(source[i]) && std::isfinite(converted);
      if constexpr (sizeof(C) == 8 && sizeof(S) == 4) {
        const auto magnitude = bits(source[i]) & 0x7fffffffffffffffULL;
        if (magnitude != 0 && magnitude < 0x3810000000000000ULL)
          finite = false;
      }
      const S want = finite ? oracle(converted, retained) : S(0);
      const auto got = output.host[retained * n + i];
      if (status.host[retained * n + i] != int(finite) || !same(want, got)) {
        std::cerr << name << " retained=" << retained << " row=" << i
                  << " input=" << std::hexfloat << source[i]
                  << " converted=" << converted << " expected=" << want
                  << " actual=" << got << std::defaultfloat << '\n';
        throw std::runtime_error(
            "native quantization differs from independent arithmetic oracle");
      }
      if (status.host[(m + 1 + retained) * n + i] != 1) {
        std::cerr << name << " retained=" << retained << " row=" << i
                  << " output=" << std::hexfloat << got << std::defaultfloat
                  << '\n';
        throw std::runtime_error(
            "native stored-container quantization is not idempotent");
      }
      if (finite) {
        require(std::fabs(got) <= std::fabs(converted) &&
                    std::signbit(got) == std::signbit(converted),
                "quantization sign/magnitude changed");
        const Word<S> exponent = sizeof(S) == 4
                                     ? Word<S>(0x7f800000ULL)
                                     : Word<S>(0x7ff0000000000000ULL);
        require((bits(got) & exponent) == (bits(converted) & exponent),
                "quantization changed exponent");
        if (converted != 0) {
          int e = 0;
          std::frexp(std::fabs(static_cast<long double>(converted)), &e);
          const auto spacing = std::ldexp(
              1.0L, std::max(e - 1, std::numeric_limits<S>::min_exponent - 1) -
                        retained);
          require(
              std::fabs(static_cast<long double>(converted) - got) < spacing,
              "truncation error bound exceeded");
        }
      } else
        ++invalid;
      ++tested;
    }
  // Known encodings independently verify normal ties and explicit FTZ-range
  // rejection. The host's own native cast also flushes and cannot be the sole
  // oracle for conversion domain support.
  if constexpr (sizeof(C) == 8 && sizeof(S) == 4) {
    for (const auto &item : std::array<std::pair<double, std::uint32_t>, 7>{
             {{std::ldexp(1.0, -149), 0},
              {std::ldexp(8388607.0, -149), 0},
              {std::ldexp(1.0, -126), 0x00800000},
              {std::ldexp(1.0, -150), 0},
              {3 * std::ldexp(1.0, -150), 0},
              {1 + std::ldexp(1.0, -24), 0x3f800000},
              {1 + 3 * std::ldexp(1.0, -24), 0x3f800002}}})
      for (int sign : {1, -1}) {
        bool found = false;
        for (int i = 0; i < n; ++i)
          if (bits(source[i]) == bits(C(sign * item.first))) {
            const bool supported =
                std::fabs(item.first) >= std::ldexp(1.0, -126);
            const auto expected =
                item.second | (supported && sign < 0 ? 0x80000000u : 0u);
            if (status.host[m * n + i] != int(supported) ||
                bits(output.host[m * n + i]) != expected) {
              std::cerr << "known conversion source=" << std::hexfloat
                        << source[i] << std::defaultfloat
                        << " expected_bits=" << std::hex << expected
                        << " actual_bits=" << bits(output.host[m * n + i])
                        << std::dec << '\n';
              throw std::runtime_error(
                  "backend f64/f32 conversion violates known FTZ-rejection/tie "
                  "encoding");
            }
            found = true;
          }
        require(found, "known conversion fixture omitted");
      }
  }
  if constexpr (sizeof(C) == 4 && sizeof(S) == 8) {
    for (std::uint32_t raw : {1u, 0x007fffffu, 0x00800000u})
      for (int sign : {1, -1}) {
        const auto key = raw | (sign < 0 ? 0x80000000u : 0u);
        const long double exact =
            raw == 0x00800000u
                ? std::ldexp(1.0L, -126)
                : std::ldexp(static_cast<long double>(raw), -149);
        bool found = false;
        for (int i = 0; i < n; ++i)
          if (bits(source[i]) == key) {
            require(same(output.host[m * n + i], S(sign * exact)),
                    "backend lost f32 subnormal on f64 promotion");
            found = true;
          }
        require(found, "subnormal promotion fixture omitted");
      }
  }
  for (int cycle = 0; cycle < 32; ++cycle)
    no_new([&] {
      input.upload();
      kernel.launch(ctx);
      r.synchronize();
      output.download();
      status.download();
    });
  require(std::memcmp(input.host.data(), source.data(), input.bytes()) == 0,
          "probe changed input storage");
  const auto after = metrics.sample();
  require(before.same_backing(after),
          "precision first-use/reuse acquired native backing or compilation");
  before.print(("before_" + name).c_str());
  after.print(("after_" + name).c_str());
  std::cout << "boundaries," << name << ",cases=" << tested
            << ",invalid=" << invalid << ",cycles=32,host_bytes="
            << input.bytes() + output.bytes() + status.bytes()
            << ",backend_bytes="
            << (direct ? 0 : input.bytes() + output.bytes() + status.bytes())
            << ",bindings=" << kernel.prepared_bytes()
            << ",first_us=" << first_us << ",cpp_new=0 PASS\n";
}

template <class C, class S>
void arithmetic(engine::Runtime &r, bool direct) {
  constexpr int n = 4, fields = 5,
                bits_kept = std::numeric_limits<S>::digits - 3;
  const Precision p(format<C>(), format<S>(), bits_kept);
  p.require_backend(r);
  const C delta = C(std::ldexp(1.0, -bits_kept));
  Buffer<C> input(r, direct, {4, n}, native_type(format<C>()));
  Buffer<S> output(r, direct, {fields, n}, native_type(format<S>()));
  Buffer<int> status(r, direct, {4, n}, PrimitiveType::i32);
  Buffer<float> diagnostic(r, direct, {n}, PrimitiveType::f32);
  const std::array<std::array<C, 3>, n> inputs{
      {{C(std::ldexp(1.0, 25)), C(1), C(-std::ldexp(1.0, 25))},
       {C(1), C(delta * C(.75)), C(delta * C(.75))},
       {C(-1), C(-delta * C(.75)), C(-delta * C(.75))},
       {C(1), C(delta * C(.25)), C(0)}}};
  for (int i = 0; i < n; ++i) {
    for (int field = 0; field < 3; ++field)
      input.host[field * n + i] = inputs[i][field];
    input.host[3 * n + i] = C(.125);
  }
  IRBuilder b;
  const auto ct = native_type(format<C>()), st = native_type(format<S>());
  auto *in = b.create_ndarray_arg_load({0}, ct, 2, 0),
       *out = b.create_ndarray_arg_load({1}, st, 2, 0),
       *ok = b.create_ndarray_arg_load({2}, PrimitiveType::i32, 2, 0),
       *diag = b.create_ndarray_arg_load({3}, PrimitiveType::f32, 1, 0);
  auto *scalar = b.create_arg_load({4}, ct, false, 0);
  auto *count = b.create_arg_load({5}, PrimitiveType::i32, false, 0);
  auto *loop =
      b.create_range_for(b.get_int32(0), count, false, r.cpu_threads());
  {
    auto guard = b.get_loop_guard(loop);
    auto *i = b.get_loop_index(loop);
    auto *a = load(b, in, {b.get_int32(0), i}),
         *v = load(b, in, {b.get_int32(1), i}),
         *c = load(b, in, {b.get_int32(2), i});
    const auto combined =
        truncate_position(b, b.create_add(b.create_add(a, v), c), p);
    const auto first = truncate_position(b, b.create_add(a, v), p);
    const auto premature = truncate_position(
        b, b.create_add(b.create_cast(first.value, ct), c), p);
    const auto late = truncate_position(
        b,
        b.create_add(b.create_add(b.create_cast(a, PrimitiveType::f32),
                                  b.create_cast(v, PrimitiveType::f32)),
                     b.create_cast(c, PrimitiveType::f32)),
        p);
    const auto parameter = truncate_position(b, scalar, p);
    const std::array<engine::numeric::QuantizedPosition, 4> q{
        {combined, premature, late, parameter}};
    for (int field = 0; field < 4; ++field) {
      store(b, out, {b.get_int32(field), i}, q[field].value);
      store(b, ok, {b.get_int32(field), i}, q[field].valid);
    }
    store(b, out, {b.get_int32(4), i},
          b.create_cast(load(b, in, {b.get_int32(3), i}), st));
    // Deliberately lower-width independent diagnostic; no effect on position.
    store(b, diag, {i},
          b.create_mul(b.create_cast(v, PrimitiveType::f32),
                       b.create_cast(v, PrimitiveType::f32)));
  }
  const auto name = std::to_string(sizeof(C) * 8) + "_" +
                    std::to_string(sizeof(S) * 8) +
                    (direct ? "_direct" : "_bulk");
  engine::CompiledKernel kernel(
      r, "precision_arithmetic_" + name, b.extract_ir(),
      {engine::Param::array(ct, 2), engine::Param::array(st, 2),
       engine::Param::array(PrimitiveType::i32, 2),
       engine::Param::array(PrimitiveType::f32), engine::Param::scalar(ct),
       engine::Param::scalar(PrimitiveType::i32)},
      engine::MathMode::Strict);
  auto call = kernel.invocation(true);
  auto &ctx = call.get();
  input.bind(ctx, 0);
  output.bind(ctx, 1);
  status.bind(ctx, 2);
  diagnostic.bind(ctx, 3);
  input.upload();
  engine::argument_float(ctx, 4, 1 + std::ldexp(1.0, -40));
  engine::argument_int(ctx, 5, 0);
  kernel.launch(ctx);
  r.synchronize();
  prepared_test::Metrics metrics(r, {});
  const auto before = metrics.sample();
  no_new([&] {
    engine::argument_int(ctx, 5, n);
    kernel.launch(ctx);
    r.synchronize();
    output.download();
    status.download();
    diagnostic.download();
  });
  for (int i = 0; i < n; ++i) {
    const C a = inputs[i][0], v = inputs[i][1], c = inputs[i][2];
    volatile C sum = a + v;
    const C combined = sum + c;
    const S want = oracle(static_cast<S>(combined), bits_kept);
    volatile C intermediate =
        static_cast<C>(oracle(static_cast<S>(sum), bits_kept));
    const S premature = oracle(static_cast<S>(C(intermediate + c)), bits_kept);
    require(same(output.host[i], want) && same(output.host[n + i], premature),
            "selected arithmetic/premature-quantization result wrong");
    require(output.host[2 * n + i] ==
                oracle(static_cast<S>(C(float(float(a) + float(v)) + float(c))),
                       bits_kept),
            "late-widening negative control wrong");
    require(
        same(output.host[3 * n + i],
             oracle(static_cast<S>(C(1 + std::ldexp(1.0, -40))), bits_kept)),
        "f64 scalar/cache/store narrowed unexpectedly");
    require(output.host[4 * n + i] == S(.125) &&
                diagnostic.host[i] == float(v) * float(v),
            "physical velocity or independent f32 diagnostic changed");
  }
  require(
      output.host[0] == S(sizeof(C) == 8 ? 1 : 0) && output.host[2 * n] == 0,
      "wider calculation did not preserve cancellation detail");
  if (sizeof(C) == 8)
    require(output.host[n + 1] != output.host[1],
            "individual filtering hid the combined-contribution difference");
  // Feed back only accepted published position; no rounding remainder exists.
  // Change the prepared scalar every cycle to exercise the already frozen ABI.
  for (int cycle = 0; cycle < 32; ++cycle) {
    const double parameter = 1 + std::ldexp(double(cycle + 1), -40);
    no_new([&] {
      engine::argument_float(ctx, 4, parameter);
      input.upload();
      kernel.launch(ctx);
      r.synchronize();
      output.download();
      status.download();
      diagnostic.download();
    });
    require(output.host[3] == S(1) && output.host[4 * n + 3] == S(.125),
            "discarded positional detail accumulated or velocity reset");
    require(same(output.host[3 * n],
                 oracle(static_cast<S>(C(parameter)), bits_kept)),
            "prepared scalar lost f64 detail on reuse");
    require(std::all_of(status.host.begin(), status.host.end(),
                        [](int v) { return v == 1; }),
            "finite arithmetic rejected");
    input.host[3] = C(output.host[3]);
  }
  int boundary_trials = 0;
  if constexpr (std::numeric_limits<C>::digits >=
                std::numeric_limits<S>::digits - 2) {
    for (int sign : {1, -1})
      for (int cycle = 0; cycle < 16; ++cycle) {
        input.host[3] = C(sign * (C(1) + delta));
        input.host[n + 3] = C(sign * (cycle % 2 ? C(.25) : C(-.25)) * delta);
        no_new([&] {
          input.upload();
          kernel.launch(ctx);
          r.synchronize();
          output.download();
        });
        require(output.host[3] == S(sign * (cycle % 2 ? C(1) + delta : C(1))),
                "boundary crossing was incorrectly claimed to suppress all "
                "oscillation");
        ++boundary_trials;
      }
  }
  require(before.same_backing(metrics.sample()),
          "arithmetic probe acquired native backing/layout/compilation");
  before.print(("arithmetic_before_" + name).c_str());
  metrics.sample().print(("arithmetic_after_" + name).c_str());
  std::cout << "arithmetic," << name << ",retained=" << bits_kept
            << ",discard_cycles=32,scalar_updates=32,boundary_trials="
            << boundary_trials << ",host_bytes="
            << input.bytes() + output.bytes() + status.bytes() +
                   diagnostic.bytes()
            << ",backend_bytes="
            << (direct ? 0
                       : input.bytes() + output.bytes() + status.bytes() +
                             diagnostic.bytes())
            << ",bindings=" << kernel.prepared_bytes() << ",cpp_new=0 PASS\n";
}
}  // namespace
int main(int argc, char **argv) {
  try {
    static_assert(std::numeric_limits<float>::is_iec559 &&
                  std::numeric_limits<float>::digits == 24);
    static_assert(std::numeric_limits<double>::is_iec559 &&
                  std::numeric_limits<double>::digits == 53);
    static_assert(std::numeric_limits<long double>::digits >= 53 &&
                  std::numeric_limits<long double>::min_exponent < -1074);
    bool direct = false, cuda = false;
    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--direct")
        direct = true;
      else if (arg == "--cuda")
        cuda = true;
      else
        throw std::invalid_argument("expected --direct or --cuda");
    }
    require(!(direct && cuda), "CPU direct borrowing cannot target CUDA");
    settings();
    engine::InstanceSettings config;
    config.read_environment = false;
    config.cpu_threads = 2;
    config.cuda = cuda;
    engine::Runtime runtime(config);
    Precision(Real::F64, Real::F64, 52).require_backend(runtime);
    require(!cuda || runtime.on_cuda(), "requested CUDA profile ran on CPU");
    std::cout << "actual_backend=" << (runtime.on_cuda() ? "cuda" : "cpu")
              << ",path=" << (direct ? "direct" : "bulk") << '\n';
    boundaries<float, float>(runtime, direct);
    boundaries<double, double>(runtime, direct);
    boundaries<float, double>(runtime, direct);
    boundaries<double, float>(runtime, direct);
    arithmetic<float, float>(runtime, direct);
    arithmetic<double, double>(runtime, direct);
    arithmetic<float, double>(runtime, direct);
    arithmetic<double, float>(runtime, direct);
    std::cout << "precision transport, primitive and arithmetic PASS\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
