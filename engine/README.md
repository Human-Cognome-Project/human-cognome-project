# Engine

HCP's engine path is native C++.

It links directly against the project's modified Taichi fork rather than using
Taichi's Python frontend. The fork provides the mass-particle execution
machinery and portable backend compiler/device layer; the HCP C++ workspace
supplies the concrete field formulas, state, lifecycle and control structure.

The modified fork is expected at:

```text
engine/taichi/
```

## Layout

| Path | Role |
|---|---|
| `src/engine/` | Thin native wrapper around the Taichi runtime: instance lifetime, kernel compile/launch, devices, transfers. |
| `src/field/` | Native C++ field/tick implementation. |
| `src/assembly/` | Fixed-study host preparation, identity/scale mapping and publication. |
| `apps/` | Device/runtime inspection utilities. |
| `tests/` | Native smoke, field, preparation, assembly and >2^31 dense-index regressions. |
| `cmake/` | Locates the matching Taichi source/build/runtime set. |
| `scripts/` | Native runtime environment helpers. |
| `docs/` | Recovered design, vetting, configuration and measurement record. |
| `taichi/` | Modified Taichi fork (curated separately from the HCP wrapper). |

See [ARCHITECTURE.md](ARCHITECTURE.md) before changing boundaries.

## Python boundary

Taichi upstream ships a primarily Python-facing abstraction layer for arbitrary
models and rich data. HCP deliberately does not use that as its engine surface:
our formulas and working structures are simple enough to be expressed directly
in C++.

Python in the Taichi fork is vendor frontend/build/test material. Project
Python is authorized only for bulk database population during bootstrap,
including preparation and validation of those batches. It must not become the
physics engine, runtime harness, database kernel, WAL kernel, or kernel-network
implementation.

The previous planner-generated Python field engine has been archived under
`archive/2026-09-planner-field-python/`.

## Building

The native wrapper expects the modified fork source and a matching Taichi build
tree. The external dependencies are normal Git submodules rooted at
`engine/taichi/external/`.

After cloning:

```sh
git submodule update --init --recursive
```

Build the fork first. The HCP model is backend-independent: Taichi compiles the
same native model for the hardware enabled in its build.

The reproducibility baseline is CPU-only because it is universally available
in CI and proves the source/build/runtime path without requiring accelerator
hardware:

```sh
cmake -S engine/taichi -B engine/taichi/build-review -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_C_COMPILER=/usr/lib/llvm-15/bin/clang \
      -DCMAKE_CXX_COMPILER=/usr/lib/llvm-15/bin/clang++ \
      -DLLVM_DIR=/usr/lib/llvm-15/lib/cmake/llvm \
      -DCLANG_EXECUTABLE=/usr/lib/llvm-15/bin/clang \
      -DTI_WITH_PYTHON=OFF -DTI_WITH_C_API=ON \
      -DTI_WITH_CUDA=OFF -DTI_WITH_AMDGPU=OFF \
      -DTI_WITH_OPENGL=OFF -DTI_WITH_VULKAN=OFF \
      -DTI_WITH_METAL=OFF -DTI_WITH_GGUI=OFF \
      -DTI_BUILD_TESTS=OFF -DTI_BUILD_EXAMPLES=OFF
cmake --build engine/taichi/build-review --target taichi_c_api -j 2
```

A development-machine CUDA build enables `-DTI_WITH_CUDA=ON` in a separate
Taichi build tree. The HCP wrapper reads that build's CUDA setting;
`ENGINE_TAICHI_CUDA=AUTO` is the default, with `ON` and `OFF` available as
consistency checks. The native wrapper currently selects LLVM/x64 or CUDA;
other Taichi backends require HCP linkage and functional qualification.

Taichi generates native LLVM runtime bitcode from source. The HCP wrapper stages
the artifacts required by the backends present in the matched build into its
own build directory for `TI_LIB_DIR`; it does not depend on Taichi's Python
package staging path.

Then build the HCP wrapper:

```sh
cmake -S engine -B engine/build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_COMPILER=/usr/lib/llvm-15/bin/clang++
cmake --build engine/build -j 4
ctest --test-dir engine/build --output-on-failure
. ./engine/build/engine-env.sh
./engine/build/engine_devices
```

A different fork/build can still be supplied with
`ENGINE_TAICHI_ROOT` / `ENGINE_TAICHI_BUILD`.

### Local CUDA build

Observed 2026-10-20 (see below) on a GTX 1070 (compute capability 6.1), NVIDIA
driver 535.309.01, Ubuntu 24.04, LLVM/clang 15. **No CUDA toolkit or `nvcc` is
installed or needed:** Taichi's CUDA backend emits PTX through LLVM NVPTX and
talks to the driver's `libcuda`.

1. Fetch the pinned submodules as CI does. If GitLab is unavailable, the pinned
   Eigen commit is also on a GitHub mirror, and Git still checks out the exact
   recorded commit:

   ```sh
   git submodule update --init --recursive || \
   git -c 'url.https://github.com/eigen-mirror/eigen.git.insteadOf=https://gitlab.com/libeigen/eigen.git' \
       submodule update --init --recursive
   ```
2. Configure the fork in a **separate** build tree (never reuse the CPU tree),
   with the CPU recipe above but `-DTI_WITH_CUDA=ON`, and the extra OFF flags
   the CI workflow uses: `-DTI_WITH_DX11=OFF -DTI_WITH_DX12=OFF
   -DTI_WITH_BACKTRACE=OFF -DTI_BUILD_RHI_EXAMPLES=OFF`. Build target
   `taichi_c_api`, for example in `engine/taichi/build-cuda`.
