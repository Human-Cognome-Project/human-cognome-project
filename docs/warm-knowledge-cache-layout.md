# Warm knowledge cache: root-addressed layout

Status: **Layout proposal for implementation-plan development, 2026-10-13.**
Patrick's directions below are the design basis. The logical layout and
publication mechanism proposed here need refinement into a bounded native
implementation plan; no schema, cache manager or analyst is implemented by
this document.

## Purpose and settled directions

The warm knowledge cache is expected to use PostgreSQL and permit multiple
writers. The cache manager supplies prepared primary-feed objects ahead of
analyst demand or in response to an analyst request. The analyst may also use
the same working store for temporary object construction and integration of
personal databases. This is more than multiple cache-manager workers.

Use root-addressing protocols to distinguish owning sources and working
namespaces. Shared-source, personal-source and temporary objects use the same
structural representation; their origins do not require separate object types
or a copy of every shared definition. Root rules govern writer authority,
lifetime and permitted propagation. Addresses identify those rules; actual
write paths must enforce them.

For the first study, language letters are the primary object LoD; their defining
UTF-8 bytes are the underlying base level. The intervening core-character
reference is a naming/construction equivalence, not an automatic conceptual
scale step. Language, alphabet/punctuation and language-family memberships
supply organizational fields. Follow actual recorded memberships; do not
invent family edges absent from the seed data.

## Root protocol

| Root role | Identity and ownership | Warm-cache use |
|---|---|---|
| Shared core `00` | Existing computational/encoding identities | Cache manager prepares source-backed definitions. |
| Shared Linguistics `02` | Existing language endpoint/group identities | Cache manager prepares letter objects and grouping relationships. |
| Instance-private source root/block | Allocation remains unspecified; includes personal databases | Integrate source-backed personal objects locally while retaining their source identity. |
| Working/temporary root/block | Allocation and lifetime remain unspecified | Analyst constructions receive their own identities and reference shared/private definitions. |

No new meaning is assigned to `01`. Do not choose private or temporary numeric
roots in implementation merely to fill examples.

Propose a configuration-level root directory: address prefix, owning instance
where applicable, source binding, allowed writer role(s), lifetime rule and
outward-propagation rule. Specify unambiguous prefix resolution and reject
unknown or conflicting bindings. Physical source bindings belong behind the
cache-manager boundary; object records remain location-blind.

Resolve whether private blocks are globally unique or qualified by an instance
context. A cache-local revision/publication key must not silently replace the
object's address identity. Preparing a shared object in a private warm cache
does not allocate another shared identity or authorize writing back to its source.

## Proposed logical records

These are logical record groups, not finalized SQL table names or a new cold
schema. Reuse the existing relationship semantics; decide physical tables and
keys in the implementation plan.

| Record group | Required content | Role |
|---|---|---|
| Root bindings | Root/prefix and the rules above | Resolve ownership, allowed writes and source context. |
| Prepared definition | Full object address, prepared revision, source dependency references, exact structural mass when known, preparation completeness | Reusable object description; shared and personal source identities are retained. |
| Ordered construction | Owning definition/revision, ordinal, referenced constituent identity/revision, exact contribution | Preserve repetitions and reconstruct the selected base structure. |
| Membership links | Owning object/group references and their direct reciprocal lists | Keep language/family organization separate from literal construction. |
| Study assembly description | Study/working root, selected definitions/revisions, primary boundaries, equivalence/compression attachments and exposure choices | Express a particular focus without assigning permanent LoD to a cold identity. |
| Prepared geometry/field data | Supplied component layout, enclosing extent, grouping identities and preparation inputs | Feed native preparation; distinguish literal mass from group aggregation. |
| Publication/dependency bookkeeping | Complete available revision, expected previous revision, pinned dependencies and unresolved preparation | Let readers acquire a coherent assembly while writers prepare updates. |

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
3. Preserve byte order and repeated occurrences. Resolve contribution masses
   from verified definitions; a NULL parent-row mass is not zero.
4. Bind the language-letter object as the primary study level, core-character
   naming equivalence at 1:1, and bytes as the underlying level. The finest
   exposed level anchors distance units; changing exposure must use explicit
   scale roles, not raw SNode depth.
