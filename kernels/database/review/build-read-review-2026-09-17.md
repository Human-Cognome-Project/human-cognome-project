# Adversarial review — READ core (raw radial), 2026-09-17

**Reviewer:** fresh independent adversary. No stake in the module passing.
**Under review (untouched working tree, no file modified):**
`engine/db_kernel/read/{read_core.h, read_core.cpp, read_core_test.cpp, README.md}`.
**Governing spec:** PLAN.md §I.C, §I.G, §II.4, Part V; NOTES.md "Relationship
model & type — firmed 2026-09-17", "READ — record-tier exploratory read"
(incl. Re-convergence RULED), "Build-phase rulings — firmed 2026-09-17".
**Foundation consumed (confirmed unmodified):** `command/command_ir.h`
(`ReadRecord`, `ReadDirection`, `ReadAxis`, `validate_read`), `controller/controller.h`
(`parents_of` / `children_of` / `members_of` / `member_of` / `token_exists`),
`codec/codec.h`.

---

## Verbatim build + test outcome

Built exactly per README §"Build and run the tests":

```
g++ -std=c++17 -O2 -Wall -Wextra \
    -I. -I../codec -I../command -I../controller -I"$(pg_config --includedir)" \
    read_core.cpp read_core_test.cpp \
    ../codec/codec.cpp ../command/command_ir.cpp ../command/span_planner.cpp \
    ../controller/controller.cpp \
    -L"$(pg_config --libdir)" -lpq -o read_core_test
```

Build: **clean, no warnings** under `-Wall -Wextra` (exit 0).

Run against the live local Postgres (`hcp3_core`, reset from `../schema/schema.sql`):

```
NOTICE:  drop cascades to 5 other objects
DETAIL:  drop cascades to table token / token_parent / token_child / members / member_of
ok   depth0: status ok
ok   depth0: rolled-up node only, nothing expanded (no members/parents in scope)
ok   radial depth1: status ok
ok   radial depth1: anchor + structure-down (p1,p2) + structure-up (top) + membership-up (grp), in fixed follow order
ok   structure-only forward: status ok
ok   structure-only forward: parents_of(top) = [c1, c2], no membership axis touched
ok   membership-only forward: status ok
ok   membership-only forward: members_of(grp) = [c1, c2], no structure axis touched
ok   structure reverse: status ok
ok   structure reverse (children_of): c1 -> top only, not parents_of(c1)
ok   membership reverse: status ok
ok   membership reverse (member_of): c1 -> grp only, not members_of(c1)
ok   depth1 from top: 3 nodes (top, c1, c2)
ok   depth2 from top: expands one more level per branch, LoD dial widens what is in scope
ok   no-dedup: shared node p1 (reachable via c1 AND c2) appears twice, not collapsed to one
ok   no-dedup: each p1 occurrence individually correct (depth 2, via structure)
ok   no-dedup: each p1 occurrence individually correct (depth 2, via structure)
ok   no-dedup: exactly two p1 occurrences located
ok   exclusions: naming c2 prunes c2 AND everything beneath it (p1-via-c2, p3) -- p1 now appears only once, via c1
ok   exclusions: a terminal-wildcard tree region ("C*") prunes the same branch as the specific-id exclusion, via pure prefix comparison (no store scan)
ok   nonexistent anchor: ok status, empty result
ok   wildcard anchor: flagged kAnchorWildcardDeferred, not resolved here
ok   wildcard anchor: no nodes produced
PASS read_core_test
```

23/23 checks pass (exit 0). The run is a genuine end-to-end exercise against a
real store built through the controller door, not a mock.

---

## Spec-conformance findings

### 1. Only-follow / never-search — CLEAN
Every traversal edge is a PK-follow: `follow_addresses` (read_core.cpp:95–116)
dispatches only to `parents_of` / `children_of` / `members_of` / `member_of`.
The anchor is reached via `token_exists` (read_core.cpp:155), the controller's
documented direct-address PK probe (controller.h:82–85), not a scan. The
wildcard-exclusion test `address_under_prefix` (read_core.cpp:27–39) is a pure
value comparison between two addresses already in hand — no store call inside it,
confirmed by inspection (no `ctl` parameter). No scan/search anywhere.

