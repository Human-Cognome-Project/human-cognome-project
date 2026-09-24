# Adversarial re-review #2 — `command/` (Command IR + span planner) after F-1 / wildcard / cover-N changes

**Reviewer:** fresh independent adversary (no stake in passing).
**Date:** 2026-09-17.
**Target (untouched working tree):** `/opt/project/repo/engine/db_kernel/command/`
— `command_ir.h`, `command_ir.cpp`, `span_planner.h`, `span_planner.cpp`,
`command_ir_test.cpp`, `span_planner_test.cpp`, `README.md`.
**Governing spec:** `PLAN.md` §I.B–I.F, §II.1–II.2, §II.5, Part V; `NOTES.md`
"Relationship model & type — firmed 2026-09-17", "UPDATE — record-tier ops",
"READ — record-tier exploratory read", "The literal intake formula";
`codec/codec.h` + `codec/codec.cpp` (`is_valid_address`).

Changed since re-review #1 (by mtime + byte size): `command_ir.h` (11706→14512),
`command_ir.cpp` (9820→13691), `span_planner.h` (6162→7197),
`span_planner.cpp` (8290→11118), `command_ir_test.cpp` (15944→24160),
`README.md` (7352→9389). `span_planner_test.cpp` (9727) unchanged. `codec/*`
untouched (mtime 2026-09-14). Nothing outside `command/` was altered by this
work.

---

## 1. Test build + run — VERBATIM

From `command/`, exactly as README/task specify:

```
g++ -std=c++17 -O2 -Wall -Wextra -I../codec -o /tmp/cir_test ../codec/codec.cpp command_ir.cpp span_planner.cpp command_ir_test.cpp && /tmp/cir_test
g++ -std=c++17 -O2 -Wall -Wextra -I../codec -o /tmp/sp_test  ../codec/codec.cpp command_ir.cpp span_planner.cpp span_planner_test.cpp && /tmp/sp_test
```

Both compiled with **zero warnings** under `-Wall -Wextra`, zero errors.
`command_ir_test` — **50/50 `ok`**, final `PASS command_ir_test`, exit 0.
`span_planner_test` — **31/31 `ok`**, final `PASS span_planner_test`, exit 0.
Independently reproduced; the claimed green is real.

## 2. Test-quality judgement — MEANINGFUL

The new MOVE tests assert real status across the whole matrix: explicit accept /
empty reject / open-FROM-non-last-destination reject / under-cover reject /
over-cover reject (known-N path); range accept / backward-range reject / prefix
accept / concrete-as-kPrefix reject / wildcard-dest-any-count accept / mixed
explicit+range defers / full-grammar destination accept (unknown-N path). The
wildcard per-op tests assert accept-vs-reject per field. All assert concrete
`ValidationStatus`, not mere non-crash. Non-vacuous. Caveat: the READ/ADD
wildcard tests (`command_ir_test.cpp:446-473`) **lock in** a contested permissive
semantic decision — see G-2/G-3; a green test over a wrong-way bake is still a
defect regardless of its greenness.

## 3. OLD-MODEL-LEAK check — CLEAN

Grep over all four sources: no TYPE field/gating (comments only), no
sibling-group, no label-CHILDREN field, no token_child membership notion. Two
`grep` hits are benign: `command_ir.h:33` "sibling-in-this-statement" = a
peer-**address** reference within one statement (not the dropped sibling-group
edge); `command_ir.h:58` "not by this type" = the C++ type `ConstituentList`,
not the DECLARE TYPE field. State clean.

## 4. inf1 / inf2 / inf3 — NOT regressed

- **inf1** (N=1 for a pure-grouping node) — `command_ir.cpp:108`, logic
  unchanged → still **FAITHFUL-DERIVATION**.
- **inf2** (grouping-only requires non-blank NOTATION) — `command_ir.cpp:160-168`,
  unchanged → **FAITHFUL-DERIVATION**.
- **inf3** (`Reference` exactly one of {address, nested}) — `command_ir.cpp:34-49`,
  unchanged → **CODE-STRUCTURE**.

## 5. Mechanical re-checks — CORRECT

- **MoveSource well-formedness** (`command_ir.cpp:243-273`): `kExplicit` requires
  a concrete valid address (`is_explicit_address`); `kRange` requires a TO, both
  ends concrete, and `span_length(from,to)` valid = forward same-depth; `kPrefix`
  requires `is_valid_address && is_wildcard` (last element partial). All three
  enforced correctly; `kPrefix` given a concrete (non-partial) address is
  rejected. Correct as implemented (existence of `kPrefix` is a separate spec
  question — G-1).
- **Cover-N split** (`command_ir.cpp:277-324`): all-`kExplicit` ⇒ N =
  `sources.size()` known ⇒ `plan(destination, N)` with `kInvalid` rejected (same
  path as DECLARE; `kPendingSeam` correctly falls through to valid). Any
  non-explicit source ⇒ `validate_span_shape(destination)` only, cover-N skipped
  for the whole statement. Mixed explicit+range correctly defers (via
  `std::all_of`). Matches the stated "whole-MOVE deferral". Correct.
