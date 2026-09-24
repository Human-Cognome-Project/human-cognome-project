# API.md accuracy review — 2026-09-18

**Reviewer:** fresh independent adversary (no stake in outcome).
**Target:** `/opt/project/repo/engine/db_kernel/API.md` (read in full).
**Method:** every documented signature / field / type / behaviour checked against
the live headers and `.cpp` implementations, the schema, the seed driver, and
NOTES.md "Current build state" / "Build-phase rulings".

## Verdict

**PASS-WITH-FIXES.** The document is accurate to the code to an unusually high
degree: every request-struct field, every verb `execute()`/core signature, every
return-type struct, the full door-primitive table, the dispatch variants, the
wildcard per-verb table, the DELETE ruling, and the built-vs-deferred table all
match the code. The worked C++ example compiles against the real headers. I found
**no BLOCKER and no MAJOR** issues. One MINOR defect (F1) is worth fixing because
it names a non-public symbol an agent could try to call.

- **Must-fix:** F1 (minor, but it invents a callable `command::` symbol).

---

## Findings

### F1 — MINOR — `command::is_wildcard()` is presented as a callable public symbol; it is not, and it does not do what the sentence says

**API.md** §2 DELETE_RECORD, line 392–393:
> "**Specific id only — `command::is_wildcard()` rejects any wildcard** (terminal or not) on `token`."

Two inaccuracies in one sentence:

1. **Not a public symbol.** `is_wildcard` is defined in an *anonymous* namespace in
   `command/command_ir.cpp:26` and is not declared in `command/command_ir.h`.
   There is no `command::is_wildcard()` an agent can call or link against. The
   public surface that rejects a wildcard `token` is the validator
   `command::validate_delete_record` (`command_ir.cpp:380`), via the file-local
   helper `is_explicit_address` (`command_ir.cpp:30-32`).
2. **"terminal or not" mis-attributes the inline case.** `is_wildcard` only
   detects a *terminal* wildcard: `return !address.empty() && address.back().partial;`
   (`command_ir.cpp:26-28`). An *inline* (non-terminal) wildcard is not caught by
   `is_wildcard` at all — it is rejected because it is not a valid codec address
   (`codec::is_valid_address`, called inside `is_explicit_address`). So the inline
   rejection comes from `is_valid_address`, not from `is_wildcard`.

Net behaviour the sentence claims (a wildcard `token`, terminal or inline, is
rejected) **is correct** — `is_explicit_address` = `is_valid_address && !is_wildcard`
rejects both — and API.md states the inline-invalid-address rule correctly
elsewhere (§1 line 76-77, §4 line 527-530). Only this one parenthetical names the
wrong (and non-public) function.

**Fix:** reword to e.g. "`command::validate_delete_record` rejects any wildcard on
`token` (a terminal wildcard as non-explicit; an inline wildcard as an invalid
codec address)." Drop the `command::is_wildcard()` reference or mark it as an
internal helper.

---

## Spot-checks that PASSED (representative, not exhaustive)

**Request structs / fields / types — exact match to `command/command_ir.h`:**
- `DeclareRecord` (address/notation/parents/members/member_of + nested `MemberOf`
  {shared, per_member}) and `Reference` {address, nested} — API.md §2 lines 111-132
  vs `command_ir.h:166-209, 38-54`. ✓
- `ReadRecord` (anchor/direction/depth/exclusions/cache_shaped), `ReadAxis`,
  `ReadDirection` — §2 lines 200-214 vs `command_ir.h:216-239`. ✓
- `MoveSource` {Kind kExplicit/kRange/kPrefix, from, to} and `MoveRecord`
  {sources, destination} — §2 lines 272-283 vs `command_ir.h:259-316`. ✓
- `AddConnection` {group, elements}, `DeleteRecord` {token},
  `DeleteConnection` {a, b} — §2 vs `command_ir.h:321-339`. ✓

**Validator names — all six exist and are spelled correctly** (`command_ir.h:362-367`):
`validate_declare`, `validate_read`, `validate_move_record`,
`validate_add_connection`, `validate_delete_record`, `validate_delete_connection`. ✓

**DECLARE constraints — every rule matches `command_ir.cpp`:**
- structure floor `>= 2` (`:130`), grouping floor `>= 1` (`:143`), mixed node
  `N=1` (`:116`), NOTATION length `== N` (`:191`), `per_member` in-range-only
  (`:205-206`), ADDRESS covers N via `plan()` (`:244`), nested-declare ADDRESS
  segment rejected when PARENTS present (`:229-236`). All ✓.

**Verb execute()/core signatures and return structs — exact:**
- `declare::execute(...) -> declare::Result`; `Outcome{ingested,address,reason}` —
  `declare/declare_core.h:40-61`. ✓
- `dbread::read(...) -> dbread::ReadResult`; `ReachedVia`, `ReadNode{address,
  depth,via}`, `ReadStatus::kOk` only — `read/read_core.h:23-68`. ✓
