# Adversarial build review 2 — conformed DECLARE core (`declare/`)

**Reviewer role:** fresh, independent adversary (verifying the conform against the
now-RECORDED grouping ruling). No stake in it passing.
**Date:** 2026-09-17
**Predecessor:** `review/build-declare-review-2026-09-17.md` (this is the re-review
of the fixes to that review's #1 NEEDS-PATRICK and the F-4 door-idempotency
finding).
**Tree:** `dbkernel-design-checkpoint`, conform commit `722ad2c`
("conform to grouping-node ruling + door idempotency"). Reviewed the working tree
untouched; nothing modified. Built + ran from the session scratchpad only.

**Read in full:** `declare/declare_core.{h,cpp}`, `declare/declare_core_test.cpp`,
`declare/README.md`; the updated `command/command_ir.cpp` (`validate_declare`) and
`command/command_ir.h`; `controller/controller.cpp` (`add_membership`) and
`controller/controller.h`; the recorded ruling in `NOTES.md` ("Grouping-node
naming literal (label DECLARE)") and `PLAN.md` I.B / II.3.

---

## 1. Verbatim build & test outcome

**Build** (exact README invocation; `-Wall -Wextra`):

```
BUILD EXIT: 0
```

Clean compile, no warnings.

**Run** (live local Postgres; `hcp3_core` present; schema reset+reapplied):
57/57 checks `ok`, ending `PASS declare_core_test` / `RUN EXIT: 0`. Full log:

```
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
ok   use-provided-ID: returns the referenced naming-literal token_id, not a freshly placed address
ok   use-provided-ID: members written (downward)
ok   use-provided-ID: member_of reciprocal written (upward)
ok   use-provided-ID: an absent referenced token does not throw
ok   use-provided-ID: an absent referenced token is rejected, not fabricated
ok   use-provided-ID via inline nested mint: the naming literal is the nested statement's own minted address
ok   use-provided-ID via inline nested mint: MEMBERS written against it
ok   manager-placed (ADDRESS omitted): does not throw
ok   manager-placed (ADDRESS omitted): rejected, not guessed at
ok   @-pin: the actual assigned address is the pinned address (no override mechanism exists yet)
ok   undeclared hook: still N outcomes
ok   undeclared hook: the sibling with a concrete slot still ingests
ok   undeclared hook: the undeclared slot's point is rejected un-ingested
ok   pending seam: an unresolved AFTER origin does not throw
ok   pending seam: rejected pending the deferred trunk map (G5), not guessed at
ok   per-member MEMBER_OF: both members ingest
ok   per-member MEMBER_OF: member 0 gets only the shared group
ok   per-member MEMBER_OF: member 1 gets the shared group AND its own sparse entry
ok   >1-outcome nested reference: does not throw
ok   >1-outcome nested reference: rejected rather than picking one address arbitrarily
ok   eager nested mint is NOT rolled back when the outer point later rejects (README.md 'Atomicity boundary') ...
ok   the outer point's own address was never minted, only rejected
ok   membership re-add: the door itself no-ops, no exception
ok   membership re-add: no duplicate edge recorded
PASS declare_core_test
```

Genuine live pass. 21 net-new checks over review-1's 36 — the previously-untested
branches (F-6) are now covered.

---

## 2. Verification against the recorded spec

### 2.1 NOTATION-as-token_id overload DELETED (prior #1) — CONFIRMED

- `grep decode_token_id declare/*.cpp *.h` → **only** the test helper
  `A()` (`declare_core_test.cpp:50`). **No prose→token lookup remains in the
  core.** The old `execute_grouping_only` function is gone
  (`grep execute_grouping_only` → none).
- `notation` in the core is used only by `notation_at` for surface-form storage
  (`declare_core.cpp:91-97,224`); it plays **no identity role**. Matches the
  recorded ruling: "NOTATION is NOT the handle — optional and human-facing only."
- **use-provided-ID** (`execute_grouping_use_provided_id`,
  `declare_core.cpp:272-338`): resolves `ADDRESS` via `command::plan(*node.address,
  1)` → the shared `resolve_placement` helper → `Controller::token_exists`
  (`303`). Absent → un-ingested with reason "…does not reference an existing
  token" (`304-308`), not thrown, not fabricated. Tested (existing-token,
  absent-token, inline-nested-mint variants). ✓
- **mint form** (`PARENTS` present): dispatched to `execute_structure`
  (`declare_core.cpp:377`), which mints the naming literal from its `PARENTS`
  composition, placed at the target `ADDRESS`, then authors `MEMBERS` against that
  placed address (`245-258`). Tested (mixed-node grouping). ✓

### 2.2 `author_membership_dedup` DROPPED; door idempotent — CONFIRMED

- `grep author_membership_dedup declare/` → **none**. Both call sites now call
  `ctl.add_membership(...)` directly (`declare_core.cpp:229` structure/MEMBER_OF,
  `254` mixed-node MEMBERS, `330`/`333` use-provided-ID).
