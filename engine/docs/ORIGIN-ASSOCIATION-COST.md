# Origin definition-key ownership and cost comparison

Bounded comparison under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343),
2026-11-16, awaiting review. Starts from main
`9518d04368e53c0723142fd3c0437c25b221abdd`, with source-accepted #352 at
`9a86962e46735a2ae6c80dc0f928f703cfb5ff32` as an explicit pending dependency.
The comparison branch preserves both histories at public merge `1123133b`;
this is not integration of #352 on main. Tested comparison source:
`fcef214c895992dcd8e2fa50ac5eb62c4c20eec4`.

## Specific candidate

[`Origin`](../src/assembly/active_workbench.h) retains an occurrence key, a
`definition` ObjectKey, and optional full shared/local provenance. Current
producers copy the same ObjectKey into `definition` and into the selected pin:

| Producer / source kind | `definition` | Existing pinned object |
| --- | --- | --- |
| `ActiveAssembly::origins`, shared occurrence | `o.definition.object` | `shared->object`, copied as part of the full ToolRef |
| `ActiveAssembly::origins`, local value | `v.ref.object` | `local->object`, copied as part of LocalRef |
| Installed-hierarchy observation fixture in `live_hierarchy_test.cpp` | `o.definition.object` | `shared->object` |

Source: [active producer](../src/assembly/active_workbench.cpp),
[native observation fixture](../tests/live_hierarchy_test.cpp).
ActiveSession reports and the prepared observer receive these values from the
existing producer. No located production writer gives the extra ObjectKey an
independent identity or lifetime. This is one duplicate **within a provenance
row**, not a reason to merge repeated occurrences or remove source revisions.

The exact shared ToolRef must remain intact: its object includes scope/address,
and its revision carries the immutable source pin. LocalRef additionally retains
owner/version. Replacing either with a bare definition key would lose information.
Membership, comparison fields, native claims and physical mass are separate
associations and are not this candidate.

## Ownership and lifetime

`ActiveAssembly::origins()` constructs a detached vector. Both key copies are
ordinary owning UuidText/address containers within the same Origin value; they
do not alias the owner's source or each other. Their nested requests live until
that row is destroyed. Clearing rows releases both copies while the vector's
capacity remains; destroying the vector releases its backing. There is no need
to extend the source/Runtime lifetime to use the object already owned by the
row's full pin.

The comparison obtains the source through #352's checked association view after
real native bootstrap. It does not retain row loans across native mutation or
owner teardown. The producer still returns all source alternatives: this fixture
has four origin rows but only two exposed native bodies. Neither hidden metadata
nor repeated occurrences may be pruned to obtain a saving.

The [test-only witness](../../tools/viewport/native/prepared_owner_state_test.cpp)
uses the existing fixed-storage allocation-lifetime observer. It records the
actual `origins()` construction, then resolves the UuidText and address buffer
pointers for each `definition` and pin object to their live C++ allocation
records. Equal keys have distinct backing. All exposed rows also match the
checked plan's full occurrence/ToolRef associations. This attributes actual
retained requests; it does not estimate cost by timing a synthetic key copy.

Borrowing `origin.shared->object` for comparison uses no new owning storage or
lookup wrapper. The test verifies allocation-free access, complete release of
the measured rows and unchanged availability of the checked execution. Existing
snapshot/source-pin/transfer equivalence and tick controls then run normally.
The fixed current-thread observer is test instrumentation, not production memory
or a full native/driver/process accounting surface.

## Measurements

The two-occurrence, shared-parts fixture has four origin alternatives and two
exposed bodies. On the tested Linux x64 ABI, its measured producer/row lifetime is:

| Measurement | C++ request scope |
| --- | --- |
| Complete `origins()` construction | 29 allocation calls; 4,368 requested bytes cumulatively |
| Requests retained by the returned vector and rows | 2,808 bytes |
| Scoped construction peak | 3,662 bytes |
| Attributed duplicate `definition` text/address backing | 8 live allocations; 208 requested bytes |
| Pin's own equal object text/address backing | Separate live allocations; 208 requested bytes |
| Vector capacity | 4 Origin objects × 520 bytes = 2,080 requested bytes |
| After clearing rows | Only the 2,080-byte vector buffer remains |
| After vector destruction | Zero measured live bytes/blocks; every acquired request released |
| Comparing through the already owned pin member | Zero allocation calls/requested bytes |

`sizeof(ObjectKey)` is 88 bytes inside each Origin. That member extent is already
part of the 520-byte row and its containing vector request. It is not another
88-byte allocation, and the table does not add it again. The 208 attributed
bytes refer only to its separately allocated UuidText/address buffers.

