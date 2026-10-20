#include <iostream>

#include "engine/transfer.h"
#include "field/native_set.h"
#include "harness.h"

namespace {
using namespace taichi::lang;
int checks = 0;
void check(bool ok, const char *label) {
  ++checks;
  harness::record(ok, label, "native activity contract failed");
  if (!ok) throw std::runtime_error(label);
}
}  // namespace

int main(int argc, char **argv) {
  try {
    engine::InstanceSettings settings;
    settings.read_environment = false;
    settings.cpu_threads = 2;
    settings.cuda = argc == 2 && std::string(argv[1]) == "--cuda";
    engine::Runtime runtime(settings);
    auto root = std::make_unique<SNode>(0, SNodeType::root);
    const auto particles = field::detail::NativeSet::attach(*root, 128);
    const auto centroids = field::detail::NativeSet::attach(*root, 1);
    runtime.program().add_snode_tree(std::move(root), false);
    Ndarray counter(&runtime.program(), PrimitiveType::i32, {1});
    Ndarray work(&runtime.program(), PrimitiveType::i32, {2});
    Ndarray values(&runtime.program(), PrimitiveType::f32, {128});
    Ndarray mean(&runtime.program(), PrimitiveType::f32, {1});
    std::vector<float> retained(128);
    for (int i = 0; i < 128; ++i) retained[i] = i + 1;
    engine::upload(values, retained.data(), retained.size() * sizeof(float));
    const auto clear_work = [&] {
      const int zero[2]{};
      engine::upload(work, zero, sizeof(zero));
    };
    clear_work();
    const auto count = [&](const auto &set, const char *name) {
      IRBuilder b;
      auto *out = b.create_ndarray_arg_load({0}, PrimitiveType::i32, 1, 0);
      b.create_global_store(b.create_external_ptr(out, {b.get_int32(0)}),
                            b.get_int32(0));
      auto *loop = b.create_struct_for(set.cells, false, 2);
      {
        auto guard = b.get_loop_guard(loop);
        b.create_atomic_add(b.create_external_ptr(out, {b.get_int32(0)}),
                            b.get_int32(1));
      }
      return engine::CompiledKernel(runtime, name, b.extract_ir(),
                                    {engine::Param::array(PrimitiveType::i32)});
    };
    auto count_particles = count(particles, "native_particle_count");
    auto count_centroids = count(centroids, "native_centroid_count");
    const auto visits = [&](engine::CompiledKernel &kernel) {
      auto ctx = kernel.make_context();
      ctx.set_arg_ndarray({0}, counter);
      kernel.launch(ctx);
      runtime.synchronize();
      int n = -1;
      engine::readback(counter, &n, sizeof(n));
      return n;
    };
    const auto work_counts = [&] {
      runtime.synchronize();
      std::vector<int> result(2);
      engine::readback(work, result.data(), result.size() * sizeof(int));
      return result;
    };
    check(visits(count_particles) == 0 && visits(count_centroids) == 0,
          "inactive type sets have zero calculation-body entries");

    // A deterministic CPU probe of the fork's live leaf-bit test. Production
    // particle calculations must not activate peers while consuming that set.
    if (!settings.cuda) {
      IRBuilder seed;
      particles.activate(seed, seed.get_int32(3));
      engine::CompiledKernel initial(runtime, "native_probe_seed",
                                     seed.extract_ir(), {});
      auto seed_ctx = initial.make_context();
      initial.launch(seed_ctx);
      IRBuilder b;
      auto *out = b.create_ndarray_arg_load({0}, PrimitiveType::i32, 1, 0);
      auto *loop = b.create_struct_for(particles.cells, false, 1, 1);
      {
        auto guard = b.get_loop_guard(loop);
        auto *i = b.get_loop_index(loop);
        b.create_atomic_add(b.create_external_ptr(out, {b.get_int32(0)}),
                            b.get_int32(1));
        particles.deactivate(b, i);
        auto first = b.get_if_guard(
            b.create_if(b.create_cmp_eq(i, b.get_int32(3))), true);
        particles.activate(b, b.get_int32(4));
      }
      engine::CompiledKernel probe(runtime, "native_live_leaf_probe",
                                   b.extract_ir(),
                                   {engine::Param::array(PrimitiveType::i32)});
      auto ctx = probe.make_context();
      ctx.set_arg_ndarray({0}, work);
      probe.launch(ctx);
      check(work_counts()[0] == 2 && visits(count_particles) == 0,
            "CPU native leaf bits are live: a peer activated inside traversal "
            "can enter that same calculation loop");
      clear_work();
    }

    {
      auto dense_root = std::make_unique<SNode>(0, SNodeType::root);
      const auto dense = field::detail::NativeSet::attach(*dense_root, 70);
      runtime.program().add_snode_tree(std::move(dense_root), false);
      IRBuilder b;
      auto *loop =
          b.create_range_for(b.get_int32(0), b.get_int32(70), false, 2);
      {
        auto guard = b.get_loop_guard(loop);
        dense.activate(b, b.get_loop_index(loop));
      }
      engine::CompiledKernel seed(runtime, "native_dense_seed", b.extract_ir(),
                                  {});
      auto ctx = seed.make_context();
      seed.launch(ctx);
      auto count_dense = count(dense, "native_dense_count");
      check(visits(count_dense) == 70,
            "padded native capacity traverses all 70 requested logical slots "
            "without activating padding");
    }
    IRBuilder touch;
    auto *duplicate = touch.create_range_for(touch.get_int32(0),
                                             touch.get_int32(4096), false, 2);
    {
      auto guard = touch.get_loop_guard(duplicate);
      auto *index = touch.create_select(
          touch.create_cmp_lt(touch.get_loop_index(duplicate),
                              touch.get_int32(2048)),
          touch.get_int32(3), touch.get_int32(64));
      particles.activate(touch, index);
    }
    engine::CompiledKernel activate(runtime, "native_duplicate_touch",
                                    touch.extract_ir(), {});
    auto touch_ctx = activate.make_context();
    activate.launch(touch_ctx);
    check(visits(count_particles) == 2 && visits(count_centroids) == 0,
          "4096 parallel native activations across pointer blocks collapse to "
          "two particle entries");

    IRBuilder p;
    auto *keep = p.create_arg_load({0}, PrimitiveType::i32, false, 0);
    auto *data = p.create_ndarray_arg_load({1}, PrimitiveType::f32, 1, 0);
    auto *pwork = p.create_ndarray_arg_load({2}, PrimitiveType::i32, 1, 0);
    auto *ploop = p.create_struct_for(particles.cells, false, 2);
    {
      auto guard = p.get_loop_guard(ploop);
      auto *i = p.get_loop_index(ploop);
      p.create_atomic_add(p.create_external_ptr(pwork, {p.get_int32(0)}),
                          p.get_int32(1));
      auto *ptr = p.create_external_ptr(data, {i});
      p.create_global_store(
          ptr, p.create_add(p.create_global_load(ptr), p.get_float32(1)));
      {
        auto consume = p.get_if_guard(
            p.create_if(p.create_logical_not(p.create_logical_and(
                keep, p.create_cmp_eq(i, p.get_int32(64))))),
            true);
        particles.deactivate(p, i);
      }
      centroids.activate(p, p.get_int32(0));
    }
    engine::CompiledKernel particle_phase(
        runtime, "native_particle_phase", p.extract_ir(),
        {engine::Param::scalar(PrimitiveType::i32),
         engine::Param::array(PrimitiveType::f32),
         engine::Param::array(PrimitiveType::i32)});
    const auto calculate_particles = [&](bool keep_moving) {
      auto ctx = particle_phase.make_context();
      ctx.set_arg_int({0}, keep_moving);
      ctx.set_arg_ndarray({1}, values);
      ctx.set_arg_ndarray({2}, work);
      particle_phase.launch(ctx);
    };
    calculate_particles(true);
    check(work_counts()[0] == 2 && visits(count_particles) == 1 &&
              visits(count_centroids) == 1,
          "particle phase consumes fixed entries, retains carried activity and "
          "collapses centroid touches");

    IRBuilder g;
    auto *emit = g.create_arg_load({0}, PrimitiveType::i32, false, 0);
    auto *members = g.create_ndarray_arg_load({1}, PrimitiveType::f32, 1, 0);
    auto *gwork = g.create_ndarray_arg_load({2}, PrimitiveType::i32, 1, 0);
    auto *result = g.create_ndarray_arg_load({3}, PrimitiveType::f32, 1, 0);
    auto *gloop = g.create_struct_for(centroids.cells, false, 2);
    {
      auto guard = g.get_loop_guard(gloop);
      centroids.deactivate(g, g.get_loop_index(gloop));
      g.create_atomic_add(g.create_external_ptr(gwork, {g.get_int32(1)}),
                          g.get_int32(1));
      auto *a = g.create_global_load(
          g.create_external_ptr(members, {g.get_int32(3)}));
      auto *b = g.create_global_load(
          g.create_external_ptr(members, {g.get_int32(64)}));
      g.create_global_store(
          g.create_external_ptr(result, {g.get_int32(0)}),
          g.create_mul(g.create_add(a, b), g.get_float32(0.5)));
      auto propagate = g.get_if_guard(g.create_if(emit), true);
      for (int index : {3, 3, 5, 64, 96})
        particles.activate(g, g.get_int32(index));
    }
    engine::CompiledKernel centroid_phase(
        runtime, "native_centroid_phase", g.extract_ir(),
        {engine::Param::scalar(PrimitiveType::i32),
         engine::Param::array(PrimitiveType::f32),
         engine::Param::array(PrimitiveType::i32),
         engine::Param::array(PrimitiveType::f32)});
    const auto calculate_centroids = [&](bool propagate) {
      auto ctx = centroid_phase.make_context();
      ctx.set_arg_int({0}, propagate);
      ctx.set_arg_ndarray({1}, values);
      ctx.set_arg_ndarray({2}, work);
      ctx.set_arg_ndarray({3}, mean);
      centroid_phase.launch(ctx);
    };
    calculate_centroids(true);
    check(work_counts() == std::vector<int>({2, 1}) &&
              visits(count_particles) == 4 && visits(count_centroids) == 0,
          "centroid phase consumes its own bit and reactivates particles once "
          "without running their calculation phase");
    float centre = 0;
    engine::readback(mean, &centre, sizeof(centre));
    check(
        centre == 35.5f,
        "centroid placement retains the consumed sleeping member contribution");
    calculate_particles(false);
    check(work_counts() == std::vector<int>({6, 1}) &&
              visits(count_particles) == 0 && visits(count_centroids) == 1,
          "following particle phase calculates each of four selected entries "
          "once and consumes fixed activity");
    calculate_centroids(false);
    check(work_counts() == std::vector<int>({6, 2}) &&
              visits(count_particles) == 0 && visits(count_centroids) == 0,
          "centroid without further effects consumes activity without erasing "
          "particle numerical state");
    calculate_particles(false);
    check(work_counts() == std::vector<int>({6, 2}) &&
              visits(count_centroids) == 0,
          "subsequent inactive particle phase enters no calculation body");
    std::vector<float> received(128);
    engine::readback(values, received.data(), received.size() * sizeof(float));
    retained[3] += 2;
    retained[64] += 2;
    retained[5] += 1;
    retained[96] += 1;
    check(received == retained,
          "type-phase consumption preserves all untouched numerical values");
  } catch (const std::exception &e) {
    harness::record(false, "unexpected exception", e.what());
  }
  std::cout << checks << " native activity checks\n";
  return harness::report("native_activity_test");
}
