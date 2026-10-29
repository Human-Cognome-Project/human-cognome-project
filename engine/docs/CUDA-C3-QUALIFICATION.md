# Local CUDA corrections and remaining work

Status: **corrections proposed for review**, from main
`a05ae246762204de2055eeb556996d142c2a55fc`. Local tests use GTX 1070 GPU0
(Pascal 6.1), driver 535.309.01 and Clang/LLVM 15.0.7. GPU1 is excluded.
[Retained evidence](../tests/cuda_local/README.md) identifies the tested source.
This is not the local-study build or Docker release PR.

## What failed and what changed

The numerical engine already uses native Taichi Ndarrays, SNode activity and
compiled traversal/kernels. C++ owns the harness, mappings and observation
boundary. These findings do not justify replacing Taichi with a separate
scheduler or physics implementation. Backend independence requires the fork's
CUDA lowering to preserve the same required semantics.

1. **Publication ordering.** LLVM 15 NVPTX emitted plain `atom.or.b32` /
   `atom.xor.b32` for the sequentially consistent selector exchanges on sm_61.
   Generic LLVM fences could not be lowered on this target. The fork now brackets
   integral bitwise atomics with `llvm.nvvm.membar.gl` and keeps their per-thread
   exchanges rather than reducing them across a warp. Arithmetic reductions keep
   their existing path. LLVM and actual PTX audits check the ordering; deleting
   either barrier must fail. This has a possible cost and is not a speed claim
   or proof for every CUDA architecture.
2. **Strict math.** The shared CUDA JIT used the session math setting and enabled
   unsafe arithmetic under FTZ even for strict kernels. Each compiled kernel now
   carries its math setting to target options and function attributes. Mixed
   default/strict kernel tests check multiply/add separation, cancellation,
   signed zero and f64 square root. Existing FTZ policy is unchanged.
3. **Recurring host allocation.** Repeated CUDA function lookup formatted a log
   message and allocated during otherwise prepared execution. Cache function
   handles per JIT module, with a mutex protecting lookup/insertion. Initial
   lookup is preparation; subsequent launches reuse the cached handle. Existing
   no-recurring-new and fixed-backing checks remain active.
4. **Unpublished motion.** A moving hierarchy produced a finite subnormal
   post-brake velocity while its published position stayed unchanged; the
   normal-or-zero guard withdrew readiness. Patrick authorized the separate
   [motion-model correction](ACCEPTED-MOTION.md): retain accepted displacement/dt
   for positive dt and incoming velocity for zero dt. A quiet positive-duration
   publication retains zero motion. Transient integrated velocity may underflow;
   NaN/infinity and unsupported retained state still fail. This deliberately
   discards residual momentum and changes future trajectories. It is not merely
   a backend repair or an epsilon-based activity rule.
5. **Explicit qualification access and honest observation.** Normal builds still
   reject C3 CUDA installation. `HCP_WITH_CUDA_QUALIFICATION=ON` admits only the
   tested GTX 1070/6.1 device. Native test runners require the requested actual
   architecture, account for backend-specific scratch, reject CPU-direct CUDA
   requests and export the same active execution. Browser evidence reports the
   capture's backend instead of hard-coding CPU. The compile option is a bounded
   test path, not general GPU admission.

