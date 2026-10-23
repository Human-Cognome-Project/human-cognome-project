# Warm knowledge cache: root-addressed layout

**Current disposition (2026-10-23):** Original A–D completed through #149; #139 is
closed. D is no longer awaiting implementation. #171 adds [coordinate-free v2 recipes](../kernels/database/warm/STRUCTURAL-RECIPES.md)
while preserving the supplied-state v1 fixture described below. #156/#170 own
continuous operational integration; #145 retains device/transfer qualification.
The historical stage-release statements below are not current gates.

**2026-10-16 status:** Accepted through merged #137 and staged in #139. Native
[Stage A storage](../kernels/database/warm/README.md) is accepted and merged
in #141 at `e7acdea`. [Stage B projection](../engine/docs/LETTER-PROJECTION.md)
is accepted/merged in #143 at `6ccd5a3`. C is implemented in #146, with no code
blockers and all nine hosted checks green, accepted and merged at `43f418a`.
D is released in #148 and [implemented for review](../engine/docs/ACTIVE-WORKBENCH.md).
The [pre-D design](pre-d-hot-workbench-design.md)
is accepted in #147 under #145. The proposal wording below records the design's origin; the accepted
[implementation contract](warm-toolbox-implementation-plan.md) and stage tracker
govern execution. No cold data migration or analyst implementation is included.

Originally proposed on 2026-10-13; Patrick's directions below are the design
basis. The bounded implementation contract now supplies the physical storage
and publication choices. Stage A's implementation is separate from this design
record; no cache-manager scheduler or analyst is built by it.

## Purpose and settled directions

The warm knowledge cache uses PostgreSQL and permits multiple writers. The cache
manager supplies shared prepared tools/parts ahead of demand or on request.
Patrick's 2026-10-16 clarification supersedes earlier manager/private-tool wording:
private/local information belongs to the analyst and joins only in active hot
assembly. It never enters the manager's warm publication/acquisition/refresh
path. Existing generic scoped storage and `y*` reservation do not authorize it.

**Warm is the tool/part box; hot is the host workbench; GPU memory runs the
prepared objects together.** The workbench holds objects and relationships in
progress and prepared generations, combining shared tools with analyst-owned
local inputs. Its numerical projection feeds the execution surface. Backend-
appropriate pinned host transfer is a design requirement under #145, with
ordinary vectors as the existing baseline. GPU reporting returns to the active
owner; it grants no permission to propagate private data to the cache manager.

Use root-addressing protocols to distinguish owning sources and working
namespaces. Shared-source, personal-source and temporary objects use the same
structural representation; their origins do not require separate object types
or a copy of every shared definition. Root rules govern writer authority,
lifetime and permitted propagation. Addresses identify those rules; actual
write paths must enforce them. Exclude private database namespaces/handles from
manager requests and authorized source resolution before any content lookup.
Do not implement manager-side reading followed by filtering, classification or
redaction. Public/shared knowledge about an entity remains a shared-cache concern;
its private records are outside the manager's access scope.

For the first study, language letters are the primary object LoD; their defining
UTF-8 bytes are the underlying base level. The intervening core-character
reference is a naming/construction equivalence, not an automatic conceptual
scale step. Language, alphabet/punctuation and language-family memberships
supply organizational fields. Follow actual recorded memberships; do not
invent family edges absent from the seed data.

## Root protocol

| Root role | Identity and ownership | Pipeline use |
|---|---|---|
| Shared core `00` | Existing computational/encoding identities | Cache manager prepares source-backed definitions. |
| Shared Linguistics `02` | Existing language endpoint/group identities | Cache manager prepares letter objects and grouping relationships. |
| Personal prefix family `y*` | Reserved by Patrick on 2026-10-15; internal subdivisions remain deferred | Analyst-owned private inputs enter active hot assembly only; no cache-manager access or warm integration. |
| Temporary prefix family `z*` | Reserved by Patrick on 2026-10-15; internal subdivisions and eventual lifetime remain deferred | Analyst-prepared temporary definitions receive their own identities and reference shared/private definitions; live uses belong to hot assembly. |

