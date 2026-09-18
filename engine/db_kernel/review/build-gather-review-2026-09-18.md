# Adversarial review — `gather` primitive (controller door)

**Reviewer:** fresh independent adversary. No stake in the outcome.
**Date:** 2026-09-18
**Target:** `gather` (two overloads) in `engine/db_kernel/controller/` —
`controller.h`, `controller.cpp`, `controller_test.cpp`. Working tree,
branch `dbkernel-design-checkpoint`, not modified by this review.
**Governing spec:** `NOTES.md` "Build-phase rulings — firmed 2026-09-18"
(Gather primitive; Address column collation; Hard constraints); `PLAN.md`
§II.0 gather bullet; `codec/codec.h`; `schema/schema.sql`.

---

## Method (independent rebuild + run + EXPLAIN)

Everything below was rebuilt and run from scratch by the reviewer; nothing
is taken on the implementer's word.

- **Build.** Recompiled `controller_test` and `controller_advtest` from
  `controller.cpp` + the codec + libpq, per `README.md` "Build and run".
  Both compiled clean with `-Wall -Wextra` (no warnings).
- **Live DB.** Ran both against a real local Postgres 16.15 cluster.
  `hcp3_core` collation is `en_US.UTF-8` / `en_US.UTF-8`
  (case-interleaved) — i.e. **the genuine adversarial condition** the
  `COLLATE "C"` pin exists to defend against, not a C-locale cluster that
  would pass trivially. `token.token_id` column collation confirmed `C`;
  PK index `token_pkey` is a plain btree on `token_id`.
- **EXPLAIN.** Ran the exact predicate shapes `gather` emits, both as-is
  and with `enable_seqscan=off`, and before/after `ANALYZE`.
- **Branch coverage.** The committed test only exercises the `first+1`
  bound branch (trunk E). I wrote a separate harness driving the two
  **untested** branches — the alphabet-max carry and the open-ended
  last-trunk case — through the real `gather`/`wildcard_bounds` code path.

### Verbatim test results

```
===== controller_test =====
... (72 checks) ...
ok   gather(prefix): trunk E returns exactly its 4 members, in byte order (EA, EC.AA, EZ, Eb — EZ before Eb, deeper address included)
ok   gather(prefix): a sibling trunk (F) is excluded from trunk E's gather
ok   gather(prefix): a trunk with no members returns empty
ok   gather(range): [EA, Eb] inclusive returns the same 4 members in the same PK order as the prefix form
ok   gather(range): [EB, ED] picks up only the deeper EC.AA address
ok   gather(range): a single-point [EA, EA] range is inclusive of EA itself
ok   gather(range): an empty region returns empty
ok   gather precondition: an inline (non-terminal) wildcard does not even decode as a valid address (codec-level guarantee)
ok   gather(range): a partial/wildcard endpoint is rejected
ok   gather(prefix): a fully-specified (non-wildcard) address is rejected
PASS controller_test        (rc=0)

===== controller_advtest =====
... (45 checks) ...
PASS controller_advtest     (rc=0)
```

`controller_advtest` (no gather-specific checks, but the regression guard)
is **green** — no regression.

### Independent branch-coverage harness (untested branches)

```
ok   gather(z*): open-ended last-trunk gathers zA,zZ,zb,zz (byte order, no upper bound)
ok   gather(AA.z*): carry sets low={AA.zA}, high={AB} excl; returns AA.zA,AA.zz; excludes AA and AB
ok   gather(zz.z*): leading exhausted -> open-ended; returns zz.zA, excludes bare zz
PASS adv_gather
```

All three previously-uncovered branches are **correct** through the real
code path.

### Independent EXPLAIN

Bounded prefix `E*` → `token_id >= '{EA}' AND token_id < '{FA}'`, **as-is**
on the tiny un-analyzed table:

```
 Bitmap Heap Scan on token
   Recheck Cond: ((token_id >= '{EA}'::text[]) AND (token_id < '{FA}'::text[]))
   ->  Bitmap Index Scan on token_pkey
         Index Cond: ((token_id >= '{EA}'::text[]) AND (token_id < '{FA}'::text[]))
```

