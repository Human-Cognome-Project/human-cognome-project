// Reports the devices this host offers, which one the engine binds, and what
// an engine instance costs on it. Setup and measurement only: it computes
// nothing.
//
//   . ./build/engine-env.sh
//   ./build/engine_devices
//   ./build/engine_devices --visible 1 --device-memory-gb 0.25
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "engine/device.h"
#include "engine/runtime.h"

namespace {

double to_gib(std::size_t bytes) {
  return static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);
}

[[noreturn]] void usage(int code) {
  std::printf(
      "usage: engine_devices [--visible LIST] [--cpu-only]\n"
      "                      [--snode-capacity N] [--tree-capacity N]\n"
      "                      [--device-memory-gb F] [--cells N]\n"
      "                      [--trees N] [--leaves N] [--tree-cells N]\n"
      "                      [--arith-cells N] [--arith-rounds N]\n"
      "  --visible selects which physical devices the process sees, before\n"
      "  CUDA starts; the engine then binds visible ordinal 0.\n");
  std::exit(code);
}

const char *value_of(int argc, char **argv, int &i) {
  if (i + 1 >= argc) {
    std::fprintf(stderr, "%s needs a value\n", argv[i]);
    usage(2);
  }
  return argv[++i];
}

// Warm launch time of a trivial multiply-add loop per cell, at both field
// precisions. It measures the arithmetic the card is willing to do, nothing
// about any model.
// One-time costs and the repeated cost, kept apart: a fixed pool is paid for
// once at instantiation, and a tick pays only the launch.
struct ArithTiming {
  double tree_ms{0.0};    // allocation and structure compilation, once
  double first_ms{0.0};   // first launch, which compiles the kernel, once
  double warm_ms{0.0};    // every launch after that
};

ArithTiming time_arithmetic(engine::Runtime &runtime,
                            taichi::lang::DataType dt,
                            int cells,
                            int rounds) {
  ArithTiming timing;
  using namespace taichi::lang;
  auto root = std::make_unique<SNode>(/*depth=*/0, SNodeType::root);
  SNode &cell = root->dense(Axis(0), cells);
  SNode &leaf = cell.insert_children(SNodeType::place);
  leaf.dt = dt;
  SNode *field = &leaf;
  {
    const auto start = std::chrono::steady_clock::now();
    runtime.program().add_snode_tree(std::move(root), /*compile_only=*/false);
    runtime.synchronize();
    timing.tree_ms = std::chrono::duration<double, std::milli>(
                         std::chrono::steady_clock::now() - start)
                         .count();
  }

  const bool is_double = dt->is_primitive(PrimitiveTypeID::f64);
  IRBuilder b;
  auto constant = [&](double value) -> Stmt * {
    return is_double ? static_cast<Stmt *>(b.get_float64(value))
                     : static_cast<Stmt *>(b.get_float32(
                           static_cast<float>(value)));
  };
  auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(cells), false,
                                  runtime.cpu_threads());
  {
    auto guard = b.get_loop_guard(loop);
    auto *i = b.get_loop_index(loop);
    auto *accumulator = b.create_local_var(dt);
    // Seeded from the cell index, so the arithmetic cannot be folded away at
    // compile time and the measurement is of real work.
    b.create_local_store(accumulator, b.create_cast(i, dt));
    auto *inner = b.create_range_for(b.get_int32(0), b.get_int32(rounds));
    {
      auto inner_guard = b.get_loop_guard(inner);
      auto *k = b.get_loop_index(inner);
      // Each round depends on the round number as well as on the running
      // value, so nothing can be hoisted out of the loop.
      b.create_local_store(
          accumulator,
          b.create_add(b.create_mul(b.create_local_load(accumulator),
                                    constant(1.0000001)),
                       b.create_cast(k, dt)));
    }
    b.create_global_store(b.create_global_ptr(field, {i}),
                          b.create_local_load(accumulator));
  }
  const std::string name =
      std::string("arith_") + (is_double ? "f64" : "f32");
  engine::CompiledKernel kernel(runtime, name, b.extract_ir(), {});

  auto launch_once = [&] {
    auto ctx = kernel.make_context();
    kernel.launch(ctx);
    runtime.synchronize();
  };
  {
    const auto start = std::chrono::steady_clock::now();
    launch_once();
    timing.first_ms = std::chrono::duration<double, std::milli>(
                          std::chrono::steady_clock::now() - start)
                          .count();
  }
  const auto start = std::chrono::steady_clock::now();
  constexpr int kRepeats = 5;
  for (int r = 0; r < kRepeats; ++r) {
    launch_once();
  }
  timing.warm_ms = std::chrono::duration<double, std::milli>(
                       std::chrono::steady_clock::now() - start)
                       .count() /
                   kRepeats;
  return timing;
}

}  // namespace

