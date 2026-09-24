# Adversarial review 4 — PLAN.md (post-fix second pass)

**Reviewer standing.** Fresh, independent adversary. No stake in the plan
passing. Second-pass review verifying the fixes applied after round 3
(`review/plan-review-3-2026-09-17.md`, PASS-WITH-FIXES, findings F1–F9). Complete
review; no summarization.

**Target reviewed:** `/opt/project/repo/engine/db_kernel/PLAN.md` (read in full).
**Governing spec:** `NOTES.md`, authoritative section *"Relationship model &
type — firmed 2026-09-17 (governs on conflict)"* (read end to end).
**Ground-truth re-verified:** `controller/controller.h`, `controller/README.md`,
`schema/schema.sql` (for the fix-touched claims F1/F7).

---

## A. Round-3 finding verification (F1–F9)

Legend: FIXED-CORRECTLY / FIXED-BUT-IMPERFECT / NOT-FIXED / REGRESSED.

| ID | Round-3 ask | Current PLAN.md location + quoted text | Verdict |
|---|---|---|---|
| **F1** | §II.0: state `mint` drops its `type` param and stops writing `token.type` | §II.0 mint bullet (l.239-244): *"`mint` — structure only … **drops its `type` parameter and stops writing `token.type`** (the column is gone; `mint` no longer records a kind); `Constituent.mass` → `std::optional<int>` and LINK writes SQL NULL for blank mass (not `0` …)."* | **FIXED-CORRECTLY** |
| **F2** | Surface grouping-node no-own-address / naming-literal handle in the face grammar | New §I.B paragraph *"Grouping-node identity (no own address)."* (l.125-132): *"A pure-grouping node (`MEMBERS` present, `PARENTS` absent) carries **no independent `ADDRESS` span** … Its handle is its **naming-literal token_id** (crude stage: the `NOTATION` prose construction-post, matured later by the deferred prose→token_id swap). That naming literal must be a live or inline-nested `token` row, because `add_membership` FKs both endpoints to real tokens."* | **FIXED-CORRECTLY** (one phrasing observation N1, non-blocking) |
| **F3** | II.1: qualify co-index equal-length to admit NOTATION blanks + sparse MEMBER_OF §2 | §II.1 (l.260-266): *"**Co-index** rule: equal-length applies to the member-*differentiator* arrays … it does NOT apply to the two intra-field forms — `NOTATION` is length-N with legal blank slots (successive commas), and `MEMBER_OF` section 2 is a *sparse* (≤N) per-member array. Neither is a co-index violation."* | **FIXED-CORRECTLY** |
| **F4** | Reduce/annotate over-declared Agent 5→3 dep | Part III table, Agent 5 (l.352): *"1, 2 (Agent 3 for test fixtures only — the UPDATE cores build on the II.0 door + II.2 span grammar, not on the DECLARE core)"* | **FIXED-CORRECTLY** |
| **F5** | G7 quote not in NOTES — cite NOTES or mark out-of-band | §II.5 G7 note (l.310-312): *"inherently swarm/multi-instance infra not present (`NOTES.md` UPDATE — DELETE ops); it is deferred to the swarm layer (Patrick, out-of-band: "verification added to swarm factors later")."* — deferral now cited to NOTES; quote marked out-of-band | **FIXED-CORRECTLY** |
| **F6** | I.G: state grouping-node DECLARE return | §I.G (l.207-208): *"A grouping-only node returns its **naming-literal token_id** (the borrowed handle), not a freshly placed address."* | **FIXED-CORRECTLY** |
| **F7** | II.0: `rekey` re-derives `token_text` | §II.0 rekey (l.249-252): *"`rekey(old_id, new_id)` re-keys `token`, **re-derives `token.token_text` via the codec** (the dot-join follows `token_id`, as `mint` does), and cascade-repoints every referencing row …"* | **FIXED-CORRECTLY** |
| **F8** | Part III: annotate where II.6 is enforced | Part III (l.358-360): *"**II.6 (synchronous reciprocal) is not a separate agent** — it is enforced by `add_membership` writing both directions (Agent 1) and by the DECLARE/UPDATE cores calling it synchronously (Agents 3, 5)."* | **FIXED-CORRECTLY** |
| **F9** | §II.0: state new reads return deterministic order | §II.0 door reads (l.235-237): *"Both new reads return a **deterministic (sorted) order**, matching `children_of`, so READ linearization is stable."* | **FIXED-CORRECTLY** |

