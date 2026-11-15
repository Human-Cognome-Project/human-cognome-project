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

## Many-tick dt/Q observations — 2026-11-14

The test-only `intent_trajectories` case follows #333 review 5899774502 and
integrated main `a504af8e9d639483b1002da5ed279fcb04a9e78d`. It runs 64 production
ticks for each combination of dt=0.125/0.5 and body/field fraction bits=8/24/52.
The three-body, two-level fixture has unequal whole-body masses (1,2,3), partial
participation (1/2 for the first two bodies), sparse physical records, and initial
X/Y carried velocity. Contacts and radial damping are explicitly disabled to
isolate this first bounded sweep; field publications evolve normally.

At every sample the candidate is checked against the source-derived long-double
reference described above, including all physical slots and unavailable/masked
states. Every tick also uses the existing independent physical reference:
participating mass generates force, whole-body mass supplies inertia, the current
brake acts once, and accepted displacement determines retained velocity. Normal
activity selection continues after initial invalidation; the fixture does not
force all bodies awake on every tick. Candidate copies must become stale after
each tick. Inspection and tick allocation checks remain in place.

`intent_trace` records each body's three axes before/after every tick, including
candidate status, carried velocity, evaluated force, pre-brake proposed endpoint,
post-brake/pre-Q endpoint, accepted position/displacement, retained velocity and
publication delta. Proposed endpoints are **host reference calculations**, not
native intermediate readbacks. Inactive rows report unchanged proposed endpoints;
their force field may be cached. Candidate coordinates are meaningful only when
the corresponding status is Supported; `comparable` marks supported pairs. Frame
anchors must remain unchanged. The trace allows intent drift and separation to be
computed independently of particle motion. Summary counters count body-tick events,
not unique bodies, and zero displacement alone is not labelled convergence or sleep.

Each dt uses the same number of ticks (physical durations 8 and 32), not a matched
time integration-error experiment. There is no trajectory equality assertion
between precisions, worker counts or paths, nor a fixed-intent convergence gate.
At least one moving particle and moving supported candidate witness is required
per configuration to prevent a vacuous stationary fixture. Existing production
force/braking/activity code and all resource inventories remain unchanged.

Remaining #324 work includes explicit crossing/approach controls, contact and
focal-damping comparisons, equal-force/different-intent activity invalidation,
longer moving-intent studies and a reviewed decision before any brake change.
This sweep does not qualify GPU behavior, linguistic settling, orbital stability,
performance, or full B/combined admission. Storage accounting remains separate.

Executed source `0bf93258f0c2e8dee9580a1ae3215248a43f57e7`, Taichi tree
`8b31651be07d8bad0b2a6a261369e8690b9c3b53`, LLVM/Clang 15 Release, WSL Ubuntu CPU.
The matching fork and engine rebuilt without reported compiler warnings. All ten
focused CTests passed: four trajectory profiles (one/two workers, bulk/direct),
four original live-candidate tests and two packet probes. Each trajectory profile
executed six configurations/384 ticks; all four profiles total 1,536 ticks.
No build/assertion failures. Prior lifecycle/whole-repository tests were not rerun
or relabelled. Native Windows, GPU, database, browser and performance were not tested.

All four profiles produced these summary counts (192 body-ticks per row):

| Fraction bits | dt | Moving supported candidates | Moved bodies | Active with no accepted movement |
|---|---|---|---|---|
| 8 | 0.125 | 192 | 192 | 0 |
| 8 | 0.5 | 24 | 24 | 3 |
| 24 | 0.125 | 192 | 192 | 0 |
| 24 | 0.5 | 93 | 91 | 5 |
| 52 | 0.125 | 192 | 192 | 0 |
| 52 | 0.5 | 93 | 91 | 5 |

