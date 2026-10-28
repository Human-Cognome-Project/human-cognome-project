# Current hierarchy: CPU observations and local CUDA handoff

Status: contribution for review under the
[coherent-dynamics/capture direction](coherent-dynamics-gpu-snapshots.md).
Native runtime/test/capture pin: `85ba956917a413d6a4a7d378a1ce6041be557d35`.
Viewer/camera-test pin: `b3b9ea9470a1ffde1f4174859dccc8cde68cd739`.
Only viewer code/tests/generated assets differ between those pins. Backend tree:
`6784224121f68de1bf1aa0765b54e80d20e355ad`. The evidence/documentation commit
changes no tested source. C3-1 acceptance and C3-2 structural refresh remain
separate; this work does not complete #156 or full #145 qualification.

Executed evidence: **96/96 CPU suites**, all four hierarchy modes and all 32
perturbation/reuse rounds; **180 PostgreSQL integration checks** with unchanged
cold fingerprints; actual CPU backend verification; **17 captures over 128 ticks**
from one current C3 execution; **15 model and 87 browser checks**. CUDA-enabled
fork and HCP clients compiled, and a CPU-only capture on that build passed.
Actual CUDA execution/publication/activity/f64 qualification did not run; C3 CUDA
installation remains unsupported. See the [retained results](../engine/tests/live_hierarchy/README.md).

## Current remote correction before local GPU testing — 2026-10-28

Hosted run [40604795329, native job 122541493905](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/40604795329/job/122541493905)
on `7e03e93c72755cc72757be7dd138bf8b0d080861` passed **92/96 CPU suites in
1225.44 seconds**. All four live-hierarchy modes failed the compiler audit with
`missing force production reader clone/epilogue`. PostgreSQL integration and
the subsequent CPU-backend verification were skipped. Smoke, projection and
offline viewport passed. The 96/96 result below is contributor evidence, not
this hosted result.

The inspected hosted force module contains one child and one parent selector
reader; both use SC RMW and each passed the four-word packet check. The audit
requires exactly three of each. Hosted Clang/LLVM is 15.0.7; contributor evidence
uses 15.0.6. Compiler-generated duplication is a plausible explanation, not yet
proof of complete equivalent execution or a uniquely identified version cause.
This is not a recurrence of the moving-fixture settling failure.

The remote contributor should address this in the existing #195 branch:

1. Retain and compare actual optimized force and poll modules from both build
   configurations, including target/optimization flags. Identify each required
   semantic access path, its loop/control flow and every emitted clone/epilogue.
   Establish whether fewer sites still cover all required child/parent work.
2. Replace compiler-shape-specific exact totals with structural/data-flow checks
   for semantic coverage. Do not merely change three to one, use a whitelist of
   counts, accept any nonzero total or bypass the audit. Attribute every emitted
   selector access to the correct argument and index role; verify SC acquire/
   publication operations and complete XYZ/mass loads from one acquired selector.
   Retain ownership and publication-path checks and reject unknown/plain/demoted
   accesses. Review the poll's fixed clone counts too, rather than waiting for
   the next assertion to expose the same assumption.
3. Preserve and extend negative controls for missing child or parent paths,
   ordinary selector accesses, demoted ordering and broken packet provenance.
   Controls should show that legitimate compiler duplication changes are accepted
   while actual missing or unsafe readers are rejected. If inspection finds a
   runtime omission, fix it explicitly rather than loosening the audit.
4. Run targeted audits and all four native modes in both available configurations,
   then full regression and fresh hosted CI, including the previously skipped
   integration/backend checks. Record exact source/compiler/backend pins and
   keep contributor and hosted evidence separate. Mark any unavailable local
   configuration unexecuted and use hosted evidence for that configuration.
5. Update the handoff/evidence with the correction and final results. Preserve the
   existing failing logs as historical evidence. Keep physical laws, quantization,
   coherent-dynamics criteria and active-stream capture behavior unchanged unless
   a separately explained defect requires a reviewed correction.