Here `*` describes the first-symbol prefix family; it is never a character stored
in a concrete object address. Actual identities and authorized bindings contain
full codec-valid couplets within the reserved family. A reservation does not
allocate an object or delegate writing. No new meaning is assigned to `01`, and
no additional personal expansion blocks are assigned. The personal family leaves
room to consider further allocation later without assigning it now.

Propose a configuration-level root directory: address prefix, owning instance
where applicable, source binding, allowed writer role(s), lifetime rule and
outward-propagation rule. Specify unambiguous prefix resolution and reject
unknown or conflicting bindings. Shared physical sources belong behind the
cache-manager boundary; private/local bindings belong solely to the separate
analyst input boundary. Object records remain location-blind. Never give the
manager private DB codes to inspect before deciding whether they are private.

The concrete proposal qualifies private/temporary addresses by owning instance
UUID, while shared identities retain shared scope. This is an implementation
choice for review, separate from the accepted prefix reservations. A cache-local
revision/publication key must not silently replace the object's address identity.
Shared cached revisions do not allocate another shared identity or authorize
source writes. Earlier private warm-cache integration wording is superseded by
the analyst-owned active boundary; scoped identities remain useful there.

## Proposed logical records

These are logical record groups, not finalized SQL table names or a new cold
schema. Reuse the existing relationship semantics; decide physical tables and
keys in the implementation plan.

| Record group | Required content | Role |
|---|---|---|
| Root bindings | Root/prefix and the rules above | Resolve ownership, allowed writes and source context. |
| Prepared definition | Full shared object address, prepared revision, source dependency references, exact structural mass when known, preparation completeness | Reusable shared warm tool. Analyst-local definitions retain their separate source/version at the hot input boundary. |
| Ordered construction | Owning definition/revision, ordinal, referenced constituent identity/revision, exact contribution | Preserve repetitions and reconstruct the selected base structure. |
| Membership links | Owning object/group references and their direct reciprocal lists | Keep language/family organization separate from literal construction. |
| Study assembly description | Study/working root, selected definitions/revisions, primary boundaries, equivalence/compression attachments and exposure choices | Reconstructible recipe for a focus, not live working state or permanent LoD on a cold identity. |
| Prepared geometry/field data | Supplied component layout, enclosing extent, grouping identities and preparation inputs | Feed native preparation; distinguish literal mass from group aggregation. |
| Publication/dependency bookkeeping | Complete available revision, expected previous revision, pinned dependencies and unresolved preparation | Let consumers acquire a coherent warm toolbox revision while writers prepare updates. |

