// The constructed backend is not the requested one when a CUDA-enabled build
// finds no driver or device: the Taichi LLVM executor rewrites the Program's
// architecture to the host's. Runtime must report the architecture actually
// in use, and a requested CUDA instance must not be taken as proof of CUDA.
//
//   runtime_backend_test --no-driver   driver API disabled (TI_ENABLE_CUDA=0):
//                                      request cuda, expect the CPU fallback
//   runtime_backend_test --no-device   no visible device: request cuda, expect
//                                      a refusal (the driver will not
//                                      initialize) or the CPU fallback
//   runtime_backend_test --device      real device, if any: expect cuda
//
// One mode per process: the engine allows a single instance at a time and
// device visibility must be chosen before CUDA starts.
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include "engine/device.h"
#include "engine/runtime.h"
#include "harness.h"

int main(int argc, char **argv) {
  const std::string mode = argc == 2 ? argv[1] : "";
  if (mode != "--no-driver" && mode != "--no-device" && mode != "--device") {
    std::fprintf(stderr, "usage: runtime_backend_test --no-driver|--no-device|--device\n");
    return 2;
  }
  if (mode == "--no-driver") {
    CHECK_TRUE(setenv("TI_ENABLE_CUDA", "0", 1) == 0, "CUDA driver API disabled");
  } else if (mode == "--no-device") {
    CHECK_TRUE(engine::set_visible_cuda_devices(""), "CUDA visibility set before CUDA starts");
  } else if (!engine::cuda_available() || engine::visible_cuda_devices().empty()) {
    harness::note("no visible CUDA device: real-device case skipped");
    return 0;
  }
  engine::InstanceSettings settings;
  settings.cuda = true;
  settings.read_environment = false;
  try {
    engine::Runtime runtime(settings);
    CHECK_TRUE(runtime.settings().cuda, "the request is still recorded");
    if (mode == "--device") {
      CHECK_TRUE(runtime.on_cuda(), "visible device: on_cuda() true");
      CHECK_TRUE(runtime.arch() == taichi::Arch::cuda, "visible device: actual arch is cuda");
    } else {
      CHECK_TRUE(!runtime.on_cuda(), mode + ": on_cuda() reports the CPU fallback, not the request");
      CHECK_TRUE(runtime.arch() == taichi::Arch::x64, mode + ": actual arch is the host's");
    }
  } catch (const std::runtime_error &error) {
    // Acceptable only where the driver refuses to start at all; never for a usable device.
    CHECK_TRUE(mode == "--no-device", mode + ": construction must not fail");
    harness::note(std::string("no visible device: construction refused: ") + error.what());
  }
  return harness::report("runtime_backend_test");
}
