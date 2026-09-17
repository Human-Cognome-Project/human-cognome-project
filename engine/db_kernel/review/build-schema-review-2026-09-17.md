# Adversarial build review — Schema + controller-door rebase (§II.0)

**Date:** 2026-09-17
**Reviewer:** fresh independent adversary (no stake in passing)
**Scope reviewed:** `schema/{schema.sql,verify.sql,tests.md,README.md}`,
`controller/{controller.h,controller.cpp,controller_test.cpp,controller_advtest.cpp,README.md}`,
consumed against `codec/codec.h`. Governing spec: `PLAN.md` §1, §2, §II.0, §II.5/§II.6, Part V;
`NOTES.md` "Relationship model & type — firmed 2026-09-17", Hard constraints, Governing principle.
**Working tree:** reviewed untouched; no file modified.

---

## 1. Independent build + test outcome (verbatim)

Environment: PostgreSQL 16.15 reachable via peer auth over the local unix
socket; `g++` (Ubuntu 13.3.0), libpq via `pg_config`. This is a real live
Postgres — nothing faked.

### schema.sql applied fresh + verify.sql

`schema.sql` applied cleanly onto a reset `public` schema (5 `CREATE TABLE`,
comments only — no functions/triggers). `verify.sql` output matched every
stated expectation:

- §1 exactly 5 BASE TABLEs (`member_of, members, token, token_child, token_parent`);
  `token_sibling_group` → 0 rows.
- §2 `token` = 4 columns (`token_id ARRAY NOT NULL`, `token_text text YES`,
  `notation text YES`, `mass integer YES`); `type` column → 0 rows; PK = `token_id`;
  one CHECK `token_id_nonempty`.
- §3 `token_parent` = 4 columns, `mass integer YES` (nullable, confirmed); PK
  `(token_id, ordinal)`; 2 FKs both → `token(token_id)`; no
  `token_parent_parent_token_id_idx`.
- §4 `members` = 2 array columns NOT NULL; PK `(token_id, member_token_id)`; 2 FKs → token.
- §5 `member_of` = 2 array columns NOT NULL; PK `(token_id, group_token_id)`; 2 FKs → token.
- §6 `token_child` `relkind = r` (table, not view); 2 columns; PK `(token_id, child_token_id)`; 2 FKs → token.
- §7 triggers → 0 rows.
- §8 every index across all five tables ends in `_pkey` (member_of_pkey,
  members_pkey, token_pkey, token_child_pkey, token_parent_pkey) — no
  secondary/reverse-search index anywhere.

### controller_test (base harness) — built clean (`-Wall -Wextra`, no warnings)

```
PASS controller_test
exit=0
```
All 55 checks `ok`, including: `schema: token_sibling_group no longer exists`,
`schema: token.type column no longer exists`, `schema: members and member_of both
exist`; SEE/MINT/LINK/WIRE; distinct-reverse dedupe; idempotent re-mint;
`member_of`/`members_of` both directions + `sorted deterministically`;
`attributes_of` NULL-vs-0; `mint: absent whole-token mass stored as NULL, not 0`;
`mint: absent per-constituent mass stored as NULL, not 0`; rekey across all four
stores + `member_of(moved) includes grp` + repeated-constituent repoint at every
ordinal + nonexistent-old throws + existing-new throws (no merge) + failed-rekey
leaves old intact; delete_pair reciprocal removal; delete_token; FK-rollback.

### controller_advtest (adversarial harness) — built clean, no warnings

```
PASS controller_advtest
exit=0
```
All 42 checks `ok`, including: multi-constituent FK rollback (no orphan
link/child, direct row count 0); connection recovery after a failed mint;
`token_text equals codec dot-join of token_id` and `!= notation`; SQL-injection
notation round-trips verbatim, table intact; distinct-reverse count = 1;
idempotent re-mint leaves notation/links untouched; `add_membership` rollback
of BOTH directions incl. a genuine partial-write (pre-seeded PK collision on the
second insert rolls back the first); rekey old_id gone from all four stores
(both columns, direct counts), new `token_text` codec-derived not carried over,
collision leaves both tokens untouched; delete_pair reciprocal-only; delete_token
FK-blocked (G10 default behaviour, no invented cascade) then succeeds once
unreferenced.

**Both suites green against live Postgres. No compile warnings.**

---

## 2. Test-quality judgement

**Green AND meaningful.** Every item the checklist demands is actually
exercised, not merely asserted at the shape level:

- `add_membership` writes BOTH directions — verified from each side
  (`member_of(p1)==grp`, `member_of(p2)==grp`, and `members_of(grp)==[p1,p2]`),
  plus a direct-SQL rollback proving both rows are undone on partial failure.
- Deterministic order — `members_of(grp)` asserted equal to `[p1,p2]` in sorted
  order, and the reads use `ORDER BY` on the followed column.
