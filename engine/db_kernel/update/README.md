# update — the UPDATE core (four ops)

Executes the four `UPDATE_RECORD` sub-ops (PLAN.md II.5) against the
`controller/` door: `MOVE_RECORD`, `ADD_CONNECTION`, `DELETE_RECORD`,
`DELETE_CONNECTION`. Standalone within this subdirectory: it depends on
`../codec`, `../command` (the IR, structural validation, and the address
span planner), and `../controller` (the door, including `gather`), and
touches nothing in `../schema`, `../declare`, `../read`, or the ingestion
runtime.

## Files

- `update_core.h` / `update_core.cpp` -- the four ops:
  `update::move_record`, `update::add_connection`, `update::delete_record`,
  `update::delete_connection`.
- `update_core_test.cpp` -- a standalone check harness against a real,
  disposable local `hcp3_core`, same style as `controller/controller_test.cpp`
  and `declare/declare_core_test.cpp`.

## Build & run

```sh
# from db_kernel/update/
g++ -std=c++17 -O2 -Wall -Wextra \
    -I. -I../codec -I../command -I../controller -I"$(pg_config --includedir)" \
    update_core.cpp update_core_test.cpp \
    ../codec/codec.cpp ../command/command_ir.cpp ../command/span_planner.cpp \
    ../controller/controller.cpp \
    -L"$(pg_config --libdir)" -lpq \
    -o update_core_test

./update_core_test               # uses ../schema/schema.sql
./update_core_test /path/to/schema.sql   # optional override
```

Resets the real, disposable `hcp3_core` before running, exactly as the
other `*_test.cpp` harnesses do. If no local Postgres is reachable it
prints a clear message and exits non-zero rather than faking a pass.

**Verified against a real local Postgres for this build**: all 74 checks
pass (`PASS update_core_test`).

## Scope

Each function executes exactly ONE `UPDATE_RECORD` sub-op. Cross-`COMMAND`
arraying -- a *serial stream of several UPDATE statements* sharing a
scalar frame (PLAN.md I.F, II.7) -- is a later module (Agent 6), out of
scope here. What IS in scope, and easy to mistake for that later module,
is each op's own INTRINSIC range/wildcard resolution: `MOVE_RECORD`'s
wildcard/range source and `ADD_CONNECTION`'s wildcard group/elements are
resolved here, via the controller's `gather` primitive, because that
resolution is what the op's own single statement means, not a batch of
several statements.

## MOVE_RECORD

### Interface

```cpp
struct MoveOutcome {
  bool moved = false;
  std::optional<codec::Address> from;
  std::optional<codec::Address> to;
  std::string reason;  // set iff !moved.
};
struct MoveResult { std::vector<MoveOutcome> outcomes; };

MoveResult move_record(dbk::Controller &ctl, const command::MoveRecord &op);
```

### How it resolves a source

`op.sources` is a vector of `MoveSource` (see `command/command_ir.h`):
`kExplicit` (one address), `kRange` (a `FROM..TO` pair), `kPrefix` (a
terminal wildcard). Each selector resolves to zero or more EXISTING
tokens:

- `kExplicit` contributes its one address as given, UNCHECKED for
  existence at this point -- existence is checked per-token later, at
  placement time (see "Per-source-token processing" below), so a
  nonexistent explicit source still occupies a destination slot (and
  counts toward `N`) even though its own relocation will be rejected.
  This matches how `command::validate_move_record`'s all-explicit
  cover-N check already counts `sources.size()` without any DB access.
- `kRange` / `kPrefix` contribute `Controller::gather(from, to)` /
  `Controller::gather(prefix)`'s result, in PK/address order -- these
  calls read straight off `token`, so everything they return already
  exists.

**Source ordering.** Several source selectors may coexist in one
`MoveRecord` (`command_ir.h`: "an explicit token, a range, and a prefix
wildcard may all appear in the same MOVE"). This module concatenates
their resolved token lists in SELECTOR order (the order `op.sources` lists
them), each selector's own tokens in the order it produces them. This is
a decision, not a directly-stated rule: the team-lead brief's "source in
PK/byte order <-> destination slots in order" is read here as "each
selector's own resolution is in PK/byte order" (true by construction for
`kRange`/`kPrefix`, trivially true for a single `kExplicit` address), not
as "re-sort the whole combined multi-selector list into one global PK
order" -- the latter would silently reorder an explicit selector relative
to a range/prefix selector's contribution, which nothing in the spec
calls for and which would make an intentionally-ordered mixed-selector
MOVE behave surprisingly.

