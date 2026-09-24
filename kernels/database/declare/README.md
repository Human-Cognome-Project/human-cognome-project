# declare — the DECLARE core

Executes one `DECLARE_RECORD` statement (PLAN.md II.3) against the
`controller/` door, consuming a `command::DeclareRecord` IR already
structurally validated by `command::validate_declare`. Standalone within
this subdirectory: it depends on `../codec`, `../command` (the IR + the
address span planner), and `../controller` (the door), and touches nothing
in `../schema`, `../read`, or the ingestion runtime.

## Files

- `declare_core.h` / `declare_core.cpp` -- the DECLARE core:
  `declare::execute(Controller&, const DeclareRecord&) -> declare::Result`.
- `declare_core_test.cpp` -- a standalone check harness against a real,
  disposable local `hcp3_core`, same style as `controller/controller_test.cpp`.

## Build & run

```sh
# from kernels/database/declare/
g++ -std=c++17 -O2 -Wall -Wextra \
    -I. -I../codec -I../command -I../controller -I"$(pg_config --includedir)" \
    declare_core.cpp declare_core_test.cpp \
    ../codec/codec.cpp ../command/command_ir.cpp ../command/span_planner.cpp \
    ../controller/controller.cpp \
    -L"$(pg_config --libdir)" -lpq \
    -o declare_core_test

./declare_core_test               # uses ../schema/schema.sql
./declare_core_test /path/to/schema.sql   # optional override
```

Resets the real, disposable `hcp3_core` (`DROP SCHEMA public CASCADE;
CREATE SCHEMA public;`, reapply `schema.sql`) before running, exactly as
`controller_test.cpp` does. If no local Postgres is reachable it prints a
clear message and exits non-zero rather than faking a pass.

## Scope

