# db_kernel — record-tier API reference

**Status: record tier COMPLETE** (built 2026-09-17/18, on branch
`dbkernel-design-checkpoint`; commit `912d681` and prior on this branch).
Every module below — `codec/`, `schema/`, `controller/`, `command/`,
`declare/`, `read/`, `update/`, `dispatch/`, `seed/` — is built and tests
green against a live, disposable `hcp3_core` Postgres database.

"Record tier" means: the six live verbs — `DECLARE_RECORD`, `READ_RECORD`,
`MOVE_RECORD`, `ADD_CONNECTION`, `DELETE_RECORD`, `DELETE_CONNECTION` — are
fully designed, implemented, and adversary-reviewed, end to end from the
in-process request IR down to the Postgres store. The **cache tier**
(`RECONCILE`, `UPDATE_CACHE`, `REBASE_CACHE`) is named in the dispatch
surface but not built — it returns a stub result. Several other seams are
deliberately deferred. See [§9 Built vs deferred](#9-built-vs-deferred) for
the full list, and each core's own `README.md` for the reasoning behind
every point where the spec was silent and a call had to be made.

This document is for two audiences at once: a human who needs to
understand and call this API, and an agent building on it directly in
C++. Every signature quoted below is copied from the real headers, not
paraphrased; where prose and code could drift, the code wins (noted
inline where that happened while writing this document).

---

## 1. Model in brief

Enough of the governing model to use the API correctly. Full reasoning is
in `NOTES.md` ("Relationship model & type — firmed 2026-09-17", "Build-phase
rulings — firmed 2026-09-17/18") and `PLAN.md` Part I.

- **Address IS identity — no aliasing, no forwarding.** A token's address
  (`codec::Address`, an ordered sequence of base-50 `codec::AddressElement`
  couplets) *is* its identity. The same construction always yields the
  same address (deduplicated by `mint`'s own SEE-probe); two distinct
  addresses are never bridged to a common identity. This is why
  `Controller::rekey` refuses a destination address that already exists —
  that would merge two identities.

- **Only-follow, never search.** Every read resolves a token by its exact
  address (a primary-key lookup), then follows stored lists. There is no
  reverse-search index anywhere in the schema, by design, and no code
  path builds one. Terminal wildcards do not change this: they resolve to
  a **gather** — a single contiguous primary-key range scan — never a
  predicate scan.

- **Two orthogonal relationship axes, each a reciprocal pair.**
  - **Structure** — `parent` / `child`: a token's ordered literal
    constituents (downward, `token_parent`), and reciprocally the tokens
    that use it in their own composition (upward/reverse, `token_child`).
  - **Membership** — `members` / `member_of`: what a grouping directly
    contains (downward, `members`), and reciprocally the groups a token
    directly belongs to (upward, `member_of`).

  Both pairs are written together, both directions, by a single
  sole-owner call (`mint`'s "fold AND wire" for structure;
  `add_membership` for membership) — the reciprocal is never left to be
  derived by a reverse search.

- **Type is emergent and LoD-relative — never declared, never stored.**
  There is no `TYPE` field anywhere in the request grammar and no `type`
  column in the schema. A token reads as a grouping/label when it has
  `members` in scope for the current read, and as a terminal literal when
  it does not — the same token_id can be a coarse-LoD terminal and a
  finer-LoD label. `DECLARE_RECORD` validation is purely **structural**:
  whichever downward field is present (`PARENTS` ⇒ structure node,
  `MEMBERS` ⇒ grouping node) drives the rules; a node may be both at
  once.

- **Terminal wildcards are a GATHER, never a search.** A wildcard is a
  partial *trailing* address element (`codec::AddressElement::partial`,
  e.g. `A*`) — inline (non-terminal) wildcards are invalid addresses,
  rejected by `codec::is_valid_address` itself. Because the wildcard only
  ever frees the tail, it names one contiguous trunk/subtree, which is a
  contiguous range in primary-key order (guaranteed by `COLLATE "C"` on
  every address column — see `schema/schema.sql`). `Controller::gather`
  is the one primitive that walks such a range; MOVE's bulk source,
  ADD_CONNECTION's wildcard expansion, and READ's wildcard anchor all
  resolve through it.

- **Pairwise is the substrate.** Every stored relationship is a pairwise
  edge; the structure/membership split is storage detail beneath that one
  primitive, not a competing category. There is no connection-kind
  operand and no equivalence/kind table anywhere — the "kind" of a
  relationship is which axis (which table) it sits on, never a stored
  label.

- **Mass is derived, never declared — except the seed floor.** No verb
  request carries a mass operand. `DECLARE_RECORD` always writes mass
  blank (SQL `NULL`); aggregation is later, unbuilt, manager work. The
  *only* declared masses in the whole system are the 16 hex atoms (mass
  1) and `0x` (mass 0), written once by `seed/seed_0x.cpp` through
  `Controller::mint`'s optional `mass` parameter — a bootstrap channel,
  not something `DECLARE_RECORD` or any core exposes.

---

## 2. The verb surface

All six verbs are structs in `command/command_ir.h`, validated by a
`command::validate_*` function in the same header, and executed by a core
in `declare/`, `read/`, or `update/`. Every verb's request IR is
constructed directly in C++ — there is no text/wire parser (see §6).

### DECLARE_RECORD

**Request** — `command::DeclareRecord` (`command/command_ir.h`):

```cpp
struct DeclareRecord {
  std::optional<AddressSpan> address;
  std::vector<std::optional<std::string>> notation;
  std::optional<std::vector<ConstituentList>> parents;   // ConstituentList = vector<Reference>
  std::optional<std::vector<Reference>> members;
  struct MemberOf {
    std::vector<Reference> shared;
    std::map<std::size_t, std::vector<Reference>> per_member;
  };
  std::optional<MemberOf> member_of;
};
```

A `Reference` (one PARENTS/MEMBERS/MEMBER_OF entry) is exactly one of a
plain address or a full nested `DeclareRecord`:

```cpp
struct Reference {
  std::optional<codec::Address> address;
  std::shared_ptr<DeclareRecord> nested;
};
```

**Semantics.**

- `PARENTS` sets `N` (its outer length). Each `PARENTS[i]` is a
  `ConstituentList` — the ordered constituents minted at that slot.
  `NOTATION` and `MEMBER_OF` are read positionally against the same index
  `i` when present.
- A node with neither `PARENTS` nor `MEMBERS` is rejected — one of the
  two downward fields is always required.
- **Grouping-node naming literal** (`MEMBERS` present): established one
  of two structurally distinguished ways —
  - **use-provided-ID**: `PARENTS` absent, `ADDRESS` present — references
    a pre-existing naming literal (checked for existence at execution
    time, not by the IR validator);
  - **mint**: `PARENTS` present — mints a new naming literal from that
    composition, placed at `ADDRESS` if given, else "manager-placed"
    (**not implemented** — rejected cleanly at execution time; there is
    no next-slot mechanism anywhere in this codebase yet, see G4/G5 in
    §9).
  - Neither `PARENTS` nor `ADDRESS` on a grouping node ⇒ rejected.
- **Constraints:** structure floor `>= 2` constituent references per
  `PARENTS` member (anti-alias floor); grouping floor `>= 1` on
  `MEMBERS`; when both `PARENTS` and `MEMBERS` are present, `PARENTS`
  must declare exactly `N = 1` (a naming-literal mint is a single
  literal); `NOTATION`, when present, must have length exactly `N`
  (blank/`nullopt` entries are legal); `MEMBER_OF`'s `per_member` map is
  checked only for in-range indices (`< N`), never against `N`'s full
  size; `ADDRESS` must cover `N` slots exactly, via
  `command::plan()` (§ ADDRESS span, below); a nested-declare `ADDRESS`
  segment is rejected whenever the same node also carries `PARENTS` (it
  would silently drop that slot's own `PARENTS[i]` composition).
- **No `TYPE` field anywhere**; no mass operand.

**Validator:** `command::validate_declare(const DeclareRecord &)`.

**Execution:** `declare::execute(dbk::Controller &ctl, const command::DeclareRecord &node) -> declare::Result` (`declare/declare_core.h`).

**Return contract** (`declare::Result` / `declare::Outcome`):

```cpp
struct Outcome {
  bool ingested = false;
  std::optional<codec::Address> address;  // set iff ingested — the ACTUAL assigned address
  std::string reason;                     // set iff !ingested
};
struct Result { std::vector<Outcome> outcomes; };
```

- `PARENTS` present ⇒ one `Outcome` per `PARENTS` member, in order.
- `PARENTS` absent (pure grouping node) ⇒ exactly one `Outcome`, the
  resolved naming-literal token's address.
- `address` is always the address `mint()` actually placed, never an echo
  of a requested `@`-pin (no pin-override mechanism exists yet).
- Rejection is per-member/per-point (`ingested = false`, `reason` set, no
  exception) — an ordinary rejection never throws; only a genuine
  store/connection error throws (via the controller).
- Nesting is recursive: any relationship field may be a full nested
  `DeclareRecord`; a nested reference is minted **eagerly**, at the
  moment it is resolved, so an already-committed nested mint is *not*
  rolled back if a sibling reference in the same outer member later fails
  (see `declare/README.md` "Atomicity boundary" for the exact case).

### READ_RECORD

**Request** — `command::ReadRecord`:

```cpp
enum class ReadAxis { kParents, kMembers, kBoth };

struct ReadDirection {
  std::optional<ReadAxis> axis;  // nullopt = RADIAL (default): all directions
  bool reverse = false;          // an orientation on the narrowed axis, not a peer value
};

struct ReadRecord {
  codec::Address anchor;                  // a single token_id, or a terminal wildcard
  ReadDirection direction;                // optional; default = radial
  unsigned depth = 0;                     // LoD dial; depth 0 = rolled-up (anchor only)
  std::vector<codec::Address> exclusions; // specific token_ids, or terminal wildcards
  bool cache_shaped = false;              // named only; the core always runs raw-radial
};
```

**Semantics.**

- **Anchor** — a concrete `token_id`, or a terminal wildcard (resolved via
  `Controller::gather` to every existing token under that trunk; each
  resolved token is then read as its own depth-0 root, concatenated in
  gather's PK order).
- **Direction** — unset ⇒ RADIAL: all four follows walked
  (`parents_of`/`children_of`/`members_of`/`member_of`), `reverse`
  ignored. `kParents` narrows to structure (`parents_of` forward,
  `children_of` reverse); `kMembers` narrows to membership (`members_of`
  forward, `member_of` reverse); `kBoth` walks both narrowed axes.
- **Depth** — a plain hop-count budget from the anchor; depth 0 returns
  only the anchor.
- **Exclusions** — pruned from the result *and* not expanded further; a
  specific `token_id` (exact match) or a terminal wildcard (checked by
  pure address-prefix comparison against nodes already reached — no
  store access).
- **cache_shaped** is a named face field only — the raw-radial core
  ignores it; it always executes cache-free.

**Validator:** `command::validate_read(const ReadRecord &)`.

**Execution:** `dbread::read(dbk::Controller &ctl, const command::ReadRecord &op) -> dbread::ReadResult` (`read/read_core.h`).

**Return contract:**

```cpp
enum class ReachedVia { kAnchor, kParent, kChild, kMembers, kMemberOf };

struct ReadNode {
  codec::Address address;
  unsigned depth = 0;
  ReachedVia via = ReachedVia::kAnchor;
};

enum class ReadStatus { kOk };  // the only value — no distinct error status

struct ReadResult {
  ReadStatus status = ReadStatus::kOk;
  std::vector<ReadNode> nodes;
};
```

**Linearized, once-per-path, no dedup**: a node reached at two distinct
points in the traversal (e.g. two composites sharing a constituent)
produces two separate `ReadNode` entries at their own positions — nothing
is deduplicated. Follow order within a node is the fixed convention
`kParent, kChild, kMembers, kMemberOf`. A nonexistent anchor, or a
wildcard anchor over an empty region, returns `kOk` with an empty
`nodes` vector, not a distinct error.

### MOVE_RECORD

**Request** — `command::MoveRecord`:

```cpp
struct MoveSource {
  enum class Kind { kExplicit, kRange, kPrefix };
  Kind kind = Kind::kExplicit;
  codec::Address from;                  // kExplicit: the token. kRange: range start. kPrefix: the wildcard.
  std::optional<codec::Address> to;     // kRange only: the inclusive range end.
};

struct MoveRecord {
  std::vector<MoveSource> sources;
  AddressSpan destination;
};
```

**Semantics.**

- `sources` is the arrayable side: an explicit token, a `FROM..TO` range,
  and a terminal-wildcard prefix may all coexist in the same statement.
  Each resolves to zero or more existing tokens (`kExplicit` contributes
  its address unchecked at this stage — existence is checked per-token at
  placement time; `kRange`/`kPrefix` resolve via `Controller::gather`, so
  everything they return already exists). Multiple selectors' resolved
  token lists are concatenated in selector order, each selector's own
  tokens in gather's PK order.
- `destination` is an `AddressSpan` (§ ADDRESS span) — the same grammar
  DECLARE's `ADDRESS` uses.
- **Cover-N.** `N` = the resolved source count. All-explicit sources ⇒
  `N` known statically ⇒ cover-N checked at IR-validation time
  (`command::validate_move_record`, via `command::plan`). Any
  range/prefix source ⇒ `N` depends on the live store ⇒ cover-N is
  skipped at IR level and checked by the UPDATE core once sources are
  resolved. A cover-N mismatch rejects the whole statement as one
  `MoveOutcome`.
- **Destination mints nothing.** A relocation is bookkeeping-only
  (topology-invariant, re-key + cascade-repoint, no edge added or
  removed). The core rejects the whole statement outright if
  `destination` contains a `kNestedDeclare` or `kUndeclared` segment —
  neither assigns anything a MOVE could re-key onto.
- Per-source-token processing is independent: a token whose destination
  slot is only reachable via a pending `AFTER` origin (G5), a nonexistent
  explicit source, or a destination collision (`rekey`'s own "new_id
  already exists" guard) is rejected individually; the rest of the batch
  still runs.
- A source whose planned destination is the address it already occupies
  is reported as a no-op success without calling `rekey` (avoids
  spuriously tripping the collision guard against itself).

**Validator:** `command::validate_move_record(const MoveRecord &)`.

**Execution:** `update::move_record(dbk::Controller &ctl, const command::MoveRecord &op) -> update::MoveResult` (`update/update_core.h`).

**Return contract:**

```cpp
struct MoveOutcome {
  bool moved = false;
  std::optional<codec::Address> from;
  std::optional<codec::Address> to;
  std::string reason;  // set iff !moved
};
struct MoveResult { std::vector<MoveOutcome> outcomes; };
```

### ADD_CONNECTION

**Request** — `command::AddConnection`:

```cpp
struct AddConnection {
  codec::Address group;                 // the scalar frame — must lead
  std::vector<codec::Address> elements; // the arrayable side, >= 1
};
```

**Semantics.**

- Asserts membership: each element becomes a member of `group`. No
  connection-kind operand — mints nothing; both endpoints must already
  exist as tokens.
- `group` and/or any entry in `elements` may independently be a terminal
  wildcard, resolved via `Controller::gather`; a plain address is used
  as-is after an explicit `Controller::token_exists` check.
- **Both sides wildcard ⇒ full cross-product (firmed 2026-09-18,
  intended behaviour, not merely tolerated).** M resolved groups × N
  resolved elements = M×N edges, every gathered member joining every
  gathered group.
- **Failure granularity:** an unresolvable `group` rejects the whole
  statement (one outcome) — nothing for any element to attach to. An
  unresolvable individual element entry rejects only that entry; the rest
  are attempted independently. Every write goes through
  `Controller::add_membership`, which is door-idempotent (`ON CONFLICT DO
  NOTHING`, both directions) — a re-add is a clean success, not an error.

**Validator:** `command::validate_add_connection(const AddConnection &)`.

**Execution:** `update::add_connection(dbk::Controller &ctl, const command::AddConnection &op) -> update::AddConnectionResult` (`update/update_core.h`).

**Return contract:**

```cpp
struct ConnectionOutcome {
  bool added = false;
  std::optional<codec::Address> member;
  std::optional<codec::Address> group;
  std::string reason;  // set iff !added
};
struct AddConnectionResult { std::vector<ConnectionOutcome> outcomes; };
```

### DELETE_RECORD

**Request** — `command::DeleteRecord`:

```cpp
struct DeleteRecord { codec::Address token; };
```

**Semantics.**

- **Single instruction only, never arrayed** — structurally excluded from
  `dispatch::AdditiveCommand` (§4), not merely a runtime check.
- **Specific id only.** `command::validate_delete_record` rejects `token`
  unless it is an explicit, fully-specified address (no wildcard,
  terminal or otherwise, and no other malformed address) — no bulk/range
  delete exists.
- **Gated behind full specific-target confirmation** (NOTES.md "DELETE
  gates — firmed 2026-09-18"): the caller must resupply the exact target
  as a second, separate value — re-stating the target *is* the
  confirmation mechanism, never a bare `confirm = true` flag.
- **Reject-on-referenced (G10).** Before ever calling
  `Controller::delete_token`, the core probes `parents_of` /
  `children_of` / `members_of` / `member_of`; if any is non-empty the
  delete is rejected cleanly (the token is still referenced somewhere)
  without touching the store. Only a token with all four empty (fully
  isolated) is actually deleted.
- Checks `Controller::token_exists` first and rejects cleanly on a
  nonexistent target (a `DELETE` against a missing PK affects zero rows
  and raises no error, so this check is what keeps the `deleted` flag
  truthful).
- Peer/cross-network validation (tentative → systemic across instances)
  is **not** built here — the delete report is booked by the WAL manager
  (**built**, `wal/`; see §9); the cross-network validation act itself is not
  run by it and remains deferred swarm-side.

**Validator:** `command::validate_delete_record(const DeleteRecord &)`.

**Execution:**

```cpp
DeleteResult delete_record(dbk::Controller &ctl, const command::DeleteRecord &op,
                            const command::DeleteRecord &confirm);
```

`confirm` must equal `op` exactly (`confirm.token == op.token`); a
mismatch rejects cleanly with no store access.

**Return contract:**

```cpp
struct DeleteResult {
  bool deleted = false;
  std::string reason;  // set iff !deleted
};
```

### DELETE_CONNECTION

**Request** — `command::DeleteConnection`:

```cpp
struct DeleteConnection { codec::Address a; codec::Address b; };
```

**Semantics.**

- **Single, specific pair only** — same non-arrayable, no-wildcard,
  full-confirmation discipline as DELETE_RECORD.
- **Membership axis only.** Removes the named edge and its stored
  reciprocal (both-direction removal, via `Controller::delete_pair`).
  Structure edges are never surgically removed — the whole owning
  composite must go through DELETE_RECORD.
- `a` is read as the **member**, `b` as the **group** — matching
  `Controller::add_membership`/`delete_pair`'s existing `(member, group)`
  parameter order (`DeleteConnection` itself declares no role for `a`/`b`;
  this is the module's own reading, transcribed directly from the
  door's parameter order, not a new convention).
- Confirms the edge actually exists in that direction (`b` appears in
  `member_of(a)`) before calling `delete_pair` — keeps `deleted` truthful
  and doubles as validation of the `a = member` reading.

**Validator:** `command::validate_delete_connection(const DeleteConnection &)`.

**Execution:**

```cpp
DeleteResult delete_connection(dbk::Controller &ctl, const command::DeleteConnection &op,
                                const command::DeleteConnection &confirm);
```

`confirm` must equal `op` exactly (`confirm.a == op.a && confirm.b ==
op.b`).

### Cache-tier stubs: RECONCILE / UPDATE_CACHE / REBASE_CACHE

Named face entries only — `dispatch::Reconcile`, `dispatch::UpdateCache`,
`dispatch::RebaseCache` are empty marker structs (`dispatch/dispatch.h`).
`dispatch_one` routes them to a non-fatal stub `Result` carrying the
string `"<VERB>: not yet implemented"` — the controller is never touched.
No IR fields, no validator, no core exists behind any of the three. See
§9.

---

## 3. Arraying

Per NOTES.md "Arraying is universal": an array **is** a compressed,
ordered *serial* command stream sharing a scalar frame, not an unordered
batch. `DECLARE`, `READ`, `MOVE_RECORD`, and `ADD_CONNECTION` are all
batchable; `DELETE_RECORD`/`DELETE_CONNECTION` are the sole non-batchable
ops, because they are destructive.

This shows up at two distinct layers, and both are built:

1. **Intra-command arraying** — already inside each single-op core, with
   no separate machinery needed on top:
   - `DeclareRecord::parents`' outer length **is** the member array
     (sets `N`); `NOTATION`/`MEMBER_OF` broadcast/co-index across it;
     `declare::execute` mints every member inside one call.
   - `MoveRecord::sources` is a vector of selectors, resolved and
     processed in order inside one `move_record()` call.
   - `AddConnection::elements` is the arrayable side (plus the wildcard
     cross-product), processed inside one `add_connection()` call.
2. **Cross-command arraying — the arraying executor** (`dispatch/dispatch.h`,
   PLAN.md II.7): an ordered stream of **distinct** command IR values,
   where a later command may depend on an earlier one's effect (the
   classic case: a MOVE targeting an address a preceding DECLARE in the
   same stream just placed).

```cpp
using AdditiveCommand = std::variant<command::DeclareRecord, command::ReadRecord,
                                      command::MoveRecord, command::AddConnection>;

std::vector<Result> dispatch_stream(dbk::Controller &ctl,
                                     const std::vector<AdditiveCommand> &stream);
```

`AdditiveCommand` is a `std::variant` over exactly the four arrayable
verbs — `DeleteRecordRequest`/`DeleteConnectionRequest` and the cache
stubs are not alternatives of it, so "the destructive ops are not
arrayable" is enforced by the type system, not a runtime check.
`dispatch_stream` calls `dispatch_one` once per item, **in the given
order**, against the same `dbk::Controller`, synchronously — a later
item always sees every earlier item's writes ("ordering is semantic").
One `Result` per item, in input order.

---

## 4. Wildcards

Terminal-only, globally — a wildcard is a partial *trailing*
`codec::AddressElement` (e.g. `A*`); `codec::is_valid_address` rejects an
inline (non-terminal) wildcard outright, so every validator inherits the
rule for free. A wildcard resolves via `Controller::gather` (§7) — a
single contiguous primary-key range scan, never a predicate scan.

Per-verb rules:

| Verb | Wildcard allowed | Where |
|---|---|---|
| `DECLARE_RECORD` | No | Constituent/member references are always concrete addresses (or nested declares). |
| `READ_RECORD` | Yes — **anchor**, and **exclusions** | Resolved via `gather`; a nominal, tree-constrained region read, not a property-predicate scan. |
| `MOVE_RECORD` | Yes — **source** only (`kRange`/`kPrefix`) | The destination is always an `AddressSpan`, never a wildcard. |
| `ADD_CONNECTION` | Yes — **group and/or elements**, either or both | Both-sides-wildcard is the full M×N cross-product (blessed, intended). |
| `DELETE_RECORD` | **No — hard rule** | `token` must be a concrete, explicit address. |
| `DELETE_CONNECTION` | **No — hard rule** | Both `a` and `b` must be concrete, explicit addresses. |

---

## 5. Driving the verbs (for agents)

There is **no external wire format yet** — building one is a named,
deferred seam (G6; see §9). The only surface today is the **in-process
IR**: construct the `command::` struct(s) directly in C++, route them
through `dispatch::`, and read back a typed result. This is exactly how
`dispatch_test.cpp` and every core's own `*_test.cpp` drive the API.

```cpp
#include "controller.h"
#include "command_ir.h"
#include "dispatch.h"

dbk::Controller ctl("dbname=hcp3_core");

// Build a DECLARE_RECORD IR value directly.
command::DeclareRecord node;
node.address = command::AddressSpan{command::AddressSegment::From(start_addr)};
node.parents = std::vector<command::ConstituentList>{
    {command::Reference::ToAddress(a), command::Reference::ToAddress(b)}};

// Wrap in the verb-surface variant and dispatch.
dispatch::Command cmd = node;
dispatch::Result result = dispatch::dispatch_one(ctl, cmd);

// result.value holds a declare::Result for this verb.
const auto &declared = std::get<declare::Result>(result.value);
for (const auto &outcome : declared.outcomes) {
  if (outcome.ingested) {
    // outcome.address is the token's actual assigned address.
  }
}
```

`dispatch::Command` is the full verb-surface variant:

```cpp
struct DeleteRecordRequest { command::DeleteRecord op; command::DeleteRecord confirm; };
struct DeleteConnectionRequest { command::DeleteConnection op; command::DeleteConnection confirm; };
struct Reconcile {};
struct UpdateCache {};
struct RebaseCache {};

using Command =
    std::variant<command::DeclareRecord, command::ReadRecord, command::MoveRecord,
                 command::AddConnection, DeleteRecordRequest, DeleteConnectionRequest,
                 Reconcile, UpdateCache, RebaseCache>;

struct Result {
  Verb verb;
  std::variant<declare::Result, dbread::ReadResult, update::MoveResult,
               update::AddConnectionResult, update::DeleteResult, std::string>
      value;
};

Result dispatch_one(dbk::Controller &ctl, const Command &cmd);
std::vector<Result> dispatch_stream(dbk::Controller &ctl,
                                     const std::vector<AdditiveCommand> &stream);
```

`dispatch_one` performs **no validation and adds no semantics of its
own** — it is pure routing (`DeclareRecord → declare::execute`,
`ReadRecord → dbread::read`, `MoveRecord → update::move_record`,
`AddConnection → update::add_connection`, `DeleteRecordRequest →
update::delete_record`, `DeleteConnectionRequest →
update::delete_connection`, the three cache markers → a stub `Result`).
Every core re-validates its own IR defensively on entry
(`command::validate_*`), so calling a core directly (bypassing
`dispatch::`) is equally safe — `dispatch::` is a convenience routing
layer, not a required gate.

---

## 6. Door primitives (for agents building directly on the controller)

`dbk::Controller` (`controller/controller.h`) is the **sole writer** to
`hcp3_core` — communications-only; nothing outside `controller/` touches
the tables directly. One libpq connection per `Controller` instance
(`explicit Controller(const std::string &conninfo)`, throws
`std::runtime_error` on connection failure).

**Reads — only-follow, PK lookups, never search:**

| Signature | What it does |
|---|---|
| `bool token_exists(const codec::Address &token_id)` | Direct-address probe: PK lookup on `token`. |
| `std::vector<ParentEntry> parents_of(const codec::Address &token_id)` | Ordered constituents (structure, down); each entry carries `ordinal`, `parent`, and `mass` (`nullopt` = not yet computed). |
| `std::vector<codec::Address> children_of(const codec::Address &token_id)` | Reverse structure adjacency (up); unordered in the schema, returned sorted for determinism. |
| `std::vector<codec::Address> members_of(const codec::Address &token_id)` | Membership, down: what a group directly contains; sorted for determinism. |
| `std::vector<codec::Address> member_of(const codec::Address &token_id)` | Membership, up: the groups a token directly belongs to; sorted for determinism. |
| `std::optional<TokenAttributes> attributes_of(const codec::Address &token_id)` | The token's own `notation`/`mass` pass-through; `nullopt` if the token does not exist. No `type` field — dropped from the schema. |

All five key on the leading PK column (`token_id`) — no secondary index
is ever touched.

**Gather — the terminal-wildcard / range enumerator:**

| Signature | What it does |
|---|---|
| `std::vector<codec::Address> gather(const codec::Address &prefix)` | Prefix must end in a partial element. Returns every existing token under that trunk, in PK order. Throws `std::runtime_error` if `prefix` does not end in a partial element. |
| `std::vector<codec::Address> gather(const codec::Address &from, const codec::Address &to)` | Both must be valid, fully-specified (non-partial) addresses. Returns every existing token in the inclusive `[from, to]` range, PK order. `from > to` matches nothing (not an error). Throws on an empty or partial endpoint. |

Both overloads are a single bounded PK-range scan (only possible because
every address column is `COLLATE "C"`, pinning PK order to
`codec::kAlphabet`'s byte order) — reads `token` only, never follows
parents/children/members.

**Writes — sole owner:**

| Signature | What it does |
|---|---|
| `bool mint(const codec::Address &token_id, const std::string &notation, const std::vector<Constituent> &constituents, std::optional<int> mass = std::nullopt)` | See-mint-link-wire, one atomic transaction. Returns `true` if `token_id` already existed (idempotent no-op), `false` if freshly minted. Throws on any DB error (rollback first) — e.g. a constituent that does not already exist (FK). `mass` is a plain pass-through; `nullopt` writes SQL `NULL`. |
| `void add_membership(const codec::Address &member, const codec::Address &group)` | Writes `member_of` (up) and `members` (down) reciprocally, one atomic transaction. Idempotent (`ON CONFLICT DO NOTHING` each side) — heals a one-sided row without disturbing the side already present. Both endpoints must already be live tokens (FK; mints nothing). |

**Mutation primitives — raw; no op-layer gating (that lives in `update/`):**

| Signature | What it does |
|---|---|
| `void rekey(const codec::Address &old_id, const codec::Address &new_id)` | Re-keys a token and cascade-repoints every referencing row across `token_parent`, `token_child`, `members`, `member_of`, one atomic transaction (insert new row, repoint, delete old row — never updates a still-referenced PK in place). Throws if `old_id` does not exist, or if `new_id` already exists (would merge two identities). |
| `void delete_token(const codec::Address &id)` | Deletes the `token` row. If any FK still references it, the database's default (no-cascade) behaviour applies as-is — the call throws. No cascade/repoint policy is invented here (open seam G10). |
| `void delete_pair(const codec::Address &member, const codec::Address &group)` | Removes one membership edge and its reciprocal (`member_of` + `members`), one atomic transaction. Addresses the membership axis only — structure edges are never removed by this primitive. |

Idempotency/gating properties to note when building directly on these:
`mint` and `add_membership` are SEE-idempotent (safe to call repeatedly);
`rekey`, `delete_token`, and `delete_pair` carry **no gating** at all —
no active-confirmation, no reject-on-referenced check, no
peer-validation. The `update::` core's op-layer behaviour (full
specific-target confirmation, the G10 reject-on-referenced probe) is what
makes DELETE safe to expose to a caller; calling `delete_token`/
`delete_pair` directly bypasses all of that.

---

## 7. Bootstrap

There is no manager-placed address assignment yet (G4/G5, open). The
*only* way tokens exist in a fresh `hcp3_core` is the seed floor,
written once by `seed/seed_0x.cpp` directly through `Controller::mint`'s
optional `mass` parameter — never through `declare::execute` or
`dispatch::`, since those build on top of a store that already has a
floor to ground constituent references against:

- `0x` at `AA.AA.AA.AA.AA`, mass `0` (declared, the one explicit
  exception — there is nothing to derive at the floor).
- The 16 hex atoms (`0`–`F`), mass `1` each, placed sequentially after
  `0x` in the same trunk via `command::successor` (the base-50 address
  increment).

Every other constituent reference `declare::execute` resolves ultimately
traces back to one of these 17 seed tokens, or to a composite built (via
DECLARE) on top of them.

---

## 8. Module map

| Directory | Provides |
|---|---|
| `codec/` | `codec::Address`/`AddressElement`, encode/decode, `is_valid_address`. Pure transforms, no storage. |
| `schema/` | `schema.sql` — the 5 tables (`token`, `token_parent`, `token_child`, `members`, `member_of`), `COLLATE "C"` address columns, PK-only indexes. |
| `controller/` | `dbk::Controller` — the sole door (§6). |
| `command/` | The IR (`command_ir.h`), its `validate_*` functions, and the `AddressSpan` planner (`span_planner.h`: `plan()`, `successor()`, `span_length()`, `validate_span_shape()`). |
| `declare/` | The DECLARE core. |
| `read/` | The READ core. |
| `update/` | The UPDATE core (four sub-ops). |
| `dispatch/` | The verb dispatcher and arraying executor (§5, §3). |
| `seed/` | `seed_0x` — the bootstrap seed floor (§7). |

Each directory's own `README.md` goes deeper than this document,
including every point where the spec was silent and the reasoning behind
the call made there.

---

## 9. Built vs deferred

| Area | Status |
|---|---|
| `codec/` — address/token_id transforms | **Built.** |
| `schema/` — 5-table store, `COLLATE "C"` | **Built.** |
| `controller/` — reads, gather, mint/add_membership, rekey/delete_token/delete_pair | **Built.** |
| `command/` — IR + structural validation + `AddressSpan` planner | **Built.** |
| `declare/` — DECLARE core (structure, grouping, nesting, SEE idempotency) | **Built.** Manager-placed mint (ADDRESS omitted) rejected pending G4. |
| `read/` — READ core (raw radial, axes, depth, exclusions, wildcard anchor) | **Built.** Cache-shaped mode out of scope regardless of the request flag. |
| `update/` — MOVE_RECORD, ADD_CONNECTION, DELETE_RECORD, DELETE_CONNECTION | **Built.** |
| `dispatch/` — verb dispatch + arraying executor | **Built.** No external wire (see G6). |
| `seed/` — seed floor | **Built.** |
| **G4 — next-slot mechanism** | Deferred. Analyst-supplied-start vs. per-trunk cursor — Patrick's open decision. Gates manager-placed mint. |
| **G5 — block boundaries past "hex couplets"** | Deferred. The trunk→kind map's extent past the hex-couplet kind is unfixed. |
| **G6 — external wire/transport format** | Deferred. The in-process IR (§5) is the current surface; no text/file/network framing exists. |
| **WAL manager** (`wal/` — a bookkeeper/observer over WAL reports; door surface `open`/`close`/`is_open`/`list_open`/`record_seen`, all against its own `wal_manager` DB, never `hcp3_core`) | **Built.** Books return-path + mass obligations from a change's own data and monitors for their settling writes (self-accounting, no drain, only-follow). See `wal/README.md`, `wal/USAGE.md`. The cross-network DELETE validation act, the cache manager's file-now/wire-later runtime, and the swarm side remain deferred. |
| **Cache tier** (`RECONCILE`/`UPDATE_CACHE`/`REBASE_CACHE`) | Deferred. Named face entries + dispatch stubs only; no mechanism designed. |
| **Mass aggregation** (sum-vs-centroid, nested aggregation) | Deferred. `DECLARE_RECORD` always writes mass blank. |
| **Notation derivation** (surface-from-parents) | Deferred. `NOTATION` is stored exactly as given (or blank), never derived. |
| **Prose → token_id swap** (temporary label handles) | Deferred bookkeeping item, parked on purpose. |
| **Extrapolation / relative-placement rules** | Deferred (the inert `kUndeclared` hook exists in the IR; nothing resolves it). |
| **G10 — DELETE_RECORD FK-dependent policy on a still-referenced token** | Held as reject-on-referenced (no invented cascade/repoint); flagged, not fully resolved. |
