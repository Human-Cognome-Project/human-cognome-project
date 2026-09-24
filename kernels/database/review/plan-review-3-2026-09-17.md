# Adversarial review — PLAN.md (API face + record-tier core data functions)

**Reviewer standing.** Fresh, independent adversary. No stake in the plan
passing. Complete review; no summarization.

**Target reviewed:** `/opt/project/repo/engine/db_kernel/PLAN.md` (read in full).
**Governing spec:** `NOTES.md`, authoritative section *"Relationship model &
type — firmed 2026-09-17 (governs on conflict)"* (read end to end).
**Ground-truth verified against:** `schema/schema.sql`, `schema/README.md`,
`controller/controller.h`, `controller/README.md`, `codec/README.md`,
`ingestion/README.md`, plus direct `grep` of the live door/schema symbols.

---

## A. Ground-truth verification of the §II.0 rebase claims

Every door/schema addition the plan attributes to Agent 1 was checked against the
actual code. All are genuine additions or genuine changes — none silently assume
a primitive that already exists, and the plan's description of the *current*
door/schema is accurate.

| Plan §II.0 claim | Current code (verified) | Verdict |
|---|---|---|
| Drop `token_sibling_group` | `CREATE TABLE token_sibling_group` present (`schema.sql:157`) | Genuine drop ✓ |
| Drop `token.type` column | `type text` present (`schema.sql:77`) | Genuine drop ✓ |
| Add `members` / `member_of` tables | Absent (only 4 tables: token/token_parent/token_sibling_group/token_child) | Genuine addition ✓ |
| `token_parent.mass` → nullable | `mass integer NOT NULL` (`schema.sql:131`) | Genuine change ✓ |
| `token.mass` retained, nullable | `mass integer` nullable, no default (`schema.sql:87`) | Accurate; unchanged ✓ |
| Replace `groups_of` with `members_of(id)` + `member_of(id)` | Only `groups_of` exists (`controller.h:101`); no `members_of`/`member_of` | Genuine addition ✓ |
| `attributes_of` drops `type`, keeps `mass` | `TokenAttributes{notation,type,mass}` (`controller.h:69-73`) | Genuine change ✓ |
| `add_membership(member,group)` writing both directions, replacing `add_group_membership` | Only `add_group_membership(member,group,mass)` (`controller.h:138`) | Genuine addition ✓ |
| `mint` structure-only; `Constituent.mass` → `std::optional<int>`; LINK writes NULL | `Constituent.mass = int` non-optional (`controller.h:44`); mint LINK writes per-element mass | Genuine change ✓ |
| `rekey(old,new)` cascade | Absent | Genuine addition ✓ |
| `delete_token(id)` / `delete_pair(a,b)` | Absent | Genuine addition ✓ |
| "schema has no ON UPDATE CASCADE and forbids triggers" | Plain `REFERENCES` FKs, no cascade; header: "not to triggers or functions in this schema" (`schema.sql:12-13`) | Accurate ✓ |

Baseline descriptions in §1 (controller reads `token_exists`/`parents_of`/
`children_of`/`groups_of`/`attributes_of`; writes `mint`/`add_group_membership`;
4-table schema; DECLARE/READ functional, UPDATE/cache stubs; `db_runtime` one
request per line; current DECLARE carries `--type`/`--mass`) are all accurate
against `controller.h`, `schema.sql`, and `ingestion/README.md`.

**Ground-truth conclusion: clean.** No fabricated primitive; every flagged
addition is real; the current-state description is faithful.

---

## B. OLD-MODEL-LEAK check (explicit)

Searched the plan for each forbidden old-model construct:

- **TYPE-gating / declared TYPE field / type-lens gating** — **CLEAN.** I.B: "No
  `TYPE` field, no type gating. Validation is **structural**." II.1: "no TYPE".
  II.3: "No TYPE — structural validation." §II.0: `attributes_of` drops `type`.
- **Sibling-groups (`token_sibling_group` / `SIBLINGS`)** — **CLEAN.** Referenced
  only as the table being dropped (§1, §II.0). No `SIBLINGS` field anywhere.
