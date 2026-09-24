# dispatch

The record-tier verb dispatcher (PLAN.md II.8) and the arraying executor
(PLAN.md II.7): routes an already-parsed `command::` request to the right
core over the controller door, and runs an ordered stream of additive
commands.

## Files

- `dispatch.h` / `dispatch.cpp` -- the `Command` / `AdditiveCommand`
  variants, `dispatch_one()`, and `dispatch_stream()`.
- `dispatch_test.cpp` -- unit tests against a real, disposable local
  `hcp3_core` (same harness shape as `declare/declare_core_test.cpp`).

## What the arraying executor adds

**This is a real, required capability, not a stub or an optional layer.**
Per Patrick's ruling (2026-09-18): DELETE_RECORD and DELETE_CONNECTION are
the ONLY non-batchable record-tier ops, because they are destructive.
DECLARE, READ, MOVE_RECORD and ADD_CONNECTION are ALL batchable -- the
executor below is what makes that true across separate command
invocations, and it is built and tested for all four
(`dispatch_test.cpp`'s "4-verb chained stream" check runs a DECLARE, a
READ, a MOVE_RECORD and an ADD_CONNECTION in one ordered stream, each
depending on the previous item's write).

PLAN.md II.7 asks for "arraying is universal" -- an array is a compressed,
ordered serial stream of single commands sharing a scalar frame; frame
broadcast, co-index and SEE-idempotency collapse decompress it losslessly.
Reading the already-committed cores against that requirement:

- **`DeclareRecord`** already IS its own arrayed form: `PARENTS` is the
  member array and sets N; `NOTATION`/`MEMBER_OF` broadcast/co-index across
  it; `declare_core.cpp::execute_structure` loops over all N members,
  minting each (SEE-idempotent via the door) inside ONE `execute()` call.
- **`MoveRecord`** already carries `sources`, a vector of selectors
  (explicit / range / prefix) resolved and processed in order inside ONE
  `move_record()` call.
- **`AddConnection`** already carries `elements`, the arrayable side,
  processed inside ONE `add_connection()` call (plus the wildcard
  cross-product, also inside that one call).

So the frame-broadcast / co-index / SEE-collapse machinery the spec
describes is **already built**, entirely inside each single-op core, for
every op PLAN.md I.F names as arrayable. There is no separate "broadcast
step" left for an executor to perform on top of a `DeclareRecord` /
`MoveRecord` / `AddConnection` value -- doing so again here would
duplicate declare_core.cpp/update_core.cpp's own loops, not add anything.

What is **not** built anywhere below the dispatcher is a stream of several
DISTINCT command IR values -- e.g. "declare this, then move that" as one
ordered unit, where a later command may depend on an earlier one's effect
(the classic case: a MOVE targeting an address a preceding DECLARE in the
same stream just placed). That cross-command ordering is the genuinely
additional piece, and it is all `dispatch_stream()` does:

- `AdditiveCommand` is a `std::variant` over exactly the four arrayable
  verbs (`DeclareRecord`, `ReadRecord`, `MoveRecord`, `AddConnection`) --
  `DeleteRecordRequest`/`DeleteConnectionRequest` and the cache stubs are
  not alternatives of it, so "the destructive ops are not arrayable" is
  enforced by the type itself, not a runtime check.
- `dispatch_stream()` calls `dispatch_one()` once per stream item, **in
  the given order**, against the same `dbk::Controller` -- synchronous,
  single-threaded, nothing reordered or batched. Because each call runs to
  completion before the next starts, a later item sees every earlier
  item's writes, which is exactly "ordering is semantic" (NOTES.md
  "Arraying is universal": "inter-dependent ops resolve because they
  execute in order").
- One `Result` per stream item, in the same order as the input --
  preserving "one-result-per-request" per verb (PLAN.md I.G), just
  repeated across the stream.

`dispatch_test.cpp` has two stream checks: "arrayed stream" (DECLARE then
MOVE of what it just placed) and "4-verb chained stream" (DECLARE, READ,
MOVE_RECORD, ADD_CONNECTION, each depending on the previous item's write).
Both are built around dependency across DISTINCT verb invocations because
that is the one thing a same-verb array inside a single core cannot
express, and the one thing genuinely new here -- and because it is a real
capability every additive verb must support, not a special case for one
of them.

## Dispatch surface (no external wire)

`Command` is a `std::variant` over the full verb vocabulary (PLAN.md I.A):
the four record-tier ops above, `DeleteRecordRequest` /
`DeleteConnectionRequest` (each pairing the op with its required
`confirm`, per `update/update_core.h`'s specific-target confirmation
gate), and two empty marker structs (`UpdateCache`, `RebaseCache`) for the
cache tier's named-but-unbuilt verbs. **RECONCILE is no longer a
db/cache-manager verb** (removed 2026-09-22): it is an analyst →
WAL-manager message — the analyst messages the WAL manager directly, which
promotes the relevant pending queue into a priority in-box. See
`network/ENDPOINT-ACTIVATION-NOTES.md` "RECONCILE".

`dispatch_one(ctl, cmd)` is a plain function over these in-process C++
values -- there is no request line, no text grammar, and no parser here.
Driving a verb through the dispatcher means constructing the `command::`
struct (or wrapping it in `dispatch::Command`) directly in C++, exactly as
`dispatch_test.cpp` does. This satisfies PLAN.md II.8's bar for this stage
("the verbs are testable through the dispatch layer") without building the
external wire/framing format, which stays deferred (G6, firmed
2026-09-18) -- see NOTES.md "Request transport".

Every core still re-validates its own IR defensively (the same discipline
`declare_core.cpp`/`update_core.cpp` already follow): `dispatch_one` adds
no validation, no new rejection path, and no new semantics of its own --
it is pure routing plus, for the stream form, ordering.

## Build & run the tests

Requires libpq and a local Postgres reachable as the current OS user (peer
auth, no password), matching every other module's test harness.

```
g++ -std=c++17 -O2 -Wall -Wextra \
    -I. -I../codec -I../command -I../controller -I../declare -I../read \
    -I../update -I"$(pg_config --includedir)" \
    dispatch.cpp dispatch_test.cpp \
    ../codec/codec.cpp ../command/command_ir.cpp ../command/span_planner.cpp \
    ../controller/controller.cpp \
    ../declare/declare_core.cpp ../read/read_core.cpp ../update/update_core.cpp \
    -L"$(pg_config --libdir)" -lpq \
    -o dispatch_test

./dispatch_test               # uses ../schema/schema.sql
./dispatch_test /path/to/schema.sql   # optional override
```

Output is one `ok`/`FAIL` line per check, a `PASS`/`FAIL dispatch_test`
summary, and a non-zero exit on any failure. If no local Postgres is
reachable it prints a clear message and exits non-zero rather than
pretending to pass.
