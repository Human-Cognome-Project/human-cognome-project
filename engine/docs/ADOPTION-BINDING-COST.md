# Temporary validation bindings during owner adoption

Status: contributor inspection and measurement, 2026-11-17, awaiting review.
Tested public source: `8972cd49f3b90ff58c93603132c96f61ba4f4ac1`, tree
`225853f4c0d9ea958b3bb2efd9bb734181c1a483`. Production is byte-identical to
provisionally accepted #366 `dc0854aaf104cf02d924ad4d404e6fe5a29825f7`.
Only test witnesses change executable source. Documentation follows separately.

This delivers the inspection authorized by
[#366 review 5948024253](https://github.com/Human-Cognome-Project/human-cognome-project/pull/366#pullrequestreview-5948024253).
It changes no runtime representation, validation, allocation policy or work fee.
Normal merge `266bdd8ba6d897a5c49eec16e786baa4c8f7d223` preserves current main
`8c3d0b930dcf6652e230cc0bfa1c9bbe2429eb4c`, which integrates #364/#365, and
the explicit provisional #366 dependency. Both current maintainer handoffs
and the contributor source-map record survive their handoff-only conflict.

## What each temporary establishes

[Owner adoption](../src/assembly/exchange_owner.cpp) constructs a stack
`ExchangeBindings check` once per selection, after collecting that selection's
declaration. It shares the owner's detached projection, declaration and immutable
membership catalogue, but constructs its own maps and captured facts through
the [existing binding constructor](../src/assembly/exchange_bindings.cpp).
It is not the coordinator's retained binding or candidate maps.

| Surface | Responsibility | Lifetime |
| --- | --- | --- |
| Trusted field map | Copies complete supplied field keys/handles; provides trusted initial address extents to owned-map reservation | Constructor local; destroyed on constructor return |
| Reserved occurrence/field maps | Hold complete ordered logical keys and all four native handle components; supply capture and immediate validation lookups | Members of this temporary; destroyed at the end of this selection iteration |
| Staged occurrence/field maps | Fresh selected-domain copies of caller rows, then copy into the reserved maps | Two constructor locals; destroyed on constructor return |
| Masses and outgoing handles | Exact source masses for the complete active inventory, plus full handles for this declaration's currently exposed outgoing alternative | Captured by value; destroyed with this temporary |
| Captured edges and masks | Native ledger row identity, full endpoints, ordinal, share and exact count; outgoing-record and used-edge masks | Independent captured facts; destroyed with this temporary |
| Declaration/catalogue aliases | Pin complete semantic identity, recipe/definition revisions, explicit direction, ordered parents and inherited membership | Aliases release with the temporary; the owner retains the immutable pointees |

Capture checks ready execution and current generation, complete active occurrence
inventory, distinct logical/native associations, current full body/field headers,
exact source masses, source exposure and declaration/catalogue agreement. For
each outgoing body it requires every declared sibling, ordered parent and inherited
membership ledger contribution, including repeated rows. Unmatched incident rows,
missing/aliased claims and wrong typed participation refuse. Immediate validation
checks the captured source facts again. This is exact source metadata validation,
not a numerical-state readback, native claim or tick.

Initial identity/layout associations still come from trusted admission claims.
Equal numerical values or a record index cannot authenticate a caller's logical
mapping. This inspection does not extend that trust boundary or choose a new
field-collision policy. Full pins and typed keys remain required.

The declaration is appended to the owner table while `check` is still alive;
its backing disappears at the loop iteration's end. The next temporary starts
with no preceding temporary requests live. No captured mass/handle/edge/mask
is transferred into the later retained coordinator. Operational preparation
captures fresh current facts. Immutable declarations are not native authority.

## Actual request and work attribution

The [new witness](../tests/exchange_adoption_cost.inc) uses the existing test-only
current-thread C++ allocation callbacks and the actual `ProjectionSetupResult.phase`. It tags
Bindings epochs before spare-projection setup, and excludes the later retained
coordinator's Bindings epoch. It records successful request sizes, acquisition/
release order, live and peak bytes per selection, and whole-owner overlap while
those requests live. Declaration-table growth during the temporary's lifetime
is included in the whole overlap. No production callback is introduced.

For each actual selection it invokes the exact private stack constructor again
with that owner's detached projection and retained declaration/catalogue, the
same trusted row values, and a fresh selected domain. The complete filtered
allocation/release sequence, request count, bytes and self peak must match real
adoption. This is not a substitute constructor. Its fresh meter includes the
same `sizeof(ExchangeBindings)` logical setup charge as the owner; it measures
constructor work directly. The actual whole-owner setup meter is separate.

One uninterrupted allocation-lifetime window observes owner, aliases, probe
domains and normal destruction. Every request is reclaimed, including final
weak/domain/bootstrap storage. Observer arrays are allocated before that window;
isolated-constructor deltas exclude owner/control/scratch already alive. No
observer counter is cleared to bypass outstanding allocations.

| Fixture | Per-selection requests / requested self peak | Temporary cumulative requests / bytes over adoption | Constructor logical work per selection | Actual whole-owner setup work |
| --- | --- | --- | --- | --- |
| One aggregate selection | 583 / 91,649 | 583 / 91,649 | 134,160 | 466,844 |
| Four mixed selections | 616 / 94,530 | 2,464 / 378,120 | 163,298; 163,374; 165,728; 166,094 | 1,149,866 |
| Three equal-sized alternatives | 598 / 93,003 | 1,794 / 279,009 | 143,895; 143,971; 144,032 | 796,076 |

All four dedicated compiler/path runs agree on every recorded value: 12 fixture
measurements and 32 selected-role constructor traces. Request/lifetime equality
also holds in the six full changed owner cases.

Every constructor has 90,460 requested bytes alive after its locals have died:
47,180 in 562 owned-map requests and 43,280 in five capture/mask buffers.
Constructor-local requests add 16 / 49 / 31 requests and 1,189 / 4,070 / 2,543
bytes for the one/four/three-selection fixtures. They overlap the member backing
at the measured self peak, then release before constructor return. The entire
temporary releases before the next iteration; cumulative bytes are not a sum
of simultaneous peaks. No achieved saving is implemented by this inspection.

Validation reservations remain based on the complete actual inventory:
176 units in the one-selection fixture; 656/656/736/736 in the mixed fixture;
368/368/384 in the equal-alternative fixture. These are included in constructor
work, not an extra charge to add again. All active masses, outgoing full handles,
edge headers/counts and both masks are checked against native storage, including
complete incident-row coverage and unrelated occurrence exclusion.

The 376-byte binding object is automatic storage, not a separate C++ heap request.
Its two map headers already occupy 112 of those bytes. The three constructor-local
map types have 168 bytes of headers in total. These `sizeof` extents do not
measure a complete compiler stack frame and are not added to requested heap bytes.

Retained candidates remain separate: 562 requests / 47,180 bytes per candidate
pair, independently located in the actual allocation table. Current plus candidate
map census is 94,360 bytes; complete buffer addresses and capacities stay unchanged
through every constructor probe. These candidate requests are not temporary
validator backing and are not added to its reported cost.

Whole-owner retained/observed requested peaks remain 242,404 / 294,636 / 258,014
bytes. Whole-owner overlap during individual temporary lifetimes is only
130,203; 154,859/157,857/160,855/163,869; and 137,608/139,670/141,732 bytes.
This matters for a future change: removing transient copies does not establish
a retained-owner saving, nor an equal reduction in the complete adoption peak.
The other released setup requests are 7,277 / 16,484 / 10,047 bytes, outside
these temporary constructors. No total-process, allocator, RSS, GPU, timing or
combined admission claim follows.

## Bounded proposal for review

The copied constructor maps establish no extra identity authority beyond their
inputs. Their complete association values equal the owner's already copied
initial maps, and they exist only to feed capture/reservation on this private
adoption route. A candidate follow-up is to use the existing private synchronous
const-map capture path for these validators, retaining the temporary's independent
mass/handle/edge/mask buffers and every source/pin/role/currentness check.
Source access must end before owner map reservation/mutation or publication.

Keep one complete initial owner snapshot and its prepayment. Keep standalone
value adapters independently owned. Do not move persistent candidate storage,
share mutable maps across publication, refresh captured generations, remove
captured validation facts, or reinterpret logical identity from native indices.
Only actually eliminated map construction/reservation/copy work could be removed;
the complete constructor fee is not a removable-copy estimate. Reusing validators
across selections, shrinking capture buffers, combined budget policy and broader
native guards are separate changes. Review this proposal before implementing it.

A future implementation must measure actual whole-owner requests/peaks/work,
object/argument extents and domain/weak cleanup, and preserve partial-adoption
unwind, exact/one-short work/bytes, source/owner death, standalone ownership,
full source/ledger/claim refusal, mixed/equal alternatives, both directions,
ordinary retry, postclaim faults and following ticks under the owning matrix.
Indexed guards, catalogue/mixed revisions, broader producer adoption, full B
accounting and #324 mathematics remain separate. Held #75/#111/#123 unchanged.

## Qualification and handoff

| Local scope on exact clean source | Result |
| --- | --- |
| Quick native build/backend CI, three CTests, 632 independent placement-math samples and placement capacity | Passed |
| Dedicated three-fixture probe, Clang/GCC × 1-worker bulk / 2-worker direct | All four runs passed; 12 fixture / 32 selected-role measurements agree |
| Full changed single/mixed/equal owner cases, Clang 1-bulk and GCC 2-direct | All six passed with original controls |
| Complete request/release trace, native-fact/mask, candidate/domain/weak cleanup and work controls | Passed |
| Source/HEAD, binary and extracted archive manifests; production comparison against #366 | Verified; production identical |

Production equals accepted #366 exactly. The changed default owner cases also
run this witness, so normal hosted native owner qualification exercises it.
The dedicated case runs only these three measurement fixtures. Existing owner
allocation/work/byte, exact/one-short, source/shared/weak/unwind, refusal/retry,
publication and following-tick controls remain enabled. No running workflow or
accepted full baseline was duplicated to onboard this inspection.

Native CPU/x64, C++17 Release, Clang/LLVM 15 and GCC 11, matching pinned fork/
runtime, with GPU backends and CUDA qualification off. Configured builds are
reconfigured/rebuilt for the pinned source; only the test translation unit is
explicitly made fresh. Known post-PASS Taichi cache-lock warnings are retained;
no cache or persistent store is cleaned. No full regression/compiler matrix,
database, GPU, performance or complete resource budget is claimed.
Contributor-local evidence is not independently executed maintainer evidence;
GitHub CI is authoritative for its gates. Reconcile #366's pending native run
44193455273 and this head's review/hosted gates before integration.

Retained evidence: `/tmp/hcp-adoption-binding-8972cd49-evidence.tar.gz`.
SHA-256:

```text
a8a91b4dad2e3f09a4d0056559c0c2cb215a83ab70380b28036a04a3c1d760af
```

The archive contains all three final profiles, verified manifests, source/
submodule/toolchain/configuration/binary identities, retained Clang/GCC native
binaries, exact commands, raw/parsed measurement agreement, development evidence,
production-scope comparisons and the provisional review/dependency snapshots.
Every directory in an extracted copy verifies its manifest. These are local
paths, not hosted artifacts. Review the concrete inspection/proposal before a
runtime change; reconcile its own authoritative hosted checks before integration.
