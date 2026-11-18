# Native initial C3 seed transport

Status: **contributor implementation, awaiting review**, authorized by [#369 review 5955551516](https://github.com/Human-Cognome-Project/human-cognome-project/pull/369#pullrequestreview-5955551516) under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343). Tested source: `98da1f2f8e03b8b3d7c0871cfbcbf26a309396de`, based on integrated main `2c7ec4d00c51f08d0e7e03be618c3663dbab0f0f`. Main's #369 inspection passed smoke and all 12 hosted native jobs, including 185 regression cases. This contribution's local evidence and authoritative hosted gates are separate.

The [accepted inspection](HIERARCHY-SEED-TRANSPORT.md) traced the existing host round trip and demonstrated Taichi's native copy primitive. This runtime change covers **initial C3 installation only**. Refresh's potentially sparse packet slots, reservation policy, validator reuse, capture-buffer resizing and #324 are separate.

## Ownership and publication

`CentroidCascade` still owns its prepared descriptors, values and validity arrays. A private `inspect_descriptors` uses the same `validate_observation` as public `inspect`: valid observation/readiness, result status, query/binding/context identity, source generation and completed ticks, followed by prepared-binding validation. Only the public inspector downloads the complete numerical/validity buffers. The installation-only descriptor borrow is synchronous and supplies topology/coverage metadata; its host numerical vectors are not refreshed or consumed. Nothing is retained across an operation.

The installer performs the existing weighted bind/query and all structural/work checks. At the existing mutation boundary it now:

1. Calls the already used Taichi `Program::fill_ndarray_fast_u32` on the **actual live native bank array**, writing zero over all `8H` f64 entries. This establishes inactive rows even when native destination storage was dirty. No constructor-zero or host-mirror assumption is needed.
2. Calls private `copy_published`, which repeats the shared observation/prepared-binding checks and requires a distinct destination allocation on the same device with f64 shape `[8,H]`. It uses the current owned query's node count, not a caller-supplied prefix. Eight existing `Device::memcpy_internal` slices copy columns 3/4/5/6 (published XYZ/untrimmed mass) for `[0,N)` into both banks. Source padding is never copied.
3. Uploads the original selector/configuration/coverage/response tables, executes the original publication command and completes the original final synchronization. Successful publication binds the hierarchy and invalidates bodies as before.

Fill, copy, later uploads, command execution and final synchronization remain inside the existing catch boundary that makes the Harness unready on an exception. Normal source/structural/work refusals occur before fill/publication and retain a healthy execution. The numerical binder/query kernels, arithmetic, source validation, force/tick kernels, backend eligibility guard and refresh implementation are unchanged. Existing explicit bind/query/final synchronization accounting is unchanged; the native copy primitive is synchronous separately from those explicit counters.

The full public candidate inspector still reads its seven value columns and validity vector with the original result/lifetime checks. The public live inspector still reads current native packets/history. The host seed vector remains reserved for observation; installation no longer stages numerical seed values into it. No object member, array, kernel, layout, reservation or public result field is added, removed or resized. No retained-memory saving is claimed.

## Measured before and after

Historical Clang/GCC binaries are preserved with hashes matching the accepted `c2c553c1` evidence. That source is production-equivalent to main `2c7ec4d0`. Both historical binaries were actually rerun on 1-worker bulk and 2-worker direct for this requested comparison, using the same three-node fixtures. The new probes run both host paths and compilers with the same capacities/precisions and exact one-third centroid control.

| H / Q | Old actual upload | New actual/reported upload | Old actual readback | New actual/reported readback | Removed host-transfer bytes | New native zero writes | Native copy request bytes |
|---|---:|---:|---:|---:|---:|---:|---:|
| 8 / 52 | 67,008 | 66,496 | 728 | 248 | 992 | 512 | 192 |
| 70 / 52 | 76,928 | 72,448 | 4,448 | 248 | 8,680 | 4,480 | 192 |
| 70 / 31 | 76,928 | 72,448 | 4,448 | 248 | 8,680 | 4,480 | 192 |

The removed host traffic is **`60H + 64H = 124H`**: the full candidate inspection readbacks and the host seed upload. The existing **248 reported readback bytes remain** the binder's 64 and query's 184 bytes. The removed `60H` inspection bytes were never included in that result, so they are not subtracted from 248. Upload reporting drops exactly the removed `64H` seed upload and now matches the observed remaining host calls.

Initialization is not free: the new full native zero fill writes **`64H` bytes**. Native copy requests total **`64N` bytes**, including the two banks; those copies also read and write native memory. The test separately records the actual fill invocation/extent and successful private copy invocation/request extent. The latter is a copy-boundary observation, not an instrumented per-slice RHI/DMA trace; the production helper contains the eight direct primitive calls. No latency, physical traffic, throughput or GPU speedup is inferred from these API-request counts.

## Executable controls

The Linux test executable extends the existing linker wrapping with native fill, private copy and final runtime-synchronization boundaries. Production libraries contain no instrumentation or fault switches. A fixed 2 MiB / 256-event journal is allocated before installation; normal successful and refused boundaries retain the linked `new/new[]` guard.

For each fixture:

- Query/inspect the real source before installation, capture its actual ndarray identity, and poison its **inactive numerical and validity rows**. Bind/query during installation recomputes the active prefix while leaving that poison intact. Upload and read back a nonzero sentinel over the actual destination allocation before installation.
- Obtain exact work from an independent reference installation whose test-only copy substitution uses the former full public inspection, host mapping and upload. Refuse one-less work on the dirty destination, prove no fill/copy occurred and no committed state/readiness changed, then succeed at exact work on the same owner. Production work accounting is unchanged.
- Reconcile actual host totals with returned counters and independently account for one full native fill and one private-copy request. Verify both entire native banks bit for bit against public published XYZ/mass, with **zero inactive destination rows** and still-poisoned source padding. Q31 must differ from trial values.
- Reject an obsolete binding/query and mutations of query, binding, context, generation, completed ticks and status through private descriptors, private copy and public inspection: 21 refusals with healthy unchanged installed state. Explicit public inspection still performs exactly `60H` numerical/validity readback bytes.
- Compare numerical state, both banks, selectors and history against the former-host-map reference. Two held body edits (-1 to -2 to -3) drive 16 ordinary ticks through zero and negative centres. Each changed node's previous XYZ/mass must equal its actual preceding selected packet. The installation observation becomes stale after ticking.

Three additional fixtures inject exceptions at private-copy entry **after the real native fill**, after real native copy completion, and after the real final synchronization. All leave the Harness unready and unbound, and a later install returns `Unready`. These are test-injected operation-boundary exceptions, not hardware-fault qualification. No failed execution is repaired or resumed.

## Validation and retained evidence

At the tested source, all three profile result files exit zero:

- **Quick CI:** three native CTests, the 632-sample placement arithmetic witness and placement-capacity control pass.
- **Clang 15:** **16/16 scoped CTests pass in 926.05 seconds**: all four full C3 worker/host-path combinations, four native seed cases, four weighted handoff modes and four public cascade suites. The separate existing 2-worker direct dirty-refresh case also passes.
- **GCC 11:** two native seed probes (1-worker bulk / 2-worker direct), four existing basic/publication cases and **4/4 public cascade CTests** pass.

The six native probes contain **18 successful fixture observations**, each with exact/short retry, 21 provenance refusals, public inspection, both-bank/padding/Q controls and 16 following owner ticks against the former-host-map reference. Three injected failure boundaries pass per probe. Four historical Clang/GCC mode runs also pass. The initial uncommitted development probe passed but is not counted as pinned qualification; its logs are retained. Existing post-PASS cache-lock warnings are preserved, with no cache cleanup or database changes.

Verified machine-local bundle: `/tmp/hcp-native-seed-98da1f2f-evidence.tar.gz`, SHA-256 `46276486e9d59fac061c4d6a062533549afc0aef01593c152f827c368a1b9503`. All final profile and bundle manifests were checked. It includes historical/current binaries and hashes, actual before/after logs, source/equivalence patches, full scoped CTest logs and command scripts. Publishing this hash does not make the raw bundle remotely available.

Reproduction on the matched native build:

```sh
bash engine/scripts/run_local_ci.sh quick /absolute/new/quick-evidence
ctest --test-dir engine/build -j 1 --no-tests=error --output-on-failure \
  -R '^(hierarchy_seed_transport_[12]_(bulk|direct)|live_hierarchy_[12]_(bulk|direct)|cascade_(padded|direct)|weighted_cascade_(padded|direct)|weighted_handoff_[12]_(bulk|direct))$'
engine/build/live_hierarchy_test --threads 2 --direct --case refresh_dirty
```

The retained script contains exact GCC configuration, probe/control commands, clean-source checks, native/compiler pins and binary/runtime hashes. This is scoped local CPU evidence. Full hosted regression is authoritative; no full local regression, full no-unroll matrix, database or CUDA execution is claimed. The existing supported-backend/GTX 1070 qualification eligibility remains unchanged by this patch and is not expanded by CPU passes.
