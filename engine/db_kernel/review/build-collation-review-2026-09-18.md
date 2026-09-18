# Adversarial review — `COLLATE "C"` pin on the `text[]` address columns

- **Reviewer:** fresh independent adversary (no stake in the outcome).
- **Date:** 2026-09-18.
- **Target:** the schema change pinning `COLLATE "C"` on the `hcp3_core`
  `text[]` address columns (`schema/schema.sql`, `verify.sql`, `tests.md`,
  `README.md`).
- **Method:** full read of the changed files + `codec/codec.h`, `NOTES.md`
  "Address column collation", `PLAN.md` §II.0; independent re-run of the
  catalog proof, the bounded-range proof (with a negative control), and all
  four affected test suites against a live local Postgres 16.15
  (`en_US.UTF-8` default, case-interleaved — matching the change's stated
  premise).
- **Files modified by this review:** none (this review file only).

---

## Verdict

**PASS.**

The change is a pure, correct collation pin. The bounded-`Index Cond`
proof and the correct-and-complete result both reproduced independently; a
negative control confirmed the documented failure mode of the unpinned
column. All four DB-backed regression suites are green against the
reset+reapplied edited schema. No must-fix or should-fix findings.

---

## Findings

Severity legend: **must-fix** / should-fix / nit / observation.

### F1 — DDL: all 9 address columns pinned, `token_text` correctly left default. **observation (PASS).**
- **Location:** `schema/schema.sql:96,147,157,201,205,234,237,257,261`.
- **Evidence:** the nine `text[] COLLATE "C"` DDL lines are exactly:
  `token.token_id` (PK); `token_parent.token_id` / `parent_token_id`;
  `token_child.token_id` / `child_token_id`; `members.token_id` /
  `member_token_id`; `member_of.token_id` / `group_token_id`.
  `token.token_text` (schema.sql:103) is left on the default collation and
  this is documented in-line and in README "Address column collation".
- **Why OK:** matches the required 9-column set exactly; the one debug-only
  text column is the only one excluded, and it is never keyed/ranged.

### F2 — Pure collation pin: no alphabet / couplet / PK-FK / structural drift. **observation (PASS).**
- **Location:** `git diff HEAD -- schema/schema.sql`.
- **Evidence:** filtering the added lines for anything that is neither a
  comment nor a `COLLATE` clause returns **empty**. The diff is: the nine
  `text[]` → `text[] COLLATE "C"` edits plus comments/docs (schema header
  block, README section, `tests.md`/`verify.sql` section renumber 8→9 with a
  new section 8). No change to the base-50 alphabet, couplet width, PK/FK
  definitions, cardinality CHECK, or table set.
- **Why OK:** confirms the change is scoped to collation only; `codec.h`
  `kAlphabet` ordering (`A-N,P-Z,a-n,p-z`) is plain byte order, so `COLLATE
  "C"` reproduces the documented order without any semantic change.

### F3 — Catalog proof reproduced independently. **observation (PASS).**
- **Location:** `verify.sql` §8; independent `pg_attribute`/`pg_collation`
  query over **all** columns of the five tables.
- **Evidence (my own query, all 14 columns):**
  ```
       tbl      |       col       |  type   |  coll
  --------------+-----------------+---------+---------
   member_of    | token_id        | text[]  | C
   member_of    | group_token_id  | text[]  | C
   members      | token_id        | text[]  | C
   members      | member_token_id | text[]  | C
   token        | token_id        | text[]  | C
   token        | token_text      | text    | default
   token        | notation        | text    | default
   token        | mass            | integer |
   token_child  | token_id        | text[]  | C
   token_child  | child_token_id  | text[]  | C
   token_parent | token_id        | text[]  | C
   token_parent | ordinal         | integer |
   token_parent | parent_token_id | text[]  | C
   token_parent | mass            | integer |
  ```
  All 9 address columns are `C`; `token_text` and `notation` are `default`;
  integer columns carry no collation. `verify.sql` §8 check A returns exactly
  these 9 rows all `C` (count = 9), and §8 check B (`token_text` non-default)
  returns 0 rows. The shipped tests assert precisely this.
- **Why OK:** the schema applies cleanly on a case-interleaved default DB and
  the catalog matches the documented intent exactly.

### F4 — Bounded-range proof reproduced independently (the load-bearing one). **observation (PASS).**
- **Location:** `README.md` / `NOTES.md` gather rationale; PLAN §II.0.
- **Setup:** disposable `hcp3_core` on `en_US.UTF-8`. A* trunk under
  `AA.AA.AA.AA.A*` including the pair the default collation misorders
  (`...AZ` vs `...Ab`), plus `AA` and `Az`; a sibling `B*` trunk
  (`...BA`, `...Bb`) and two other-parent trunks (`AA.AA.AA.AB.AA`,
  `AA.AA.AA.Az.AA`) to exclude; **40 000 filler rows** across many prefixes;
  `ANALYZE`; **`enable_seqscan` left ON** (not forced). Total 40 008 rows.