- **`validate_span_shape`** (`span_planner.cpp:248-310`): the exact N-independent
  subset of `plan()`'s rules — valid addresses, nested-declare recursion, closed
  `FROM..TO` forward same-depth (via `span_length`), `AFTER..TO` left unchecked
  (G5), open origin last-only; no cover-N. Complete and correct for its purpose.
- `successor` / `span_length` unchanged from re-review #1 (verified correct there).

## 6. Findings

### G-1 — `kPrefix` (prefix/context wildcard) MOVE source is not in the written spec — MAJOR (must-fix)
- **File:** `command_ir.h:243-278` (`MoveSource::Kind::kPrefix`), `command_ir.cpp:265-270`.
- **Spec (actual text):** `NOTES.md:599` — "**MOVE_RECORD.** A range from->to
  relocation reusing the intake placement variables (FROM/AFTER/TO/direct — the
  SAME span grammar as DECLARE ADDRESS)." `PLAN.md:156` — "a `from→to` relocation
  reusing the ADDRESS span grammar ... **Ranged / arrayable**." The word
  **"wildcard" appears in the spec ONLY as DELETE's prohibition** (`NOTES.md:633-634`,
  `PLAN.md:169,171,305`; also `NOTES.md:727,797`) — never as a MOVE capability.
  "Bulk correction", "misclassified branch/region", "trunk/block followed as a
  whole" (the comment's justification, `command_ir.h:230-242`) appear **nowhere**
  in NOTES or PLAN.