Patrick wants remote-capable work completed before starting the local GPU testing
phase. Finish the audit correction and supported CPU/capture/build validation
first; do not move this CPU portability issue onto the GPU session. Actual CUDA
execution, ordering/activity/f64 qualification and hardware-specific adaptation
remain local work. No claim of GPU readiness or C3-1 acceptance follows from
this guidance. C3-2 remains held pending C3-1 acceptance.

## Exact CPU build and bounded capture

Use the viewer pin above for the complete source, including the camera fix.
Native code is identical to the native test pin. The recorded toolchain is
Clang/LLVM 15.0.6, C++17, Release, strict numerical kernels. A normal Linux LLVM 15
installation may instead provide 15.0.7 (the previous hosted run did); retain
that actual version with any new results. Dependencies are CMake, Ninja, Clang/
LLVM 15 development libraries, libX11 and libpq development files (PostgreSQL
headers >=16). Node 24.19.0 and the pinned npm lockfile were used for the viewer.
The capture itself makes no database connection.

From a clean checkout at the viewer pin, initialize the pinned submodules with
`git submodule update --init --recursive`. Then:

```sh
hcp_local_llvm=/usr/lib/llvm-15
cmake -S engine/taichi -B engine/taichi/build-c3-cpu -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER="$hcp_local_llvm/bin/clang" \
  -DCMAKE_CXX_COMPILER="$hcp_local_llvm/bin/clang++" \
  -DLLVM_DIR="$hcp_local_llvm/lib/cmake/llvm" \
  -DCLANG_EXECUTABLE="$hcp_local_llvm/bin/clang" \
  -DTI_WITH_PYTHON=OFF -DTI_WITH_C_API=ON -DTI_WITH_CUDA=OFF \
  -DTI_WITH_AMDGPU=OFF -DTI_WITH_OPENGL=OFF -DTI_WITH_VULKAN=OFF \
  -DTI_WITH_METAL=OFF -DTI_WITH_DX11=OFF -DTI_WITH_DX12=OFF \
  -DTI_WITH_GGUI=OFF -DTI_WITH_BACKTRACE=OFF -DTI_BUILD_TESTS=OFF \
  -DTI_BUILD_EXAMPLES=OFF -DTI_BUILD_RHI_EXAMPLES=OFF
cmake --build engine/taichi/build-c3-cpu --target taichi_c_api -j2
cmake -S engine -B engine/build-c3-cpu -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER="$hcp_local_llvm/bin/clang++" \
  -DENGINE_LLVM_CONFIG="$hcp_local_llvm/bin/llvm-config" \
  -DENGINE_TAICHI_BUILD="$PWD/engine/taichi/build-c3-cpu" \
  -DENGINE_TAICHI_CUDA=OFF -DHCP_WITH_VIEWPORT=ON \
  -DHCP_WITH_WARM_INTEGRATION=ON
cmake --build engine/build-c3-cpu -j2
ctest --test-dir engine/build-c3-cpu --output-on-failure -j2
. engine/build-c3-cpu/engine-env.sh
engine/build-c3-cpu/engine_devices --cpu-only
engine/build-c3-cpu/live_hierarchy_test --backend cpu --threads 1 \
  --case capture --capture-output /tmp/hcp-current-hierarchy \
  --ticks 1024 --every 16
```

The finite run owner reserves/installs one C3 hierarchy and advances that same
execution. The capture interface receives a reference to it; no second Program,
CPU replay, backend switch or candidate solve supplies the snapshot. The supplied
fixture derives English A aggregate and Dutch accented-i UTF-8 byte parts from
existing native normalization/membership/preparation fixtures. It has three
present bodies, two leaf comparisons and their root. Intrinsic test extents and
initial positions are supplied, including the declared common-frame x+2 and a
2^-30 initial-position witness. Body Q8/default field Q6 are coarse stress values;
the root deliberately overrides field precision to Q5. They are not production
calibration. Later ticks retain the original physical equations/velocity.

`--ticks N` is finite (1..1,000,000), `--every K` is a positive capture cadence,
`--threads 1|2` selects the tested worker configurations, and `--direct` selects
the tested CPU-direct source path. The output directory has first/final JSON,
atomic `latest.json`, and a native evidence record. Capture time, requested
readback API bytes, numerical host-copy bytes and export cost are separate from
engine tick time; none is a GPU throughput/pinning claim.