Reverse child listings remain navigation data. Preserve the logical child sets;
range representation follows [#134](https://github.com/Human-Cognome-Project/human-cognome-project/issues/134).
Do not materialize a broad child listing as live particles. Avoid runtime
reverse searches by retaining the direct return/dependency paths needed for
refresh and invalidation.

PostgreSQL holds reconstructible definitions and assembly descriptions, not
process pointers, GPU allocations or serialized native SNode addresses.
Native C++ materializes the SNode layout and the engine-facing records in
[first-stage-data-object-model.md](first-stage-data-object-model.md).
Live occurrence identity, particle slot, position and velocity remain distinct
from the reusable cached definition. General slot survival/recomposition is
not settled by this layout or by held PR #123.

## First letter assembly

1. Resolve a selected linguistic letter address in `02` and its ordered
   core-character parents in `00`.
2. Follow each core character's exact UTF-8 byte construction. ASCII already
   uses the byte endpoint; do not insert a duplicate byte object. Keep nibble
   construction followable as underlying source detail, without requiring
   it to be exposed in this letter/byte study.
3. Preserve the source character-parent ordinals and a mapping from each
   projected byte occurrence back to its character ordinal and byte ordinal.
   Native `assembly::prepare` derives parent fields from
   `Definition.parents[].value`; its exact contribution counts must account for
   whole mass. The plan must specify the study-facing projection explicitly,
   including whether parent fields refer to character values or projected byte
   values at each exposure. Do not overwrite stored character-parent edges or
   count both projected bytes and their character aggregate as constituents at
   once. Resolve masses from verified definitions; NULL is not zero.
4. Bind the language-letter object as the primary study level, core-character
   naming equivalence at 1:1 where it represents the same complete value, and
   bytes as the underlying level. A multi-scalar component is not equivalent
   to each individual character that constitutes it. Same-scale composition
   does not imply identical values. The finest
   exposed level anchors distance units; changing exposure must use explicit
   scale roles, not raw SNode depth.
5. Follow direct language/alphabet/punctuation memberships and available
   broader-group links. Retain distinct group representations. Define how
   nested membership fields select/count live participants. Deduplicate paths
   by target field and live occurrence: English A reaching English directly
   and through its alphabet contributes once to the English field. Its English,
   alphabet, parent-value and sibling fields remain distinct participations;
   distinct live occurrences of A also remain distinct. Warm records retain the
   source membership graph; hot preparation derives this participation. This is
   still to be implemented; group anchors with NULL naming mass are not
   fabricated massive literal particles.
6. Supply geometry through the existing native enclosure boundary. Publishing
   prepared warm tools and materializing a runnable hot study are separate
   handoffs, with the failure boundaries below. No rotation, positional
   parent offsets, string geometry or new force law is implied.

Initially exercise a small connected selection: distinct language endpoints
sharing a core character, one multibyte letter, the actual multi-scalar Dutch
component `íj́` (U+00ED U+006A U+0301), a repeated-constituent fixture
and available language/family memberships. Extend to the retained inventory
after the assembly contract is demonstrated. Source coverage is the populated
batch, not every language by assertion.

### Projection contract required before implementation

Stage B must supply an exposure matrix specifying what
`Definition.parents[].value` identifies at letter, character and byte exposure.
Choose and review either source-character parent identities with a derived byte
view, or byte identities in the byte-exposed native definition with a retained
character provenance map. Do not leave this choice to separate implementers.
Stored cold character-parent edges remain unchanged in either design.

Structural mass is the sum of verified character-parent masses, counting each
recorded occurrence, or the verified definition mass consistent with that sum.
A byte projection must conserve that mass; it is not an additional contribution
on top of the character aggregate. Comparing the projected-byte sum with the
character sum is a valid conservation check. Unknown mass remains unknown, not
zero. For Dutch `íj́`, one linguistic identity has three ordered scalar parents:
whole-value identity differs from each constituent, while structural mass
remains additive. Exposure must neither collapse identity nor double mass.

B and C must publish a shared vocabulary and mapping before D starts:

| Identity | Required distinction |
|---|---|
| Source definition | Root-qualified address and pinned revision; reusable across live uses. |
| Study-local occurrence key | One selected live use, distinct from its definition and GPU slot; repeated uses receive distinct keys. |
| Projection path | Character ordinal and byte ordinal within that occurrence, sufficient to round-trip the selected construction. |
| Target field key | The particular membership, parent-value or sibling field. Duplicate membership paths use this key together with the live occurrence key. Parent contributions retain their ordinal/projection path and exact count, even when their field and live occurrence are the same. |

The bounded plan must name concrete key fields and their scope. D consumes
B/C's keys and projection maps directly instead of re-deriving composition.
This does not promise occurrence survival across arbitrary focus changes,
general recomposition or other held #123 behaviour.

The [concrete A–D proposal](warm-toolbox-implementation-plan.md) chooses the
first exposure matrix, PostgreSQL publication procedure and existing SoA hot
layout for review. It preserves repeated parent contributions in the same
parent-value field. Its bounded persisted recipe includes repeated request entries,
aggregate/part choices, normalized logical nodes, initial positions/velocities,
geometry and contacts, so a fresh client can reconstruct it without the preparing
client's memory. These are initial inputs, not a snapshot of mutable hot state.
The proposal specifies letter-only units 1 even with declared inactive byte
alternatives; exposed-byte studies use byte units 1 and letter units 64. Exposure
is normalized before native `prepare`, which does not infer that rule itself.
These choices were accepted in #137. Stage A stores supplied fixtures; B supplies
accepted normalization/projection, and C supplies accepted contextual membership fields. Full hot materialization remains a later stage.

## Multiple writers and warm publication

Cache-manager writes refresh shared source-backed tools. Analyst-owned private
constructions remain at active hot assembly and do not pass through warm
publication. Generic temporary writer concurrency remains a built Stage A
capability under its accepted bindings; this clarification does not assign a
new policy to every `z*` object. Shared source definitions remain unchanged when
the analyst adds local context or working relationships.

Propose staged revisions followed by transactional publication of the ready
revision and its dependency set. A competing update to the same publication
must detect an unexpected prior revision rather than silently overwrite it.
Different roots/objects can prepare concurrently. Decide the precise lock or
compare-and-publish mechanism in the plan.

For the first letter study, propose a publication unit covering the selected
letter definitions, their core/byte dependency closure and the membership links
and group references used by that study. Pin the exact revisions together.
Publication does not require refreshing the entire Linguistics root. The plan
must specify how shared dependencies are reused and how a changed dependency
invalidates affected study publications.

Readers acquire one coherent publication. They must not combine a new parent
list with old masses, geometry or membership preparation. Retain a pinned old
publication until its consumers release it; define stale-use policy explicitly
rather than promising that a completed cache entry is perpetually current.
A failed preparation leaves the last complete version available and reports
the requested version as incomplete; it must not signal ready for partial work.
This preserves warm availability, not rollback of an already modified engine.

A request uses an existing suitable publication or causes preparation before
the requested handoff. Proactive cache-manager work follows the same publication
rules. Cache-manager DONE means the requested prepared tools/view are ready
for consumption, not that all cold-store work is finished or that a live engine
has been materialized. Define this contract without implementing analyst
reasoning or a new scheduler.

## Hot materialization and engine readiness

The analyst-owned workbench consumes pinned shared warm tools and separately
supplied local fixtures/inputs, assembles working instances and prepares native
compiler/layout inputs plus numerical execution values. Tools-ready, assembly
completeness and execution-ready are separate outcomes. The [pre-D design](pre-d-hot-workbench-design.md)
compares portable host surfaces, direct assembly, transfer and restart lifetimes;
it is not implemented or released merely by C acceptance.

The existing bounded `FixedStudy::publish` invalidates its live harness before
preparation. A failed publication leaves it non-runnable, and refresh rejects
changed array counts. Keeping an old warm revision does not restore the old
live engine state. The initial plan must respect these limits, seed valid
centroids before the first force pass and distinguish tools-ready, hot-ready
and failed/non-runnable outcomes. General rollback, count-changing refresh and
occurrence survival remain outside this slice unless separately accepted;
held #123 supplies no authority to assume them.

The engine's equal-count refresh restriction does not constrain warm publication
size. A warm revision can be complete even when it cannot refresh a particular
existing hot study; the consumer must report that boundary explicitly. No
count-changing hot refresh is authorized by accepting a warm revision.

## Personal integration and reciprocal boundaries

At active hot assembly, an analyst-owned construction can reference shared and
local objects without copying their identities. Contextual/reciprocal links
belong to that instance-owned workbench, not the manager's warm projection or
source stores. No shared-core write is implied by referencing a core parent.
The analyst's private source database can remain persistent; active-only entry
in this pipeline does not mean deletion of its source records.

Shared-root ownership is not a release permission for a mixed/private-derived
assembly. Private DB codes/data are absent from the manager's normal workload;
attempted private-scope requests must fail before source lookup. A shared refresh
delivers new shared tools to the active owner, which retains responsibility for
its private inputs, local context, joint assembly and reports.
No automatic outward promotion is introduced. The existing
[instance-local boundary](instance-local-data.md) remains governing.

Temporary-root expiry must respect active readers and dependent constructions.
Define reclamation and source invalidation separately from cold deletion.
No encryption scheme or result-release mechanism is chosen here.

## Staged implementation handoff

The interfaces below were accepted together in #137. Tracker #139 records Stage A
accepted/merged in #141 and B accepted/merged in #143. C is reviewed with green CI
in #146, accepted and merged at `43f418a`. The expanded #145 design is accepted in #147; D still requires
explicit release in #139; C acceptance alone is insufficient.
Use these shared identities and readiness semantics in focused stage PRs.

| Stage | Deliverable and boundary | Depends on | Acceptance evidence |
|---|---|---|---|
| A. Warm toolbox storage | Built PostgreSQL layout, scoped storage capabilities, dependency revisions and coherent publication. Production manager handles shared tools only. No live engine state. | Reviewed identity/root contract, with private active-only correction above. | Existing concurrency/publication/pin evidence remains; isolated personal-storage tests are capability evidence, not authority for a manager private-data path. |
| B. Native construction projection | Letter/character/byte projection, retained source ordinals, exact masses and explicit scale roles; mapping to bounded native records. | Definition/reference interface from A; fixtures may precede DB integration. | ASCII, multibyte, Dutch multi-scalar component, repeated constituents and shared core parents without identity collapse or double-counting. |
| C. Membership-field preparation | Preserve warm membership records; derive hot participation per target field/live occurrence and distinct group representations. | A's membership interface and B's occurrence/contribution contract. | Direct-plus-alphabet paths count once in English; different target fields and distinct instances remain separate. NULL group naming mass is not invented literal mass. |
| D. Host workbench and execution handoff | Consume shared tools plus separate analyst-owned supplied local inputs, assemble objects/relationships and prepare the backend surface. | A–C acceptance, reviewed #145 pre-D design and explicit #139 release. | Exact ownership/access boundary, native format and lifetime, density-aware comparisons, mathematical coherence, seeded execution and bounded restart/refresh/failure; no assumed live rollback. |

Plan development should respect the `y*` / `z*` reservations while keeping internal
allocation/delegation deferred. Specify source refresh markers, direct-follow
references, publication granularity and a concrete concurrency mechanism without
adding a general runtime framework. Tests can act as analyst clients.

Stage A acceptance must also show unknown/conflicting root bindings rejected,
`01` left unchanged, and reservation metadata kept distinct from concrete
bindings/identities. Isolated fixture configuration may explicitly authorize
codec-valid bindings inside `y*` / `z*`; these are not production subdivisions.
Reject wildcards as object identities and reject reservation-only writes.
Show that ordered lexical or “starts with” range child sets remain exact
navigation sets and never produce live particles. Native range storage remains
the separate #134 follow-up, not a prerequisite for this layout.

B and C can use agreed fixtures while A is built, but their contracts must
agree before D integrates them. The implementation work items should link to
this shared plan and list their inputs/outputs and exits. No additional physics
qualification, speculative analyst implementation or general hot-storage
framework is required.

The concrete proposal supplies reviewable defaults for identity qualification,
writer ownership, source epochs, persisted initial recipes, exposed-scale
normalization, geometry inputs, participation, publication, stale reads and
retention. The top-level personal/temporary prefix choice is resolved. Internal
allocation, writer delegation and eventual lifetime remain deferred; shared-root
letter work uses the existing `00` / `02` bindings without waiting for them.

This is plan-development input, not authorization to implement all open
interfaces or modify existing stores. Operational code is C++; Python remains
restricted to early bulk bootstrap population and its preparation/validation.
WAL, endpoints and engine harness keep their existing separate responsibilities.