5. Follow direct language/alphabet/punctuation memberships and available
   broader-group links. Retain distinct group representations. Define how
   nested membership fields select/count live participants without counting
   the same occurrence twice through multiple paths. This preparation is
   still to be implemented; group anchors with NULL naming mass are not
   fabricated massive literal particles.
6. Supply geometry through the existing native enclosure boundary, then publish
   a complete prepared object/study for consumption. No rotation, positional
   parent offsets, string geometry or new force law is implied.

Initially exercise a small connected selection: distinct language endpoints
sharing a core character, one multibyte letter, a repeated-constituent fixture
and available language/family memberships. Extend to the retained inventory
after the assembly contract is demonstrated. Source coverage is the populated
batch, not every language by assertion.

## Multiple writers and publication

Proposed ownership: cache-manager writes refresh source-backed prepared
definitions; analyst writes own temporary constructions under the working
root. Integrating a personal object does not entitle either writer to overwrite
its source or another writer's working object. If the analyst needs to vary a
shared definition, construct a working-root object referencing it.

Propose staged revisions followed by transactional publication of the ready
revision and its dependency set. A competing update to the same publication
must detect an unexpected prior revision rather than silently overwrite it.
Different roots/objects can prepare concurrently. Decide the precise lock or
compare-and-publish mechanism in the plan.

Readers acquire one coherent publication. They must not combine a new parent
list with old masses, geometry or membership preparation. Retain a pinned old
publication until its consumers release it; define stale-use policy explicitly
rather than promising that a completed cache entry is perpetually current.
A failed preparation leaves the last complete version available and reports
the requested version as incomplete; it must not signal ready for partial work.

A request uses an existing suitable publication or causes preparation before
the requested handoff. Proactive cache-manager work follows the same publication
rules. Preserve the existing cache-manager DONE meaning: the requested view is
ready, not that all cold-store/deferred work is globally finished. Define this
contract without implementing analyst reasoning or a new scheduler.

## Personal integration and reciprocal boundaries

A temporary object can reference both shared and personal objects without
copying their identities. Its working root remains instance-local. Any reverse
links created for that construction live in the local warm projection; they
must not become writes into a global core merely because the parent has a
shared root. Distinguish projected warm reciprocals from source-store records.

Shared-root ownership is not a release permission for a mixed/private-derived
assembly. Root rules and dependency provenance must prevent private material
from being laundered through a shared cache entry. Refreshing a shared dependency
must preserve the analyst's construction and its private boundaries.
No automatic outward promotion is introduced. The existing
[instance-local boundary](instance-local-data.md) remains governing.

Temporary-root expiry must respect active readers and dependent constructions.
Define reclamation and source invalidation separately from cold deletion.
No encryption scheme or result-release mechanism is chosen here.

## Handoff for plan development

Develop one bounded native C++/PostgreSQL plan from this layout:

1. Specify root resolution, reserved working/private allocation and writer
   ownership. Keep unresolved numeric assignments explicit.
2. Map the logical records onto a minimal warm schema and direct-follow access
   contract, including source refresh markers and reference closure.
3. Specify transaction/publication and lifetime behavior for concurrent manager
   refresh and analyst construction. Choose a concrete mechanism rather than
   adding a general versioning or transaction framework.
4. Define native letter-to-byte SNode materialization and membership-field
   preparation using the accepted bounded engine API. Identify extensions
   needed for grouping; do not assume they already exist.
5. Demonstrate exact construction, repeated occurrences, equivalence scaling,
   distinct language objects, group participation, coherent concurrent reads,
   failed-publication recovery, preservation of temporary objects during source
   refresh and inward-only personal integration. Tests can act as clients;
   no analyst implementation is required.

Open decisions: private/temporary roots and instance qualification; whether
writer ownership is root-wide or delegated within a root; dependency revision
markers; geometry inputs; nested membership participation; stale-use and
reclamation policy; physical schema and publication granularity.

This is plan-development input, not authorization to implement all open
interfaces or modify existing stores. Operational code is C++; Python remains
restricted to early bulk bootstrap population and its preparation/validation.
WAL, endpoints and engine harness keep their existing separate responsibilities.
