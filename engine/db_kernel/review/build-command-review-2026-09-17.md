# Adversarial build review — `command/` (Command IR + span planner)

**Reviewer:** fresh independent adversary (no stake in passing).
**Date:** 2026-09-17.
**Target (untouched working tree):** `/opt/project/repo/engine/db_kernel/command/`
— `command_ir.h`, `command_ir.cpp`, `span_planner.h`, `span_planner.cpp`,
`command_ir_test.cpp`, `span_planner_test.cpp`, `README.md`.
**Governing spec:** `PLAN.md` §I.B–I.F, §II.1–II.2, Part V; `NOTES.md`
"Relationship model & type — firmed 2026-09-17", "The literal intake formula"
(REVISED 2026-09-17), "DECLARE format" (REVISED), "Arraying is universal",
"UPDATE — record-tier ops", "READ — record-tier exploratory read";
`codec/codec.h`.

---

## 1. Test build + run — VERBATIM

Built exactly as the module README / task specifies, from `command/`:

```
g++ -std=c++17 -O2 -Wall -Wextra -I../codec -o /tmp/cir_test \
  ../codec/codec.cpp command_ir.cpp span_planner.cpp command_ir_test.cpp && /tmp/cir_test
g++ -std=c++17 -O2 -Wall -Wextra -I../codec -o /tmp/sp_test \
  ../codec/codec.cpp command_ir.cpp span_planner.cpp span_planner_test.cpp && /tmp/sp_test
```

Both compiled with **zero warnings** under `-Wall -Wextra` and **zero errors**.

`command_ir_test` — all 26 checks `ok`, final line `PASS command_ir_test`, exit 0.
`span_planner_test` — all 31 checks `ok`, final line `PASS span_planner_test`, exit 0.

The claimed green is real and independently reproduced.

## 2. Test-quality judgement — MEANINGFUL (not shallow)

Both suites exercise the spec'd behaviour, not just "no crash." Each check
perturbs exactly one thing off a minimal valid baseline and asserts a specific
outcome; the planner tests assert **actual planned values** (slot addresses,
`nested`/`is_undeclared_hook` markers, `PlanStatus`), not merely non-failure.
`successor`/`span_length` are checked for increment, cross-element carry,
leftmost overflow→`nullopt` (G5), partial rejection, empty, backward-run
rejection, and depth-mismatch rejection. Validation is checked with paired
accept/reject cases for every DECLARE rule and lighter presence/shape coverage
for READ/MOVE/ADD/DELETE. Coverage is strong. Gaps noted in F-2/F-4 below (test
completeness, not wrong-behaviour).

## 3. OLD-MODEL-LEAK check — CLEAN

Grep over all four source files: no `TYPE` field or type gating (only comments
stating TYPE is dropped); no `token_sibling_group`/sibling-group edge; no label
`CHILDREN` field (`MEMBERS` correctly replaces it); no `token_child` membership
notion. The `sibling-in-this-statement` phrase (`command_ir.h:33`) denotes a
peer-**address** reference within one statement, not the dropped sibling-group.
`AddressSegment::Kind` is the span-grammar segment kind, not a stored
relationship connection-kind. State clean.

## 4. Findings

### F-1 — MOVE_RECORD destination validated with the full span grammar — OBSERVATION (surface to Patrick/core)
- **File:** `command_ir.cpp:228-236` (`validate_move_record` → `validate_address_span_nested` + `plan`).
- **Code:** `auto nested_result = validate_address_span_nested(op.destination); ... PlanResult plan_result = plan(op.destination, op.tokens.size());`
- **Spec:** `NOTES.md` MOVE (line 599-600): "A range from->to relocation reusing
  the intake placement variables (**FROM/AFTER/TO/direct** — the SAME span
  grammar as DECLARE ADDRESS)." MOVE is "Strictly BOOKKEEPING... Changes NO
  relationships... nothing wired or unwired." PLAN §I.D: MOVE "Bookkeeping only".
