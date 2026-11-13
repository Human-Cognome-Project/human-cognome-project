# Live response observation

Experimental extraction following #328 review 5897445910, for #324. This adds
`Harness::inspect_live_responses(work_limit)` to the existing live hierarchy owner.
It does not compute combined intent or change force, braking, activity or sleep.
Changing intent and continuing motion remain legitimate model states.

## Snapshot boundary

The synchronous caller receives owner-preallocated storage, borrowed until the next
synchronized operation on that Harness. Check `status == Ok` before consuming it;
failed queries retain old payloads. Only the first `responses` rows are live, with
column stride `capacity`; padding is transferred but is not a response.

The native kernel acquires complete current child/parent centroid packets using the
existing selector helper. Local displacement is child centroid minus recipient;
inherited displacement is parent centroid minus child centroid. Recipient XYZ is
captured in that same operative frame, accompanied by frame anchor, generation and
completed tick count. Body and source field mass are separate. Structural eligibility
uses the production MultipleBodies/complement predicates. A separate normal-or-zero
extraction flag rejects nonfinite/subnormal inputs/results and subtraction flushed
to zero. It does not certify the downstream coefficient formula, positive masses,
exact coincidence, inverse powers, or valid combined intent.

Identities and exact numerator/denominator fractions are read from the actual native
committed response arrays, without float conversion or reconstructed graph traversal.
Body records can be sparse. Packet slots and body records are owner-local references,
not persistent canonical identities or leases. This first fixture checks distinct
local/inherited records in one tree; it does not establish general alias deduplication,
refresh/rebase identity continuity, or stale-consumer invalidation.

## Storage and work

The existing owner manifest now includes one native 10-column f64 output, its host
snapshot, four host identity columns and two host u64 fraction columns. Existing
native identity/fraction arrays are reused. The prepared launch context is included
in binding bytes; embedded vector controls remain in the fixed owner object count.
No runtime-wide full-B or combined-admission claim follows from this manifest.

For response count N and prepared capacity R, the declared work charge is
`1 + N + 16*R`: one dispatch unit, one fixed response evaluation per row, and one
unit per transferred scalar including padding. It is an accounting convention, not
an instruction or time bound. The effective limit is the lesser of the caller's
limit and configured CascadeWork. A short limit returns CapacityExhausted before
synchronization, dispatch or readback. Success transfers `112*R` bytes. Capacity and
native/host buffers are prepared before inspection, including a zero-row warm launch.

## Qualification and continuation

The dedicated `live_response_1/2_bulk/direct` tests inspect before and after one
production tick, check exact work/readback charges, zero/one-short budget refusal,
preinstall rejection, sparse identities, local/inherited geometry, exact fractions,
no host allocation, and unchanged physical/activity/publication state. Separate
`intent_native_1/2` cases retain the supplied-packet candidate comparisons.

Initial source fa4781a5 built, but the four extraction tests failed because their
assertion incorrectly assumed body records 0..2. Diagnostic source 6bcd9904 exposed
the fixture's valid sparse record 69. Source c02e6d2d checks actual owner records.
Initial failures are retained rather than counted as passing evidence.

Next: integrate candidate reduction with the extracted owner surface, qualify
refresh/rebase/dedup and validity/invalidation, then many-tick dt/Q comparisons.
Keep contact separate and unresolved coincidence explicit. No stability classifier
or new sleep gate is selected. Storage work and Taichi reuse audit remain separate.
See [packet diagnostic](INTENT-NATIVE-DIAGNOSTIC.md) and
[derivation](INTENT-DERIVATION.md).

Contributor-local qualification at `c02e6d2d` (Taichi tree
`ef9795b3e135de6edb4c3f5c8d409d6487f41385`): native library and engine builds passed;
six focused CTests and 16 scoped regression invocations passed. The latter cover
basic owner/storage, forces, dirty refresh and exchange bindings under one/two CPU
workers and bulk/direct paths. Refresh regression is not a dedicated response
snapshot continuity test. No GPU, native Windows, database, browser, many-tick intent
or performance qualification was performed. Final publication adds only Markdown.

Local evidence `live-response-c02e6d2d.tar.gz` retains source/config/compiler/binary
pins, full test output, commands, and both earlier failure attempts. Manifest SHA256
`8a5ac666690840c92ce5abc0a61145c6d67780d929b271a5f37de3d7ea811778`;
archive SHA256 `d387643a17aa57903dd6e461245b05defe060f721e297eed3842abeed0bf7031`.
Raw local evidence is not independently reviewed remotely. Hosted checks and
maintainer acceptance remain separate.