3. Configure the HCP wrapper in its own tree against that build:

   ```sh
   cmake -S engine -B engine/build-cuda -G Ninja -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_CXX_COMPILER=/usr/lib/llvm-15/bin/clang++ \
         -DENGINE_LLVM_CONFIG=/usr/lib/llvm-15/bin/llvm-config \
         -DENGINE_TAICHI_BUILD=engine/taichi/build-cuda -DENGINE_TAICHI_CUDA=ON
   ```

   Add `-DHCP_WITH_WARM_INTEGRATION=ON` and/or `-DHCP_WITH_VIEWPORT=ON` when
   those clients are wanted.

Observed 2026-10-20 (evidence: [CUDA-LOCAL-VALIDATION.md](docs/CUDA-LOCAL-VALIDATION.md)):
`engine_devices` binds device 0 (the GTX 1070) and requests arch `cuda` (the
printed arch is the request, not the actual Program backend; see the evidence
note), and `engine_smoke_test`, `field_test`, and
`native_activity_test --cuda`, `activity_assembly_test --cuda` and
`active_transfer_experiment --cuda` pass. These tests likewise request CUDA
through `settings.cuda` (`runtime.cpp:40`) and do not assert the actual Program
backend; no backend check was recorded for them, and no transfer timing or
pinning figures are recorded or implied. This is bounded local functional evidence for the
existing fixture path; it does not validate engine-owned insertion (#156, not
built), and it is not performance or pinning qualification (#145 remains open).

**Target selection.** The fork JIT-compiles kernels at run time for the bound
device: `taichi/rhi/cuda/cuda_context.cpp` reads the device's compute
capability and sets the LLVM NVPTX target `sm_<cc>`, capped at `sm_86`, so the
target is derived at run time from the bound device. Patrick (2026-10-20)
expects this build to suit cards in the 1070 class (Pascal, compute capability
6.x); this is untested beyond the 1070. Only the GTX 1070 (cc 6.1) was
exercised; other cards, including older ones such as cc 5.0 (the runtime
bitcode file loaded at `llvm_context.cpp:581` is named
`cuda_runtime-cuda-nvptx64-nvidia-cuda-sm_60.bc`), are untested. Cards above
`sm_86` receive `sm_86` code through driver forward compatibility; a more
directed build (a raised cap and/or newer LLVM) might benefit newer cards, but
that is unqualified. The engine binds CUDA device 0; use `CUDA_VISIBLE_DEVICES`
(or `engine_devices --visible`) to select another card.

### Running built binaries

Built binaries are Linux x86_64 and assume an Ubuntu 24.04-class glibc. They
link dynamically to `libLLVM-15.so.1` (package `libllvm15t64` on Ubuntu 24.04,
which `llvm-15` pulls in), `libstdc++` and ICU 74. Only binaries built with
`-DHCP_WITH_WARM_INTEGRATION=ON` (the PostgreSQL clients, such as
`warm_active_integration` and the viewport capture tool) also link
`libpq.so.5`. They need `TI_LIB_DIR` pointing at the staged runtime bitcode
directory; `. <build>/engine-env.sh` sets it to `<build>/taichi-runtime`.
CUDA runs additionally need an NVIDIA driver providing `libcuda.so.1` (tested
with 535.309.01). CPU builds need no GPU.

See [docs/BUILD-VETTING.md](docs/BUILD-VETTING.md) for the CPU/GPU/load
qualification ladder.

## Current status

The [native precision prerequisites](docs/NATIVE-PRECISION.md) implement #176's
first #172 slice, accepted in #178: double packed transport, an experimental significand
clearing helper and a [live state/conversion audit](docs/PRECISION-STATE-AUDIT.md).
The existing field kernels remain at their original profile; #172/#177 retain
the later numerical and LoD-centroid conversion.

The accepted [#180 plan](../docs/upward-centroid-integration-plan.md) now has a
[candidate-only upward operation](docs/CENTROID-CASCADE.md) accepted in #182.
It reduces current native values through an explicit bounded hierarchy in f64,
with optional experimental trimming at expressed comparisons. Whole/disjoint
cohorts are first-component restrictions; live field/force/phase conversion
remains unbuilt.

The next [weighted accounting plan](../docs/weighted-centroid-accounting-plan.md)
is accepted in #183. Its [Segment A implementation](docs/SOURCE-COUNTS.md) carries
existing exact source counts through prepared commits and is accepted in #184.
[Segment B](docs/WEIGHTED-CENTROID-CASCADE.md) now supplies fractional/compatible
overlapping candidates for review in its own PR. Live arithmetic and phase
changes retain their separate contract.

See [AGENTS current state](../AGENTS.md#current-state) for accepted, active and
held work. [Prepared execution](docs/PREPARED-EXECUTION.md) records the bounded
backing/binding implementation; the [coherent-update plan](../docs/prepared-coherent-update-plan.md)
was accepted in #167 and its [bounded implementation](docs/PREPARED-UPDATES.md)
was accepted in #169 under #162; [object-support results](docs/OBJECT-SUPPORT-RESULTS.md)
record the earlier slice. Production placement/live insertion and GPU
allocation/transfer qualification retain their own acceptance work.

The [remaining core operation plan](../docs/core-operation-readiness-plan.md)
is accepted in #170 after #169. It covers the structural-to-live
lifecycle and complete CPU operation matrix before full local GPU testing.
Its first [structural recipe segment](../kernels/database/warm/STRUCTURAL-RECIPES.md)
is accepted and merged in #171. The first portion of Segment 2 supplies
[native geometry preflight](docs/PREPARED-GEOMETRY.md) for review. It reads current
occupancy/focus and prepares candidate contacts; placement, contact installation,
transactional ticks and the enduring operational lifecycle remain unbuilt.

Historical machine-specific measurements and development decisions remain under
`docs/`; read their dates and scope rather than treating each as a current guarantee.
