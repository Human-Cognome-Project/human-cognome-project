# Synchronous current maps for the private exchange owner

Status: contributor implementation, 2026-11-17, awaiting review and authoritative
GitHub CI. Tested public source: `4f74ad11853cf4ef01f41b6763ef496db53fbc58`,
tree `2ed1b7dfb8c4d77c98be40ae9173948759c137ca`. Documentation follows in a
separate commit. This implements the bounded removal authorized by
[#365 review 5946869103](https://github.com/Human-Cognome-Project/human-cognome-project/pull/365#pullrequestreview-5946869103),
following the [source-pinned inspection](EXCHANGE-BINDING-MAP-COST.md).

Main `068fe63a4160f36496052a59bccf3972e5f67642` and source-accepted #365
`80922b92b38b56bf096729896d375fc77a112c65` are preserved by normal merge
`bd4d165805cc090875f7a90cee14c0f5dfd5d84b`. That dependency also preserves
source-accepted #364 `45d30de3063724925953e88b2f87efe2b03c3e12`. Neither dependency
was integrated on main at source freeze. Their hosted gates and integration are
separate from this implementation. No duplicate workflow was dispatched.

At the handoff snapshot, GitHub reports native workflows 44166963726 (#364)
and 44177130927 (#365) completed successfully. Both dependency PRs remain open;
their integration and this implementation's review/hosted gates are separate.
The archive retains those remote status snapshots.

## Ownership and publication

The private persistent `ExchangeOwner` passes its current occurrence and field
maps as const arguments to capture, command preparation and immediate precommit
validation. Header staging still invalidates the binding and checks/shares the
selected declaration, operation target and membership catalogue. No span, raw
loan, map reference, new member, wrapper or registry survives a call.

The binding's owned map members remain empty on this path. Its independent
captured masses, outgoing full handles, source-edge facts and masks remain
reserved and are rebuilt for each operation. Capture derives validation work
from the actual argument inventories, then validates those inventories against
the captured generation, exact source masses, full native headers and source
ledger. A constant size-consistency check protects captured-mass indexing.
Semantic association, full pins, direction and all four handle components retain
their existing checks. No captured generation is refreshed during execution.

The coordinator keeps its independently reserved candidate maps. Commands copy
the actual current survivors/fields and construct incoming associations there.
On native success, actual resolved claims replace the candidate's incoming
handles. The source-argument call returns before the owner swaps the candidate
maps directly into its current maps, together with projection, reports and
generation under the existing exclusive operation. Candidate storage then holds
the complete previous current associations. The old captured binding is stale.
There is no intermediate binding-map publication.

The standalone value-taking binding/coordinator path still stages, captures and
owns independent maps. Its execution wrapper uses those maps as synchronous
source arguments, then publishes the resolved candidate into its owned maps
after that call returns. Caller projection/declaration/map/incoming copies may
die before validation or execution. Both publication modes use the same native
admission, source revalidation and actual-claim resolution.

Ordinary preclaim refusal preserves current maps and allows retry. Native
postclaim failures, including failure point 7 before mapping resolution and
failure point 8 before outer owner publication, still make the harness unready.
An unready harness never advertises old maps as a healthy new generation.
No native kernel, claim mechanism, source accounting or tick rule changes.

## Work and census

Only the persistent binding-map reservation and per-operation row/key copies
are removed. Header staging retains its fixed direction/association unit.
Capture, commands and execute still price the actual complete source inventory;
an independent standalone binding reports the same validation reservation.
The empty owned-map validation adapter cannot authorize the persistent owner.

Input admission and its component copy-work prepayment still price one complete
initial owner snapshot. Adoption's temporary standalone validation bindings and
their work/backing remain; this contribution does not remove that surface.
The recursive census still visits all map members, counting actual backing if
present. Four stores have backing on the persistent path: current and candidate
occurrence/field maps. Six map headers remain embedded across owner, binding and
coordinator; the empty binding headers are included in its object size once.

## Actual before/after measurements

Historical native binaries at `120658ca6c713b5fd5e0e75bb8aec015ee9954db` were
verified against the retained #365 manifests before execution. Both historical
and new binaries execute the whole-owner request probe and map-attribution probe
under Clang 15 and GCC 11, at one-worker bulk and two-worker direct. Each probe
constructs the real owner, destroys caller input copies, measures allocations,
and checks teardown. These are actual executions of both implementations, not
candidate-cost subtraction.

| Fixture selections | Construction allocations before → after | Cumulative requested bytes before → after | Retained / scoped peak requested bytes before → after |
| --- | --- | --- | --- |
| 1 | 2,899 → 2,337 | 388,510 → 341,330 | 289,584 → 242,404 |
| 4, mixed | 5,306 → 4,744 | 736,420 → 689,240 | 341,816 → 294,636 |
| 3, equal alternatives | 4,265 → 3,703 | 594,250 → 547,070 | 305,194 → 258,014 |

All four compiler/path configurations agree for all three fixtures. Whole-owner
and independent map-attribution probes agree on construction requests and live
backing. The archive retains the parsed comparison and all raw observations.

Each fixture removes 562 successful construction allocations and 47,180
requested bytes. Attribution finds zero persistent binding-map backing, while
current and candidate pairs each retain 47,180 bytes across 562 requests,
including inactive constructed slots (140 occurrence and 140 field slots per
pair). Their 94,360-byte total matches the independent map census. This is a
fixed-capacity fixture result, not a per-live-body or general production estimate.

The allocation window includes setup temporaries. Its maximum live requested
bytes equal final retained requests for these fixtures before and after. Released
setup requests, represented by cumulative successful request bytes minus final
live requests, remain 98,926 / 394,604 / 289,056 bytes; they are cumulative released
requests, not a simultaneous peak or a count of just temporary validators.
Selected-domain attribution, every admission/allocation failure and final
shared/weak/bootstrap cleanup remain separately checked.

Actual before/after ABI probes with both compilers report unchanged owner,
coordinator and binding extents of 608, 680 and 376 bytes. The binding's two empty
map headers occupy 112 bytes inside that 376-byte request; no header saving is
claimed. LLVM signatures confirm two explicit const-map pointer arguments to the
common private calls. With eight-byte pointers their argument representation is
16 bytes, with no separately allocated view or stored wrapper. This is not a
whole stack-frame or peak-stack measurement. Earlier relocated-header and missing
LLVM-link probe failures are retained alongside the corrected probe; production
source was not changed for those diagnostic failures.

Requested backing, retained capacity, cumulative allocation work, logical work
and scoped peak overlap and must not be added together. Allocator metadata, RSS,
whole-process/stack peaks, native pool capacity and timing are outside this claim.

## Controls and qualification

Map-attribution probes destroy temporary source argument maps immediately after
capture, then validate through the owner's unchanged current maps. Default owner
cases independently prepare standalone owned bindings after caller-source death,
compare complete keys/handles and validation work, and reject the empty owned-map
adapter on the persistent path. Prepublication snapshots verify distinct current/
candidate stores. Postpublication checks verify direct swaps, exact retired
source values, actual native claims, coherent generation and stale captured
authority. Existing source/ledger/direction/refusal tests remain enabled.

| Local scope on the exact clean tested source | Result |
| --- | --- |
| Quick native build/backend, three CTests, 632 independent placement-math samples and placement capacity | Passed |
| Focused native CTests | 16 Clang and eight GCC passed |
| Full single/mixed/equal owner matrix: both compilers × 1/2 workers × bulk/direct × default/no-unroll | All 48 passed |
| Full standalone binding/commit/field-lifecycle matrix with the same mode cross-product | All 48 passed |
| Whole-owner and map-attribution probes, each compiler at 1-bulk / 2-direct, before and after | All 16 probe runs passed; all 12 fixture/configuration before/after comparisons agree |
| Actual object/header extents and emitted pointer-argument signatures | Four compiler/version extent probes and both changed implementation units passed |
| Source/HEAD, case/mode inventory, binary and extracted archive manifests | Verified |

The owner cases cover all C++ allocation failures, every selected byte/allocation/
setup/staging/capture/command boundary and exact/one-short retry; repeated forward/
back exchanges; mixed/equal alternatives and unchanged survivors; full semantic
and native handle refusal; stale generations; ordinary retry; both postclaim
publication faults; source/owner/shared/weak/domain lifetime and following ticks.
Caller/test snapshots and standalone control construction stay outside guarded
production execution windows. Preparation/capture/execute remain allocation-free.

Native CPU/x64, C++17 Release, Clang/LLVM 15 and GCC 11, matching pinned fork and
runtime, with GPU backends and CUDA qualification disabled. The known configured
builds are reconfigured/rebuilt for the pinned source; no cache is cleaned.
Existing post-PASS Taichi cache-lock warnings are retained. No full regression,
database, GPU, performance, broader producer or complete combined budget pass is
claimed. GitHub CI is authoritative for its own scopes; contributor-local logs
are not independently executed maintainer evidence.

Retained evidence: `/tmp/hcp-owner-source-maps-4f74ad11-evidence.tar.gz`.
SHA-256:

```text
d96b91bac77b2191a95dfc9e5c20ecd4242ec45192972b2946fd54bce031083e
```

It contains all five qualification profiles, the 96-case/mode inventory, exact
commands/source/submodule/toolchain/build identities, verified before/after native
binaries, raw measurement and parsed comparison records, ABI probes/signatures,
preliminary development/failed diagnostic evidence and dependency status snapshots.
Every expected case and actual worker/path/LLVM flag was checked, and every
extracted directory manifest verified. These paths are local, not hosted artifacts.

Review this implementation and reconcile authoritative dependency/head gates
before integration. Native indexed guards, collision policy, catalogue/mixed
revisions, broader producer adoption, full accounting and #324 mathematics remain
separate. Held #75/#111/#123 are unchanged.

After review, the next bounded inspection can measure the per-selection
`ExchangeBindings check` in [owner adoption](../src/assembly/exchange_owner.cpp)
and trusted/staged map construction in [standalone binding setup](../src/assembly/exchange_bindings.cpp),
against [adoption accounting](ADOPTION-ACCOUNTING.md),
[owner resources](OWNER-RESOURCES.md) and the
[private binding lifetime contract](PRIVATE-INSTANCE-BINDING.md).
Separate temporary requests, captures and source association from retained
candidate storage; this contribution does not authorize eliminating those checks.
