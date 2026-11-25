# Scoped nibble contributions for the `ó` rollup

The fixed CPU experiment admits `ó` at byte exposure using its existing `C3`
and `B3` definitions and one global nibble `3` field. Each whole-byte branch
selects the exact low-nibble row for its own occurrence. The shared field
publishes both rows and both branches retain their full byte coverage.
[Maintainer assignment](https://github.com/Human-Cognome-Project/human-cognome-project/issues/432#issuecomment-7164713699)

| Whole-byte branch | Byte occurrence | Nibble ordinal | Scoped count | Byte denominator |
| --- | --- | ---: | ---: | ---: |
| `C3` | `[0,0]` | 1 | 1 | 2 |
| `B3` | `[0,1]` | 1 | 1 | 2 |

The prepared binding supplies the branch comparison, study-bound byte path
and ordinal. Admission resolves the pinned byte definition and the matching
participation command. The live builder resolves the same source rows again
against the prospective ledger and checks that each singleton row belongs to
its branch. Both links select distinct rows of one complete global publisher.
The native reducer reads the selected row's exact ratio and current body for
the branch's contribution. The other nibble of that byte is an ordinary
singleton leaf over the same body. Its existing publication and activity path
continues to drive the byte comparison. The character root reduces both
whole-byte fields through the existing calculation.
[Structural plan](../src/assembly/structural_admission_plan.cpp) · [Native preparation and candidate reduction](../src/field/live_hierarchy_refresh.inc)

The complete native nibble field has the original two count-1 source rows
and mass 2. It continues to publish a global centroid and its global local
response rows. Each byte field publishes mass 2, the character root mass 4,
and the two physical body claims total mass 4. This happens with six field
claims and eight participation rows. Claims and exact row mappings persist
through completed tick 1. The scoped links contribute to centroid reduction
in this two-singleton case. A byte parent with one body stays ineligible for
an inherited nibble response under the existing singleton predicate. This
experiment leaves target and brake policy open.
[Portable native result](../../tools/viewport/examples/scoped-nibble-rollup.jsonl)

Removing a selector, duplicating a path, swapping the two paths across their
branches, or naming an invalid ordinal returns `InvalidInput`. The Harness
stays ready with zero live references and zero retained owner allocation.
The valid `ó` request then publishes and ticks on that same Harness. The
disjoint `í` case publishes and ticks through ordinary child comparisons.

This scope covers initial fixed CPU admission with two singleton byte branches,
one shared nibble publisher and one ordinary sibling leaf per branch. An
explicit preparation mode selects this reducer at kernel generation time. A later
slice can apply the source-row rule to `aá`, with a complete endpoint rollup
and explicit handling of both `a` occurrences. General graph comparisons,
structural edits and live exposure changes are separate decisions.
[Preceding overlap witness](REPEATED-PARENT-OVERLAP.md)

## Local qualification

Local code `28150082a6cf2296968cef0fb8bf33ff4d9ba147` and public code
`6d9c98a8df2fda01727629d540424c126f9bfbaf` share tree
`10dcb6fbb493264d156248f6cf482705a4793b43`. This experiment is based on the
provisionally accepted #436 head `e5744bc0`.
[Maintainer review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/436#issuecomment-7164713155)

Eleven local CTests pass on actual LLVM CPU with Clang/LLVM 15.0.7. They cover
the scoped overlap, the original semantic and refusal traces, both ordinary
character cases, structural admission, direct prepared-owner bootstrap and
three quick CPU checks. The scoped experiment passes 228 assertions in the
one-worker bulk and two-worker direct profiles. The standalone two-worker
direct ordinary refresh-transport control and compiler audit pass in default
and no-unroll modes. The capture audit
identifies each scoped and global row, verifies native field and body claims,
compares refusal states, and validates the completed snapshots. At bootstrap
the distinctly placed bytes supply their own branch centres, and the global
nibble and character root both have centre x = −0.5. The captured bulk tick
moves the bodies from x = 0 and −1 to x = −0.002 and −0.998 respectively.

An initial revision emitted the scoped branch in the ordinary kernel. The
ordinary compiler audit rejected that revision's packet-load shape. The
updated preparation mode emits the scoped reducer only for the experiment,
so ordinary preparation continues to emit its audited packet reducer. The
earlier failure is retained separately from the passing evidence. The scoped
reducer has functional CPU qualification in the two profiles above. Extending
the packet compiler audit to scoped reductions remains separate work.

```sh
cmake --build engine/build --target semantic_native_trace_test -j 4
. engine/build/engine-env.sh
engine/build/viewport/semantic_native_trace_test --case scoped-nibble
engine/build/viewport/semantic_native_trace_test --case scoped-nibble --threads 2 --direct
```

Fork tree `a94a9eb350c94dfa2eb586bee49c53e0ee886648` uses runtime bitcode
SHA256 `4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada`.
The machine-local evidence archive `/tmp/hcp-scoped-nibble-28150082-evidence.tar.gz` has SHA256
`d56d058a83889cd70e103b7ef2e810b00892ab3a8b009bba087d83c565eeaafc` and its manifest passes after fresh extraction. It contains the
source bundle, exact commands, compiler and runtime pins, logs, raw captures
and audit. The checked-in capture is the one-worker bulk result. Authoritative
GitHub CI and maintainer review are pending.
