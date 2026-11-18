# C3 refresh metadata transport inspection

Status: **inspection and proposal provisionally accepted in #379**, under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343) and the [refactoring plan](../../docs/engine-refactoring-plan.md). Tested source: `9043bcb8b5a507a0bcc4ff24e5ac7c10df46d582`, tree `6d9fa746ae407dc2ac1bb0c2eb1254db5dd159ba`, independently based on main `22ea6d55866a484f80fd64ed91b6384d57fea797`. Only tests and their CMake registration change at that source. Runtime/fork implementations, kernels, workflows and retained stores are unchanged. **Achieved runtime saving at this inspection source is zero.**

The maintainer [accepted `5b5fd4c2` and authorized the runtime follow-up](https://github.com/Human-Cognome-Project/human-cognome-project/pull/379#issuecomment-6993076231). The separate [native implementation record](NATIVE-REFRESH-METADATA-TRANSPORT.md) measures the realized host-request reduction and qualifies the shared callers. This page preserves the original inspection baseline; native CI and maintainer integration remain separate gates.

The accepted [C3 refresh contract](LIVE-REFRESH.md) governs this operation. The earlier [initial seed inspection](HIERARCHY-SEED-TRANSPORT.md) and its separately reviewed [native implementation](NATIVE-HIERARCHY-SEED-TRANSPORT.md) concern initial installation. This inspection concerns repeated metadata transport during an already-installed refresh; it does not generalize initial installation's contiguous packet prefix to refresh's stable slots.

## Actual path, ownership and lifetime

`Harness::refresh_live_hierarchy` enters `update_prepared_impl` in `src/field/prepared_update.cpp`. It validates the prospective source ledger and replacement hierarchy, preserves exact source provenance and prepaid work, drains old readers, stages candidate arrays and runs candidate validation. After successful native claims it remaps scratch source/field names, publishes the live metadata and numerical commit, synchronizes, checks status and publishes one generation. Ordinary preclaim refusal keeps the owner healthy and unchanged; postclaim backend failure makes it unready. None of those rules is replaced by a byte copy.

`LiveHierarchy::Impl::Refresh` owns candidate host vectors and native `r.arrays`; the same live owner holds published native `a.arrays`. `stage_refresh` uploads candidate tables. `commit_refresh` remaps cfg, source references, coverage and response/owner maps, but does not change the three tables below before uploading their identical host payloads again. Both refresh kernels bind the same candidate allocations. Their compiled argument audits establish that these three inputs are read-only.

| Metadata | Candidate native input | Live native input | Type and full reserved shape | Commit upload bytes |
|---|---|---|---|---:|
| Child links | `r.arrays[4]` | `a.arrays[3]` | i32 `{L}` | `4L` |
| Exact ratios | `r.arrays[6]` | `a.arrays[5]` | u64 `{2,S}` | `16S` |
| Precision masks | `r.arrays[7]` | `a.arrays[6]` | u64 `{H}` | `8H` |

Here L=140 and S=512. These are arrays' actual reserved extents, including unused rows. Topology and exact denominators remain HCP-owned semantics. The host vectors remain needed by validation, remapping, host metadata publication and later refreshes; no host storage reduction is proposed.

All candidate and live arrays are allocated through the same Program and live until their owner is destroyed. Candidate *contents* are current only for the corresponding staged refresh and can change on the next operation. `finish_refresh` swaps host vectors and advances bound metadata; it does not exchange these native allocations. A future replacement must consume current native inputs synchronously within the existing exclusive commit boundary, without retaining borrowed pointers across operations or using a diagnostic journal as admission authority.

There is one `commit_refresh` call site, in `update_prepared_impl`, but multiple operations reach it. `stage_current_admission` also enters that implementation for prepared-owner bootstrap, generated admission and atomic exchange. `PreparedStudyOwner::bootstrap` and `ExchangeCoordinator::execute` are concrete callers. **The transfer observations here qualify installed refresh only.** A change to the shared commit routine also requires actual bootstrap/admission/exchange controls; these measurements do not qualify or price those paths.

## Observed calls and independent native comparison

`tests/refresh_transport_inspection.inc` reuses the existing Linux GNU/Clang linker journal in `live_hierarchy_test`. It records actual upload/readback calls, native/host identities, full byte extents and call-time payloads in a fixed 256-event / 2 MiB buffer, forwarding every real operation. Production libraries are not instrumented. The known-payload journal control and existing no-allocation guard remain active.

For each selected live upload, the test finds exactly one preceding candidate upload from the same host address and byte extent into a different native array. Type, full shape, extent and recorded payload must agree. After real refresh completion, actual native candidate and live readbacks must each match that recorded payload exactly. This establishes more than equality of two host snapshots.

For each table an independently owned, same-device scratch allocation is filled with a dirty pattern. Copying `bytes-8` through the pinned fork's existing `Device::memcpy_internal` must fail full-buffer equality by retaining a stale tail. Copying the complete reserved extent must match the actual live input bit for bit. The dirty scratch, truncated copy and full copy never write a live owner or candidate input. Full selected live-table snapshots plus existing source/numerical/packet/history state hashes remain unchanged across the scratch comparison. These extra diagnostic readbacks and copies occur outside the measured refresh boundary.

Three fixtures use one three-node, two-body tree: H8/Q52, H70/Q52 and H70/Q31. The last uses exact integers `(1<<54)+1` and `(1<<54)+3`; the journal/native comparison requires the odd first word actually present in ratio storage, rather than reconstructed from f32 shares. Each fixture first checks an identical NoChange, then two successful changes: root precision, followed by a source-count replacement affecting both leaf and root provenance. Before each successful retry, zero work must refuse while preserving readiness, committed state and all selected native inputs, with no selected live upload. Each success advances exactly one generation (2 then 3) and is followed by an actual native tick. The test invokes the existing compiled-IR positive/negative controls for both refresh kernels to check the read-only input rule.

| Host path | H / profile field bits | Recorded events | Actual and reported upload bytes | Actual and reported readback bytes | Duplicate commit upload / full scratch native-copy bytes |
|---|---|---:|---:|---:|---:|
| bulk | 8 / 52 | 41 | 113,144 | 12 | 8,816 |
| bulk | 70 / 52 | 41 | 120,336 | 12 | 9,312 |
| bulk | 70 / 31, wide integers | 41 | 120,336 | 12 | 9,312 |
| direct | 8 / 52 | 38 | 106,440 | 12 | 8,816 |
| direct | 70 / 52 | 38 | 113,632 | 12 | 9,312 |
| direct | 70 / 31, wide integers | 38 | 113,632 | 12 | 9,312 |

Both phases have these totals, unchanged across measured compilers/workers. Other source host-path transfers differ; the selected three duplicate metadata uploads have the same extents on both paths.

The last column is `560 + 8,192 + 8H`. It counts the *second* three host uploads. Initial staging still uploads those bytes. A full native replacement would request the same byte count as an intradevice copy; on CPU that still copies memory. These are API-request byte counts, not measured DMA, memory residency, latency or throughput. There is no achieved transfer reduction, physical-bandwidth measurement, allocation shrink or GPU claim. Unlike the earlier initial-seed inspection, all observed refresh uploads/readbacks match `UpdateResult` reporting at this source.

## Reviewable next proposal

After review, a focused runtime slice could replace only those three second host uploads in `commit_refresh` with three full-extent `Device::memcpy_internal` requests from the current candidate arrays into the existing owned live arrays. The pinned fork already supplies this synchronized intradevice primitive; `CentroidCascade::copy_published` demonstrates its existing engine use. The scratch comparison exercises this exact API. No new kernel, owner swap, compiled-argument rebinding, public transfer framework or reservation change is needed.

Keep the first uploads, other remapped table uploads, host semantic vectors, both compiled refresh kernels, source/claim validation, exclusive staging/commit order, work gates, final synchronization/status checks, generation publication and failure rules. Check current owner, same device, distinct source/destination, exact dtype/shape and full capacity before mutation. Copy all reserved rows; an active prefix is insufficient. Account only actual remaining host transfers in `UpdateResult`; describe any native-copy request volume separately rather than claiming it as absent memory traffic. Do not refund semantic work on the basis of byte savings.

Runtime acceptance should compare actual before/after calls and exact numerical/packet/history results, preserving precision and odd-u64 controls, dirty reserved tails, no-op/refusal/healthy retry and postclaim failure unreadiness. Because the routine is shared, include real prepared bootstrap, generated admission and atomic exchange, including reordered native claim destinations and retirement/survivor behavior. Existing refresh refusal, reuse, history, field lifetime and compiler controls must continue to pass on both host paths and scoped compilers/workers. The inspection's successful CPU scratch copies do not by themselves establish these transactional runtime properties. CUDA refresh remains unqualified. The authorized [runtime follow-up](NATIVE-REFRESH-METADATA-TRANSPORT.md) records its implementation and separate validation.

## Source-pinned local evidence

Quick CI passes its three native CTests, 632-sample independent placement arithmetic and placement-capacity witness. Clang 15 passes all four new probes (1/2 workers × bulk/direct) and the complete existing `live_refresh_1_bulk` suite: five CTests, 145.66 seconds. GCC 11 passes the new 1-worker bulk and 2-worker direct probes plus complete existing `live_refresh_2_direct`: three CTests, 123.48 seconds. Each new probe includes six successful actual refreshes, for **36 actual-transfer/native-copy observations** across compilers. Existing refresh suites retain dirty/count changes, u64 overlap, refusal, field lifetime, history, reuse and compiler controls. All three final profile result files report exit zero with unchanged clean source before/after.

The retained bundle contains commands/configuration, pinned source patch, unchanged-production proof, exact fork/submodule state, runtime/binary hashes, actual transfer records, test logs and verified profile/bundle manifests. Earlier unpinned development build/test logs are retained separately and excluded from those qualification counts; no unsuccessful test run occurred in this slice. Normal post-PASS native cache-lock warnings are retained, with no cache cleanup. Bundle: `/tmp/hcp-refresh-9043bcb8-evidence.tar.gz`, SHA-256 `3cca27c03ba3ab74fe830d055d44d1e9bf7e286c04da1a316355e6c1b86e025f`. This machine-local artifact is not remotely available merely because its digest is published. Documentation is a subsequent commit; tested code remains pinned above.

Reproduction on a matched clean native build:

```sh
bash engine/scripts/run_local_ci.sh quick /absolute/new/quick-evidence
ctest --test-dir engine/build -j 1 --no-tests=error --output-on-failure \
  -R '^refresh_transport_[12]_(bulk|direct)$'
ctest --test-dir engine/build -j 1 --no-tests=error --output-on-failure \
  -R '^live_refresh_1_bulk$'
```

The retained command script supplies the exact GCC configuration and its three scoped suites. New CTests are Linux GNU/Clang only, matching the existing linker instrumentation; explicit unsupported execution fails instead of claiming a pass. Hosted native regression includes the four registered probes. No full local regression, no-unroll, database or GPU qualification is claimed. GitHub CI is authoritative; local CPU evidence permits only the existing provisional review path. All merges remain with the maintainer.