- **Why:** the generic span validator/planner accept **every** segment kind,
  including `kNestedDeclare` (which mints a new token) and `kUndeclared` (the
  cross-connection hook). A MOVE destination bearing a nested declare or an
  undeclared hook is semantically incoherent for a mint-nothing relocation.
  NOTES line 599 parenthetically lists only FROM/AFTER/TO/direct for MOVE, while
  line 600 says "the SAME span grammar" — the spec is itself in tension; the
  module took the "same grammar" reading, which is defensible.
- **Severity rationale:** the IR layer is structural-only; enforcing a MOVE-span
  *subset* would itself be an inference. Not an implementation error. Flag the
  DECLARE-vs-MOVE span-subset question to Patrick / the UPDATE core, do not
  silently bake either way.
- **Narrowest correction:** none in this module; record the open question.

### F-2 — `kPendingSeam`-in-DECLARE is implemented but untested — MINOR (must-fix: test)
- **File:** `command_ir.cpp:192-200`.
- **Code:** `PlanResult plan_result = plan(*node.address, n); if (plan_result.status == PlanStatus::kInvalid) { return Invalid(...); }` — `kPendingSeam` deliberately falls through to `Valid()`.
- **Spec:** PLAN §I.E / Part V (G4/G5): an `AFTER` span is structurally
  well-formed but unresolvable now; it must **not** fail declare validation.
- **Why:** this is a load-bearing seam behaviour (a DECLARE whose ADDRESS is an
  `AFTER:b` tail must validate). `span_planner_test` covers `plan()` returning
  `kPendingSeam`, but **no test** drives `validate_declare` with an AFTER-based
  ADDRESS span to confirm the fall-through-to-valid. Project rule is tests on
  everything; a central seam behaviour has zero integration coverage.
- **Narrowest correction:** add one `command_ir_test` case: a structure declare
  whose `address` is `{Direct, After(...)}` covering N, asserting `kValid`.

### F-3 — "base-50 successor" naming vs base-2500 per-element radix — OBSERVATION
- **File:** `span_planner.h:31-33`, `span_planner.cpp:18-24`; README "Base-50 address successor".
- **Why:** `successor()` treats each **element** (couplet) as a base-`kCoupletSpace`
  (2500) digit. This is numerically identical to a base-50 counter over the
  couplet's two characters (couplet = `first*50 + second`; +1 carries second→first
  within the couplet, then couplet→element on overflow — verified by hand and by
  the `AA.zz → AB.AA` test). Matches PLAN §II.2's own "base-50" wording. Purely
  nominal; no semantic defect.
- **Correction:** none required.