Inclusive range `[EA,Eb]` → same shape (genuine **`Index Cond` on
`token_pkey`**, Bitmap Index Scan). Direct result of the bounded query
returns `EA, EC.AA, EZ, Eb` — deeper `EC.AA` included, `EZ` before `Eb`,
`FA` excluded — confirming byte-order correctness under `COLLATE "C"`.

Half-open `>= '{EA}'` (open last-trunk case), **as-is**:

```
 Seq Scan on token
   Filter: (token_id >= '{EA}'::text[])   (rows=210 default estimate)
```

Same, with `enable_seqscan=off`: **Bitmap Index Scan on `token_pkey`,
Index Cond `(token_id >= '{EA}'::text[])`** — i.e. the index path IS
available; the planner declines it purely on cost. After `ANALYZE` (5-row
table) it still seq-scans (whole table = one page); result still correct.

---

## Findings

### F1 — Bound derivation is correct across ALL branches — PASS (evidence)
**Severity:** none (confirmation). **Files:** `controller.cpp:67-130`.
- (a) *Subtree inclusion.* Exclusive `high` = smallest full address of the
  next trunk at the wildcard position. Array comparison orders a shared
  prefix's longer extension strictly after the divergence point, so every
  deeper address under the trunk sorts `< high` (its first element decides:
  `E?` < `FA`). Verified: `E*` gathers `EC.AA`. ✓
- (b) *Alphabet-max carry.* When the wildcard first char is `z` (max) with
  leading elements, `increment_full_address` carries the leading prefix as
  a base-50, second-char-fastest counter. Verified live: `gather("AA.z*")`
  → low `{AA,zA}`, high `{AB}` exclusive; returns `AA.zA, AA.zz`; correctly
  excludes bare `AA` (shorter array sorts before low) and `AB` (== exclusive
  high). ✓
- (c) *Fully-open last-trunk.* `z*` (no leading) and `zz.z*` (leading also
  exhausted) → `high` nullopt → half-open `>=` scan. Verified live: both
  return exactly their members, no false upper truncation. ✓
- Helpers match `codec::kAlphabet` (`kAlphabetSize` = 50, index-based
  successor); no off-by-one in the `first + 1 < kAlphabetSize` guard.

### F2 — Byte-order correctness under `COLLATE "C"` — PASS (evidence)
**Severity:** none (confirmation). This is the pre-collation-fix failure
mode, tested on the exact hostile locale. `EZ` (upper-couplet) and `Eb`
(lower-couplet) both returned, in byte order `EZ` < `Eb` (0x5A < 0x62),
neither dropped; sibling trunk `F` excluded; non-existent prefix `G*`
returns empty. Column collation confirmed `C`. ✓

### F3 — Range form correct — PASS (evidence)
**Severity:** none. `[from,to]` inclusive both ends (`>=`/`<=`); single-point
`[EA,EA]` returns `EA`; empty region returns empty; `from > to` documented
and behaves as match-nothing (not an error). ✓

### F4 — Rejection cases correct — PASS
**Severity:** none. `controller.cpp:286-289` — prefix form rejects
non-partial / empty / invalid (`is_valid_address` also catches an inline
partial, which the codec cannot even decode). `controller.cpp:321-337` —
range form's `is_full` lambda rejects empty, invalid, or any partial
endpoint. Both verified live (throw on `gather(e_a)` and
`gather(e_star, e_z)`). ✓

### F5 — only-follow honoured — PASS
**Severity:** none. Both overloads read `token` **only**
(`controller.cpp:297-307`, `341-345`); a single bounded `token_id` PK-range
predicate; no reverse-search, no secondary index created, no predicate on a
non-key column. EXPLAIN shows the access path is the existing `token_pkey`.
Parameters are bound (`$1`/`$2`), so no SQL injection surface; the array
literal is built from alphabet chars + `*` only. ✓

### F6 — Open-ended (half-open) last-trunk plan — RULED **ACCEPTABLE**
**Severity:** informational (not a defect). **Ruling:** the half-open
`>=`-only case degrading to Seq Scan on the tiny, un-analyzed test table is
an **acceptable planner cost artifact, NOT a defect requiring a fix.**
Rationale, all evidenced above:
1. It is a cost-based physical choice, **correct either way** — results are
   identical and correctly byte-ordered under both Seq Scan and the forced
   index path.
