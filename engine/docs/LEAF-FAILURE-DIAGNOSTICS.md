# Leaf assertion failure diagnostics

This test-only implementation follows [#421 decision 7113221968](https://github.com/Human-Cognome-Project/human-cognome-project/pull/421#issuecomment-7113221968)
under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343).
It depends on reviewed #421 `f739cfbcb45113afc5f71c0359da4e7cfaf5c2f4`,
reconciled with integrated main `fae58e7991f58d7bb3a781e93f4487bea3346efb`.
The [original investigation](LEAF-REFERENCE-INSPECTION.md) retains the failed
#417 artifact, successful same-head retry and limits of the constructed witness.
The original moving failure's operands and cause remain unknown.

Implementation and local qualification pin:
`e107c3372419d9b181ca2552ff38d8f819c09329`, tree
`3f9dceb4ca784515ce6cbce3dd227184fcd5a6c0`, native fork tree
`a94a9eb350c94dfa2eb586bee49c53e0ee886648`. The public commit's tree exactly
matches preserved local implementation `9dc54c3d`. No production source,
fork, compiler controls, physical check, reference calculation or tolerance changes.

## Observation at the existing assertion

In [live_hierarchy_test.cpp](../tests/live_hierarchy_test.cpp), `poll_lifetime`
still calculates the leaf reference once and applies the original `packet_near`
predicate once. Only a rejected comparison calls `dump_leaf_failure`, then
throws the original `live leaf reduction/reference mismatch` error. Success
adds no logging, body snapshot, native operation, allocation or synchronization.
Existing callers pass fixture/phase/round/tick metadata and pointers to body
snapshots they already retain. `-1` denotes a round/tick without that meaning;
poll-only callers without an earlier body snapshot explicitly report its absence.

The logger reads the already completed hierarchy inspection, prepared source
storage and physical host mirrors. It does not poll, inspect or synchronize
again. Records go to stdout, which the existing hosted compiler jobs retain in
`native.txt`. No workflow change or separate diagnostic dispatch is required.

| Record | Captured data |
| --- | --- |
| `leaf_failure_context` | Fixture, phase, round, tick, leaf index, retained precision, comparison identity, full field handle, cohort span |
| `leaf_failure_publication` | Before/after selectors and visit/publication deltas |
| `leaf_failure_packet` | All four before/selected/reference words as raw IEEE f64 bits and hexadecimal floating values |
| `leaf_failure_source` | Ordered cohort index, full occurrence and source handles, exact count/denominator, physical mass bits and derived weight bits |
| `leaf_failure_position` | Each ordered source's three current coordinates and available before coordinates, as bits and hexadecimal floating values |

`native_weight_expression_bits` evaluates `mass * (double(count) /
double(denominator))` from completed inputs. `reference_weight_bits` evaluates
the existing long-double ratio followed by its f64 conversion. These are
**derived expression values**, not native intermediate-buffer readback. Raw
mass and exact integer ratio inputs remain available for independent analysis.
Packet bits are observed selected/reference values, not scalar reproductions.

## Deterministic native rejection and silent success control

The new `--case leaf_failure_diagnostics` uses the investigation's actual native
two-leaf/root witness: body Q8, field Q6, masses `{2,1}`, complete coverage
`{2,1}`, ordered leaf ratios `1/2` and `1/1`, X positions `-0x1.8p-49` and
`0x1.d4p+5` (58.5). It installs and polls through the existing fixture/accessor.

The check invokes the real `poll_lifetime`, captures its diagnostic stream with
a temporary file, and requires the unchanged rejection string. It validates
caller/full-handle context, selectors `0 -> 0`, one visit/no publication, all
four packet words, ordered source handles/weights and body inputs. Selected X
is 29 (`403d000000000000`); reference X is 29.25 (`403d400000000000`). Y/Z are
zero and mass is 2. The existing full `state_hash` must remain unchanged across
the rejected observation.

The captured record is printed between `leaf_failure_control_begin/end` markers
with `expected_rejection=1`, so an expected witness can be distinguished from
an unexpected failure elsewhere. It is an expected rejection, not a passing
instance of the original packet predicate. A separately declared X=0 input
then produces equal selected/reference packets. That successful observation
must add no diagnostic bytes and leave its state hash unchanged.

