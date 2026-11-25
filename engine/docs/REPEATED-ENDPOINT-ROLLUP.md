# Fixed `aá` endpoint rollup

## Data-driven admission correction

The #438 correction derives the active byte contribution to each ordered
parent of a pinned endpoint. It checks the represented counts per ordinal
and the exact endpoint total before building a native plan. Explicit
comparison fields retain a child order based on the first represented
occurrence of each definition. Repeated source ordinals and byte paths
continue to identify separate contributions. [Current assignment](https://github.com/Human-Cognome-Project/human-cognome-project/issues/432#issuecomment-7172285940)

For a nibble field used by multiple byte comparisons, admission traces each
source row to its pinned byte parent and selects the branch for that whole
byte. An optional selector list checks the derived selections by path and
ordinal. Native refresh independently checks the source claims, exact ratios
and coverage of every branch. Each shared leaf publishes its complete
ledger once. Its unique source rows supply the signed links to the common
reducer. The CPU bootstrap and its existing response modes still govern
execution.

The same test executable admits disjoint `í`, shared-parent `ó`, repeated
`aá` and the changed `abá` recipe. The changed input has four ordered
parent contributions of 2, 2, 2 and 4. It publishes mass 10 through five
distinct byte paths and three character comparisons. The caller supplies
the comparison fields and omits the optional selector list. Two nibble
publishers each retain three rows at bootstrap and the following tick.

| Shared nibble | Byte occurrence paths contributing one count each |
| --- | --- |
| 1 | `[0,0]`, `[2,0]`, `[3,1]` |
| 6 | `[0,0]`, `[1,0]`, `[2,0]` |

Omitting a required nibble link, dropping an endpoint branch or supplying
a stale endpoint revision refuses before publication and releases the
preparation allocations. The same empty Harness then publishes the valid
recipe. This slice covers explicit endpoint and character comparisons over
whole-byte branches with scoped nibble leaves. Selection and editing of
data-driven objects in the existing live CLI is the next milestone.

### Local qualification

The tested executable source is local `cfab62742314862203d046087f813ea8b50c8dc7`
and GitHub `e9a42b64f4ac6064fded92e90026bf2105b3a3eb`, tree
`90be955623b30ecf0a1e3370ef92163e0b5ecbe0`. Main at
`49e3d8dc` includes the #443 guidance. Tests used Clang/LLVM 15.0.7,
native x64 CPU, the Taichi tree and runtime bitcode pinned below.

Nineteen scoped CTests passed, including disjoint and shared-character
controls, both varied-recipe host paths, repeated rollup, ordinary refresh,
capacity and owner submission. The viewport validator passed 30 native
captures. The ordinary LLVM packet audit passed both default and no-unroll
modes. Replay the varied recipe with the existing trace executable.

```sh
cmake --build engine/build --target semantic_native_trace_test -j 4
ctest --test-dir engine/build --output-on-failure -R '^semantic_varied_rollup_(bulk|direct)$'
```

An isolated local carry preview on #442 uses `35860b3f`, tree
`4672e90e708cf9201672541abbb7c20b43bf43f8`. It resolves the shared test
runner and CTest registration conflicts while preserving both sets of
checks. Eight CTests passed there, including the original scoped default
and no-unroll checks, live Unicode exchange, and both CLI host paths.
In that preview the varied recipe passed 371 assertions in each compiler
mode. Each compiler mode rejected the shifted source row and omitted
response controls. The published descendant heads await the maintainer's
review of the #438 correction.

The machine-local archive `/tmp/hcp-data-driven-admission-cfab6274-evidence.tar.gz`
has SHA256 `274de56ca7aaf01c51c151819e545b1d70433b54381ce784ef9cfe27a5db7229`.
Its manifest verifies after fresh extraction. It includes the test output
and captures, ordinary and scoped LLVM modules, source and binary pins,
and the local carry patch. The evidence covers the listed CPU cases.
Full hosted regression and database qualification remain authoritative.

## Original fixed-study evidence

The fixed CPU study publishes the repeated endpoint at both aggregate and byte
exposure in separate native executions. The aggregate's three ordered counts
are 2, 2 and 4. The byte exposure reproduces those counts with four distinct
body claims and one endpoint field of mass 8. [Maintainer assignment](https://github.com/Human-Cognome-Project/human-cognome-project/issues/432#issuecomment-7166806643)

| Character ordinal | Byte path | Byte | Endpoint count | Scoped nibble `1` count |
| ---: | --- | --- | ---: | ---: |
| 0 | `[0,0]` | `61` | 2 | 1 |
| 1 | `[1,0]` | `61` | 2 | 1 |
| 2 | `[2,0]` | `CC` | 2 | — |
| 2 | `[2,1]` | `81` | 2 | 1 |

Both `61` occurrences keep the same byte definition revision and different
paths and full native handles. Their ordinary whole-byte field publishes mass
4. The `CC` and `81` fields each publish mass 2. The character fields for `a`
and the combining acute each publish mass 4, and the endpoint reduces them to
mass 8. The two separate `a` rows stay visible with ordinals 0 and 1 in the
shared character field. The acute keeps two byte paths under ordinal 2.

One global nibble `1` publisher owns three count-1 rows at nibble ordinal 1.
The `61` comparison selects the rows for `[0,0]` and `[1,0]`, while `81`
selects `[2,1]`. These selections reuse the existing rows. The global field
publishes mass 3 and the four physical bodies retain total mass 8. The
prepared owner retains the four body and ten field claims from tick 0 to 1.
[Native capture](../../tools/viewport/examples/repeated-endpoint-rollup.jsonl)

## Inherited response for review

The repeated `61` branch has two body occurrences. A scoped singleton under
that branch can therefore contribute an inherited response under the existing
complement predicate. The experimental response uses the selected byte's
actual position as its child centroid, the owning byte branch's published
centroid and mass as parent, and the original exact ratio 1/2. Both `61`
paths are eligible. The `[2,1]` row remains ineligible under its singleton
`81` branch. The global nibble's ordinary local responses continue to use
the global publisher.

The capture records these rows as `scoped_inherited`. Its child packet
identifies the global publisher, its parent packet identifies the owning
byte branch, and the recipient claim identifies the singleton selected from
the publisher's source rows. The audit checks the actual response operands
and the existing force formula. This is a proposed response interpretation
for the fixed endpoint experiment. General graph and target/brake rules are
separate decisions.

## Boundary and refusals

The fixed CPU setup checks the exact endpoint revision, the 2 + 2 + 4 parent
shape, all four byte paths, the three nibble selectors and the complete
endpoint tree before native publication. Missing, duplicate, foreign-branch
and wrong-ordinal selectors, an incomplete endpoint, and a stale endpoint
revision all refuse with equal before/after Harness state and zero retained
owner allocation. The valid experiment then publishes and ticks on that
Harness. The proof includes the prior `ó`, disjoint `í` and ordinary native
paths as separate regression cases.

## Qualification

Local code `f6bfd32322d09e7ea2c0efe74ba1ee6b37ffa775` and public code
`1a7891150302c6d6377c8680366014aa5b09c040` share tree
`dedd0b27c57525310a02d6e086bc32b88eca056f`. This slice is based on the
provisionally accepted #437 head `72bf6fc9`.
[Maintainer review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/437#issuecomment-7166806156)

Thirteen local CTests pass on actual LLVM CPU with Clang/LLVM 15.0.7. They cover
the new repeated endpoint, prior semantic, character and refusal witnesses,
structural admission, prepared owner response capture, snapshot serialization
and three quick checks. The one-worker bulk and two-worker direct repeated runs
each pass 291 assertions. An independent audit compares both exposures and both
ticks in each profile, including the actual field packets, source rows and
response operands. The ordinary C3 compiler audits and refresh transport
pass in default and no-unroll modes. Scoped compiler-shape qualification
remains open.

The first native publication succeeded with four reserved contact slots and
the candidate tick refused when it needed six. The fixture now reserves all
six possible pairs and keeps the usual native contact validation. The capture
shows native prepared generation going from 1 to 2 after contact coverage
changes. The four body claims, ten field claims and completed source context
stay coherent. The earlier compiler type error and contact refusal are retained
separately from the passing qualification.

```sh
cmake --build engine/build --target semantic_native_trace_test -j 4
. engine/build/engine-env.sh
engine/build/viewport/semantic_native_trace_test --case repeated-rollup
engine/build/viewport/semantic_native_trace_test --case repeated-rollup --threads 2 --direct
```

The viewer model tests and offline bundle rebuild pass. The browser passes
128 retained-fixture checks and loads the two repeated snapshots with their
scoped rows intact. A malformed scoped row with equal child and parent packet
IDs refuses while preserving the completed observation. The browser makes zero
network requests in these runs.

Fork tree `a94a9eb350c94dfa2eb586bee49c53e0ee886648`, runtime bitcode SHA256
`4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada`.
The machine-local archive `/tmp/hcp-repeated-rollup-f6bfd323-evidence.tar.gz`, SHA256 `c02552fa20b372bdc5e03b93b0219574b89afad0f6a407bfa5868c79fb1cf2da`,
contains the clean source guards, compiler, runtime and binary pins, exact
commands, development refusals, raw captures and audits. Its manifest passes
after fresh extraction. The checked-in capture is the one-worker bulk result.
GitHub CI and maintainer review are pending.