### 2. Direction — CLEAN
`active_follows` (read_core.cpp:68–87). Structure axis = `kParents`
(parents_of/children_of); membership axis = `kMembers`
(members_of/member_of); `kBoth` = both axes. Default (axis unset) = RADIAL =
all four follows. `reverse` is realised as an orientation *on a narrowed axis*
(forward→down, reverse→up), never as a peer value: `kParents` forward→kParent
(parents_of, structure-down), reverse→kChild (children_of, structure-up);
`kMembers` forward→members_of (membership-down), reverse→member_of
(membership-up). This matches NOTES.md L657–659 ("parents = structure-down,
members = membership-down … reverse orientation likewise selectable") and
PLAN.md §I.C exactly. `kBoth` forward = {parent, members}, reverse =
{child, memberOf} — the only reading that keeps `kBoth` (a narrowing) distinct
from RADIAL (no narrowing); correct.

### 3. Depth / LoD — CLEAN
`visit` (read_core.cpp:124–136) emits the node, then returns before expanding
iff `depth >= max_depth`. Depth 0 therefore yields the anchor alone (verified:
"depth0" check). Each rung expands exactly one level along the active follows.
"Depth governs members-in-scope" is realised as the natural consequence of the
shared hop budget (at depth 0 nothing beyond the anchor is exposed, membership
included; at depth ≥1 membership comes into scope iff a membership follow is
active). No invented per-axis depth dial. Matches PLAN.md §I.C, §II.4.

### 4. Exclusions — CLEAN
`excluded` (read_core.cpp:47–57): specific `token_id` (exact `==`) or
terminal-wildcard region (`address_under_prefix`). No property predicates.
Applied in `visit` before both emit and expansion, so a matched node is dropped
*and* its whole branch pruned (matches "prune named branches", PLAN.md §I.C;
NOTES.md Build-phase rulings). Wildcard-region semantics verified correct: `C*`
matches any address whose leading couplet's first char is `C` and everything
beneath it, by pure prefix compare; multi-element prefixes (`AA.B*`) correctly
require the address to lie strictly *under* the prefix (size > cut).

### 5. Linearized once-per-path, NO dedup — CLEAN
No visited-set anywhere; recursion is bounded solely by the depth budget
(read_core.cpp:130). A shared node reached via two distinct paths is emitted
twice at its own positions (the isolated no-dedup check on `p1`, reached through
both `c1` and `c2`, asserts `count == 2` with each occurrence individually
correct). Matches NOTES.md "Re-convergence — RULED: once per path, NO dedup"
and PLAN.md §I.C "repetition + position encode the structure losslessly."
Termination is by finite depth, not cycle detection — faithful to the ruling
(no-dedup extends to not collapsing a repeat).

### 6. OLD-MODEL-LEAK — CLEAN
No `TYPE`, no `token.type`, no `token_sibling_group`, no label-`CHILDREN`
notion, no type-lens over a shared table. `ReachedVia` names the new-model axes
(kMembers/kMemberOf) and the structure axis (kParent/kChild). The two axes are
distinct follows over distinct stores, exactly per "Relationship model & type —
firmed 2026-09-17". State clean.

### 7. Invention / over-reach — CLEAN (one OBSERVATION)
The traversal, result shape, and status set are all spec-traceable. `ReadNode`
carries `depth` and `via` in addition to `address` — see OBS-1; harmless
enrichment, not a semantic change.

### Scope confinement — CONFIRMED
`git status` shows `read/` as a new untracked directory; `command/`,
`controller/`, `codec/` carry no modifications from this module. It consumes
the foundation unmodified, as required.

---

## Wildcard ANCHOR deferral — ruling: **FAITHFUL-DEFERRAL**

`read()` returns `ReadStatus::kAnchorWildcardDeferred` with no nodes and no scan
when `op.anchor` is a terminal wildcard (read_core.cpp:143–145).

Resolving a wildcard anchor means enumerating which addresses are actually
*populated* under the prefix. The controller exposes no such primitive: every
read keys off an existing token's PK (controller.h:82–104); a partial/prefix
address is not a PK any live token row carries, and there is no edge to follow
from nothing. Enumerating a populated prefix region is a store range-scan, which
(a) only-follow forbids this module from inventing and (b) would require
touching `controller/`, outside this module's scope. This is precisely the shape
of problem PLAN.md defers elsewhere: MOVE_RECORD's wildcard/range *source* and
ADD_CONNECTION's wildcard group/elements are both "store-resolved (execution-time)"
by Agent 5 (PLAN.md §I.D, §II.5; NOTES.md Build-phase rulings — MOVE source
"unit count N is store-resolved (execution-time)"; ADD_CONNECTION "resolves to
its address range at execution"). NOTES.md *permits* a wildcard anchor as a
"nominal, tree-constrained read," but permission is not a mandate to resolve it
in the core; the resolution mechanism is the same deferred execution-layer
concern as the other wildcard-region cases. Returning a distinct status (a
STOP-and-handoff signal), rather than guessing or erroring, is the correct,
non-inventive handling. Deferring is correct.

---

## The 5 spec-silent decisions — adjudication

1. **RADIAL ignores `reverse`** — **FAITHFUL.** PLAN.md §I.C / NOTES.md L653–659
   frame RADIAL as "all directions outward" and `reverse` as an orientation "on
   those axes" once one is narrowed. With nothing narrowed, `reverse` has nothing
   to orient. Walking all four follows regardless of `reverse` is the literal
   reading; a "reversed radial" is not a spec concept and inventing one would be
   over-reach. (read_core.cpp:69–72.)

2. **Fixed follow order {parent, child, members, member_of}** — **CODE-STRUCTURE.**
   PLAN.md/NOTES.md specify per-axis semantics but no cross-axis linearization
   order. Determinism requires only *some* fixed order; combined with the
   controller's per-follow deterministic ordering (parents_of by ordinal, the
   other three sorted — controller.h:87–104, PLAN.md §II.0), the linearization is
   stable. Latitude confined to code structure. (read_core.cpp:68–87.)

3. **Nonexistent anchor → `kOk` with empty result** — **FAITHFUL.** Spec states
   no error taxonomy for READ (return contract §I.G is just "the linearized
   once-per-path return"). "Only-follow from the anchor" with no token to follow
   yields nothing; empty is the plain reading. No information is lost: an existing
   anchor always emits itself at depth 0, so a caller distinguishes "nonexistent"
   (0 nodes) from "exists, no neighbours" (≥1 node) at any depth. Inventing a
   distinct error status would be over-reach. (read_core.cpp:155–157.)

4. **Exclusions apply to the anchor too** — **FAITHFUL.** Spec states no anchor
   exemption; exclusions "prune named branches" uniformly. A self-excluding read
   yielding empty is degenerate but consistent, and exempting the anchor would be
   an invented special case ("no invented complication"). The uniform application
   is the non-inventive reading. (read_core.cpp:128, applied in `visit` to the
   anchor as to any node.) *Untested — see OBS-3.*

5. **Repeated constituents in `parents_of` not folded** — **FAITHFUL.**
   `follow_addresses` (read_core.cpp:99–105) unpacks every ParentEntry ordinal
   without deduping, so a constituent used at two ordinals produces two follows.
   This is consistent with the no-dedup return rule and mirrors the store (mint's
   LINK writes all ordinals; WIRE dedups only `token_child`). Confirmed reachable:
   the anti-alias floor is a count check (`list.size() < 2`, command_ir.cpp:118),
   not a distinctness check, so `[p1, p1]` is a valid declare. *Untested — see
   OBS-2.*

---

## Test-quality judgement — GOOD, with three coverage gaps

The suite is genuinely spec-exercising, not shallow: it uses exact-sequence
comparison (order + depth + via all asserted, `same_sequence`), isolates the
no-dedup assertion on a *genuinely shared* node reached via two paths (not a
mere repeat), covers depth 0 vs deeper LoD, reverse orientation on **each** axis
separately, and **both** specific-id and terminal-wildcard exclusion pruning
(asserting they prune the identical branch). Wildcard-anchor and nonexistent-
anchor are both asserted. This is well above a shallow smoke test.

Gaps (all OBSERVATION-level — the covered paths are correct; these are untested,
not wrong):

- **OBS-1 / return enrichment.** `ReadNode` exposes `depth` and `via` beyond the
  bare address linearization the spec's return contract names. This adds no
  semantic and loses nothing (position + repetition remain the load-bearing
  encoding); it is defensible as code-structure of the return type. Noted, not a
  finding.
- **OBS-2 / decision (5) untested.** No fixture token is minted with a *duplicate*
  constituent (`[p1, p1]`), so the "parents_of duplicates preserved" path the
  README/decision (5) describes is never exercised. The README even cites
  "mint's own c2 fixture" as the example, but this suite's `c2` has *distinct*
  constituents `{p1, p3}`. A one-line fixture (`mint(x, "x", {p1, p1})`) would
  close it.
- **OBS-3 / `kBoth` and anchor-exclusion untested.** `ReadAxis::kBoth` (both
  forward and reverse) is a distinct branch of `active_follows` with no test.
  Decision (4) (a literal exclusion matching the anchor → empty) is also
  unexercised. Both are cheap to add.

None of these block: the implemented behaviour is correct where tested, and the
untested branches are simple and inspected-correct above.

---

## Verdict: **PASS**

Clean build, 23/23 tests pass against the live store, faithful to spec on every
checked axis (only-follow, direction, depth/LoD, exclusions, no-dedup),
old-model-clean, scope confined to `read/`. The wildcard-anchor deferral is
correct and the five spec-silent decisions are each defensible (four FAITHFUL,
one CODE-STRUCTURE). The three coverage gaps (OBS-1/2/3) are observations for a
follow-up test pass, not defects — no behaviour under review is wrong. Nothing
requires Patrick's adjudication.
