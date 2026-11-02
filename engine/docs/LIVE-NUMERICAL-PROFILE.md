# Live trial numerical profile (C1)

**Local motion revision, 2026-10-29:** [accepted-displacement motion](ACCEPTED-MOTION.md)
supersedes the untrimmed-velocity rule described below. The historical accepted
runs retain their original source pins; validation of the revision is separate.

Status: **accepted in #187, 2026-10-25**, following accepted [#186](../../docs/live-numerical-profile-plan.md)
under #172/#177. Runtime/test source is pinned in the
[CPU evidence](../tests/numerical_profile/README.md). Default legacy f32 behavior
remains selectable and is covered by the existing regression suites.

This describes pinned C1. The later [C2b-1 correction](CUMULATIVE-PUBLICATION.md)
is accepted in #192 under accepted #191: revised trial activity and final
speed publication. Its evidence is separate from C1's retained run.

## Boundary and use

`PreparedCapacity(limits, NumericalProfile::trial(body_bits, field_bits))`
selects an immutable profile for that prepared `Harness`. Both settings are
explicit retained fraction bits in 0..52; 52 preserves the full f64 container.
Pass the same profile to `PreparedStorage::required_scratch` before construction.
The default capacity profile remains legacy f32. Unsupported settings, descriptor
dimensions, scratch reservations or backend data64 support reject before native
layout creation. There is no profile migration during execution.
Fraction-bit selectors control trial publication only; legacy has no added Q
stage and capture reports its actual 23 stored fraction bits.

Use `tick64(Parameters64)` and `particle64`, `group64`, `particles64`, `groups64`,
`determined64` and `universal64` for the trial. `inspect_prepared` refreshes the
reserved typed readback buffers; the host arrays are observations/staging, not
a second simulation authority. Old float element/tick accessors reject a trial.
The old public float vector references remain empty in a trial; modifying their
shape fails the existing binding validation. Legacy calls retain their original
types and arithmetic. This preserves source usage, not a binary ABI guarantee.

The supported path is the bounded prepared numerical fixture and coherent
update interface. Production placement, operational moving-contact transactions,
rebasing, enduring owners and the #156 integrated operation exit are separate.
Structural recipes acquire no mandatory positions or velocities. Native assembly
preparation still supplies its existing f32 physical mass/radius and shares;
using them in a wider execution recovers no discarded source detail.

## State and numerical publication

All eleven body rows, eight field rows, physical accepted displacement, previous
field centres, determiner speed, focal controls, universal mass, update results,
new fixture rows, geometry summaries and their authoritative inspection buffers
use f64 in the trial. Sensitive constants and parameters enter as f64. Handles,
CSR and flags retain their integer widths. Legacy source shares remain f32 and
exact counts/denominators remain u64.

Live participating mass is current physical body mass times the native f64
count/denominator ratio. Every participating source row requires known positive
counts, a consistent legacy share and checked per-field/per-occurrence coverage
at most its denominator. Coverage validation sorts reused scratch before CSR
packing; numerical reductions retain original source order. Unknown accounting,
contradictions and integer overflow reject before publication. Counts may overlap
across unrelated fields; they do not create extra universal physical mass.

A prepared flat u64 descriptor contains E counts, E denominators and G flags
indicating whether a field has more than one physical occurrence. The last
flags prevent repeated ordinals of a solitary occurrence from producing a
fictitious self-response when its published field centre is coarser than its
body position. Force and wake exclusion use this same structural eligibility;
the force law and its numerical amplitude remain unchanged. These flags are
source metadata, not activity or an additional scheduler. Successful commits
remap field flags to actual claimed records and upload the committed descriptor.

Integration publishes `Q_b(trial_next)` once. Accepted displacement is published
next minus published old. Physical velocity survives independently, and the
trial-to-publication difference is discarded. No hidden unquantized position,
compensation term or positional tail feeds a later tick. Mass is never trimmed.

Accepted movement declares the existing native centroid bit. A dirty field
performs one ordered full-member f64 reduction, including sleepers, anchored to
its first source member. `group_delta[0..2]` temporarily holds that anchor and is
zero after publication; it is not pending motion in this profile.
`group_delta[3..5]` holds the previous published centre for ordinary native wake
comparisons. Keeping the arithmetic anchor separate avoids overwriting that
history and makes repeated unchanged reductions stable. Seed and repair use
the same first-member reduction and Q publication. Repair of an existing field
keeps its old published centre as history; a new field starts with its newly
published centre. Survivor physical state is preserved.

An exact-count edit can change a trial weight even when its f32 share is
unchanged. Such edits repair affected field mass/centres and invalidate force
inputs. The equivalent edit stays metadata-only for legacy arithmetic.
Focus combines selected published fields in f64 without an additional Q stage;
universal mass counts each physical occurrence once. Geometry and both optional
candidate queries read the selected resident body dtype directly.

## Activity and failure policy

