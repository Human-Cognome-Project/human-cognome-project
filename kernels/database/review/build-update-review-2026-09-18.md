# Adversarial review — UPDATE core (`update/`) — 2026-09-18

**Reviewer:** fresh, independent adversary. No stake in a pass.
**Target:** `engine/db_kernel/update/{update_core.h,update_core.cpp,update_core_test.cpp,README.md}`
**Method:** full read of the module + governing spec (PLAN.md §I.D/§I.F/§II.0/§II.5/§II.6, Part V; NOTES.md "Relationship model & type", "Build-phase rulings 2026-09-17/18", "UPDATE — record-tier ops"); full read of the consumed foundation (`command/command_ir.{h,cpp}`, `command/span_planner.{h,cpp}`, `controller/controller.{h,cpp}`, `schema/schema.sql`); **independent rebuild** of `update_core_test` in a scratchpad and run vs a disposable live `hcp3_core` (PostgreSQL 16.15); plus **7 additional adversary probes** for cases the provided test omits.

## Verdict: **PASS-WITH-FIXES**

Must-fix ids: **[U1]** (double-wildcard ADD_CONNECTION resolves an unruled seam by executing a cross product).

Everything else is correct, data-safe, faithful to spec, and read-back-verified. The module compiles clean (`-Wall -Wextra`, zero warnings), does not touch anything outside `update/`, and its provided test's 58 checks all pass on an independent build. [U1] is not a data-integrity defect — the behaviour is additive/idempotent/reversible — but it unilaterally resolves an open seam contrary to the plan's stated "flag, don't resolve" discipline and the codebase's own reject-until-ruled precedent (command ruling #2).

---

## Scope / isolation (confirmed clean)

- `git diff --stat HEAD -- command/ controller/ schema/ codec/` → **no diff** for command/controller/schema (codec appears only as a staged add). The foundation the UPDATE core consumes is untouched.
- `update/` is a new, self-contained directory. It depends only on `../codec`, `../command`, `../controller`; it writes nothing into `../schema`, `../declare`, `../read`, or the ingestion runtime. **only-follow / address-IS-identity respected; no OLD-MODEL leak** (no `token_sibling_group`, no `type`, no `groups_of`; uses `members`/`member_of`, `gather`, `rekey`, `delete_token`/`delete_pair` exactly as the rebased door exposes them).

## Independent build + run (verbatim)

Built in scratchpad with the README's own command line (sources from the working tree, unmodified), run against a freshly reset `hcp3_core`:

```
NOTICE:  drop cascades to 5 other objects
DETAIL:  drop cascades to table token / token_parent / token_child / members / member_of
ok   explicit MOVE: one outcome
ok   explicit MOVE: reported moved
ok   explicit MOVE: outcome reports from/to
ok   explicit MOVE: old address no longer exists
ok   explicit MOVE: new address exists
ok   explicit MOVE: structure cascade -- c1's parent[0] repointed to new address
ok   explicit MOVE: structure cascade -- new address's children_of includes c1
ok   explicit MOVE: no orphan -- old address's children_of is gone (address doesn't exist)
ok   explicit MOVE: membership cascade -- new address's member_of includes grp
ok   explicit MOVE: membership cascade -- grp's members_of includes new address
ok   wildcard MOVE: gather resolved N=3 outcomes
ok   wildcard MOVE: every resolved source moved
ok   wildcard MOVE: old trunk is empty afterward
ok   wildcard MOVE: destination sequentially filled FA/FB/FC
ok   wildcard MOVE: gather order paired with destination fill order
ok   cover-N mismatch: whole MOVE rejected as one outcome
ok   cover-N mismatch: reason names the cover-N failure
ok   cover-N mismatch: nothing moved, sources untouched
ok   cover-N mismatch: destination untouched
ok   MOVE destination nested-declare: rejected as one outcome
ok   MOVE destination nested-declare: reason names the nested-declare segment
ok   MOVE destination nested-declare: nothing minted
ok   MOVE destination nested-declare: source untouched
ok   MOVE destination undeclared hook: rejected as one outcome
ok   MOVE destination undeclared hook: reason names the undeclared-hook segment
ok   MOVE destination undeclared hook: source untouched
ok   MOVE destination collision: rejected
ok   MOVE destination collision: both tokens untouched
ok   ADD_CONNECTION explicit: pair added
ok   ADD_CONNECTION explicit: re-add is a clean success (idempotent)
ok   ADD_CONNECTION explicit: no duplicate edge (member_of)
ok   ADD_CONNECTION explicit: no duplicate edge (members_of, reciprocal)
ok   ADD_CONNECTION wildcard elements: 3 pairs enumerated
ok   ADD_CONNECTION wildcard elements: every pair added
ok   ADD_CONNECTION wildcard elements: group's members_of includes all three
ok   ADD_CONNECTION wildcard group: 2 pairs enumerated
ok   ADD_CONNECTION wildcard group: every pair added
ok   ADD_CONNECTION wildcard group: member's member_of includes both groups
ok   ADD_CONNECTION missing element: rejected, not thrown
ok   ADD_CONNECTION missing element: nothing written
ok   ADD_CONNECTION missing group: whole statement rejected, not thrown
ok   ADD_CONNECTION missing group: nothing written
ok   DELETE_RECORD mismatched confirm: rejected
ok   DELETE_RECORD mismatched confirm: reason names the mismatch
ok   DELETE_RECORD mismatched confirm: target untouched
ok   DELETE_RECORD correct confirm: executed
ok   DELETE_RECORD correct confirm: token gone
ok   DELETE_RECORD reject-on-referenced: rejected, not thrown
ok   DELETE_RECORD reject-on-referenced: reason names G10
ok   DELETE_RECORD reject-on-referenced: token still present
ok   DELETE_RECORD nonexistent target: rejected, not thrown
ok   DELETE_CONNECTION mismatched confirm: rejected
ok   DELETE_CONNECTION mismatched confirm: edge untouched
ok   DELETE_CONNECTION correct confirm: executed
ok   DELETE_CONNECTION correct confirm: member_of edge gone
ok   DELETE_CONNECTION correct confirm: members_of reciprocal gone too
ok   DELETE_CONNECTION no such edge: rejected
ok   DELETE_CONNECTION no such edge: reason names the missing edge
PASS update_core_test
```