- `rekey` — repoints ALL FOUR stores (asserted by controller reads AND by direct
  `count(*)` on both columns of all four tables), re-derives `token_text` via the
  codec (compared to `encode_token_id`), leaves no dangling ref (old_id count 0
  everywhere), throws on existing `new_id` (no-MERGE) and on missing `old_id`,
  and a repeated constituent is repointed at every ordinal.
- `delete_pair` removes the reciprocal too (both `members_of` and `member_of`),
  and only the named pair.
- `mint` writes SQL NULL (not 0) for blank whole-token AND per-constituent mass,
  with mass=0 kept distinct from NULL.
- `token.type` / `token_sibling_group` gone — asserted directly against
  `information_schema`.

The harnesses reset the schema from `schema.sql` each run, so the tests validate
the reviewed DDL, not stale state. No test tests the wrong thing; none is a
tautology.

---

## 3. OLD-MODEL-LEAK check — CLEAN (for this module)

Grep across `schema/`, `controller/`, `codec/` for `type` column/param,
`token_sibling_group`, `groups_of`, `add_group_membership`, `SIBLING`, membership
kind column: **every hit is either an absence-assertion (verify.sql / tests /
schema-shape checks), a "dropped/replaced" doc line, or a reference to the NOTES
section title "Relationship model & type".** No live old-model surface:

- No `type` column (`schema.sql` token = 4 columns) and no `type`/kind param on
  `mint`; `TokenAttributes` has no `type`.
- No `token_sibling_group`.
- No `groups_of` / `add_group_membership` — replaced by `members_of` / `member_of`
  / `add_membership`.
- `members` / `member_of` carry no mass and no kind/category column (2 columns each).

---

## 4. Findings

No must-fix defects. Observations, all NON-BLOCKING:

