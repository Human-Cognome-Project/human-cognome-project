# #430 hosted placement qualification: SNode ID budget

## Confirmed hosted failure

PR #430 head `8dbe7ec7` is on integration hold under [review 7147550376](https://github.com/Human-Cognome-Project/human-cognome-project/pull/430#issuecomment-7147550376).
Its earlier preparation-probe finding remains resolved. Run [45493722124](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/45493722124)
failed all four native-compiler jobs at `SNode ID 2048 exceeds llvm_snode_capacity 2048`.
The jobs are 138802935545 (two/bulk), 138802935552 (one/direct),
138802935596 (two/direct), and 138802935607 (one/bulk). Native build,
projection, database and regression shards 1–3 passed. The aggregate native gate
failed. Existing contributor CPU evidence does not override that result.

The inspected one/bulk compiler artifact (14970243976, SHA256
`aa18c6132c110a04f6dc6fccc9c775426d52b70b4041e5f8be27aa9014143c2c`)
shows default/no-unroll collection and separate native/refresh/selection/contact
steps preceding the incomplete no-unroll placement step. That placement process
records 23 named case passes, including grouped_exchange and live_focus, then
admission failure points 2 and 3 before aborting during the next fixture for point 6.
This identifies the stopped invocation and last completed work; 2048 alone is only
the first denied ID, not the complete invocation's eventual high-water mark.

The artifact source is GitHub's generated merge `f2e76e59`, whose tree
`07b4b1c9446d7765f5668eb279d2e232751422fc` exactly equals reviewed head `8dbe7ec7`.
The source-pinned fork tree is `a94a9eb350c94dfa2eb586bee49c53e0ee886648`.
Four raw compiler job logs and the original artifact are retained with the local
evidence; the artifact digest was verified before inspection.

## Cause and scoped correction