**58 ok, 0 FAIL, exit 0.** Matches the README's "58 checks pass" claim exactly.

**Test quality — genuinely read-back-verified, not return-code theatre.** Mutations are checked against the live store through the door: MOVE confirms `token_exists(old)` is false and `token_exists(new)` true, `parents_of` repointed to the new address, `children_of(new)` includes the composite, `children_of(old)` gone (no orphan), and `member_of`/`members_of` repointed **both directions**. ADD_CONNECTION confirms `count_of(member_of)==1` after a re-add (no duplicate) and the reciprocal. DELETE_CONNECTION confirms both the `member_of` row and its `members` reciprocal are gone. DELETE_RECORD confirms the token row disappears (or survives, on reject). This is the correct discipline for ops that mutate/delete data.

## Additional adversary probes (cases the provided test does not cover)

Independently written, built, and run vs a fresh `hcp3_core`. Verbatim:

```
=== P1: ADD_CONNECTION both-sides wildcard -> cross product (Decision 2) ===
outcomes=6 (expect 6 = 2 groups x 3 members)
all added=1
NA member_of RA=1 RB=1 ; NC member_of RA=1 RB=1
=> both-wildcard EXECUTES a full cross product of 6 edges

=== P2: ADD_CONNECTION wildcard side matching nothing -> silent empty result ===
group=Z* (empty) outcomes=0 (silent no-op; no rejection outcome)
elements=Z* (empty) outcomes=0

=== P3: MOVE kRange source (never exercised by provided test) ===
outcomes=3 (expect 3)
KA/KB/KC gone=111 ; LA/LB/LC exist=111

=== P4: MOVE mixed selectors (explicit + prefix) -> selector-order concat ===
outcomes=3 (expect 3)
  [0] from=WA to=XA moved=1
  [1] from=YA to=XB moved=1
  [2] from=YB to=XC moved=1

=== P5: MOVE explicit-nonexistent source still occupies its slot ===
outcomes=2 (expect 2)
  [0] from=PA to=QA moved=1 reason=''
  [1] from=PB to=QB moved=0 reason='source token does not exist'
PA moved to QA? exists(QA)=1 exists(PA)=0 ; QB minted spuriously? exists(QB)=0

=== P6: MOVE destination overlaps source (forward-shift a trunk) ===
outcomes=3
  [0] from=SA to=SB moved=0 reason='rekey: new_id already exists (would merge identities)'
  [1] from=SB to=SC moved=0 reason='rekey: new_id already exists (would merge identities)'
  [2] from=SC to=SD moved=1 reason=''
post-state SA=1 SB=1 SC=0 SD=1
=> forward-overlapping bulk MOVE partially applies (collision per-token)

=== P7: DELETE_CONNECTION with reversed a/b roles ===
reversed-role delete deleted=0 reason='no such membership edge exists between the pair'
edge still intact? VA member_of VB=1 (expect 1 = safe, not wrongly deleted)
```