Four CTests, `leaf_failure_diagnostics_{1,2}_{bulk,direct}`, exercise the default
compiler mode. The existing full native runner also includes this case, so
hosted default regression and no-unroll compiler jobs cover it. The owning
runtime calculation and acceptance gate remain unchanged.

## Local qualification and evidence

All checks below pass on that clean public implementation pin. Both Release
wrappers enable warm integration/viewport and disable CUDA qualification; they
use the matching pinned Clang/LLVM 15.0.7 CPU SDK. GCC 11.5 compiles the second
host wrapper; it does not substitute a different native compiler/runtime.

| Locally executed scope | Profiles/results |
| --- | --- |
| `run_local_ci.sh quick` | Three CTests, placement interval math and placement capacity pass; actual x64 confirmed |
| Diagnostic case | All 16 combinations: Clang 15/GCC 11 wrappers × one/two workers × bulk/direct × default/no-unroll |
| Existing Clang callers | `basic`, `tree`, `publication`, `moving`, `moving_permuted` each pass on one-worker bulk/default and two-worker direct/no-unroll |
| Existing GCC callers | `moving` and `publication` pass on those same two profiles |
| Completed native capture | Clang two-worker direct/no-unroll, two ticks/every tick, passes |
| Output/state audit | All sixteen expected rejections have four packet, two source and six position records; native state unchanged and successful observation silent; no unexpected diagnostic records in caller controls |
| Native/fork boundary | Pass; production, fork and workflows unchanged |

The moving suites retain all 33 observation windows per fixture, ordinary-tick
physical/publication checks, held settling and terminal frozen controls. The
capture exercises its existing completed-state path. No original moving failure
is reproduced or inferred from these successful controls. Post-PASS Taichi
offline-cache lock warnings are retained; no cache was cleared.

The exact argv/stdout/stderr and CTest `LastTest.log` are retained. Principal
commands were `bash engine/scripts/run_local_ci.sh quick
/tmp/hcp-leaf-diagnostics-e107c337-quick`, followed by the retained
`hcp-leaf-diagnostics-qualify.sh` for each wrapper. That script runs the four
registered tests, then `live_hierarchy_test --threads {1,2} [--direct]
--compiler-no-unroll --case leaf_failure_diagnostics` on every worker/path
combination. Its existing-caller controls use one-worker bulk/default and
two-worker direct/no-unroll. No tests were skipped or retried in qualification.

Reversing only the diagnostic declarations/formatter, selftest/dispatch, caller
metadata and leaf logging branch reconstructs the complete parent translation
unit byte for byte. All ten original caller bodies and every numerical helper
are preserved. Production, fork and workflow diffs against #421 are empty.
The reconstruction script, both sources, generated reconstruction and proof
are retained with the evidence.

Archive `/tmp/hcp-leaf-diagnostics-e107c337-evidence.tar.gz`, SHA-256
`84e27f6cde55ab117274f7a9027a4c95d5c89b16eab86ebcf0b592e919fac2c9`,
retains the three clean-pinned runs and manifests, raw diagnostic/control output,
commands/configurations/toolchain/backend, native binaries/runtime bitcode,
source mapping/patch, original/reconstructed sources, proof/audit/boundary scripts
and results, and separately labelled pre-commit development logs. Runtime bitcode
hash is `4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada`.
Fresh extraction verifies the root and all three nested manifests. The raw
bundle is machine-local; the digest does not make it remotely available.
Only Markdown changes follow qualification.

These are local actual CPU/x64 results, not GPU, database, full hosted matrix
or performance qualification. Hosted CI remains authoritative. #421's reconciled
head has smoke passed and native qualification pending at this handoff; this
dependent implementation requires its own maintainer review and hosted gates.
No manual duplicate workflow was dispatched. All merges remain maintainer-only.

Next: reconcile that review and hosted diagnostic output. A recurring failure
can now retain the missing operands. Any reference/error-bound or production
arithmetic correction remains a separate reviewed task; this instrumentation
does not diagnose the original #417 failure or authorize an epsilon/cell allowance.
