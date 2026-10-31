# B1 contact-safe candidate tick CPU qualification

Implementation source: `3cabc70f1812fceaaf759460fe9328279bfa9d23`, on
`157f46c2c07331429387376c0db2aef22a0839fb` (accepted #200).
Taichi gitlink: `350b7516fba428710a65ec04bf74b1b963daa840`.
Clang/LLVM 15.0.6, actual x64 execution, 2026-10-31. This is contributor CPU
qualification for #201 under accepted #199; maintainer acceptance and hosted
validation remain separate. B2 placement, B3 admission, owners, CUDA and Docker
are outside this record.

## Results

| Gate | Observed result |
|---|---|
| Full CPU regression | **109/109**, zero failures, **2520.39 seconds**, CTest `-j2`. All 105 existing suites and four new B1 suites passed. |
| B1 functional matrix | **8 modes / 96 case groups**: one/two workers, BulkCopy/CpuDirect, default/no-unroll LLVM. Four default modes are extracted from the full CTest log; four no-unroll runs are separately retained. |
| Actual new emitted modules | **544**, collected and LLVM-verified: 31 candidate kernels plus three native-selection primitives, two capacity/precision profiles, eight configurations. |
| Existing emitted modules | **56 C3/refresh + 168 selection modules** reproduced byte for byte. Uncompressed hashes and collection/audit logs are retained under `previous/`; the matching modules remain in the accepted [refresh](../../live_hierarchy/refresh-v1/ir) and [selection](../../native_selection/v1/ir) records. All 768 actual modules were parsed/verified. |
| New collector controls | **592** unsafe writer/copy mutations rejected and **464** safe redundant-store controls accepted. Actual candidate C3 force/poll additionally preserve **960** unsafe and **64** safe semantic/publication controls across the collector profiles. Mutated modules are LLVM-verified before audit; unsafe mutations are not executed. |
| Existing extra execution gates | All four no-unroll hierarchy, refresh and selection modes passed (12 runs), without changing their pass criteria. |
| PostgreSQL-to-active | **180 checks**, `private_reads=0`, identical cold fingerprints before/after. One new warm fixture; retained core/Linguistics stores remain read-only. PostgreSQL 16.15 and image identity are recorded in `results.json`. |
| Actual backend | CPU/x64; CPU-only device verification passed. The new qualification CLI explicitly refuses CUDA. |

The complete raw output, argument ABI/shape/scalar offsets, source and binary
hashes, build/configuration/toolchain records and per-mode metrics are retained.
Each retained hierarchy profile has 34 prepared contexts and 431 actual argument
entries; its binding reservation allows 531 argument slots and 34 contexts.
`results.json` is the index; `SHA256SUMS` hashes the retained files. LLVM modules
and large text logs use gzip. The clean wrapper build preceded the final
edge-dominance audit rebuild; production source was unchanged by that test-only
follow-up. Binary hashes were checked again after all tests.

## What the witnesses establish

Moving contact overflow rejects with named C demand while preserving the entire
committed numerical/control/native/topology/report/query snapshot. Late
GeometryWork and reference-peak rejection do the same. Valid retries match an
independent execution that never attempted the rejected tick. Sleeping targets
are exercised on both index sides and across pointer blocks, with contact
entry/exit and both endpoint force paths checked independently.

The actual shared production builders agree with ordinary single-level ticks
bit for bit and with independent long-double force/motion references. Dirty
multilevel tests retain selector 1, unequal packets, distinct previous history,
unpublished lag and pending notifications. Full same-poll centroid work and
at-most-one visits remain qualified; no frozen awake list replaces native work.

Thirty-two retained letter/UTF-8 reject/commit cycles preserve the measured native
backing, list/allocator/pool, layout, compiler and binding counters. Samples span
the first attempted tick and subsequent reuse; linked C++ allocation observation
covers individual ordinary and healthy rejected operations and records zero
recurring `new` calls. These are the measured counters, not whole-process malloc,
driver residency or GPU pinning accounting.

Asymmetric `(P,G)=(4096,70)` and `(70,4096)` tests use **three present bodies and
fields**, exercise the last records (including field 4095), and preserve flags,
final speed and coherent packets through 32 ticks without backing growth. This
qualifies unused blocks and capacity edges, not a dense 4096-body study. Empty
logical populations, disabled capability, prerequisite and measured-native
ScratchBytes rejection are retained too.

Unexpected candidate/publication failure injections make the execution unready;
no healthy rollback after copying begins is claimed. Public query tokens survive
healthy rejection and become stale after successful publication. Generation/tick
assertions and the existing same-execution capture gates preserve the reader
contract: changed generation invalidates source mappings; stable mappings can be
reused to observe subsequent completed ticks.

Actual emitted writer/root/SC checks and frozen ownership/type/extent checks are
bounded to these compiled profiles. Copy checks require paired complete rows
and an enabled control-flow edge dominating each write. The controls include
wrong targets, missing surfaces, mixed columns and missing guards. This is not a
general alias/equivalence proof. TickWork reports conservative copy/reference
bounds, not FLOPs; requested work is distinct from completed geometry comparisons.
Static scratch preflight is supplemented by measured native setup demand before
Ready. See the [implementation contract](../../../docs/CONTACT-SAFE-TICKS.md).

## Reproduction and provenance

Use the repository's [native CPU build](../../../README.md), matching backend
and compiler pins. The ordinary CTest inventory contains all four default B1
modes; the following is a representative additional mode and collector:

```sh
source engine/build/engine-env.sh
ctest --test-dir engine/build -j2 --output-on-failure
engine/build/live_hierarchy_test --threads 2 --direct --compiler-no-unroll --case contact_tick
hcp_b1_ir=$(mktemp -d)
HCP_B1_TICK_IR_DIR="$hcp_b1_ir" engine/build/live_hierarchy_test --threads 2 --direct --compiler-no-unroll --case contact_tick_ir
```

The retained suite/matrix runners record the exact cloud invocation, with
installation paths specific to that environment. The initial bulk shard finished
before the remaining disjoint worker/path shards ran concurrently; the coordinator
split interrupted no engine test and repeated no mode. CPU affinity reports five
threads; the environment's cgroup quota is four CPUs. No wall-time acceptance
threshold is inferred from this run.

The integration run retained warm fixture
`hcp_warm_d_20261031172304_17318`, using cold databases
`hcp_c3_core_20261027231537_16221` and
`hcp_c3_ling_20261027231537_16221`. Cold hashes and native client library provenance
are under `integration/`. No source/private store reset or write was performed.

Hosted [run 41220868948](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/41220868948)
at implementation head `3cabc70` is separate evidence and was still running when
this contributor record was assembled. The earlier run at `3d1ec94` was canceled
when the audit-only edge-dominance follow-up superseded it; it is not a failed
engine qualification or acceptance evidence for this source. No manual rerun of
the accepted baseline was launched.
