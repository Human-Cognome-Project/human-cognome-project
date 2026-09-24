// Does the >2^31 dense-ndarray index cap actually lift, end to end?
//
// A normal small-N test cannot see this: the cap only bites once the
// flattened element offset exceeds 2^31 = 2,147,483,648. This builds one
// 2-axis dense Ndarray {2, N} with N chosen so the high cell's flattened
// offset 1*N + (N-1) = 2N-1 clears that line, writes a sentinel into a LOW
// cell {0,0} and a HIGH cell {1,N-1} through a kernel using the same
// create_ndarray_arg_load(total_dim=2) / create_external_ptr(array,{a,b})
// mechanism field.cpp documents (U2), and reads both back. The kernel never
// iterates the array -- it touches exactly the two cells named -- so this
// is a proof of the index arithmetic (U1's i64-widened codegen), not a bulk
// throughput test.
//
// Element type is u8: at N ~= 1.09e9, shape {2, N} is ~2.03 GiB, well under
// device_memory_gb's preallocated pool and under the GTX 1070's free
// memory, while 2*N still clears 2^31 elements.
#include <cstdint>
#include <string>

#include "engine/runtime.h"
#include "engine/transfer.h"
#include "harness.h"
#include "taichi/platform/cuda/detect_cuda.h"

