# Adversarial review — READ core terminal-wildcard anchor (over `gather`)

**Reviewer:** fresh independent adversary. No stake in passing.
**Date:** 2026-09-18.
**Target (untouched working tree, not modified):**
`read/read_core.{h,cpp}`, `read/read_core_test.cpp`, `read/README.md`.
**Governing spec read in full:** `PLAN.md` §I.C, §II.4, §II.0 (gather);
`NOTES.md` "Build-phase rulings — firmed 2026-09-18" (Gather primitive;
READ terminal wildcards) and "READ — record-tier exploratory read"
(once-per-path, no dedup); `controller/controller.h` (gather + follow reads);
`controller/controller.cpp` gather/`wildcard_bounds` implementation.

## What changed (verified against HEAD)

`git diff HEAD -- read/read_core.cpp` shows exactly one functional change:
the wildcard-anchor branch, which previously returned
`ReadResult{ReadStatus::kAnchorWildcardDeferred, {}}`, now does

```cpp
if (is_wildcard(op.anchor)) {
  for (const codec::Address &member : ctl.gather(op.anchor)) {
    visit(ctl, member, 0, ReachedVia::kAnchor, op.depth, op.exclusions, follows,
          result.nodes);
  }
  return result;
}
```

plus the pure `const std::vector<ReachedVia> follows = active_follows(...)`
being hoisted above the branch so both paths share it. The concrete-anchor
path (`token_exists` guard + single `visit`) is **byte-identical** to HEAD.
`ReadStatus` reduced to `kOk` only. Change confined to `read/` (4 files).

## Independent rebuild + run (verbatim)

Built from a clean scratchpad copy of the command in `read/README.md`
(g++ -std=c++17 -O2 -Wall -Wextra, libpq), against a live local Postgres
(`pg_isready` → accepting connections). **Build: clean, zero warnings.**

```
NOTICE:  drop cascades to 5 other objects
DETAIL:  drop cascades to table token ... token_parent ... token_child ... members ... member_of
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
ok   wildcard anchor: status ok, not deferred
ok   wildcard anchor: gathers the A* trunk (p1, p2, p3) and concatenates each member's own radial read in gather order; c1 recurs across p1's and p2's reads, not deduplicated
ok   wildcard anchor, empty region: status ok (not a deferred/error status)
ok   wildcard anchor, empty region: no nodes produced
PASS read_core_test
EXIT=0
```

26/26 checks pass on an independently-built binary against disposable
`hcp3_core`.

## Scrutiny findings

### R1 — INFO — Cross-member no-dedup is genuinely proven, not just within-anchor
- **File:** `read/read_core_test.cpp:312-324`.
- **Evidence:** `A*` gathers `[p1, p2, p3]` (AA<AB<AC). The exact-sequence
  assertion locks the whole linearized return
  `{p1/0/kAnchor, c1/1/kChild, c2/1/kChild, p2/0/kAnchor, c1/1/kChild,
  p3/0/kAnchor, c2/1/kChild}`. `c1` appears under **p1** (pos 1) *and* under
  **p2** (pos 4) — two DIFFERENT gathered anchors — and `c2` appears under p1
  and under p3. This proves no-dedup holds ACROSS gathered members, exactly as
  point 1 required, not merely within one anchor. Verified by inspection:
  `read()` loops gather members appending into one `result.nodes`, and `visit`
  carries no visited-set, so cross-member dedup is impossible by construction.
- **Spec:** NOTES "Re-convergence — RULED: once per path, NO dedup"; PLAN
  §I.C "repetition + position encode the structure losslessly." Conformant.

### R2 — INFO — only-follow preserved
- **File:** `read/read_core.cpp:161-167`.
- **Evidence:** wildcard anchor resolves via `ctl.gather(op.anchor)` (the
  sanctioned contiguous PK-range walk — `controller.cpp:285-317`, a bounded
  `token_id >= low [AND < high] ORDER BY token_id` on the PK btree). Per-node
  reads remain `parents_of/children_of/members_of/member_of` PK follows via the
  unchanged `visit`. No reverse-search, no secondary index, no predicate on a
  non-key column introduced.
- **Spec:** NOTES "Gather primitive"; Hard constraints (only-follow). Conformant.

### R3 — INFO — Empty region → `kOk` + empty
- **File:** `read/read_core.cpp:161-166`; test `read_core_test.cpp:329-334`.
- **Evidence:** `Z*` gather returns empty; the for-loop body never executes;
  `result` stays default `{kOk, {}}`. Test asserts both `kOk` and empty nodes,
  explicitly "not a deferred/error status." Conformant (see Decision 2).