### Cover-N

`N` = the resolved source count. For an all-explicit `MoveRecord`,
`command::validate_move_record` already checked cover-N at IR level
against this same `N` (no DB access needed, since explicit-source count is
static). For any range/prefix source, `N` depends on the live store, so
this module calls `command::plan(op.destination, N)` itself, after
resolving sources -- this is the "execution-time cover-N check" PLAN.md
II.5 assigns to the UPDATE core. A cover-N mismatch rejects the WHOLE
statement as one `MoveOutcome` (mirroring `declare::Result`'s use of a
single `Outcome` for a whole-node rejection) -- no source is touched.

### The destination mint-bearing-segment rejection (F-1, deferred half)

`command::validate_move_record` is a pure/parse-level, no-DB-access check;
it permits a destination `AddressSpan` containing a `kNestedDeclare` or
`kUndeclared` segment, because -- taken alone -- both are structurally
legal `AddressSegment` shapes (the same grammar `DECLARE`'s `ADDRESS`
uses). This module is where they are actually rejected: PLAN.md II.5
states outright that "a relocation mints nothing," so BEFORE resolving
any source or planning anything, `move_record` scans `op.destination` and
rejects the whole statement (one `MoveOutcome`, `from`/`to` both empty) if
any segment is `kNestedDeclare` (would mint a whole new sub-structure) or
`kUndeclared` (the inert extrapolate-address hook -- also assigns nothing
a MOVE could rekey onto). Covered by `update_core_test.cpp` for both
segment kinds independently.

### Per-source-token processing

For each resolved source token, in order:

1. If its planned destination slot has no concrete address (only reachable
   via an `AFTER`-pending origin, G5 -- `kNestedDeclare`/`kUndeclared` were
   already rejected for the whole statement above), that ONE token is
   rejected with a G5-pending reason; the rest of the batch still runs.
2. If the source token does not exist (`Controller::token_exists`), reject
   that one token ("source token does not exist") -- this is the only
   place a nonexistent `kExplicit` source is actually caught.
3. Otherwise call `Controller::rekey(from, to)`, wrapped in try/catch: a
   destination collision (rekey's own "new_id already exists" guard --
   "no aliasing/forwarding" forbids merging two identities) or any other
   rekey failure rejects that ONE token with the primitive's own message;
   it does not abort the rest of the batch. Every reverse edge across all
   four relationship stores is repointed by `rekey` itself (already tested
   at the controller level; `update_core_test.cpp` re-confirms it through
   this op's own interface: structure and membership cascades, no
   orphaned old-address rows).

This per-token independence is the same discipline
`declare::execute_structure` uses for its per-member outcomes, and matches
NOTES.md "Arraying is universal": "ordering is semantic... inter-dependent
ops resolve because they execute in order."

### Same-address MOVE (decision, spec silent)

If a resolved source's planned destination slot is the SAME address it
already occupies, this module reports it as a no-op success (`moved =
true`) WITHOUT calling `Controller::rekey`. Calling `rekey(x, x)` would
spuriously trip its "new_id already exists" guard -- a check that exists
to catch a genuine two-identity merge, not a token that is (trivially)
already where it is asked to go. Not stated either way in the spec;
flagged here as the one behavioural call this module makes beyond direct
transcription.

## ADD_CONNECTION

### Interface

```cpp
struct ConnectionOutcome {
  bool added = false;
  std::optional<codec::Address> member;
  std::optional<codec::Address> group;
  std::string reason;  // set iff !added.
};
struct AddConnectionResult { std::vector<ConnectionOutcome> outcomes; };

AddConnectionResult add_connection(dbk::Controller &ctl, const command::AddConnection &op);
```

### Wildcard resolution and the both-sides cross-product

