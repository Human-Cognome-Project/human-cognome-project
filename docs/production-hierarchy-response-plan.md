# Production hierarchy, response ownership and coupled recovery

Status: **plan accepted in #194, 2026-10-27; C3-1 accepted in #195**.
C3-2 is the next core implementation slice; see the
[current continuation sequence](core-local-study-continuation-plan.md).
#196's local CUDA/motion corrections are under review. Earlier evidence below
retains its original source and numerical contract. Baseline: `main` at
`33c4a4f50eacbcaaf0638e34cf22568dd9044473`, the merge of #193.
Owners: [#177](https://github.com/Human-Cognome-Project/human-cognome-project/issues/177)
and [#172](https://github.com/Human-Cognome-Project/human-cognome-project/issues/172).
Operational completion remains under #156; full GPU qualification remains under
#145 after the integrated CPU exit.

The [weighted native probe](../engine/docs/WEIGHTED-NATIVE-HANDOFF.md) is accepted
in #193. Hosted native CPU job 122025151768 passed 92/92 suites in 1600.20 seconds;
contributor evidence remains separately pinned at 930.44 seconds. This qualifies
held-input propagation on the tested CPU kernels. The following production
ownership, readers and moving-body coupling are accepted as the bounded plan in
#194; the fixed-tree CPU implementation was accepted in #195.

The proposed result is an explicitly installed, source-validated hierarchy in
the existing prepared f64 trial execution. Local and coarser responses reach the
same physical force accumulator; each body integrates once. Production here means
the engine's ordinary tick path, not completion of operational admission or owners.
Acceptance of this plan releases C3-1 below. C3-2 follows only after C3-1
acceptance, in a separate implementation PR. Merging this document changes no math.

## Acceptance clarification — 2026-10-27

Patrick's [coherent dynamics and GPU snapshot direction](coherent-dynamics-gpu-snapshots.md)
supersedes the universal moving-fixture settling requirement in section 6.
Exact resolution is an asymptotic ideal; continued motion alone is not a failure.
Mathematical correctness, publication/input lifetimes, held-input convergence,
state preservation and resource bounds remain hard requirements. Moving fixtures
must retain declared observation windows, independent checks and all recovery/reuse
rounds while reporting settled versus evolving separately. Historical failed runs
remain failed under their original criteria; revised acceptance needs new evidence.
The immediate bounded GPU observation milestone does not require completion of
all #156 operations. The current-hierarchy capture adapter is accepted in #195. #196 supplies
bounded contributor GTX 1070 execution evidence and backend corrections under
review; this is not full GPU operation/transfer qualification.

## Motion clarification — 2026-10-29

The [continuation plan](core-local-study-continuation-plan.md) records Patrick's
authorized wide-profile accepted-displacement motion change proposed in #196.
For positive dt, retain published displacement/dt; for zero dt, retain incoming
velocity. This supersedes earlier untrimmed-velocity intent for that profile.
C3-2 must preserve the current accepted motion/state at structural boundaries;
this does not authorize resetting survivors. Historical evidence remains pinned.

## 1. Installed ownership and supported subset

Use one supplied calculation tree in one prepared execution/source generation.
Memberships need not form a global tree. Reuse the weighted candidate's full
handle, source row/generation/ordinal, alias and exact coverage validation; extend
it with a prospective-source view for C3-2 rather than trusting query-supplied weights.

For occurrence i, retain its structural denominator D_i and validated integer
coverage C(n,i). Require `C(parent,i) = sum C(child,i)` with checked integer sums
and `0 < C(n,i) <= D_i` wherever present. Physical weight is current resident
`m_i * (f64(C(n,i)) / f64(D_i))`. Root participating mass need not equal universal
physical mass. Universal mass counts every physical occurrence once.

The initial live subset has these explicit bounds:

- One expressed comparison per distinct, present native field, identified by full
  field handle, comparison identity and declared conceptual level. Every present
  field has an owner in this tree; every present body participates in a leaf.
  Each node selects its owner's complete ordered source ledger after alias
  normalization. A partial selection from that ledger rejects in this subset.
- An internal node owns an upward publication and an inclusive comparison for its
  immediate children. A leaf owns its existing body-to-field comparison. The root
  has no external parent response. A centroid never becomes a physical particle.
- Repeated references to one source row coalesce. Distinct ordinals remain
  distinct local contributions; distinct occurrences of one definition remain
  distinct bodies. Compatible fractional overlap is supported, not normalized away.
- Anonymous `publish=false` grouping rejects before mutation. It cannot borrow an
  expressed field's output or silently introduce another Q/response boundary.
  Same-level **expressed** comparisons with distinct identities are supported.
  Anonymous arithmetic grouping remains a separately bounded extension.
- All numerical state uses the current common operative frame. There is no new
  distance multiplier on aggregation or response edges. Existing source-to-frame
  conversion occurs once. Requests needing an unreviewed conceptual transition
  conversion reject; record/SNode depth and skipped level labels select no factor.
- Use the existing immutable f64 body profile and an explicit experimental Q_b
  setting for each expressed comparison, 0..52. Keep finite/FTZ constraints and
  untrimmed mass. No density selector, precision ladder or production rounding
  policy is added. Legacy and uninstalled single-level executions remain selectable.

The live installer is separate from candidate binding: an invalid attempt must
not discard a healthy installed hierarchy. Candidate bind/query/inspection remain
read-only with respect to live publications, ownership and activity. A synchronized
uninstall, if supplied, must restore flat centres from current bodies and invalidate
their consumers before enabling flat forces; it cannot expose stale mirrors.

## 2. Proposed local and inherited force contract

This section records the downward equation accepted for the bounded C3 trial.
It applies the existing inclusive-centroid attraction law to immediate expressed
participants, then distributes the resulting **force**, not displacement or
velocity. It does not change the brake, radial damping or contact law.

Let c_n and M_n be a node's complete published centre and untrimmed participating
mass. For nonroot child n with parent p, define delta = c_p - c_n:

```text
F(n <- p) = G * M_n * M_p * delta / |delta|^3
w(n,i)    = current m_i * f64(C(n,i)) / f64(D_i)
F(i <- n,p) = (w(n,i) / M_n) * F(n <- p)
```

The producer is the directed comparison `(parent comparison, immediate child
comparison)`. Its recipient is the child's represented whole, of mass M_n.
Every covered body absorbs its distributed force through its **full physical
mass** at integration. Intermediate centroids have no independent velocity or
integration. For each ancestor edge, use that edge's child's coverage directly;
do not multiply participation shares again while walking through descendants.
The equivalent `w(n,i) * G * M_p * delta / |delta|^3` can avoid unnecessary
intermediates; implementation evidence must state operation order and error bounds.

Exact zero cases precede inverse-distance arithmetic: coincident centres, zero G,
no complementary parent coverage, or a parent containing only one distinct physical
occurrence produce zero. Complementary coverage is the checked integer difference
`C(p,i)-C(n,i)`, not rounded mass subtraction. The singleton gate extends C1's
protection against fictitious self-force from repeated ordinals and coarse Q.
An empty/nonpositive represented mass is invalid in this installed subset.

At a leaf, preserve C1's source-row force: participating row mass times inclusive
leaf mass over squared separation, directed from body to published leaf centre.
Apply the same exact coincidence/G and distinct-physical-occurrence exclusions.
Distinct ordinals retain canonical source order. The eligible cohort is the
validated selected ledger, not a guessed complement from floating mass.

### Direct versus inherited ownership

For an installed field, direct body-to-field forces survive **only at leaves**.
Suppress the old flat direct force for every internal field, including the root.
Each immediate child-to-parent influence replaces that internal field's direct
body path. A body receives its local leaf contributions and the distributed
contributions of every ancestor edge covering it. Compatible overlap may give
one body several distinct child influences; that is explicit participation.

Influence identity uses execution/context, full owning field handles and
comparison identities: leaf plus source-row ordinal for a local contribution;
parent plus immediate child for an inherited contribution. Repeated membership
paths to one influence do not add it again. Equal vectors are not duplicate
identity. A distributed ancestor edge is included once per recipient using summed
coverage for that occurrence, even when reached through several lower paths.
Do not retain an internal direct response alongside its replacement, integrate
an aggregate in addition to its exposed parts, or count centroid mass as universal mass.

Each active body gathers its owned influences in a declared stable order, adds
the existing contact forces, then executes the existing velocity update, overshoot
brake, radial damping and one Q_b position publication. Preserve physical velocity,
accepted displacement and final cached speed independently. Discard only the final
positional remainder. No force, incoming displacement or ancestor contribution is
individually trimmed at the receiving body.

### Numerical examples to make the review decision concrete

These are one-dimensional, pre-brake/pre-integration forces, with G=1, b=52 and
unit frame conversion. They illustrate the proposed ownership, not new calibration.

| Case | Published inputs | Expected response/accounting |
|---|---|---|
| Two levels | Leaf A has a: m=1,x=0 and b: m=1,x=2, so M_A=2,c_A=1. Leaf B has c: m=2,x=5, so M_B=2,c_B=5. Root M=4,c=3. | A receives +2 and B -2 from their root comparison. Distribution is a:+1,b:+1,c:-2. Local A forces are a:+2,b:-2; singleton B gives zero. Combined body forces are +3,-1,-2; accelerations are +3,-1,-1. Each body integrates once. No additional direct root attraction is applied. |
| Repeated ordinals | a has physical mass 6,D=3 and two distinct count-one rows; b has mass 2,D=1 and count one. The recipient cohort weighs 4+2=6. | An incoming whole-cohort force +12 distributes a:+8,b:+4. Both a ordinals remain in local arithmetic; an alias of either row adds nothing. Physical universal mass is 8, not participating mass 6. A separate occurrence of a has its own coverage/state. |
| Compatible overlap | a: m=1,D=1,x=0; b: m=2,D=2,x=2; c: m=1,D=1,x=4. A selects a=1,b=1; B selects b=1,c=1; root selects a=1,b=2,c=1. A is (c=1,M=2), B (c=3,M=2), root (c=2,M=4). | Root influences are A:+8,B:-8. A distributes a:+4,b:+4; B distributes b:-4,c:-4. b's inherited net is zero, not a deduplicated whole-body +8 or -8. Root mass and universal mass are both 4 here. Two children each selecting b=2 reject. |

For a third level above that overlap, a force +16 on its root of mass 4 adds
a:+4,b:+8,c:+4 exactly once through root coverage. It is not separately delivered
in full through A and B. In every example, changing a current physical mass must
change participating weights without changing its structural denominator.

## 3. Ordinary tick and complete publication lifetime

Keep one existing particle NativeSet and one existing centroid NativeSet.
An installed node uses its owner's native field record; no probe activity tree
survives into production. Body movement declares the affected leaf bits through
prepared source adjacency. Parents receive native notifications after publication.

1. At tick entry, body/focal consumers read the preceding complete publications.
   No hierarchy writer runs during force/contact/integration. Combine all physical
   contributions and consume bodies under C2b-1; later notifications remain pending.
2. Finish existing contact propagation, with both endpoint directions. Body state
   is then immutable for the entire centroid phase, including sleeping sources.
3. Traverse the existing centroid NativeSet once. Consume a node's bit **at entry**;
   acquire its own and child selectors with the qualified native SC RMW operations.
   Leaves reduce current accepted body inputs; parents reduce complete published
   child XYZ/mass, with anchored f64 arithmetic and Q once at this destination.
4. On changed XYZ **or mass**, retain the old complete publication/history, write
   all four inactive words, exchange the selector, then notify the parent and
   perform the recipient input checks below. A new activation may be visited now
   or next tick. No later propagation-only phase consumes a centroid notification.
5. Complete all readers and synchronize before another poll, structural writer,
   inspection operation or slot reuse. Update completed-tick provenance normally;
   it never chooses work. No per-depth dispatch loop or full candidate query runs
   as the ordinary scheduler.

One writer and at most one publication per node per poll are hard obligations.
Two slots suffice only with this reader completion cut: every acquired packet
remains immutable until its reader finishes, including the former selected slot.
The old slot is not spare scratch immediately after exchange. Parent reads need
not form a simultaneous sibling snapshot; later notifications must preserve a
further opportunity to calculate. No lock, spin, scheduling epoch or second mask.

### Sleeping recipients and pending input

Node coverage supplies potential recipients; reverse body-to-influence adjacency
supplies each recipient's complete local/inherited input set. Within a publishing
node's visit, check the change in the recipient's **combined** eligible attraction
using acquired complete packets, holding all other acquired inputs equal and
substituting this node's old/new publication. Bodies are fixed during this phase.
Do not wake all members solely because a centroid's coordinate changed, or Q each
field contribution separately. Exact zero/cancellation and singleton exclusions
must be exercised. A consequential input change activates the existing particle
bit for its next phase; that phase evaluates all connected responses plus contacts.

These paired checks are numerical input checks, not a second physical integration
or an awake registry. Concurrent node publications may cause conservative extra
wakes; every later publication performs its own check. Check work and repeated
recipient visits are charged separately from physical force work. If a future
optimization previews Q at a combined body destination, it must preserve pending
input semantics rather than treating subthreshold force as nonexistent.
An inconclusive check preserves the recipient's input invalidation through its
existing native bit; it cannot declare the input harmless. Ordered multi-publisher
controls must demonstrate that cancellation checks cannot strand a changed final
combined response behind an empty particle mask.

Mass/count/source/contact changes have independent invalidation even when XYZ or
the paired force is unchanged. Focus change retains the existing radial-origin
invalidation when applicable. Physical velocity is not zeroed to make masks quiet.
Preserve contact notifications and all accepted displacement exactly once.

### Reader and writer inventory

The implementation must name and qualify the actual compiled specialization of
every row below. This table fixes the data authority; merely reusing the probe's
aggregate atomic count is insufficient.

| Production path and current source | Proposed lifetime and access |
|---|---|
| Centroid own/child reduction and recipient checks; new installed path beside [field.cpp](../engine/src/field/field.cpp) / [activity.cpp](../engine/src/field/activity.cpp) | Concurrent readers acquire selectors with SC RMW; complete slots live through poll completion. Recipient paired checks use slots, never the concurrently written flat mirror. Consume only at numerical entry. |
| Body force and inherited gather, `launch_force` | Acquire complete child/parent packets while hierarchy writers are absent; all values live through the body phase. No hidden flat direct internal-field force. |
| Clear/mark/reduce/finalize and seed, `launch_clear` / `launch_centroid` | The installed path is the sole writer of bound field publications. Old flat reduction/clear/finalize must not overwrite them or their history. Initialize both slots completely before enabling the installed tick. |
| Focal origin in `Activity::begin`; [geometry preflight](../engine/src/field/prepared_geometry.cpp) | Read a synchronized end-of-poll mirror of selected XYZ/mass. No additional Q at combined focus. Geometry still reads current bodies, including sleepers; read-only preflight installs no contacts. |
| [Prepared candidate repair/commit](../engine/src/field/prepared_update.cpp) | C3-1 rejects nonempty edits before mutation while installed. C3-2 validates prospective ownership and numerical repairs together, then publishes under exclusive synchronization. No old flat repair of a hierarchy-owned field. |
| Download, `inspect_prepared`, candidate/reference query and report/capture adapters | Synchronized observation only. Copy selected complete publications into reserved typed field/readback surfaces; staging is not an authority or an upload source. Candidate queries have separate scratch/results and cannot repair live values. |
| Focus configuration, frame anchor and future rebase | Supplied prepared focus uses the same completed mirror. Existing unsupported focus replacement/rebase remains rejected before mutation. A future rebase must translate both slots, retained old XYZ, mirrors, bodies and focal controls together, or reject; mass, velocities and displacement vectors do not translate. |
| Reset, republish, replacement and teardown | No public upload/seed/reset can bypass installation ownership. Reject unsupported calls before mutation. Replacement waits for all readers; teardown drains bindings before arrays/runtime. |

For control/report compatibility, the existing flat group XYZ/mass becomes a
projection of each selected live packet, written by its sole node writer and
read only after completion. Its accumulator rows are not a hidden source of
discarded moments. Retain a distinct old XYZ/mass history surface; existing
`group_delta[3..5]` may project old XYZ for compatible observations. Do not clear
history with reduction scratch or treat accepted displacement as positional residue.
History survives until its numerical/wake consumers finish. Mass-only publication
retains old mass too. Ordinary rejection preserves history and pending bits.

Audit retained LLVM for **each actual selector access path**, including indirect
child/ancestor/recipient loops and compiler clones/epilogues. Attribute argument
bases and index roles; fail on ordinary, unknown, missing or demoted selector
accesses. Exclusively synchronized initialization/inspection stores are separately
identified and cannot appear on concurrent paths. Qualify one/two CPU workers and
both host paths. The probe does not qualify arbitrary production kernels or CUDA;
reject installation on unqualified backends. A required fork change needs its own
review, rather than silently replacing the native protocol.

## 4. Initialization, structural changes and failure

Initial installation validates ownership, coverage, source generation, response
adjacency, precision, domain and all resource/work bounds before committed writes.
Seed all expressed nodes bottom-up from current accepted bodies and published
children into reserved prospective slots. This one-time ordered calculation is
permitted; it is not the ordinary tick algorithm. Commit readiness only with both
slots, selectors, mirror, history, source binding and response ownership coherent.
Wake physical inputs once for the changed force ownership. Do not reset survivors'
position, velocity, accepted displacement, speed, physical mass or contacts.
Retain each replaced flat publication as old history until its installation
invalidation is delivered; a genuinely new comparison has no prior publication.

C3-1 explicitly rejects every nonempty `update_prepared` while installed, including
count-only, retirement, contact and focus edits. It also rejects unsupported raw
publication/reseed. The old healthy execution remains usable. A source generation
mismatch prevents a body tick **before** motion; it cannot be repaired afterward.

C3-2 adds one bounded combined source/hierarchy refresh operation. Its input names
the expected committed generation, the existing structural command batch and a
complete replacement hierarchy specification. Existing handles or pending
definition-command references resolve against the validated prospective ledger;
source counts still come from that ledger, not asserted replacement weights.

- Validate the final identities, exact coverage, stable influence ownership,
  affected closure, claims, old-plus-new peaks, numerical repairs, focus and
  resource limits before live mutation. Same-batch retirement cannot fund claims.
- Preserve unchanged comparison slots, selectors, history and pending bits by full
  identity, even if their source rows are renumbered. Do not globally reseed the
  hierarchy to conceal lag. New/changed leaves use current accepted bodies;
  affected ancestors repair once from complete current/prospective child packets.
  Unaffected dirty children retain their publication and pending work.
- Commit source topology/counts/generation, field ownership, both repaired slots,
  mirror/history, adjacency and required particle/centroid invalidation together
  under the exclusive owner boundary. No body tick can see half a refresh.
  Metadata-only changes still invalidate newly consequential response inputs.
- New comparisons start with complete equal slots and appropriate initial history;
  retired/recycled records retain no prior influence, selector or history. Surviving
  physical motion/state and independent copies are preserved. Pending work is
  preserved or covered by explicit new-generation invalidation, never lost by reset.
- Rejection before mutation preserves readiness, publications, source generation,
  Demand, frame/focus, contacts, motion and activity. A backend/invariant failure
  after claims or native writes makes the execution unready; no rollback is claimed.

The existing fixture start supplier remains test-only for insertion/replacement
recovery. It neither introduces recipe positions nor implements #170 nearest-free
admission, moving-contact transactions, operational focus/rebase or enduring owners.
Later production insertion must incorporate this combined commit and #170's
candidate-motion/contact validation, not mutate a source underneath a live tree.

## 5. Prepared storage and cost contract

Reserve before installation: H comparison slots, L child links, S selected source
rows, U unique node/occurrence coverage entries and R body/influence references.
R includes ordered local rows and one entry per covered physical occurrence per
ancestor edge; shared lower paths do not duplicate an entry. Its size/work must be
bounded explicitly, not assumed equal to S or hidden in a recursive traversal.
Reuse appropriate existing Cascade budgets and add a response-reference/work
reservation if their bounds do not cover R. No payload scales with potential
address space beyond already prepared native declarations.

The implementation manifest must account for these simultaneous surfaces:

| Surface | Reservation obligation |
|---|---|
| Live publication | Two f64 XYZ/mass slots plus i32 selector: **68H bytes** per physical copy; not the old four-word group alone. |
| Old publication/history | f64 XYZ/mass: **32H bytes** per owned copy, retained through its consumers; old group history/mirrors remain charged if still allocated. |
| Binding/topology/precision | Full handles/context/levels, field-to-node ownership, parent/child and source CSR, per-node Q/eligibility; actual ABI widths, sentinels and capacities. |
| Source and response identity | Exact counts/denominators, source row/generation/ordinal, U coverage entries, node-to-recipient and body-to-influence adjacency including R; do not charge just raw body IDs. |
| Prospective refresh | Simultaneous committed and prospective topology, ownership, response indexes, source validation and publication/history repair. Charge both host/native banks actually retained, including old/new native claim peak. |
| Observation and control | Typed selected/old mirrors, status/Demand, candidate reference and geometry scratch, readback/upload buffers, binding contexts and diagnostic allocations. Existing candidate/probe storage is not free reusable production backing. |

Do not add a persistent force/awake cache by default. Bodies can gather the
prevalidated influence list into their existing force accumulator. Charge the
repeated producer and paired-wake arithmetic honestly. Any proposed response
cache changes publication, invalidation and peak storage and requires an explicit
revision to this contract. Source metadata and numerical history are not activity.

Warm kernels, prepared call contexts, native pointer blocks and list high-water
marks before measuring; count setup, initial installation, first tick, ordinary
ticks and structural refresh separately. Require zero observed recurring linked
C++ new/new[] and stable known native backing/layout/compiler counters through
32 tick/recovery/refresh reuse rounds. Report arithmetic, selector/bit/adjacency
work, validation and transfers separately. Copy every owned host/native surface
into the manifest exactly once, with actual `sizeof` and retained binding bytes;
include caller inputs/fixed headers/compiler/runtime pools separately. Existing
observations do not qualify arbitrary malloc, driver residency, page faults or
pinned host memory. No compile, layout rebuild, allocation or implicit expansion
belongs in ordinary ticks or within-capacity refresh.

## 6. Evidence and implementation sequence

### C3-1: fixed installed tree with coupled response

Implement the supported installed subset, complete reader/history contract,
replacement force ownership, native centroid poll and body recipient checks as
one coherent opt-in trial path. Do not publish an intermediate mode that reads
mixed flat/hierarchical fields or adds inherited force to old internal direct force.
Unsupported structural edits reject before mutation. Submit source-pinned CPU
evidence and the separate implementation PR; acceptance is required before C3-2.

Required evidence includes:

- Exact examples above, an independent f64/high-precision force reference and
  one combined physical integration reference with the existing brake/damping.
  Cover non-unit physical masses, thirds, repeated ordinals/aliases, independent
  copies, compatible overlap, partial root coverage and singleton coarse Q.
  Include native u64 counts above 2^53 with independently bounded conversion error.
- Complete initialization, two/three levels, forward/reverse chains, fan-in,
  permuted rows, skipped labels, 1:1 expressed contexts, holes, word/block edges
  and final valid records. Reject aliases, anonymous grouping, partial owner
  ledgers, stale/foreign/recycled references, cycles and incompatible coverage.
- Every production compiled selector path and acquired packet lifetime, including
  the recipient checks and history. Repeat before/after-consume collision controls,
  mass-only publication, duplicate notifications and the original unsafe controls.
  Force concurrent parent/child and sibling publication orders around paired
  recipient checks, including cancelling intermediates with consequential finals.
- Individually subthreshold contributions crossing a destination cumulatively,
  cancellation, unequal body/centroid Q, and discarded final tails. Verify all
  applicable forces combine before body Q and published children before parent Q.
- Read-only geometry/focal/inspection consistency, candidate-query isolation,
  rejection preservation, post-mutation unready failures and protected reset/reseed.
  No stale cached speed, all-member wake shortcut or suppression of contact inputs.

For held bodies, retain the probe's **16 ordinary poll** limit and **eight quiet
poll** observation, checking reference values as well as activity. For moving
fixtures, predeclare starts, intrinsic test extents, parameters and retained bits;
use an initial **4096 ordinary tick** limit followed by **32 observation ticks**.
Selected small letter/byte fixtures must exercise nonzero local and inherited
forces, contact and radial damping, multiple active depths and later sleeper input.

The moving criterion is stable published body positions and hierarchy XYZ/mass,
no consequential pending native work, and agreement of the last evaluated body
publication and settled hierarchical values with independent references. Retain
physical velocities, force/attempt diagnostics, oscillation/reversal, ticks/time
and active particle/centroid/adjacency work. Quantized positional/computational
settling does not imply zero velocity or zero physical force under C2b-1. No
epsilon or velocity clearing selects sleep. A quiet mask with stale values fails.
Report a limit reached as a failure of that declared fixture's settling gate;
do not loosen parameters after the run to rename it a pass. No universal
trajectory equality or complete same-tick cascade is required.

Repeat coupled settling after local/deep physical perturbations through the
bounded test adapter, without resetting publications. Use selected frozen
candidate/reference observations, never a mandatory per-tick full solve.

### C3-2: coherent source/hierarchy refresh and recovery

After C3-1 acceptance, implement the combined refresh from section 4 in its own
PR. Retain the C3-1 numerical/lifetime contract. Add count-only edits invisible
to f32 share, mass-only fixture changes, source row reorder, contact changes,
insertion/retirement/reuse, changed comparisons and root coverage while ticking.
Require ordinary rejection preservation and survivor/history/pending-input
preservation across successful changes. Exercise simultaneous prospective/committed
capacity exhaustion and post-mutation failure. Repeat the declared multilevel
settling/recovery and 32 reuse rounds after each supported operation; insertion
uses labelled fixture starts until #170's operational admission is implemented.

Both segments run the full existing native CPU regression (currently 92 suites)
plus their new cases, with actual CPU selection on both host paths and one/two
workers. Retain source/backend/compiler pins, raw results, resource inventories
and the per-path compiler audit. Hosted acceptance remains separate from
contributor runs. Browser validation/build is required only if report/capture
interfaces change; rendering is observational. Do not write Python engine tools
or use retained databases as mutable test fixtures.

This plan leaves general matching/exposure, anonymous grouping, transition/rounding
calibration, #156 operational admission/contact/rebase/owner completion and #145
GPU execution/pinning qualification open. It adds no database, cache-manager,
analyst or swarm implementation and closes none of those owning issues.
