# C3-2 coherent refresh CPU evidence

Tested source: `e0773b838efcf57cb14bff053b93ac56be03f2ab`, based on main
`5b6d7957d452fe56a23735241a0307419ee0e638`. The production implementation is
`fc48ac3`; the later commits add tests and strengthen the old-history witness.
The CMake-only correction at `204eff7e74075906e96e6dd476a418e21ae1d772`
supplies the runtime path to the four new CTest registrations; engine, test and
integration binaries remain byte-identical to the completed local regression.
The [implementation contract](../../../docs/LIVE-REFRESH.md) follows accepted
[#197](../../../../docs/live-hierarchy-refresh-plan.md).

Maintainer acceptance is complete in #198, reviewed head `f4c5ba9`, merge
`5ea9c7674f87ef01d5cad3024785b574bc79a75f`. This record preserves contributor
and independently inspected hosted CPU evidence separately. It does not qualify
CUDA refresh, native admission, contact-safe
candidate ticks, an enduring owner or the later interactive/Docker study.
The accepted-displacement motion rule from #196 is unchanged.

## Executed scope

| Validation | Result and source |
|---|---|
| Local CPU-only regression | **101/101**, 1830.99 seconds, source `e0773b8`. |
| Complete refresh matrix | **8/8** modes: one/two workers × bulk/direct × default/scalar; **72/72** case groups. |
| Refresh compiler controls | 64 expected unsafe-control rejections and 32 safe-control passes across the eight modes, covering both precision profiles. |
| Retained emitted modules | 56 local and 56 hosted modules; all 56 pairs identical after removing only Clang version-ident metadata. Local 15.0.6, hosted 15.0.7; actual target CPU znver3. |
| PostgreSQL integration | **180/180**, zero private reads and unchanged cold fingerprints; separately pinned at `fe7299c` with unchanged production and integration executable. |
| Clean-environment CTest correction | **4/4**, 335.23 seconds, parent `TI_LIB_DIR` explicitly unset; CMake-only change committed as `204eff7`. |
| First hosted native CPU | [Run 41065599375 / job 124096008985](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/41065599375/job/124096008985): **97/101**, 2070.71 seconds. Four new refresh registrations fail during initialization, before assertions, because `TI_LIB_DIR` was omitted. Later scalar/integration/backend steps skipped. |
| Budget-limited hosted native CPU | [Run 41101471179 / job 124219287541](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/41101471179/job/124219287541), source `204eff7`: **101/101**, 3353.15 seconds. Compiler collection/audit passed; the overall 90-minute job limit cancelled the final extra refresh mode. Integration/backend steps skipped, so this is incomplete hosted acceptance. |
| Accepted hosted native CPU | [Run 41123391538 / job 124295521781](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/41123391538/job/124295521781), source `904f2bc`: **101/101**, 3355.27 seconds; all four extra hierarchy and four refresh no-unroll modes; compiler collection/audit; **180/180** integration with zero private reads and unchanged cold fingerprints; actual CPU-only backend verification. All nine checks passed. Total job: 5697 seconds. |

All eight refresh modes execute nine case groups. The tests retain a nonzero
selector, unequal selected/inactive packets and distinct old history on an
unchanged dirty subtree; an affected ancestor uses its current child publication,
while the unpublished body change and native pending bit survive. This catches
global reseeding and selector/history resets, with a later poll consuming the
remaining input.

The matrix also covers count-only edits with unchanged f32 shares, exact u64
provenance above 2^53 with identical numerical packets, distinct source rows with
equal ordinals and repeated-row aliases, overlapping participation, changed
root coverage, an independently checked combined physical integration after
source/contact edits, new and retired comparisons, and generation-qualified
query/capture rejection. Capture observes the same native execution through
a noncontiguous stable-slot mapping.

Each mode retains 32 insertion/retirement reuse rounds using intrinsic values
from the retained English letter fixture, with distinct simultaneous occurrences,
64 successful reuse commits/contact edits, and exact survivor motion/state at
the edit boundary. Changed physical mass is tested through a new occurrence;
there is no survivor-mass setter or public start-position field. Ten independent
reservation failures, 21 ordinary reference/contract rejections, a pre-claim
failure and four actual-claim late failures exercise healthy rejection versus
explicit unready disposition. Source-empty contacts and cross-block endpoint
invalidation retain their native semantics; operational contact discovery is
not supplied by this slice.

Known native backing, list high-water, layout, compiler and pool counters remain
equal across each measured reuse window, with zero observed linked C++ new/new[].
These observations do not cover arbitrary malloc, driver residency or pinned
host transfers. Work charges include a conservative source-preflight reservation;
they are not a FLOP or performance measurement. `results.json` and raw logs
retain the per-mode inventory and counters.

## Compiler evidence and limits

Eight configurations cover one/two CPU workers, BulkCopy/CpuDirect and default/
no-unroll LLVM optimization. Each retains the original three hierarchy modules
and candidate/commit refresh modules for body52 and body8, for **56 optimized
modules** per environment (**112** retained local/hosted files). `configuration.txt` retains compiler/target settings and actual
argument/shape offsets. All modules were parsed by LLVM; the original concurrent
reader audit remains semantic CFG/data-flow coverage with its existing unsafe
and safe controls.

The four actual refresh specializations in each numerical mode reject candidate
writes to committed/body inputs, commit writes to survivor state, incomplete
packet/selector delivery and mixed packet/history components. LLVM-verified
negative controls remove packet/selector writes or inject a candidate body write;
safe redundant stores pass. The matrix supplies independent numerical/state
checks alongside this bounded emitted-code audit. The audit is not a universal
compiler-equivalence proof, and these CPU modules are not CUDA ordering evidence.

The backend tree is `350b7516fba428710a65ec04bf74b1b963daa840`. The original
`configuration.json`'s `backend` field records the repository checkout revision;
it is not a separate backend tree hash. `results.json` makes that distinction
explicit without rewriting the original capture.

`uncompressed-sha256.json` records original byte lengths and SHA256 for losslessly
compressed logs/modules. `SHA256SUMS` verifies the retained artifacts themselves.
Earlier C3 failures and #196's separate mixed-precision subnormal failure remain
unchanged historical evidence.

## Integration provenance and superseded preparation

The PostgreSQL run is pinned to `fe7299c5a0afcaf5eeebbf0b736bbb1740eada57`.
Only the history test and its case selector changed afterward. The native
integration executable is byte-identical at the final source; production engine
source is unchanged. It passed 180 checks with 24 addressed reads, zero private
reads and identical cold-source fingerprints. Existing cold databases were
read-only; only the new warm fixture `hcp_warm_d_20261030132420_15483` was created
and retained. `integration/` preserves this distinct source pin and raw checks.

The first local full run was deliberately stopped after 30/101 suites completed,
before the changed refresh cases. Hosted run 41062786258 at `fe7299c` was cancelled
during module collection when the stronger history witness superseded that test
source. Neither is reported as a full regression success or an invariant failure.
The local results above come from the completed final-test-source runs.

`failed-hosted/` preserves the first hosted job's raw CTest output, full test log,
failed steps, job/step disposition and source pin. It is a failed run, despite
its passing original suites and compiler collection. `runtime-path-check/`
preserves the focused correction, generated CTest environment, passing
clean-environment output and byte-identical executable/runtime hashes. The local
full regression inherited the runtime path, which explains why it did not expose
the missing registration property. No engine arithmetic or assertion changed.

`budget-limited-hosted/` retains run 41101471179's complete passing CTest log,
raw job log, explicit GitHub time-limit annotation and step dispositions. Four
extra no-unroll hierarchy modes and three refresh modes finished successfully;
the fourth refresh log is partial, with four passing case groups and no suite
completion. Its partial output is not counted as a mode pass. All 56 actual
compiler modules are byte-identical to the earlier retained `hosted-ir/` files;
the comparison manifest records their hashes alongside this run's actual
source/build/compiler metadata and audit output. Integration and final backend
checks were skipped, so neither local integration nor CTest success completes
this hosted gate. Source review at `d97027c` found no implementation blockers
and requested the narrow CI budget correction before one replacement run.

`accepted-hosted/` retains that completed replacement's full CTest and job logs,
all eight complete extra modes, integration output/fingerprints, job/step and
nine-check dispositions, acceptance review and actual source/build/compiler
metadata. Its 56 modules are also byte-identical to the retained `hosted-ir/`
files, as recorded by the comparison manifest. The successful hosted warm
fixture is `hcp_warm_d_20261030220147_309`. This record downloads existing
artifacts; it launches no new native run or database operation. #198's acceptance
supersedes earlier pending-hosted wording while preserving those earlier results.

## Reproduction

Build the matched native CPU substrate and engine as documented in
[engine onboarding](../../../../docs/onboarding/FIRST-RUN.md), then source
`BUILD/engine-env.sh` and run:

```sh
env -u TI_LIB_DIR ctest --test-dir BUILD --output-on-failure -j 2
for workers in 1 2; do
  for host_path in bulk direct; do
    args=()
    if [ "$host_path" = direct ]; then args+=(--direct); fi
    BUILD/live_hierarchy_test --threads "$workers" --case refresh \
      --compiler-no-unroll "${args[@]}"
  done
done
```

Use `HCP_C3_IR_DIR=DIRECTORY BUILD/live_hierarchy_test --case ir` with the
corresponding worker/path/unroll flags to retain actual modules. The ordinary
hierarchy auditor accepts `--audit-ir DIRECTORY` and never executes foreign IR.
PostgreSQL reproduction uses the existing owned-container acceptance script and
retained read-only cold fixtures; no reset/drop is part of validation.

Full hardware execution/resource/ordering qualification follows locally after
CPU acceptance. The default CUDA guard stays in place; CPU execution and
CUDA-enabled compilation are not GPU qualification. #172/#177/#156/#145 remain open.