**F1 — severity: none (faithful, noted for the record).**
`controller.h:135-137` / `controller.cpp:211-213` — `mint` retains an optional
whole-token `mass` parameter (default `std::nullopt`). PLAN §II.0's `mint` bullet
foregrounds `Constituent.mass` + NULL-for-blank and does not explicitly name a
whole-token mass param. Ruling: faithful-derivation, not over-reach — `token.mass`
is retained nullable **for the seed floor** (`NOTES.md`: "Only the seed floor
writes a mass literal (`0x` = 0/undefined; 16 hex = 1)"), and `mint` is the sole
write door for `token` rows, so it must accept the seed's declared mass. Blank
declares pass `nullopt` → SQL NULL. Correct.

**F2 — severity: none (spec-satisfied, mechanism noted).**
`controller.cpp:141-186` — determinism of `children_of`/`members_of`/`member_of`
is provided by `ORDER BY` on the array column (`child_token_id` /
`member_token_id` / `group_token_id`), i.e. Postgres element-wise text[] ordering.
PLAN §II.0 requires only "deterministic (sorted) order"; array ordering is
deterministic within the database. No collation is pinned, but none is required
by spec. Acceptable.

**F3 — severity: none (raw-primitive behaviour, correct at this layer).**
`controller.cpp:422-450` `delete_pair` and `:411-420` `delete_token` are ungated
(no active-confirmation, no peer tags, no wildcard-safety). This is **correct for
this module** — PLAN §II.0 and §II.5 place gating at a later op layer (Agent 5),
and controller.h/README document it plainly. Not a defect.

---

## 5. Invention / over-reach

None. Every element traces to spec. `rekey`'s insert-first / repoint / delete-last
ordering is a faithful realization of PLAN §II.0's "the primitive owns FK-safe
ordering itself (no ON UPDATE CASCADE, no triggers)". `delete_token`'s reliance on
the database's default FK behaviour for a still-referenced token is exactly the
G10-flagged, deliberately-uninvented policy (Part V). No anticipatory abstraction,
no imported machinery.

---

## 6. Downstream note (NOT a defect of this module)

Confirmed: `ingestion/` (`ingestion.cpp:77,91`, `ingestion.h`, `db_runtime.cpp:108,224`,
`ingestion_cli.cpp:125`, `ingestion_test.cpp:223,239`, `db_runtime_test.sh:43`) and
`seed/seed_0x.cpp:153,178,187` still call the removed surface (`mint(...point.type...)`,
`add_group_membership`, `groups_of`, `token_sibling_group`) and will **not compile**
against the rebased controller. This is correctly **out of this module's scope**
(PLAN Part III: Agents 3/6 own DECLARE core / dispatcher wiring, and the ingestion
rewrite). The schema+controller module itself is **self-consistent**: it builds,
links, and tests green standalone with only libpq + codec.

---

## 7. Four flagged decisions — adjudication

**D1 — `members`/`member_of` unordered (no ordinal), reads sorted: FAITHFUL-DERIVATION.**
`NOTES.md` firmed section makes ordering a **structure** property only ("a token's
*ordered* constituents (parent)") and calls membership a one-level "roster" that
"REPLACES the sibling-group in full" (sibling group had no ordinal). Membership
returning "the same shape the old parent/child returned" + PLAN §II.0's explicit
"Both new reads return a deterministic (sorted) order, matching `children_of`"
(itself an unordered store sorted at read) makes no-ordinal-store + sorted-return
the directly-specified design.

**D2 — `delete_pair` addresses the membership axis only: NEEDS-PATRICK.**
The spec settles that ADD_CONNECTION is membership-only (NOTES/PLAN I.D), and the
record tier "rekeys rather than deletes structure," which makes a membership-only
DELETE_CONNECTION a defensible dual — and the choice is *documented*, not silently
baked (`controller.h:176-178`). But `NOTES.md` also frames the substrate as
axis-agnostic pairs ("the store holds one thing — pairs"), and whether
DELETE_CONNECTION must be able to target a **structure** pair is nowhere resolved.
This is a genuine semantic-scope question. It does **not block this module** (the
`delete_pair` primitive is correctly a membership primitive); it should be
confirmed with Patrick at the op-layer (Agent 5) stage before an axis-agnostic /
structure-delete capability is either added or foreclosed.

**D3 — `delete_token` on a nonexistent id is a silent 0-row no-op: CODE-STRUCTURE.**
No precept dictates a raw primitive's behaviour on an absent target; a bare
`DELETE` naturally affects 0 rows without error, and this layer is explicitly the
ungated raw primitive (all meaningful gating deferred to the op layer). This is a
code-structure latitude call, not a semantic gap.

**D4 — `rekey` carries `notation`/`mass` verbatim, only `token_text` re-derives: FAITHFUL-DERIVATION.**
Directly stated in PLAN §II.0 ("`token_text` re-derived via the codec … every
other stored attribute (notation, mass) carries over unchanged") and grounded in
NOTES MOVE_RECORD "topology invariant / identity-only / bookkeeping only":
`token_text` is the sole attribute derived from the address, so it must re-derive;
everything else is address-independent and invariant across a move.

---

## Verdict

**PASS** — spec-faithful, both test suites green against live Postgres and
meaningful. The one item warranting Patrick's attention (D2, DELETE_CONNECTION
axis scope) is a later-op-layer seam, not a defect of the schema/door module,
which is complete, self-consistent, and ready.

Decision rulings: **d1=FAITHFUL, d2=NEEDS-PATRICK, d3=CODE-STRUCTURE, d4=FAITHFUL.**

---

## Follow-up review — `add_membership` idempotency change (2026-09-17)

build-1-rebase made `add_membership` idempotent (`ON CONFLICT DO NOTHING` on
BOTH inserts, same one transaction), which also heals a one-sided membership row
by filling only the missing side; the old "PK-collision forces partial rollback"
advtest was replaced by idempotent-re-add + one-sided-healing tests. Diff
confined to `controller/` (git: only `controller.{h,cpp,test,advtest}.cpp` +
`README.md`).

**Rebuilt + reran both suites, live Postgres 16, no warnings:**
`PASS controller_test` (58 checks, exit 0), `PASS controller_advtest`
(44 checks, exit 0).

1. **Idempotency correct.** Re-adding an existing pair is a clean no-op: base
   test asserts no throw + `member_of` stays 1 row + `members` unchanged (2, no
   dup); advtest asserts `count(*)==1` on each side after re-add. Meaningful
   (row-count assertions, not vacuous).

2. **Dropped-coverage — NO GAP.** The removed test targeted a PK-collision
   partial rollback, genuinely moot now (a PK conflict no longer throws under
   `ON CONFLICT DO NOTHING`). The still-relevant **FK-violation rollback path is
   retained** (advtest §7 first block: `add_membership(member, missing_group)` →
   throws, `member_of` count 0, `members` count 0 — both sides rolled back). FK
   violations are NOT suppressed by `ON CONFLICT DO NOTHING`, so this path is
   real and exercised. No coverage lost.

3. **One-sided healing — FAITHFUL-DERIVATION.** Idempotency is *required* by the
   arraying/SEE contract (PLAN I.F: "Decompression is exact via frame broadcast +
   SEE idempotency" — an arrayed ADD_CONNECTION broadcasting a shared group
   re-adds the group per member) and mirrors `mint`'s SEE-idempotent
   ensure-plus-link. Per-statement `ON CONFLICT DO NOTHING` is the minimal
   realization; one-sided healing is a zero-cost consequence, adding no extra
   path/scan/search. It *restores* the spec's stated invariant — NOTES firmed
   section: "Membership stores both directions (write one side, the reciprocal is
   maintained)" — and aligns with the deferred-reciprocal, eventually-consistent
   runtime (NOTES Process runtime: "the members/member_of reciprocal being
   eventually-consistent while deferred work drains"), where a transiently
   one-sided row is an anticipated state a later reciprocal write must converge
   without a duplicate-key error. Not invented complication.

4. **No regression.** All mint / rekey / delete_token / delete_pair / reads /
   schema-shape checks remain green; scope confined to `controller/`.

**Follow-up verdict: PASS** — idempotency change clean and ready; healing =
FAITHFUL; no dropped-coverage finding.