These observed counters are not cross-profile trajectory equality checks or
stability classifications. In particular, candidate motion and accepted particle
motion need not have equal counts. An unchanged active endpoint alone cannot
identify whether braking, quantization or cancellation caused it; inspect the
separate proposal/publication trace. Continued motion is legitimate.

Local archive `intent-trajectories-0bf93258.tar.gz`: manifest SHA256
`f112e735c9eed7b554158a97572fcb18ebc1b0248af40fdce398757afc83fe33`;
archive SHA256 `34e308b3520a63ff04f5e044ecd3e30f6f406a041760e3e7b423f138ecc5eeae`.
Source/config/compiler/binary pins, commands, complete CTest output and per-axis
traces are retained locally. Raw local evidence has not been independently reviewed
remotely. Final publication after the tested pin changes three Markdown files only;
hosted qualification and review remain pending.

## Independent motion controls — 2026-11-14

The test-only `intent_controls` case follows #334 review 5904501254. Seven
one-tick controls separate approach, a crossing **pre-brake proposal**, tangential
carried motion, contact off/on and focal damping off/on. All use identical
one-field/two-body source geometry and a Supported initial intent at x=2.
Changing contact stiffness or damping must not change the initial field-only
candidate or its force. The paired controls keep contacts/focus installed in both
runs, varying only the corresponding parameter. No production mechanics change.

Each case independently checks the native candidate against extracted sources,
then the complete tick against the existing physical reference. The contact pair
requires the analytically expected -0.6 X force increment; the focal pair requires
unchanged force, reduced outward X displacement and unchanged tangential Y
motion. The approach proposal lies between the recipient and intent plane; the
crossing proposal passes that plane before braking. Neither assertion requires
the accepted endpoint to cross. The tangential control requires retained sideways
movement without sideways field force. No stability interpretation is a test gate.

The per-axis output identifies candidate, field force, combined force, carried
velocity, host-reference pre-brake/pre-Q proposals and native accepted position
and retained velocity separately. Fresh post-tick candidate status accompanies
its coordinates; old copied reports must be stale. References are not native
intermediate readbacks. These test traces are groundwork for viewport-usable
observations, not a new viewport API or UI. No-allocation query/tick checks remain.

This bounded slice does not complete equal-force/different-intent **activity**
invalidation coverage; the existing packet arithmetic controls do not substitute
for it. That remains next under #324, followed by further integrated observations
as needed. No new brake, sleep criterion or conceptual interpretation is selected.

Qualification: source `7cf12be4a415728763e4d7335ee8ea78d7a3ee51`, Taichi tree
`8b31651be07d8bad0b2a6a261369e8690b9c3b53`, WSL Ubuntu CPU, Clang/LLVM 15 Release.
Matching fork/engine build and all ten focused CTests passed: four controls
(one/two workers, bulk/direct), four original candidate and two packet tests.
The four control profiles each execute all seven cases. No build/assertion failures
or reported compiler warnings. The prior many-tick/lifecycle suites were not rerun.
No GPU, native Windows, database, browser or performance qualification is claimed.

Local archive `intent-controls-7cf12be4.tar.gz`: manifest SHA256
`e3b2e88b81a9e433829cf5221bca5b9a83b437a4faea5d350e3c274f1a1adf51`;
archive SHA256 `d9b559d73cd47ee07726e6d145dfd297a6df21f18d3618d81e9c582b72d2554d`.
Source/config/compiler/binary pins, commands and full outputs are retained locally;
raw archive is not independently reviewed remotely. Final delta after the tested
source is three Markdown files. #334 remains the source-accepted parent pending
hosted qualification; no duplicate main/parent CI or self-merge was requested.

## Equal-force intent publication and activity — 2026-11-14

