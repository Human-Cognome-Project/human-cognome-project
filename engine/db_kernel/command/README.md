# command

The record-tier Command IR and the address span planner: the parsed,
in-process request shape the record-tier cores (DECLARE / READ / UPDATE)
will consume, plus its structural validation and the ADDRESS-span
resolver those cores need before they touch storage.

Pure/parse-level: standard library and the codec only. No DB, no libpq,
no controller dependency -- this module sits entirely upstream of the
door. Text/wire framing into this IR (PLAN.md I.H, seam G6) is a separate,
unbuilt layer; the IR here starts from already-parsed C++ values.

## Files

- `command_ir.h` / `command_ir.cpp` -- the typed request structures for
  every record-tier verb (`DeclareRecord`, `ReadRecord`, `MoveRecord`,
  `AddConnection`, `DeleteRecord`, `DeleteConnection`) and their
  structural validation.
- `span_planner.h` / `span_planner.cpp` -- the ADDRESS span planner
  (`plan()`) and the base-50 address-successor helper the codec does not
  provide.
- `command_ir_test.cpp` / `span_planner_test.cpp` -- unit tests
  (standalone; same tiny check harness as `codec/codec_test.cpp`, no
  external test framework).

## Build & run the tests

```
g++ -std=c++17 -O2 -Wall -Wextra -I../codec -o /tmp/command_ir_test \
  ../codec/codec.cpp command_ir.cpp span_planner.cpp command_ir_test.cpp \
  && /tmp/command_ir_test

g++ -std=c++17 -O2 -Wall -Wextra -I../codec -o /tmp/span_planner_test \
  ../codec/codec.cpp command_ir.cpp span_planner.cpp span_planner_test.cpp \
  && /tmp/span_planner_test
```

Each prints `PASS <name>` and exits 0 if every check passes; otherwise it
prints `FAIL` lines and exits non-zero.

## No TYPE, anywhere