- `Controller::add_membership` (`controller.cpp:299-334`) is SEE-idempotent:
  `INSERT ... ON CONFLICT DO NOTHING` on **both** `member_of` (upward) and
  `members` (downward reciprocal), inside one BEGIN/COMMIT with ROLLBACK on error.
  The finding from review-1 (F-4: idempotency belongs in the door, not the caller)
  is resolved exactly as recommended.
- **End-to-end re-add no-op** is proven two ways: (a) the SEE-idempotency
  re-declare test (`member_of(BA).size()==1` after re-declaring the already-linked
  edge); (b) a **direct door** re-add test (`declare_core_test.cpp:515-530`):
  `add_membership(p1, grp_fixture)` twice, no throw, exactly one edge recorded. ✓

### 2.3 Rulings #2 (N>1 mixed) and #3 (nested-ADDRESS/PARENTS collision) enforced upstream — CONFIRMED

- **#2:** `validate_declare` rejects a mixed `PARENTS`+`MEMBERS` node with
  `parents->size() != 1` (`command_ir.cpp:116-127`), citing the ruling.
- **#3:** `validate_declare` rejects any `kNestedDeclare` ADDRESS segment when
  `has_parents` (`command_ir.cpp:221-238`), citing the silent-drop reasoning.
- **The core makes no unsafe assumption these shapes can reach it.** `execute()`
  re-runs `validate_declare` on entry (`declare_core.cpp:343`), including on every
  recursive nested call — so even a caller that bypasses `command::` is gated
  (defense in depth). Beyond that gate:
  - The mixed-node `MEMBERS` loop is written **generically over
    `result.outcomes`** (`245-258`) and its comment states "N is always 1 here by
    validation, but nothing below assumes it." If an N>1 mixed node somehow
    arrived, it would broadcast (the old #2 behaviour) — but it cannot arrive
    (validated + re-validated). Benign; no live path reaches it.
  - `resolve_placement`'s `kNestedDeclare` branch (`142-150`) is exercised only by
    the **legal** case (a `PARENTS`-absent use-provided-ID node whose ADDRESS is an
    inline nested mint); the colliding `PARENTS`-present case is rejected upstream
    before dispatch. Correct reliance on the upstream check, no re-implementation.

### 2.4 Tests meaningful/green · OLD-MODEL-LEAK · scope — CONFIRMED

- **Tests meaningful:** all assertions read back live store state through the door
  (`parents_of`, `attributes_of`, `members_of`, `member_of`), not just the return
  struct. New coverage is substantive: use-provided-ID (present/absent/inline-
  nested), manager-placed rejection, `@`-pin, undeclared-hook (with a surviving
  sibling), pending-seam, per-member `MEMBER_OF` sparse entry, `>1`-outcome nested
  reference rejection, the atomicity-boundary token-existence checks, and the
  door-level re-add no-op. Green: 57/57.
- **OLD-MODEL-LEAK: CLEAN.** No `TYPE`/`token.type`, no `sibling`/
  `token_sibling_group`, no `CHILDREN`-as-field, no `add_group_membership`,
  no `token_child` handling in the core. (The only `type` token in the module is
  the citation of the NOTES section title "Relationship model & type" in a
  comment.) Validation is purely structural.
- **Scope confined to `declare/`.** `declare_core.cpp` includes only its own
  header + `span_planner.h`; the header includes `codec`, `command_ir`,
  `controller`. The module writes nothing into `command/`, `controller/`,
  `schema/`, `read/`, or ingestion. (The conform commit's edits to `command/` and
  `controller/` are the foundation changes the ruling required — Agent 1/2
  territory, separately reviewed in `build-command-review-4` /
  `build-schema-review` — which this module merely consumes.)

---

## 3. Adjudications

### 3.1 STOP-flag — mint form, `ADDRESS` omitted ("manager-placed") rejected at execution → **FAITHFUL-DEFERRAL**

`execute()` rejects a mixed `PARENTS`+`MEMBERS` node with no `ADDRESS`
(`declare_core.cpp:366-376`), un-ingested, with a reason naming the gap
("…no next-slot/address-assignment mechanism exists yet (PLAN.md Part V,
G4/G5); supply a target ADDRESS … or … use-provided-ID"). Not thrown, not
guessed. Tested (`manager-placed (ADDRESS omitted): rejected, not guessed at`).

**Ruling: FAITHFUL-DEFERRAL — correct not to invent a trunk/placement.** The
recorded ruling does call for "else manager-placed," but manager-placement
*requires* the address-assignment mechanism that is explicitly OPEN: G4 (next-slot
mechanism — analyst-supplied-start vs per-trunk cursor, undecided per NOTES "Open
decisions") and G5 (trunk→kind map boundaries, deliberately unfixed). Choosing a
trunk/kind/cursor here would be precisely the "no invented complication" the
module is bound against. Rejecting cleanly with a gap-naming reason is consistent
with every established precedent in this codebase:
- the span planner reports `kPendingSeam` for `AFTER` and returns `nullopt` from
  `successor()` at a depth boundary rather than growing a ring (G5);
- `validate_declare` treats `kPendingSeam` as *not* a validation failure, and the
  DECLARE core then rejects the pending slot at execution (the tested "pending
  seam" case) — the same IR-valid/exec-rejected split;
- ingestion's inert `extrapolate_address` hook assigns nothing and reports the
  point un-ingested, never force-fitting;
- MOVE_RECORD's cover-N is deferred from IR to the UPDATE core for the same
  "store-dependent, not knowable now" reason.

**The IR-accepts / core-rejects split is an acceptable "valid-but-not-yet-
executable" state, not a defect.** The shape *is* a legal grammar form per the
recorded ruling (the mint form MAY omit ADDRESS), so rejecting it at
`validate_declare` would wrongly encode "manager-placement is forbidden" instead
of "not yet implemented." The capability gap belongs at execution time, exactly
where the module puts it — and it mirrors the already-blessed `kPendingSeam`
layering. The README flags it as the module's one significant open point and notes
it becomes a small additive follow-up (a manager-side allocator this module calls
into) once G4/G5 land. No code change warranted now.

### 3.2 Atomicity boundary — eagerly-minted nested product not rolled back on a later reject → **FAITHFUL / ACCEPTABLE**

A nested reference is minted eagerly when resolved (`resolve_reference` →
`execute`); if a *later* reference in the same member's list then fails, the outer
member is reported un-ingested but the already-minted nested product persists.
Documented (README "Atomicity boundary: eager nested mint is not rolled back") and
tested directly (`>1-outcome nested reference` case: `MA`/`MB` exist afterward,
`MC` — the rejected outer point — does not).

**Ruling: FAITHFUL / ACCEPTABLE — not a concern.**
- **SEE idempotency makes it harmless and reusable.** The minted sub-structure is
  a validly-constructed token in its own right (address-IS-identity); on a retry
  with the missing reference fixed it is a SEE no-op and the outer point mints, so
  the net post-retry state is correct. It is not orphaned garbage — an
  unreferenced token is inert until referenced, consistent with "nothing holds
  addresses / relocation is free; connection is the invariant."
- **Per-point independence is spec'd** (PLAN.md I.G: "a rejected point →
  un-ingested," singular; array points independent), and the spec's atomicity unit
  is the **per-`mint` transaction** (§II.6 makes each reciprocal synchronous
  *within its authoring op*), **not** a statement-spanning transaction. Eager
  nested minting sits exactly on that spec'd boundary.
- **Adding a whole-statement rollback would be invented machinery** — a
  transaction spanning the recursive resolution is neither called for by spec nor
  cheap, and would breach "no anticipatory abstraction." The correct move is to
  document + test the boundary, which the conform does.

Minor: "per-member atomicity" is therefore best read as "the member's *own* token
is never minted unless all its refs resolve," not "no writes at all occur for that
member" — the README now says this explicitly (the review-1 F-5 observation is
resolved).

---

## 4. Residual findings

None rising to must-fix. Two benign observations, no action required:

- **O-1 (benign):** the mixed-node `MEMBERS` loop remains generic over N although
  validation forces N=1; it is unreachable for N>1 and the comment says so. An
  `assert(result.outcomes.size() == 1)` would document the invariant, but the
  generic loop is harmless. Not a leak of the old #2 behaviour (the shape cannot
  arrive).
- **O-2 (tracking, already flagged by the author):** manager-placed mint remains
  unimplemented pending G4/G5 — the module's one open capability gap, correctly
  deferred (§3.1). Carry it as a G4/G5 follow-up, not a bug.

Review-1's must-fix (#1) and its F-4 recommendation are both fully resolved;
F-6 (untested branches) is resolved by the 21 new checks; F-5 (atomicity wording)
is resolved in the README.

---

## 5. Verdict

**PASS.**

The conform faithfully implements the recorded grouping-node ruling: the
NOTATION-as-token_id overload is deleted and replaced by an address-based
use-provided-ID path plus the mint-form structure path; membership idempotency now
lives in the door (`ON CONFLICT DO NOTHING`, both directions) with the caller-side
guard removed; rulings #2 and #3 are enforced upstream in `validate_declare` and
the core safely relies on them (with a re-validation gate for defense in depth).
Build is clean under `-Wall -Wextra`; 57/57 live checks pass; no old-model
leakage; scope confined to `declare/`. The two adjudicated behaviours — the
manager-placed rejection and the eager-nested-mint atomicity boundary — are both
correct, spec-consistent, documented, and tested. No must-fix items.
