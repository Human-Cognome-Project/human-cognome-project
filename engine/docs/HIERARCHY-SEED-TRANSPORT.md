# Live hierarchy seed transport inspection

Status: **inspection accepted in #369**, under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343) and the [refactoring plan](../../docs/engine-refactoring-plan.md). Tested source: `c2c553c14bfe242080152ecb4c5ac3b73f319eb5`, based on main `22273dd25862f2ac4fe6f5e9af408e5906efa39b`. Main integrates reviewed #366–#368 with their hosted checks passed. This slice changes tests/build registration only; `engine/src`, the Taichi fork, common code, kernels and workflows are byte-identical to that main.

Accepted at `473807bb` by [review 5955551516](https://github.com/Human-Cognome-Project/human-cognome-project/pull/369#pullrequestreview-5955551516). The separately reviewed [initial C3 runtime follow-up](NATIVE-HIERARCHY-SEED-TRANSPORT.md) implements the authorized replacement. The measurements and proposal below remain the source-pinned inspection record.

The maintainer's [#368 review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/368#pullrequestreview-5949686979) requests one remaining duplicated responsibility, its owner/lifetime, existing native support and measured actual cost before a runtime proposal. This inspection selects **transport of initial published centroid packets into C3's two live packet banks**. It changes no runtime transport, public inspection, accounting, arithmetic or allocation policy. Validator reuse and capture-buffer resizing remain separate.

## Actual path and owners

The owning behavior is [C3 installation](LIVE-HIERARCHY.md#boundary-and-identity), using the [weighted candidate contract](WEIGHTED-CENTROID-CASCADE.md). This is a one-time explicit candidate solve, not ordinary tick propagation or C3 refresh.

| Operation / owner | Responsibility and lifetime |
|---|---|
| `LiveHierarchy::install` in `src/field/live_hierarchy.cpp` | Requires ready wide CPU execution and current generation. Calls the private weighted binder and candidate query, then validates the complete rooted installation and reserves no new metadata at this boundary. |
| `CentroidCascade::bind_weighted` / `query` in `src/field/centroid_cascade.cpp` | Preserve full source identities, ordinals, counts/denominators, exact child coverage, native source checks, finite-value flags, work bounds and Q. The cascade owns the native `values_array` (seven f64 columns) and `valid_array` until its Harness is destroyed. Query/result currentness is narrower than allocation lifetime. |
| `CentroidCascade::inspect` | Checks readiness, valid result/query/binding/context, generation and completed ticks; validates prepared bindings; downloads the full reserved values and validity buffers. Host descriptors are already owned metadata. Inspection borrows expire at the next synchronized operation; a reference does not extend them. |
| Installer host preparation | Reads those descriptors to validate/install native topology and response tables. Reads numerical columns 3/4/5/6 (published XYZ and untrimmed mass), copying each logical node to both four-column host seed banks. It does not consume the downloaded validity vector. |
| Live hierarchy publication | Uploads all `8H` host seed doubles, plus its other prepared tables; command kernel and synchronized completion install the hierarchy and invalidate bodies. Native slots belong to this live owner. The host slot vector is also reused by explicit live inspection, which refreshes it from native storage. |

All structural/work checks precede committed publication. A normal validation rejection preserves flat execution or an already installed tree; a backend failure after mutation makes execution unready. Neither a native SNode layout nor a byte copy replaces HCP composition semantics or source validation. No source-array pointer or borrowed descriptor may become a retained cross-operation view.

## Actual transfer measurement

The Linux GNU/Clang `live_hierarchy_test` alone uses GNU linker `--wrap` for the existing `engine::upload` and `engine::readback` symbols. Production libraries have no instrumentation. A fixed journal, allocated before the measured boundary, records the actual calling-thread calls, ndarray/host identities, extents and copied call-time payloads. Its 256-event / 2 MiB bound fails closed. The normal linked `new/new[]` guard remains enabled during the real installer; the journal adds no allocation there. A separate known-payload control checks direction, bytes and enabled/disabled recording. The normal wrapper path forwards directly when recording is disabled.

Three fixtures use the same three-node, two-body weighted tree with capacities `H=8` and `H=70`, Q52 and Q31. Exact positions -1/+1 and masses 1/2 produce a one-third root centre whose trial and Q31 publication differ. All installed source/coverage validations still run. Both source host paths use these actual native centroid and live packet transfers.

| Reserved H / field Q | Reported and observed upload bytes | Reported readback | Observed readback | Extra inspection readback | Included seed upload | Active native scratch-copy bytes |
|---|---:|---:|---:|---:|---:|---:|
| 8 / 52 | 67,008 | 248 | 728 | 480 | 512 | 192 |
| 70 / 52 | 76,928 | 248 | 4,448 | 4,200 | 4,480 | 192 |
| 70 / 31 | 76,928 | 248 | 4,448 | 4,200 | 4,480 | 192 |

Each real install records 26 transfer calls. The observed upload total equals `LiveHierarchyResult::upload_bytes`. Its 248 reported readback bytes are the binder's 64 and query's 184 status/summary bytes. `inspect` adds **`7H*8 + H*4 = 60H` bytes**, outside that result's reported readback total. This is a concrete installation reporting gap; it is not a defect in the public candidate contract, which already documents additional explicit inspection bytes. The subsequent **`8H*8 = 64H` seed upload** is included in reported upload.

These are API-request byte totals on actual LLVM/x64, not physical DMA, page traffic, latency or throughput measurements. Reservation remains fixed and host vectors remain allocated. The combined `124H` host-transfer bytes identify a replacement opportunity (992 / 8,680 bytes in these fixtures), not savings implemented by this PR.

## Existing native primitive and isolated witness

Taichi already provides `Device::memcpy_internal(DevicePtr dst, DevicePtr src, uint64_t size)` in `taichi/rhi/public_device.h`: synchronized, intra-device copying with byte offsets. The CPU implementation in `taichi/rhi/cpu/cpu_device.cpp` uses the two allocations and offsets with `std::memcpy`. The CUDA implementation in `taichi/rhi/cuda/cuda_device.cpp` uses its driver's device-to-device copy; that path was only source-inspected here and is not qualified by these CPU results.

The test captures the actual centroid ndarray from the real readback and identifies the actual seed upload by its owned host-vector pointer. While the fixture and source remain alive and quiescent, it allocates separate native scratch, checks device identity, and copies four published columns into each of two banks: **eight native slices of `N*8` bytes**, 192 bytes for N=3. A readback of the entire scratch destination matches the actual original seed upload **bit for bit**, including both banks, untrimmed mass, Q31 publication and zero inactive rows.

A second separate source allocation receives the captured values with poisoned inactive rows. Copying only `[0,N)` still matches; copying all H demonstrably fails. No poison reaches production arrays. The Q31 control requires an actual trial/publication difference, so using trial columns would fail. Existing installed-state hashing and control snapshots remain unchanged across these scratch operations, as do readiness, source generation and completed ticks. This proves the bounded native transport primitive and mapping on CPU; it is not a production replacement, tick qualification or transactional-failure proof.

## Reviewable next proposal

A subsequent runtime slice could use the existing candidate result/currentness checks to obtain the cascade's owned descriptors and native published values synchronously inside installation, without performing public numerical inspection. After all existing structural/work checks, use the native eight-slice copy into the live owner's native bank allocation and preserve zero inactive destination rows. Retain the existing final publication/synchronization and failure rules. No numerical recomputation, new generation protocol, scheduler, borrowed view stored in an owner, alternate transfer framework or reservation shrink is needed.

Keep all binder/query native source and validity/summary checks and their 248 readback bytes. Keep the full public candidate inspector and its lifetime behavior. Keep all other installation table uploads and host descriptor validation. Update transfer reporting to describe the calls actually performed by the replacement; do not subtract an inspection readback from the current 248, since it was never included there. Preserve independent public live inspection, which must still return current native slots. Initial installation's contiguous logical prefix must not be generalized to refresh's stable packet slots with holes.

Review must authorize that runtime replacement before implementation. Its acceptance subset should add source/currentness/work refusal and healthy retry controls, exact both-bank publication/history comparison, precision controls, nontrivial inactive-capacity controls, backend-failure unreadiness and scoped compiler/host-path coverage. Actual before/after journal totals should establish the realized transfer change. This inspection supplies the primitive and cost evidence, not those future runtime proofs.

## Local evidence and limits

At the tested source, quick CI passes its three native CTests, 632-sample placement arithmetic and placement-capacity witness. All four new Clang CTests pass (1/2 workers × bulk/direct, 53.87 seconds total). GCC 11 passes 1-worker bulk and 2-worker direct probes. Each run includes all three fixtures: **18 successful actual-transfer/native-copy observations**. Both compilers also pass existing basic and publication cases on 1-worker bulk / 2-worker direct: **eight existing case passes** with recording disabled. Clang 15/GCC 11 configurations, native runtime hashes, wrapped symbols, source-clean checks, logs and checksums are retained. All three final profile result files report exit zero.

The bundle also preserves unsuccessful development: an unpinned test-only snapshot misuse crashed because a tick-executor snapshot was applied before tick preparation; the corrected test uses installed-state hashing/control snapshots. Published `119e494e` then failed the precision-sensitivity guard on four Clang CTests and the first GCC probe because its initial coordinates did not distinguish trial from Q31 publication. The next commit replaces them with the one-third control. Failed logs/source and the correction remain retained; they are not counted as passes. Normal post-PASS Taichi cache-lock warnings remain unchanged, with no cache cleanup.

Retained bundle: `/tmp/hcp-hierarchy-seed-c2c553c1-evidence.tar.gz`, SHA-256 `c5a62ed159277f7adc2d2ad6c23054bba99c42e0ab7479a644e319d8cdfcedec`. Final profile and bundle manifests were verified. This machine-local artifact is not remotely available merely because its hash is published..

Reproduction on the matched native build:

```sh
bash engine/scripts/run_local_ci.sh quick /absolute/new/quick-evidence
ctest --test-dir engine/build -j 1 --no-tests=error --output-on-failure \
  -R '^hierarchy_seed_transport_'
engine/build/live_hierarchy_test --threads 1 --case basic
engine/build/live_hierarchy_test --threads 2 --direct --case publication
```

The retained command script includes exact GCC configuration and both 1-worker bulk / 2-worker direct runs. New CTests are registered only on Linux GNU/Clang, matching the linker instrumentation; an explicit unsupported case fails rather than claiming an inspection pass. Four normal native CTests will run in hosted regression. The accepted full baseline was not repeated locally: no full regression, no-unroll matrix, database or GPU qualification is claimed. GitHub CI remains authoritative.