### F-4 — zero-width elastic tail is silently accepted and untested — OBSERVATION
- **File:** `span_planner.cpp:202-224`.
- **Why:** if segments before an open (un-`TO`'d) `FROM`/`AFTER` last segment
  already cover exactly N, `remaining == 0`; the elastic loop runs zero times and
  the origin address is ignored. Spec (§I.E) does not forbid a zero-width elastic
  tail; behaviour is defensible but untested and mildly surprising.
- **Correction:** optional — a test pinning the chosen behaviour would remove the
  ambiguity. Not a defect.

### F-5 — no IR slot for a live/inline-nested grouping naming-literal — OBSERVATION (scope)
- **File:** `command_ir.h:162-193` (`DeclareRecord`); enforced at `command_ir.cpp:143-152`.
- **Why:** PLAN §I.B: a grouping node's naming literal "must be a live or
  inline-nested token row." The IR represents the naming literal **only** as
  NOTATION prose (crude stage). It cannot carry a live-token or inline-nested
  naming literal for a grouping-only node.
- **Severity rationale:** correct for the **crude stage**, which is the stated
  current build; the mature form is reached via the deferred `prose→token_id`
  SET_FIELD UPDATE swap, not a direct DECLARE. Scoping assumption, not a defect.
- **Correction:** none now; note the scope.

No BLOCKER or MAJOR findings. IR fidelity to the face grammar (§ checklist 1),
structural validation without TYPE gating (checklist 2), and the span planner
(checklist 3) are all spec-faithful; specifics verified: DECLARE fields =
ADDRESS/NOTATION/PARENTS/MEMBERS/MEMBER_OF (two `|` sections) with no TYPE;
all six record-tier verbs present; TO folded into its origin (standalone TO
structurally unrepresentable — faithful to §I.E "never standalone"); required
downward field (reject neither), structure `>=2`, grouping `>=1`, NOTATION
length-N with legal blanks, MEMBER_OF §2 sparse (index-range-only), span
cover-N, nested recurse; elastic-last-only, interior self-delimiting, nested =
one slot, base-50 successor carry/overflow correct, G4/G5 not resolved
(AFTER→`kPendingSeam`, overflow→`nullopt`, analyst-supplied FROM fill).

## 5. Adjudication — the 3 flagged inferences

### Inference 1 — N = 1 for a pure-grouping node → **FAITHFUL-DERIVATION**
- **Code:** `command_ir.cpp:91` `const std::size_t n = has_parents ? node.parents->size() : 1;`
- **Spec:** the IR is per-node (`PLAN §I.B` "Applies per node"). A grouping
  declare mints exactly **one** group with **one** naming literal (`NOTES.md`
  grounding rule, lines 520-528, 540-549). `NOTES.md:587`: "a group's members ARE
  an array" — the arraying that enables label-start is the MEMBERS roster itself,
  authoring **one** group, not multiple. So N (tokens this node declares) = 1,
  mirroring "PARENTS is the member array and sets N" (`NOTES.md:466`) on the
  structure side. NOTATION then names that one group (length 1), and MEMBER_OF §2
  indices range over the single slot. Correctly follows the cited spec; not a
  silent semantic invention. The IR modelling MEMBERS as a flat `vector<Reference>`
  (one roster, not many) is consistent with this and reinforces it.

### Inference 2 — grouping-only node requires a non-blank NOTATION → **FAITHFUL-DERIVATION**
- **Code:** `command_ir.cpp:143-152`.
- **Spec:** grounding rule (`NOTES.md:520-528, 540-546`): "a label with no naming
  literal is unaddressable — unreferenceable, unable to roll up"; crude-stage
  naming literal *is* the temporary NOTATION prose (the deliberate **construction
  post**, lines 530-538). A grouping-only node has no PARENTS, so the general
  "blank/absent NOTATION → surface derived by concatenating parent values"
  (`NOTES.md:494-497`) cannot apply — there is nothing to derive from. The
  construction-post rule *reinforces* the reject: a post by definition bears prose;
  a blank grouping node has neither a real naming literal nor a construction post,
  hence no identity at all. Correctly derived from cited spec; not a silent bake.

### Inference 3 — a `Reference` is exactly one of {address, nested} → **CODE-STRUCTURE**
- **Code:** `command_ir.cpp:17-24` (`validate_reference`), `command_ir.h:35-54`.
- **Spec:** `NOTES.md:382-383` / PLAN §I.B recursion: a relationship field is "a
  reference **OR** a full nested DECLARE." The exclusivity is the natural reading
  of that disjunction; both-set is contradictory and neither-set is an empty node.
  Rejecting both/neither is an IR well-formedness invariant (an implementation
  choice about the parsed representation), not a semantic decision the spec leaves
  open. Acceptable to keep.

---

## Verdict — **PASS-WITH-FIXES**

The module is spec-faithful (no old-model leak, no TYPE gating, faithful grammar
+ validation + planner), tests are green and meaningful, and all three flagged
inferences are acceptable to keep (two faithful derivations, one code-structure).

**Must-fix (1, minor):**
- **F-2** — add a `command_ir_test` case driving `validate_declare` with an
  `AFTER`-based ADDRESS span, asserting it validates (`kPendingSeam` must not fail
  declare validation). A load-bearing seam behaviour currently has zero coverage.

**Surface to Patrick / UPDATE core (not a module fix):**
- **F-1** — decide whether MOVE_RECORD's destination span is the full DECLARE
  grammar or the FROM/AFTER/TO/direct subset (the module accepts mint-bearing
  nested declares and the undeclared hook in a bookkeeping-only relocation).

Observations F-3/F-4/F-5 need no action.
