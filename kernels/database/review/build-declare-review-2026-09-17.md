# Adversarial build review — DECLARE core (`declare/`)

**Reviewer role:** fresh, independent adversary. No stake in the module passing.
**Date:** 2026-09-17
**Scope reviewed (read in full):** `declare/declare_core.h`, `declare/declare_core.cpp`,
`declare/declare_core_test.cpp`, `declare/README.md`.
**Consumed foundation (read in full, confirmed untouched):** `command/command_ir.h`,
`command/command_ir.cpp`, `command/span_planner.h`, `command/span_planner.cpp`,
`controller/controller.h`, `schema/schema.sql`.
**Governing spec (read the named parts in full):** `PLAN.md` §I.B, §I.E, §I.F, §I.G,
§II.3, §II.6, Part V; `NOTES.md` "Relationship model & type — firmed 2026-09-17",
"The literal intake formula" (REVISED), "DECLARE format" (REVISED), "Build-phase
rulings — firmed 2026-09-17", the label-dual / construction-post paragraphs.

No file was modified. The working tree was reviewed untouched; the test binary was
built and run from the session scratchpad only.

---

## 1. Verbatim build & test outcome

**Build** (exact README invocation, output to scratchpad binary):

```
g++ -std=c++17 -O2 -Wall -Wextra -I. -I../codec -I../command -I../controller \
    -I$(pg_config --includedir) declare_core.cpp declare_core_test.cpp \
    ../codec/codec.cpp ../command/command_ir.cpp ../command/span_planner.cpp \
    ../controller/controller.cpp -L$(pg_config --libdir) -lpq -o <scratch>/declare_core_test
EXIT: 0
```

Clean compile. No warnings under `-Wall -Wextra`.

**Run** (live local Postgres; `hcp3_core` present; schema reset+reapplied):

```
NOTICE:  drop cascades to 5 other objects
DETAIL:  drop cascades to table token ... token_parent ... token_child ... members ... member_of
ok   seed fixtures: atoms minted directly via the controller
ok   single declare: returns one outcome
ok   single declare: returns the actual assigned address
ok   single declare: constituents linked in order, references only
ok   single declare: no per-constituent mass (LINK writes SQL NULL)
ok   single declare: blank mass stored as SQL NULL
ok   single declare: NOTATION stored
ok   single declare: MEMBER_OF authors the up-membership edge
ok   single declare: MEMBER_OF's members reciprocal is written too
ok   multi-member set: N outcomes for N=2 PARENTS
ok   multi-member set: member 0 placed at the FROM origin
ok   multi-member set: member 1 placed at the sequential successor
ok   multi-member set: NOTATION co-indexed to the right member
ok   multi-member set: each member's own constituents linked
ok   grouping (mixed node): the structural placement is returned
ok   grouping (mixed node): members written (downward)
ok   grouping (mixed node): member_of reciprocal written (upward), both members
ok   nested declare: outer statement returns its own placed address
ok   nested declare: the nested sub-structure was minted
ok   nested declare: outer constituent list collapses onto the nested address
ok   nested declare: the whole connected sub-structure was built in one call
ok   SEE idempotency: re-declaring an existing token does not throw
ok   SEE idempotency: re-declare still returns the actual (unchanged) address
ok   SEE idempotency: re-declare leaves token_parent unchanged (no duplicate links)
ok   SEE idempotency: re-declare leaves the MEMBER_OF edge singular (no duplicate)
ok   blank NOTATION: declare still succeeds with NOTATION entirely absent
ok   blank NOTATION: stored blank (empty string), not a placeholder
ok   blank NOTATION case also confirms mass stored as SQL NULL
ok   rejection setup: the missing constituent is absent
ok   rejection: a missing constituent does not throw
ok   rejection: the point is reported un-ingested with a reason
ok   rejection: nothing was minted at the rejected point's address
ok   pure grouping-only node: returns its naming-literal token_id, not a freshly placed address
ok   pure grouping-only node: members written (downward)
ok   pure grouping-only node: member_of reciprocal written (upward)
ok   pure grouping-only node: an unresolvable naming literal is rejected, not fabricated
PASS declare_core_test
EXIT: 0
```

