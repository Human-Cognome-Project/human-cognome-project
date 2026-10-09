# NAPIER system guide

**Status (2026-10-04):** Integrated account of the architecture discussed with
Patrick and the active repository. It explains intended behaviour; the status
table below separates that intent from running code. The dated
[discussion record](napier-system-discussion.md) preserves the derivation and
open alternatives. For the native build boundary, read
[engine/ARCHITECTURE.md](../engine/ARCHITECTURE.md).

## Conceptual LoD and compression depth (Patrick, 2026-10-09)

Conceptual scale is tied to the **primary SNode structure** of the study.
Secondary SNode structures attached within one conceptual LoD inherit that
LoD's operative distance scale throughout their internal compression elements.
Expanding a compression rollup is **1:1**: the environment retains its current
granularity while that particular representation is expanded. It does not
rebase the environment or introduce another 64/128 scale step.

For example, a legal document literal can reference deduplicated boilerplate.
All expansion steps of that boilerplate remain within the document literal's
SNode object at the same conceptual distance scale, including nested dedupe.
A conceptual transition is expressed through the primary study structure;
storage depth, secondary-tree depth and the names paragraph/phrase/character
do not independently create one.

This settles the model distinction and structural direction. Mapping primary
and secondary roles onto native Taichi layouts remains implementation work:
SNode nesting and activation provide storage machinery, not an intrinsic
conceptual-scale or physical-distance policy. A secondary attachment does not
imply arbitrary shared physical subtree pointers in Taichi.