**All nine round-3 findings are fixed correctly.** No fix was skipped, botched, or
regressed. The three MINOR must-fixes (F1/F2/F3) and the six OBSERVATIONS
(F4–F9) are each addressed at the exact location round 3 named, with the substance
round 3 asked for.

---

## B. Ground-truth re-verification of the fix-touched claims

- **F1 (drop is a genuine change).** The current `mint` signature carries
  `const std::string &type = ""` (`controller.h:131`) and writes `token.type`
  verbatim: header comment *"`type` and `mass` are optional pass-through
  attributes stored verbatim: an empty `type` string and an absent `mass` are
  each written as SQL NULL"* (`controller.h:118-120`). The `token.type` column
  exists (`schema.sql:77`). ⇒ Dropping the `type` param + the write is a **real
  change**, not a description of already-absent behaviour. ✓
- **F7 (`token_text` is the codec dot-join at mint).** *"`token_text` is the
  codec's dot-joined rendering of `token_id` (derived here, not passed in)"*
  (`controller.h:114-116`); *"MINT — insert the `token` row. `token_text` is the
  codec's dot-joined rendering of `token_id`, derived by the controller (not a
  parameter, so it cannot drift from the identity)"* (`controller/README.md:59-61`);
  column is loader-populated text (`schema.sql:59-62`). ⇒ A re-key that changes
  `token_id` must re-derive `token_text` or it drifts. The F7 fix is accurate. ✓
- **Round-3 §A rebase claims still genuine (spot-check).** `token.type`
  (`schema.sql:77`), `token_sibling_group` (`schema.sql:157`),
  `add_group_membership(...)` (`controller.h:138`), `groups_of` (`controller.h:101`),
  `token_parent.mass NOT NULL` (`schema.sql:131`), and `Constituent.mass = int`
  non-optional (`controller.h:44-45`) all remain present/unchanged — so every
  §II.0 "drop/add/change" is still a genuine delta against the live tree. ✓

---

## C. New-defect hunt (defects introduced BY the fixes)

I checked each edit for a new contradiction, imprecision, scope creep, broken
cross-reference, or delegation cycle.

- **F1 edit vs the rest.** Consistent with F7 ("`mint` … re-derives token_text …
  as `mint` does") and with §II.0 `attributes_of` "drops `type`". No column write
  survives anywhere. **Clean.**
- **F3 edit vs I.F.** I.F (l.197-198) states the general co-index equal-length
  rule; II.1 now carves out NOTATION (length-N, blanks) and MEMBER_OF §2 (sparse
  ≤N). I.B (l.111-117) already described both forms, so I.F/I.B/II.1 are now
  mutually consistent. **Clean.**
- **F4 edit vs the ordering prose and II.5.** The relaxed edge keeps
  1,2,3,4,5,6 valid (3 still precedes 5) and matches II.5 (l.299-306: UPDATE
  builds on `rekey`/`delete_*`/`add_membership` + II.2 span, none on the DECLARE
  core). No cycle introduced; F8's "Agents 3, 5" annotation is unaffected.
  **Clean.**
- **F6 vs F2.** I.G's grouping return ("naming-literal token_id") is consistent
  with I.B's grouping-node handle (also a token_id) and with II.3. No conflict.
  **Clean.**
- **F2 scope.** The new paragraph surfaces an in-scope face-grammar rule (task
  scope point 1) and explicitly defers the prose→token_id maturation
  ("matured later by the deferred prose→token_id swap"), so it does **not**
  resolve a deferred model. **In scope.**
- **Cross-references.** All G-seam references (G4/G5 in I.E/II.2; G6 in I.H; G7/G10
  in II.5) resolve to Part V, which lists G4/G5/G6/G7/G10. `add_membership`,
  `rekey`, `delete_token`/`delete_pair` referenced by the fixes are all defined in
  §II.0. No dangling reference introduced. **Clean.**

One non-blocking observation from the F2 edit:

### N1 — OBSERVATION — F2 conflates the crude prose construction-post with a token_id
- **Location:** §I.B, *"Grouping-node identity (no own address)"* (l.127-131):
  *"Its handle is its **naming-literal token_id** (crude stage: the `NOTATION`
  prose construction-post …). That naming literal must be a live or inline-nested
  `token` row, because `add_membership` FKs both endpoints to real tokens."*
