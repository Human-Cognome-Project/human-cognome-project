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
    const auto current = field::detail::NativeSet::attach(*root, 128);
    const auto next = field::detail::NativeSet::attach(*root, 128);
    runtime.program().add_snode_tree(std::move(root), false);
    Ndarray counter(&runtime.program(), PrimitiveType::i32, {1});
    Ndarray values(&runtime.program(), PrimitiveType::f32, {128});
    std::vector<float> retained(128);
    for (int i = 0; i < 128; ++i) retained[i] = i + 1;
    engine::upload(values, retained.data(), retained.size() * sizeof(float));
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
    auto count_current = count(current, "native_current_count");
    auto count_next = count(next, "native_next_count");
    const auto visits = [&](engine::CompiledKernel &kernel) {
      auto context = kernel.make_context();
      context.set_arg_ndarray({0}, counter);
      kernel.launch(context);
      runtime.synchronize();
      int n = -1;
      engine::readback(counter, &n, sizeof(n));
      return n;
    };
    check(visits(count_current) == 0 && visits(count_next) == 0,
          "native inactive sets have no calculation-body entries");
    engine::CompiledKernel touch;
    {
      IRBuilder b;
      auto *loop =
          b.create_range_for(b.get_int32(0), b.get_int32(4096), false, 2);
      auto guard = b.get_loop_guard(loop);
      auto *i = b.get_loop_index(loop);
      auto *index = b.create_select(b.create_cmp_lt(i, b.get_int32(2048)),
                                    b.get_int32(3), b.get_int32(64));
      current.activate(b, index);
      touch = engine::CompiledKernel(runtime, "native_duplicate_touch",
                                     b.extract_ir(), {});
    }
    auto touch_context = touch.make_context();
    touch.launch(touch_context);
    check(visits(count_current) == 2 && visits(count_next) == 0,
          "4096 parallel touches collapse to two native active entries");
    engine::CompiledKernel consume;
    {
      IRBuilder b;
      auto *loop = b.create_struct_for(current.cells, false, 2);
      auto guard = b.get_loop_guard(loop);
      auto *i = b.get_loop_index(loop);
      next.activate(b, b.create_add(i, b.get_int32(1)));
      current.deactivate(b, i);
      consume = engine::CompiledKernel(runtime, "native_consume_to_next",
                                       b.extract_ir(), {});
    }
    auto consume_context = consume.make_context();
    consume.launch(consume_context);
    check(visits(count_current) == 0 && visits(count_next) == 2,
          "consumption clears current bits and retains separate next-phase "
          "activation");
    auto second_context = touch.make_context();
    touch.launch(second_context);
    check(visits(count_current) == 2 && visits(count_next) == 2,
          "native reactivation does not consume or overwrite the next phase");
    std::vector<float> received(128);
    engine::readback(values, received.data(), received.size() * sizeof(float));
    check(received == retained && received[3] + received[64] == 69,
          "native scheduling consumption preserves sleeping numerical members "
          "outside control storage");
  } catch (const std::exception &e) {
    harness::record(false, "unexpected exception", e.what());
  }
  std::cout << checks << " native activity checks\n";
  return harness::report("native_activity_test");
}