namespace {

using namespace taichi::lang;
using engine::CompiledKernel;
using engine::InstanceSettings;
using engine::Param;
using engine::Runtime;

// 2 * kN = 2,180,000,000 > 2^31 (2,147,483,648). Each individual index
// (0/1 on axis 0, up to kN-1 on axis 1) stays far inside i32 range: what is
// being tested is the flattened offset the engine computes internally, not
// the arguments the kernel is called with.
constexpr std::int64_t kN = 1'090'000'000;
constexpr std::int64_t kCap = 1LL << 31;

constexpr std::uint8_t kLowSentinel = 0x2A;   // 42
constexpr std::uint8_t kHighSentinel = 0xC7;  // 199

// arr[a, b] = v (cast from the i32 the kernel is launched with). No loop:
// this is one statement writing one cell, the same shape as the top-level
// non-loop store field.cpp's clear kernel uses to zero the universal cell.
CompiledKernel make_write_cell(Runtime &runtime, const std::string &name) {
  IRBuilder b;
  auto *arr = b.create_ndarray_arg_load({0}, PrimitiveType::u8,
                                        /*total_dim=*/2, /*arg_depth=*/0);
  auto *a = b.create_arg_load({1}, PrimitiveType::i32, /*is_ptr=*/false,
                              /*arg_depth=*/0);
  auto *idx = b.create_arg_load({2}, PrimitiveType::i32, /*is_ptr=*/false,
                                /*arg_depth=*/0);
  auto *v = b.create_arg_load({3}, PrimitiveType::i32, /*is_ptr=*/false,
                              /*arg_depth=*/0);
  auto *v8 = b.create_cast(v, PrimitiveType::u8);
  b.create_global_store(b.create_external_ptr(arr, {a, idx}), v8);
  return CompiledKernel(runtime, name, b.extract_ir(),
                        {Param::array(PrimitiveType::u8, /*total_dim=*/2),
                         Param::scalar(PrimitiveType::i32),
                         Param::scalar(PrimitiveType::i32),
                         Param::scalar(PrimitiveType::i32)});
}

// out[0] = arr[a, b]: reads exactly one cell back through a 1-element
// output ndarray, so the host transfer stays tiny regardless of N.
CompiledKernel make_read_cell(Runtime &runtime, const std::string &name) {
  IRBuilder b;
  auto *arr = b.create_ndarray_arg_load({0}, PrimitiveType::u8,
                                        /*total_dim=*/2, /*arg_depth=*/0);
  auto *a = b.create_arg_load({1}, PrimitiveType::i32, /*is_ptr=*/false,
                              /*arg_depth=*/0);
  auto *idx = b.create_arg_load({2}, PrimitiveType::i32, /*is_ptr=*/false,
                                /*arg_depth=*/0);
  auto *out = b.create_ndarray_arg_load({3}, PrimitiveType::u8,
                                        /*total_dim=*/1, /*arg_depth=*/0);
  auto *val = b.create_global_load(b.create_external_ptr(arr, {a, idx}));
  b.create_global_store(b.create_external_ptr(out, {b.get_int32(0)}), val);
  return CompiledKernel(runtime, name, b.extract_ir(),
                        {Param::array(PrimitiveType::u8, /*total_dim=*/2),
                         Param::scalar(PrimitiveType::i32),
                         Param::scalar(PrimitiveType::i32),
                         Param::array(PrimitiveType::u8, /*total_dim=*/1)});
}

std::uint8_t read_cell(CompiledKernel &reader, Ndarray &arr, Ndarray &out,
                       int a, std::int64_t idx) {
  auto ctx = reader.make_context();
  ctx.set_arg_ndarray({0}, arr);
  ctx.set_arg_int({1}, a);
  ctx.set_arg_int({2}, idx);
  ctx.set_arg_ndarray({3}, out);
  reader.launch(ctx);
  std::uint8_t host = 0xFF;
  engine::readback(out, &host, sizeof(host));
  return host;
}

void write_cell(CompiledKernel &writer, Ndarray &arr, int a,
                std::int64_t idx, std::uint8_t v) {
  auto ctx = writer.make_context();
  ctx.set_arg_ndarray({0}, arr);
  ctx.set_arg_int({1}, a);
  ctx.set_arg_int({2}, idx);
  ctx.set_arg_int({3}, v);
  writer.launch(ctx);
}

void check_index_cap(bool cuda) {
  const std::string arch = cuda ? "cuda" : "cpu";
  harness::note(arch + ": index cap, N=" + std::to_string(kN) +
               " (2N=" + std::to_string(2 * kN) + ", 2^31=" +
               std::to_string(kCap) + ")");

  InstanceSettings settings;
  settings.cuda = cuda;
  settings.device_memory_gb = 4.0;
  Runtime runtime(settings);

  // kN must fit an Ndarray axis (an int), and 2*kN must clear 2^31: the
  // whole point of this test.
  CHECK_TRUE(kN <= INT32_MAX, arch + ": N fits an Ndarray axis");
  CHECK_TRUE(2 * kN > kCap, arch + ": 2N exceeds 2^31 (test is meaningful)");

  Ndarray arr(&runtime.program(), PrimitiveType::u8,
             std::vector<int>{2, static_cast<int>(kN)});
  Ndarray out(&runtime.program(), PrimitiveType::u8, std::vector<int>{1});

  CompiledKernel writer = make_write_cell(runtime, arch + "_write_cell");
  CompiledKernel reader = make_read_cell(runtime, arch + "_read_cell");

  const std::int64_t high_idx = kN - 1;
  const std::int64_t high_offset = 1 * kN + high_idx;  // 2N-1
  CHECK_TRUE(high_offset > kCap,
             arch + ": HIGH cell's flattened offset exceeds 2^31");

  // LOW cell first: an in-bounds-under-any-arithmetic control.
  write_cell(writer, arr, /*a=*/0, /*idx=*/0, kLowSentinel);
  runtime.synchronize();
  std::uint8_t low = read_cell(reader, arr, out, /*a=*/0, /*idx=*/0);
  CHECK_TRUE(low == kLowSentinel,
             arch + ": LOW cell {0,0} round-trips (got " +
                 std::to_string(int(low)) + ")");

  // HIGH cell: flattened offset 2N-1 overflows i32 if the codegen still
  // truncates to 32 bits. Without the widening this either lands on the
  // wrong cell or faults; with it, it round-trips exactly.
  write_cell(writer, arr, /*a=*/1, high_idx, kHighSentinel);
  runtime.synchronize();
  std::uint8_t high = read_cell(reader, arr, out, /*a=*/1, high_idx);
  CHECK_TRUE(high == kHighSentinel,
             arch + ": HIGH cell {1,N-1} (offset 2N-1=" +
                 std::to_string(high_offset) +
                 " > 2^31) round-trips (got " + std::to_string(int(high)) +
                 ", expected " + std::to_string(int(kHighSentinel)) + ")");

  // The HIGH write must not have clobbered the LOW cell: guards against a
  // wraparound that happens to alias back into bounds.
  std::uint8_t low_after = read_cell(reader, arr, out, /*a=*/0, /*idx=*/0);
  CHECK_TRUE(low_after == kLowSentinel,
             arch + ": LOW cell unchanged by the HIGH write (got " +
                 std::to_string(int(low_after)) + ")");
}

}  // namespace

int main() {
  check_index_cap(/*cuda=*/false);
  if (taichi::is_cuda_api_available()) {
    check_index_cap(/*cuda=*/true);
  } else {
    harness::note("cuda unavailable on this host: GPU index-cap check skipped");
  }
  return harness::report("index_cap_test");
}