## Prepaid binding correction — review 5897775574

The first submission counted the observation context in actual prepared bytes but
omitted it from the earlier binding reservation used by required_scratch/admission.
The corrected live-enabled inventory adds 32 arguments and one context: six live
contexts instead of five. Both CPU host metadata/ABI storage and CUDA's additional
24 bytes per argument plus per-context flag/u64/pointer terms are reserved. A
live-disabled profile receives no increment; unrelated conservative slack is not
used to justify the new context.

The response fixture now checks the isolated live-enabled and live-disabled wide
profile reservations against their documented inventory, with both CPU and CUDA
arithmetic executed on CPU. It also checks the installed owner's actual prepared
binding total against the complete declared reserve. CUDA arithmetic is not GPU
execution or device qualification. Production dynamics remain unchanged.

Correction validation at `917a41b3bc2152a5424c9c1dbf9079cbea05fc0e`, unchanged
Taichi tree: build, six focused CTests and four basic owner/manifest invocations
passed (one/two CPU workers, bulk/direct). The local ABI's added allowance is
3,584 CPU bytes and 4,369 CUDA-arithmetic bytes. Actual CPU prepared bindings are
39,560 bytes against the complete 43,232-byte reserve. Isolated live-disabled
wide-profile reserves remain 17,584 CPU / 21,726 CUDA-arithmetic bytes. These are
local ABI observations; only CPU was executed. No corrective build/test failures.
The earlier 22-check report remains scoped to its original source, not rerun here.

Corrective local archive `live-response-917a41b3.tar.gz`: manifest SHA256
`4cd1b09a7fcd5237d024a939e708b5c5e6f2b324c5cc5aed3e1bb9831a90bea5`;
archive SHA256 `d7f72b2a5e96c5e64373fc2d34bfa94fae4d6219ff35db9aa550c731d10a503c`.
Source/config/compiler/binary pins and full logs retained; raw local evidence is
not independently reviewed remotely. Publication after this pin is Markdown only.
Prior-head hosted smoke passed; native CI was still queued when checked, and was
not manually retriggered. Updated-head hosted qualification and re-review remain due.

## Response validity and lifecycle follow-up

Following #329 acceptance review 5898430428 at 24b02279, the next bounded slice
adds fixtures around the existing extraction API. No production implementation,
resource inventory, intent formula or dynamics changes are made in this follow-up.
The parent remains pending integration while its hosted native qualification runs.

`response_validity` uses test-only injection into an installed owner. It checks
NaN, both infinities and subnormal recipient XYZ/mass; only that recipient's rows
become extraction-invalid. It corrupts each of the four words in selected and
inactive centroid packets separately: selected packet corruption invalidates the
rows referring to it, while inactive packet corruption does not. All checks retain
source identities/fractions, generation/tick/frame metadata and readiness, and
hash physical/activity/publication state before and after inspection. Test-owned
snapshot copies are outside the allocation-free inspection interval.

Zero and negative finite body mass remain extraction-valid, explicitly showing
that this flag is not downstream formula/positive-mass certification. Exact local
coincidence remains a visible zero displacement with valid extraction; it is not
silently removed or interpreted as valid coefficient intent. These synthetic
injections do not qualify production admission of invalid inputs.

`response_lifecycle` reuses the existing refresh and exact-rebase fixtures. Duplicate
source-row aliases and serialization-only sibling reorder preserve snapshots.
A hierarchy precision refresh advances snapshot generation while retaining source
records/fractions and completed ticks. Exact coordinate rebasing at capacities 70
and 128 advances generation, records the current frame anchor, translates recipient
XYZ, and preserves displacement, masses, eligibility, validity and source identities.
Inspection itself remains allocation-free and leaves the owner state unchanged.

These are bounded identity continuity witnesses for existing rows, not a general
canonical-identity/lease scheme. Retire/reuse, changed coverage/ratios, stale borrowed
consumers, range-overflow/subtraction-underflow controls, integrated coefficient
reduction and many-tick dt/Q comparisons remain pending. No wake/sleep requirement
or requirement that supported intent stop moving is introduced.

Follow-up qualification at `dcbba8316903bf8f14b58f6467e6b6fd5ef77c2a` (unchanged
Taichi tree ef9795b3e135de6edb4c3f5c8d409d6487f41385): native/engine build and all
14 focused CPU CTests passed: four validity, four lifecycle, four original response
and two supplied-packet intent tests. Each validity configuration covers 31 controls.
Lifecycle cases exercised both capacities and completed in 141–143 seconds under
the original 180-second timeout. Publication adds only a 600-second lifecycle timeout
for slower hosted setup plus Markdown; executable sources are unchanged after the
qualified pin. No assertion/build failures. The local script initially named a
nonexistent branch and stopped before building; correcting its name resumed setup.
No GPU/native Windows/database/browser/performance or many-tick intent qualification.

