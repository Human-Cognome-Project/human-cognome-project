# Live Unicode character exchange

The first live Unicode profile uses one `í` endpoint and the disjoint
`C3 AD` character rollup. It prepares both exposures from the same sealed
recipe. `ActiveSession::lod_prepared` publishes aggregate → byte → aggregate
in the original session and study. The fixed cohort uses the two immutable
plans for its published occurrence map. Presence edits return unsupported.
[Maintainer assignment](https://github.com/Human-Cognome-Project/human-cognome-project/issues/432#issuecomment-7169139713) · [Character rollup](CHARACTER-ROLLUP.md)

The aggregate and byte plans share the root-relative layout and the
`ParentValue(í)` focus field. Preparation validates the declared aggregate,
both ordered byte paths and full definition pins, the exact masses, and the
cross-exposure address relationship. The same grouped native placement
operation stages both directions. The expanding group reads the aggregate's
current state. Compression reads the two current byte states. The existing
native reduction and balance check preserve the group's mass and momentum.

| Completed state | Ordered occurrence paths and counts | Character field mass |
| --- | --- | ---: |
| Aggregate bootstrap | `[]` → 4 | 4 |
| Expand and following tick | `[0,0]` → 2, `[0,1]` → 2 | 4 |
| Compress and following tick | `[]` → 4 | 4 |

Expansion claims two new bodies before the original body is retired.
Compression claims one new aggregate before the bytes are retired. Both
directions peak at three live particle records and eight field records. The
character field keeps its full native claim and focus across the exchange and
ticks. The restored aggregate keeps its occurrence key and address with a new
native incarnation. The owned study handle retains the same session, study,
and logical generation. Each exchange advances the native generation once at
the existing completed tick. The trace records a following native tick after
each exchange.

The test reserves three particle addresses and three particle records. It
checks native inherited-response rows against the represented byte paths and
exact coverage, along with the physical mass. The short preparation-work
refusal leaves the completed numerical state, native generation and claims
intact. The independent capture audit rechecks ordered coverage and the exact
native claims through the recorded ticks.
[Two-worker direct capture](../../tools/viewport/examples/live-unicode-exchange.jsonl)

## Separate scoped compiler coverage

The `ó` fixture uses `ScopedNibbleBootstrap`. Its retained seed and poll
IR now pass both default and no-unroll profiles and reject a shifted scoped
source row in each. Ordinary poll packet checks still apply. The existing
`aá` witness continues to check the scoped inherited force and wake paths
in `ScopedEndpointBootstrap`.
[Compiler audit](SCOPED-HIERARCHY-COMPILER-AUDIT.md)

## Qualified source and evidence

The implementation matrix runs at local `a7b2ba04782c0912ed94250b7cc8daa55e7439a5`
and GitHub `1259ae5cd5470dee5ccfab7e9c3c96b1d371bf55` with tree
`3077bdbcbd56748bbfff34be35117bf2974b66f9`. The additional three-address
qualification runs at local `2a085d204678b5fd13ca1bd25a490f15c55090d4` and GitHub
`8162528bd71dca052399c79b6a5313587e3feb04` with tree `b1cde9e60e2ec0e7f746f95d44eea8c1ec3bc640`.

Twenty-two selected local CTests pass on the implementation commit. They
cover the two exchange modes, both `ó` and `aá` compiler profiles,
the disjoint and overlapping character controls, all four English
whole-field profiles, prepared owners and occurrence edits, viewport
captures and CPU smoke/strict math. The final tight check compiles the two
changed translation units from the exact commit with the same build flags
and links the qualified libraries. Its two CTest profiles pass all 76
assertions each. Both `ó` compiler profiles pass 232 assertions each.

The runs use Clang/LLVM 15.0.7 on native x64 CPU, fork tree
`a94a9eb350c94dfa2eb586bee49c53e0ee886648` and runtime bitcode SHA256
`4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada`.

```sh
ctest --test-dir engine/build --output-on-failure -R '^semantic_unicode_exchange_(bulk|direct)$'
ctest --test-dir engine/build --output-on-failure -R '^semantic_scoped_nibble(_no_unroll)?$'
```

The machine-local archive `/tmp/hcp-live-unicode-exchange-evidence.tar.gz`
has SHA256 `1e340cc209904358e42d625789bf624368e1d6d7442a14a9cb983109c7fd796d`.
It includes source and binary pins, commands, JUnit output, the retained `ó`
LLVM modules, capture audits, and the final bulk and direct observations.
Its checksums pass after fresh extraction.

This scope covers the explicit disjoint two-byte character profile on LLVM
CPU. Live exchange of the repeated `aá` endpoint and general Unicode cohorts
remain open. Hosted CI remains authoritative and integration follows the
reviewed parent order.

## Scoped audit carry-forward

#440 includes the [#439 lane-aware compiler correction](SCOPED-HIERARCHY-COMPILER-AUDIT.md#hosted-default-audit-correction).
Six focused native CTests pass on local `95c30e00` and GitHub `5007bd2b`,
tree `c0b387aac5fa7d11feb0858f46f3f8d95714d16a`. The `aá` and `ó`
checks pass under default and no-unroll with their negative controls. The
three-address `í` exchange passes in bulk and direct mode with 76
assertions each. The machine-local CTest bundle
`/tmp/hcp-440-lane-audit-evidence.tar.gz` has SHA256 `8ae6f5860da6fec9ca394b483a12923fe0fb648582b00b8d725caaf883c2c048`.
Hosted CI and parent-first integration are pending.
