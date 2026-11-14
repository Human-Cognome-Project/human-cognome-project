# Live native candidate observation

Experimental diagnostic following #331 review 5899185239. The existing live owner
now exposes `Harness::inspect_live_intent(gravity, work_limit)`. It does not change
production force, integration, braking, contacts, activity or sleep. The coefficient
candidate remains a diagnostic proposal; moving intent and continued motion are
legitimate states, not failures to converge or evidence of orbital stability.

## Native source and arithmetic boundary

One native launch refreshes the existing response extraction buffer; a second
reduces the committed per-body response ranges into preallocated output. No host
response enumeration, graph reconstruction or per-query storage preparation occurs.
The source is current published centroid packets and current recipient coordinates,
not an independently rebuilt centroid graph. The same strict arithmetic helper in
`intent_ir.h` is used by the existing supplied-packet native probe.

For each response, participating mass is body mass times the exact numerator /
denominator converted to f64; coupling is gravity times participating mass times
source field mass. Conversion follows the existing native force path's f64 boundary;
u64 fractions themselves remain available through response inspection. Structural
eligibility controls whether a row contributes. Complete extracted inputs are still
validated, including inactive rows. Gravity must be finite normal-or-zero and
nonnegative; body/source masses must be positive normal; numerator/denominator must
be nonzero. Coupling, squared distance, cube, coefficient and accumulated results
use sticky range checks, including flushed nonzero products/divisions and subnormal
or flushed non-cancelling sums. This is conservative diagnostic domain handling,
not new production admission or force semantics.

The field-only candidate uses alpha=coupling/|delta|^3, field force=sum(alpha*delta),
and relative candidate=force/sum(alpha). The operative-frame candidate adds the
recipient position; the separate frame anchor identifies that frame. Field force
here is a strict diagnostic recomputation, not a readback of the production force
buffer or a claim of bitwise agreement with its reciprocal-square-root path. Exact eligible
coincidence remains unresolved, with its production-style gated force contribution
zero; other finite contributions can remain observable. Contact force is excluded
from this reduction and normalizer. The packet probe continues to report contact
force/validity separately; this API does not claim to compute live contact force.

## Report and lifetime

`status` on the report describes execution/budget availability. Only read values
when it is Ok. Per-body column zero reports Absent=0, Supported=1,
UnresolvedCoincidence=2, Unsupported=3, with unsupported taking precedence. Columns
are: state, field availability, coefficient sum, coincidence count, field-force XYZ,
relative candidate XYZ, operative-frame candidate XYZ. Candidate coordinates are
masked unless Supported. Unsupported field results are masked; coincidence counts in Unsupported
rows must not be interpreted as valid source counts. All numeric payload columns are finite; gravity metadata echoes the request even
when that requested value is invalid.

Column stride is the prepared physical body capacity. Indexes are body record
slots, not occurrence identities. Slots without response rows are Absent. No contact
columns or persistent identity handles are invented. Generation, completed tick,
gravity and frame anchor accompany the report. It is borrowed until the next
synchronized Harness operation; owned copies require the same consumer freshness
and identity discipline as [response extraction](LIVE-RESPONSE-OBSERVATION.md).
Neither API is a lease or a cross-owner identity validator.

## Prepaid storage and work

One 13-column f64 native array and matching host snapshot are added to the live
owner's typed manifest. The reusable six-argument reduction context is counted in
actual bindings and in the earlier CPU/CUDA binding reservation: seven live contexts.
Embedded controls are included in the fixed owner size. Existing extraction storage,
exact fraction arrays and committed response offsets are reused. The constructor
warms the context with zero bodies. This does not establish full-runtime allocation
or complete B/admission coverage.

For N live response rows and P physical body capacity, the declared charge is
`2 + 2*N + 14*P`: two dispatch units, two response visits, one body visit and thirteen
readback scalar units per body. Success reads `104*P` bytes. Work units are a fixed
accounting convention, not instructions or time. A short effective limit (minimum
of caller and configured CascadeWork) returns before synchronization, dispatch or
readback, with zero work used. Old payloads remain unconsumable on refusal.

## Qualification boundary and next step

The new fixture compares source-derived long-double field sums, coefficient sums
and relative/operative candidates before and after a production tick, including
zero gravity and sparse absent slots. It checks invalid gravity, unresolved
coincidence, unsupported-over-coincidence precedence, masked finite output,
preinstall/short-budget behavior, allocation-free inspection, unchanged model state,
and frame/generation/tick/work/readback metadata. The accepted packet cases continue
to exercise shared helper arithmetic, including independent contact handling.

The initial integration did not qualify candidate-specific lifecycle; the follow-up
below records that bounded evidence. Many-tick dt/Q comparisons, GPU/native Windows
and live contact diagnostics remain separate. Extraction evidence alone does not
qualify the reduced report. No new braking law or stability classifier is selected.

## Contributor-local evidence