The test-only `intent_activity` case follows #335 review 5904523468. It exercises
two live leaf fields under one shared root, sharing one recipient, with unchanged and changed-source
controls. Initial leaf packets have (X,mass)=(1,1),(-2,1); changed packets are
(2,4),(-4,4). The root changes from (-0.5,2) to (-1,8). Each response force is unchanged, so the recipient's combined X force
remains exactly 0.375 while the coefficient-weighted intent moves from 162/499 to 324/499. The recipient participates by one half in each leaf; both inherited responses are retained in the coefficient sum even though their forces cancel.
All inputs use exact binary geometry/mass arithmetic. Five real fixture bodies
supply those packets through ordinary native field publication, not supplied
candidate packets or a host replacement for publication.

After an initial zero-duration tick consumes body work, the test modifies source
body masses/positions through test-only access and selects only fields. The next
ordinary tick must evaluate zero bodies, publish the fields, and leave the shared
recipient awake only for the changed case. The current mass-publication
invalidation path is the expected cause: equal combined force does not suppress
that path. **This is not proof of a candidate-aware wake predicate**, nor a test
of geometric equal-force changes with unchanged source mass. No production
predicate, invalidator, brake or sleep law changes.

Observations match the independent source-derived reference. The unchanged case
retains candidate values but its old copied observation still becomes stale when
the completed tick changes; freshness is distinct from both value change and wake.
The changed case preserves the retained native force accumulator and exact
candidate force while changing intent. Inspection must not change the wake flag.
The following ordinary body phase consumes the native wake and is checked against
the existing physical reference, including retained motion. With dt=0, recipient
position and velocity remain zero; this is activity evidence, not a trajectory
or stability study. No-allocation query/tick checks are retained.

Traces separately report source change, old/new force and intent, post-publication
wake, following body visits, stale-copy rejection and accepted position. This
establishes current behavior for viewport-oriented observation without treating
conceptual interpretation as an acceptance gate. The direct source mutations are
fixture setup, not an operational edit/refresh API qualification. Further geometric
controls and the eventual viewport observation interface remain separate work.

Qualification at `a2706e3a7f560234e2f5120e53c739fb78949d02`, Taichi tree
`8b31651be07d8bad0b2a6a261369e8690b9c3b53`, WSL Ubuntu CPU/Clang 15 Release:
matching build and all ten focused CTests passed (four activity profiles,
four original candidate profiles, two packet probes). One/two workers and
bulk/direct paths were exercised. Every activity profile reported unchanged:
force 0.375, intent 162/499, no wake or following body visits; changed: force
0.375, intent 324/499, recipient awake and five following body visits. Both reject
the old observation and preserve the dt=0 recipient position/velocity. These
are current-behavior observations, not an intent-aware activity policy.

The initial source `e73cb04fb973dde060ab7a772b32f1d62d3119fc` built but failed
all four new activity tests at installation: the fixture incorrectly supplied
two roots. The correction adds the required shared root and includes inherited
responses in the independent expectation. The other six checks passed in that
initial run; it is not relabelled successful. Initial build/test logs are retained
inside the final archive. Final build has no reported compiler warnings.

Local archive `intent-activity-a2706e3a.tar.gz`: manifest SHA256
`1cef70d9d16eaa685f506aba941480c37079dbd77aad1c5254671b38ee739d02`;
archive SHA256 `c569a9b2a2b211b634e04626a91c07ccbc4e1797579eeb60662ba71263f96e05`.
Source/config/compiler/binary pins, commands, full outputs and initial failure
are retained locally, not independently reviewed remotely. Final delta after
the tested pin is three Markdown files. Earlier trajectory/control/lifecycle
suites were not rerun; GPU/native Windows/database/browser/performance were not
tested. #335 remains the source-accepted parent pending hosted qualification.

## Unchanged-mass geometric activity — 2026-11-14