`TYPE` is dropped (NOTES.md "Relationship model & type -- firmed
2026-09-17"). Validation in `command_ir.cpp` is entirely presence-based:
`PARENTS` present means a structure node, `MEMBERS` present means a
grouping node, and a node may be both at once. There is no declared-type
branch anywhere in this module.

## Structural validation rules (`command_ir.cpp`)

- **Required downward field.** A declare node must carry `PARENTS` and/or
  `MEMBERS`; a node with neither is rejected.
- **Structure floor.** Each `PARENTS` member is a list of `>= 2`
  constituent references (anti-alias floor).
- **Grouping floor.** `MEMBERS`, when present, must be non-empty (`>= 1`
  -- the literal's `>= 2` floor does not transfer to grouping; NOTES.md's
  "label dual" locks it at `>= 1`).
- **Grouping-node identity.** A pure-grouping node (`MEMBERS` present,
  `PARENTS` absent) may not also carry its own `ADDRESS` placement, and
  must carry a non-blank `NOTATION` -- with no `PARENTS` there is nothing
  to derive a surface from, so the naming-literal prose is the only
  handle it has (PLAN.md I.B).
- **NOTATION co-index.** When present, `NOTATION`'s length must equal
  `N` exactly; individual blank entries ("successive commas") are legal
  and are a content rule, not a length exemption.
- **MEMBER_OF section 2 is sparse.** Its per-member map is checked only
  for in-range indices (`< N`); its size is never compared to `N` --
  unlike `NOTATION`, a short section 2 is not a violation.
- **ADDRESS must cover N**, delegated to `span_planner::plan()`.
- **Recursion.** Every `Reference` (inside `PARENTS`, `MEMBERS`,
  `MEMBER_OF`) and every nested-declare `ADDRESS` segment is validated
  recursively via `validate_declare`, so a mixed nested statement is
  checked node by node.

`READ_RECORD` / `MOVE_RECORD` / `ADD_CONNECTION` / `DELETE_RECORD` /
`DELETE_CONNECTION` get lighter, presence/shape-only validation (valid
codec addresses, non-empty arrayable sides) -- PLAN.md does not specify
deeper structural rules for these the way it does for DECLARE.

**Wildcards -- per NOTES.md "Build-phase rulings -- firmed 2026-09-17".**
That section is the governing spec for everything below; PLAN.md I.C/I.D/
II.5 record the same rulings at the plan level.

- **Terminal only, globally.** A wildcard is a partial *trailing* address
  element (the codec's existing partial-element concept, e.g. `A*`);
  an inline (non-terminal) wildcard is invalid. `codec::is_valid_address`
  already enforces this (a partial element is rejected unless it is the
  address's last one), so every validator in this module inherits the
  rule for free -- `command_ir_test.cpp`'s `test_wildcards_are_terminal_only`
  pins it explicitly across READ, ADD_CONNECTION, and MOVE's `kPrefix`.
- **MOVE's source** may be a range or a terminal-wildcard prefix (a
  `MoveSource` of kind `kExplicit` / `kRange` / `kPrefix`) -- MOVE's
  primary use is BULK CORRECTION, relocating a whole misclassified
  branch/region at once. A range/prefix selector is still only-follow,
  not a search: a contiguous addressed region, walked, never predicate-
  scanned. Its unit count N, and the destination's cover-N against it,
  are store-resolved -- see the cover-N paragraph below.
- **READ's anchor and exclusions** may be a terminal wildcard: a
  **nominal, tree-constrained** region read (walking a connected
  branch), distinct from the forbidden arbitrary property-predicate scan.
- **ADD_CONNECTION's group and/or elements** may be a terminal wildcard,
  in **either direction** -- "add all wildcard-selected members to this
  group" or "add this member to all wildcard-selected groups." Endpoints
  still must pre-exist. The bidirectional **wildcard expansion** --
  resolving the wildcard to its definable address range and enumerating
  the resulting pairs -- is an EXECUTION-time concern for the UPDATE core
  (PLAN.md II.5); this module validates only that the address itself is
  a well-formed (terminal-only) wildcard or an ordinary explicit address,
  nothing about what it expands to.
- **DELETE stays explicit-only** (NOTES.md: "specific ids/pairs only ...
  no wildcards") -- the one hard, unchanged prohibition.
  `is_wildcard()` / `is_explicit_address()` in `command_ir.cpp` guard
  `DeleteRecord::token` and `DeleteConnection::a`/`b` against any
  wildcard, terminal or not.

**MOVE_RECORD's cover-N depends on whether N is statically known.**
`sources` is a `vector<MoveSource>`, so explicit, range, and prefix
selectors freely coexist in one MOVE. `validate_move_record` takes two
paths: if every source is `kExplicit`, N = `sources.size()` is known
statically and the destination is checked to cover it exactly, via
`plan()` -- the same as DECLARE's ADDRESS. If ANY source is a
`kRange`/`kPrefix` wildcard, N depends on which addresses in that region
are actually populated in the live store -- not knowable at pure
IR-validation time -- so cover-N is skipped entirely for the whole
statement; only the destination's own shape is checked
(`span_planner::validate_span_shape`, the N-independent subset of
`plan()`'s grammar rules: valid addresses, nested-declare recursion, a
closed `FROM..TO`'s forward-walkability, elastic-only-if-last). "Does the
destination cover the resolved source-N" for a wildcard/range source is
deferred to the UPDATE core (PLAN.md II.5), which has DB access -- along
with resolving the range/wildcard itself to its concrete member
addresses in the first place. This module never touches the store, so
neither resolution step happens here.

## The address span planner (`span_planner.cpp`)

`plan(span, n)` resolves an `AddressSpan` (I.E's `FROM` / `AFTER` / `TO` /
direct / `@`-pin / undeclared-hook / nested-declare alphabet) against a
slot count `N`:

- segments lay end to end; validity requires them to cover exactly `N`;
- an open (un-`TO`'d) `FROM` or `AFTER` origin is elastic and legal only
  as the **last** segment;
- every other segment -- direct, pin, a closed `FROM`/`AFTER..TO` run, a
  nested declare, the undeclared hook -- is self-delimiting and legal
  anywhere, including interior positions;
- a nested declare is always exactly one slot, anywhere.

`plan()` returns one of three statuses: `kValid` (fully resolved),
`kInvalid` (malformed -- does not cover N, a bad elastic placement, an
unwalkable `FROM..TO` run), or `kPendingSeam` (structurally well-formed
but containing an `AFTER` segment whose concrete origin needs the
deferred trunk map -- see Open seams below).

### Base-50 address successor

The codec (`codec/codec.h`) has no whole-address increment, so this
module adds one: `successor()` treats an `Address` as a mixed-radix
counter over base-`kCoupletSpace` couplets, most-significant element
first, carrying leftward on overflow. `span_length()` computes an
inclusive slot count between two addresses by exact digit-wise
subtraction over that same representation (not by stepping `successor`
in a loop). Both reject partial (wildcard) elements, since a context/
prefix element addresses a range, not a point, and has no single
successor.

## Open seams (Patrick's; PLAN.md Part V) -- not resolved here

- **G4 -- next-slot mechanism.** This module fills sequentially only from
  an analyst-supplied `FROM:x` start (needs no cursor). No per-trunk
  cursor is built.
- **G5 -- block boundaries past "hex couplets".** `successor()` returns
  `nullopt` rather than grow an address into a new ring when the carry
  overflows the leftmost element -- that depth-expansion decision depends
  on the (unfixed) trunk map. `AFTER:b`'s concrete origin ("the next
  block after b") depends on the same map, so `plan()` never computes a
  concrete address for an `AFTER` segment; it structurally accepts the
  grammar and reports `kPendingSeam` instead of guessing a boundary. A
  closed `AFTER:b TO:y` cannot even have its slot count verified this way
  (count = distance from an unknown start to `y`), so `plan()` reports
  `kPendingSeam` as soon as it reaches one, with whatever prefix was
  already planned.

## Decisions made where the spec was silent

Flagged here, not silently assumed -- see the handoff report for full
reasoning:

- **N for a pure-grouping node.** PARENTS "sets N" for a structure node;
  NOTES.md gives no grouping-side equivalent. This module takes a
  grouping-only node as declaring exactly one token (its naming literal),
  so `N = 1` there -- used only for `NOTATION` length and `MEMBER_OF`
  section-2 index-range checks.
- **Grouping-only node requires non-blank NOTATION.** Not stated as a
  reject rule in so many words, but implied by "no PARENTS to derive a
  surface from" plus "its only handle is the naming-literal prose" --
  enforced as a hard validation failure when absent/blank.
- **A `Reference` is exactly one of {address, nested}.** Both-or-neither
  is rejected as malformed IR, not left as an unspecified state.
- **TO is folded into its preceding FROM/AFTER segment** rather than
  modelled as a free-standing segment kind, since I.E states it is
  "never standalone" -- this makes a standalone TO structurally
  unrepresentable rather than a validation case to reject.
