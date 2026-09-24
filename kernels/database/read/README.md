# read

The READ core, raw radial (PLAN.md II.4; NOTES.md "READ -- record-tier
exploratory read"). Only-follow traversal from a validated
`command::ReadRecord`'s anchor, over the controller door.

Pure execution layer: standard library, the codec, `command/` (the IR)
and `controller/` (the door) only. It adds no storage access of its own
beyond what the controller already exposes (`parents_of` / `children_of`
/ `members_of` / `member_of`, and -- for a terminal-wildcard anchor --
`gather`, each called once per node/region visited); it does not modify
`command/` or `controller/`.

Cache-shaped mode is out of scope regardless of `ReadRecord::cache_shaped`
-- this module always executes the raw-radial (cache-free) core, per
PLAN.md I.C ("the core function is raw-radial only").

## Files

- `read_core.h` / `read_core.cpp` -- the traversal (`dbread::read`), its
  result shape (`ReadNode`, `ReadResult`, `ReachedVia`, `ReadStatus`).
- `read_core_test.cpp` -- a standalone check harness against a real,
  disposable local Postgres (same style/bootstrap as
  `controller/controller_test.cpp`).

## Interface

```cpp
enum class ReachedVia { kAnchor, kParent, kChild, kMembers, kMemberOf };

struct ReadNode {
  codec::Address address;
  unsigned depth = 0;
  ReachedVia via = ReachedVia::kAnchor;
};

enum class ReadStatus { kOk };

struct ReadResult {
  ReadStatus status = ReadStatus::kOk;
  std::vector<ReadNode> nodes;  // meaningful iff status == kOk.
};

ReadResult read(dbk::Controller &ctl, const command::ReadRecord &op);
```

The caller runs `command::validate_read(op)` first (that step is not
repeated here); `read()` assumes a structurally valid `ReadRecord`,
including the one case validation deliberately leaves open for this
module to resolve: a wildcard anchor (see "Terminal wildcards" below).

## Direction, depth, exclusions, linearization

- **Direction.** `ReadDirection::axis` narrows to a single axis
  (`kParents` = structure, `kMembers` = membership) or `kBoth`; unset
  means RADIAL, the default. `reverse` is an orientation on a narrowed
  axis, not a peer value (NOTES.md): `kParents` forward follows
  `parents_of` (structure-down), reverse follows `children_of`
  (structure-up); `kMembers` forward follows `members_of`
  (membership-down), reverse follows `member_of` (membership-up).
  RADIAL walks all four follows regardless of `reverse` -- see "Decisions"
  below for why.
- **Depth** is a plain hop-count budget from the anchor. Depth 0 returns
  only the anchor (the "rolled-up node"); at depth *k* the traversal has
  expanded *k* levels along whichever follows are active. This is the
  direct reading of "depth 0 = the rolled-up node... governs whether
  members are in scope" (PLAN.md I.C): at depth 0 nothing beyond the
  anchor is exposed, so members are trivially out of scope; at depth >= 1
  they come into scope exactly when the active follows include a
  membership axis. No separate per-axis depth-gating rule is implemented
  beyond this -- see "Decisions."
- **Exclusions** prune named branches: a node matching an exclusion is
  dropped from the result AND not expanded, cutting off everything beneath
  it. An exclusion is either a specific `token_id` (exact match) or a
  terminal wildcard (a partial/prefix address, per NOTES.md "Build-phase
  rulings" -- READ's exclusions may be "a terminal-wildcard tree region").
  Wildcard exclusions are resolved by a pure address-prefix comparison
  against whatever node the traversal has already reached by following --
  no store access, so this is squarely only-follow. See "Terminal
  wildcards" for why this differs from a wildcard *anchor*.
- **Linearization / no dedup.** The traversal is depth-first, pre-order:
  a node is emitted, then each of its active follows is walked in turn,
  each producing its own recursive sub-sequence before the next follow
  (or sibling) is visited. Nothing is deduplicated: a token_id reached at
  two structurally distinct points (e.g. two composites sharing a common
  constituent) produces two separate `ReadNode` entries, at their own
  positions and depths -- exactly NOTES.md's "Re-convergence -- RULED:
  once per path, NO dedup." Termination is guaranteed by the depth budget
  alone (no visited-set), which is also why a structural cycle, were one
  ever to occur, would not hang the traversal -- it would simply be
  walked, unindexed, until the budget ran out, consistent with "no dedup"
  extending to not collapsing a repeat visit either.

## Terminal wildcards: anchor and exclusions, both implemented

NOTES.md ("Build-phase rulings -- firmed 2026-09-17") permits READ's
anchor AND its exclusions to be terminal wildcards, both described as a
"nominal, tree-constrained" read/selection -- distinct from a forbidden
arbitrary property-predicate scan. The two cases reach that result by
different mechanisms, because they start from different amounts of
information:

- **A wildcard exclusion** is checked against nodes the traversal has
  *already reached* by following a stored edge. Testing "does this
  address, already in hand, fall under that wildcard's prefix" is a pure
  comparison between two address values -- no query, no scan, nothing
  that touches the store beyond the follow that produced the address in
  the first place. This module implements it in full
  (`address_under_prefix` in `read_core.cpp`).
- **A wildcard anchor** has no node to start from: resolving it to "the
  contiguous tree region" it names means enumerating which addresses are
  actually *populated* in the store under that prefix. No plain
  `parents_of`/`children_of`/`members_of`/`member_of` follow does this
  (they all key off an existing token's PK, and a partial/prefix address
  is not a PK any live token row carries), but the controller's **gather**
  primitive (`Controller::gather(prefix)`, firmed 2026-09-18) does: a
  contiguous PK-range scan over `token` returning every existing token
  under that prefix, in PK/address order -- only-follow (a bounded key
  range on the existing PK), not a reverse-search. `read()` resolves a
  wildcard anchor by calling `ctl.gather(op.anchor)` once, then running
  the ordinary raw-radial `visit()` from each resolved token in turn (own
  depth 0, `ReachedVia::kAnchor`), appending each member's nodes in
  gather's order. The result is exactly the linearized, once-per-path,
  no-dedup return the concrete-anchor path already produces, just seeded
  from several anchors instead of one; nothing is deduplicated across
  members either, consistent with "no dedup" everywhere else in this
  module. An empty gathered region (no existing token under the prefix)
  is a well-formed read of nothing: `ReadStatus::kOk` with an empty
  `nodes` vector, not a distinct status.

  This was deferred until `gather` existed (there was previously no
  only-follow way to enumerate an unpopulated prefix); it is filled here
  now that `gather` is a committed door primitive. `ReadStatus` no longer
  carries a wildcard-deferral member -- `kOk` is its only value.

## Decisions made where the spec was silent

Flagged here, not silently assumed:

- **RADIAL ignores `reverse`.** PLAN.md/NOTES.md describe RADIAL as "all
  directions outward" and describe `reverse` as an orientation applied
  "on those axes" once one is narrowed. Read literally, `reverse` has
  nothing to select when no axis is narrowed, so this module always walks
  all four follows under RADIAL regardless of `ReadDirection::reverse`.
  The spec never states what a RADIAL-plus-`reverse=true` request should
  do differently; this is the reading that treats `reverse` as
  meaningless off a narrowed axis rather than inventing a second flavour
  of radial.
- **Follow order is a fixed, documented convention**, not a stated rule:
  structure before membership, forward (down) before reverse (up) within
  an axis -- `kParent, kChild, kMembers, kMemberOf`. NOTES.md/PLAN.md
  specify per-axis semantics but never a linearization order for combined
  axes (RADIAL or `kBoth`). Determinism only requires *some* fixed order;
  this is the one chosen, applied identically inside `active_follows()`
  in `read_core.cpp`.
- **Depth is a single, uniform hop budget**, not a per-axis dial. NOTES.md
  frames depth as "the LoD dial... governs whether members are in scope,"
  which this module reads as the natural consequence of a shared depth
  budget (nothing is in scope past the budget, membership included) rather
  than as license to invent a second, axis-specific depth mechanism the
  spec does not describe.
- **A nonexistent anchor returns `kOk` with an empty result**, not a
  distinct error status. Not discussed in NOTES.md/PLAN.md; this is the
  plain "there was nothing to follow" reading, not a fabricated error
  case.
- **Exclusions apply uniformly to every node reached, including a literal
  match on the anchor itself** (an empty result, degenerate but
  consistent) -- no special-casing of the anchor against the exclusion
  set, since the spec states no exemption for it.
- **`parents_of` duplicates are preserved, not folded.** A composite
  whose declared constituent list repeats a parent at two ordinals (e.g.
  `mint`'s own `c2` fixture in `controller_test.cpp`) is walked as two
  separate structure-down follows here too, matching the no-dedup return
  rule rather than collapsing to the parent's distinct-child count (that
  distinctness is a `token_child`/WIRE property, not a READ property).
- **A wildcard anchor's resolved members are each their own depth-0 root,
  concatenated in gather's order.** NOTES.md describes the wildcard
  anchor as reading "a range / full construct," not a single tree with
  one root; the plain reading is N independent raw-radial reads (one per
  existing token gather() finds under the prefix), one after another in
  the order gather returns them (PK/address order), rather than
  synthesizing a single artificial super-root or renumbering depth across
  members. Depth, direction and exclusions all apply per member exactly
  as they would to a concrete single-token anchor.

## Build and run the tests

Requires libpq and a local Postgres (same as `controller/`):

```sh
# from kernels/database/read/
g++ -std=c++17 -O2 -Wall -Wextra \
    -I. -I../codec -I../command -I../controller -I"$(pg_config --includedir)" \
    read_core.cpp read_core_test.cpp \
    ../codec/codec.cpp ../command/command_ir.cpp ../command/span_planner.cpp \
    ../controller/controller.cpp \
    -L"$(pg_config --libdir)" -lpq \
    -o read_core_test

./read_core_test               # uses ../schema/schema.sql
./read_core_test /path/to/schema.sql   # optional override
```

`read_core_test` resets the real, disposable `hcp3_core` database (drop +
recreate `public`, reapply the schema) exactly as `controller_test` does,
builds the fixture described above, and covers: a radial read; structure-
axis-only; membership-axis-only; reverse orientation on each axis; depth 0
vs. a deeper level (the LoD dial); a specific-id exclusion pruning a named
branch; a terminal-wildcard exclusion pruning the same branch by pure
prefix comparison; the linearized once-per-path return with a shared node
(`p1`, reachable through two different composites) recurring at two
positions, not deduplicated; a nonexistent anchor; a wildcard anchor that
gathers a trunk and reads each member radially (asserting the linearized
result over the whole region, including a node shared across members
recurring per path, not deduplicated); and a wildcard anchor over an
empty region (asserting `kOk` with no nodes, not a deferred status). If
no local Postgres is reachable it prints a clear message and exits
non-zero rather than claiming a pass.