One `DECLARE_RECORD` statement, including its N-member `PARENTS` set
(`PARENTS` sets N; co-indexed `NOTATION`/`MEMBER_OF`) and its nesting: any
relationship field (`PARENTS`, `MEMBERS`, `MEMBER_OF`) may be a full nested
`DeclareRecord`, recursed here, building a whole connected sub-structure in
one call. Cross-`COMMAND` arraying -- a *serial stream of several DECLARE
statements* sharing a scalar frame (PLAN.md I.F, II.7) -- is a later module
(the arraying executor); this module executes exactly one statement (whose
own `PARENTS` array is itself the set/array form -- "single vs set = scalar
vs array", NOTES.md).

## Interface

```cpp
struct Outcome {
  bool ingested = false;
  std::optional<codec::Address> address;  // set iff ingested
  std::string reason;                     // set iff !ingested
};

struct Result {
  std::vector<Outcome> outcomes;
};

Result execute(dbk::Controller &ctl, const command::DeclareRecord &node);
```

Return shape (PLAN.md I.G; NOTES.md "Grouping-node naming literal"):

- **`PARENTS` present** (a structure node, or the **mint form** of a
  grouping node's naming literal when `MEMBERS` is also present): one
  `Outcome` per `PARENTS` member, in order (`N = parents->size()`; `N`
  is forced to `1` by `command::validate_declare` whenever `MEMBERS` is
  also present), each the ACTUAL assigned address (never an echo of a
  requested `@`-pin -- there is no override mechanism anywhere in this
  codebase yet).
- **`PARENTS` absent** (a grouping node's **use-provided-ID** form --
  `MEMBERS` present, `ADDRESS` required by validation): exactly one
  `Outcome` -- the referenced, pre-existing naming-literal token's
  address.
- **Rejection**: `ingested = false`, `reason` populated, `address` empty.
  No exception for an ordinary rejection (a missing constituent, an
  absent use-provided-ID naming literal, an omitted mint-form `ADDRESS`
  ["manager-placed", not implemented -- see below], an undeclared-hook
  slot, an address pending the deferred trunk map). A genuine
  store/connection error still throws, via the controller -- this module
  does not swallow those.

## The grouping-node naming literal

NOTES.md "Grouping-node naming literal" / PLAN.md I.B (ruling, firmed
2026-09-17): a grouping node (`MEMBERS` present) establishes its naming
literal one of two ways, both structurally distinguished and enforced by
`command::validate_declare` before this module ever runs:

- **Use-provided-ID** (`PARENTS` absent, `ADDRESS` present): `ADDRESS`
  references a **pre-existing** naming literal. Resolved via the same
  span-planning machinery a structure placement uses
  (`command::plan(*node.address, 1)`, then the shared placement-resolution
  helper -- see "How nesting works" below), then a plain
  `Controller::token_exists` check; absent -> rejected un-ingested, never
  thrown, never fabricated.
- **Mint** (`PARENTS` present): the naming literal is minted from that
  composition -- exactly the structure-node mint path, since `PARENTS`
  present already means "mint a token here" regardless of `MEMBERS`.
  Placed at `ADDRESS` if given (the mint's target); if `ADDRESS` is
  **omitted entirely**, the spec calls for "manager-placed" -- **not
  implemented here**: there is no next-slot / address-assignment
  mechanism anywhere in this codebase (`PLAN.md` Part V, **G4/G5 --
  open, Patrick's**), and choosing one (which trunk, which kind,
  sequential from where) would be exactly the invented complication this
  module is bound not to introduce. This shape is valid IR (accepted by
  `command::validate_declare`) but this module rejects it cleanly at
  execution time with a reason naming the gap, rather than guessing an
  address. **Flagged for Patrick/team-lead**: once G4/G5 are resolved,
  this is a small, additive follow-up (a manager-side address allocator
  this module would call into) -- not a redesign.

`NOTATION` plays no role in identity in either form -- it is
optional/human-facing only, exactly like a plain structure node's
`NOTATION`.

## How nesting works

Every relationship field -- `PARENTS`' constituent references, `MEMBERS`,
`MEMBER_OF`'s shared and per-member sections -- is a `command::Reference`:
either a plain address (must already exist, checked via
`Controller::token_exists`) or a full nested `DeclareRecord`
(`Reference::nested`). Resolving a nested reference recurses into
`execute()` and, per PLAN.md I.E ("a nested declare mints one item -> one
address -> one self-delimiting slot"), the reference collapses onto the
nested statement's own resulting address. A nested declare that produces
more than one outcome (e.g. its own multi-member `PARENTS` set) cannot
collapse to a single reference value; this module rejects that reference
rather than picking one address arbitrarily -- not stated in the spec, so
not invented as a silent default. Covered directly by
`declare_core_test.cpp`.

The `ADDRESS` span (`command::plan()`) can itself carry a `kNestedDeclare`
segment -- a nested statement occupying one of the node's own placement
slots. This is executed the same way (the shared `resolve_placement`
helper): the nested statement is minted first, and its resulting address
becomes that slot's placement. `command::validate_declare` now **rejects**
this whenever the *same* node also carries `PARENTS` (it would collide
with that slot's own `PARENTS[i]` composition, silently dropping it --
NOTES.md ruling #3) -- this module relies on that upstream rejection and
does not re-implement the check. The remaining legal case (a
`PARENTS`-absent grouping node whose `ADDRESS` is itself a nested declare
-- "use-provided-ID via an inline nested mint") is exercised by
`declare_core_test.cpp`: the nested mint's resulting address is used, and
trivially already exists by the time the existence check runs.

`SEE` idempotency (re-declaring an existing token is a no-op/link-only)
falls straight out of `Controller::mint()`'s own SEE-probe: this module
always calls `mint()` unconditionally for a resolved placement, and a
shared nested product minted from several call sites simply gets probed
and skipped on every call after the first. `Controller::add_membership()`
is now SEE-idempotent at the door itself (`ON CONFLICT DO NOTHING`, both
directions), so this module calls it directly with no caller-side dedup
guard.

### Atomicity boundary: eager nested mint is not rolled back

A nested reference is minted **eagerly**, at the moment it is resolved
(`resolve_reference` calls `execute()` immediately) -- not deferred until
the whole outer statement is known to succeed. Per-member atomicity (see
below) means the *outer* member's own token is never minted if one of its
references later fails to resolve, but if an EARLIER reference in that
same member's list was itself a nested declare, that nested declare's own
mint has already committed by the time a LATER reference fails and the
outer member rejects. Nothing in this module rolls that back -- `mint()`
has no notion of a multi-statement transaction spanning the whole
recursive resolution, and this module does not add one. Concretely: if
`PARENTS[i] = [nested_declare_ref, missing_address_ref]`, the nested
declare is minted, then the second reference fails and member `i` is
reported un-ingested -- but the nested product stays in the store,
SEE-reachable, just not linked as `PARENTS[i]`'s own constituent (since
`PARENTS[i]`'s token was never minted at all). Demonstrated directly in
`declare_core_test.cpp`'s ">1-outcome nested reference" case (the nested
sub-structure's tokens exist afterward even though the outer point
rejects).

## The N-member set

`PARENTS`' outer length sets `N`. The `ADDRESS` span is resolved once via
`command::plan(*node.address, N)`; each `PlannedSlot` (direct/pin value,
closed `FROM`/`AFTER..TO` run element, elastic `FROM` fill, nested
declare, undeclared hook, or an unresolved slot pending `AFTER`'s deferred
trunk map) becomes one member's placement, in order. `NOTATION` and
`MEMBER_OF` section 2 are read positionally against the same index `i`.
Each member is executed with **per-member atomicity**: its own
constituent list and its `MEMBER_OF` groups (section 1 `shared` union
section 2's sparse entry for `i`) are all resolved *before* anything is
written; if any reference in that set fails, the member is rejected
un-ingested and nothing is written for it, but the other members in the
same statement are attempted independently (PLAN.md I.G: "a rejected
point -> un-ingested", singular -- the array's points are independent).
Exercised directly for the undeclared-hook, `@`-pin, pending-seam, and
per-member-`MEMBER_OF` cases in `declare_core_test.cpp`.

## Decisions made where the spec was silent

Flagged here per the build instructions, not silently assumed.

- **Manager-placed mint (mint form, `ADDRESS` omitted).** Not
  implementable without inventing an address-assignment mechanism (G4/G5
  are open). Rejected cleanly at execution time rather than guessed --
  see "The grouping-node naming literal" above. This is the one
  significant open point in this module today.
- **`PARENTS`-absent `ADDRESS` that is itself a nested declare
  ("use-provided-ID via an inline nested mint").** Not called out
  explicitly in the team-lead brief, but structurally legal per
  `command::validate_declare` (ruling #3's rejection only fires when the
  *same* node also carries `PARENTS`). Handled by reusing the same
  placement-resolution helper the mint form uses, then applying the
  ordinary existence check -- no new mechanism, and exercised by a test.
- **Blank `NOTATION` storage.** `Controller::mint()`'s `notation`
  parameter is a plain (non-optional) `std::string`; there is no NULL
  channel to request through it. A blank slot (`NOTATION` shorter than
  `N`, or an explicit `nullopt` entry) is therefore stored as the empty
  string `""`, which this module takes as "stored blank" per NOTES.md's
  "a blank ... field ... reads blank now."
- **`MEMBER_OF` / `MEMBERS` reference failure granularity.** A structure
  member's own constituents and its `MEMBER_OF` groups are resolved
  together, atomically, before that member is minted (see "The N-member
  set" above) -- a failure in either rejects the whole point. By contrast,
  once a member is *already* ingested, a `MEMBERS` roster entry that
  fails to resolve (mint-form grouping, layered on top) only skips that
  one edge; it does not retroactively un-mint an already-returned point.
  These are different points in the statement (the member's own return
  value vs. supplementary grouping metadata over it), so this asymmetry
  is intentional, not an oversight.
- **Re-validation.** `execute()` re-runs `command::validate_declare` on
  entry (including on every recursive nested call) even though the spec
  says the IR arrives pre-validated. An already-valid node is unaffected;
  this is defensive insurance against being handed a malformed node
  directly (e.g. in a future caller that does not gate through
  `command::` first), not a semantic decision.

Two earlier flagged points from the first build are now resolved
upstream, in `command/` itself, and no longer need a decision here:
whether a mixed `PARENTS`+`MEMBERS` node can have `N > 1` (now rejected
by `command::validate_declare`, ruling #2 -- this module never sees that
shape) and the `ADDRESS`-span-nested-declare-vs-`PARENTS` collision (now
rejected by `command::validate_declare`, ruling #3, for the same reason).
This module relies on those upstream rejections rather than
re-implementing them.

## What this module does NOT do

- Cross-`COMMAND` arraying (PLAN.md II.7) -- a later module.
- Mass aggregation -- the analyst path always passes `mass = std::nullopt`
  to every `mint()` call; the seed floor is inserted directly through the
  controller by the test fixtures, never through `declare::execute()`.
- Notation derivation (the surface-from-parents concatenation) -- NOTATION
  is stored exactly as given (or blank), never derived.
- Manager-placed address assignment (mint form, `ADDRESS` omitted) -- see
  above; deferred to G4/G5.
- `AFTER`'s concrete origin (G5, the deferred trunk map) or the `@`-pin
  override (no override mechanism exists anywhere in this codebase yet)
  -- both are reported as-is (pending / as requested), never guessed at.