### R4 — INFO — `kAnchorWildcardDeferred` removal is clean; no regression
- **Evidence:** `grep -rn` across the whole `db_kernel` tree finds the symbol
  ONLY in the historical `review/build-read-review-2026-09-17.md` (a
  point-in-time record); it is absent from every `.cpp`/`.h`. `ReadStatus` in
  both `read_core.h:44-49` and `README.md:37` carries `kOk` only. Concrete-path
  regression coverage (depth0 existence + single visit, direction/depth,
  reverse orientation on each axis, the specific-id and wildcard-region
  exclusions, within-anchor no-dedup, nonexistent anchor) all still pass on the
  rebuilt binary. Diff confirms the concrete path is unchanged from HEAD.

### R5 — INFO — OLD-MODEL-LEAK clean
- **Evidence:** `grep -rni 'sibling|TYPE|CHILDREN|kind'` over `read/` returns
  only legitimate hits: `children_of`/`kChild` (structure-reverse follow names),
  and `ExecStatusType`/`PQresultStatus` (libpq). No `token_sibling_group`, no
  declared TYPE, no label-`CHILDREN` membership semantics, no stored kind.

### R6 — MINOR (test-completeness observation; not a defect) — wildcard anchor tested only under RADIAL / depth 1
- **File:** `read/read_core_test.cpp:312-334`.
- **Evidence:** the two wildcard-anchor cases exercise RADIAL direction at
  depth 1 (populated) and depth 2 (empty). There is no explicit case combining
  a wildcard anchor with a narrowed axis (`kParents`/`kMembers`/`kBoth` +
  reverse) or with exclusions. This is a coverage observation, NOT a bug: each
  gathered member is funnelled through the identical `visit()` call as a
  concrete anchor, with the same `follows`/`op.exclusions`/`op.depth`, and those
  dimensions are independently proven on the concrete path — so the combination
  is covered by construction. Also, the `A*` fixtures are all uppercase single
  couplets, so this test does not itself stress `COLLATE "C"` ordering; that is
  gather's own test's responsibility (build-1c/1d), outside READ's scope. No fix
  required; noted for completeness.

## Adjudication of the two flagged decisions

### Decision 1 — each gathered member is an INDEPENDENT depth-0 anchor, concatenated in gather order (not folded into one synthetic super-root with renumbered depths)
**Ruling: FAITHFUL.**
NOTES "READ — terminal wildcards permitted" and PLAN §I.C describe a wildcard
anchor as reading "a range / **full construct**" — a range of tokens, not a
single tree with one root. The plain reading is N independent raw-radial reads,
one per existing token gather finds, concatenated in gather's PK/address order,
each rooted at its own depth-0 `kAnchor`. Synthesizing an artificial super-root
and renumbering depths across members would be **invented complication**
(NOTES Hard constraints, "no invented complication") with no spec basis. The
implementation (`read_core.cpp:162-165`) does exactly the faithful thing, and
the test actively asserts it: every gathered member's leading entry is
`depth 0 / ReachedVia::kAnchor` (`read_core_test.cpp:317-320`). Cite:
NOTES "READ — terminal wildcards permitted"; PLAN §I.C; §V "READ's earlier
wildcard-anchor deferral is now fillable over [gather]."

### Decision 2 — empty gather result → `kOk` + empty nodes (not a distinct status)
**Ruling: FAITHFUL.**
NOTES/PLAN state no distinct status for an empty wildcard region. The
established precedent in this same module is the nonexistent concrete anchor,
which returns `kOk` + empty as "a well-formed read of nothing"
(`read_core.cpp:174-176`; NOTES only-follow "nothing to follow from"). An empty
gathered region is the same situation — the request is well-formed, the
traversal runs, it produces no nodes. The removed `kAnchorWildcardDeferred` was
a build-time placeholder for an unimplemented path, never a spec-mandated
result status; collapsing `ReadStatus` to `kOk` and treating empty-region as
`kOk`+empty is the natural, non-inventive reading consistent with the
nonexistent-anchor precedent. Cite: NOTES "READ — record-tier exploratory
read"; the nonexistent-anchor decision already documented in
`read/README.md` "Decisions made where the spec was silent."

## Verdict

**PASS.** No must-fix findings. Build is clean (zero warnings), 26/26 tests
pass on an independently-rebuilt binary against live Postgres, the change is
surgically confined to `read/`'s wildcard branch with the concrete path
provably untouched, `kAnchorWildcardDeferred` is cleanly removed tree-wide,
only-follow and no-dedup (including the required cross-member case) are proven,
and both flagged decisions are FAITHFUL to spec. R6 is a completeness
observation, not a defect.
