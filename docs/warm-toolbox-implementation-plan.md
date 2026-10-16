# First warm toolbox and hot letter study

Status: **Accepted in #137, merged at `50bd968`, 2026-10-15.**
[#139](https://github.com/Human-Cognome-Project/human-cognome-project/issues/139)
governs staged implementation. [Stage A](../kernels/database/warm/README.md) is
implemented for review under #140, with 90 native PostgreSQL acceptance checks.
B–D remain queued. No cold-store modification is part of Stage A.
The [layout](warm-knowledge-cache-layout.md) supplies the governing boundaries.

Warm PostgreSQL stores reusable tools and reconstructible recipes. The hot
working object store uses native memory and feeds the engine arrays. First
objects are language letters and the existing punctuation components, using
#136's populated pair. Analyst requests are exercised by native test clients;
no analyst behavior, general scheduler or new field law is implemented.

## Accepted shared decisions

| Question | Choice for this slice |
|---|---|
| Warm backend | One separate PostgreSQL 16 database, schema `warm_toolbox`; native C++/libpq access. No changes to either cold database. |
| Source revision | Explicit epochs identifying validated immutable source snapshots. Initial epochs name the retained paired exports. No live WAL decoder or changes to cold token tables. |
| Publication unit | One requested selection of letters, its complete construction dependencies, required linguistic membership graph and persisted occurrence/exposure/initial-layout recipe. Tools are reused between selections. |
| Warm concurrency | Build outside the write transaction; lock/check bindings and publication head, then atomically insert the complete revision and advance its head. A stale expected head is a conflict. |
| Exposure | Letter definitions retain character parent identities. A derived byte view creates byte occurrences; it does not replace the letter's parents with byte parents. |
| Membership | Deduplicate membership paths per field/live occurrence. Preserve each ordered parent contribution separately. |
| Hot memory | Reuse the existing packed SoA host vectors and matching Taichi Ndarrays in `field::Harness`, with native occurrence/provenance metadata. No new arena or zero-copy backend. |
| Geometry | Caller-supplied, versioned component spheres and centroids, checked by the existing enclosure API. No inferred packing or universal radius. |
| Private/temporary roots | Patrick reserved `y*` for personal data and `z*` for temporary objects on 2026-10-15. These are prefix families, not concrete object allocations. Instance-qualified identities and explicitly authorized full-couplet bindings remain required. |

## Identity and ownership shared by A–D

Use `codec::Address` without wildcards for an object address. Its PostgreSQL
representation remains `text[] COLLATE "C"`, one Base62 couplet per element.
Never use notation, source byte text or a GPU slot as the object's identity.

| Key | Concrete fields and lifetime |
|---|---|
| `ObjectKey` | `(scope_uuid, address)`. Shared objects use a nil scope UUID; private/temporary objects use the owning instance UUID. The UUID qualifies identity; it does not allocate an address within a reserved family. |
| `ToolRef` | `(ObjectKey, revision_uuid)`, naming one immutable cached definition revision. Multiple live uses share it. |
| `InputEpoch` | A binding-lifetime UUID. Source bindings also retain the validated immutable export hash (`SourceEpoch` provenance); temporary bindings use their local lifetime UUID without claiming an external snapshot. Neither allocates an object address. |
| `BundleRef` | `(selection_uuid, revision_uuid)`, a cache-local publication identity, separate from token identity. |
| `OccurrenceKey` | `(study_uuid, request_entry_uint64, path)`. Aggregate path is empty; byte path is `(character_ordinal, byte_ordinal)`. Request entries distinguish repeated uses of the same letter. No survival across different studies is promised. |
| `FieldKey` | `(kind, ObjectKey)`, with distinct `SiblingDefinition`, `ParentValue` and proposed `Membership` kinds. Ordinal never splits a parent-value field. |
| Parent edge | `(OccurrenceKey, FieldKey, source/projection ordinal path)`, with exact integer contribution. Repeated parents retain distinct edges. |
| Membership edge | `(OccurrenceKey, Membership FieldKey)`, once despite multiple paths through the membership graph. |

Within one bundle/study, each `ObjectKey` resolves to exactly one `ToolRef`.
Conflicting revisions of the same object are rejected; different studies may
pin different revisions. Native opaque IDs encode the full object/occurrence
keys losslessly; CPU-side maps retain typed keys and provenance.

Root resolution uses a validated configuration directory. A binding names the
scope, address prefix, source handle, source epoch, policy version and permitted
writer actors. Reject overlapping prefixes within a scope, unknown prefixes,
wildcards in concrete bindings and an incorrect instance context. Source handles
resolve connections outside object records. The initial source directory contains
shared `00` and `02`. Configuration also records the personal `y*` and temporary
`z*` family reservations: the first symbol of the first couplet is `y` or `z`.
The asterisk is descriptive prefix notation, never a stored address symbol.
It gives `01` no meaning and assigns no internal subdivisions or expansion blocks.
Personal/temporary `root_binding.prefix` values contain only full codec-valid
couplets under the appropriate family. Reservation alone grants no object address
or writer authority; a concrete
binding/allocation must be supplied explicitly before private/temporary writes.
Internal subdivision, writer delegation and eventual lifetime policy remain for
later discussion; the bounded retention policy below does not settle those choices.
The initial `00` / `02` bindings share one validated pair-deployment epoch UUID,
with the separate export hashes retained on their binding records. Adopting a
new pair changes both bindings in one warm transaction. Do not combine independently
installed source epochs that have not been validated as a compatible pair.
Private prefixes must not shadow shared prefixes. Source references to `00` /
`02` resolve in shared scope; other source references require the source
instance's explicitly configured binding. Private prefixes can be reused in
different instance scopes without conflating their identities.

Trusted configuration creates writer contexts; a request cannot grant itself
an actor or role. Manager writers prepare shared source-backed tools. A temporary
writer creates immutable definitions within its configured instance binding;
a changed temporary construction needs a new authorized object address. A retry
by the same creator with identical data is idempotent; it cannot adopt another
creator's address. It references shared tools without overwriting them. Base SQL mutation
credentials belong only to the native store owner, and ordinary clients use
the scoped native operations. This is a bounded API authority contract, not an
analyst authentication or network protocol implementation.

Creation/retry by supplying a body is separate from explicitly referencing an
already published `ToolRef`. Stage A's `prepare(recipe, existing_roots)` loads
that immutable dependency closure under read scope and preserves its authors,
revisions and original address claims. Another same-instance writer may reference
it unchanged without claiming the creator's address. New constructions still
require their own authorized addresses; a supplied body cannot replace a reference
or contradict one of its pinned dependencies.

Shared tool bodies may depend only on shared definitions. Instance-scoped
tools may depend on shared definitions or that same instance's definitions;
cross-instance private references are rejected. Every bundle/geometry recipe
has a local owning instance even when all its tools are shared. A trusted reader
context must match that owner and be authorized for every private dependency.
No publication operation exports a bundle or promotes private-derived tools
into shared source identity. Shared source ownership grants no release right.

## A. PostgreSQL storage and coherent publication

Stage A's physical tables follow. `T` below expands to the three non-null
`ToolRef` columns `(scope_uuid uuid, address text[] COLLATE "C", revision_uuid uuid)`;
`B` expands to `(selection_uuid uuid, revision_uuid uuid)`. Reference columns
use the same types/collations as their targets. All lookups use the listed
primary keys or their leading prefixes; no reverse-search indexes are added.

| Table | Primary key | Other columns / references |
|---|---|---|
| `root_binding` | `(scope_uuid, prefix)` | Source handle, epoch, policy version, permitted actors, ownership/lifetime rules. Runtime configuration supplies prefixes. |
| `tool_head` | `(scope_uuid, address, input_epoch, recipe_key)` | Nullable revision UUID during creation; resolved `T` references `tool_revision`. Cache lookup/reuse by known address and preparation recipe. |
| `object_slot` | `(scope_uuid, address)` | Sorted per-object publication lock and creator-bound permanent temporary revision claim. Prevents create-only address rules being bypassed through another recipe key. |
| `tool_revision` | `T` | Binding prefix FK, input epoch and optional source snapshot hash, recipe key, optional notation and `bigint` whole mass, author actor. No live state or object-type column. |
| `tool_parent` | `(T, ordinal)` | Referenced parent `T` FK, original nullable parent-row mass and separately resolved exact contribution. Both values retain their meanings. |
| `tool_member_of` | `(member T, group T)` | Direct source membership edges for the requested linguistic graph; both endpoint references are FKs. |
| `bundle_revision` | `B` | Owning instance, recipe version and creator; existence becomes visible only with a complete publication transaction. |
| `bundle_head` | `selection_uuid` | Nullable published revision UUID; composite FK to `bundle_revision`. |
| `bundle_definition` | `(B, ObjectKey)` | Resolved `T` FK; optional supplied geometry centroid and component spheres in `bundle_sphere`, keyed `(B, ObjectKey, ordinal)`. |
| `bundle_node` | `(B, node_index)` | Logical layout node, nullable parent index FK within this bundle and explicit `ScaleRole`. The recipe version fixes the bounded SNode shape and exposed-level normalization described in B; no native pointer is stored. |
| `bundle_occurrence` | `(B, entry_id, path)` | Definition ObjectKey FK into `bundle_definition`, node index FK and finite initial position/velocity triples. Empty path is the letter aggregate; `(character_ordinal, byte_ordinal)` is a byte position. Repeated uses have distinct entry IDs even when their definition is the same. |
| `bundle_selection` | `(B, entry_id)` | Aggregate path column constrained to empty, composite occurrence FK and `Aggregate` / `Parts` choice. Every requested entry has one row. |
| `bundle_part` | `(B, entry_id, ordinal)` | Ordered part occurrence FK within the same entry; its path retains both source ordinals. Each part appears in exactly one selection. |
| `bundle_contact` | `(B, ordinal)` | Two occurrence FKs for explicit initial contact coverage; endpoints must be active, distinct and unique as an unordered pair. Empty coverage is stored as zero rows. |
| `bundle_child` | `(B, parent ObjectKey, child ObjectKey)` | Exact projected return lists built from the included ordered parents; endpoint FKs to `bundle_definition(B, ObjectKey)`. Resolve pinned revisions there. No ordinal; repeated parents produce one navigation pair. |
| `bundle_member_of` / `bundle_members` | `(B, member ObjectKey, group ObjectKey)` / `(B, group ObjectKey, member ObjectKey)` | Both projected membership directions, with bundle-definition endpoint FKs; revisions resolve through that bundle. |
| `epoch_use` | `(binding scope/prefix, source_epoch, B)` | Direct return list of bundle revisions using that epoch; FK to the bundle revision. |
| `bundle_lease` | `(B, consumer_uuid)` | Explicit pin ownership. Consumer shutdown releases its rows; crash recovery requires confirmed consumer termination. No guessed time-based expiry. |

These are new warm tables, not additions to the cold five-table schema.
Addresses are nonempty codec-valid arrays. Ordinals are nonnegative and
contiguous within construction/layout records. Mass/contribution values fit
nonnegative signed 64-bit storage and native exact-count conversion; reject
overflow. Active definitions and contributions must be positive. NULL whole
mass remains permitted for organizational anchors and prevents their use as
massive live definitions. No trigger derives masses from local or remote rows.

Tool bodies are immutable once committed. Only heads, configured bindings and
leases are mutable through the native API. `tool_head` selects reusable tool
revisions; supplying the same epoch/recipe with different definition data is
a conflict, not an overwrite. Canonical ordered records are compared before
reusing a tool. Geometry belongs to the bundle's supplied recipe, so different
studies can use different geometry without changing source identity or mass.
FKs always target the listed primary keys; no redundant uniqueness index is
needed to reference a bundle's resolved revision. Native publication also
checks that every stored parent/membership `ToolRef` agrees with that bundle's
one revision per `ObjectKey`. Temporary input epochs follow the binding's local
lifetime, so a different body cannot evade the create-only address check by
claiming a new external source epoch.

### Persisted recipe and fresh-client reconstruction

Stage A validates and round-trips **supplied normalized fixtures** through its
engine-independent C++ value interface. Production projection/normalization is B;
runtime SNode reconstruction/materialization is D. A does not call B, avoiding an
A→B→A implementation dependency. Source binding attestation is trusted
configuration: the owner supplies previously validated immutable installations
and their paired epoch/export hashes; A enforces consistency and changes.

The request supplies ordered entry IDs and letter keys, an exposure per entry,
geometry for every declared live-capable definition, and initial placements and
velocities for the aggregate and every declared byte alternative. Missing values
are an incomplete request, not an instruction to infer packing. The request also
supplies explicit contact pairs (possibly empty). B expands and validates the
construction paths and normalizes the logical node bindings before publication.
All of those results are stored in the tables above in the same bundle transaction.
Entry IDs are checked uint64 values stored as `numeric(20,0)`; paths contain checked
uint32 ordinals stored as `bigint[]`. SQL/native checks reject overflow, duplicate
entries/paths, missing parts, overlapping selections and inconsistent source paths.
Node indices are contiguous from zero, parents precede children and roles must
match the normalized versioned shape. Occurrence paths have length zero or two;
every declared occurrence belongs to its entry's selection, whose parts exactly
cover that letter's ordered byte construction. No stray active occurrence is
allowed outside those selections. Initial numeric triples use `double precision`
with finite-value validation; source mass/counts remain exact integers.

`acquire(selection, consumer)` returns the pinned bundle and its complete immutable
recipe, including repeated entries, ordered parts, exposure, logical nodes, initial
geometry/state and contacts. It does not depend on the preparing client's memory.
A fresh native consumer supplies a new study UUID, runtime and explicit
`field::Parameters`; no additional occurrence or exposure input is needed.
It reconstructs the versioned SNode shape, checks direct parent bindings and maps
`(entry_id, path)` to `OccurrenceKey` under that study UUID. Slots are assigned
deterministically by entry/path order to all declared alternatives; capacity is
their count, checked against the native i32 limit. Slots are not persisted identity.

Initial position/velocity and contact coverage are immutable preparation inputs.
Ticked positions, velocities, centroids, forces, device buffers and live readiness
are never written back to this recipe. A new materialization starts from the stored
initial inputs; this is reconstruction, not resumption of a running study. Runtime
parameters remain an explicit caller input, so the recipe alone does not promise
identical trajectories across different parameter sets or backends. Changing the
request or its initial inputs requires a new bundle revision under head checking.

Warm child/member return lists describe exactly the selected projection. They
are not copies of complete cold child lists. Core byte endpoints can have
millions of cold children; this study never loads that whole reverse listing.
Read source attributes, ordered parents and the required `member_of` lists by
known owner addresses. Construction closure includes characters, bytes and
their nibble dependencies. Linguistic membership traversal starts from the
selected linguistic endpoints and follows actual group ancestry; it invents
no absent family link. Core classification memberships are outside this recipe.

Proposed native operations are `resolve(ObjectKey)`, `prepare(request, writer)`,
`publish(candidate, expected_head)`, `acquire(selection, consumer)` and
`release(BundleRef, consumer)`. Their records are C++ values and libpq-bound
parameters. No runtime Python pipe, generic coordinator or new message transport
is introduced. A read-only source adapter exposes the existing controller's
addressed read semantics; it exposes no cold mutation operations and enforces
read-only PostgreSQL sessions. Connection setup remains outside object records.

Publication procedure:

1. Capture the used binding/epoch records coherently in a short read transaction,
   then fetch/validate the bounded closure through
   direct addressed reads. Source snapshot installations stay immutable under
   the existing bootstrap-only mutation restriction. Do source reads, native
   projection and geometry validation before taking warm write locks.
2. Begin one warm transaction. Lock the used binding rows in canonical order
   with `FOR SHARE`; recheck actor authority, policy versions and source epochs.
   A changed binding rejects this candidate for retry against the new binding.
3. Create missing tool-head rows and lock the affected heads in canonical
   key order. Reuse identical complete tool revisions or insert new complete
   tool bodies and their parent/membership records. Insert all token bodies
   before relationship rows so endpoint FKs stay enabled.
4. Create/lock the selection head with `FOR UPDATE`, compare it to the caller's
   expected revision (NULL for the first publication), and reject stale writers.
   Insert the bundle revision, definitions, geometry, nodes, occurrences,
   selections/parts, initial contacts, projected reciprocal lists and epoch return
   paths. Validate recipe references/exposure, closure, mass and both reciprocals.
5. Advance `bundle_head` to this complete revision and commit once. Any failure
   rolls back the entire transaction. No 2PC is needed: both cold stores are
   read-only and all warm changes are in this one PostgreSQL database.

`acquire` uses one short transaction: share-lock the bindings and head, check
current epochs and reader scope/ownership, insert its lease for the exact
complete revision, then commit.
An existing pinned consumer may finish using an old immutable bundle. A new
default acquire of a superseded epoch returns `Stale`, never silently serves
the old revision. An explicitly adopted new validated source epoch uses
`epoch_use` return lists to identify affected bundles; rebuilding remains scoped
to their selections, not the complete Linguistics root. Live mutable-source
refresh/WAL adaptation is outside this first slice.

Only the trusted consumer owning a lease can release it; release is idempotent.
The first implementation retains superseded revisions. No automatic deletion
or private-root expiry is required for initial acceptance. Releases and pin
accounting are tested now; later reclamation must preserve current heads and
dependency/lease closure. Failure of a consumer must not prematurely delete
objects that another consumer uses.

## B. Construction projection and exposure matrix

Choose **source-character parents with a derived byte view**. All source
parent rows and canonical identities remain intact. The native records are
study projections of those definitions, not changes to their cold records.

| Exposure | Active objects | Native parent-value contributions |
|---|---|---|
| Letter aggregate | One occurrence of the selected linguistic definition | Its ordered core-character parents, each counted with that verified character's mass. |
| Character equivalent | No additional active occurrence when it is merely the same complete value under another name | Retained reference/provenance; no duplicate particle and no conceptual scale step. A multi-scalar letter has no whole-value equivalence to each individual character. |
| Bytes | One occurrence of each byte position, with the letter aggregate inactive through `Selection::Parts` | Each byte definition retains its two ordered nibble parents and their known mass-1 contributions, as in the existing native byte fixture. Nibbles require no live particles. |

ASCII characters already use byte endpoints. Generate one derived byte
occurrence for each ASCII character position; do not traverse that endpoint's
nibble parents as though they were UTF-8 bytes. For non-ASCII characters, use
their recorded ordered byte parents. Validate the expected UTF-8 sequence
against the exact source form during preparation; never normalize or casefold.
Each byte occurrence records its owning letter occurrence and both ordinals.

The sum of byte occurrence masses must equal the letter's verified structural
mass. For Dutch `íj́`, character masses are `4 + 2 + 4 = 10`; five mass-2 bytes
conserve that mass. The test-only `aá` fixture has repeated `a` parents at
ordinals 0 and 1, each contributing 2, plus the mass-4 accent: total 8.
At letter exposure those two contributions remain separate in the same parent
field. Navigation backlinks may deduplicate their pair; participation may not.

### Exposed-level normalization

Choose exposure before constructing bindings. The finest **active, exposed** LoD
anchors units 1 across the study. Inactive byte alternatives do not force a
letter-only study to use byte units. Existing `scale_bindings` reads explicit
nodes independently of `Selection`; `prepare` does not perform this normalization.
B must produce and persist the following normalized bindings before calling it:

| Requested exposure | Root binding / active byte units | Letter aggregate binding / units |
|---|---|---|
| Letter-only: every entry uses `Aggregate` | Root anchors letters. All byte alternatives are inactive. | Every active letter binds to Root: **1**. |
| Byte-only: every entry uses `Parts` | Root anchors bytes: **1**. | Inactive letter alternatives bind to a direct `PrimaryStep` child: 64. |
| Mixed: different entries use aggregate and parts | Root anchors the exposed bytes: **1**. | Active letters and inactive letter alternatives bind to a direct `PrimaryStep` child: **64**. |

The bounded layout recipe constructs the actual SNode root and, for byte-only or
mixed studies, one direct dense child (axis 0, extent 1) for that primary letter
boundary. Required storage leaves do not create scale bindings. Letter-only
inactive byte alternatives use Root as a validation placeholder; they produce no
particles or fields and this binding declares no active byte/letter equivalence.
Activating them requires rebuilding the normalized study. Naming/compression
attachments, when present, inherit their bound scale at 1:1; raw depth never
determines units. The recipe version identifies this shape and binding rule.

Geometry is supplied in each definition's local conceptual units; each occurrence's
initial position and velocity use its bound level's local units. Native preparation
converts active values into the study's normalized units. For an input position 2
and enclosing radius 0.5, an active letter prepares to position 2 / radius 0.5 in
letter-only exposure and position 128 / radius 32 in mixed exposure; a byte in the
mixed study with those inputs prepares to position 2 / radius 0.5. This is a
distance-unit conversion, not a radius-per-mass rule or a packing prescription.
No trajectory equivalence or transfer of old live coordinates across studies is
assumed. Every exposure change creates a new study UUID and normalized layout,
even if array counts happen to agree; equal-count refresh within an unchanged
exposure/binding profile remains the supported existing operation.

Every selection declares its aggregate and disjoint byte parts with distinct
occurrence keys/slots; `Selection` chooses exactly one active representation.
Mixed letter/byte exposure may apply to different letter occurrences in one
initial study. Budget capacity for all declared slots, including inactive
alternatives, as the existing native API requires. A new exposure that changes
fixed array counts starts a new fixed study, not an in-place resize.

## C. Membership fields

Propose a minimal extension to `assembly::StudyInput`: explicit membership
requests `(occurrence_id, group_ObjectKey)` and `GroupKind::Membership`.
Preparation validates the selected active occurrence and generates a whole-mass
participation with share 1. Group naming anchors remain metadata, not live
massive `Definition` records. Existing parent/sibling field generation remains.

For each selected linguistic occurrence, follow the pinned upward membership
graph and deduplicate reached target groups. Cycles or missing references
reject preparation. Direct English and alphabet-mediated English paths thus
identify one English target, while alphabet and English remain separate fields.

At letter exposure, emit one membership participation per reached group and
active letter occurrence. At byte exposure, its active byte parts inherit that
letter occurrence's reached groups; each byte occurrence contributes its own
mass once to each field. Their combined membership mass equals the replaced
letter's mass. Repeated byte positions and repeated uses of the letter remain
distinct. This preserves organizational fields across exposure without also
activating the aggregate or inventing source memberships on the byte definition.
This exposed-LoD behavior was accepted in #137's maintainer review: it distributes
organizational response across active parts. Conserving field mass does not claim
the same force distribution or trajectory as the aggregate representation.

Deduplication is only for duplicate membership paths to the same target/live
occurrence. Parent edges retain ordinal paths/counts; sibling participation
remains once per active occurrence. Do not merge field kinds, split parent
fields by ordinal or rewrite the warm/cold source membership graph.

## D. Hot working object store and handoff

Use a fixed-study native owner for the lifetime of the initial study:

- Pinned `BundleRef`, typed definition/occurrence/projection maps and exact
  counts on the CPU; a slot is not an identity.
- Packed SoA host vectors already defined in `field/field.h`, indexed
  `field * item_count + item`, with i32 edge/contact indices and f32 numerical
  fields. Exact integers and provenance stay in native CPU metadata.
- Matching existing Taichi `{fields, max(items,1)}` Ndarrays. Stage through
  `FixedStudy`/`Harness::publish`; the GPU never queries PostgreSQL.
- The caller keeps SNode layouts and bundle pins alive until study teardown
  and completion of outstanding device work. Host positions/velocities are
  staging or last-downloaded values; the device is authoritative while ticking.

Reconstruct the persisted initial recipe from the pinned bundle, then prepare
native definitions, projections, membership edges, contacts and supplied geometry
before the initial handoff. Missing geometry or unknown mass
for a requested live definition is an explicit incomplete/error result. It
does not produce an arbitrary radius, zero mass or a partial ready signal.
Warm group anchors with NULL naming mass can still name membership fields.

Use the existing enclosure function on supplied centroids/spheres, then the
existing conversion/publication path to seed centroids before the first force
pass. Warm-ready means a complete pinned tool bundle is available. Hot-ready
additionally requires successful native staging, transfer and centroid seeding.
Database commit success alone cannot signal hot-ready.

An equal-count refresh uses the existing fail-closed `FixedStudy::publish`.
On failure the model is non-runnable; recovery supplies a valid target through
that supported path or creates a new fixed study. New counts require a new
fixed study with a new study UUID. No live rollback, automatic state transfer,
cross-study occurrence survival or concurrent device mutation is promised.

## Implementation sequence and exits

These interfaces were accepted together in #137. Tracker #139 releases only A
through #140 now; B–D remain sequenced follow-ups. Stage A's focused implementation
is ready for review; later stages consume its shared values rather than defining
competing identities or publication rules.

| Stage | Planned code boundary | Required acceptance |
|---|---|---|
| A | `kernels/database/warm/` native `toolbox`, read-only `cold_reader` and separate `schema.sql`, with codec/libpq dependencies | Isolated new DB, two native writer connections, same-head race (one winner/one conflict), unrelated selections, full rollback on a late relationship/recipe failure, exact reciprocal/closure and complete recipe read-back, pin/epoch behavior and no cold writes. |
| B | `engine/src/assembly/letter_projection.{h,cpp}` pure native projection records/function; no PG dependency | English/French `A` retain distinct linguistic keys and shared core parent, a multibyte letter, Dutch `íj́`, repeated `aá`, exact conservation and ordinal round-trip, aggregate/parts exclusion; letter-only units 1 with inactive byte alternatives, byte-only units 1, mixed byte 1 / letter 64, and converted geometry/initial coordinates. |
| C | Small explicit membership input/kind extension in `engine/src/assembly/`, using the existing field kernel | English duplicate paths count once per target/live occurrence; two uses remain distinct; repeated parents still produce two contributions; byte/letter membership mass agrees and seeded response acts on the selected parts; NULL naming anchors never become live masses. No aggregate/part trajectory-equivalence assertion. |
| D | Fixed-study adapter/test application consuming A–C and existing packed host/device layouts | Fresh-client recipe round-trip described below, source closure to warm publication to hot-ready, deterministic record checks and seeded first force, independent mutable instances, supported equal-count refresh/failure, no SQL in tick and cold source tables unchanged. |

The A–D round-trip fixture publishes two entries of the **same English `A`**:
entry 0 selects the aggregate and entry 1 selects its byte part. Both retain their
inactive alternatives, explicit distinct initial placements and velocities, and
one contact between the two active occurrences. Destroy the preparing client;
a fresh client acquires the selection using only its authorized context and
selection ID, recovers both entries/exposures, paths, geometry, contacts and
normalized bindings, then materializes with a new study UUID and supplied runtime
parameters. Assert distinct occurrence keys, aggregate/part exclusion, exact mass
2 per active occurrence, letter units 64 / byte units 1, the expected converted
initial state and seeded first force. Ticking the hot study must leave the stored
recipe unchanged. Failure injected after a recipe row insertion must expose no
partial new bundle or head advancement.

Use new uniquely named fixture databases; never reset/drop/truncate a retained
database. Integration uses the accepted seed pair read-only. Tests can run the
existing native CPU backend; retain the same engine path for a later actual
GPU run and report it separately. No unexecuted GPU result is claimed.

Private/temporary writer acceptance uses explicitly authorized concrete bindings
inside `y*` / `z*`, supplied in isolated fixture configuration rather than allocated
as production subdivisions by the implementation. Test both family reservations,
reject a wildcard as an object identity, and reject writes with only a reservation
but no binding (`UnboundRoot`). Initial shared-letter work and B–D fixtures need
no internal personal/temporary allocation. Do not mark private integration accepted
from rejection-only tests. Multi-writer/private coverage remains part of A's full
exit, even if the shared-letter milestone is delivered first.

## Maintainer actions

The top-level personal/temporary reservation decision is supplied: `y*` / `z*`.
Do not request it again or treat the deferred internal layout as a gate on shared
`00` / `02` letter work. The persisted recipe, exposed-scale rules and contextual
organizational response on exposed byte parts were accepted together in #137.
Review Stage A's implementation under #140 before releasing subsequent stages
through #139. No additional cold data
load, physics requalification or revival of #111/#123/#75 is needed. No new meaning
for `01` or additional personal expansion block is assigned.

[#138](https://github.com/Human-Cognome-Project/human-cognome-project/issues/138)
tracks membership sub-classification and digit/operator coverage and layer
placement. That follow-up preserves existing seed identities and is not a new
gate on the initial warm letter study; this proposal allocates no concept root.

## Design-time validation before Stage A

The existing CPU `assembly_test` passed on 2026-10-15, including repeated parent
contributions, exposure exclusion, explicit scale bindings, seeded first force
and fail-closed publication. This verifies the native boundary being reused;
the new storage, projection and membership code was not implemented at that point.
Read-only queries against #136's retained populated pair confirmed distinct
English/French `A` records of mass 2 and Dutch `íj́` of mass 10, with ordered
character parents of masses 4, 2 and 4 and the expected five-byte construction.
The Dutch parent-row mass cells remain NULL. No source rows were changed and
no proposed warm schema was installed during these checks.
The subsequent recipe/scale corrections were checked against the native
`StudyInput`, `Selection`, `scale_bindings` and `FixedStudy` source contracts.
Their new integration acceptance cases above remain planned, not executed.

## Stage A execution

[Native acceptance evidence](../kernels/database/warm/acceptance.json)
records 90 passing PostgreSQL 16.15 checks, full fresh-client recipe recovery,
concurrent publication/rollback, exact source closure and reciprocals, source-pair
epoch/policy checks, owned retained pins and positive personal/temporary cases.
Review regressions cover cross-writer immutable temporary references/composition
and finite subnormals across coordinates, velocities, centroids and radii.
Complete data-only hashes of both retained cold stores match before and after.
Stage A executes storage/recipe fixtures only; B–D engine integration cases remain
planned. See [build/reproduction instructions](../kernels/database/warm/README.md).
