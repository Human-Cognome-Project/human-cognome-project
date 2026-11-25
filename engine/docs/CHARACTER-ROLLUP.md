# Native rollup for one Unicode character

The byte exposure of the `í` fixture now admits two physical byte occurrences
under a native `ParentValue(í)` field. The inactive language endpoint's single
core character pin resolves to `C3 AD`. The two active paths `[0,0]` and `[0,1]`
retain their distinct occurrence IDs and byte definition revisions.
[Issue #432 assignment](https://github.com/Human-Cognome-Project/human-cognome-project/issues/432#issuecomment-7147550733)

## Preparation and publication

The private admission request names the inactive aggregate occurrence and exact
core character revision. The plan checks those pins against the sealed active
projection and requires the complete ordered byte cohort. Each byte adds one
full-share row to the character field, with its byte ordinal and the count from
the validated source definition. The ordinary nibble and whole-byte rows still
participate. The root binds the whole-byte child fields in source order, and
existing hierarchy validation checks their exact per-occurrence coverage.
The native owner places and publishes both byte bodies, constructs the fields
and advances the tick through the existing Harness path.
[Admission builder](../src/assembly/structural_admission_plan.cpp)

| Native field or object | Exact source coverage | Physical mass in the fixture |
| --- | --- | ---: |
| C3 body and whole-byte field | `[0,0]` contributes 2 | 2 |
| AD body and whole-byte field | `[0,1]` contributes 2 | 2 |
| Character root field | Ordered child coverage 2 + 2 | 4 |

The native ledger contains two body claims. Their total mass is four. The
character field represents their rollup and has its own field claim. Both body
claims and all seven field claims persist from completed tick 0 to tick 1 in
the same logical generation. The capture retains exact source revisions,
participation commands, field and body claims, comparison packets and response
observations for both completed states.
In the one-worker direct capture, the character centre starts at x = −0.5.
Native response records both inherited whole-byte paths with exact ratio 2/2.
The following tick moves C3 from x = 0 to −0.0016 and AD from x = −1 to
−0.9984. These are completed positions for this fixture.
[Portable native trace](../../tools/viewport/examples/character-rollup-trace.jsonl)

The recipe focuses an ordinary byte field. The fixture selects the prepared
character root through the existing active focus override. Preparation refuses
stale character revisions, foreign occurrences, incomplete or reordered
children, and a root declaration without the explicit rollup request. These
inputs return no plan and release the reservation domain. The fixed rollup
request also refuses an editable workspace before native publication.

The current comparison tree admits this single-character cohort. The repeated
`a`, `a`, combining acute example in the existing trace still has a shared
nibble field across byte branches and returns `InvalidInput`. Its separate
parent ordinals and byte paths remain present in the trace. Resolving that
overlap and qualifying live changes in exposure are subsequent work.
[Seven-profile source and overlap trace](SEMANTIC-NATIVE-WORKED-TRACE.md)

## Local CPU evidence

The clean local commit `4f0ddd3d58938b1d2047bb6ba040d574795871d5`
and public code commit `9f8226fa981a4c881c9e05cdfc00e74f53ef6605` share tree
`d3d482460676f833f788765e55b3ea19dc668a65`. That source builds on the
maintainer-reconciled #433 head `cbfd7177`, which includes #431 and #430 in
dependency order. Local main was pulled to `1f060ddd` before selecting that
reviewed dependency. The reconciled parents still require hosted integration.
[Dependency #433](https://github.com/Human-Cognome-Project/human-cognome-project/pull/433)

On native CPU with Clang/LLVM 15.0.7, seventeen CTests pass. They cover the
structural plan, four prepared owner and four whole-field worker/readback
configurations, the combined whole-field and response-capture CLI, the existing
seven-profile projection and native trace, two new character tests, and three
quick CPU checks. Two additional direct invocations cover the other worker and
readback combinations. Each of the four character profiles passes 99 checks.
The snapshot audit checks both states in each profile and resolves the character
and child packets to actual claims, ordered byte paths and exact coverage.

```sh
cmake --build engine/build --target semantic_native_trace_test -j 2
. engine/build/engine-env.sh
engine/build/viewport/semantic_native_trace_test --case character-rollup
engine/build/viewport/semantic_native_trace_test --case character-rollup --threads 2 --direct
```

The fork tree is `a94a9eb350c94dfa2eb586bee49c53e0ee886648` and the staged
runtime bitcode SHA256 is
`4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada`.
The local archive `/tmp/hcp-character-4f0ddd3d-evidence.tar.gz`, SHA256 `06f4596890ba5db17ed68ac47a84ced9046e994503fc3a1aceff9ef5d0de13bc`,
retains source and binary pins, exact commands, test logs, audit output, the
incremental source bundle and failed development attempts. The first focus
fixture was refused by warm validation and the first active focus correction
needed the owning input-text type. Both failures are outside the passing pin.
The archive manifest verifies after fresh extraction. The checked-in capture
is the one-worker direct output and can be reviewed without the local archive.
Authoritative GitHub CI and maintainer review are pending.