Construction cumulative bytes, final retained bytes and scoped peak are different
measurements. The witness records only quiescent, current-thread C++ allocation
lifetimes and verifies that the constructor's new/new[] call count matches the
tracked acquisitions. Attribution excludes allocator overhead, arbitrary malloc,
stack/TLS residency, native backing and RSS. It is neither full B nor a measured
before/after production optimization.

## Current consumers and removal boundary

[`viewport::context`](../../tools/viewport/native/snapshot.cpp) uses
`Origin.definition` to select labels; `viewport::snapshot(ActiveReport,...)`
uses it for the serialized definition key. The prepared observer constructs
origins from its authorized source; ActiveSession's report does the same.
The installed-hierarchy fixture constructs shared-pinned Origin rows, and
observer tests compare detached definition/pin values. These callers must be
updated together if the redundant field is removed.

Current snapshot validation checks handle/occurrence/slot/exposure coverage but
does not establish that `definition` equals exactly one pin's object. A manually
constructed report can therefore contain inconsistent, missing or ambiguous
provenance without this particular equality check. The located producers do not
emit those cases. This comparison does not silently redefine their acceptance
or choose a fallback key.

Smallest proposed follow-up, subject to review:

1. Establish the valid Origin contract explicitly: exactly one full shared/local
   pin and a matching definition for current producers. Decide/refuse invalid
   manually constructed provenance before changing which key a reader uses.
2. Reuse the object already owned by that pin through a small checked accessor,
   then remove the extra ObjectKey member and update the listed producers and
   consumers. No new locator, cache, shared-pointer layer or origin registry is
   needed. Retain numeric occurrence identity/order and all source alternatives.
3. Prove identical snapshots/labels and exact shared/local source pins, detached
   lifetime after caller/owner release, and refusal for mismatched/no/dual pins.
   Keep private input authorization and shared-cache rejection unchanged.
4. Repeat the source-pinned cost comparison against the actual changed producer.
   Only then report a realized before/after saving. Account for changed vector
   element size, growth/peak and remaining formatting/readback separately.

The current duplicate's nested requests and member extent identify an opportunity;
they are not a measured net removal saving. In particular, do not add a member's
sizeof charge to its containing vector request as another allocation, or infer
construction/peak savings from final retained bytes alone.

## Scope and evidence

This slice changes the observer test only, plus contribution notes. Production
code remains byte-identical to source-accepted #352; no Origin field, native
claim, candidate buffer, owner operation or numerical policy is removed. The
local test uses the current shared V2 prepared subset. Local provenance behavior
is source-inspected here; it requires its own controls before the proposed
cross-producer removal. Direct indexed lifetime guards, mixed-revision/schema
work, producer migration, full accounting and #324 mathematics remain separate.

Personally executed at public `fcef214c`, exact recursive submodules and matched
CPU-only LLVM/Clang 15 fork tree `8b31651be07d8bad0b2a6a261369e8690b9c3b53`:

- `bash engine/scripts/run_local_ci.sh quick /tmp/hcp-origin-fcef214c-quick`:
  PASS, native targets built, actual CPU, three CTests, 632 independent placement
  interval samples and placement_capacity passed.
- `flock engine/build/.local-ci.lock bash /tmp/hcp-origin-fcef214c-focused.sh`:
  five CTests passed in 208.42 seconds. All four prepared observer worker/path
  profiles passed 65 checks each, including the new attribution/lifetime witness
  and prior snapshot/source-pin/tick/transfer controls. Legacy snapshot passed
  its existing 64 checks. All profiles produced the measurements above.
- `bash /tmp/hcp-origin-fcef214c-gcc.sh`: fresh GCC 11 Release wrapper on the
  matched fork; two CTests passed in 53.94 seconds (one-worker bulk observer and
  legacy snapshot). The same origin attribution and release measurements passed.
  Prior #352 transfer deltas remain 39 allocation requests / 3,847 requested bytes;
  those are a different comparison from this candidate's 208 retained bytes.

The runs retained clean-source checks, configuration/commands, raw logs, binary
hashes and per-file manifests. Manifest verification passed. The final notes are
Markdown only; the tested witness is unchanged. Raw evidence directories:
`/tmp/hcp-origin-fcef214c-{quick,focused,gcc}`. Archive
`/tmp/hcp-origin-fcef214c.tar.gz` (GCC build products excluded), SHA256
`f284da01bd63953394bf5747f897540e2ba5da9956980985581075a6ef88910c`.

These are contributor-local results; raw archives are not remotely inspected
merely by recording their hashes. Full regression/compiler/database matrices,
browser tests, GPU and native Windows were not run locally for this comparison.
GitHub CI remains authoritative; #352's pending hosted qualification and
integration are separate gates. No duplicate workflow dispatch was requested.