`intent_geometry` follows #336 review 5904575854. Two leaves and a shared root
receive real body positions through native publication. The shared recipient has
mass 1 and half participation in each leaf; the other two bodies retain mass 0.5.
Initial leaf centroids are (1,4,8) and (1,-4,-8), each with mass 1 and recipient
distance 9. Edited centroids are (8,2,16) and (8,-2,-16), with the same masses and
distance 18. The root moves from (1,0,0) to (8,0,0), retaining mass 2. The X offset
grows eightfold while distance doubles, preserving the local combined X force
1/729. Symmetric Y/Z forces and inherited forces cancel analytically. Local and
inherited coefficient sums change, producing a different candidate intent.

The test verifies all body and published packet masses unchanged, native candidate
X force exactly preserved, and native force readback against the analytical value.
It reports per-axis native force equality separately; analytical cancellation is
not silently promoted to bitwise equality. Independent source-derived references
include both local and inherited responses. No supplied candidate packets replace
the native publication mechanism.

After initial work consumption, test-only position edits select only fields. The
first ordinary tick must perform zero body evaluations and wake the recipient only
in the changed case. Three further ordinary ticks permit parent propagation and
consume native work, checked against the physical reference. The unchanged control
has no wake or following body visits. Both cases reject old copied observations
after the completed tick. dt=0 preserves recipient position and retained velocity,
keeping freshness, activity and accepted motion separate.

This records current publication behavior. Intermediate publication sets can
change individual or combined response forces even when the final force agrees;
the wake is not evidence of a candidate-aware predicate, nor proof that the final
force alone selected work. No production formula, activity rule, accounting or
viewport API changes. Direct fixture edits do not qualify an operational edit API.
The #336 inline comment now correctly names published packet masses rather than
each edited body mass. Viewport observation integration and positive-duration
geometric studies remain separate follow-up work, without an interpretation gate.

Qualification: `2952fa5dace693b0ad0887fdaa9c5f9df726de3b`, Taichi tree
`8b31651be07d8bad0b2a6a261369e8690b9c3b53`, WSL Ubuntu CPU, Clang/LLVM 15 Release.
Matching build and all ten focused CTests passed: four geometric profiles
(one/two workers, bulk/direct), four original candidate tests and two packet probes.
No build/assertion failures or reported compiler warnings. Prior activity/control/
trajectory/lifecycle suites were not rerun. No GPU/native Windows/database/browser
or performance qualification is claimed.

Observed changed-case X intent: 0.32920543250580747 to 2.1151741357348248. Native
force readback was exactly unchanged in all axes, with X=0.0013717421124828531 and
Y=Z=0. Recipient wake was set and three following body visits were reported. The
unchanged control preserved intent, stayed asleep and reported zero body visits.
Both rejected stale copies and retained zero recipient position/velocity. The
independent strict candidate reduction has small cancellation residuals in changed
Y/Z intent (about -8.36e-17/-6.69e-16), covered by its arithmetic reference tolerance;
these are not silently equated with the production force readback. Exact native
force equality is reported evidence; the test separately asserts exact diagnostic
X force equality and checks native components against the analytical reference.

Local archive `intent-geometry-2952fa5d.tar.gz`: manifest SHA256
`efe38624dfec2ac895898246e7d7e72a2918e33be2a515c15f1c4d4b499cef9c`;
archive SHA256 `8f81f10ee38dacda9da2f7c021dd18da9fe3910a5b11b11a1ef55bb6c4bdf77e`.
Source/config/compiler/binary pins, commands and complete outputs retained locally;
raw evidence is not independently reviewed remotely. Final delta from tested pin
is three Markdown files. Source-accepted #336 remains the parent pending hosted
qualification; no duplicate parent/main CI or self-merge was requested.

## Positive-duration geometric observations — 2026-11-14

`intent_geometry_motion` continues #337 review 5904746631 using the same accepted
unchanged/edited geometry fixtures. It first runs their zero-duration activity
controls, then advances each through 32 ordinary ticks at dt=0.125 (four model
time units). The duration change invalidates the first body phase as production
already requires; later phases follow native activity. No observation selects
work and no stability criterion is added. A nonzero accepted-coordinate witness
ensures this is actual motion coverage rather than a stationary replay.