- **The misorder is real (scalar, same DB):**
  ```
   c_az_lt_ab | enus_az_lt_ab
  ------------+---------------
   t          | f
  ```
  `'AZ' < 'Ab'` is **true** under `C` (byte Z=90 < b=98) but **false** under
  `en_US` (case-interleaved b < Z) — so the default collation would reorder
  the trunk.
- **EXPLAIN (ANALYZE, VERBOSE, COSTS off), planner free:**
  ```
   Index Only Scan using token_pkey on public.token (actual time=0.019..0.021 rows=4 loops=1)
     Output: token_id
     Index Cond: ((token.token_id >= '{AA,AA,AA,AA,AA}'::text[]) AND (token.token_id < '{AA,AA,AA,AA,B}'::text[]))
     Heap Fetches: 0
   Planning Time: 0.604 ms
   Execution Time: 0.042 ms
  ```
  A genuine bounded **`Index Cond`** (both `>=` and `<` bounds) on the PK —
  Index Only Scan, **not** `Filter`, **not** `Seq Scan`, seqscan not forced.
- **Result correct and complete:**
  ```
     token_text
  ----------------
   AA.AA.AA.AA.AA
   AA.AA.AA.AA.AZ
   AA.AA.AA.AA.Ab
   AA.AA.AA.AA.Az
  ```
  Exactly the 4 A* members, in byte order — `AZ` **before** `Ab` (the
  misordered pair handled correctly), `Ab` present (not dropped), `Az`
  present; the `B*` siblings and both other-parent trunks excluded.

### F5 — Negative control confirms the documented "silent drop" hazard. **observation (PASS).**
- **Setup:** a twin `token_unpinned (token_id text[] PRIMARY KEY)` on the DB
  default collation, loaded with the same rows.
- **(a) Order under default collation:** `AA, Ab, Az, AZ` — `AZ` sorts
  **last** (case-interleaved), so the byte-order contiguity assumption fails.
- **(b) Silent drop:** the byte-order window `[..AA, ..Az)` (which should
  yield `AA, AZ, Ab` = 3 rows) returns only `AA, Ab` = **2 rows** on the
  unpinned column — `AZ` silently dropped — versus the correct **3 rows**
  (`AA, AZ, Ab`) on the pinned column.
- **Why this matters:** this is exactly the "a bare range scan under the
  wrong collation silently drops members" failure the schema comment /
  README / NOTES describe. It demonstrates the pin is load-bearing, not
  cosmetic.

### F6 — Regression: all four DB-backed suites green against the reapplied pinned schema. **observation (PASS).**
Built into scratch (not the project tree) with the README flags, each run
with the absolute edited-schema path so it reset+reapplied the pinned
schema. Verbatim tails:
- `controller_test` → **`PASS controller_test`**, exit 0 (58 `ok` checks,
  including schema-shape assertions, mint/link/wire, rekey, membership, FK
  rollback).
- `controller_advtest` → **`PASS controller_advtest`**, exit 0 (rollback,
  recovery, derivation, injection, distinct-reverse, idempotency, heal,
  rekey-collision, FK-blocked delete).
- `declare_core_test` → **`PASS declare_core_test`**, exit 0 (declare, nested
  declare, membership, rejection, use-provided-ID, atomicity boundary).
- `read_core_test` → **`PASS read_core_test`**, exit 0 (radial reads,
  structure/membership axes, no-dedup, exclusion by id and by `C*`
  terminal-wildcard prefix, wildcard-anchor deferral).

As predicted, equality/point reads are collation-independent, so the pin
causes no regression. `command/` is pure and DB-free — **not run** (correctly
out of scope).

### F7 — FK collation consistency holds. **observation (PASS).**
- **Evidence:** the schema applies with no error (FK columns and their
  `REFERENCES token(token_id)` target now share collation `C`), and the
  FK-violation regression checks (`mint with a missing constituent throws
  (FK violation)`, `add_membership rollback: missing group throws`) pass.
- **Why OK:** the pin is applied uniformly to both sides of every FK, so no
  collation-mismatch FK breakage is introduced.

### F8 — Scope confined to `schema/`; no OLD-MODEL-LEAK. **observation (PASS).**
- **Evidence:** `git status --porcelain schema/` shows only the four files
  under review as modified; nothing else touched by this review. The change
  introduces no pre-rebase constructs (`token_sibling_group`, `token.type`)
  and removes none — it is orthogonal to the model, a collation-only pin.

---

## Reproduction notes

- Postgres 16.15, default DB collation `en_US.UTF-8` (verified
  case-interleaved on the same server), so the "without the pin" premise is
  faithfully reproduced rather than assumed.
- The disposable `hcp3_core` was dropped after the run. Test binaries were
  built into the session scratchpad, not the project tree. No project file
  was modified.
- One incidental: the collation *object* for the default locale is named
  `en_US` (not `en_US.UTF-8`) in this cluster; the scalar flip demo uses
  `COLLATE "en_US"`. Immaterial to the change under review — it only affects
  how the contrast is written, not the result.