- **Why it's a defect:** `kRange` already covers the stated "range from->to". A
  prefix wildcard ("A\*" selecting the whole block/trunk beneath it) is a
  distinct, additional selection mechanism the written spec does not grant MOVE.
  The ADDRESS span alphabet (`PLAN.md:174-185`) is FROM/AFTER/TO/direct/@-pin/
  undeclared-hook/nested — a prefix/context wildcard is not a member of it, so
  "reusing the SAME span grammar" does not supply it. Further, "select the whole
  block beneath a prefix" is contestable against the **only-follow / never-search**
  hard constraint (`PLAN.md IV`; `NOTES.md:634` "consistent with only-follow /
  never-search") — the code comment *asserts* it is a follow-not-a-search, but the
  codec defines a partial address as a prefix/context node addressing a **set**
  ("the up to 50 couplets it could still extend to", `codec.h:70`), and walking
  that set is exactly the kind of subtree selection that needs Patrick's ruling,
  not a baked assertion. The test/comment attribute this to "Patrick's later
  ruling" (`command_ir_test.cpp:307`) — if such a ruling exists it is **not
  recorded in the governing text**, which is precisely the single-locus /
  unrecorded-decision hazard the project's own rules warn against.
- **Narrowest correction:** either (a) record the wildcard-MOVE-source ruling in
  `NOTES.md` "UPDATE — record-tier ops" / `PLAN.md §I.D` (with its only-follow
  justification) so the code has spec backing, or (b) remove `kPrefix` until it
  is written down. Do not leave an unrecorded capability baked.

### G-2 — READ accepts wildcard exclusions, against a stated "specific token_ids ONLY" rule — MAJOR (must-fix: surface, don't bake)
- **File:** `command_ir.cpp:222-238` (`validate_read`, plain `is_valid_address`),
  tests `command_ir_test.cpp:455-460`.
- **Spec (actual text):** `NOTES.md:662-663` — "**Exclusions** — prune named
  branches (**specific token_ids ONLY**, NOT property filters — **a predicate
  would force a search**)."
- **Why:** a partial/wildcard address is **not** a specific token_id — it is a
  prefix/context node addressing a set (`codec.h:70`). Excluding "everything under
  A\*" is a pattern/branch prune, i.e. exactly the predicate-that-forces-a-search
  the spec rules out (and contradicts only-follow / never-search). The code's
  comment claims "no stated rule either way, so none is invented"
  (`command_ir.cpp:223-228`) — but there **is** stated text ("specific token_ids
  ONLY"), and accepting a wildcard silently resolves it in the permissive
  direction. This is a bake, not neutrality: `validate_read` returns `kValid` for
  a wildcard exclusion.
- **Ruling:** **NEEDS-PATRICK** (leaning should-reject). Narrowest correction:
  do not return `kValid` for a wildcard exclusion until Patrick confirms; surface
  the "specific token_ids only" conflict.

### G-3 — READ anchor and ADD_CONNECTION endpoints accept wildcards, against leaning spec text — MAJOR (must-fix: surface, don't bake)
- **File:** `command_ir.cpp:229` (READ anchor), `command_ir.cpp:326-343`
  (ADD_CONNECTION), tests `command_ir_test.cpp:449-453, 462-472`.
- **Spec (actual text):**
  - READ anchor — `NOTES.md:652` "**Anchor** — a single token_ID to begin from";
    `PLAN.md:141` "a single token_id". A wildcard is not a single leaf token.
  - ADD_CONNECTION — `NOTES.md:611-617` "add one or more elements to a label ...
    Declares/mints no terms ... **all endpoints must pre-exist** ... **each
    addition is a pair(label, element)**"; `PLAN.md I.B:130` "`add_membership`
    **FKs both endpoints to real tokens**". A wildcard/partial address does not
    identify one real token row that can pre-exist, and a wildcard element is a
    bulk/pattern assertion, contrary to the pairwise-specific framing.
- **Why:** both fields silently accept a wildcard (`kValid`) although the spec
  describes single, concrete, pre-existing tokens. Same bake-in-permissive-
  direction pattern as G-2. The READ-anchor case is weaker (a context node is
  still an addressable node); the ADD_CONNECTION case is stronger (pairwise +
  FK-to-real-token).
- **Ruling:** **NEEDS-PATRICK** for both. Narrowest correction: surface the
  conflict; don't return `kValid` for wildcard ADD_CONNECTION endpoints (and,
  pending Patrick, the READ anchor) as a baked allowance.

### G-4 — `is_explicit_address` / `is_valid_address` accept the EMPTY address — OBSERVATION
- **File:** `command_ir.cpp:26-32`; `codec/codec.cpp:92-103` (`is_valid_address({})`
  returns true).
- **Why:** an empty address is neither a wildcard nor a real token, yet
  `is_explicit_address({})` is true, so `DELETE_RECORD`/`DELETE_CONNECTION`/
  `MoveSource::Explicit` accept an empty token. Latent (codec-level, pre-existing),
  not introduced here.
- **Correction:** optional — an explicit `!address.empty()` guard in
  `is_explicit_address`. Flag only.

### G-5 — MOVE destination accepts mint-bearing segments (nested declare, undeclared hook) — OBSERVATION (consciously deferred)
- **File:** `command_ir.cpp:298-323`; test `command_ir_test.cpp:411-420` asserts a
  nested-declare + undeclared-hook + open-tail destination is accepted.
- **Why:** for a bookkeeping-only relocation (`NOTES.md:599-608` "mints/wires
  nothing"), a nested-declare (which mints) or undeclared-hook destination segment
  is semantically odd — this is my prior F-1 concern. Now a deliberate design
  choice (per the team lead: F-1 destination-cover-N and grammar-subset deferred
  to the UPDATE core, Agent 5).
- **Correction:** none in this module; the UPDATE core must reject mint-bearing
  MOVE destinations. Recorded so it is not lost.

---

## 7. Per-op wildcard adjudication (the central ask)

| Op | Behaviour | Spec cited | Ruling |
|---|---|---|---|
| DELETE_RECORD / DELETE_CONNECTION | reject wildcard (`is_explicit_address`) | `NOTES.md:633-634`, `PLAN.md:169,171` — "specific IDs and connections ONLY — no wildcards" | **FAITHFUL** |
| READ (anchor + exclusions) | accept wildcard (plain `is_valid_address`) | `NOTES.md:652` "a single token_ID"; `NOTES.md:662-663` "specific token_ids ONLY ... a predicate would force a search" | **NEEDS-PATRICK** (G-2/G-3) |
| ADD_CONNECTION (group + elements) | accept wildcard | `NOTES.md:611-617` "endpoints must pre-exist / each addition is a pair"; `PLAN.md I.B:130` "FKs both endpoints to real tokens" | **NEEDS-PATRICK** (G-3) |
| MOVE source (`kPrefix` wildcard) | accept wildcard as a source kind | `NOTES.md:599` / `PLAN.md:156` — MOVE = range from->to only; "wildcard" nowhere for MOVE | **NEEDS-PATRICK** (G-1: record in spec or remove) |

The implementer's governing principle ("don't invent a restriction OR an
allowance") is right in spirit but misapplied: for READ-exclusions,
ADD_CONNECTION, and the MOVE `kPrefix` source the spec is **not** silent —
`NOTES.md:662` ("specific token_ids only"), `NOTES.md:611-617` (pairwise,
endpoints pre-exist), and the range-only MOVE grammar all lean against the
permissive branch. Choosing "accept" and returning `kValid` **is** a decision;
the honest move for a genuine gap is to surface it, not to hard-code the
permissive reading.

---

## Verdict — **PASS-WITH-FIXES**

The module compiles clean, both suites are green and meaningful, the cover-N
split / MoveSource well-formedness / `validate_span_shape` mechanics are correct,
DELETE's no-wildcard rule is faithfully enforced, the OLD-MODEL-LEAK check is
clean, and inf1/inf2/inf3 are not regressed. But three spec-conformance items are
baked without written spec support and must be resolved before "ready":

**Must-fix:**
- **G-1** — `kPrefix` MOVE source: record Patrick's ruling in `NOTES.md`/`PLAN.md`
  (with only-follow justification) or remove it. Not in the written spec.
- **G-2** — READ wildcard **exclusions** conflict with `NOTES.md:662` "specific
  token_ids ONLY / a predicate would force a search"; surface, don't return
  `kValid`.
- **G-3** — READ anchor + ADD_CONNECTION endpoints accept wildcards against
  leaning spec text (`NOTES.md:652`, `611-617`; `PLAN.md I.B:130`); surface to
  Patrick, don't bake the allowance.

Observations G-4 (empty address) and G-5 (mint-bearing MOVE destination, deferred
to Agent 5) need no change in this module.