36/36 checks pass. This is a genuine pass against a live store — not a
skipped/faked run.

---

## 2. Test-quality judgement

**Meaningful, not shallow.** The harness resets and reapplies the real schema and
asserts against read-back state through the door, not just against the return
struct. Every item the instructions called for is present and non-trivial:

- **Blank mass → SQL NULL (not 0):** asserted on both the whole-token mass
  (`attributes_of(BA)->mass` is nullopt) AND the per-constituent mass
  (`parents_of(BA)[i].mass` is nullopt). This is the "0 != not-yet-computed"
  invariant, checked on the correct nullable channels. Strong.
- **Multi-member set (N from PARENTS, co-indexed NOTATION/MEMBER_OF):** N=2 from
  `parents->size()`, FROM origin fills slot 0 = `CA` and slot 1 = `CB`
  (successor), NOTATION `multi-0`/`multi-1` land on the right members, each
  member's own 2-constituent list is linked. Verifies the co-index + sequential
  fill, not just the count.
- **Grouping node writing BOTH directions:** `members_of(DA) = {p3,p4}` AND
  `member_of(p3)={DA}`, `member_of(p4)={DA}` — reciprocal confirmed on both
  tables (§II.6 synchronous reciprocal).
- **Nested declare building a sub-structure:** outer `EA` constituent list
  collapses onto nested `EB`'s address; `EB`'s own 2-constituent composition
  (`p1,p3`) is independently verified — the whole connected sub-structure in one
  call.
- **SEE idempotency:** re-declares `BA` with the already-linked MEMBER_OF edge;
  asserts no throw, unchanged address, `token_parent` count unchanged, and the
  MEMBER_OF edge still singular. Exercises both the mint-SEE and the
  membership-dedup guard.
- **Returns actual assigned address:** every ingested check asserts
  `outcome.address == <expected>`, not merely `ingested`.
- **Rejection → un-ingested (not thrown):** missing constituent `ZZ`; asserts
  no exception, `!ingested` with a non-empty reason, and that nothing was minted
  at `GA`.

**Gaps in coverage (test-suite completeness, not code faults).** The three
flagged edge combinations are, by the implementer's own admission, NOT exercised:
(a) MEMBERS broadcast for a mixed node with N>1; (b) an ADDRESS-span
`kNestedDeclare` segment colliding with the same slot's own PARENTS entry; (c) a
mixed node's MEMBERS entry that fails to resolve (the skip-one-edge path,
lines 271-276). Also untested: `@`-pin placement (`kPin`), the undeclared-hook
rejection (`resolve_placement` line 158-162), the AFTER/pending-seam rejection
paths, MEMBER_OF section-2 (`per_member`) sparse entries, and a nested reference
that collapses to >1 outcome (the `resolve_reference` size!=1 rejection,
lines 67-72). These are real branches with no test. See F-6.

---

## 3. OLD-MODEL-LEAK check

**Clean.** No `TYPE` field or `type` argument anywhere (`mint` is called with the
new 4-arg no-type signature, `declare_core.cpp:246`). No `token_sibling_group` /
`SIBLINGS` reference. No `CHILDREN`-as-a-field; grouping is authored through
`MEMBERS` → `members`/`member_of` via `add_membership`. Validation is purely
structural (presence of PARENTS / MEMBERS). Scope is confined to `declare/`:
`declare_core.cpp` includes only its own header + `span_planner.h`; it writes
nothing in `command/`, `controller/`, `schema/`, `read/`, or the ingestion
runtime. Consumes the foundation exactly as documented.

---

## 4. `author_membership_dedup` ruling (instruction item 6)

