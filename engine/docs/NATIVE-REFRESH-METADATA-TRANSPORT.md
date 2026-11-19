# Native C3 refresh metadata transport

Status: **contributor implementation awaiting review**, under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343). The maintainer [provisionally accepted #379 and authorized this runtime replacement](https://github.com/Human-Cognome-Project/human-cognome-project/pull/379#issuecomment-6993076231) at `5b5fd4c2b1001ab652d28d9a9140c3e2daacc4ad`. This is stacked on that unmerged inspection; main remains `22ea6d55` at source selection. Tested implementation: `6196eca80c163747c4a590b2d7e43619e1824f4d`; tree `d0bc67a916e6da2896f07597e1bb207a8e63e63d`. The [accepted inspection](REFRESH-METADATA-TRANSPORT.md) retains its original baseline and scratch-only measurements. Authoritative GitHub CI and maintainer integration remain separate gates.

## Runtime change and preserved boundary

`LiveHierarchy::commit_refresh` retains its existing postclaim exclusive boundary. It now copies child links (`r.arrays[4]` → `a.arrays[3]`), exact u64 ratios (`6` → `5`) and precision masks (`7` → `6`) through the pinned fork's existing synchronous `Device::memcpy_internal`. Each request covers the entire reserved array, including inactive padding. Before each copy, it checks nonnull/same device, distinct allocations, matching dtype/shape and both full byte extents against the existing host semantic vector. A failure after claims retains the existing unready rule.

Initial candidate uploads, host semantic vectors and their swaps, remapped cfg/source/coverage/response/owner tables, both compiled refresh kernels, native source/claim checks, conservative work charges, final synchronization/status checks and one-generation publication are preserved. Native allocations and frozen argument bindings stay owned by the same live hierarchy; no pointer or borrowed view escapes the operation. No storage reservation, layout, compilation policy, numerical rule or eligibility guard changes.

`UpdateResult::upload_bytes` and `readback_bytes` report the remaining host requests. The additive `native_copy_bytes` field separately counts completed intradevice copy request bytes. It is diagnostic result metadata, not an admission token or authority for numerical state. Refusals/NoChange perform no metadata copies and report zero for this field. An exception after a partial commit does not return a healthy successful result. Native copies still move memory; host-request reductions are not claims about physical DMA, resident bytes, timing or throughput.

The shared routine is reached from installed `refresh_live_hierarchy` and `stage_current_admission` for bootstrap, generated admission and atomic exchange. All continue through the same claim/remapping/publication/failure protocol. This is not an installation seed change, owner lifetime refactor, new transfer abstraction, public operation or CUDA qualification.

## Measured host traffic and native request volume

The accepted baseline is `9043bcb8b5a507a0bcc4ff24e5ac7c10df46d582`. The same three-node/two-body fixture uses L140/S512, H8/Q52 or H70/Q52/Q31; Q31 also retains odd exact u64 provenance above double's exact integer range. Each fixture performs a precision edit and a source-count edit.

| Host path | H | Baseline host upload bytes | Current host upload bytes | Host readback bytes (unchanged) | Removed host upload bytes | Reported native copy bytes |
|---|---:|---:|---:|---:|---:|---:|
| bulk | 8 | 113,144 | 104,328 | 12 | 8,816 | 8,816 |
| bulk | 70 | 120,336 | 111,024 | 12 | 9,312 | 9,312 |
| direct | 8 | 106,440 | 97,624 | 12 | 8,816 | 8,816 |
| direct | 70 | 113,632 | 104,320 | 12 | 9,312 | 9,312 |

Both phases and both H70 precision profiles have these totals. The host journal records three fewer calls: 38 events in bulk and 35 in direct, compared with 41/38 in the accepted inspection. Native request volume is `4L + 16S + 8H`, with no reservation shrink or semantic work refund.

The shared callers also reconcile all current host calls with their returned counters. These are current measurements, without a separately qualified before/after saving for these callers:

| Successful caller / fixture | Bulk host upload bytes | Direct host upload bytes | Host readback bytes | Reported native copy bytes |
|---|---:|---:|---:|---:|
| Bootstrap, body bits52 | 118,368 | 111,664 | 812 | 9,312 |
| Bootstrap, body bits8 | 123,936 | 117,232 | 812 | 9,312 |
| Generated admission, P70, each of two rounds | 106,880 | 100,176 | 604 | 9,312 |
| Generated admission, P128, each of two rounds | 112,448 | 105,744 | 604 | 9,312 |
| Atomic expansion or compression | 107,816 | 101,112 | 1,332 | 9,312 |

The fixed test-only linker journal observes real upload/readback calls and call-time payloads. The probes resolve each candidate array from both actual frozen kernel bindings, require exactly one full staging upload and no selected live host upload, then compare full native staged/live buffers with that recorded payload. Native-copy volume comes from the result counter, independently checked against the three full array extents and the completed commit-boundary counter; it is not a hardware bandwidth measurement or a lower-level device-call trace.

Before success, the probes poison the final eight unused bytes of each real live metadata array. NoChange and zero-work refusal preserve committed state; the latter preserves the dirty tails and remains healthy for retry. Successful copying must clear every tail by matching the full staged payload. Both refresh kernel IR audits retain their positive/negative read-only controls. Actual source/numerical/packet/history snapshots remain unchanged during the subsequent observation. Each refresh advances exactly one generation and is followed by a native tick. The earlier accepted inspection retains its separate truncated-copy counterexample; this implementation tests full padding through the real commit.

## Shared callers and failure controls

The existing `admission_bootstrap`, `admission_commit` and `exchange_commit` witnesses now observe each successful shared commit with the same journal and exact full-buffer checks, including dirty destination tails and reconciled host/native counters. All diagnostic setup/readbacks occur outside the allocation-guarded operation.

Bootstrap checks full/reduced body precision, explicit focus, refused work/input/preclaim attempts and healthy retry. On its empty test Harness only, temporary native map claims are released in reverse order before any model is admitted, requiring actual recycled destinations to differ from scratch row order. It preserves zero live/source state and changes no production allocator policy. Generated admission covers two rounds at capacities 70/128, actual claim remapping, exact survivor state, contact installation, stable prepared backing and following ticks. Atomic expansion/compression checks incoming mappings, outgoing retirement, unchanged independent survivors, transferred motion, final contacts/history and exactly one unticked generation. Retirements still cannot fund new claims. No caller imports recipe starts or reconstructed float ratios.

The new refresh operation-boundary injections throw after claims before entry to the metadata commit, after its actual completed copies/publication, and after the following real final synchronization. All leave the owner unready without advancing the public generation/tick, and later refresh refuses. Two additional controls change only a live ndarray's wrapper dtype or shape, requiring the real commit guard to reject after claims; wrapper tags are restored solely for teardown and failed execution is never resumed. These are test-injected boundary/invariant failures, not hardware-fault qualification.

Existing generated-admission failures now include point 3 after numerical/metadata commit as well as points 2/6. A bootstrap point-3 witness covers both body precision profiles and requires a completed real metadata commit before failure; its existing point-2/6 controls remain. Atomic exchange retains failures after claims, numerical writes, contact publication and before map publication. Ordinary refusal remains distinct from postclaim failure. Production code has no diagnostic legacy branch or fault switch added by this slice.

## Source-pinned local validation

At the tested clean source on actual CPU, using the exact matching fork tree `8b31651be07d8bad0b2a6a261369e8690b9c3b53` and recursive submodule pins:

- Quick CI passes three native CTests, 632 independent placement arithmetic samples and the placement-capacity witness.
- Clang 15 passes **9/9 CTests in 517.56 seconds**: all four native transport modes, existing complete refresh 1-worker bulk/2-worker direct, initial native seed 1-worker bulk, and both prepared-study-owner modes. All five shared cases above also pass on 1-worker bulk and 2-worker direct: **10 case passes**.
- GCC 11's wrapper with the matching Clang-built fork passes **5/5 CTests in 282.93 seconds**: native transport 1-worker bulk/2-worker direct, complete refresh 2-worker direct, initial native seed 2-worker direct and prepared-study-owner 2-worker direct. Its five shared cases pass on 2-worker direct: **5 case passes**.

The six transport probes contain **36 successful actual refresh observations** and **30 postclaim injected boundary/guard controls**. The shared cases add **24 successful instrumented commits**, with remapping, survivor/retirement, ordinary refusal/retry, postclaim failure and following-tick controls. Existing complete refresh tests retain dirty/history/u64/overlap/field-lifetime/reuse and compiler checks. Initial seed and prepared-owner regressions remain independently qualified within their selected modes. All three profile result files exit zero with clean, unchanged source before/after. Subsequent documentation changes no tested code.

Development logs retain the intentionally interrupted overlapping second build, the subsequent serial build and one genuine failing test: the first bootstrap probe asserted nonidentity destinations on an untouched empty map, which actually supplied identity order. Its native payload check passed, but the remapping control correctly failed. The fixture now explicitly exercises recycling and keeps the nonidentity assertion. No runtime rule was weakened. Development runs are excluded from source-pinned qualification counts. Normal post-PASS native cache-lock warnings are preserved without cache cleanup.

Verified machine-local bundle: `/tmp/hcp-native-refresh-6196eca8-evidence.tar.gz`, SHA-256 `8c88b8a673e67a95582cf5004e9dbfc4dc287de9eb9c999ba6dfa4af14702831`. It contains exact commands/configuration, before/after clean-source checks, source patch and unchanged-other-components proof, fork/submodule and runtime/binary hashes, full CTest/shared-case logs, development failure reproduction and checked profile/bundle manifests. Publishing the digest does not make the raw bundle remotely available.

Reproduction on the matching clean native build:

```sh
bash engine/scripts/run_local_ci.sh quick /absolute/new/quick-evidence
ctest --test-dir engine/build -j 1 --no-tests=error --output-on-failure \
  -R '^(refresh_transport_[12]_(bulk|direct)|live_refresh_(1_bulk|2_direct)|hierarchy_seed_transport_1_bulk|prepared_study_owner_(1_bulk|2_direct))$'
engine/build/live_hierarchy_test --threads 1 --case admission_bootstrap
engine/build/live_hierarchy_test --threads 1 --case admission_bootstrap_failures
engine/build/live_hierarchy_test --threads 1 --case admission_commit
engine/build/live_hierarchy_test --threads 1 --case admission_commit_failures
engine/build/live_hierarchy_test --threads 1 --case exchange_commit
```

The retained script repeats all five shared cases with two workers/direct and supplies exact GCC configuration/commands. The normal hosted placement matrix includes the bootstrap failure increment and shared-path checks; the four existing transport probes now test the runtime replacement. GitHub CI is authoritative. Full local regression, the no-unroll matrix, retained database requalification, GPU execution and performance remain outside this local evidence. No retained store is changed. All merges remain with the maintainer.