2. The `Index Cond` on `token_pkey` **is available** (proven with
   `enable_seqscan=off`); nothing is missing — no absent index, no query
   rewrite needed.
3. It arises **only** for the literal last trunk of the entire address space
   (when `wildcard_bounds` legitimately returns `high = nullopt`: top-level
   `z*`, or a fully-exhausted leading prefix like `zz.z*`). Every normal
   trunk yields a two-sided bounded query, which rides the `Index Cond`
   **even on the tiny table as-is** (F-EXPLAIN above).
4. It is not a forbidden "search": a planner-chosen Seq Scan over a bounded
   PK-range predicate introduces no reverse-search index and no non-key
   predicate. The only-follow guarantee is a property of the *access-path
   design* (PK range only, no reverse index), which holds; the planner's
   bitmap-vs-seqscan pick is orthogonal to it. At production scale on
   analyzed tables a genuinely huge half-open range may still legitimately
   seq-scan — that is the planner doing its job, not a design breach.

### F7 — Test coverage gap on the trickiest branches — **MUST-FIX (tests)**
**Severity:** medium (correctness is fine, but coverage is not).
**File:** `controller_test.cpp:366-446`.
The committed gather tests exercise **only** the `first + 1` bound branch
(trunk `E`, and ranges within it). The two most error-prone branches ship
with **zero committed coverage**:
- the alphabet-max **carry** (`increment_full_address`, base-50
  second-char-fastest, leading-element carry), and
- the **open-ended** last-trunk case (`high == nullopt`, half-open scan).

I verified both are correct (F1b/F1c) via my own harness, so this is a
coverage defect, not a behaviour defect — but under the project's standing
"tests on everything / interrogate completeness" discipline a primitive
whose hardest logic is untested is not complete.
**Fix:** fold carry + open-ended cases into `controller_test.cpp` (e.g.
`gather("z*")`, `gather("AA.z*")` asserting bare-`AA`/`AB` exclusion, and a
`zz.z*` open-ended case). The reviewer's `adv_gather.cpp` harness content is
a ready starting point.

### F8 — `controller/README.md` does not document `gather` — **MUST-FIX (docs)**
**Severity:** low (documentation / module completeness).
**File:** `controller/README.md` (Interface → Read side table, lines
42-49). Every other door method (`token_exists`, `parents_of`,
`children_of`, `members_of`, `member_of`, `attributes_of`) has an
Interface-table row; the mutation primitives get their own subsection.
`gather` — a new **public** door read primitive — is entirely absent. The
door README's stated job is to document the door's interface; shipping a
public method undocumented breaks that contract. It is cheap to fix (one or
two table rows / a short paragraph noting: two overloads, terminal-wildcard
prefix vs inclusive `FROM..TO` range, PK-range/only-follow, byte order,
existing-tokens-only). Classifying **must-fix** for module completeness,
though it is doc-only and carries no correctness risk.

### F9 — Scope confined; no regression — PASS
**Severity:** none. `git diff HEAD` on `controller/` is **purely additive**
(258 insertions, 0 deletions) across exactly the three controller files;
no other files touched, nothing smuggled, no existing behaviour altered.
The `COLLATE "C"` schema pin the primitive depends on is a **prior** commit
(`5d4609a`), correctly out of this change's scope. `controller_advtest`
green. Tests assert **exact sets and order** (`e_gathered == e_expected`
with explicit vectors), not mere non-emptiness. ✓

---

## Verdict

**PASS-WITH-FIXES.**

The `gather` primitive is **correct** — bound derivation (all branches,
including the two the committed suite does not cover), byte-order under
`COLLATE "C"` on the genuinely hostile locale, inclusive/point/empty
ranges, all rejection cases, only-follow, and a genuine `Index Cond` on
`token_pkey` for bounded ranges are all independently verified. No
correctness defect found. Scope is clean, no regression.

**Must-fix before this is called complete (neither is a correctness bug):**
- **F7** — add committed tests for the carry and open-ended bound branches
  (the primitive's hardest logic currently ships untested).
- **F8** — document `gather` in `controller/README.md` (module completeness;
  every other door method is documented).

**Ruled acceptable (no fix):**
- **F6** — the half-open last-trunk Seq Scan is a planner cost artifact,
  correct and index-capable, arising only for the top of the address space;
  not a defect.
