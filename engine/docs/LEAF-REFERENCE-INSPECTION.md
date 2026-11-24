# Hosted leaf reduction/reference investigation

Investigation under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343),
following [#420 decision 7111426753](https://github.com/Human-Cognome-Project/human-cognome-project/pull/420#issuecomment-7111426753)
and [#417 diagnosis 7111421242](https://github.com/Human-Cognome-Project/human-cognome-project/pull/417#issuecomment-7111421242).
Inspected/local execution source: main `3080687a3832ff7c91311f267e261277a32aff8b`,
tree `a0df276df9aaffeaae791ea1c68fe1a3da3bea37`, native fork tree
`a94a9eb350c94dfa2eb586bee49c53e0ee886648`; recursive pins match.
All executable source matches reviewed #417 `0d1fd0a2`. The field, engine,
test/helper and fork sources also match qualified #418 `7831326c`.
This contribution changes documentation only.

**Disposition:** the original artifact identifies a leaf-reference assertion,
but lacks the compared operands. A separate native witness demonstrates that
the current exact Q6 packet comparison can reject two rounding paths for valid
inputs. It does **not** establish the original failing leaf or cause. Preserve
the failed artifact and comparison; add failure-only operand/context capture as
the next proposed slice before choosing a reference or arithmetic correction.

## Personally inspected hosted evidence

| Evidence | Original failure | Passing control |
| --- | --- | --- |
| PR/head | #417 `0d1fd0a2` | #418 `7831326c` |
| Native run | [45220070965, attempt 1](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/45220070965/attempts/1) | [45230551268](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/45230551268) |
| Artifact | [14841453601](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/45220070965/artifacts/14841453601) | [14849284321](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/45230551268/artifacts/14849284321) |
| Artifact source/test merge | `d0f0a89be37ec9a72e7b7957798c9a01f63225a4` | `5a79d7e25a9b322019d4b18986e5ef7e856df4cf` |
| Tree equals reviewed head | `4f9d675488663184a6cf81960d811851d1f7f017` | `68d4e38c029d1a683f30a4b46a1900701b26f2d0` |
| Collector optimizer target | `icelake-client` | `znver3` |
| `2-direct-no-unroll/native.txt` | One case failure, `moving` | Aggregate passes; moving completes all 33 rounds of each fixture |

Both artifacts record LLVM/Clang 15.0.7, actual x64, two workers, CpuDirect,
disabled-unroll test controls and strict C3 math. Their default/no-unroll external
IR audits each pass all three modules. The different optimizer targets are a
recorded limit of the comparison, not evidence that target choice caused failure.
The runtime bitcode hash is identical:
`4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada`.

Downloaded ZIP digests match GitHub's artifact metadata:

- Original: `937de5fb9cf626367ebbf8f2ae2bd535330d8120c207f2cb982f03c04f6496fb`.
- Passing: `c33bd7252e040d49137d99a1f6dbeb57cb87719b575763e5a69e95d9a903277f`.

The original `native.txt:1241` reports `live leaf reduction/reference mismatch`.
Its last completed summary is `three_level_overlap`, round 27, followed by that
round's four body-dynamics rows. Round 27 is evolving after its finite window;
that status is permitted by the [coherent-dynamics contract](../../docs/coherent-dynamics-gpu-snapshots.md).
**Source/control-flow inference:** the mismatch occurs in an ordinary tick of
round 28, before its summary. After round 27's evolving summary, no further
`poll_lifetime` call occurs until the next round; terminal frozen diagnostics
occur only after all 33 rounds. The artifact does not identify the failing tick.

The passing trace already differs in work/tick history at round 1. It completes
round 28 and all later rounds. Neither matching trajectories nor stillness is
required; that pass does not explain or clear the failed leaf invariant.

During final reconciliation, #417's same-head attempt 2 also passed. Its
[retry artifact 14856309877](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/45220070965/artifacts/14856309877)
has digest `ba847c17999412248c0730bb3ce41f04dd8f46d2fa002a58f770286ae82f6cf3`.
It records the exact same test-merge source, native executable hash
`f62072223fb27507ea0414ca63444780f018bcc689a2fa8989cb1f54cb0c2c78`,
runtime bitcode and declared `icelake-client` optimizer target as the original.
Its moving case completes all rounds and the aggregate passes. The first
different timing-excluded three-level round summary is round 0's field-work
count, 35 versus 36. That is a difference in recorded history, not the missing
first divergent leaf. A successful retry establishes current hosted
qualification without diagnosing the original rejection.

## Comparison boundary and missing observation

In [live_hierarchy_test.cpp](../tests/live_hierarchy_test.cpp), `moving_case`
checks physical state, then calls `poll_lifetime` after each completed tick.
That helper first checks visit/publication limits, selector/history and flags.
For an entered leaf it compares the selected packet with `leaf_reference`.
Unlike the parent branch, this leaf branch does not choose between pre/post-poll
child packets. The earlier permitted mixed-parent publication explanation is
therefore insufficient to diagnose this assertion.

`leaf_reference` derives weights from current physical mass and exact source
counts/denominators, casts each weight to f64, then `fold` accumulates weighted
coordinates in long double. It converts the centroid to double before `quantize`.
`packet_near` requires exact XYZ equality whenever retained fraction bits < 52;
its later `4e-12` relative check cannot admit unequal Q6 coordinates.

The native leaf path in [live_hierarchy.cpp](../src/field/live_hierarchy.cpp)
instead accumulates `weight * (position - first_position)` in f64, divides by
the accumulated mass, adds the anchor and applies the destination mask. A
source-equivalent expression over real numbers need not round identically at
each f64 operation before that mask.

Missing from the failed hosted cut: tick, leaf/field identity, precision, before/
after selectors and packets, visits/publications, ordered cohort source handles,
exact ratios, physical masses and body positions before/after the tick. The
round summaries contain extrema/work totals; the bounded moving trace does not
record every tick or print operands at this assertion. It cannot reconstruct
which of the three leaves failed or its actual reference inputs.

## Constructed native boundary witness

A temporary C++ driver reuses the pinned test's `Fixture`, native installation,
poll, `leaf_reference` and original `poll_lifetime` assertion. It uses two leaves
and a parent, with complete root coverage `{2,1}`. Leaf 0 has:

| Ordered body | Physical mass | Source count / denominator | Effective mass | X |
| --- | --- | --- | --- | --- |
| First | 2 | 1 / 2 | 1 | `-0x1.8p-49` |
| Second | 1 | 1 / 1 | 1 | `0x1.d4p+5` (58.5) |

Both positions are representable by the fixture's Q8 body profile. Destination
precision is Q6; Y/Z are zero. The other leaf supplies the remaining half of the
first body, preserving the existing full source/coverage checks. One targeted
native leaf poll enters once and publishes no new packet; the retained packet
already equals that native calculation. Installation's production IR audits pass.

| Measured/reproduced quantity | Anchored f64 path | Independent host path |
| --- | --- | --- |
| Strict C++ scalar reproduction before Q | `0x1.d3fffffffffffp+4` | `0x1.d4p+4` |
| Native selected X / original `leaf_reference` X | `0x1.dp+4` (29) | `0x1.d4p+4` (29.25) |
| XYZ remaining words / packet mass | 0, 0 / 2 | 0, 0 / 2 |

The pre-Q values in the first row are a separate scalar reproduction of the
source expressions, not a native unquantized-buffer readback. The selected and
reference packets in the second row are observed from the actual native owner
and original reference. In the anchored expression, subtraction rounds away the
small first coordinate before division, then adds it back afterwards. The host
path rounds the long-double centroid to f64 once. Their one-f64-ULP difference
straddles a Q6 boundary; the resulting packets are one 0.25 cell apart.

All eight combinations of one/two workers × bulk/direct × default/no-unroll
produce those packets and trigger the original assertion. The witness catches
that exact rejection and exits successfully only after checking the predicted
packets and unchanged comparison. This is an expected counterexample, not eight
passing instances of the original predicate and not a repaired acceptance gate.
It selects neither a replacement centroid algorithm nor a wider tolerance.

## Local controls, evidence and next slice

A fresh minimal Release CPU build at the inspected main uses Clang/LLVM 15.0.7
and the matching pinned SDK, with warm integration/viewport/CUDA qualification
off. The unmodified runner passes `--threads 2 --direct --compiler-no-unroll
--case moving`, including both fixtures and their 33 windows. A temporary
failure-diagnostic translation unit passes the same focused case. Neither run
reproduces the original moving failure. Reversing its diagnostic/site-capture
insertions recovers the entire original translation unit byte for byte.

The boundary witness is run as `live_hierarchy_boundary WORKERS PATH UNROLL`;
its recorded commands cover workers `1,2`, path `bulk,direct` and unroll
`default,no-unroll`. All probe sources, generators, compiler/link argv and raw
stdout/stderr are retained. Post-PASS offline-cache lock warnings are retained;
no cache was cleared. Native/fork boundary checks pass. Exact executable-source
equivalence permits reuse of verified #416 quick evidence at `9c3f03a8`; no
accepted quick/full native baseline was rerun merely for this investigation.

Archive `/tmp/hcp-leaf-reference-3080687a-evidence.tar.gz`, SHA-256
`891698f5d7a1437db8b83bde04d5d09cb4193dc50141afcb790ebf28de0bccab`,
retains both original ZIPs, metadata/digests and selected texts, source/tree and
recursive pins, minimal build configuration/logs, two moving controls, eight
native witnesses, scalar reproduction, reconstruction proof and verified reused
quick evidence. Fresh extraction verifies root and reused-quick manifests. Raw
evidence remains machine-local; this report distinguishes personally inspected
hosted artifacts from locally executed probes. No GPU, database or performance
qualification is claimed.

Completion archive `/tmp/hcp-leaf-reference-3080687a-hosted-completion.tar.gz`,
SHA-256 `68653f4366dbbd8b1d223c8c7b078bf702436896fc14da4b38cb4d6c5d59a9e1`,
adds the personally inspected successful retry ZIP, final hosted status, exact
source/binary comparison and local runtime command lines. Fresh extraction
verifies its manifest. The initial archive's earlier CI snapshot is preserved;
the completion record updates status without overwriting original evidence.

**Proposed next bounded PR:** add failure-only capture at the existing leaf
comparison and pass moving fixture/round/tick context to it. Read the already
completed inspection and body mirrors; record all four packet words and ordered
source inputs in hex, then throw the same assertion. The constructed witness
can exercise that path deterministically. Preserve the success path, exact
predicate, physical checks, activity and publication protocol. This supplies the
observation needed for the next hosted failure without guessing its cause.
Any subsequent reference/error-bound or production-arithmetic proposal needs
the actual operands and separate reviewed controls; a blanket epsilon or one-cell
allowance is not justified by this inspection.

At handoff main remains `3080687a`. #417–#420 are source-accepted and now pass
hosted smoke/native qualification, with #417 qualified by its maintainer-requested
same-head attempt 2. All four remain unmerged. Attempt 1 and its artifact remain
failed evidence; original cause is unresolved. Disposition of the prior
integration hold and all merges remain with the maintainer. No extra hosted
rerun was dispatched here. Return this inspection for review and authoritative
CI; numerical/model changes under #324 remain separate.
