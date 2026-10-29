#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include "engine/device.h"
#include "engine/runtime.h"
#include "engine/transfer.h"

using namespace taichi::lang;
namespace {
void require(bool ok, const char *why) {
  if (!ok) throw std::runtime_error(why);
}
std::unique_ptr<Block> arithmetic() {
  IRBuilder b;
  auto *in = b.create_ndarray_arg_load({0}, PrimitiveType::f64, 1, 0);
  auto *out = b.create_ndarray_arg_load({1}, PrimitiveType::f64, 1, 0);
  const auto load = [&](int i) {
    return b.create_global_load(b.create_external_ptr(in, {b.get_int32(i)}));
  };
  const auto store = [&](int i, Stmt *v) {
    b.create_global_store(b.create_external_ptr(out, {b.get_int32(i)}), v);
  };
  store(0, b.create_add(b.create_mul(load(0), load(1)), load(2)));
  store(1, b.create_sub(b.create_add(load(3), load(4)), load(3)));
  store(2, b.create_mul(load(5), load(6)));
  store(3, b.create_sqrt(load(6)));
  return b.extract_ir();
}
}  // namespace
int main(int argc, char **argv) {
  try {
    bool cuda = false, dump = false;
    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--cuda") cuda = true;
      else if (arg == "--dump-code") dump = true;
      else throw std::invalid_argument("expected --cuda [--dump-code]");
    }
    if (cuda) require(engine::cuda_available(), "no visible CUDA device");
    // Intentionally leave the session in fast mode; a per-kernel Strict
    // request must still survive the shared JIT before/after default kernels.
    default_compile_config.fast_math = true;
    default_compile_config.print_kernel_asm = dump;
    default_compile_config.print_kernel_llvm_ir_optimized = dump;
    engine::InstanceSettings settings;
    settings.read_environment = false;
    settings.cuda = cuda;
    settings.device_memory_gb = 0.5;
    engine::Runtime r(settings);
    require(r.arch() == (cuda ? taichi::Arch::cuda : taichi::Arch::x64),
            "backend fallback");
    std::array<double, 7> input{1 + std::ldexp(1., -27),
                                 1 - std::ldexp(1., -27), -1.,
                                 std::ldexp(1., 53), 1., -0., 2.};
    Ndarray in(&r.program(), PrimitiveType::f64, {7});
    Ndarray out(&r.program(), PrimitiveType::f64, {4});
    engine::upload(in, input.data(), sizeof(input));
    int strict_checks = 0;
    for (int round = 0; round < 4; ++round) {
      const bool strict = round % 2;
      engine::CompiledKernel k(
          r, "math_mode_" + std::to_string(round), arithmetic(),
          {engine::Param::array(PrimitiveType::f64),
           engine::Param::array(PrimitiveType::f64)},
          strict ? engine::MathMode::Strict : engine::MathMode::Default);
      auto ctx = k.make_context();
      ctx.set_arg_ndarray({0}, in);
      ctx.set_arg_ndarray({1}, out);
      k.launch(ctx);
      r.synchronize();
      std::array<double, 4> result{};
      engine::readback(out, result.data(), sizeof(result));
      if (strict) {
        require(result[0] == 0., "strict multiply/add was contracted");
        require(result[1] == 0., "strict cancellation was reassociated");
        require(result[2] == 0. && std::signbit(result[2]),
                "strict signed zero was lost");
        require(result[3] == std::sqrt(2.), "strict f64 square root changed");
        strict_checks += 4;
      }
    }
    std::cout << "strict_math,actual_backend=" << (cuda ? "cuda" : "cpu")
              << ",mixed_mode_kernels=4,strict_checks=" << strict_checks
              << " PASS\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  } catch (const std::string &e) {
    std::cerr << e << '\n';
    return 1;
  }
}