The [PTX 6.3 barrier specification](https://docs.nvidia.com/cuda/archive/10.0/parallel-thread-execution/index.html#parallel-synchronization-and-communication-instructions-membar)
is the device-ordering reference. The compiler inspection is complemented by
on-device packet, notification, numerical and reuse checks.

## Executed local checks

- CPU regression after the motion change: **100/100**, 2398.13 seconds, on the
  CUDA-enabled substrate executing the CPU backend. This is not a new CPU-only
  substrate build or hosted acceptance run.
- CUDA CTest group: **4/4**, 423.86 seconds: native handoff, weighted handoff,
  strict math and accepted motion. The weighted probe covers 18 graphs and 76
  matrix cases; the native probe covers 48 cases and repeated publication reuse.
- Actual C3 CUDA cases: basic, forces, singleton, cumulative crossing and
  cancellation, tree, publication, moving, permuted moving, retained letters
  and capture all pass. Moving studies retain all 33 observation/recovery rounds
  and the arithmetic, history, pending-input and allocation assertions.
- Native capture: 64 GPU ticks, 9 captures, exact f64 JSON round-trip and unchanged
  observed state. The browser passes **89 checks** on the resulting three-body
  capture, using an offline `file://` page. Automated watcher checks use a
  substitute directory handle; the human folder-picker interaction is not proven.
- Native NAS warm-to-active integration: **180 checks**. All ten cold-table counts
  and aggregate row fingerprints remain unchanged. The retained core/Linguistics
  pair already matched the October 13 exports; no cold-data update was needed.

Earlier ordering/PTX diagnostics and the original tiny-velocity failure are
retained as historical evidence, not relabelled as final-source tests. Changes
remain proposed until review. Ordinary vectors are not pinned; these results
are not complete allocation/transfer qualification under #145.

## Core issues still to address

### 1. Add items to a running hierarchy without resetting survivors

`Harness::update_prepared` currently rejects every nonempty edit while the live
hierarchy is installed, including insertion, retirement, contact and count edits.
This explicit guard is the immediate obstacle to Patrick's interactive study.
Removing it alone would permit incoherent source/topology and publication state.

Implement the accepted [C3-2 combined refresh](../../docs/production-hierarchy-response-plan.md#4-initialization-structural-changes-and-failure)
contract: validate prospective source and hierarchy together, preserve surviving
positions/motion, full identities, comparison history and pending work, then
publish coherently at the owner boundary. Rejection must preserve the healthy
old execution. This correction PR does not implement that operation.

### 2. Operational admission and a small live-study control surface

C3-2's labelled fixture starts are not production placement. The accepted
[#170 readiness plan](../../docs/core-operation-readiness-plan.md) also requires
nearest-unoccupied admission at focus, candidate motion/contact validation and
owner lifetime. A retained-letter Add command must use those native facilities;
it must not rebuild the whole study, reset survivors, invent recipe positions
or introduce analyst behavior. Existing fixed fixtures do not supply this
interactive run owner.

### 3. A rendered flow that can actually be inspected

The viewer already watches atomic `latest.json` replacements and preserves the
camera/selection between updates. The C3 capture runner finishes its tiny fixture
in milliseconds, so a browser polling every 100–1000 ms can miss its evolution.
Its tick-based `--every` option does not provide display pacing or attach-before-
start control. The older legacy capture runner has `--pace-ms`, but runs a
separate legacy study and cannot substitute for the active C3 execution.

The local-study build must expose regular completed-state captures from the
same running GPU study, allow the person to attach before motion starts and
observe after adding items, and offer wall-clock pacing independently of dt.
It must distinguish real motion from a renderer animation or CPU replay. A final
snapshot and passing browser import tests do not meet this visual milestone.
No new pacing, control UI or insertion implementation is included here.

### 4. Unresolved CUDA precision boundary

The separate precision probe passes 7,776 f32-to-f32 and 17,172 f64-to-f64
boundary cases, then fails f32-to-f64 promotion: input `0x1p-149` becomes zero
instead of `0x1p-149`. This FTZ/subnormal conversion issue remains open. The
current f64 C3 path passing does not qualify mixed precision. Preserve the
failure and resolve the conversion semantics before claiming backend parity
for that path; the motion change does not fix it.

### 5. Separate build/release work

Docker recipes, images, final snapshot filing and a loadable interactive study
belong in a separate PR after the core issues are addressed. Development and
visual tests use the native local instance with NAS PostgreSQL; Docker is the
release state. Keep transient work in RAM where practical to limit wear on the
old disks, and budget memory to avoid swapping. No credentials or NAS endpoints
belong in the source or evidence. C3-2, #156 and #145 remain open.