These confirm: the kRange source path works (untested by the harness); mixed-selector concatenation is selector-order-correct; a nonexistent explicit source correctly occupies its slot without spuriously minting the destination; reversed DELETE_CONNECTION roles reject safely (no wrong deletion); and the both-wildcard cross product executes live (finding [U1]).

---

## Findings

### [U1] — MUST-FIX (medium) — double-wildcard ADD_CONNECTION resolves an open seam by executing a full cross product
- **file:** `update_core.cpp:225-241` (`add_connection`, the nested `expand_side` × `expand_side` loops); documented `README.md` "Wildcard resolution and the cross-product decision".
- **evidence:** Probe P1 — `group=R*` (2 tokens) × `elements=[N*]` (3 tokens) writes **6** membership edges, all sides resolved independently and enumerated.
- **spec:** NOTES.md "ADD_CONNECTION — terminal wildcards, either side" states exactly **two single-wildcard directions** ("add all [wildcard members] to this group", or "add this member to all [wildcard groups]"). The **both-sides-wildcard** case is unstated. PLAN.md Standing/Discipline: "where the spec defers or is silent, this plan flags a seam and does **not** resolve it." Executing a cross product *is* resolving it. The codebase's own precedent for an analogous unspecified broadcast is **reject-until-ruled** (`command_ir.cpp:116-127`, ruling #2: a mixed PARENTS+MEMBERS node with N>1 "fails fast rather than guessing a semantic").
- **why it matters:** additive and idempotent, so no corruption — but the magnitude is |groups|×|members|, a plausibly large, analyst-unintended write, and it ships an unruled semantic as live behaviour. README-flagging documents the choice but does not un-resolve the seam.
- **fix:** reject the both-sides-wildcard case until ruled (mirror ruling #2 — detect `is_wildcard(op.group) && any element is_wildcard`, return a single clean rejection outcome), **or** obtain Patrick's explicit blessing of the cross-product semantic and record it as a firmed ruling. Either closes the discipline gap.

### [U2] — LOW / informational — forward-overlapping bulk MOVE partially applies
- **file:** `update_core.cpp:113-163` (per-token rekey in ascending gather order).
- **evidence:** Probe P6 — MOVE `S*`→`FROM:SB` (sources SA,SB,SC in PK order; destinations SB,SC,SD): SA→SB and SB→SC both hit `rekey: new_id already exists`, only SC→SD succeeds. Post-state SA,SB survive, SC gone, SD created.
- **assessment:** **not a defect, data-safe** — each rekey is atomic, the collision guard is `rekey`'s own identity-merge protection, failures are per-token with clear reasons, no corruption. The true bulk-correction case (relocate a branch to a *fresh* trunk, P3/P4) works cleanly. The overlap only bites a reshuffle-in-place where the destination interleaves the source, and processing in ascending order guarantees each slot collides with the next unmoved source. NOTES.md "Arraying is universal" itself notes ordering is semantic ("a MOVE targeting a slot an earlier MOVE placed works... because they execute in order") — here ascending order is the wrong order for a forward shift.
- **fix (optional):** none required for correctness; worth a one-line README/NOTES caveat that a self-overlapping forward MOVE partially applies, or (future) reverse-order processing when destination > source.

### [U3] — LOW / informational — a wildcard side matching nothing returns a silent empty result
- **file:** `update_core.cpp:225-241`.
- **evidence:** Probe P2 — `group=Z*` (no tokens under Z) → `outcomes.size()==0`, no rejection outcome; likewise an empty-matching `elements` wildcard.
- **assessment:** defensible (a gather over an empty trunk legitimately has nothing to connect), but note the asymmetry with an *explicit* group-not-found, which is a whole-statement rejection (`update_core.cpp:212-217`). A typo'd wildcard prefix silently no-ops. Not a correctness bug; flag for API-surface awareness.

### [U4] — INFORMATIONAL — DELETE_RECORD is effectively restricted to fully-isolated tokens
- **file:** `controller.cpp:565-574` (`delete_token`, bare DELETE) + `schema.sql:147-263` (every relationship column FK-references `token(token_id)`, no `ON DELETE CASCADE`).
- **evidence:** because `token_parent.token_id`, `token_child.child_token_id`, `members.*`, `member_of.*` all FK into `token`, a token that is a **composite** (has its own `token_parent`/`token_child` rows), a **constituent**, a **member**, or a **group** cannot be deleted — its own edges reference it. Only a token with *no* structure and *no* membership is deletable.
- **assessment:** **correct** under the held G10 policy (PLAN.md G10 / Part V: "reject-on-referenced, no invented cascade"). Worth stating explicitly because "reject-on-referenced" reads like "referenced by *others*", whereas a composite is also blocked by its *own* structure rows, and there is no structure-edge-deletion primitive at the record tier (spec: structure removal is via DELETE_RECORD only) — so a wired literal is presently undeletable. This is Patrick's open seam, not a module bug; the module correctly declines to invent a teardown.

### No-issue confirmations
- MOVE mint-bearing rejection (`kNestedDeclare`/`kUndeclared`) runs **before** source resolution or planning (`update_core.cpp:77-91`) → no gather cost, no partial mutation. Verified (harness + code order).
- MOVE cover-N is re-enforced at execution for wildcard/range via `plan(destination, resolved_N)` (`update_core.cpp:101-108`); all-explicit is additionally checked at IR (`command_ir.cpp:328-342`). Verified (cover-N mismatch test; P3).
- rekey cascade repoints **all four** stores both directions; no dangling old id (harness read-back).
- `delete_token` FK failure does **not** poison the connection: it is a bare autocommit DELETE (no open transaction to abort), confirmed by the test continuing cleanly through the nonexistent-target case immediately after the reject-on-referenced case.
- DELETE gates are literal: confirmation is a full field-for-field target match, no `bool confirm` shortcut anywhere; mismatch rejects with **no DB touch** (harness read-back: target/edge untouched).

---

## The four flagged decisions — adjudicated

**1. Same-address MOVE = no-op success (skips rekey to dodge the merge-guard).** — **FAITHFUL (code-structure).**
Spec is silent. A token asked to relocate to the address it already occupies has trivially satisfied the relocation; reporting `moved=true` is the honest reading, and calling `rekey(x,x)` would spuriously trip the identity-merge guard (which exists to catch a genuine two-identity merge, `controller.cpp:510-516`). Data-safe, no semantic invention. Sound.

**2. Both-sides-wildcard ADD_CONNECTION → full cross product.** — **NEEDS-PATRICK.**
NOTES states only the two single-wildcard-side directions; the double-wildcard case is unstated. Executing the cross product resolves an open seam, contrary to PLAN's "flag, don't resolve" discipline and inconsistent with command ruling #2's reject-until-ruled treatment of the analogous unspecified broadcast. It is *not* an over-reach in mechanism (the cross product is the natural generalization, no imported machinery), but shipping it live is the over-reach. **Recommend: reject-until-ruled, or Patrick's explicit blessing.** This is finding [U1].

**3. DELETE_CONNECTION a/b read as (member, group).** — **FAITHFUL (code-structure).**
`command_ir.h` leaves the roles undocumented. Reading `a=member, b=group` matches the existing `add_membership(member, group)` / `delete_pair(member, group)` parameter order — the one reasonable reading, no new convention. The pre-delete `member_of(a) contains b` check both keeps `deleted` truthful and *validates the direction*: probe P7 shows a reversed-role call rejects ("no such membership edge") and leaves the edge intact — it cannot delete the wrong thing. Sound.

**4. G10 FK detection by matching the controller's error STRING ("foreign key"/"violates").** — **CODE-STRUCTURE; acceptable flagged interim (not a must-fix), improvement recommended.**
The controller exposes no structured SQLSTATE (`controller.cpp:168-174` formats a message only), so string-matching is the only signal available today. It is **adequately scoped for `delete_token` specifically**: the only constraint that can fire on that bare DELETE is an FK from a referencing table, so a match cannot be a false positive here; genuine faults (connection loss, etc.) lack both substrings and are correctly **re-thrown** (`update_core.cpp:295`); there is no transaction to poison. Two caveats: (a) the `"violates"` substring is over-broad in general (would also catch CHECK/UNIQUE/NOT-NULL) — harmless here only because those cannot arise on this delete, so flag against reuse of `looks_like_fk_violation` for other ops; (b) the **more correct / more only-follow-idiomatic** design is a **pre-delete only-follow reference probe** (check the four stores before deleting) — it needs no message text, gives a precise reason, and matches the "only-follow, never search" spirit — or switching to SQLSTATE `23503` once the controller surfaces it. Recommended, not blocking.

---

*Reviewer note: no working-tree file was modified. This review file is the sole artifact written. The independent test binary and probes were built and run only in the session scratchpad.*