Tested source `25fdae23ccab17db239d8aab1d86c2c5c9b40ec5`, Taichi tree
`ef9795b3e135de6edb4c3f5c8d409d6487f41385`: native/engine builds and 38 scoped
CPU checks passed. Ten focused CTests cover four live candidates, four extraction/
reservation profiles and the two supplied-packet probes (29 cases each). Sixteen
owner regressions cover basic storage, forces, dirty refresh and exchange bindings.
Twelve extraction regressions cover validity, range and retirement/reuse. All use
one/two workers and bulk/direct where applicable; independent test processes ran
in four lanes. No GPU computation or performance qualification.

The live fixture uses 994 declared work units and 7,280 readback bytes. Local added
binding allowance is 672 CPU / 833 CUDA-arithmetic bytes; actual CPU bindings are
40,168 bytes against a 43,904-byte complete reserve. Live-disabled reservation is
unchanged. CUDA arithmetic on CPU is not CUDA execution. Values are local ABI
observations, not portable constants or complete runtime allocation totals.

Initial source `2c0db5aa` built and passed the six packet/extraction tests, while
four candidate tests failed an invalid-gravity assertion that mistakenly included
retired setup handles. Those rows correctly report Absent. The corrected fixture
uses current owner headers and checks both live and absent slots. No production
implementation changed for that correction. Initial failed logs are retained.

Local archive `live-intent-25fdae23.tar.gz`: manifest SHA256
`f71cd29bb57c8d053c96b3227af2bc44725acab136c860e95bf8dfa75acb1fe5`;
archive SHA256 `93294ba0bbaec20e729bc4e30b22594d7448f93d03b394ce814b20d03b9afc2a`.
Source/config/compiler/binary pins, commands, full focused and regression results,
and initial failures are preserved. Raw local evidence has not been independently
reviewed remotely. Final publication after the tested pin changes only Markdown.
Hosted CI and maintainer acceptance of this integration remain pending.

## Candidate-specific lifecycle follow-up

Following #332 review 5899338489, the existing lifecycle fixtures now exercise
reduced reports directly. This follow-up changes tests only. A reusable host
long-double reference recomputes coefficient sums, force and relative/operative
candidates from the current extracted source rows at each transition. It handles
valid coincidence and absence, checks every physical slot, and does not call the
native IR helper. It is source-derived arithmetic evidence, not an independent
reimplementation of extraction or an invalid-range oracle.

Refresh coverage includes duplicate aliases/serialization-only reorder (unchanged
candidate values and generation), precision refresh (new generation), deliberately
lagged dirty publications, and a count-only edit with unchanged f32 share. The latter
must change the candidate coefficient sum and match the current exact source ratio.
It does not replace published packets with independently recomputed body centroids.

Rebase coverage at capacities 70 and 128 compares independently recomputed values,
requires state/availability/coefficient/coincidence/force/relative columns unchanged,
and checks translated operative candidates with the established numerical tolerance.
Absent or unresolved masked coordinates remain zero. Inverse rebasing restores the
values but cannot revive a copied report's old generation.

The 32-round occurrence fixture runs at capacities four and 70. Native insertion
must create a non-absent candidate; retirement must clear its status and coefficient/
force/candidate payload, while all slots continue to match fresh source-derived
expectations. The four-slot profile forces 31 reincarnations. Copied reports are
rejected across each tick and separately across retirement without tick advancement.
The same-owner test consumer also checks readiness, requested gravity and frame.
These checks are consumer discipline, not a new production lease/freshness API;
cross-owner validity and direct test-only mutation within a generation are not claimed.

Snapshot copies are made outside allocation-free inspection intervals. Every query
checks unchanged physical/activity/publication state and budget/readback metadata.
No production formula, source state, accounting inventory, braking or activity rule
changes. Many-tick dt/Q comparisons remain next; the zero-gravity turnover ticks in
these lifecycle fixtures are not trajectory/stability qualification.

Lifecycle qualification at `64a505af9825f65a400028e515d066f5c91509c1`, unchanged
Taichi tree `ef9795b3e135de6edb4c3f5c8d409d6487f41385`: builds and all ten focused
CPU CTests passed: four candidate lifecycle, four original live candidate, and two
supplied-packet probes. One/two workers and bulk/direct paths were exercised with
four independent test processes. Each lifecycle case includes refresh basics,
dirty/count refresh, rebase at capacities 70/128, and turnover at capacities 4/70.
The existing model-survival, native backing and allocation checks remain in those
fixtures. No build/assertion failures. This run does not relabel the earlier 38-check
integration qualification or infer GPU/native Windows/database/browser/performance
or many-tick dt/Q results.

Local archive `intent-lifecycle-64a505af.tar.gz`: manifest SHA256
`fb3c8a86c6cbaeab1c3c8fed760c4cbf102ee53a001be668f21673b2201dacd8`;
archive SHA256 `d920f34cd7a196c868709fdfda2c0e9107b06680a3dd6ccf8c213d37d184903e`.
Source/config/compiler/binary pins, commands and full outputs retained locally;
raw evidence is not independently reviewed remotely. Final publication after the
tested pin changes Markdown only. Hosted qualification and review remain pending;
no duplicate parent CI was requested and no self-merge was performed.
