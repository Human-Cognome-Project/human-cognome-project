# NAPIER system guide

**Status (2026-09-26):** Integrated account of the architecture discussed with
Patrick and the active repository. It explains intended behaviour; the status
table below separates that intent from running code. The dated
[discussion record](napier-system-discussion.md) preserves the derivation and
open alternatives. For the native build boundary, read
[engine/ARCHITECTURE.md](../engine/ARCHITECTURE.md).

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
built codec still uses base-50; the [primary address transition](address-encoding-transition.md)
adopts the RFC 4648 §5 URL-safe Base64 alphabet while keeping the pairs.
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
being asked. The cold store holds explicit object pieces and direct links,
not one fixed tree for all questions. A composed element can occur at more
than one locus of a working tree without creating a new underlying identity.

| Form | Owner and use | LoD consequence |
|---|---|---|
| Main databases / cold archive | Durable tokens, compositions, memberships, and discovered connections; the raw object library. | Direct relationships remain followable at each stored level. |
| Warm cache | Cache manager projects relevant dimensions into prepared SNode pieces and trees for a field of study. | It chooses nested detail or compressed aggregates according to the study and system budget. |
| Hot memory / GPU working structure | Future analyst assembles a current model from warm pieces; harness loads the active field state. | Its SNode tree exposes detail near the inquiry and coarse perspectives elsewhere. |

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

## Identity, fields, and traversal

`token_id` is the base token identity and stored address; `particle_id` is an
allocated instance of that token in a loaded model. A token can have multiple
simultaneously exposed particles. All such instances automatically belong to
an always-applicable **same-token sibling field** in the intended model. This
field uses the ordinary particle-to-centroid law, with whole-mass participation;
it is not the database's former `token_sibling_group` table.

The durable graph has two direct, reciprocal relationship axes:

| Axis | Forward walk | Return walk | Role in a working model |
|---|---|---|---|
| Composition | Ordered `token_parent` occurrences, including each occurrence's mass; repeated parent identities keep separate positions. | `token_child` lists composites using a parent. | Parents provide the next possible LoD and, through their order, an orientation for comparing like pieces. |
| Group membership | `member_of` lists direct groups a token participates in. | `members` lists each group's direct participants. | Group centroid fields contribute at the exposed level; groups may themselves join higher groups. |

The reverse lists make future n-dimensional traversal an address follow instead
of a search for unseen back edges. For example, byte value `01` may belong
directly to several encoding tables; each table can belong to vendor/source
and format groups. The value's direct memberships and each table's higher
classifications are separate links. The cache manager composes them under the
study root. Ordered duplicate parents remain distinct in the model's intended
partial-force and rotary-alignment effects; the current field harness has no
such orientation calculation. See the [database guide](../kernels/database/WORKING-SET-AND-LEDGER.md)
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
| Native field engine | Taichi-backed C++ particle/group/edge kernels, inclusive seeded centroids, field force, integration/brake, end-of-tick centroid publication, full on-demand download. | Combined destination and brake reach; ordered parent reorientation without carried spin; automatic sibling groups; conditional mass/centroid/particle activity; selective readback. |
| PostgreSQL records | Five-table direct reciprocal structure/membership schema, controller, command/dispatch and tier-2 fixture-driven DB-manager handler. | Study-root composition, warm view composer, provisional address handling, file-now/wire-later ownership, local private DB implementation. |
| WAL | Fixture-fed report recognition, per-source monitor, durable obligation/History tables, local endpoint handler emitting owed identities. | Live logical-decoding adapter, privacy-aware routing, cache-manager deferred consumer, RECONCILE staging, recovery/replay. |
| Network | In-memory boxes, endpoint registry, cooperative priority scheduler. | Runtime balancer, configuration/bridges, concurrency, swarm/tracker. |
| Analyst | No analyst cognition code. | Request/view contract, hot assembly, numerical inspection cadence and decision logic. |

The immediate design questions are how the field's directed vectors produce an
effective **destination distance** for the brake; what motion ratio is
significant at an active resolution; how ordered/repeated parents reorient;
what makes a particle safely resolved and wakes it again; and how to measure
the claimed calculation savings. The local DB release/encryption design and
the exact live WAL adapter remain distinct design passes. The
[working record](napier-system-discussion.md) keeps derivations and historical
alternatives visible while these questions are settled.