- **`token_child` membership front-door** — **CLEAN.** §1: "`token_child` reverts
  to pure structure-reverse. Neither [structure table] carries membership any
  longer." Membership rides `members`/`member_of` only.
- **Shared overloaded table with an endpoint-type lens** — **CLEAN.** II.4: "no
  type-lens over a shared table … structure and membership are separate stores."
- **Label `CHILDREN` as a declared field** — **CLEAN.** I.B: `MEMBERS`
  "(replaces the old label `CHILDREN`)." No `CHILDREN` field in the grammar.

**OLD-MODEL-LEAK verdict: CLEAN.** No leak of any of the five forbidden
constructs.

---

## C. Completeness walk (per firmed record-tier precept)

Legend: COVERED / PARTIAL / MISSING.

**Literal intake**
- PARENTS sets N — COVERED (I.B).
- `>=2` anti-alias floor — COVERED (I.B).
- ADDRESS span alphabet (FROM/AFTER/TO/direct/@-pin/undeclared-hook) — COVERED (I.E).
- cover-N validity; elastic-last-only; interior self-delimiting; nested-as-one-slot — COVERED (I.E).
- NOTATION positional-partial, blanks legal — COVERED (I.B).
- MEMBER_OF two-section `|` split (broadcast + sparse per-member) — COVERED (I.B).

**Grouping / label dual**
- MEMBERS floor `>=1` — COVERED (I.B).
- label has no own address — **PARTIAL** (stated in II.3 core; not surfaced in the
  face grammar I.B/I.E — see F2).
- naming-literal identity — **PARTIAL** (II.3 names "naming-literal token_id"; the
  face grammar does not say how a grouping node supplies that handle — see F2).

**READ**
- anchor — COVERED (I.C).
- direction radial-default — COVERED (I.C).
- depth / LoD (governs members-in-scope) — COVERED (I.C, II.4).
- named-branch exclusions (token_ids only, no predicates) — COVERED (I.C).
- linearized once-per-path, no dedup — COVERED (I.C, II.4).
- per-axis reads (structure vs membership as distinct follows) — COVERED (II.4).
- reverse as orientation, not a peer value — COVERED (I.C).

**UPDATE — four ops**
- MOVE: bookkeeping / topology-invariant / re-key+cascade / ranged-arrayable — COVERED (I.D, II.5).
- ADD_CONNECTION: group-leads / pairwise / no-kind-operand / endpoints-pre-exist / reciprocal — COVERED (I.D, II.5).
- DELETE_RECORD & DELETE_CONNECTION: single / never-arrayed / confirm+peer-validate / specific-no-wildcard / reciprocal-removal — COVERED (I.D, II.5).

**Arraying**
- serial ordered (not unordered batch) — COVERED (I.F).
- additive arrayable (DECLARE/READ/MOVE/ADD_CONNECTION) — COVERED (I.F).
- destructive not arrayable — COVERED (I.F).
- cache tier excluded — COVERED (I.F).

**Schema/door rebase** — COVERED except mint's `type` parameter removal, which is
implied by dropping the column but not stated (F1).

**Cross-cutting** — pairwise substrate, only-follow/never-search,
address-IS-identity/no-forwarding/no-equivalence-table, no-invented-complication —
COVERED (Part IV map + inline references).

---

## D. Itemized findings

No BLOCKER or MAJOR findings. Three MINOR precision gaps (each has its substance
present elsewhere in the plan but is under-surfaced where an implementing agent
will look), and six OBSERVATIONS.

### F1 — MINOR — `mint`'s `type` parameter removal not stated
- **Location:** §II.0, "Door writes → `mint` — structure only (see-mint-link-wire
  into `token_parent` / `token_child`); `Constituent.mass` → `std::optional<int>`
  …".