The proposed variable-distance implementation gives compression-only
transitions ratio 1 and conceptual transitions their calibrated scale factors.
Whether those factors use a common scalar with view conversion or additional
relationship-dependent multiples remains open (#106, #115).

## Component-derived particle extent (Patrick, 2026-10-08)

Particle sizes are relative: prepare an enclosing extent sufficient to hold
the component elements without added gaps. This is primarily a setup
calculation for each centroid/object, with reuse while its defining inputs
remain unchanged. It supersedes the fixed-diameter-for-every-object rule;
one represented particle need not have the same diameter as another.

Patrick's intended example is that a single and double hex couplet have
essentially the same extent, with a nominal radius adjustment for the centroid
at the common point; triples and quadruples are larger. The exact radial
construction and packing calculation remain to be specified and checked.
Do not substitute a summed-volume formula or assert exact sphere tiling.

Keep enclosing extent, structural mass and operative distance scale distinct.
The finest-visible particle-unit convention anchors measurement; it no longer
requires all represented objects to occupy identical physical extents.
A compressed object remains a spherical Markov blanket with proportional
whole-particle response. Computing its extent does not restore internal force
application offsets or rotation.

Conversion sequence and verification: [model realignment plan](model-realignment-plan.md).

## Corona-disabled test direction (Patrick, 2026-10-08)

Test with the **corona effect disabled**. Particles may come to rest in ordinary
body contact; competing field forces should separate them where appropriate.
Remove imposed corona spacing, overlap/abut regimes and corona-derived gaps
from the experimental model. Retain ordinary contact.

The proposed explanation for earlier difficulties is the limited size of the
early modeled structures. Test that hypothesis in larger, more connected
environments; it is not an established result. Assess settled contact,
field-driven separation, cohesion, numerical stability and settling with the
LoD-scaled, whole-particle response model.

This is a test direction, not a claim that the native implementation has already
changed or that permanent removal is validated. Historical corona implementation
and results below describe the baseline only. The old corona separation ladder
is not an acceptance requirement for the disabled experiment. Runtime work and
comparative evidence are tracked in #119.

## Whole-particle response and focus-relative distance (Patrick, 2026-10-08)

**Settled model direction; implementation reconciliation remains pending.**
Parents retain proportional participation: the participating parent mass/share
sets its contribution, and that contribution always acts on the whole particle.
Preserve ordered composition and repeated parent contributions. Remove positional
response, internal application offsets, torque, rotation and rotary alignment
from the requirements; there is no rotational reference to calculate against.

When finer LoD is exposed, its components operate normally as particles.
When it is compressed, the object is a **marble-shaped Markov blanket**:
the boundary inputs and outputs are the relevant points of interaction.
Do not calculate hidden internal geometry or orientation to apply a parent
response. This does not remove proportional parent participation.

**Distance is relative to the current focus of study.** An object distant in
LoD from one perspective can be directly adjacent from another. Warm-cache
SNode definitions and their composition must support this study-relative
expression; a cold object identity does not have one universal fixed distance
or permanently assigned operative LoD. Preserve the finest-visible unit baseline,
equivalence compression and consistent scale conversion within each view.

The possible relationship-dependent distance multiples/degrees of separation
must be interpreted within that focus. Test 64 initially, consider higher/lower
multiples where needed, and monitor whether these belong in warm SNode
definitions. Their exact representation remains open (#106, #115).
Changing study focus must retain identity and proportional participation while
recomposing the exposed relationships and their operative distances.

## Purpose and division of work

The project joins a lossless archive of reality **as humans can encode it**
with NAPIER, a digital cognitive engine meant to navigate, aggregate, and help
analyze that archive. The archive's eventual cold shard swarm shares connections
among instances; its role is analogous to a collective subconscious. This
network is a long-term direction, not a running swarm in this repository.

The cache manager prepares study-oriented **warm** structures from durable
records. The future **analyst** chooses and assembles a **hot** working structure,
controls the engine through a harness, and interprets numerical output. The
field **model** evaluates interacting factors across many ticks. In the human
analogy, the analyst is the conscious consideration and the model the
superconscious evaluation of the current thought. The analogy describes roles;
the analyst's functions have not been designed or implemented.

Independent kernels handle requests, record operations, deferred work, and
future swarm activity at their own cadences. Endpoints connect them. The
**engine harness controls field physics**; it does not take over the database,
WAL, topology, or kernel scheduler. The modified Taichi fork provides the
portable compiler/runtime/device substrate, native C++ `engine_support`
wraps it, and native field code supplies current tick mechanics. See
[kernel activation](../network/KERNEL-ACTIVATION.md) for the separate
coordination layer.

## Archive foundation

The [architecture](architecture.md) starts from a four-bit undefined base
particle and a defined two-nibble byte as the smallest relational unit.
It treats compound tokens as definitions by parts, addressed by arrays of
two-character pairs; dotted addresses are a human-facing rendering. The
built codec uses byte-ordered Base62 while keeping the literal pairs in
`text[] COLLATE "C"` (see the [paired-address decision](address-encoding-transition.md)).
The built PostgreSQL token schema uses the address array as `token_id`, with
direct composition and membership links. The field substrate's proposed
undefined-particle intake and the full archival ingestion path should not
be inferred from that record schema alone.

The [data protocol](data-protocol.md) sets an archival intent of admitting
what humans encode while preserving source, provenance, read-status and
unread slots. Source quality controls *ordering* and annotation, rather
than silently dropping lower-quality material. Lossless here means that
compression and aggregation remain traceable to what was recorded; the
model's provisional address or partial hot perspective does not replace
the archived record. The [physics basis](physics-basis.md) gives the
project's research rationale for balancing, distinct from validation of
the running model.

## Three forms of the data

An SNode tree is a working object definition: it can describe itself and
participate in more complex compositions. Its root represents the question
being asked. The cold store holds reusable atomic and composed object
definitions and their direct links; it does not materialize every fully
expanded universe-level tree. A composed element can occur at more than one
locus of a working tree without creating a new underlying token identity.

**Physics object design is the work itself.** The cache manager starts with
the finest relevant components and composes upward into study-shaped objects
and rollups toward the selected root. The large SNode structures supported
by the modified Taichi fork serve these working object rollups, not a giant
pre-expanded hierarchy in PostgreSQL. A vast physics world of planets, ships, cities, characters, and
components is composed from reusable standard objects; its current manipulable levels
are the ones exposed in the working structure. This describes the intended
cache-to-engine bridge, not an implemented cache manager.

| Form | Owner and use | LoD consequence |
|---|---|---|
| Main databases / cold archive | Durable tokens, compositions, memberships, and discovered connections; the raw object library. | Direct relationships remain followable at each stored level. |
| Warm cache | Cache manager projects relevant dimensions into prepared SNode pieces and trees for a field of study. | It chooses nested detail or compressed aggregates according to the study and system budget. |
| Hot memory / GPU working structure | Future analyst assembles a current model from warm pieces; harness loads the active field state. | Its SNode tree exposes detail near the inquiry and coarse perspectives elsewhere. |

The cache manager will treat `hcp_core` as an always-relevant core database and
access other databases according to the study. The 2026-09-28 encoding-floor
snapshot is the initial candidate core to inspect and expand; the full
multi-database runtime remains under development.

For an encoding-table study, **Encoding tables** could be the root. Table
memberships, vendor/source and format groupings, and represented byte values
must fit beneath that root through progressively specific steps. The store
retains the direct edges; the warm projection chooses which of those steps are
expanded. **Nested** means a construct can be broken down for this inquiry;
**compressed** means its appropriate aggregate stands in at this LoD. Neither
means the archived details have been lost. Available device memory and system
settings constrain how many levels can be resident at once, changing the
tradeoff between scope and granularity. The view-spec interface and policy
are still to be derived. See
[working set and ledger](../kernels/database/WORKING-SET-AND-LEDGER.md).

Cold/warm/hot names describe preparation and residence. **Global/local** names
describe visibility and address scope; they are a separate axis. Each future
NAPIER instance has a reserved private address block for its local history and
relationships. `personality.db` and `relationships.db` are provisional names,
not deployed schemas. Personal notes, interaction history, and differentiated
expression belong there. Private records stay within the instance even if they
inform its warm or hot view. Global changes may create followup work **into** a
local store; local changes do not ordinarily write directly **out** to the
global store or another instance. A separately governed release path may share
significant derived results; its safeguards, encryption, and key lifecycle
remain to be designed. See [instance-local data](instance-local-data.md).

## Operative distance follows expressed LoD (Patrick, 2026-10-07)

**Settled model intent; not yet implemented or runtime-verified.** The most
granular LoD visible in the current modeling operation defines **1 particle =
1 unit of distance**. Each successively less granular LoD expressed in that
operation is a factored expansion. The proposed factor is **64 or 128**
(power-of-two); the choice between them remains open. For factor B, expressed
scale steps k use B^k units relative to the finest visible baseline.

Stored parent depth, address depth and SNode nesting do not automatically
increment k. Compressed equivalences can omit a distance step. In particular,
the UTF endpoint to the character in use is an equivalence: a name change of
scale in measurement, not necessarily another full expansion.

When the analyst zooms in to expose the next finer operative LoD, that level
becomes the one-unit baseline and the previously viewed level moves up one
unit equivalence (one factored scale step). This changes the measurement
frame; it does not redefine cold identity or ordered composition.

Literal composition preserves ordered parent occurrences, including repeats.
It does **not** require a spatial string through the literals, a straight line
of touching beads, or a chain folded inside a unit sphere. The model uses
molecular-level operative distances and higher/lower attraction. Ordinal
composition order remains data; it is not itself a distance coordinate.

The C++ bridge and field evaluation must carry the expressed scale consistently
through positions, centroid geometry and operative distance.
The existing inverse-square law remains the law; no additional force multiplier,
mass rescaling, or zoom-dependent change to physical relationships is specified
by this clarification. Exact conversion of the numerical state and movement
units is implementation work to verify, not a formula supplied here.

Implementation is tracked in [#115](https://github.com/Human-Cognome-Project/human-cognome-project/issues/115), with the C++ bridge in [#106](https://github.com/Human-Cognome-Project/human-cognome-project/issues/106).
Whitespace discussion is tracked in [#116](https://github.com/Human-Cognome-Project/human-cognome-project/issues/116).

### Local cohesion, dispersion and calibration (Patrick, 2026-10-08)

Distance scaling supplies the intended shear resistance at every expressed
level. Component distances are multiplicatively smaller than macro distances,
so their inverse-square attractive forces are correspondingly stronger.
Ordered composition survives without beads on a string, flexible-joint
machinery, or an additional shear-specific protective gate.

**Limited scope is an effect of distance and dispersion, not a scope mask.**
The fact that the entire model is composed of hex codes has no meaningful
general effect on an individual hex code when those occurrences are dispersed;
nearby configuration dominates at that scale. This repeats at each level up.
Do not implement an artificial construct boundary or disable universal fields
to achieve this. This is the intended model behavior to verify under load,
not a claim that dispersion mathematically guarantees cancellation.

A **binary (power-of-two) factor** is preferred for clean, fast arithmetic.
Test **64** first; **128** is the next candidate if needed. The requirement is
sufficient local force scaling while permitting universal effects. As the
model grows, linear distance grows too; monitor both local cohesion and
aggregate distant effects rather than calibrating on one small assembly.

**Monitoring / discussion, not a schema decision:** degrees of separation
means that different kinds of relationships may need higher or lower distance
multiples to produce the needed effects. One universal factor may not suffice.
Those relationship-dependent separations may need to be part of warm-cache
definitions of SNode objects. This is not graph-hop count, parent/address depth
or raw SNode nesting. Test 64 as the initial binary factor, then assess the
multiples needed by each relationship kind; no per-kind values, field layout
or conversion formula are fixed yet. Track in #115 and the bridge issue #106.

### Whitespace: sparse recording and local connection (Patrick, 2026-10-07)

**Storage rule settled:** whitespace is always excluded as a recorded token;
it exists through gaps in the representation. Each represented token retains
its representative place. Positions with no recorded value are spaces, so
multiple empty positions preserve multiple spaces; excluding values must not
compact the positional sequence.

**Model rule settled; instantiation remains a choice:** a gap may be represented
by an inert whitespace particle. It has no direct semantic value and is relevant
as a separator/boundary condition on the internal structures it bridges.
Its only attractive participation is lower/higher positioning within that
particular string or construct: the connecting force that makes a molecule
a molecule rather than a collection of atoms. It has no field effect with any
other whitespace occurrence, including elsewhere in the same construct.

Whitespace participates in no parent/child relationships and contributes no
mass to any gathering. Do not add a shared whitespace identity field or a cold
whitespace-token row to obtain this model behavior. If instantiated, the local
connecting participation must remain distinct from structural parent/child
participation. This settles its role, not an independent force formula or a
numerical inertial mass for integration; do not assume mass-zero division is
safe in existing kernels.

The separator's operative distance uses the expressed LoD metric. Positional
recording does not reinstate a straight spatial string through literals.
Implementation and remaining representation details are tracked in #116.



## Identity, fields, and traversal

`token_id` identifies a reusable token in the record library: the variable
representing a thing, atomic or composed. `particle_id` identifies one
instantiated particle in the loaded model. A dimension is an aspect that
makes the thing what it is or a measurable plane of commonality. These are
general terms, not LLM-specific vocabulary. Taichi's `SNode::id` names a
layout node, not a live particle; mapping instance IDs to Taichi cells/indices
is part of the C++ bridge. A token can have multiple simultaneously exposed
particles. All such instances automatically belong to
an always-applicable **same-token sibling field** in the intended model. This
field uses the ordinary particle-to-centroid law, with whole-mass participation;
it is not the database's former `token_sibling_group` table.

The durable graph has two direct, reciprocal relationship axes:

| Axis | Forward walk | Return walk | Role in a working model |
|---|---|---|---|
| Composition | Ordered `token_parent` occurrences, including each occurrence's mass; repeated parent identities keep separate positions. | `token_child` lists composites using a parent. | Parents provide the next possible LoD and proportional contributions acting on the whole particle. |
| Group membership | `member_of` lists direct groups a token participates in. | `members` lists each group's direct participants. | Group centroid fields contribute at the exposed level; groups may themselves join higher groups. |

The reverse lists make future n-dimensional traversal an address follow instead
of a search for unseen back edges. For example, byte value `01` may belong
directly to several encoding tables; each table can belong to vendor/source
and format groups. The value's direct memberships and each table's higher
classifications are separate links. The cache manager composes them under the
study root. Repeated parents remain distinct proportional contributions to
whole-particle response. Positional response and rotary alignment are removed
from the intended model. See the [database guide](../kernels/database/WORKING-SET-AND-LEDGER.md)
and [field model](../engine/docs/ACTIVE-FIELD-MODEL.md).

## From prepared view to numerical evaluation

1. The cache manager follows explicit cold links and prepares a warm,
   study-rooted projection. The analyst's requested focus and system limits
   jointly determine its eventual view specification.
2. The analyst eventually assembles hot particles and exposed fields. Its
   address suggestions are provisional because its view contains only part of
   the direct data; the cache manager owns actual cold-store placement.
3. The harness stages the model and seeds inclusive centroids. On initial
   ticks a new base may move more and cost more while its geometry settles.
4. Each active particle's exposed fields read previously published centroids;
   their inverse-square directed pulls combine into a resultant. The model
   advances positions, applies a discretization brake, and places affected
   centroids **at tick end** for the next tick. Changes may propagate along
   memberships over several ticks.
5. The analyst reads selected raw numerical results through the harness and
   can adjust its view. A browser rendering is only a human inspection aid;
   its refresh rate does not define the model's tick or the analyst's read
   cadence. Selective result readback is intended; current `download()` reads
   whole arrays on demand.

The intended optimization rule is **calculate only what changes, and reuse each
result**. A changed interaction supplies both its particle's pull and a cheap
bidirectional *would this centroid effectively move?* decision. Any such
decision marks that centroid for one end-of-tick placement; a stable centroid
need not schedule outward work until a tracked change wakes it. Unchanged mass
can be reused until a member or its mass changes. Unresolved particles visit
their active exposed fields; resolved ones can sleep only while a valid wake
path preserves their necessary work. This shrinking active set is **not yet in
the field harness**. [Active field model](../engine/docs/ACTIVE-FIELD-MODEL.md)
records the exact built order and pending formula choices.

The archive is a **pairwise discovery ledger**: store a discovered direct
connection and its effects once; later analyses follow it rather than repeat
discovery. A view may exclude effects it has not exposed **for that explicitly
scoped calculation**; the exclusion must be invalidated when its underlying
data, view, or parameters change. Per-tick negligible motion is enough to gate
that tick's centroid placement, but not by itself enough for a persistent
predicate exclusion. Full comparison among `n` items is `Θ(n²)`; the project's
`O(log N)` active-work aim, where `N` means the exposed fields across all
particles, still needs a precise operation, active-edge bound, and wake-cost
argument. No logarithmic total-tick guarantee is established.

## Durable followup and component activation

The WAL manager observes per-source database changes and records the reciprocal
paths and mass work still owed. Its own PostgreSQL `obligation` relation is the
durable outstanding set; a followup write closes an obligation when its report
is observed. Its History holds decoded footprints. A volatile box can notify
the cache manager of work without becoming the durable queue. The intended
**RECONCILE** request goes from analyst to WAL manager alone; the WAL manager
would select relevant existing obligations into the cache manager's normally
empty highest-priority box. The cache manager then writes the followups.
This live reconciliation path, and the real PostgreSQL WAL-to-report adapter,
are not built. See [report to work](../kernels/wal/REPORT-TO-WORK.md).

The endpoint monitor selects occupied boxes at fixed priority; the current
local C++ scheduler is cooperative. Future monitored runtime machinery can
activate secondary kernel sets on mailbox activity and scale primary workers
with demand, without changing the foundational calculation dependencies.
Remote bridges, setup topology, runtime balancing, and p2p/content tracking
are later work. The swarm is lowest priority relative to a working local
record → WAL → cache-manager → model loop.

## Implementation boundary and next decisions

| Area | In this repository now | Clarified intent / open seam |
|---|---|---|
| Operative LoD distance | No verified expressed-scale bridge or zoom conversion. | Finest visible LoD anchors one distance unit; less granular expressed levels expand by 64 or 128; compressed equivalences skip steps. Factor choice and implementation remain open. |
| Native field engine | Taichi-backed C++ particle/group/edge kernels, inclusive seeded centroids, field force, integration/brake, end-of-tick centroid publication, full on-demand download. | Combined destination and brake reach; proportional whole-particle parent response without application offsets; automatic sibling groups; conditional mass/centroid/particle activity; selective readback. |
| PostgreSQL records | Five-table direct reciprocal structure/membership schema, controller, command/dispatch and tier-2 fixture-driven DB-manager handler. | Study-root composition, warm view composer, provisional address handling, file-now/wire-later ownership, local private DB implementation. |
| WAL | Fixture-fed report recognition, per-source monitor, durable obligation/History tables, local endpoint handler emitting owed identities. | Live logical-decoding adapter, privacy-aware routing, cache-manager deferred consumer, RECONCILE staging, recovery/replay. |
| Network | In-memory boxes, endpoint registry, cooperative priority scheduler. | Runtime balancer, configuration/bridges, concurrency, swarm/tracker. |
| Analyst | No analyst cognition code. | Request/view contract, hot assembly, numerical inspection cadence and decision logic. |

The immediate design questions are how the field's directed vectors produce an
effective **destination distance** for the brake; what motion ratio is
significant at an active resolution; how repeated parent contributions are retained;
what makes a particle safely resolved and wakes it again; and how to measure
the claimed calculation savings. The local DB release/encryption design and
the exact live WAL adapter remain distinct design passes. The
[working record](napier-system-discussion.md) keeps derivations and historical
alternatives visible while these questions are settled.
