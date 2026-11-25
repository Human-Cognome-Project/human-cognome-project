# Repeated parent paths and shared nibble coverage

The `aá` fixture reuses the definition of `a` at parent ordinals 0 and 1.
At aggregate exposure, native admission keeps both exact count-2 rows in one
character field. That field publishes mass 4, and the single aggregate body
publishes mass 8 before and after a tick. The same pinned construction expands
to four ordered byte paths. [#432 assignment](https://github.com/Human-Cognome-Project/human-cognome-project/issues/432#issuecomment-7147550733)

| Character ordinal | Byte path | UTF-8 byte |
| ---: | --- | --- |
| 0 | `[0,0]` | `61` |
| 1 | `[1,0]` | `61` |
| 2 | `[2,0]` | `CC` |
| 2 | `[2,1]` | `81` |

The two `61` paths retain the same exact byte definition revision. Their
occurrence IDs differ by character ordinal. The `CC` and `81` paths retain the
pinned combining-acute parent while their byte ordinals differ.

## Residual coverage conflict

Nibble `1` occurs at nibble ordinal 1 in both `61` occurrences and in `81`.
The planner's field key identifies one nibble definition across the study.
Each projected row contributes exact count 1. The proposed `61` and `81`
whole-byte branches both reference that nibble comparison, and the full
nibble cohort includes paths outside each branch.

| Whole-byte branch | Nibble `1` paths inside branch | Nibble `1` paths outside branch |
| --- | --- | --- |
| `61` | `[0,0]`, `[1,0]` | `[2,1]` |
| `81` | `[2,1]` | `[0,0]`, `[1,0]` |

For every outside path above, the whole-byte field has zero coverage and its
candidate child adds one. The current plan requires exact child coverage for
each occurrence and gives each comparison one parent. The repeated fixture
also lacks a composition root in its fine field inventory.

To separate those two obstacles, the single-scalar control uses `ó` with bytes
`C3 B3`. It uses the same explicit character-root binding as the accepted `í`
fixture, with two ordered child branches and full root coverage. Its one
nibble `3` field still spans the two byte branches. Each branch gets coverage
from the other byte at the shared leaf. Native owner preparation returns
`InvalidInput` for this case as well. The counts above come from the fixture's
projection rows and proposed comparisons. The owner returns a common input
status for the failed plans.
[Existing prepared admission contract](STRUCTURAL-ADMISSION-PLAN.md)

Both refusals use the same empty prepared Harness. The trace records equal
before and after generation, completed tick, readiness, live reference count,
population and hierarchy state. Each failed preparation releases its owner
reservation bytes. The disjoint `í` control then publishes C3 and AD on that
Harness with root mass 4 and completes tick 1. The successful aggregate and
retry are separate studies. [Portable fixture capture](../../tools/viewport/examples/repeated-parent-overlap.jsonl)

The next representation decision is how a comparison identifies its cohort
when one pinned nibble definition appears under two whole-byte branches. The
review case needs to retain the shared definition, distinguish branch and
occurrence paths, preserve exact child totals, and count each physical byte
once. This fixture exposes the branch-local coverage that such a decision
would need to account for. [Prior single-character slice](CHARACTER-ROLLUP.md)

## Qualification

Local commit `218be67db625956360c111e331117a2a142f9887` and public code
`a14a00a924d7241b4b7aad8e939d53f4bbbda5b4` share tree
`4c5b78f0d0f103643030ae96ac61262034f110f1`. This fixture stacks on #435 at
`16e4ffb4`. Its single-character fixed-study acceptance is provisional and
hosted integration still follows the parent order.
[Maintainer review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/435#issuecomment-7164198854)

Eight local CTests pass on actual LLVM CPU with Clang/LLVM 15.0.7. These include
the existing semantic projection and seven-profile trace, both disjoint
character profiles, the two-worker direct overlap case, and three quick CPU
checks. The one-worker bulk overlap command also passes. Both overlap runs
pass 177 assertions. The independent audit validates their coverage rows and
paths, both refusal state comparisons, and the four completed native snapshots
in each run.

```sh
cmake --build engine/build --target semantic_native_trace_test -j 3
. engine/build/engine-env.sh
engine/build/viewport/semantic_native_trace_test --case parent-overlap
engine/build/viewport/semantic_native_trace_test --case parent-overlap --threads 2 --direct
```

The fork tree is `a94a9eb350c94dfa2eb586bee49c53e0ee886648`. Runtime bitcode
SHA256 is `4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada`.
The machine-local archive `/tmp/hcp-parent-overlap-218be67d-evidence.tar.gz`, SHA256 `855292a3219cfa83a718101121fca556253e3109436d4428756e25b2da1b4025`,
contains the guarded source and build pins, exact commands, full logs, both
captures, audit, and incremental source bundle. Its manifest passes after
fresh extraction. The repository capture is the one-worker bulk result.
Authoritative GitHub CI is pending.