Local archive `response-validity-dcbba831.tar.gz`: manifest SHA256
`a8e067f9d5effef47f1ed3d954a688a66aa41a53a26df1faf10b2238a06584b6`;
archive SHA256 `aff17f52d059ee696ead0cd76a3d99e28f457db2ba7d19a5ad04e60971fb9360`.
Source/config/compiler/binary pins, commands and full output are retained locally;
raw evidence is not independently reviewed remotely. Hosted qualification and
maintainer review of this follow-up remain pending. No duplicate parent CI requested.

## Retirement/reuse and subtraction follow-up

Continues #330 review 5898902458 with qualified main 2e758a57 integrated. The
previously unused initial response snapshot in refresh_occurrence_reuse now serves
as the baseline for inserted and retired rows. This remains test-only work.

Each capacity variant of the 32-round fixture ticks, inserts an occurrence with two response rows
and contact edges, ticks again, then retires it. New checks require the exact source
ratio and extraction flag on inserted rows, remove retired rows from the live
prefix, recover the baseline source identities/fractions for survivors, and observe
31 forced slot reuses with distinct native handle incarnations in a four-slot
profile (three survivors). The original 70-slot profile remains covered without
assuming immediate slot recycling. Raw body/packet integers
remain owner-local storage references, not persistent occurrence identities.

A same-owner test consumer checks status, generation, completed ticks and frame
anchor on copied snapshots. It rejects copies across 32 ticks and 32 retirements;
retirement is checked separately with no intervening tick. This illustrates required
consumer discipline using existing metadata, not a new production freshness API,
lease, or cross-owner identity validator. Borrowed report storage still expires at
the next synchronized operation. The snapshots used across operations are owned
copies, not retained borrowed references. Direct test-only source mutation within a
generation is outside this freshness witness.

Sixteen range controls cover both local and inherited subtraction: positive and
negative overflow, both directions of a subnormal difference between normal inputs,
exact cancellation, minimum normal and maximum finite results, and signed zero.
Unsupported extraction remains explicit while rows/identities/fractions remain
visible. A subnormal difference may be preserved or flushed by the backend; either
must be extraction-invalid. These controls do not select a coincidence law or
qualify inverse powers/coefficient reduction. Inspection stays allocation-free and
bitwise owner-state preserving, and the fixture restores its synthetic inputs.

Next: integrate native candidate observations with owner-accounted storage/work,
then many-tick dt/Q comparisons. General identity/lease validation and cross-owner
consumers remain outside these same-owner fixtures. Moving intent is legitimate;
no production braking/activity change or full-B acceptance follows.

Qualification at `ea428fa0b7e18bf0fe10a6fd89eb51e568b14783` (Taichi tree
`ef9795b3e135de6edb4c3f5c8d409d6487f41385`): builds and all 18 focused CPU CTests
passed, four concurrent test processes, one/two workers and bulk/direct: four range,
four reuse, four existing validity, four original extraction, two native intent.
Each reuse configuration observed 31 same-slot reincarnations at capacity four,
zero immediate slot reuses at capacity 70, and 64 stale-copy rejections per capacity.
Each range configuration passed all 16 controls. No GPU/native Windows/database/
browser/performance or integrated many-tick intent qualification. Parent rebase/
lifecycle tests were not repeated; only the changed subset and related controls ran.

Initial source `4a96c762` built; 14/18 checks passed but all four reuse tests failed
because they incorrectly required immediate recycling in the 70-slot profile.
Native prepared allocation consumes the available free list; spare slots make that
assumption invalid. The correction preserves capacity 70 and adds capacity four
with three survivors, forcing the one vacant slot to recycle. No allocator behavior
was changed. Initial failures are retained, not relabelled as passing evidence.

Local archive `response-reuse-ea428fa0.tar.gz`: manifest SHA256
`829377a574e7d3937291381be260dfd15dc03b0ea9154c7e96f41e6bddee311c`;
archive SHA256 `a1d6c4a4e41b170b6cd5b0fe278f5164d89097c3a5e3b0098ccbed70d92598b7`.
Source/config/compiler/binary pins, commands, full results and initial failure logs
are retained locally; raw evidence is not independently reviewed remotely. Final
publication after the tested pin changes Markdown only. Hosted checks/review remain
pending; no duplicate parent run or self-merge.