Every tick checks candidate arithmetic from current extracted sources, the
independent combined-force/integration/Q reference, exact retained displacement
velocity, unchanged frame, completed tick advancement, copied-report rejection,
and allocation-free query/tick boundaries. The per-body/per-axis trace includes
before/after candidate status and coordinates, prior position/carried velocity,
diagnostic field force, native force, host-reference pre-brake/pre-Q endpoints,
native accepted position/displacement/velocity, evaluated-body and next-awake
flags. Candidate coordinates require Supported status; inactive native force can
be cached. Host-reference endpoints are not native intermediate readbacks.

This extends numerical observation evidence and changes no production formula,
activity predicate, resource inventory or viewport interface. The prior equal-force
relationship describes the staged geometric endpoints, not a constraint imposed on
subsequent evolving trajectories. Contacts and radial damping remain disabled in
this geometric study; they have separate controls above. This is one dt and one
precision profile, not an accuracy sweep or performance/stability qualification.

### Viewport connection identified (not implemented here)

The existing [CompletedState/HarnessBinding](../../tools/viewport/native/snapshot.h)
and [native observer](../../tools/viewport/native/completed_state.cpp) already
capture an exclusively owned completed boundary, validate record incarnations and
prepared generation, retain actual backend/tick/frame, and account requested
readback separately from owned copies. Follow-on integration should reuse that
route with explicit optional candidate inspection and its gravity/work/status
metadata, joining by validated physical record to existing occurrence identity.
It must not label the test-only reference proposals as engine-owned observations.
The strict [capture schema](../../tools/viewport/snapshot.schema.json), browser
validation and display need a coordinated extension before candidate payloads
are exposed. No new JSON shape or browser behavior is claimed by this test slice.

Qualification: source `1ac2838974a6db41e4bf11a3bdfab698e226b72f`, Taichi tree
`8b31651be07d8bad0b2a6a261369e8690b9c3b53`, WSL Ubuntu CPU/Clang 15 Release.
Matching build and all ten focused CTests passed: four geometric-motion profiles,
four original candidate tests and two packet probes. Each motion profile runs both
zero-duration geometry controls followed by 32 positive-duration ticks per case;
256 positive-duration ticks total across one/two workers and bulk/direct paths.
All profiles exercised nonzero accepted-coordinate steps in both cases. No build/
assertion failures or reported compiler warnings. This does not rerun or relabel
the earlier broader trajectory/lifecycle matrix. No GPU, native Windows, database,
browser, viewport rendering or performance qualification is claimed.

Local archive `intent-geometric-motion-1ac28389.tar.gz`: manifest SHA256
`50b4e4bf4a7cd42fb945a462947ae69923a635b2b8417b91883228d643dd08cd`;
archive SHA256 `65b1bf5ad406ead1d1ee753542b856925e1e954aba0b6373815981efb5968bb8`.
Source/config/compiler/binary pins, commands and full per-axis outputs retained
locally; raw archive is not independently reviewed remotely. Final delta after
tested source is three Markdown files. Source-accepted #337 remains the parent
pending hosted qualification; no duplicate parent/main CI or self-merge requested.

## Viewport candidate integration — 2026-11-14