int main(int argc, char **argv) {
  std::string visible;
  bool cpu_only = false;
  int cells = 0;
  int trees = 0;
  int leaves = 4;
  int tree_cells = 8;
  int arith_cells = 0;
  int arith_rounds = 64;
  engine::InstanceSettings settings;

  for (int i = 1; i < argc; ++i) {
    const char *arg = argv[i];
    if (!std::strcmp(arg, "--visible")) {
      visible = value_of(argc, argv, i);
    } else if (!std::strcmp(arg, "--arith-cells")) {
      arith_cells = std::atoi(value_of(argc, argv, i));
    } else if (!std::strcmp(arg, "--arith-rounds")) {
      arith_rounds = std::atoi(value_of(argc, argv, i));
    } else if (!std::strcmp(arg, "--trees")) {
      trees = std::atoi(value_of(argc, argv, i));
    } else if (!std::strcmp(arg, "--leaves")) {
      leaves = std::atoi(value_of(argc, argv, i));
    } else if (!std::strcmp(arg, "--tree-cells")) {
      tree_cells = std::atoi(value_of(argc, argv, i));
    } else if (!std::strcmp(arg, "--cells")) {
      cells = std::atoi(value_of(argc, argv, i));
    } else if (!std::strcmp(arg, "--cpu-only")) {
      cpu_only = true;
    } else if (!std::strcmp(arg, "--snode-capacity")) {
      settings.snode_capacity = std::atoi(value_of(argc, argv, i));
    } else if (!std::strcmp(arg, "--tree-capacity")) {
      settings.snode_tree_capacity = std::atoi(value_of(argc, argv, i));
    } else if (!std::strcmp(arg, "--device-memory-gb")) {
      settings.device_memory_gb = std::atof(value_of(argc, argv, i));
    } else if (!std::strcmp(arg, "--help")) {
      usage(0);
    } else {
      std::fprintf(stderr, "unknown option %s\n", arg);
      usage(2);
    }
  }

  // Visibility must be chosen before anything touches CUDA.
  if (!visible.empty() && !engine::set_visible_cuda_devices(visible)) {
    std::fprintf(stderr, "CUDA already initialized; --visible came too late\n");
    return 1;
  }

  const engine::HostInfo host = engine::host_info();
  std::printf("host: %d cpu threads, %.1f GiB memory\n", host.cpu_threads,
              to_gib(host.total_memory_bytes));

  if (!engine::cuda_available()) {
    std::printf("cuda: unavailable on this host\n");
  } else {
    for (const engine::CudaDevice &device : engine::visible_cuda_devices()) {
      std::printf("cuda device %d: %s, compute capability %d.%d%s\n",
                  device.visible_index, device.name.c_str(),
                  device.compute_capability_major,
                  device.compute_capability_minor,
                  device.bound ? "  <- the engine binds this one" : "");
    }
    const engine::CudaDevice bound = engine::bound_cuda_device();
    std::printf(
        "bound: %s, %.2f GiB total, %.2f GiB free, memory pool path %s\n",
        bound.name.c_str(), to_gib(bound.total_bytes), to_gib(bound.free_bytes),
        bound.memory_pool_path ? "on" : "off");
  }

  // What one instance costs. On CUDA the difference in free device memory is
  // the runtime's preallocation, which follows from the capacities and the
  // memory budget.
  const bool use_cuda = engine::cuda_available() && !cpu_only;
  settings.cuda = use_cuda;
  const std::size_t before = use_cuda ? engine::cuda_free_memory() : 0;
  {
    engine::Runtime runtime(settings);
    const auto &config = runtime.program().compile_config();
    std::printf(
        "instance: arch %s, snode capacity %d, tree capacity %d, "
        "device memory budget %.2f GiB, %d launch threads\n",
        use_cuda ? "cuda" : "x64", config.llvm_snode_capacity,
        config.llvm_snode_tree_capacity, config.device_memory_GB,
        runtime.cpu_threads());
    if (use_cuda) {
      const std::size_t after = engine::cuda_free_memory();
      std::printf("instance device memory: %.3f GiB reserved at startup\n",
                  to_gib(before > after ? before - after : 0));
    }
    // One dense array of i32, to measure what resident rows cost.
    if (cells > 0) {
      const std::size_t before_field = use_cuda ? engine::cuda_free_memory() : 0;
      auto root = std::make_unique<taichi::lang::SNode>(
          /*depth=*/0, taichi::lang::SNodeType::root);
      taichi::lang::SNode &cell = root->dense(taichi::lang::Axis(0), cells);
      taichi::lang::SNode &leaf =
          cell.insert_children(taichi::lang::SNodeType::place);
      leaf.dt = taichi::lang::PrimitiveType::i32;
      const auto build_start = std::chrono::steady_clock::now();
      runtime.program().add_snode_tree(std::move(root), /*compile_only=*/false);
      runtime.synchronize();
      std::printf("field build: %.1f ms for %d cells\n",
                  std::chrono::duration<double, std::milli>(
                      std::chrono::steady_clock::now() - build_start)
                      .count(),
                  cells);
      if (use_cuda) {
        const std::size_t after_field = engine::cuda_free_memory();
        std::printf(
            "field of %d i32 cells: %.3f GiB reserved, %.1f bytes per cell\n",
            cells, to_gib(before_field > after_field ? before_field - after_field : 0),
            static_cast<double>(before_field > after_field ? before_field - after_field : 0) /
                static_cast<double>(cells));
      }
    }
    // Many independent trees at once, to exercise the configured capacities
    // rather than the defaults.
    if (trees > 0) {
      const std::size_t before_trees = use_cuda ? engine::cuda_free_memory() : 0;
      const int ids_before = runtime.issued_snode_ids();
      const auto start = std::chrono::steady_clock::now();
      for (int t = 0; t < trees; ++t) {
        auto root = std::make_unique<taichi::lang::SNode>(
            /*depth=*/0, taichi::lang::SNodeType::root);
        taichi::lang::SNode &cell =
            root->dense(taichi::lang::Axis(0), tree_cells);
        for (int l = 0; l < leaves; ++l) {
          taichi::lang::SNode &leaf =
              cell.insert_children(taichi::lang::SNodeType::place);
          leaf.dt = taichi::lang::PrimitiveType::i32;
        }
        runtime.program().add_snode_tree(std::move(root),
                                         /*compile_only=*/false);
      }
      runtime.synchronize();
      const double seconds =
          std::chrono::duration<double>(std::chrono::steady_clock::now() - start)
              .count();
      std::printf(
          "%d trees of %d leaves: %d snode identifiers, %d live trees, %.2f s\n",
          trees, leaves, runtime.issued_snode_ids() - ids_before,
          runtime.program().get_snode_tree_size(), seconds);
      if (use_cuda) {
        const std::size_t after_trees = engine::cuda_free_memory();
        std::printf("those trees: %.3f GiB reserved\n",
                    to_gib(before_trees > after_trees ? before_trees - after_trees
                                                      : 0));
      }
    }
    if (arith_cells > 0) {
      const ArithTiming f32 = time_arithmetic(
          runtime, taichi::lang::PrimitiveType::f32, arith_cells, arith_rounds);
      const ArithTiming f64 = time_arithmetic(
          runtime, taichi::lang::PrimitiveType::f64, arith_cells, arith_rounds);
      std::printf("arithmetic %d cells x %d rounds\n", arith_cells,
                  arith_rounds);
      std::printf(
          "  once  : field build f32 %.1f ms, f64 %.1f ms; "
          "first launch with compilation f32 %.1f ms, f64 %.1f ms\n",
          f32.tree_ms, f64.tree_ms, f32.first_ms, f64.first_ms);
      std::printf(
          "  access: warm launch f32 %.3f ms, f64 %.3f ms, f64 costs %.1fx\n",
          f32.warm_ms, f64.warm_ms,
          f32.warm_ms > 0.0 ? f64.warm_ms / f32.warm_ms : 0.0);
    }
    std::printf("snode identifiers issued in this process: %d\n",
                runtime.issued_snode_ids());
  }
  if (use_cuda) {
    std::printf("after release: %.2f GiB free\n",
                to_gib(engine::cuda_free_memory()));
  }
  return 0;
}
