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

The [leaf-reference investigation](docs/LEAF-REFERENCE-INSPECTION.md) records the original hosted failure, successful retry and a constructed quantization-boundary case. [Failure-only diagnostics](docs/LEAF-FAILURE-DIAGNOSTICS.md) now capture completed operands at the unchanged assertion; the native witness checks their output, original rejection and silent success behavior.

The [live editable CPU study](docs/LIVE-EDITABLE-STUDY.md) adds an operator CLI
through the prepared session/owner: add a declared-template occurrence, tick,
remove it and tick the same study. Commands, a captured run, identity/survivor
controls and scoped local qualification are included for review. Whole-field LoD
exchange follows separately.

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

The [refactor exit reconciliation](docs/RESPONSIBILITY-MAP.md#refactor-exit-reconciliation--2026-11-23)
maps the integrated ownership/build boundaries and native-reuse decisions to
#343's exit criteria, with source-pinned evidence and separate capability limits.
Its completion disposition is for review; #324 mathematics remains separate.

The active workbench keeps logical `ActiveAssembly` implementation in
`src/assembly/active_workbench.cpp` and native `ActiveSession` lifetime/reporting
implementation in `src/assembly/active_session.cpp`. Logical callers use the
value-only `assembly/active_assembly.h` and standalone `active_assembly` target;
`assembly/active_workbench.h` remains the compatibility umbrella. Native callers
retain the `active_workbench` target and explicit study/field dependencies. See
the [boundary and qualification](docs/ACTIVE-WORKBENCH.md#logical-header-and-standalone-target--2026-11-23).

The [native nested-instance inspection](docs/NATIVE-INSTANCE-INSPECTION.md)
compares compiled layouts, indexed runtime cells and HCP occurrences, including
catalogue revision boundaries, native recycling and accounting limits. Its
focused witness is an inspection fixture, not a production object-model migration.
The [prepared-submission lifetime inspection](docs/PREPARED-SUBMISSION-LIFETIMES.md)
traces the real session refusal/retry path. It distinguishes retired source/record
roots from Program-retained activity/selection trees and compiled data, preserves
a warmed sibling and proposes component-local native tree ownership. Production
and achieved saving remain unchanged by that inspection. Its [authorized tree
ownership correction](docs/NATIVE-TRAVERSAL-TREE-OWNERSHIP.md) retains native handles
in Activity/selection, drains before dependent teardown and retires after it,
including constructor unwind. Actual host/root and pool/list costs stay separate.

The [legacy submission lifetime inspection](docs/LEGACY-SUBMISSION-LIFETIMES.md)
then traces real v1 parameter failures and successful Execution teardown. Source
roots remain live while Activity roots retire. Its minimal proposal retains the
source handle in the existing Execution with FixedStudy-before-source teardown;
production lifetime and runtime saving remain unchanged by the inspection.

Its [authorized source-tree ownership correction](docs/SESSION-SOURCE-TREE-OWNERSHIP.md)
captures native handles on both submit paths and orders drain, dependent study/
owner/Harness destruction and source retirement in the existing Execution. Actual
root retirement, 8-byte host-object overhead and scoped local evidence are
recorded separately from retained compiler data and broader resource accounting.

The [compiler/JIT ownership inspection](docs/COMPILER-JIT-LIFETIMES.md) then
separates HCP definitions, cached IR/data, launcher rows and JIT/ORC ownership.
Real legacy/prepared refusals retain different registered footprints; cached
data clears during finalization while kernel wrappers survive until Runtime
member destruction. Production eviction and byte accounting are unchanged.

Its [authorized native count APIs](docs/NATIVE-COMPILER-JIT-COUNTS.md) observe
existing compiler/session owners without lazy creation, disk loading or module
registration. Pending/loaded objects and runtime/kernel modules remain distinct;
inspection budgets and explicit incompleteness prevent unsupported byte or
reclamation claims. Fresh, partially initialized, loaded and finalization cases
have source-pinned native CPU evidence, including a standalone test without
warm integration or viewport.

The [reclamation design/inspection](docs/NATIVE-RECLAMATION-DESIGN.md) then
checks actual shared compiled pointers/handles, retained rows and surviving
native task symbols/dispatch. It proposes borrower protection before any cache
removal and distinguishes CPU ORC resources from module wrappers. Production
reclamation and byte/admission policy are unchanged; backend and teardown limits
are explicit.

The [native borrower prerequisite](docs/NATIVE-COMPILED-BORROWS.md) tracks
HCP x64 compiled-data guards through shared acquisition, moves and unwind.
Raw marking follows the returned record's actual architecture; tracked acquisition
checks that actual record before issuing a guard. Context/Kernel destruction
precedes guard release; guards validate data access but retain no Program/LLVM context.
Actual metadata overhead and the required matching native rebuild are recorded;
reclamation, launch-row reuse and byte/admission policy remain separate.

The [serialized unregistered-record removal](docs/NATIVE-UNREGISTERED-REMOVAL.md)
adds a bounded operation in the existing native manager for positively tracked,
zero-borrow pending LLVM/x64 records. Unknown/raw/live clients, disk data,
incomplete inspection and any registration attempt refuse unchanged.
Exact destruction, fresh retry, partial-registration refusal and surviving
dispatch have scoped local evidence. Native record overhead and released C++
requests are separate from complete B, automatic policy, registered/JIT/ORC
retirement and admission.

The [registration-failure inspection and combined correction](docs/NATIVE-REGISTRATION-FAILURES.md)
preserve the original source/ABI pins. Its reviewed next slice,
[native CPU registration attempts](docs/NATIVE-REGISTRATION-ATTEMPTS.md), records
progress beside existing slots and shares lifetime-safe metadata identity without
acquiring a client borrow. Missing rows/identity and earlier JIT side effects
stay explicitly incomplete. Focused failure/retry/dispatch/shutdown evidence and
actual launcher/slot/longer-control-lifetime costs are recorded separately from
registered retirement, ORC cleanup and admission.

The [CPU JIT creation inspection](docs/CPU-JIT-CREATION-OWNERSHIP.md),
[checked-name implementation](docs/CPU-JIT-NAME-ISSUANCE.md) and
[attribution inspection](docs/CPU-JIT-ATTRIBUTION.md) preserve their original
sources and evidence. The [native ID association](docs/CPU-JIT-ID-ASSOCIATION.md)
implements #413's reviewed direction: retain the issued ID in the existing CPU
row and query presence through the live session with its own prepaid bound.
Actual row extent grows 80→88 bytes / capacity eight 640→704. Seven histories
preserve no-ID/failure/retry/finalization and survivor dispatch. Local checks pass;
#414 passed hosted qualification and is integrated at `c5770a2e`. Cleanup,
registered retirement and admission credit remain outside this scope.

The [real submission/JIT disposition](docs/REFUSED-SUBMISSION-JIT-TRACE.md)
connects that integrated API to real owner refusal, retry, surviving
dispatch and finalization. It reconciles rows, record identities and present
dylibs without production changes. The [private request-preflight extraction](docs/PREPARED-REQUEST-PREFLIGHT.md)
implements the approved binding/layout/frame/scratch split, preserving exact
check order, status/demand and native ownership. Full native bytes remain an
admission-accounting limit.
New review and authoritative CI remain separate from the local evidence.

The [prepared source-layout inspection](docs/PREPARED-SOURCE-LAYOUT.md) records
the approved keep-local decision and its binding, registration and lifetime
comparison. Its original source and unqualified materialization-failure edge
remain recorded separately.

The [private source-layout helper](docs/PREPARED-SOURCE-LAYOUT-HELPER.md)
implements that approved extraction. It constructs the unchanged unregistered
graph and fills existing binding pointers; the session keeps native registration
and the candidate's existing guard. Source reconstruction and CPU before/after
controls qualify the bounded move. #418 source-accepts the helper; authoritative
native CPU CI remains pending after smoke success.

The [responsibility reconciliation](docs/RESPONSIBILITY-MAP.md#current-prepared-path-reconciliation--2026-11-23)
finds a sufficient prepared transaction boundary. #419 source-accepts that
disposition and authorizes the logical caller's header/geometry separation
implemented above. Its original source-pinned dependency probes remain historical.
The current handoff records the existing #417 hosted compiler failure; new local
qualification does not clear that failure or pending authoritative CI.

The [exchange parent ownership inspection](docs/EXCHANGE-PARENT-OWNERSHIP.md)
measures declaration copies against the owner's existing pinned recipe, with
ordered provenance, detached public values and explicit lifetime controls. The
[private recipe-backed implementation](docs/EXCHANGE-RECIPE-PARENTS.md) is
integrated through #384 at main `6dcf3fef`; the inspection preserves its original
source measurements. The [membership target-pin inspection](docs/MEMBERSHIP-TARGET-PINS.md)
now measures repeated complete targets and their catalogue/source lifetime.
The [owned membership target table](docs/MEMBERSHIP-TARGET-TABLE.md) implements
the accepted catalogue-local reuse while preserving independent rows, full
detached values and native publication. Its actual costs and tradeoffs are scoped
to the recorded source and local qualification.

The [projected definition-pin inspection](docs/PROJECTION-DEFINITION-PINS.md)
measures repeated full definitions against each projection's already owned
immutable recipe. It records detached public row costs/lifetime and proposes a
bounded representation/output review; production and runtime saving are unchanged.
Its [approved recipe-index implementation](docs/PROJECTION-DEFINITION-INDICES.md)
uses the projection's existing immutable payload and explicit count/index owning
exports. Contextual internal readers, complete provenance, actual replacement
costs and the CPU qualification scope are recorded there.
The [row-study identity inspection](docs/PROJECTION-STUDY-IDENTITY.md) measures
repeated UUID backing against the existing projection header and preserves full
key/provenance and public export costs. #391 accepted the bounded header/reader
proposal; its [checked contextual implementation](docs/PROJECTION-HEADER-IDENTITY.md)
records actual retained/peak/work changes while public exports remain complete.
The [byte-owner key inspection](docs/PROJECTION-BYTE-OWNER-KEYS.md) now measures
remaining complete owner keys against that projection and its aggregate rows,
with actual consumer refusals and a checked association proposal. Production
remains unchanged by the inspection. Its [approved owner-index implementation](docs/PROJECTION-BYTE-OWNER-INDICES.md)
records actual narrower rows, complete detached output, consumer guards and
retained/peak/work changes while preserving native execution authority.
The [parent-contribution study inspection](docs/PROJECTION-PARENT-STUDY-IDENTITY.md)
attributes the remaining repeated study strings in active/spare parent pools,
checks detached values and publication, and proposes using the projection header
for private rows while preserving complete public output. It changes no runtime behavior.
Its [approved header implementation](docs/PROJECTION-PARENT-HEADER.md) records
actual narrower pools, complete indexed active exports, refusal/lifetime controls
and measured whole-owner storage/work costs. The inactive pool stays private.
The [parent field-target inspection](docs/PROJECTION-PARENT-TARGETS.md) measures
remaining scoped-object copies against the existing recipe, with checked source/
parent selectors and explicit guard-work overhead. Runtime remains unchanged
at that inspection source. Its [authorized selector implementation](docs/PROJECTION-PARENT-TARGET-INDICES.md)
records actual 72-byte rows, complete owning output, pool-role handling during
repartitioning, refusal/retry and whole-owner storage/work costs.

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
      -DTI_WITH_DX11=OFF -DTI_WITH_DX12=OFF -DTI_WITH_BACKTRACE=OFF \
      -DTI_BUILD_TESTS=OFF -DTI_BUILD_EXAMPLES=OFF -DTI_BUILD_RHI_EXAMPLES=OFF
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

For repeatable local CPU checks with source pins, retained logs and provisional
review reports, use the [local CI profiles](docs/LOCAL-CI.md). These include a
separate local PostgreSQL fixture flow and do not replace hosted acceptance.

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

Start a new instance with the [onboarding/spec brief](../docs/onboarding/AGENT-START.md).
The accepted CPU baseline is through #241 as checked 2026-11-09, including private
exchange ownership, nested-map reuse, scoped retained-capacity/lifetime evidence
and hierarchy-sealing/capture/command/staging preparation gates, plus adoption
input-snapshot admission and selected buffer/map-outer/shared-object/table allocation limits. Native run 42515939810
and smoke passed; see [acceptance evidence](tests/admission/README.md#maintainer-integrated-acceptance--241-2026-11-09).
Remaining nested-key and other constructor allocation/setup limits and
combined resources remain open before remaining-operation integration and controls.
GPU qualification remains separate. Earlier stage
descriptions below retain their bounded acceptance scope, not a second work queue.

The [native precision prerequisites](docs/NATIVE-PRECISION.md) implement #176's
first #172 slice, accepted in #178: double packed transport, an experimental significand
clearing helper and a [live state/conversion audit](docs/PRECISION-STATE-AUDIT.md).
The default field profile retains its original arithmetic. The bounded C1
[live trial profile](docs/LIVE-NUMERICAL-PROFILE.md) is accepted in #187;
#172/#177 retain live hierarchy and inherited-response integration.

The accepted [#180 plan](../docs/upward-centroid-integration-plan.md) now has a
[candidate-only upward operation](docs/CENTROID-CASCADE.md) accepted in #182.
It reduces current native values through an explicit bounded hierarchy in f64,
with optional experimental trimming at expressed comparisons. Whole/disjoint
cohorts are first-component restrictions; live hierarchical propagation remains
separate from this optional query.

The next [weighted accounting plan](../docs/weighted-centroid-accounting-plan.md)
is accepted in #183. Its [Segment A implementation](docs/SOURCE-COUNTS.md) carries
existing exact source counts through prepared commits and is accepted in #184.
[Segment B](docs/WEIGHTED-CENTROID-CASCADE.md) now supplies fractional/compatible
overlapping candidates accepted in #185, with all eight hosted checks and 74
native CPU suites passing. The [live numerical profile plan](../docs/live-numerical-profile-plan.md)
is accepted in #186. Its [C1 implementation](docs/LIVE-NUMERICAL-PROFILE.md)
and [CPU evidence](tests/numerical_profile/README.md) are accepted in #187: one selected
f64 live state, count-based weights, explicit positional publication, preserved
physical/history state and native activity. Its provisional sub-grid activity
measurements remain historical; the C2b-1 correction below is accepted in #192.
Live hierarchy and #156 operations remain open.

The next [live hierarchy handoff plan](../docs/live-hierarchy-handoff-plan.md)
is accepted in #188. Its release is limited to a native same-type investigation,
collision and publication probe. Its [diagnostic implementation](docs/NATIVE-HANDOFF.md)
and [CPU evidence](tests/native_handoff/README.md) are accepted in #189; all eight
hosted checks passed, including 84 CPU suites. The accepted #191
[cumulative publication/activity plan](../docs/cumulative-publication-integration-plan.md)
follows #190 with an existing-profile activity/speed-cache correction and separate
weighted numerical handoff probe. The [C2b-1 implementation](docs/CUMULATIVE-PUBLICATION.md)
and [CPU evidence](tests/cumulative_publication/README.md) are accepted in #192.
The [C2b-2 weighted native probe](docs/WEIGHTED-NATIVE-HANDOFF.md) and
[CPU evidence](tests/weighted_handoff/README.md) are accepted in #193. Comparison
ownership, inherited response and coupled settling/recovery now have the accepted
[#194 production integration plan](../docs/production-hierarchy-response-plan.md).
C3-1 fixed-tree implementation is accepted in #195; C3-2 refresh is accepted in
#198. CPU contact-safe ticks are accepted in #201; placement/admission follow the
[current continuation plan](../docs/core-local-study-continuation-plan.md).
The [C3-1 fixed-tree implementation](docs/LIVE-HIERARCHY.md) and
[CPU investigation](tests/live_hierarchy/README.md) are accepted under the
[coherent-dynamics direction](../docs/coherent-dynamics-gpu-snapshots.md).
Moving state is observed separately from invariant failure. The bounded
[active-capture handoff](../docs/c3-cpu-capture-handoff.md) reuses the current
execution. #196's bounded local GTX 1070 corrections/evidence are accepted;
full CUDA qualification remains local and mixed-precision conversion still has
a retained failure. The [C3-2 combined refresh implementation](docs/LIVE-REFRESH.md)
is accepted in #198 with [source-pinned CPU evidence](tests/live_hierarchy/refresh-v1/README.md).
Final hosted validation passed 101/101 suites in 3355.27 seconds, all extra
no-unroll hierarchy/refresh modes, 180 integration checks and actual CPU-only
backend verification; all nine checks passed. Earlier failed/cancelled runs remain
separately retained. #201 supplies CPU contact-safe ticks; native admission follows.
Operational admission/owners and CUDA refresh qualification remain separate.

The accepted [#199 plan](../docs/native-admission-contact-transaction-plan.md)
starts B1 with native selection qualification. #200's
[workspace and shared C3 builders](docs/NATIVE-SELECTION.md) are accepted in #200 with [source-pinned CPU evidence](tests/native_selection/v1/README.md).
Local validation at `ada217a` passes 105 CPU suites, eight selection modes and
180 integration checks. Hosted validation separately passed 105/105 CPU suites in
3576.79 seconds, the additional compiler-mode tests, 180 integration checks and
CPU-only backend verification; all eight checks passed.
This first gate preserves committed selection through abandon/retry and native
same-poll full centroid work. #201 now implements the remaining
[contact-safe candidate ticks](docs/CONTACT-SAFE-TICKS.md) accepted for CPU execution:
prepared numerical/control/contact staging, typed unchanged rejection and one
publication through `tick_prepared`. [Local CPU qualification](tests/contact_ticks/v1/README.md)
passes 109 suites, all eight B1 modes and 180 integration checks; hosted validation independently passed 109 suites in 4746.01 seconds, all
extra modes, 180 integration checks and backend verification (all eight checks). Fixture inputs
remain private; certified placement, operational admission and owners are separate.

The opt-in [B2 certified placement candidate](docs/CERTIFIED-PLACEMENT.md) is
accepted in #203 under #199/#201. It reserves native interval-search storage, reports a
global lower/feasible upper distance certificate and checks actual published
coordinates. The candidate query claims no occurrence; accepted private publication and
remaining D7 coordinator scope are documented in [native admission](docs/NATIVE-ADMISSION.md).

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
[native geometry preflight](docs/PREPARED-GEOMETRY.md), accepted in #175. It reads current
occupancy/focus and prepares candidate contacts. #201 adds transactional ticks and
accepted CPU physical contact installation; placement and the enduring operational
lifecycle remain unbuilt.

Historical machine-specific measurements and development decisions remain under
`docs/`; read their dates and scope rather than treating each as a current guarantee.

