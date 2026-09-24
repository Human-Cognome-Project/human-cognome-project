# hcp3_core schema — coordinator checklist

Concrete checks to run after applying `schema.sql`, using `verify.sql`
(section numbers match `verify.sql`'s `-- N.` headers).

1. **Tables exist.** Query 1 returns exactly 5 rows, ALL as `BASE TABLE`:
   `member_of`, `members`, `token`, `token_child`, `token_parent`. The
   follow-up query returns 0 rows: `token_sibling_group` must not exist.
   Fail if any expected table is missing, any extra objects appear, or
   `token_child` shows as `VIEW`.

2. **`token` shape.**
   - 4 columns: `token_id` (array, NOT NULL), `token_text` (text, nullable),
     `notation` (text, nullable), `mass` (integer, nullable). The follow-up
     query returns 0 rows: `type` is dropped (type is emergent/LoD-relative,
     never a stored column — see NOTES.md "Relationship model & type —
     firmed 2026-09-17").
   - `mass` is nullable with no default (base atoms carry a real value such
     as 0; composites stay NULL until later logic fills them).
   - PRIMARY KEY is exactly `token_id`.
   - CHECK constraint `token_id_nonempty` exists (still the only CHECK — no
     CHECK on `mass`).

3. **`token_parent` shape.**
   - 4 columns: `token_id`, `ordinal` (integer, NOT NULL), `parent_token_id`,
     `mass` (integer, **nullable** — changed from NOT NULL: blank at
     declare, filled later by the deferred aggregation workstream; NULL is
     distinct from a real 0) — `token_id`/`parent_token_id` are arrays,
     NOT NULL.
   - PRIMARY KEY is `(token_id, ordinal)`, in that order.
   - Exactly 2 FOREIGN KEYs, both to `token(token_id)`: one from `token_id`,
     one from `parent_token_id`.
   - **No** index named `token_parent_parent_token_id_idx` (or any other
     secondary index on `parent_token_id`) — reverse-search indexes are not
     allowed; reverse lookups go through `token_child` instead.

4. **`members` shape** (membership, downward: a group's member roster).
   - 2 columns: `token_id`, `member_token_id` (arrays, NOT NULL). No mass
     column, no kind/category column — "kind" is read from which axis a
     row sits on, never stored.
   - PRIMARY KEY is `(token_id, member_token_id)`.
   - Exactly 2 FOREIGN KEYs, both to `token(token_id)`.
   - One-level arrayed follow keyed on the leading column (`token_id`); PK
     index only, no secondary index (covered by check 9).

5. **`member_of` shape** (membership, upward: the groups a token joins).
   - 2 columns: `token_id`, `group_token_id` (arrays, NOT NULL). No mass
     column, no kind/category column.
   - PRIMARY KEY is `(token_id, group_token_id)`.
   - Exactly 2 FOREIGN KEYs, both to `token(token_id)`.
   - Reciprocal of `members`: both directions are written together (by
     `add_membership` in the controller), so `members`/`member_of` stay
     mutually consistent the same way `token_parent`/`token_child` do.

6. **`token_child` — a real, materialized table.**
   - `relkind` is `r` (ordinary table), not `v` (view).
   - 2 columns: `token_id`, `child_token_id` (arrays, NOT NULL).
   - PRIMARY KEY is `(token_id, child_token_id)`.
   - Exactly 2 FOREIGN KEYs, both to `token(token_id)`: one from
     `token_id`, one from `child_token_id`.
   - This table is written by the write/mint controller ("fold AND wire"),
     not derived by SQL — nothing here computes its contents.

7. **No triggers/functions.** Query 7 returns 0 rows across all five
   objects — confirms no logic snuck into the schema layer.

8. **Address columns carry `COLLATE "C"` (firmed 2026-09-18).** Query 8's
   first check returns exactly 9 rows, ALL `collname` = `C`:
   `token.token_id`, `token_parent.token_id`, `token_parent.parent_token_id`,
   `token_child.token_id`, `token_child.child_token_id`, `members.token_id`,
   `members.member_token_id`, `member_of.token_id`,
   `member_of.group_token_id`. This is the fix for the case-interleaved
   default collation (e.g. `en_US.UTF-8`) not matching
   `codec::kAlphabet`'s byte order — without it the PK btree order does not
   equal address order, so a contiguous trunk is not a contiguous PK range
   (gather/range queries silently drop members or degrade to a full index
   walk). The second check (`token_text`, database default collation)
   returns 0 rows — `token_text` is plain debug text, not an address
   column, and must stay on the default collation, never pinned to `"C"`.

9. **PK indexes only.** Query 9 lists every index across all five tables;
   every `indexname` must end in `_pkey`. In particular, confirm
   `token_parent_parent_token_id_idx` does NOT appear, and neither
   `members` nor `member_of` carries any index beyond its own PK — those
   reverse-search indexes are not allowed; the CPU only follows stored
   lists, it never searches.

## Shape check for the foundational root (structural only — no data loaded)

Not run by `verify.sql` (there is no data yet), but worth a manual sanity
pass once the root is populated by the data-loading agent:

- `0x` (address `AA.AA.AA.AA.AA`) should be representable as a `token` row
  with **no** `token_parent` row where it is the whole (`token_id`) side —
  i.e. it has no composition, consistent with "unallocated base container:
  no weight/mass/presence." It may still appear as a `parent_token_id` in
  other tokens' `token_parent` rows, or as a `member_token_id` in
  `members` rows, once real particles are loaded.
- The 16 real particles plus `0x` should all share leading couplet `AA`
  (root namespace) and be distinguishable by their trailing couplet under
  `AA.AA.AA.AA.*`.
- This is a shape check, not a data check: nothing above requires rows to
  exist yet, only that the schema does not prevent this shape.
