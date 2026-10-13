# Letter seed population

Status: **Accepted for bounded bootstrap implementation, 2026-10-13 (PR #132).**
The numeric map is accepted for this initial seed batch; it is not evidence
of allocated records. Implement the writer and demonstrate the checks below
in isolated databases. Review its schema changes and population evidence before
applying it to an existing user store. No retained core has been modified.

The current test objects are individual letters in each language. Populate
their distinct linguistic endpoints, ordered core character parents, reciprocal
core child listings and language/alphabet memberships. Preserve the punctuation
inventory already prepared. This work does not implement object or SNode
assembly, cache-manager functions, runtime routing or analyst behavior.

## Accepted bootstrap address allocation

[bootstrap-address-map.json](../data/linguistics/bootstrap-address-map.json)
gives the accepted bootstrap map for the 20 source locales and the three existing
English variant group placeholders. The four characters after `02` express
major family, subfamily, language and regional delta. These are practical
bootstrap groupings, not a complete human-language taxonomy or a final claim
about common ancestry. Unused codes remain available for additional families,
isolates and scripts.

| Grouping | Base language prefixes |
|---|---|
| Germanic | English `02.00.00`, German `02.00.10`, Dutch `02.00.20` |
| Romance | French `02.01.00`, Spanish `02.01.10`, Portuguese `02.01.20`, Italian `02.01.30` |
| Slavic | Russian `02.02.00`, Ukrainian `02.02.10`, Bulgarian `02.02.20`, Serbian `02.02.30`, Czech `02.02.40` |
| Hellenic | Greek `02.03.00` |
| Indo-Iranian | Persian `02.04.00`, Urdu `02.04.10` |
| Armenian | Armenian `02.05.00` |
| Semitic | Arabic `02.10.00`, Hebrew `02.10.10` |
| Oghuz | Turkish `02.20.00` |
| Georgian | Georgian `02.30.00` |

Under each prefix, reserve the fourth couplet `00` for word components. The
last couplet reserves `00` for the language grouping, `01` for its alphabet
grouping and `02` for its punctuation grouping. Component endpoints begin at
`03`. This leaves 3,841 component slots per initial language frame. The current
batch fits; complete words and compound phrases are outside this allocation.

For the first allocation, list alphabet members before remaining punctuation
members, using exact scalar order within each set. This ordering does not claim
to be an alphabet's formal order. Preserve shared source forms and exact
language-specific identities. Keep a reviewed address ledger after allocation;
new entries append to unused slots rather than changing existing addresses.

English variant placeholders use `02.00.01` (UK), `02.00.02` (Canada) and
`02.00.03` (Australia). They remain grouping placeholders, with no new regional
letter data or fallback rules. The modern English base retains the current `en`
inventory. No `01` namespace meaning is assigned.

Broader group anchors use reserved frames: Germanic at `02.00.zz.00.00` and
Indo-European at `02.0z.zz.00.00`. Their membership links stay separate from
literal construction. Group anchors retain unknown structural mass until their
own naming construction is defined; no grouping centroid is calculated here.
Reserve these broader-group frames in the ledger so later language/regional
allocation cannot consume them. Existing assigned addresses never shift to
reflect a revised linguistic classification.

## Cross-store data requirement

The current schema cannot store the approved links across two PostgreSQL
databases as written. `hcp_linguistics.token_parent.parent_token_id` would
refer to a core token absent from its local `token` table; a new
`hcp_core.token_child.child_token_id` would refer to a linguistic token absent
from the core's local `token` table. Both columns currently have local target
foreign keys. PostgreSQL cannot enforce a foreign key across databases.

The proposed bootstrap change keeps each row's owning-token foreign key and
all local membership foreign keys. It replaces only the two incompatible
destination foreign keys with explicit validation against the destination
store during the bulk population cycle. Global address roots identify the
store: `00` is core, `02` is Linguistics. No core character copies, remote-token
placeholder rows, new type columns or parallel child-list tables are introduced.

The population implementation must present these exact schema changes for
review along with its writer:

```sql
-- hcp_core: child destinations may be owned by Linguistics.
ALTER TABLE public.token_child
  DROP CONSTRAINT token_child_child_token_id_fkey;

-- hcp_linguistics: character parents are owned by hcp_core.
ALTER TABLE public.token_parent
  DROP CONSTRAINT token_parent_parent_token_id_fkey;
```

These statements are a proposal, not an instruction to run them now. Removing
the local destination keys changes where their integrity is enforced. The
bootstrap writer must validate local as well as external destinations and
reject missing or conflicting endpoint definitions before committing. Native
record operations remain unchanged and receive no new cross-store behavior
from this bootstrap work. Existing PR #111 integrity findings remain separate.

### Schema, mass and writer boundary

Create Linguistics from the reviewed table/key schema, not by cloning the
populated core dump with its local-parent mass trigger. That trigger resolves
parents in its own database and cannot derive a linguistic endpoint's mass
from a core parent. Preserve the existing core trigger and core definitions.

For this bootstrap, validate each parent against the selected core and compute
the linguistic endpoint's whole mass as the sum of those verified parent
masses, once per ordered occurrence. Write that mass explicitly and compare it
with the seed evidence. New parent-row mass cells remain NULL under the current
deferred-cell convention; do not reinterpret NULL as zero. Group anchors retain
NULL whole mass until their naming constructions are supplied. Read back these
distinct mass policies along with exact source notation and rendered addresses.
Reject an unexpected destination schema/trigger configuration before writing;
do not disable triggers globally to make the load succeed.

Dropping a destination FK affects subsequent writers too. Run the bounded load
with exclusive write access to both selected stores, protecting validation
through commit/recovery. After cross-store links exist, keep mutations on these
populated stores restricted to the reviewed bootstrap procedure; the unchanged
native controller does not preserve remote reciprocals during mint/rekey/delete.
Read-only inspection remains available. This restriction does not request a new
runtime router or cross-store controller in this slice.

## Bulk population cycle

Use Python solely for this early bulk population and its preparation/checks.
The writer consumes the approved v2 manifest, reviewed numeric map and address
ledger, with explicitly selected core and Linguistics PostgreSQL connections.

1. Resolve every seed key to a unique five-couplet address and verify the full
   batch before writing. Check component capacity, grouping membership,
   contiguous parent ordinals, exact repeated occurrences and all reciprocal
   core child additions. Require the expected current core values and masses.
2. Compare any existing destination records with the complete proposed
   definitions. An unchanged rerun is allowed; conflicting or partial existing
   definitions fail. Keep core character definitions, parents and memberships
   unchanged. Insert linguistic endpoint/group records, ordered parent rows,
   both membership directions and additive core child rows.
3. Use a recoverable PostgreSQL transaction procedure for both stores. A
   failed preflight or preparation must leave no new rows. Once both store
   batches are prepared, complete both or recover the same recorded batch;
   do not report population complete from only one store's commit. This is a
   bounded bootstrap operation, not a new runtime WAL or database kernel.
   Persist the batch identity, input/ledger hashes, selected store identities
   and commit/abort decision before they are needed for recovery. A crash after
   one store prepares or commits must be distinguishable from an unrelated
   partial/conflicting pre-existing batch: recover the recorded batch rather
   than rejecting it as an ordinary rerun conflict. Do not roll back one side
   after a commit decision. Refuse unsupported transaction configuration before
   any schema/data changes. Implement this as a bounded bootstrap procedure,
   with no standing coordinator service or runtime work queue.
4. Read back the records from both stores. Verify exact notation, mass,
   ordered parent occurrences, separate membership reciprocals and every
   language endpoint in each corresponding core child listing. Preserve all
   pre-existing core child entries. Run the same batch again and verify zero
   additional rows or changed definitions.
5. Retain the addressed seed ledger and a reproducibility export after the
   approved population. Include exact counts and checksums in the data PR.

For the current 20-language batch, expect 2,707 linguistic component endpoints
plus 65 grouping anchors (2,772 tokens), 2,741 ordered parent rows, 5,460 rows in
each membership direction, and 2,741 additive child rows in the core. The full
retained core's 4,333,104 existing child rows would become 4,335,845; its token,
parent and membership data must remain unchanged.

## Validation boundary

The proposal covers all 20 source locales with 23 unique base/variant frames;
the component counts fit every reserved range. PostgreSQL 16.15 was started
in an isolated container with no network listener, and functional SQL plus
repeatable startup were verified. A transaction-local probe accepted an
existing core parent/child pair and rejected a linguistic child under the
current local target foreign key with SQLSTATE `23503`. The failed probe left
no persistent tables or seed data. These checks demonstrate the allocation
bounds and schema limitation; they do not validate a population writer.

First exercise the population procedure in explicitly created, isolated
PostgreSQL 16 databases. Use the retained UTF-8 snapshot as the source for
verified core data; never reset an existing database. Check a successful
letter sample, an invalid sample that writes nothing, the complete batch,
read-back integrity and an unchanged rerun. Inject interruption after the first
store prepares, after both prepare, and between store commits; recover the same
batch, preserving all prior data and leaving no unresolved transaction from it.
Include the schema changes in rollback/recovery checks, not just inserted rows. Persistent population of an
existing user store waits for the reviewed map, schema change, writer and
explicit target connection. No assembly or physics test is part of this slice.
