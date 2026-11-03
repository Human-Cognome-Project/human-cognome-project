#include <cmath>
#include <cstring>
#include <iostream>
#include <random>
#include <stdexcept>
#include "engine/transfer.h"
#include "field/placement_math.h"
#include "llvm/ADT/APInt.h"
#include "llvm/Support/CommandLine.h"

namespace {
using namespace taichi::lang;
using field::detail::placement::Interval;
using field::detail::placement::Math;
using Exact = llvm::APInt;
void require(bool yes, const char *why) {
  if (!yes)
    throw std::runtime_error(why);
}
// All finite binary64 values are exact integers in units of 2^-1074. An
// 8192-bit integer also holds every product used by this independent oracle.
Exact exact(double value) {
  std::uint64_t bits;
  std::memcpy(&bits, &value, 8);
  const auto exponent = (bits >> 52) & 2047;
  require(exponent != 2047, "nonfinite native interval endpoint");
  Exact out(8192, (bits & 0xfffffffffffffULL) | (exponent ? 1ULL << 52 : 0));
  if (exponent)
    out <<= exponent - 1;
  return bits >> 63 ? -out : out;
}
bool le(const Exact &a, const Exact &b) {
  return a.sle(b);
}
void check(double lo, double hi, const Exact &value, int scale = 0) {
  require(std::isfinite(lo) && std::isfinite(hi) && lo <= hi,
          "invalid native interval endpoints");
  require(le(exact(lo).shl(scale), value) && le(value, exact(hi).shl(scale)),
          "native interval does not enclose exact integer arithmetic");
}
void run(int threads) {
  engine::InstanceSettings settings;
  settings.cpu_threads = threads;
  engine::Runtime runtime(settings);
  std::vector<std::array<double, 2>> pairs{
      {0, 0},       {1, 1},
      {1, -1},      {3, 4},
      {9, 0},       {1, 0x1p-53},
      {1, 0x1p-52}, {0x1.fffffffffffffp0, 0x1.0000000000001p0}};
  std::mt19937_64 random(199201);
  for (int exponent : {-380, -128, -1, 0, 27, 128, 380}) {
    const double x = std::ldexp(1., exponent);
    for (double a :
         {x, std::nextafter(x, 0.), std::nextafter(x, INFINITY), x * 1.5})
      for (double c :
           {x, -x, std::nextafter(-x, 0.), std::ldexp(1., -exponent)})
        pairs.push_back({a, c});
  }
  for (int k = 0; k < 512; ++k) {
    const auto number = [&] {
      auto word = (random() & 0x800fffffffffffffULL) |
                  (std::uint64_t(1023 + int(random() % 761) - 380) << 52);
      double x;
      std::memcpy(&x, &word, 8);
      return x;
    };
    pairs.push_back({number(), number()});
  }
  const int n = pairs.size();
  std::vector<double> input(2 * n), output(15 * n);
  for (int k = 0; k < n; ++k) {
    input[k] = pairs[k][0];
    input[n + k] = pairs[k][1];
  }
  Ndarray in(&runtime.program(), PrimitiveType::f64, {2, n}),
      out(&runtime.program(), PrimitiveType::f64, {15, n});
  IRBuilder b;
  auto *source = b.create_ndarray_arg_load({0}, PrimitiveType::f64, 2, 0),
       *destination = b.create_ndarray_arg_load({1}, PrimitiveType::f64, 2, 0);
  auto *loop =
      b.create_range_for(b.get_int32(0), b.get_int32(n), false, threads);
  {
    auto guard = b.get_loop_guard(loop);
    auto *row = b.get_loop_index(loop);
    auto *status = b.create_local_var(PrimitiveType::i32);
    b.create_local_store(status, b.get_int32(0));
    Math m(b, status);
    const auto load = [&](int k) {
      return b.create_global_load(
          b.create_external_ptr(source, {b.get_int32(k), row}));
    };
    auto *a = load(0), *c = load(1);
    const auto put = [&](int k, Interval x) {
      b.create_global_store(
          b.create_external_ptr(destination, {b.get_int32(k), row}), x.lo);
      b.create_global_store(
          b.create_external_ptr(destination, {b.get_int32(k + 1), row}), x.hi);
    };
    put(0, m.add(m.point(a), m.point(c)));
    put(2, m.sub(m.point(a), m.point(c)));
    put(4, m.mul(m.point(a), m.point(c)));
    auto *denominator =
        b.create_select(b.create_cmp_eq(c, m.f(0)), m.f(1), b.create_abs(c));
    put(6, m.divide_positive(m.point(b.create_abs(a)), m.point(denominator)));
    put(8, m.root(m.point(b.create_abs(a))));
    const auto norm = m.norm2({m.point(a), m.point(c), m.point(m.f(0))});
    put(10, norm);
    put(12, m.root(norm));
    b.create_global_store(
        b.create_external_ptr(destination, {b.get_int32(14), row}),
        b.create_cast(b.create_local_load(status), PrimitiveType::f64));
  }
  engine::CompiledKernel kernel(
      runtime, "placement_interval_qualification", b.extract_ir(),
      {engine::Param::array(PrimitiveType::f64, 2),
       engine::Param::array(PrimitiveType::f64, 2)},
      engine::MathMode::Strict, engine::Optimization::Local);
  engine::upload(in, input.data(), input.size() * 8);
  auto call = kernel.invocation(true);
  auto &ctx = call.get();
  ctx.set_arg_ndarray({0}, in);
  ctx.set_arg_ndarray({1}, out);
  kernel.launch(ctx);
  runtime.synchronize();
  engine::readback(out, output.data(), output.size() * 8);
  for (int k = 0; k < n; ++k) {
    const auto value = [&](int field) { return output[field * n + k]; };
    if (value(14) != 0) {
      std::cerr << "interval_domain,row=" << k << ",a=" << std::hexfloat
                << pairs[k][0] << ",b=" << pairs[k][1] << '\n';
    }
    require(value(14) == 0, "supported interval sample rejected");
    const auto a = exact(pairs[k][0]), c = exact(pairs[k][1]);
    check(value(0), value(1), a + c);
    check(value(2), value(3), a - c);
    check(value(4), value(5), a * c, 1074);
    const auto aa = exact(std::abs(pairs[k][0])),
               cc = exact(pairs[k][1] == 0 ? 1 : std::abs(pairs[k][1]));
    require(le(exact(value(6)) * cc, aa.shl(1074)) &&
                le(aa.shl(1074), exact(value(7)) * cc),
            "division enclosure");
    require(value(8) >= 0 &&
                le(exact(value(8)) * exact(value(8)), aa.shl(1074)) &&
                le(aa.shl(1074), exact(value(9)) * exact(value(9))),
            "sqrt enclosure");
    const auto norm = a * a + c * c;
    check(value(10), value(11), norm, 1074);
    require(value(12) >= 0 && le(exact(value(12)) * exact(value(12)), norm) &&
                le(norm, exact(value(13)) * exact(value(13))),
            "norm enclosure");
  }
  require(output[10 * n + 3] == 25 && output[11 * n + 3] == 25 &&
              output[12 * n + 3] == 5 && output[13 * n + 3] == 5,
          "exact tangency was widened");
  bool demoted = false;
  try {
    check(output[5], output[5], exact(pairs[5][0]) + exact(pairs[5][1]));
  } catch (const std::runtime_error &) {
    demoted = true;
  }
  require(demoted,
          "independent exact oracle accepted a demoted outward endpoint");
  std::cout
      << "placement_interval_math,samples=" << n << ",workers=" << threads
      << ",oracle=independent_8192bit_integer,operations=add_sub_mul_div_sqrt_"
         "norm,exact_tangency=1,inward_endpoint_control=rejected PASS\n";
}
}  // namespace
int main(int argc, char **argv) {
  try {
    int threads = 1;
    bool no_unroll = false;
    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--threads" && i + 1 < argc)
        threads = std::stoi(argv[++i]);
      else if (arg == "--compiler-no-unroll")
        no_unroll = true;
      else
        throw std::runtime_error("unknown argument");
    }
    if (no_unroll) {
      const char *options[] = {"placement-compiler-control", "-unroll-count=1",
                               "-unroll-max-count=1",
                               "-unroll-full-max-count=1"};
      llvm::cl::ParseCommandLineOptions(4, options);
    }
    std::cout << "placement_math_compiler,llvm_unroll="
              << (no_unroll ? "disabled-test-control" : "default") << '\n';
    run(threads);
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "placement math: " << e.what() << '\n';
    return 1;
  }
}