- `update::move_record/add_connection` + `MoveOutcome`/`ConnectionOutcome`/results,
  and `delete_record`/`delete_connection(ctl, op, confirm)` + `DeleteResult` —
  `update/update_core.h:43-116`. ✓

**Door primitives table — every signature matches `controller/controller.h`:**
`token_exists`, `parents_of` (ParentEntry{ordinal,parent,mass}), `children_of`,
`members_of`, `member_of`, `attributes_of` (TokenAttributes{notation,mass}, no
`type`), both `gather` overloads (throw conditions correct), `mint` (full
signature incl. `std::optional<int> mass = std::nullopt`, returns true=existed /
false=minted), `add_membership`, `rekey`, `delete_token`, `delete_pair`. The
idempotency/gating notes (mint/add_membership SEE-idempotent; rekey/delete_token/
delete_pair raw, no gating) are true to the header comments and to `update_core`'s
op-layer gating. ✓

**Behaviour claims spot-verified in the `.cpp`, not just headers:**
- ADD_CONNECTION: unresolvable `group` → whole-statement reject; unresolvable
  element → per-entry reject; both-sides-wildcard M×N cross-product; idempotent
  add — `update_core.cpp:208-241`. ✓
- MOVE: same-address no-op reported success without calling `rekey`; destination
  collision rejected per-token; nested/undeclared destination rejected whole-
  statement — `update_core.cpp:143-162`. ✓
- DELETE_RECORD: confirm==op, token_exists check, G10 reject-on-referenced probe
  before `delete_token` — `update_core.cpp:265-298`. ✓
- DELETE_CONNECTION: a=member/b=group reading, edge-exists check via `member_of(a)`
  before `delete_pair` — `update_core.cpp:309-332`. ✓
- READ wildcard anchor via `gather`; terminal-wildcard exclusions via pure
  prefix comparison (`address_under_prefix`, no store access) —
  `read/read_core.cpp:27-51, 161-178`. This confirms API.md over the *stale*
  `command_ir.h:234` comment ("specific token_ids only"); API.md correctly
  documents the code, not the outdated comment. ✓

**Dispatch section — matches `dispatch/dispatch.h`:**
- `AdditiveCommand` = variant<DeclareRecord, ReadRecord, MoveRecord, AddConnection>
  (destructive ops structurally excluded) — `dispatch.h:76-77`. ✓
- `Command` full 9-way variant incl. `DeleteRecordRequest`/`DeleteConnectionRequest`
  {op, confirm} and `Reconcile`/`UpdateCache`/`RebaseCache` empty markers —
  `dispatch.h:52-84`. ✓
- `Result{Verb verb; variant<declare::Result, dbread::ReadResult, update::
  MoveResult, update::AddConnectionResult, update::DeleteResult, std::string>}`,
  `dispatch_one`, `dispatch_stream` — `dispatch.h:91-123`. ✓
- Cache stubs return `"<VERB>: not yet implemented"`, controller untouched — ✓
  (§2 line 474 / §5, matches dispatch.h:88-96, 105-106).

**No invention:**
- §5 correctly states the in-process IR is the only surface and G6 (external
  wire) is deferred — matches `command_ir.h:14-16`, `dispatch.h:19-22`, NOTES.md
  "Request transport (firmed 2026-09-18)". No endpoint/wire/format is invented. ✓
- The worked C++ example (§5 lines 553-577) uses real symbols only
  (`command::AddressSpan`, `AddressSegment::From`, `ConstituentList`,
  `Reference::ToAddress`, `dispatch::Command`, `dispatch_one`, `std::get<
  declare::Result>`) and the includes pull the needed decls; it would compile. ✓

**DELETE documented per the code/NOTES ruling** (local full specific-target
confirmation via re-supplied `confirm`, local execution, peer/cross-network
validation deferred to WAL) — API.md §2 DELETE_RECORD lines 393-410 and §9 match
`update_core.h:88-116` and NOTES.md "DELETE gates — firmed 2026-09-18". The
document does NOT use PLAN's earlier "validation-tags/peer-validated" phrasing. ✓

**Status accuracy** — §9 built-vs-deferred table matches NOTES.md "Current build
state" (all modules Built; G4/G5/G6/WAL/cache/mass/notation/prose→token_id/
extrapolation/G10 deferred). Wildcard per-verb table (§4) is correct: DECLARE no;
READ anchor+exclusions; MOVE source only; ADD either/both; DELETE_RECORD/
DELETE_CONNECTION hard no. ✓

**Seed / schema** — §7 (0x at AA.AA.AA.AA.AA mass 0; 16 hex atoms mass 1 via
`command::successor`; only-declared-mass path is `mint`'s optional param) matches
`seed/seed_0x.cpp:137-190`. §8/§9 "5 tables, COLLATE C, PK-only" matches
`schema/schema.sql`. ✓

## Could not verify / out of scope
- Green-test claims (§ intro "tests green") were not re-run; I verified source
  accuracy, not test execution.
- `codec.cpp` encode/decode internals were not audited beyond `is_valid_address`'s
  terminal-only-partial contract (`codec.h:105-107`), which the doc relies on and
  which holds.
