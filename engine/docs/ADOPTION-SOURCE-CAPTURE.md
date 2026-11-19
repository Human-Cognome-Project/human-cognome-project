# Capture adoption facts from prepaid owner maps

The subsequent [validator-buffer reuse inspection](ADOPTION-VALIDATOR-REUSE.md)
measures the remaining per-selection five-buffer construction and exercises an
independent scratch replay. #381 accepted that proposal; the separate
[adoption-local implementation](ADOPTION-LOCAL-VALIDATOR.md) records actual
production reuse and the required full validation, refusal and lifetime controls.
The earlier source/evidence below retains its historical per-selection behavior.

Status: contributor implementation, 2026-11-17, awaiting review and authoritative
GitHub CI. Tested public source: `da083031c9d5f6b99880939f5561c311d56ec9cc`,
tree `ab8aafb378806488bbd5e41a907c4853e5b458aa`. Documentation follows separately.
This implements the bounded private adoption change authorized by
[#367 review 5948664040](https://github.com/Human-Cognome-Project/human-cognome-project/pull/367#pullrequestreview-5948664040),
following the [temporary-binding inspection](ADOPTION-BINDING-COST.md).

Normal merge `7029c0782cd1983ed40f3987b3a8fa85d3d600cf` preserves current main
`8c3d0b930dcf6652e230cc0bfa1c9bbe2429eb4c` and provisional #367
`d49e63290a39ca18de0f6af0c3d13c1647b56cb8`, including provisional #366
`dc0854aaf104cf02d924ad4d404e6fe5a29825f7`. Their hosted and integration gates
remain separate from this source. No running workflow was duplicated.

## Private adoption boundary

The owner still prepays and constructs exactly one detached projection and one
occurrence/field snapshot. Its per-selection validator now receives those maps
by const argument through a private constructor overload. The same shared setup
meter admits its independent mass/outgoing-handle/edge/mask buffers, stages the
retained declaration/catalogue and explicit declaration direction, and performs
full capture plus immediate validation against the actual supplied inventories.
There is no private unlimited allowance, stored span, source pointer, new member
or registry. Source access ends when the constructor returns, before later owner
map reservation/mutation or publication.

Five capture/mask requests remain independent for every selection. Captured
masses, full outgoing handles, native row/endpoints/ordinal/share/count facts,
both masks, full typed keys and revision pins retain their existing checks.
Expected/current generation, readiness and complete incident-row validation are
unchanged. A failed construction unwinds to adoption's existing boundary; no
partial owner or native mutation is published. Captured generations are never
refreshed to make stale source valid. Later operational preparation still captures
fresh facts; these temporary validators are discarded within their iteration.

The standalone vector/value constructors are unchanged and still own their maps,
including the caller-source-death guarantee. The private overload is selected
only by initial owner adoption and its test adapter. Candidate storage, actual
native claim resolution, ordinary retry, postclaim faults 7/8, projection/map/
report publication and following ticks retain their original paths.

## Reservation bounds and work

The removed reservation helper scanned projection paths, tool and trusted field
addresses to size duplicate key backing. Those scans allocate no semantic identity
authority; full association checks remain in capture. The helper also supplied
checked particle-plus-batch and field-plus-min(field,batch) sums and subsequent
map append bounds. The private constructor explicitly preserves both checked
sums and inventory-count bounds before capture. The existing body-buffer bound
and every native claim/header/source check remain. Current/candidate reservation
still runs later for its own storage. No field-collision policy changes.

Only eliminated temporary owned-map reservation, trusted/staged-map construction
and row/key copies lose their work/request charges. The existing binding-object
setup fee, header association/direction unit and complete actual-inventory capture/
validation work remain. The preserved bounded scalar checks use the existing
fixed metadata admission; the old helper did not meter those arithmetic operations
separately. C/S byte admission, component copy-work prepayment, shared declaration/
catalogue storage and candidate work are unchanged. No combined gate is added.

Tests reject zero/one-short constructor work, retry at exact work and verify full
current source. An oversized field inventory specifically exercises the preserved
reservation bound, with all capture backing reclaimed. Existing overflow checks,
full owner allocation/budget sweeps and native refusal/unready controls remain.

## Actual before/after

Verified retained Clang/GCC binaries at inspected source
`8972cd49f3b90ff58c93603132c96f61ba4f4ac1` and the new source both execute the
adoption/request and independent whole-owner recipe probes at 1-worker bulk and
2-worker direct. This measures real old/new owners, not candidate-cost subtraction.
The actual adoption epochs still match the exact private constructor's ordered
acquisition/release trace, request count and peak for every selection.

| Fixture | Whole construction allocations before → after | Cumulative requested bytes before → after | Whole setup work before → after | Retained / observed requested peak (unchanged) |
| --- | --- | --- | --- | --- |
| One selection | 2,337 → 1,759 | 341,330 → 292,961 | 466,844 → 381,949 | 242,404 |
| Four mixed selections | 4,744 → 2,300 | 689,240 → 484,240 | 1,149,866 → 772,470 | 294,636 |
| Three equal-sized alternatives | 3,703 → 1,924 | 547,070 → 397,901 | 796,076 → 528,149 | 258,014 |

All four compiler/path configurations agree for every fixture. The independent
recipe probe matches the adoption probe's whole construction requests and peak;
constructor traces match real adoption, and all other setup work is unchanged.

Actual cumulative setup-request reductions are 578 / 2,444 / 1,779 allocations
and 48,369 / 205,000 / 149,169 bytes. Setup logical work reduces by
84,895 / 377,396 / 267,927 units. These counter types are separate, not additive.

Each temporary now has five requests / 43,280 bytes for independent capture
buffers, versus 583/616/598 requests and 91,649/94,530/93,003-byte self peaks.
No owned-map or constructor-local map request remains on this path. Per-selection
constructor work is now 49,265; 68,949/69,025/71,379/71,745; and
54,586/54,662/54,723. Validation reservations remain exactly
176; 656/656/736/736; and 368/368/384 units, respectively. No complete source
validation charge was deleted or calculated from empty owned maps.

Retained current/candidate maps still total 94,360 bytes; the independently
located candidate pair remains 47,180 bytes across 562 requests. Their complete
buffer identities/capacities and native state are unchanged through the probe.
Other released setup requests stay 7,277 / 16,484 / 10,047 bytes. Whole-adoption observed
peaks remain dominated by final retained backing in these fixtures;
there is no retained-owner saving or equal whole-adoption-peak reduction claim.

Actual before/after extent probes under both compilers report unchanged owner/
coordinator/binding sizes of 608/680/376 bytes. Empty binding map headers still
occupy 112 bytes within its 376-byte object. The three old constructor-local map
objects, whose headers total 168 bytes, are no longer constructed on this path;
they remain in standalone setup. Emitted LLVM signatures show two const-map
pointer arguments (16 bytes on this ABI), replacing two vector-reference
arguments of the same pointer width; setup work is a required reference. No
allocated view or stored loan is introduced. These type/argument extents do not
measure complete compiler stack frames or establish a 168-byte stack saving.

Requested bytes, retained census, cumulative allocation work, logical work,
temporary self peak and whole-owner overlap remain separate, overlapping
measurements. No allocator metadata, RSS, whole-process peak, GPU or timing claim
follows. Full B accounting and admission remain open.

## Qualification and handoff

| Local scope on the exact clean tested source | Result |
| --- | --- |
| Quick native build/backend CI, three CTests, 632 independent placement-math samples and placement capacity | Passed |
| Single/mixed/equal owner matrix: Clang/GCC × 1/2 workers × bulk/direct × default/no-unroll | All 48 passed |
| Standalone binding/commit/field-lifecycle cases: both compilers at 1-bulk and 2-direct | All 12 passed |
| Exact old/new adoption and independent whole-owner request probes | Eight before and eight after runs passed; all 12 fixture/configuration comparisons agree |
| Constructor zero/short/exact retry and independent field bound, source facts/masks, domain/weak cleanup | Passed |
| Actual old/new object/header extents and emitted constructor arguments | Four compiler/version extent probes and LLVM signatures verified |
| Source/HEAD, binary, case/mode inventory and extracted archive manifests | Verified |

The owner matrix covers all C++ allocation failures and selected byte/allocation/
setup/staging/capture/command boundaries, exact/one-short retry, mixed/equal
alternatives and both directions, complete source/claim/role refusal, surviving
objects, source/owner/shared/weak lifetime, ordinary retry, both postclaim faults
and following ticks. The adapted request witness still validates complete masses,
full handles, ledger facts and exhaustive masks. It rejects the empty owned-map
adapter on private adoption. Standalone lifetime tests retain independent maps
after caller copies die. Final selected-domain/bootstrap/weak cleanup is checked.

Native CPU/x64, C++17 Release, Clang/LLVM 15 and GCC 11, matching pinned fork/
runtime, GPU backends and CUDA qualification disabled. Known configured builds
are reconfigured/rebuilt for exact clean source; no cache is cleaned. Existing
post-PASS Taichi cache-lock warnings are retained. No full regression, database,
GPU, performance or complete combined-resource pass is claimed. Local logs are
contributor evidence; GitHub CI remains authoritative for its gates.

Retained evidence: `/tmp/hcp-adoption-source-da083031-evidence.tar.gz`.
SHA-256:

```text
f98475d6218ad507c16bf225c4731963ed22dfd9f2553154a74f29c2ca2f7ad3
```

The archive contains all three qualification profiles and their exact commands,
68-case/mode inventory, verified before/after native binaries, raw measurements
and parsed comparisons, source/submodule/build/toolchain/runtime identities,
ABI probes/signatures, development evidence, the accepted review and remote
status snapshots. Every extracted directory manifest verified. These paths are
local evidence, not hosted artifacts. At the retained handoff snapshot #366's
native workflow 44193455273 completed successfully; #367 native 44203059956
remained running. Both dependencies' integration and this head's gates are separate.

Review this implementation and reconcile provisional dependency/head hosted gates
before integration. Further refactoring should select another concrete association
or duplicated responsibility through its actual owner/lifetime and measurements.
Validator reuse across selections, capture-buffer shrinking, indexed guards,
collision policy, catalogue/mixed revisions, broader producer adoption, full
accounting and #324 mathematics remain separate. Held #75/#111/#123 unchanged.
