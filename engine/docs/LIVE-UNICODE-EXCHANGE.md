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

## Live CLI qualification

`viewport_live_study --unicode-i` selects the fixed disjoint cohort in the
existing operator session. The CLI and the native round-trip test call the
same prepared submission builder. The operator uses the existing CLI tick
parameters and completed-state capture. Both directions keep the same
session, study, logical generation and character field claim. The CLI
test observes both exposures and their response traces, refuses `add 20`
and `remove 2` while expanded, and completes ticks at 3 and 5.
[Operator commands](../../docs/onboarding/OPERATOR.md#change-the-live-í-studys-level-of-detail)

The executable source is local `c566fd7d2232ff59768c15b13e7d2415aa6555ca`
and GitHub `84f2258bc95abc8e232712079089ae4797209262` with matching
tree `6c5d22df71f531bfb10bf8941ac5bed8091c96ca`, based on #440
`307511a8726e915c05287c4de4942433aba82465`. The run uses Clang/LLVM 15.0.7
on x64 CPU and the Taichi tree and runtime bitcode pinned above.

```sh
ctest --test-dir engine/build --output-on-failure -R '^viewport_unicode_i_(bulk|direct)$'
```

This runs the new CLI profiles with one worker in bulk and two workers in
direct mode. Twenty selected local CTests passed, including both new CLI
profiles, both native exchange profiles (73 assertions each), the editable
English and whole-field CLI cases, character and scoped rollups, native
trace and projection, snapshot and CPU smoke. The viewport model test
passed and the validator accepted all 28 CLI snapshots from bulk, direct
and standard input runs.

The machine-local archive `/tmp/hcp-live-unicode-cli-c566fd7d-evidence.tar.gz`
has SHA256 `8fa8fa292462dbe79cef52144c2b68c74be0815826d3a56374b32ac6119e345d`.
It contains the commands, configuration, CTest and capture validation
results, binary hashes and complete JSONL records. Its checksums pass
after fresh extraction.

The full regression suite, no-unroll compiler matrix, PostgreSQL,
CUDA and browser runs fall outside this local scope. Hosted CI remains
authoritative.

## Observe a round trip without advancing the tick

At #441 head `0522268b36de22e7d1d564391eb0037bbcb9ca40`, expand the
fixed `í` study, advance two ticks, then compress and immediately expand
again. Read the completed tick and native generation on each JSON line.
The second pair of LoD commands stays at tick 2 and publishes two new
generations. [Maintainer observation handoff](https://github.com/Human-Cognome-Project/human-cognome-project/issues/432#issuecomment-7171674217)

```sh
. engine/build/engine-env.sh
engine/build/viewport/viewport_live_study --threads 2 --direct --unicode-i \
  --response-work-limit 1000000 > /tmp/unicode-i-roundtrip.jsonl <<'EOF'
lod parts
tick 2
lod aggregate
lod parts
tick 4
lod aggregate
tick 1
quit
EOF
```

| Completed command | Tick / native generation | Body x in occurrence order | Focal x | Response rows / eligible |
| --- | --- | --- | ---: | ---: |
| Create aggregate | 0 / 1 | 0 | 0 | 2 / 0 |
| Expand | 0 / 2 | 0, −1 | −0.5 | 10 / 2 |
| Tick 2 | 2 / 2 | −1.603175, 0.603175 | −0.5 | 10 / 2 |
| Compress | 2 / 3 | −0.5 | −0.5 | 2 / 0 |
| Expand | 2 / 4 | −0.5, −1.5 | −1 | 10 / 2 |
| Tick 4 | 6 / 4 | −2.291875, 0.291875 | −1 | 10 / 2 |
| Compress | 6 / 5 | −1 | −1 | 2 / 0 |
| Tick 1 | 7 / 5 | −1 | −1 | 2 / 0 |

Coordinates are in the capture's unchanged operative frame. At tick 2 the
two byte velocities are approximately −0.444219 and +0.444219 along x before
compression. The aggregate receives the group's mass-weighted velocity, zero
in this pair.
Re-expansion restores `C3` and `AD` at the same ordered occurrence paths with
fresh native claims, new starting positions and zero velocities. The two
exchanges preserve total mass 4 and the complete character-field claim.
The −0.5 change in focal x on re-expansion occurs at the same completed tick.
Native grouped placement claims fresh starting positions around the outgoing
group centre and checks balance. [Transaction and placement contract](WHOLE-FIELD-LOD.md#one-native-transaction)

At byte exposure the two eligible rows are inherited responses from byte
comparisons 104 and 105 to character comparison 106, each with exact coverage
2/2 and source mass 4. Adding each row's delta to its recipient position
recovers the current focal position in this fixture. The existing field-only
diagnostic reports that position for both byte bodies. At aggregate exposure
both response rows are ineligible and the single body's diagnostic position
is null. The comparison inventory, raw response and diagnostic each carry
their own meaning. The fixed-input structural/confounded comparison and brake
correspondence belong to [#324](https://github.com/Human-Cognome-Project/human-cognome-project/issues/324).
[How to read response captures](COMPLETED-RESPONSE-CAPTURE.md#reading-the-payload)

All eight snapshots passed the viewport validator. A capture audit verified
tick and generation order, study and field identity, ordered paths, exact
masses, full response claims and packet provenance, and total momentum within
`1e-12` of zero in each axis. The audit used two-worker direct x64 CPU
execution with Clang/LLVM 15.0.7, the #441 executable and runtime pins above,
and Node 24.11.1. The machine-local archive
`/tmp/hcp-441-operator-observation-evidence.tar.gz` has SHA256
`6f4fd5c784d76873088e1bd740e75be74ffc0dee25804c0f262e4267bf53fb71`. It contains the original JSONL, commands, audit and toolchain
pins. The manifest verifies after fresh extraction. This result covers the
fixed disjoint cohort and one local direct CPU run. Hosted qualification of
the operator implementation stays with #441.
