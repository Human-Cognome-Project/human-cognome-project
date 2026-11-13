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