- **Analysis:** NOTES describes the crude-stage handle in two registers — as a
  *"temporary `notation` prose string"* that "stands in for" the naming literal
  (`NOTES.md:527`, `:549-550`), and as a *"placeholder identity you build the real
  structure against, then pull"* (`NOTES.md:531-532`). The plan takes the second
  register (identity = a real token) and hardens it via the FK: since
  `members`/`member_of` mirror `token_parent`/`token_child` and thus FK to
  `token`, a group referenced by a bare prose *string* could not be stored, so the
  handle must be a live token. That inference is **correct and internally
  consistent** (I.B, I.G and II.3 all treat the handle as a token_id). The only
  imprecision is that the phrase "the `NOTATION` prose construction-post" reads as
  if the prose string itself is the token_id, whereas what must exist is a *token*
  bearing that crude prose as its `notation`. This is a wording nuance, not a
  mechanism error; the FK sentence immediately disambiguates it.
- **Why non-blocking:** the resolution is FK-driven (no invented mechanism), the
  maturation is explicitly deferred, and the three grouping-node sites agree.
  Nothing an implementing agent builds turns on the phrasing.
- **Optional correction (not required):** reword to "its handle is a naming-literal
  **token** (crude stage: a construction-post token bearing the `NOTATION` prose)".

No other new defect found. No BLOCKER/MAJOR/MINOR was introduced by the fixes.

---

## D. Independent OLD-MODEL-LEAK re-check (on the fixed plan)

Re-run against the current text, including all added paragraphs:

- **TYPE-gating / declared TYPE** — **CLEAN.** F1's edit removes the last write
  path (mint's `type` param); I.B "No `TYPE` field, no type gating", II.1 "no
  TYPE", II.3 "No TYPE", §II.0 `attributes_of` "drops `type`". The F2 paragraph
  adds no type notion.
- **sibling-group / SIBLINGS** — **CLEAN.** Present only as the dropped table
  (§1, §II.0). No added paragraph reintroduces it.
- **token_child membership front-door** — **CLEAN.** §1 "`token_child` reverts to
  pure structure-reverse"; membership rides `members`/`member_of` only; F2/F6 key
  the group by naming-literal token_id via membership edges, not via `token_child`.
- **shared-table endpoint-type lens** — **CLEAN.** II.4 "no type-lens over a
  shared table … separate stores" unchanged.
- **label-CHILDREN-as-field** — **CLEAN.** I.B "`MEMBERS` (replaces the old label
  `CHILDREN`)"; F2/F6 use "MEMBERS"/"naming-literal", never "CHILDREN".

**OLD-MODEL-LEAK verdict: CLEAN** on all five forbidden constructs.

---

## E. Scope / open-seam regression check

- **No out-of-scope design added.** F2 is in-scope face grammar; it defers the
  prose→token_id swap. No fix designed a cache-tier core or the async
  file-now/wire-later split (still §0 / II.6 deferred, synchronous writes).
- **No deferred model resolved.** Mass aggregation ("unresolved — not settled
  here", I.B), notation derivation ("reads blank until derivation lands", II.3),
  prose→token_id (F2 + §V), extrapolation (inert seam, §0) — all still open.
- **Open seams still correctly open.** G4 (analyst-supplied-start meanwhile), G5
  (trunk map unfixed), G6 (transport parked behind IR), G7 (local gate now, swarm
  transport deferred — F5 keeps this), G10 (FK-dependent DELETE policy flagged,
  not resolved). None was silently closed by a fix.
- **Delegation still acyclic.** 1,2 independent; 3←{1,2}; 4←{1,2}; 5←{1,2} (+3
  test-fixtures-only per F4); 6←{3,4,5}. Order 1,2,3,4,5,6 valid; no cycle. F4's
  edit *weakened* an edge (over-declaration removed), which cannot create a cycle.

---

## F. Overall verdict

**PASS — ready for implementation; coding agents may be dispatched.**

All nine round-3 findings (F1–F9) are FIXED-CORRECTLY at the locations named, with
the substance requested. The two fix-touched ground-truth claims (F1 `mint` still
carries/writes `type`; F7 `token_text` is the codec dot-join at mint) are
re-verified genuine against the live tree, and the rest of the §II.0 rebase deltas
remain real. The OLD-MODEL-LEAK re-check is **clean** on all five forbidden
constructs. Scope discipline holds — no out-of-scope design added, no deferred
model resolved, all open seams (G4/G5/G6/G7/G10) still correctly open — and the
delegation graph is acyclic with Agent 1 the foundation. The fixes introduced no
new BLOCKER/MAJOR/MINOR; the single new item (N1) is a non-blocking phrasing
observation whose underlying resolution is correct and internally consistent.

No must-fix remains.