- **Precept / ground-truth:** the firmed section drops `token.type`
  (`NOTES.md:146` "The declared `TYPE` field and the `token.type` column are both
  dropped"). The current `mint` signature carries `const std::string &type = ""`
  and writes `token.type` (`controller.h:129-132`, README step MINT).
- **Defect:** §II.0 enumerates the mint changes (`Constituent.mass` optional, NULL
  for blank) but omits that `mint` must drop its `type` parameter and stop writing
  `token.type` once the column is gone. `attributes_of` "drops type" is stated for
  the read side but not the write side. An agent could leave the dead `type`
  param, writing to a dropped column.
- **Narrowest correction:** add to §II.0 mint bullet: "and drops its `type`
  parameter (the `token.type` column is gone; `mint` no longer writes a kind)."

### F2 — MINOR — grouping-node identity/handle absent from the face grammar
- **Location:** §I.B (DECLARE grammar) and §I.E (ADDRESS span) present `ADDRESS`
  as a field applicable to a declare; the label case is treated only in §II.3
  ("label-has-no-own-address … referencing the group by its naming-literal
  token_id").
- **Precept:** `NOTES.md:540-556` — "a label is not given an address of its own …
  label intake carries **no independent ADDRESS placement** like the literal set
  does: what gets placed is the naming literal (via the literal path); the label
  is realized as membership edges keyed on the members, referencing the group by
  the naming-literal token_id." The completeness item "grouping/label dual …
  label has no own address; naming-literal identity" is in scope (task scope
  point 1: the *face* of the API, full record-tier grammar).
- **Defect:** the face (I.B/I.E) implies `ADDRESS` is a uniform field on every
  DECLARE, so a reader cannot tell from the grammar how a pure-grouping node
  (MEMBERS present, PARENTS absent) is identified/placed — that it carries **no**
  own ADDRESS span and its handle is the naming-literal token_id (crude stage:
  the NOTATION prose construction-post, matured later by the deferred prose→
  token_id swap). Because `add_membership` FKs both endpoints to live `token`
  rows, the grouping DECLARE also *depends on* the naming-literal token existing
  (referenced or inline-nested); this dependency is not flagged. The semantics
  live in II.3, but the grammar face — the thing Agent 3 implements against — is
  silent.
- **Narrowest correction:** in I.B state that a grouping-only node carries no
  independent `ADDRESS` span (label-has-no-own-address); its identity is the
  naming-literal token_id (crude: NOTATION), which must be a live/nested token
  because membership edges FK both endpoints; note the crude→token_id maturation
  is the deferred prose→token_id swap (already out of scope). No new mechanism —
  just surface the II.3 rule in the face.

### F3 — MINOR — validation "array co-index (equal length)" omits the two spec'd exceptions
- **Location:** §II.1, "array co-index (equal length), span cover-N"; and §I.F
  "co-index (equal length = member count, aligned to the `from` slot)".
- **Precept:** `NOTES.md:496-503` — NOTATION is "positional-partial … blanks
  (successive commas) are legal placeholders" (length-N with legal blanks), and
  MEMBER_OF section 2 is "a **sparse** positional array of additional per-member
  memberships" (length ≤ N). The equal-length co-index rule
  (`NOTES.md:716-720`, In flight) applies to the differentiator arrays, not to
  these two intra-field forms.
- **Defect:** stated flatly as "equal length", II.1 would wrongly reject a legal
  NOTATION-with-blanks or a legal sparse MEMBER_OF section-2. I.B does carry both
  nuances, so the plan is self-consistent overall, but II.1 (the validation
  module spec that Agent 2 implements) is the one place that must encode the
  carve-outs and does not.
- **Narrowest correction:** in II.1, qualify: co-index equal-length applies to the
  member-differentiator arrays; NOTATION is length-N with legal blank slots, and
  MEMBER_OF section 2 is a sparse (≤N) per-member array — neither is a co-index
  violation.

### F4 — OBSERVATION — dependency edge Agent 5 → Agent 3 looks stronger than required
- **Location:** Part III table, Agent 5 (II.5 UPDATE) "Depends on 1, 2, 3".
- **Analysis:** UPDATE's cores draw on `rekey`/`delete_*`/`add_membership`
  (II.0, Agent 1) and the ADDRESS span grammar (II.2, Agent 2). None of the four
  UPDATE ops build on the DECLARE core (II.3, Agent 3). The 5→3 edge is defensible
  only if Agent 5's *tests* stand up fixtures through the DECLARE core rather than
  through the controller door directly. It does not break acyclicity (still
  1,2→3→4→5→6) and is conservative, so it is not a correctness defect — but it is
  an inaccurate/over-declared logical dependency.
- **Suggestion:** either note the dependency is test-fixture-only, or reduce it to
  1, 2 and let Agent 5's tests seed fixtures via the Agent 1 door.

### F5 — OBSERVATION — G7 quotation not traceable to the governing spec
- **Location:** §II.5 G7 note: `("verification added to swarm factors later" —
  Patrick)`; echoed in Part V.
- **Analysis:** that exact phrase does not appear in `NOTES.md`. The *deferral* is
  sound — peer-validation/becomes-systemic-on-peer-confirm is inherently swarm/
  multi-instance (`NOTES.md:625-631`), so leaving the transport out and building
  the local gate now is correct. Only the attributed quote is unverifiable
  against the provided spec (it may come from the graph or a live exchange).
- **Suggestion:** cite the NOTES DELETE-ops text for the deferral, or mark the
  Patrick quote as sourced outside NOTES.

### F6 — OBSERVATION — DECLARE return contract silent on the label/grouping case
- **Location:** §I.G, "`DECLARE_RECORD` → the **actual assigned address(es)**".
- **Analysis:** a grouping node has no own address (F2). The return contract does
  not say what a grouping-only DECLARE returns (the naming-literal's address? a
  membership-write acknowledgement?). Ties to F2.
- **Suggestion:** state that a grouping node's return is its naming-literal
  token_id/address (the borrowed handle), not a freshly placed address.

### F7 — OBSERVATION — `rekey` should also re-derive `token_text`
- **Location:** §II.0, "`rekey(old_id, new_id)` re-keys `token` and
  cascade-repoints every referencing row".
- **Analysis:** `token.token_text` is the codec dot-join of `token_id`
  (`controller.h` mint step 2; `schema.sql:59-62`). A re-key changes `token_id`,
  so `token_text` must be re-derived or it drifts from identity.
- **Suggestion:** note that `rekey` re-derives `token_text` via the codec, as
  `mint` does.

### F8 — OBSERVATION — II.6 (synchronous reciprocal) not assigned in the delegation table
- **Location:** Part III table (rows for II.0–II.5, II.7, II.8; no II.6 row).
- **Analysis:** II.6 is a *property* realized by `add_membership` writing both
  directions (Agent 1) and by the DECLARE/UPDATE cores calling it synchronously
  (Agents 3, 5), so it is genuinely covered — but the delegation map does not say
  so, leaving a reader to infer where the "synchronous reciprocal" precept is
  enforced.
- **Suggestion:** annotate that II.6 is enforced across Agents 1/3/5 (not a
  separate module).

### F9 — OBSERVATION — `members`/`member_of` follow-order determinism unspecified
- **Location:** §1 / §II.0 (new tables "PK-keyed, one-level arrayed follows").
- **Analysis:** READ's once-per-path/no-dedup linearization is deterministic only
  if each follow returns a deterministic order. `children_of` already returns
  "sorted for determinism" (`controller.h:96-97`). The plan does not state that
  `members_of`/`member_of` follow the same determinism convention.
- **Suggestion:** state that the two new reads return a deterministic order
  (sorted), matching `children_of`, so READ linearization is stable.

---

## E. Scope-error check

- **Cache-tier core** — not designed; named stubs only (§0, I.A, II.4
  cache-shaped deferred). ✓ correctly out of scope.
- **Async runtime file-now/wire-later split** — explicitly deferred; the record
  cores write synchronously (§0, II.6). ✓ not resolved.
- **Deferred models** — mass aggregation (sum-vs-centroid "unresolved — not
  settled here", I.B); notation derivation ("reads blank until derivation lands",
  II.3); prose→token_id swap (§0, §V); extrapolation/relative-placement (inert
  seam, §0). ✓ none resolved.
- **No out-of-scope item is designed, and no in-scope item is claimed done but
  missing** — the one write-side omission (F1) is a stated-scope item left
  under-specified, not an out-of-scope intrusion.

## F. Open-seam correctness (nothing silently resolved)

- **G4 next-slot** — correctly open (`NOTES.md:761-764`); planner uses
  analyst-supplied-start, which `NOTES.md:94-98` sanctions ("needs no cursor"). ✓
- **G5 block boundaries** — correctly open (`NOTES.md:765-766`); depth expansion
  leans on the deferred trunk map, nothing hardcoded. ✓
- **G6 transport/framing** — correctly open (`NOTES.md:767-768`); IR + parser
  designed, serialization parked. ✓
- **G7 delete peer-validation** — correctly deferred to swarm infra
  (`NOTES.md:625-631`); local gate + tags built now. (See F5 re: the quote.) ✓
- **G10 DELETE_RECORD FK policy** — genuinely unspecified in NOTES; the plan
  flags it and adopts the minimal FK-respecting interim (refuse delete of a still-
  referenced token) rather than inventing cascade/repoint. This is the
  non-invented default (the FK already forbids it), not a resolution of the
  block/cascade/repoint decision, and the plan says "until decided." ✓
- **No other decision is silently resolved** — checked `0x`=0-vs-undefined
  (written "0/undefined", not settled), nested aggregation (deferred), members
  ordinal/order (left to agent latitude, F9). ✓

## G. Delegation-soundness check

- **Acyclicity:** 1 and 2 independent (baseline); 3←{1,2}; 4←{1,2}; 5←{1,2,3};
  6←{3,4,5}. Topological order 1,2,3,4,5,6 is valid; no cycle. ✓
- **No agent depends on an unbuilt capability:** every core sits on the II.0
  door/schema rebase (Agent 1), which leads; the span grammar/IR (Agent 2) is
  baseline-only; DECLARE/READ/UPDATE consume 1+2; arraying/dispatcher consume the
  cores. The only questionable edge is 5→3 (F4), which is an *over*-declaration,
  not a missing dependency. ✓
- **Agent 1 as foundation:** correct — `members`/`member_of`, mutation/delete
  primitives, and `Constituent.mass` optionality are prerequisites every core
  needs, and all are verified-absent today (§A). ✓
- **Tests on everything:** Part II preamble and Part III both assert "ships
  tests" per module/agent, honouring the project rule. ✓

---

## H. Overall verdict

**PASS-WITH-FIXES.**

The plan is a faithful, precise rendering of the rebased (2026-09-17) model. The
OLD-MODEL-LEAK check is **clean** on all five forbidden constructs; the §II.0
ground-truth claims are **all verified genuine** against the live door/schema;
scope discipline holds (no out-of-scope design, no deferred model resolved); the
open seams (G4/G5/G6/G7/G10) are correctly left open with sound interim
positions; and the delegation graph is acyclic with Agent 1 correctly the
foundation. No BLOCKER or MAJOR defect exists.

**Must-fix before dispatching coding agents (all MINOR, all are precision gaps
where the substance already lives elsewhere in the plan):**

- **F1** — state in §II.0 that `mint` drops its `type` parameter and stops writing
  `token.type` (before Agent 1).
- **F2** — surface in the face grammar (I.B/I.E) that a grouping-only node carries
  no own ADDRESS span; its handle is the naming-literal token_id (crude: NOTATION)
  which must be a live/nested token (`add_membership` FKs both endpoints) — before
  Agent 3.
- **F3** — qualify the co-index "equal length" rule in II.1 to admit NOTATION
  blank slots (length-N) and MEMBER_OF section-2 sparseness (≤N) — before Agent 2.

F4–F9 are advisory OBSERVATIONS; they improve accuracy but do not block
implementation.