Build/open the existing offline viewer:

```sh
cd tools/viewport
npm ci
npm test
npm run build
```

Open `tools/viewport/index.html` locally and choose the final JSON, or use the
existing read-only folder watcher on the output directory. The viewer shows
actual backend/mode/precision, supplied labels, native masses/radii and sleepers.
It does not render field overlays or make centroids into physical bodies. Native
f64 values survive JSON/import/inspection exactly. Camera/display arithmetic has
ordinary rounding; its test uses a 32-epsilon display-transform bound, which is
never an engine activity or numerical acceptance threshold. The real folder
picker remains a local manual check; cloud automation used the existing in-memory
page path and synthetic read-only watch source without changing browser policy.

## Integration evidence

The revised PostgreSQL-to-active check restores fresh copies of the retained
paired exports after verifying their SHA-256 hashes:

- Core: `f8019624a617a84410c0fd927b4947ed87955b48280016af8fb40b35eb8ac121`.
- Linguistics: `9784ed93d361de61bfb169d13a6e2fd9d2a58eb4dd1ac889b9dc6e0d09e32296`.

The existing `engine/scripts/run_warm_active_acceptance.sh` runs against those
owned cold fixtures and creates a fresh warm test DB. Cold data fingerprints are
compared before/after. Existing user stores are not reset or edited. This is the
legacy PostgreSQL-to-active regression check, not C3 operational admission.
Its [retained output](../engine/tests/live_hierarchy/integration-v2.txt) reports
180 checks, source addressed reads and zero private reads.

## CUDA preparation and the remaining local gate

The remote environment has no visible NVIDIA device/driver. Compiling CUDA-enabled
libraries is distinct from executing/qualifying their native kernels. The
[retained evidence](../engine/tests/live_hierarchy/coherent-v2.json) records what
compiled, what ran on CPU, and what did not run. Cloud-only extracted LLVM packages
needed their missing versioned Z3 alias resolved and the existing shared libpq
selected for linking; neither workaround changes project source or kernel laws.
A normal system development-package installation supplies those linker entries.

For a CUDA-enabled build, use a separate build directory and the same fork flags
above with `-DTI_WITH_CUDA=ON`. Configure the HCP build against that directory with
`-DENGINE_TAICHI_CUDA=ON`. No CUDA toolkit was needed for the observed native fork
build; a supported NVIDIA driver/device is still needed for actual GPU execution.
Do not replace the qualified CPU build while preparing it.

The C3 runner currently rejects `--backend cuda`, and
`LiveHierarchy::install` still rejects an actual non-x64 backend. The legacy
`viewport_letter_capture --backend cuda` is a flat study and does not bypass that
gate. This handoff does **not** provide a ready-to-run C3 GPU calculation.

The next local qualification/adaptation must:

1. Start the requested CUDA Program and verify its actual architecture, rejecting
   CPU fallback before claiming any GPU result.
2. Qualify native selector publication/acquisition, complete four-word packet
   visibility and reuse/history lifetimes on the supported GPU. The CUDA codegen
   delegates integer atomics to the common LLVM SC implementation, but CPU IR or
   successful compilation does not prove emitted GPU scope/fences or visibility.
3. Exercise native activity around same-word/pointer-block boundaries, duplicate
   and late notifications, one visit/publication per poll, held-input 16/8
   convergence and the f64/count/Q path. Preserve force/integration, finite/domain,
   identity, pending-input and resource assertions. Continuing motion is observed
   in declared windows; it is not itself failure.
4. Keep explicit unsupported behavior until a reviewed backend contract supports
   C3 installation. Then capture from that same CUDA execution through the shared
   interface, comparing snapshot values with its synchronized native readback.
   A separate CPU trajectory is not the snapshot authority and need not match it.
5. Retain device/driver/build/source pins, first/final captures and separate
   execution/readback/export measurements. Any hardware-specific correction and
   broader #145 throughput/pinning qualification retain their own review.

Independent CPU/export/viewer preparation is complete only to its recorded scope.
Actual CUDA publication/activity/f64 execution and rendered GPU-stream snapshots
remain local work. C3-2 still waits for C3-1 acceptance.