The following provisional activity rule describes built C1. The merged
[#190 clarification](../../docs/precision-exposure-and-concept-catalogue.md#cumulative-significand-clarification--2026-10-26)
supersedes it. The accepted #191
[cumulative publication/activity plan](../../docs/cumulative-publication-integration-plan.md)
specifies the [C2b-1 implementation](CUMULATIVE-PUBLICATION.md), accepted in #192,
including cached speed and preserved input invalidation. The historical C1 rule
below is not the revised activity predicate.

The single particle and centroid NativeSets and the existing type order remain:
particle calculation/consumption, contact propagation, dirty-field publication,
then centroid consumption and particle activation for the following phase.
No queue, scheduling epoch, extra awake mask or full-loaded scheduler is added.

C1 provisionally retained the native particle bit for an untrimmed displacement
attempt, accepted movement or a velocity change. Only accepted movement dirtied
field inputs. Demand separately exposed motion attempts and published movement.
Its bounded constant-velocity watch fixture kept one body awake for 32 ticks,
with zero published changes, preserved velocity and zero field reductions.
Initial input invalidation also evaluated the stationary body once: 33 body
evaluations in total. This was positional quiescence with continuing work, not
complete computational settling. Changing force and later two-sided contact
perturbations exercised publication crossing and sleeper recovery. Those source-
pinned observations remain historical rather than acceptance of the correction.

Ordinary validation/work/capacity rejection preserves committed source,
physical/control/history/activity state and readiness. Native errors after an
in-place tick starts mutation mark execution unready and throw; no rollback is
claimed. Trial scalar/staged values and checked publication/status outputs must
be finite normal-or-zero encodings. Participating mass may not underflow to
zero. The existing fork FTZ mode is unchanged: arithmetic that has already
flushed a tiny result to zero cannot be distinguished from exact zero by these
guards. This is not gradual-underflow qualification or full-range error control.
Experimental toward-zero Q has an origin/crossing bias; production rounding and
per-LoD precision policies remain open.

## Resources and observations

The manifest charges actual typed count × element bytes × copies. Trial source
descriptors have four copies (committed/prospective, host/native),
`4 * (2E+G) * 8` bytes with physical sentinels. Update/geometry/inspection/reset
workspaces use the selected widths; integer budgets are not doubled. Five extra
arguments extend existing force, centroid, wake, repair and determiner bindings.
No new recurring contexts or runtime/compiler substrate are introduced.

The 22 numerical shapes describe one payload each, separately from scratch S.
Primary body/field/share/index/contact/speed/universal shapes have one reserved
host copy and, for BulkCopy, an additional owned native copy. CpuDirect borrows
the host payload. Remaining Activity shapes have one native copy; their reserved
inspection/CSR/reset host workspaces are charged in S. The evidence includes
these per-shape copy counts and host/native byte totals. Native map roots,
pool backing and fixed call-frame readback arrays are separate; S also includes
unused binding reservation and is not a measurement of resident RAM.

`RealBuffer` owns only one populated typed vector. Its unused vector object is
fixed metadata, not a shadow numerical array. Fixed `Harness`, `PreparedStorage`,
Demand and GeometryResult sizes are recorded separately. Fixed origin/focal
inspection objects contain inactive small float/double arrays, excluded from the
selected payload manifest and included in object sizes. Caller inputs, vector
headers, compiler/runtime storage, pool alignment and physical residency are
separate from typed payload. Host vectors remain ordinary, unpinned memory.

The eight CPU modes retain full manifests, numerical shapes, prepared binding
bytes, descriptor bytes, update upload/readback bytes, before/after native
counters, linked C++ allocation observations and setup/steady timing. A trial
tick report reads 160 counter bytes and 64 focal bytes. Full physical inspection
additionally reads flags/counts/state, `(3P+6G)*8` motion bytes, 88 focal bytes,
and the selected body/group/speed/universal data; CpuDirect needs no body array
copy. Geometry result readback uses its actual four-element selected width.
Explicit query and inspection transfers are separate from arithmetic work.

The optional viewport accepts tagged double-valued ActiveReport data, records
profile/bits in capture metadata, and preserves sub-f32 values through native
JSON and browser validation. Old snapshots without metadata retain f32
validation. ActiveSession/FixedStudy still construct legacy executions: this
bounded value/report support does not claim an operational f64 owner lifecycle.
WebGL rendering precision is not authoritative engine precision.

## Evidence and remaining work

See the [retained CPU run](../tests/numerical_profile/README.md) for pins, raw
results and measurement limits. Reproduce after the normal native build with:

```sh
ctest --test-dir engine/build -R '^numerical_profile_' --output-on-failure -j2
ctest --test-dir engine/build --output-on-failure -j2
cd tools/viewport
npm test
npm run build
```

C1 supplies independent single-level live fields only. Live child-to-parent
propagation, same-type consume/reactivation safety, inherited/downward response,
multi-depth settling/recovery and matching/exposure remain the following
bounded contract. The full candidate solve is still optional, not a mandatory
per-tick barrier. #172/#177/#156/#145 remain open; GPU acceptance remains after
the CPU operation matrix. No database work is included.

