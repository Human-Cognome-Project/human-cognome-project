# Semantic parents and native worked trace

## Scope and source

Source `d9537f1a35e0fea05f5d5da8cc77149edf2ebd92`, tree
`9c83b7d8240d8db90b3e710dbbc1ac25d6046aff`, extends reviewed #431
`164708c5`. Main remains `b528d494`; #430 and #431 are provisionally accepted
but unmerged with native CI pending. This dependency reuses #431's existing
response observer. No integration merge or production numerical change is made.

This is the first bounded worked semantic trace under [#343's handoff](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343#issuecomment-7137040893)
and [the parent/field clarification](https://github.com/Human-Cognome-Project/human-cognome-project/issues/324#issuecomment-7135833675).
The fixture-owned C++ runner records actual source, projection, admission, owner
associations, fields, response operands, diagnostic arithmetic and one completed
tick. Independent scalar references are test assertions; they never supply native
starts, force, targets or motion. The existing private owner performs admission,
placement, publication and ticking. All active bodies use baseline physical mass
equal to exact source mass, with the existing fixture geometry. No retained store
is acquired or changed, and fixture addresses are not production allocations.

## Run and read

With the existing native build configured with `HCP_WITH_VIEWPORT=ON`:

```sh
cmake --build engine/build --target semantic_native_trace_test -j 2
. engine/build/engine-env.sh
engine/build/viewport/semantic_native_trace_test > /tmp/semantic-trace.jsonl
engine/build/viewport/semantic_native_trace_test --projection-only
engine/build/viewport/semantic_native_trace_test --threads 2 --direct --case ascii
```

The full run emits seven `hcp-semantic-native-trace` JSON records. Each records
reachable immutable definition pins, ordered parents and exact counts, memberships,
active/inactive occurrence paths, byte-to-character/endpoint provenance, explicit
layout roles/units and actual declared SNode IDs. Comparison declarations are
explicit fixture choices. SNode IDs and depth do not identify Unicode objects.
The root/optional-primary binding describes the supported layout; it does not
materialize a native object for every semantic character.

Successful plans also record the actual pending participation commands, preserving
parent ordinals and counts. Native `before`/`after` each contain an unchanged
viewport `snapshot`, `field_sources` resolving full native claims back to typed
field keys and published XYZ/mass, and `arithmetic` copied from the native intent
query after independent test assertions. The snapshots are valid against #431's
schema; the containing trace has its own fixture format. Completed ticks are 0/1
in the same owner execution for each admitted profile. Different profiles are
separate executions, not an in-place LoD exchange or a live context toggle.

| Profile / exposure | Active bodies | Fields | Admission |
| --- | ---: | ---: | --- |
| English A + French A / aggregate | 2 | 3 | Accepted by native owner |
| Same pair / bytes | 2 | 3 | Accepted by native owner |
| Same pair + imposed comparison / aggregate | 2 | 4 | Accepted by native owner |
| `í` endpoint / aggregate | 1 | 2 | Accepted by native owner |
| `í` endpoint / bytes | 2 | 6 | Structural plan refuses |
| `aá` endpoint / aggregate | 1 | 3 | Accepted by native owner |
| `aá` endpoint / bytes | 4 | 7 | Structural plan refuses |

`í` has one core Unicode parent with ordered bytes C3 AD. `aá` preserves core
parents `a`, `a`, U+0301 at ordinals 0, 1, 2, with contributions 2, 2, 4. Its byte
paths remain `[0,0]`, `[1,0]`, `[2,0]`, `[2,1]`; the final scalar encodes CC 81.
The repeated `a` definition is shared, while its occurrences and ordinals remain
distinct. The aggregate's two `a` parent commands remain separate rather than
being removed by definition deduplication.

## Fixed-input English/French example

English A and French A are distinct language endpoints with the same pinned core
A parent. At aggregate exposure, each singleton endpoint field is a native leaf;
the common ParentValue(A) field is their parent. The two baseline bodies have
mass two at x=0 and x=-1, and that field publishes mass four at x=-0.5.
Singleton local rows are ineligible. Both inherited endpoint-to-A paths transmit
nonzero force: available shared parentage is not merely assumed to act.

At G=1, for either endpoint, participating mass is `2*(2/2)=2`, source field mass
is four and displacement has magnitude 0.5. Thus coupling is eight,
`alpha = 8 / 0.5^3 = 64`, force is respectively -32 or +32 along x, and the native
diagnostic `x + F/alpha` equals -0.5. This establishes candidate correspondence
to the composition parent's centre for this one-path case; it does not create
general structural/confounded target storage or select an additional target rule.

The imposed cross-language group is attached through explicit endpoint revision
pins; existing pinned definitions are never silently overwritten. Exact parent
pins, structural bodies, mass, geometry, initial native positions, frame, tick,
structural field packets and diagnostic arithmetic must compare identically.
Only contextual metadata changes. The new comparison has the same cohort as its
sole child. Both additional inherited rows have zero displacement and fail the
native complement predicate. They contribute zero, and the candidate remains
-0.5. The runner retains those rows and asserts `fixed_pre_tick_control=passed`.
This is a measured zero-effect control, not a nonzero imposed relationship.

At byte exposure, both separate occurrences reuse core A. Nibble ParentValue
fields are leaves under SiblingDefinition(A), whose rolled-up mass is four,
counting each physical occurrence once. Each endpoint's two local responses
produce total coefficient 32 and force magnitude 16 at the initial positions.
The two eligible inherited rows per body have exact zero displacement. The
existing diagnostic therefore reports **unresolved coincidence**, with null
candidate, while the production tick remains healthy. This status is preserved;
it is not relabelled as an absent force or a completed confounded target.
Coarse and fine field arithmetic must not be assumed identical merely because
the shared literal and root mass agree. Coordinates/roles and publication time
remain explicit in the trace.

## Representation boundary and continuation

The follow-up single-character rollup is submitted for review with a native
publication and tick for `í`. The findings below describe the original seven
profiles. The repeated-parent byte overlap remains unresolved.
[Single-character rollup and evidence](CHARACTER-ROLLUP.md)

[Issue #432](https://github.com/Human-Cognome-Project/human-cognome-project/issues/432)
records the reproduced boundary for maintainer disposition. At multibyte byte
exposure, the projection retains the semantic character pin in byte provenance,
but the active ledger emits only whole-byte sibling fields and those bytes' nibble
parent fields. No field covers all active bytes in full. The literal C3/AD branches
have two roots; the repeated form's three byte-definition branches also share the
ParentValue(nibble 1) field. A different ordering cannot supply a missing complete
root in the existing field inventory.

The current owner requires every field once, one rooted comparison tree, one
parent per comparison and exact per-occurrence child coverage summing to parent
coverage. Both source plans return InvalidInput before native bootstrap. The
independent tree witness records missing full coverage, root indices and shared
children; it is not an invented native failure diagnostic. Valid aggregate
controls still admit and tick. These refusals describe the current subset, not a
permanent requirement that these literal structures be unsupported. Preserve
ordered/repeated paths when choosing the representation extension. No dummy
membership, source relabelling, forced graph deduplication or new physics is used
to bypass the boundary.

The native arithmetic and one observed tick are recorded separately. The settled
[brake reference](https://github.com/Human-Cognome-Project/human-cognome-project/issues/324#issuecomment-7135937435)
is unchanged. Fixed-input brake off/on comparison, general target correspondence,
character/aspect rollup and following these controls through a live whole-field
exchange remain subsequent work. #430's English single-byte whole-field subset
and its eventual response-enabled integration check remain separate.

## Qualification

On the clean public implementation pin above, Ubuntu 24.04, LLVM/Clang 15.0.7,
GCC 11.5.0 and actual native CPU:

- Quick local CI passes three CTests, native build/backend and placement
  math/capacity controls.
- Both compiler profiles pass the projection and full native trace CTests:
  28 source/binding checks and 220 native/source/arithmetic checks per profile.
  Together with quick CI these are **seven CTests**.
- Two additional one-runner/two-worker CPU-direct ASCII controls pass 157 checks
  each, including the fixed pre-tick comparison. Default full runs use one
  worker/bulk readback.
- All **32 fresh native snapshots** across those runs pass #431's existing schema
  and exact field-claim-to-source association audits. No browser suite was rerun;
  observer/schema/bundle implementation is unchanged from reviewed #431.

The [portable seven-case trace](../../tools/viewport/examples/semantic-native-trace.jsonl)
is the qualified Clang full-run output extracted from its CTest log. Its native
snapshots are separate from the surrounding fixture trace metadata. Source body
mass, exact participation, actual claims, packet paths, published field positions,
diagnostic status and before/after motion remain inspectable. For the ASCII pair,
one dt=0.01 tick moves x=0/-1 to -0.0016/-0.9984 at aggregate exposure, identically
with/without the imposed comparison. The byte exposure moves to -0.0008/-0.9992;
these accepted positions are recorded separately from the diagnostic candidate.
Single-body Unicode aggregate controls remain stationary with absent diagnostic.

Taichi tree `a94a9eb350c94dfa2eb586bee49c53e0ee886648` and runtime bitcode SHA256
`4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada`
match the recorded native build. Exact submodule/compiler/configuration,
commands/executable hashes, full logs, snapshots, audit programs and clean-source
HEAD guards are retained in `/tmp/hcp-semantic-d9537f1a-evidence.tar.gz`, SHA256
`d2a2432a9c934db92a8d219f9940e7ebd38ac99b959094fcb422256d59dfbce9`.
The manifest, nested manifests and a fresh archive extraction were personally
verified. The local archive hash does not make its logs remotely available;
the committed trace is portable evidence. Initial fixture focus/geometry-identity
errors and the earlier unguarded development executable are retained and excluded
from current-source qualification. Offline-cache lock warnings remain in logs;
no cache or persistent-store cleanup was performed.

This is local CPU fixture evidence for provisional review. Hosted GitHub CI is
separate and authoritative. No GPU, concurrency/performance, retained acquisition,
general Unicode exchange, full B, allocation policy or complete target/brake
qualification is claimed. Numerical kernels, source/owner implementation and
braking remain unchanged.