The optional completed-state candidate capture now follows #338 review direction
through the existing validated HarnessBinding observer. See the viewport's
[optional intent contract](../../tools/viewport/README.md#optional-field-intent-in-completed-captures)
for status, mapping, coordinate, budget and failure semantics. The schema, browser
validator, selected-object inspector and offline bundle change together. Same-tick
optional payload changes refresh the watch display; legacy snapshots remain valid.
The letter ActiveSession producer does not automatically request this optional
Harness diagnostic. Test-only proposals remain outside this native payload.

Native qualification at `2c5478f38f5f8ccbf1f4b82763173c7db9a7762a`, unchanged
Taichi tree `8b31651be07d8bad0b2a6a261369e8690b9c3b53`: matching build,
`viewport_snapshot_test` and four direct native capture profiles passed (one/two
CPU workers, bulk/direct, tick 0 through 2). Each generated capture checks source
candidate fidelity, absent/null coordinates, budget refusal, state neutrality,
readback accounting and stale tick/record identity. Native query exceptions are
not silently converted into absence. The geometric trace precision cleanup uses
max_digits10 and restores the previous stream precision.

Final browser source `9e159d1c9f1f8a30b5c5da3f3cd873a169f31163` differs from the
native qualification source only in browser slot validation and its rebuilt bundle.
Windows Node 22.13.0: all 16 model tests and 127 offline browser checks passed
(Chrome 157.0.8103.50, configured SwiftShader, no browser errors/network requests).
Three actual native JSON snapshots passed the browser validator; the generated
latest snapshot was loaded and selected in the browser and visually inspected.
The inspector showed Supported intent and the native local/relative coordinates.
No physical-GPU engine, native Windows engine, database or performance qualification.
The retained bundle was rebuilt from the pinned dependencies.

Local wrapper issues were corrected without relabelling tests: trailing CR caused
an initial final-log command and an exit command to fail after native tests passed;
wrappers now use LF. A first JSON validation helper mistakenly included the evidence
manifest as a snapshot; filtering was corrected and its failure log retained.
No native build/assertion or browser/model-test failures occurred.

Local archive `viewport-intent-2c5478f3.tar.gz`: manifest SHA256
`21340d6f7df9f6ff86add0a43b7e8a0e538faa6262c3f885e9135931bb0cd2fb`;
archive SHA256 `eb86ae54778cdfc6d24a440b4aeb769a2928700e76f70c8863960c002085e1dd`.
Native source/config/compiler/binary pins, captures/logs, browser results/screenshot
and helper notes retained locally, not independently reviewed remotely. Final
publication after the browser source changes Markdown only. Hosted review/CI remain
pending; no self-merge or duplicate parent CI. No new engine force/brake/activity,
resource inventory or reference-proposal implementation is introduced.

## Owning-run capture configuration — 2026-11-14

Source `6f4e53829a0f3a73f4c23e403161a8ca36426f04`, Taichi tree
`8b31651be07d8bad0b2a6a261369e8690b9c3b53`. Stacked on source-accepted #339,
with qualified main `1541729b` merged; no self-merge or inference of parent acceptance
from CI. The C3 capture runner now exports intent only with explicit
`--intent-work-limit N`, using its own gravity and the existing validated completed
boundary. Omission and zero-budget refusal remain distinct. Optional pacing leaves
model ticks unchanged. Legacy ActiveSession capture is unchanged.

Local qualification on WSL Ubuntu, CPU-only Clang 15 Release: matching native
build and viewport_snapshot_test passed. Windows Node 22.13.0: all 16 model tests
passed. Three actual finite native runs (ticks 0–4) fed the offline Chrome
157.0.8103.50 watch reader: successful intent (one worker/bulk), omitted intent and
zero-budget refusal (two workers/direct). Each applied all five captures,
preserved selection, matched final JSON exactly, and displayed the expected
inspector status with no browser errors or network requests. Native reference,
f64, state-neutrality and stale-binding checks ran within each capture case.
Four malformed budgets and a repeated budget option rejected before engine setup.
The enabled run screenshot was visually inspected.

The adapter reads bytes and metadata from one opened native file on every poll;
it does not fabricate snapshots. It substitutes for the directory picker, so
real picker/handle support remains unqualified. Browser rendering is SwiftShader;
no physical GPU engine, native Windows engine, database, or dedicated performance
qualification is claimed. Regression diagnostic probes are separate from the
configured export's accounting. No formula, activity or accepted-motion changes.

Local raw evidence: workspace `setup-evidence/intent-config-6f4e5382.zip` contains
native source/build/config/compiler/binary pins, option failures, three sets of
native files/logs, watch results and screenshots. Archive SHA256:
`0ca0833726a6461a8b3375f15a32230c54fa84591621629bc027d1ea15914ec0`.
Adjacent `intent-config-manifest.sha256` SHA256:
`369c5c98c65784f3bf57721916ddc5d91239b3829fa6c7fc3a6e52da57faced3`.
These are contributor-reported local observations, not independently inspected
hosted qualification. Final source delta after the tested pin is documentation only.

Next: review the explicit C3 configuration and native file-watch evidence. The
bounded fixture is usable for live inspection; broader owning-run/retained study
integration remains separate. Overlays are optional; keep candidate observations,
reference proposals and accepted motion distinct.

## Retained producer diagnostic requests — 2026-11-14

The retained `viewport_letter_capture` producer now accepts optional
`--intent-work-limit N`, with the same strict unsigned decimal range (including
zero) and duplicate-option rejection. It passes its actual run gravity into the
owner-checked ActiveSession completed-state observer. Omission remains omission.

ActiveSession currently constructs legacy flat FixedStudy execution. It cannot
supply the C3 diagnostic: every explicit request therefore returns `unavailable`,
even with zero budget, without invoking or replaying a C3 query. No candidate
rows, additional numeric copies or diagnostic readbacks are claimed. Prepared
`intent.generation` is null only for unavailable flat execution; the capture's
existing session/study/generation UUID still identifies the checked owner report.
Tick and frame remain explicit and checked, and schema/browser semantics reject
null-generation successful/refused results. A prepared C3 generation is never
fabricated. Existing prepared Harness captures retain their prior semantics.

The native test covers omitted/zero/max-budget requests, unchanged physical
snapshot/copy/readback payload, wrong owner, invalid gravity, stale frame/tick,
refreshed handles and false legacy refusal. CLI tests cover budget boundaries,
malformed values and duplicate options. This is request/status integration, not
completion of the retained-to-C3 execution bridge. Next implement that bridge
under the existing owner, with validated occurrence/record bindings and prepared
hierarchy lifetime; then replace explicit unavailability with actual diagnostic
inspection. Do not compute a host approximation or treat flat results as C3.

Qualification: native source `008eeda1` with Taichi tree
`8b31651be07d8bad0b2a6a261369e8690b9c3b53`, WSL CPU/Clang15 Release. Built retained
capture/options/snapshot targets; both focused CTests passed. All 17 browser-model
tests passed and pinned bundle rebuilt. Browser source
`38e3a93d70ed3c10488a5cd7f04930146a159f18` differs only by a browser assertion:
128 offline Chrome157.0.8103.50 checks passed, direct file navigation, no browser
errors/network requests. SwiftShader rendering, no physical GPU computation.

An actual retained letter capture used the existing read-only core/linguistics
services and a fresh retained local warm database. Four CPU ticks completed;
three native snapshot files validated, the final 16-object capture opened in the
browser, and its unavailable inspector screenshot was visually inspected. This
is static real-capture validation plus the existing browser suite, not a new
real-picker or retained live-watch qualification. No database was reset/dropped.
No broader numerical, GPU, native Windows engine or performance qualification.

Local archive: workspace `setup-evidence/retained-intent-008eeda1.zip`, SHA256
`b78fb115970dec4e9331d29da508a91f6d4c53dd95d53de015771d7e5eb6d24f`, contains build,
config/compiler/binary/source pins, native logs/captures and browser evidence.
Local evidence is contributor-reported, not independently inspected. Final delta
after the browser test source is documentation only. Hosted review/CI pending.

## Active coordinate-free handoff — 2026-11-15

First retained-to-C3 bridge step, proposed independently from pending #341 on
main `6f72bd42`: `ActiveAssembly::bind_structure(nodes)` now exposes the existing
v2 membership/projection structural handoff. It preserves source definitions,
occurrence identity/path/roles, selections, contacts, memberships and recipe focus.
No positions, velocities, numerical slots, record claims, hierarchy or runtime
are created by this call. Native admission remains the numerical authority.

An explicit `focus()` override replaces recipe focus, including an explicit empty
selection. Revision and logical restart preserve that distinction. An empty
selection is only a structural handoff, not populated admission readiness; D1's
nonempty positive focal selection remains required at native bootstrap. Legacy
`bind` behavior stays unchanged. V1 recipes reject on the structural path, and
v2 recipes reject on the legacy path. Required missing private input and any
present local overlay reject explicitly: private structural overlays are not yet
implemented and must not be silently omitted.

The next bridge step is owned prepared admission: reserve a trial-profile Harness,
translate exact structural masses/counts and intrinsic geometry into native
commands, resolve comparison/focus identities against the same prospective source,
and call the accepted generated-placement/bootstrap boundary. Do not import
`PreparedFixtureAccess`, guessed slots, legacy f32 shares/radii as original f64
source data, or caller world starts. Publish occurrence bindings only after native
admission commits; then expose completed observations through the existing owner.
This handoff does not implement those steps or change the retained producer mode.

Validation source `e0155331aaaa815bebbb71a246486e4ee4dfe355`, Taichi tree
`8b31651be07d8bad0b2a6a261369e8690b9c3b53`, WSL CPU/Clang15 Release:
`structural_recipe_test` and `active_workbench_test` built and passed. The source
checks exercise inherited/replaced/cleared focus, revision/restart, missing/local
input rejection and legacy/structural separation. Source-path comparison explicitly
respects independent projections' distinct study UUIDs. Legacy workbench execution
remains covered. No physical GPU, database or browser run was needed for this scope.

Earlier test development failures are retained: ambiguous FocalField and colliding
legacy fixture variable names, then an incorrect cross-projection identity check.
The latter first tried full IDs despite independently generated study UUIDs; final
checks retain each owner's study prefix and compare structural source paths. These
were corrected, not relabelled successful. Local archive
`setup-evidence/active-structural-e0155331.tar.gz` SHA256
`5dbc5370d68222c323808fc69d6c567faf7b2acf2d17092934244fceb3f90e17`
contains final source/config/compiler/binary pins, logs and prior failed attempts.
Final delta after tested source is documentation only; hosted CI/review pending.
No acceptance of #341 or full retained C3 execution is inferred.

### Review correction: standalone projection boundary — 2026-11-15

Review 5910700086 identified a real hosted regression: structural_recipe_test is
also built by the standalone letter_projection project without Taichi/PostgreSQL.
The initial PR added an ActiveAssembly header to that shared source, accidentally
pulling in the native runtime. Earlier top-level native passes did not cover this
build route and do not clear that failure.

The original structural_recipe_test source and membership_fields link dependency
are restored exactly. All new ActiveAssembly checks moved to
`active_structural_checks.h`, invoked by native active_workbench_test. No Taichi or
PostgreSQL dependency was added to the standalone project; production code is
unchanged by this correction.

At `33081aaa76c741a4d172097be840fda5e6e72191`, a fresh standalone configure/build
(`cmake -S engine/tests/letter_projection`) and all four CTests passed. The native
active_workbench_test rebuilt and passed with the moved binding/focus/revision/
restart/local-input checks. Local evidence includes commands, logs, both build
configurations/compiler versions, source/Taichi pins and binary hashes in
`setup-evidence/structural-boundary-33081aaa.tar.gz`, SHA256
`f3025e1452885a92ce328b009461b453ae16ec6d97dbd51affe55e06cf89f85a`.
Final delta after this tested pin is documentation only. This supersedes the prior
qualification scope for #342; hosted rerun/review of the correction remain pending.
