# hcp3_core schema — cold swarm cache

> **Address transition (2026-09-26):** The codec now implements the RFC 4648
> §5 alphabet, whose value order differs from `COLLATE "C"` byte order for
> digits, `-` and `_`. It is decided that address columns move from `text[]`
> to `smallint[]` pair codes, whose integer order is address order (see
> [storage key ordering](../../../docs/address-encoding-transition.md#storage-key-ordering-decided-2026-09-26)).
> Until that storage step lands, this schema is unchanged, and the controller
> stores only the letter subset `A–Z a–z`, for which byte order still equals
> address order. The `text[]` descriptions below are the current interim
> state.

Drafted schema for the `hcp3_core` database: the passive backing store for
the token-graph. Greenfield — no relationship to `hcp2_core` / `db/core.sql`.
No logic lives here (no triggers/functions); construction and validation
logic is the C++ layer's job.

Files:

- `schema.sql` — CREATE TABLE statements (five tables; no views).
- `verify.sql` — read-only information_schema/pg_catalog checks, each with the expected result in a comment.
- `README.md` — this file.
- `tests.md` — the checklist to run `verify.sql` against.

**Reverse-search indexes are not allowed.** The CPU only follows stored
lists; it never searches. Every index in this schema is a PK index needed
to resolve a direct address — nothing else. This is why `token_child`
(child expressions) is a materialized table rather than a view, and why
`token_parent`, `members` and `member_of` carry no secondary index on their
reference columns (see the mapping table and OPEN TEST SLOTS below).

## Two relationship axes, each a reciprocal pair

Rebased 2026-09-17 (see `NOTES.md` "Relationship model & type — firmed
2026-09-17", which governs on conflict). The store holds two orthogonal
relationship axes, each a one-level arrayed follow, each written both
directions by a single sole-owner controller call:

- **Structure — `token_parent` / `token_child`.** A token's ordered,
  literal constituents, and the reciprocal reverse-adjacency ("who uses me
  as a constituent"). Unchanged in shape from before the rebase.
- **Membership — `members` / `member_of`.** What a group directly contains
  (`members`), and the reciprocal: the groups a token directly belongs to
  (`member_of`). This REPLACES `token_sibling_group` in full; that table is
  dropped.

Neither axis stores a "kind"/category column. Which axis a row sits on
(structure vs membership) IS the kind — read from the table, never from a
stored label. `token.type` is dropped for the same reason: type is
emergent and LoD-relative (a token reads as a label when it has `members`
in scope, a terminal literal when it does not), so it cannot be a stored
branch.

## token_id representation choice

`token_id` is `text[]` — the address itself, one base-50 couplet per array
element, used directly as the PRIMARY KEY and as the FK target everywhere
else in the schema. The address *is* the canonical identity, so there is no
separate surrogate identity to keep in sync with it.

A second column, `token.token_text`, holds a dot-joined rendering (e.g.
`AA.AA.AA.AA.AA`) for human/debug legibility. It is a plain column populated
by the loading layer, not a SQL-generated column — derivation logic stays
out of the schema, per the "no clever triggers/functions" constraint.

## Address column collation

Firmed 2026-09-18. Every `text[]` **address** column — `token.token_id` (the
PK) and the four FK columns that reference it (`token_parent.token_id`,
`token_parent.parent_token_id`, `token_child.token_id`,
`token_child.child_token_id`, `members.token_id`, `members.member_token_id`,
`member_of.token_id`, `member_of.group_token_id`) — is declared
`COLLATE "C"`.

**Why.** `text[]` element comparison uses the array element type's
collation. Left unpinned, these columns inherit the database's default
collation (here `en_US.UTF-8`), which orders **case-interleaved**
(`aAbB…`). `codec::kAlphabet`'s order (`A-Z` minus `O`, then `a-z` minus
`o` — i.e. `A-N,P-Z,a-n,p-z`) IS plain byte order. So under the default
collation the PK btree's sort order does **not** match address order: a
contiguous trunk (a terminal-wildcard prefix, or a `FROM..TO` range) is
**not** a contiguous PK range. That breaks every operation that depends on
walking a trunk as one bounded key interval — the **gather** primitive
(the terminal-wildcard / range enumerator backing `MOVE`'s bulk source,
`ADD_CONNECTION`'s wildcard expansion, and `READ`'s wildcard anchor), and
sequential fill. A bare range scan under the wrong collation silently
drops members that sort out of the assumed contiguous window; a
query-side `COLLATE "C"` override on an unpinned column still works but
degrades planning to a full index walk — the reverse-search cost this
schema forbids. Pinning the column itself is what turns the range into a
genuine, planner-recognized **Index Cond** bounded scan on the PK.

This is a **pure collation pin, not an alphabet change**: byte order
already equals the codec's documented order, so no character or ordering
semantics change, only which collation the comparison operators use.
**Equality / point reads are unaffected** — collation only governs
ordering (`<`, `<=`, `>`, `>=`, `ORDER BY`, btree range scans), never `=`.
The arrayed `text[]` storage itself is unchanged (still chosen for address
compression and to avoid re-parsing a dot-joined string on every action).

`token.token_text` (the dot-joined human/debug rendering) is deliberately
**left on the database default collation** — it is plain text, not an
address column: nothing compares, ranges, or keys on it, so there is
nothing for the pin to fix.

## Alphabet / couplet encoding

Base-50 alphabet: `A-Z`, `a-z` (52 chars) minus `o` and `O` → 50 chars. Each
couplet is 2 characters from this alphabet. This is documented as config in
`schema.sql`'s header comment and here — **not** enforced by a CHECK
constraint. Changing it for an already populated store still requires an
identity/PK/FK and address-order migration, even though the SQL column type
can remain `text[]`.

## Format element -> schema mapping

| Format element | Table / column | Notes |
|---|---|---|
| Token identity | `token.token_id` (`text[]`, PK) | The address array itself is the canonical identity. |
| Human/debug rendering of identity | `token.token_text` | Loader-populated, not derived in SQL. |
| Temporary notation | `token.notation` | Nullable, TEMPORARY — construction-time reference check, will be dropped. |
| Token's own mass | `token.mass` | `integer`, nullable, no default. A literal's sum-of-parents, a label's centroid-of-members. Stored as given, never computed here; NULL means "not yet computed" (kept distinct from a real 0, e.g. `0x`). Aggregation is a later step. |
| Structure (1): ordered constituent list | `token_parent.token_id`, `token_parent.ordinal`, `token_parent.parent_token_id` | One row per (whole token, position); PK `(token_id, ordinal)`. One level of extension only. |
| Structure (1): per-element mass | `token_parent.mass` | `integer`, **nullable** (changed from NOT NULL): blank at declare, filled later by the deferred aggregation workstream; NULL != a real 0. Stored as given, never computed. |
| Structure (2): reverse adjacency | `token_child` (materialized table) | Stored, not derived: a read follows this table directly rather than searching `token_parent`. No position or mass stored. Deliberately redundant with `token_parent`; kept consistent by a single sole-owner write/mint controller that wires a new composite into each of its parents' child-expression lists on write ("fold AND wire"). |
| Membership (3): what a group contains, downward | `members.token_id`, `members.member_token_id` | PK `(token_id, member_token_id)`. No mass, no kind column. `token_id` is the group (its naming-literal token_id); `member_token_id` a direct member. |
| Membership (4): the groups a token joins, upward | `member_of.token_id`, `member_of.group_token_id` | PK `(token_id, group_token_id)`. Reciprocal of `members`; both directions written together by `add_membership`, same discipline as `token_parent`/`token_child`. |

## OPEN TEST SLOTS (from schema.sql, collected here)

1. **Delta-compressed address storage.** Addresses are conceptually "recorded
   only as the delta, the shared prefix assumed by position." This schema
   stores the full address array (lossless, testable) in every table that
   carries a token_id. Delta compression is a later optimization, not
   implemented — would need its own design, not an invented scheme here.

2. **`token_parent.ordinal` base.** Chosen 0-based as a documented
   convention. Not verified against the C++ producer's actual iteration
   order — confirm before data load; flip to 1-based if needed (single
   column, no structural change).

## Explicitly not addressed here

- No `CREATE DATABASE hcp3_core` — this file assumes it's applied while
  already connected to that database (coordinator's step).
- No indexes beyond the PK indexes required to resolve a direct address.
  In particular, `token_parent.parent_token_id`, `members.member_token_id`
  and `member_of.group_token_id` carry NO secondary index: reverse
  lookups happen only through stored reciprocal tables (`token_child`,
  `member_of`, `members`), never through a reverse-search index — the CPU
  only follows, it never searches.
- **Dropped in the 2026-09-17 rebase:** `token_sibling_group` (replaced in
  full by `members`/`member_of`) and `token.type` (type is emergent/
  LoD-relative, never a stored column). See `NOTES.md` "Relationship model
  & type — firmed 2026-09-17" for the full rationale.