**Code:** `declare_core.cpp:47-55` reads `ctl.member_of(member)` and skips
`add_membership` if `group` is already present, to avoid a duplicate-key throw
(the schema PKs `member_of(token_id, group_token_id)` / `members(token_id,
member_token_id)`; `add_membership` has no `ON CONFLICT`).

**Ruling: correct and within-scope as implemented; but it flags a door-layer gap
that should be closed at the controller, not left as a per-caller guard.**

- *Functionally correct for this runtime.* The store is single-owner (the
  controller is the sole writer, one process owns the DB per NOTES "Process
  runtime"), and execution is sequential, so the read-then-write is not a real
  TOCTOU here. `add_membership` is atomic across both rows, so checking one side
  (`member_of`) is sufficient — the two tables can never disagree. Verified live
  by the SEE-idempotency test (the second link is skipped, no throw).
- *But the architecturally correct home is `add_membership` itself.* The spec
  makes SEE-dedup a property of the sole-owner **write path**, not of each
  caller: `mint` carries its own SEE probe (`controller.h:116-118`; NOTES "Hard
  constraints": "deduped by mint's SEE"). The membership axis is described as the
  exact mirror of that discipline ("add_membership … both directions", "the same
  discipline 'fold AND wire' applies", schema header). Leaving `add_membership`
  non-idempotent forces every membership-writing caller — this module today,
  ADD_CONNECTION (Agent 5) and the arraying executor (Agent 6) tomorrow — to
  re-implement the same guard, and a caller that forgets it gets a hard throw
  instead of ensure-plus-link. That is the opposite of the "store the tax once /
  ensure-plus-link, no explosion" model.
- *Why it is nonetheless the right call for Agent 3.* PLAN.md Part III scopes
  Agent 3 to `declare/` with `command/controller/schema` untouched. The
  implementer could not add SEE to `add_membership` without breaching that
  boundary. Given the boundary, an in-module guard is the correct choice, and it
  is honestly disclosed in the README.

**Recommendation (follow-up, not a must-fix for this module):** move the SEE
probe into `Controller::add_membership` (mirroring `mint`), then delete
`author_membership_dedup` and call `add_membership` unconditionally. This is a
controller (Agent 1) change, to be scheduled before Agent 5/6 duplicate the
guard. Severity: LOW (design-consistency), not a correctness defect.

---

## 5. Adjudication of the 4 flagged decisions

### Decision #1 — grouping-only naming literal → **NEEDS-PATRICK** (genuine IR/grammar gap)

`execute_grouping_only` (`declare_core.cpp:289-339`) resolves a PARENTS-absent
node's naming literal by reading `NOTATION` slot-0 as a **token_id string**
(`codec::decode_token_id(*node.notation[0])`) and requiring `token_exists`.

**This is not faithful transcription; it is a semantic overload of a
prose-defined field, and the correct fix is an IR change out of this module's
authority. It must be surfaced.**

Grounds, from spec:

1. **NOTATION is defined as prose, not an address.** PLAN.md I.B: NOTATION is
   "quoted; positional-partial … blanks legal". NOTES "DECLARE format": "may
   contain spaces, e.g. `"single hex codes"`". Examples are `"0x"`,
   `"particle-3"`, `"single hex codes"` — none of which `decode_token_id` (which
   parses a dot-joined base-50 couplet form like `AA.AB`) can accept. A genuine
   prose NOTATION would be **rejected** by this code. The path only works if the
   caller abandons prose and stuffs a token_id string into the prose field.
2. **It inverts the construction-post doctrine.** NOTES (label dual /
   bootstrap): in the crude stage the naming literal "is stood in for by the
   temporary `notation` prose string"; temp labels are "deliberate construction
   posts — a placeholder identity you build the real structure against, then pull
   once the genuine naming literal exists." The prose is precisely the thing that
   does **not** yet correspond to a real token. This code **requires** it to
   already `token_exists`, defeating the bootstrap-breaking purpose of the crude
   prose stage.
3. **only-follow forbids the prose→token route.** The store keys by
   address/token_id; there is no index from notation prose to a token (that would
   be a search). So a real prose value can never resolve; only a token_id-string
   resolves — a category error, not a lookup.
4. **The IR genuinely lacks the needed field.** For a PARENTS-absent node the
   only fields are `notation` (prose), `members`, `member_of`; `address` is
   *forbidden* (`command_ir.cpp:155-159`). PLAN.md I.B's own rule — the naming
   literal "must be a live or inline-nested `token` row, because `add_membership`
   FKs both endpoints to real tokens" — presupposes a grammar that can *reference*
   that live/nested token. No such `Reference` field exists. That absence is the
   gap.
5. **The implementer concedes it.** README: "If this reading of 'must pre-exist'
   is wrong, the correct fix is almost certainly a new IR field on `DeclareRecord`
   for a PARENTS-less node's naming-literal reference, which is out of this
   module's authority to add unasked." That is an accurate self-diagnosis of a
   NEEDS-PATRICK gap.

**Nuance that narrows (not closes) the gap.** The FK on `members`/`member_of`
does force the group endpoint to be a real token, so a purely-prose crude
grouping declaration is *unwritable against this schema regardless* — PLAN.md I.B
already anticipated this. So requiring a real token is defensible; the defect is
the **channel** (overloading NOTATION) and the **missing reference field**, not
the pre-existence requirement itself. The companion "be inline-nested" reading —
mapped to the mixed PARENTS+MEMBERS node (`execute_structure`, where the placed
structure address is the naming literal) — **is faithful** to PLAN.md I.B ("what
is placed is the naming literal (via the structure path)") and is the correct,
well-tested realization of building the naming literal in-statement.

**Required action:** surface to Patrick. Do not lock the NOTATION-as-token_id
reading. The likely fix is a new `DeclareRecord` field carrying a `Reference`
(address or nested) to a PARENTS-absent node's naming literal, replacing the
NOTATION overload in `execute_grouping_only`. Until ruled, the pure-grouping path
should be treated as a flagged crude stand-in, not settled behaviour.

### Decision #2 — MEMBERS broadcast to every placed member for N>1 → **NEEDS-PATRICK** (minor, disclosed)

`execute_structure` (`declare_core.cpp:265-278`) hangs the full MEMBERS roster off
**every** successfully-placed member when a mixed node's PARENTS sets N>1, by
analogy with MEMBER_OF section-1's broadcast-to-all-N. Spec examples of a mixed
node are all N=1; nothing states MEMBERS semantics for a mixed node with N>1.

This is a **semantic** decision (it determines which membership edges get
written), not code structure, so it exceeds implementer latitude. Importing
MEMBER_OF's broadcast rule onto MEMBERS is exactly the "fill a spec gap with
familiar machinery" pattern the operating discipline warns against. The by-analogy
default is reasonable but unproven and untested. **Surface to Patrick.** Given the
construct is rare and unexercised, consider the more conservative alternative of
**rejecting** an N>1 mixed node as unspecified rather than silently broadcasting,
until Patrick rules. Severity: LOW.

### Decision #3 — ADDRESS-span `kNestedDeclare` colliding with the slot's own PARENTS entry → **NEEDS-PATRICK** (edge case, disclosed)

When ADDRESS segment `i` is `kNestedDeclare`, `resolve_placement`
(`declare_core.cpp:164-172`) mints the nested statement and returns its address as
`placed`. `validate_declare` still requires `PARENTS[i]` to carry its own ≥2
constituents, so `execute_structure` then also calls `mint(placed, …)` with
`PARENTS[i]`'s constituents — which, by SEE, is a no-op because the nested mint
already created that token. Net effect: **`PARENTS[i]`'s own composition is
silently dropped**; only the nested statement's composition is recorded.

This is a valid IR configuration (for N=1, `ADDRESS=[Nested(sub)]` +
`PARENTS=[[c0,c1]]` passes validation), and the spec never says what a token's
composition should be when two composition specs collide on one address. The
implementer takes "collapses to its address" literally and lets SEE pick the
first. The silent-drop is a **semantic** choice on an unspecified configuration.
**Surface to Patrick.** Untested. Reasonable alternatives (reject the collision;
or forbid PARENTS[i] when segment i is nested) are also Patrick's call. Severity:
LOW.

### Decision #4 — blank NOTATION stored as `""` → **FAITHFUL** (forced by the door signature)

`mint`'s `notation` parameter is `const std::string&` with no NULL channel
(`controller.h:135`). `notation_at` (`declare_core.cpp:113-119`) renders a blank
slot (short vector or nullopt entry) as `""`. NOTES: "a blank … field … reads
blank now." `""` is a faithful rendering of "reads blank"; there is no other
representation available through the door, and inventing a NULL channel is out of
scope. The test confirms the column is present-and-empty, not a placeholder
string. This is a code-structure/faithful fill, no Patrick input needed.

---

## 6. Itemized findings

**F-1 — Decision #1: NOTATION-as-token_id overload for the grouping-only naming
literal.**
Severity: **HIGH (must surface / must-fix-by-escalation).**
`declare_core.cpp:294-307`:
```cpp
if (node.notation.empty() || !node.notation[0].has_value()) { reject }
else { auto decoded = codec::decode_token_id(*node.notation[0]);
       if (!decoded.has_value() || !ctl.token_exists(*decoded)) { reject } ... }
```
Spec: PLAN.md I.B (naming literal must be a live/inline-nested token row; NOTATION
is prose); NOTES label-dual/construction-post (prose is a placeholder, not a
resolvable token); Hard constraints (only-follow — no lookup by prose). Why: a
prose field is reinterpreted as an address; genuine prose is unresolvable;
pre-existence contradicts the construction-post purpose; the IR has no field to
reference the naming literal. Narrowest correction: **do not resolve this
in-module.** Escalate to Patrick for an IR field (a `Reference` for a
PARENTS-absent node's naming literal); until then treat `execute_grouping_only`
as a flagged crude stand-in, and keep the reject-if-not-a-live-token behaviour
(it is the only FK-safe option) but document that it cannot accept real prose.

**F-2 — Decision #2: silent MEMBERS broadcast on an N>1 mixed node.**
Severity: LOW (surface). `declare_core.cpp:265-278`. Spec: PLAN.md I.B (N=1
examples only); operating discipline (no imported machinery). Narrowest
correction: surface to Patrick; optionally reject N>1 mixed nodes until ruled.

**F-3 — Decision #3: PARENTS[i] composition silently dropped under an ADDRESS
`kNestedDeclare` at slot i.** Severity: LOW (surface). `declare_core.cpp:164-172`
+ `246`. Spec: PLAN.md I.E (silent on the colliding own-PARENTS entry). Narrowest
correction: surface to Patrick; optionally reject the collision.

**F-4 — `author_membership_dedup` idempotency lives in the caller, not the door.**
Severity: LOW (design consistency). `declare_core.cpp:47-55`. Spec: NOTES Hard
constraints / schema header (SEE is a write-path property, mirrored across axes).
Correct within Agent 3's scope; recommend moving the probe into
`Controller::add_membership` before Agent 5/6 re-implement it. See §4.

**F-5 — Eager nested minting is not rolled back when the referencing point later
rejects.** Severity: LOW (observation, not a defect). A nested declare in an
ADDRESS slot or a PARENTS constituent list mints during resolution
(`resolve_placement`/`resolve_reference`), and that mint survives even if a later
sibling reference in the same member fails and the outer point is reported
un-ingested. "Per-member atomicity" therefore means "the member's **own** token
is not minted unless all its refs resolve", not "no writes at all occur." This is
consistent with the design: mints are per-`mint` atomic (no statement-level
transaction in the spec), and SEE makes a retry safe (the already-minted nested
product is a no-op). Worth a one-line note in the README's atomicity paragraph so
the boundary is explicit; no code change required.

**F-6 — Untested branches.** Severity: LOW (test completeness). No test for:
`@`-pin (`kPin`) placement; undeclared-hook rejection (`declare_core.cpp:158-162`);
AFTER/pending-seam rejection (`176-179`, `200-205`); MEMBER_OF section-2
`per_member`; a nested reference collapsing to >1 outcome (`67-72`); the mixed-node
MEMBERS skip-one-edge path (`271-276`); and Decisions #2/#3's own combinations.
Project rule is "tests on everything"; these are real branches. Add targeted
checks (they need no new fixtures beyond the existing seed atoms).

**No correctness defect found in the exercised paths.** Structure mint (ordered
refs, `mass=nullopt`, actual address returned), grouping both-directions, nesting
collapse, SEE idempotency (mint + membership), blank NOTATION/mass as the right
null-vs-empty channels, and rejection-without-throw all match spec and are
confirmed live.

---

## 7. Spec-conformance spot checks (confirmed)

- **§II.3 structure:** per PARENTS member, `mint(placed, notation, refs,
  std::nullopt)`; constituents carry `{addr, std::nullopt}` mass (LINK → SQL
  NULL). Returns the actual `placed` address. ✓ (`declare_core.cpp:233-252`)
- **§I.B / §II.6 grouping:** no own address for the group; realized as
  `add_membership` edges, both directions, synchronously, referenced by the
  naming-literal token. ✓
- **§I.B NOTATION:** positional-partial; blanks stored blank; derivation deferred.
  ✓
- **§I.E nesting:** relationship fields and ADDRESS slots may nest a full DECLARE;
  a nested product collapses to one address; >1-outcome nested reference rejected
  rather than arbitrarily picked. ✓ (`declare_core.cpp:66-77`)
- **§I.G return contract:** ordered actual addresses; grouping-only → naming-
  literal token; un-ingested+reason on reject; no throw for ordinary rejection,
  store errors still propagate via the controller. ✓
- **address-IS-identity / no-alias / no override:** `@`-pin resolves to the
  requested address (no override mechanism exists), reported as the actual
  address; no forwarding introduced. ✓
- **only-follow:** all reads are `token_exists` / `member_of` PK follows; no
  search introduced — except the NOTATION-as-token_id path (F-1), which is a
  decode+existence probe, not a search, but still an out-of-model channel.

---

## 8. Verdict

**PASS-WITH-FIXES.**

The module builds clean under `-Wall -Wextra`, passes 36/36 checks against a live
store, is scope-confined to `declare/`, carries no old-model leakage, and is
faithful to spec on every exercised path. It does not ship any silent semantic
violation — the two genuinely-underspecified areas (Decisions #1–#3) are honestly
flagged, and Decision #4 is a faithful, door-forced fill.

**Must-fix (by escalation, before the grouping-only path is relied upon):**

- **F-1 (Decision #1):** escalate to Patrick. The NOTATION-as-token_id resolution
  is a semantic overload of a prose field and a real IR/grammar gap; the fix is a
  new `DeclareRecord` naming-literal reference field, out of this module's
  authority. Do not lock the current reading.

**Should-fix / surface (Patrick's ruling, low stakes):** F-2 (Decision #2), F-3
(Decision #3).

**Recommended follow-ups (no Patrick input):** F-4 (move SEE into
`add_membership`), F-6 (cover the untested branches), F-5 (one-line atomicity
note).

None of these block landing the module as the crude-stage DECLARE core, provided
F-1 is carried to Patrick as an open decision rather than treated as settled.