`op.group` and each of `op.elements` may independently be a terminal
wildcard (a partial-last-element address) or a plain address. Each is
resolved via a shared `expand_side` helper: a wildcard resolves via
`Controller::gather` (which reads straight off `token`, so its results
already exist -- no further existence check needed); a plain address is
used as-is after an explicit `Controller::token_exists` check (mirroring
`declare::resolve_reference`'s "must pre-exist" check).

**Both sides wildcard -> the full cross-product, BLESSED (Patrick,
2026-09-18).** NOTES.md's "Build-phase rulings" section states two
directions explicitly ("add wildcard-members to a group"; "a member into
wildcard groups") and now states the both-wildcard case too: "Both sides
wildcard (firmed 2026-09-18): full cross-product... M groups x N members =
M×N edges. A legitimate bulk operation." PLAN.md II.5 records the same
ruling. This module resolves both sides independently via the shared
`expand_side` helper and enumerates the full cross product of the group's
resolved set with each element's resolved set -- every gathered member
joins every gathered group, idempotently, endpoints still pre-existing.
Pinned as INTENDED behaviour (not merely tolerated) by
`update_core_test.cpp`'s explicit 2-groups x 3-members = 6-edges case.

### Failure granularity

- **Group fails to resolve** (a plain, nonexistent address; a wildcard
  cannot fail this way, since gather only returns tokens that already
  exist): whole-statement rejection, ONE outcome. NOTES.md: the group "is
  the scalar frame (must lead)" -- if it names nothing, there is nothing
  for any element to attach to.
- **One element entry fails to resolve**: that entry alone is rejected
  (one outcome, `member` set to the raw unresolved address, `group`
  empty); the other elements in `op.elements` are still attempted
  independently.
- **Every successfully-resolved (member, group) pair** is written via
  `Controller::add_membership` (door-idempotent -- `ON CONFLICT DO
  NOTHING`, both directions) and reported as its own successful outcome,
  even when the resolution came from a wildcard expanding to several
  tokens on either side.

## DELETE_RECORD / DELETE_CONNECTION

### Interface

```cpp
struct DeleteResult {
  bool deleted = false;
  std::string reason;  // set iff !deleted.
};

DeleteResult delete_record(dbk::Controller &ctl, const command::DeleteRecord &op,
                            const command::DeleteRecord &confirm);
DeleteResult delete_connection(dbk::Controller &ctl, const command::DeleteConnection &op,
                                const command::DeleteConnection &confirm);
```

### How confirmation is expressed

NOTES.md ("DELETE gates -- firmed 2026-09-18"): "the op echoes the exact
target... and requires explicit confirmation of THAT target before it
executes; never a bare/fire-and-forget delete, never a blanket confirm
flag." This module represents that literally in the type system: the
confirmation parameter is THE SAME IR TYPE as the op itself
(`command::DeleteRecord` / `command::DeleteConnection`), and the caller
must construct a value equal to the target field-for-field
(`confirm.token == op.token`, or `confirm.a == op.a && confirm.b ==
op.b`). There is no `bool confirm = true` shortcut anywhere in this
interface -- restating the exact target IS the only way to reach
execution. A mismatch rejects cleanly (`deleted = false`, a reason naming
the mismatch) without ever calling into the controller.

This is a code-structure decision (how the confirmation gate is
represented), not a semantic one: the actual gate -- exact-target-match,
local-only, no blanket flag -- is transcribed directly from NOTES.md.

### G10 -- reject-on-referenced, via an only-follow reference probe

`Controller::delete_token` has no cascade/repoint policy (PLAN.md G10 is
explicitly open, "flagged, not resolved"); left to its own devices it
issues a plain `DELETE`, which the database's own FK constraints would
reject if any of the four relationship stores still reference the token.

**This module does not let that happen and does not rely on the
database's error at all (Patrick, robustness ruling 2026-09-18).** Before
ever calling `delete_token`, `delete_record` probes the token with the
door's own only-follow reads -- `parents_of`, `children_of`,
`members_of`, `member_of` -- and rejects cleanly
(`DeleteResult{false, "...reject-on-referenced (G10)..."}`) if ANY of the
four is non-empty, WITHOUT attempting the delete at all. Only when all
four come back empty (the token is genuinely isolated) is `delete_token`
called.

This replaces an earlier version of this module that caught
`delete_token`'s thrown FK-violation exception and distinguished it from a
genuine error by sniffing the message text (Postgres's own wording,
"foreign key"/"violates") -- fragile (tied to the server's message
format, not a structured code) and not only-follow-idiomatic (it let a
`DELETE` attempt reach the database before deciding it should not have).
The probe is strictly stronger: given this schema's own reciprocal-write
discipline (`mint`'s "fold AND wire", `add_membership`'s both-directions
write), these four reads between them cover every FK column across all
four tables that could block the delete --

- `token_parent.token_id = X` (X carries its own PARENTS rows) <->
  `parents_of(X)` non-empty, directly;
- `token_child.child_token_id = X` (X has parents recorded) is the SAME
  fact as the row above, by construction (WIRE always accompanies LINK);
- `token_child.token_id = X` (X is used as someone else's constituent) <->
  `children_of(X)` non-empty, directly;
- `token_parent.parent_token_id = X` (the raw FK column that fact would
  actually violate) is guaranteed mirrored into the row above by `mint`'s
  WIRE step, so `children_of(X)` catches it too;
- `members.token_id = X` (X is a group with members) <->
  `members_of(X)` non-empty, directly;
- `member_of.group_token_id = X` (X is referenced as someone's group) is
  mirrored into the row above by `add_membership`'s both-directions write;
- `member_of.token_id = X` (X belongs to a group) <-> `member_of(X)`
  non-empty, directly;
- `members.member_token_id = X` (X is listed as someone's member) is
  mirrored into the row above by `add_membership`.

So the probe is complete, not a heuristic subset, PROVIDED the
reciprocal-write invariant holds -- which is exactly the guarantee this
whole schema is built to keep (deliberately redundant, sole-owner-written
tables; see `schema/schema.sql`'s own header). `delete_token` is no
longer wrapped in try/catch at all: any exception it does raise (a genuine
store/connection fault, since the FK case is now pre-empted) propagates
un-swallowed, same discipline as before. `update_core_test.cpp` exercises
all four axes independently (a token used as a constituent, a composite
carrying its own PARENTS, a group with a member, and a member of a
group), each confirmed via read-back that nothing was deleted.

### DELETE_RECORD's own existence check

Before calling `delete_token`, `delete_record` checks
`Controller::token_exists(op.token)` and rejects cleanly
("target token does not exist") if absent. This is necessary for an
ACCURATE result: a `DELETE` against a nonexistent PK affects zero rows and
raises no error, so without this check a nonexistent-target delete would
silently report `deleted = true`. Not stated in the spec either way (which
is silent on a delete against a target that was never there); the
existence check is the direct, no-extra-machinery way to keep the
`deleted` flag truthful.

### DELETE_CONNECTION -- the a/b -> member/group mapping

`command::DeleteConnection` (`command_ir.h`) is declared generically:

```cpp
struct DeleteConnection { codec::Address a; codec::Address b; };
```

with no stated member/group role for `a`/`b`. `Controller::add_membership`
and `Controller::delete_pair`, by contrast, both take an explicit
`(member, group)` parameter order. This module reads `op.a` as the member
and `op.b` as the group, matching that existing parameter order directly
-- the lowest-risk, no-invented-complexity mapping available, not a new
convention. Before calling `delete_pair`, `delete_connection` confirms the
edge actually exists IN THAT DIRECTION (`op.b` appears in
`Controller::member_of(op.a)`), which serves two purposes at once: (1) it
keeps `deleted` truthful (a `DELETE ... WHERE` matching no row is silent,
same reasoning as `DELETE_RECORD`'s existence check), and (2) it is the
practical validation of the `a = member` reading itself -- if the pair
were actually stored in the reverse role, this check reports "no such
membership edge exists" rather than doing nothing silently or deleting
the wrong thing. `delete_pair` then removes both the `member_of` row and
its `members` reciprocal in one atomic transaction (already the
controller's own contract); `update_core_test.cpp` confirms both sides
are gone afterward.

**Flagged, not a STOP**: this is a direct, unambiguous transcription of an
already-existing primitive's parameter order, not a genuinely open
semantic gap -- the brief's guidance to stop on genuinely unspecified
semantics did not seem to apply here, since there was exactly one
reasonable reading available (the one the door's own functions already
use) and no second plausible reading to choose between.

## What this module does NOT do

- Cross-`COMMAND` arraying (PLAN.md II.7) -- a later module (Agent 6).
- The peer/cross-network validation of a delete (tentative -> systemic
  across instances) -- resolved into WAL management per NOTES.md's
  2026-09-18 G7 ruling; only local full-target confirmation + local
  execution are built here.
- Any cache-tier behaviour -- out of scope for the whole record tier.
- Re-deriving `command::validate_*`'s own checks beyond the defensive
  re-run at the top of each function (same discipline as
  `declare::execute`): a caller that skips `command::validate_move_record`
  etc. first is not this module's contract to enforce beyond that.
