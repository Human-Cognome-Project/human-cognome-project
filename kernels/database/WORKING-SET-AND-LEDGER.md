# Database paths, discovery ledger, and working-set composition

**Status (2026-09-26):** This is the cache/record component view of the
[NAPIER system guide](../../docs/napier-system-guide.md). The five-table schema,
controller, and fixture-driven tier-2 handler are built. Study-rooted warm
composition, delayed reciprocal writes, and private stores are design work.
Read [NOTES.md](NOTES.md) for the detailed record-tier rules and
[API.md](API.md) for current commands.

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

## Cold graph and direct follows

The database identifies a token by its arrayed `token_id` address. One
definition has one stored identity; a loaded field model may allocate
several `particle_id` instances of that token. The cold store holds direct
relationships, rather than a universal SNode hierarchy or a position in
the model. Its two reciprocal axes are:

| Direct relation | Reciprocal relation | Stored meaning |
|---|---|---|
| `token_parent` | `token_child` | Ordered constituent occurrence (including mass/ordinal) and direct reverse link to the composite. Repeated occurrences survive by ordinal; a distinct parent/composite pair needs only one reverse navigation link. |
| `member_of` | `members` | Direct group participation and direct reverse list of that group's members. A group can itself participate in another group. |

These return lists are the fast walks that make future traversal possible
without reverse searches. In an encoding-table study, a byte value's direct
membership in a table is a stored edge; that table's vendor and format
classifications are higher edges. Recursive composition can then place them
under an **Encoding tables** root. A composed piece may appear at multiple
loci in the study tree. The schema stores reciprocal links, but neither the
schema nor `Controller::add_membership` enforces membership composability to
a named study root today.

In the field model, every simultaneously exposed instance of the same
`token_id` also belongs to an automatic sibling field. This is an active
identity grouping using the same field law; **there is no stored
`token_sibling_group` table**. The present field harness does not store
`token_id` per allocated particle or synthesize this group.

## The pairwise discovery ledger

Discovering direct commonality and its effects pays a pairwise cost once.
Recording those direct links makes later use an addressed follow plus
alignment in the working model, instead of repeating the discovery. The
result may be revised when new source material or connections arrive; the
ledger is not a claim that the entire world has already been compared.

An exclusion always has a **named scope**:

| Mechanism | What it can skip | What invalidates it |
|---|---|---|
| DB READ/address follow | Branches outside the addressed keys or specified prefix of this read, under the [READ rules](NOTES.md). | A different read request or relevant record change. |
| Warm view projection | Cold links/dimensions not exposed by this study and LoD view. | New relationship, changed view spec, or parameter on which exposure depends. |
| Field active set | Redundant particle/field work after loading, for the current exposed model. | A tracked interaction, changed centroid/membership/mass, or changed model/view parameter that wakes it. |

The first mechanism is implemented in the record tier. The latter two are
modeling/runtime goals. None makes a global claim that omitted relationships
do not exist. A per-tick small-effect gate cannot alone justify indefinitely
skipping a field: continued exclusion needs a predicate absolute for the
defined scope and a complete invalidation path. See
[active field model](../../engine/docs/ACTIVE-FIELD-MODEL.md).

## Expressed scale in warm SNode definitions (2026-10-08)

The most granular visible LoD anchors one distance unit; less granular expressed
levels expand by a binary factor, testing 64 first and 128 if needed. Compressed
equivalences need no full step. Local cohesion and shear resistance should arise
from ordinary scaled attraction; distant dispersion is not a construct scope mask.

**Monitor and discuss:** relationship kinds may need higher or lower distance
multiples; one universal factor may not produce the needed effects everywhere.
Those degrees of separation may need recording in warm-cache SNode object
definitions. This does not mean graph-hop count or raw nesting depth. No field
layout or per-relationship multiple is settled yet.
Carry operative scale separately from raw stored depth, and validate local cohesion
and universal effects as model extent grows (#115, #106).

Whitespace is a gap in recorded representative positions. A warm/hot model may
instantiate an inert separator solely for local lower/higher connection, without
parent/child relationships, gathering mass or shared whitespace fields (#116).

## Compose the working projection

The cache manager's future view spec combines system limits and an
analyst-defined need. The manager follows relevant cold addresses, selecting
dimensions and LoD **per area**, then assembles warm SNode pieces and trees
rooted in the field of study. At any locus, a **nested** SNode exposes parts
for that inquiry; a **compressed** SNode carries an appropriate aggregate.
The future analyst chooses and assembles hot structures from these pieces.
Underlying cold links stay followable regardless of the hot residency
budget. A moving inquiry can refocus an existing view (`UPDATE_CACHE`) or
compose a new basis (`REBASE_CACHE`); their exact invocation and update rules
are still design work in [NOTES.md](NOTES.md).

Address suggestions made from a partial view are provisional. The analyst
can propose position within the composition; the cache manager decides
actual placement in the cold address space. A new observation can alter
the suggestion without invalidating stored token identity.

The recovered [storage/working split](../../docs/storage-and-working-split.md)
describes explicit cold links and a compressed per-analysis projection; this
warm preparation stage is the intermediate form clarified later. The older
note's unqualified “nothing built” and capacity proposals describe its
original date, not the present state of the native engine.

## Writes and deferred returns

**Built:** `Controller::mint` and `Controller::add_membership` write both
directions synchronously. The DB-manager's tier-2 handler runs the record
commands over an endpoint and replies to the caller's return endpoint. It
does not emit a live WAL report. **Intended:** file primary changes promptly;
the [WAL manager](../wal/REPORT-TO-WORK.md) observes source reports, records
owed returns and mass work, and sends obligations into the cache manager's
lower-priority pending-work box. A followup write settles its obligation on
the next observed report. Analyst RECONCILE, sent to WAL alone, would promote
relevant outstanding work to the manager's top priority box.

The design must reconcile today's eager controller with the future delayed
writer; copying the WAL bookkeeper into the DB manager would obscure that
boundary. The WAL book's durable relation owns outstanding work while
endpoint boxes carry transient notifications. This path also needs explicit
scope/target routing for [instance-local databases](../../docs/instance-local-data.md)
so their deferred work flows inward only.