The [maintainer handoff 7147687096](https://github.com/Human-Cognome-Project/human-cognome-project/pull/430#issuecomment-7147687096)
identifies the existing startup configuration. The qualification runner disables
environment reads and explicitly selects 2048 for `--case placement` before
constructing one Runtime. Exporting `TI_LLVM_SNODE_CAPACITY` in CI cannot override
that invocation. The [fork contract](../taichi/modernization/INSTALL-CONFIGURATION.md)
and `SNode::SNode`/`Program` source establish monotonic IDs over a Program lifetime,
including released fixture layouts. Tree slots can recycle; individual SNode IDs
are not reclaimed for this purpose. This is an ID-space metadata budget, not live
particle capacity or evidence by itself of an ownership leak.

The candidate at clean code `4999781818eb40578892b2e2d41fcc800c64ee2e`, tree
`f1be40074684ef1fbb7718ecf3ac89b0c8346b31`, reserves 4096 for this combined test
only. Other test overrides and production defaults remain unchanged. Per-case
read-only `SNode::counter` snapshots record begin/end, created IDs, highest ID and
the actual configured limit; the complete invocation must stay within that limit.
No counters are reset and no fixture, compiler audit or numerical assertion is
removed. The same grouped, failure, ownership and remap cases continue in one
Runtime. Model particle/field capacities and allocation policy are unchanged.

All three complete configurations below create **2,094 IDs**, highest **2,093**.
The grouped_exchange span is 1553–1643: **90 IDs** for its three fixture layouts.
Subtracting that measured span gives 2,004, within the former 2,048 reservation;
adding it exceeds that reservation by 46. This is a derived counterfactual,
not a separately executed pre-grouped matrix. The observed growth matches the
added layouts without establishing a new lifetime or reclamation claim.

| Case near the exhausted budget | Begin | End (exclusive) | Created |
|---|---:|---:|---:|
| grouped_exchange | 1553 | 1643 | 90 |
| exchange_owner | 1643 | 1674 | 31 |
| exchange_owner_multiple | 1674 | 1704 | 30 |
| exchange_owner_equal | 1704 | 1734 | 30 |
| live_focus | 1734 | 1974 | 240 |
| admission_commit_failures | 1974 | 2064 | 90 |
| admission_contact_remap | 2064 | 2094 | 30 |

The last two cases now finish in that same Runtime; isolated grouped_exchange
success alone would not qualify the cumulative budget.

At 24 bytes per configured SNode slot on this 64-bit target, the test Program's
slot-table request increases from 49,152 to 98,304 bytes before page alignment:
48 KiB additional metadata. Tree capacity and populated model arrays are unchanged.
This is neither complete runtime-byte accounting nor a general installation policy.

## Regression cancellation and bounded execution time

The cancelled regression shard also contains confirmed failures. Job
[138802935580](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/45493722124/job/138802935580)
records the same 2048 error in placement_1_bulk, placement_1_direct and placement_2_bulk.
The fourth placement test was running when cancellation stopped the shard.
The retained inventory has 269 tests; index-based shard 4 selects 67, including
all four placement tests at indexes 72, 92, 112 and 132. Only 32 completed
before cancellation. A cancelled shard is not passing evidence, and those three
recorded test failures must not be hidden by its overall cancelled status.

Job timestamps are 21:49:45–23:20:01 UTC, about 90m16s, under a configured
90-minute job budget. This is consistent with exhaustion of that budget; the log
itself reports cancellation rather than a more specific cause. Keeping four
roughly 21–22-minute placement invocations serially in one shard already leaves
little room for the remaining work. The correction gives regression consumers
180 minutes while retaining their -j1 execution, exact disjoint inventory, all
registrations and per-test timeouts. Compiler-job limits and check propagation
remain unchanged. A complete replacement hosted run is required; the longer
allowance is not itself successful timing or regression qualification.

## Validation and remaining gate

Fresh local qualification uses code/tree above, native LLVM CPU with
Clang/LLVM 15.0.7, the pinned fork tree, and runtime bitcode SHA256
`4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada`.
Before/after source guards are clean for every run. Exact CMake configurations,
compiler versions, binary hashes, commands and complete output are retained.

After sourcing `engine/build/engine-env.sh`, each command passes all 25 named
cases with identical ID accounting:

```sh
engine/build/live_hierarchy_test --threads 2 --compiler-no-unroll --case placement --direct
engine/build/live_hierarchy_test --threads 1 --compiler-no-unroll --case placement
engine/build/live_hierarchy_test --threads 2 --case placement
```

That is 75 named case passes across three complete invocations. The default
command is a direct execution of a previously failing CTest command, not an
additional executed CTest. Quick qualification also passes three CTests
(engine_smoke_test, prepared_capacity_test, strict_math_cpu), standalone placement
arithmetic and placement-capacity controls, plus build/backend checks.
`check_hosted_cpu_ci.sh` passes its complete/disjoint shard, provenance and
failure-propagation controls; ShellCheck passes the four hosted orchestration
scripts. No numerical assertions, compiler audits, case registrations or
per-test limits are removed. The workflow edit changes only the regression job
allowance from 90 to 180 minutes; compiler jobs retain 90 minutes.

The initial census compilation used incorrect native accessor names. Its failed
build and corrected build are retained, and the failed build is excluded from
qualification. The public qualified source uses SNode::counter and
Program::compile_config().llvm_snode_capacity. Final evidence/workflow metadata
does not change executable source from the qualified code pin.

Local archive `/tmp/hcp-430-49997818-evidence.tar.gz`, SHA256
`35e8a8bde3ca04fc4ae3ddb7b9c3507607313086d3e280c420236216d8a41068`,
contains original raw job logs, both downloaded hosted artifacts, all four local
qualification directories and manifests, source patch/incremental Git bundle,
configuration/binary/runtime pins and development failure logs. Root and all
nested manifests pass after fresh extraction. The archive is machine-local;
its digest does not make the full logs remotely available. The measured rows
and commands above are the bounded portable review record.

This is scoped local CPU evidence, not a full local hosted matrix, GPU result,
general memory policy or restored hosted acceptance. Run 45493722124 remains
failed. Submit the corrected head for a fresh automatic authoritative GitHub
run and maintainer re-review; #430 remains on integration hold until reconciled.
All merge events remain maintainer-only.
