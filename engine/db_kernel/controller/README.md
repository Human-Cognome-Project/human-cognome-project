# hcp3_core write/mint controller

The single **door** to the `hcp3_core` token-graph store. Sole-owner writer,
communications-only: callers go through the controller and never touch the
tables directly. It is the C++ layer the schema header calls "a later
write/mint controller (fold AND wire)."

Binds **directly to a real Postgres `hcp3_core` database over libpq** — not an
in-memory or virtual store. The data is disposable/consumptive, so there is no
durability, migration, or persistence-abstraction machinery: the controller
speaks plain SQL against real tables and keeps it simple.

Standalone: it builds, runs, and tests on its own. Its only dependency is the
codec (`../codec/`), which owns all address ↔ token_id handling. It does not
assume it is the only resident process; it just opens its own connection.

Files mirror `../codec/`'s shape:

- `controller.h` / `controller.cpp` — the door plus the read/write ops.
- `controller_test.cpp` — a standalone check harness against a real,
  disposable local Postgres.
- `controller_advtest.cpp` — an adversarial harness over the same store
  (rollback, recovery, derivation, injection, distinct-reverse, idempotency).
- `README.md` — this file.

## Interface

`dbk::Controller` opens one libpq connection from a conninfo string
(e.g. `"dbname=hcp3_core"`), and throws `std::runtime_error` if the connection
fails. All addresses cross the interface as `codec::Address`; the controller
renders them to the schema's `text[]` couplet arrays and their dot-joined
`token_text` internally, always via the codec.

### Read side — "only-follow"

Never searches. Look a token up by its exact address (PK), then follow stored
lists only, across the two relationship axes (structure, membership). There is
no reverse-search index in the schema by design, and the reader never requires
one. There is also no aliasing/forwarding: an address IS the identity, so two
distinct addresses are never bridged to a common token.

| Method | Follows | Notes |
|---|---|---|
| `token_exists(id)` | `token` PK | Direct-address probe. |
| `parents_of(id)` | `token_parent` by `token_id` PK prefix, `ORDER BY ordinal` | Structure, downward. Ordered constituents, each with ordinal and per-element mass (`std::nullopt` when not yet computed). |
| `children_of(id)` | `token_child` by `token_id` PK prefix | Structure, reverse. Unordered in the schema, returned sorted for determinism. |
| `members_of(id)` | `members` by `token_id` PK prefix | Membership, downward: what a group directly contains. Unordered in the schema, returned sorted for determinism. |
| `member_of(id)` | `member_of` by `token_id` PK prefix | Membership, upward: the groups a token directly belongs to. Unordered in the schema, returned sorted for determinism. |
| `attributes_of(id)` | `token` PK | `notation`/`mass` pass-through; no `type` (dropped from the schema). |

All follows key on the **leading PK column** (`token_id`), so they ride the PK
index — no secondary/reverse index is touched.

### Write side — "see-mint-link-wire"

`mint(token_id, notation, constituents, mass)` runs as **one atomic
transaction** (`BEGIN` … `COMMIT`, `ROLLBACK` on any error) and returns `true`
if the token_id already existed (idempotent no-op), `false` if freshly minted:

1. **SEE** — probe `token` by PK. If the token_id already exists, nothing is
   written.
2. **MINT** — insert the `token` row. `token_text` is the codec's dot-joined
   rendering of `token_id`, derived by the controller (not a parameter, so it
   cannot drift from the identity); `notation` is the caller's temporary
   human surface form; `mass` is optional pass-through (`std::nullopt` → SQL
   NULL, never a stand-in 0). No `type`/kind is written — the column is gone
   (type is emergent/LoD-relative, never stored; see `NOTES.md`).
3. **LINK** — insert one ordered `token_parent` row per constituent, each with
   its ordinal (= position) and per-element `mass` (`Constituent::mass` is
   `std::optional<int>`; absent → SQL NULL).
4. **WIRE** — insert a `token_child` row for each **distinct** constituent,
   pointing that parent back at the new composite ("fold AND wire"). A
   constituent repeated at several ordinals yields a **single** reverse edge,
   because `token_child` carries no position — this is what keeps the
   deliberately-redundant `token_child` consistent with `token_parent`.

Constituents must already exist as `token` rows (FK). A missing constituent
raises `std::runtime_error` after the transaction rolls back, leaving no
partial token behind.

`add_membership(member, group)` replaces `add_group_membership`: it writes the
`member_of` row (member → group, upward) AND its `members` reciprocal (group →
member, downward), both directions, in **one atomic transaction**. No mass
operand, no kind operand — both endpoints must already be live `token` rows
(FK; mints nothing).

**Idempotent, SEE-style.** Each side is written with `ON CONFLICT DO NOTHING`,
so re-adding an already-present pair is a clean no-op — no throw, no duplicate
rows — the same discipline as `mint`'s no-op on an existing `token_id`. This
also means a one-sided/inconsistent membership row (only `member_of` or only
`members` present, e.g. from manual repair) is healed on its missing side
without disturbing the side already there.

### Mutation primitives (raw; op-layer gating is a later module)

These back the UPDATE record-tier ops (`MOVE_RECORD`, `DELETE_RECORD`,
`DELETE_CONNECTION`) but carry **no gating themselves** — no active-
confirmation, no peer-validation tags, no wildcard-safety check. That
op-layer behaviour is a separate, later module; these are the bare store
operations it sits behind.

- **`rekey(old_id, new_id)`** — re-keys a token and cascade-repoints every
  referencing row across `token_parent`, `token_child`, `members` and
  `member_of`, in one atomic transaction. `token_text` is re-derived via the
  codec from `new_id` (same derivation `mint` uses); `notation`/`mass` carry
  over unchanged. The schema has no `ON UPDATE CASCADE` and forbids
  triggers/functions, so the primitive owns the FK-safe ordering itself: it
  **inserts** the `new_id` row first, **repoints** every referencing row from
  `old_id` to `new_id` (both columns, across all four stores), then
  **deletes** the `old_id` row — it never updates a still-referenced PK in
  place, which would fail the FK check immediately. Throws
  `std::runtime_error` after rollback if `old_id` does not exist, or if
  `new_id` already exists (rekeying onto a live token_id would merge two
  identities — forbidden by "no aliasing/forwarding" / no MERGE).
- **`delete_token(id)`** — deletes the `token` row itself. If other rows
  still reference it, the database's default (no-cascade) FK behaviour
  applies as-is and the call throws; no cascade/repoint policy is invented
  here (`DELETE_RECORD`'s FK-dependent policy is an open seam, G10 in the
  plan).
- **`delete_pair(member, group)`** — removes one specific membership edge
  AND its stored reciprocal, in one atomic transaction: the `member_of` row
  (member, group) and the `members` row (group, member). This is the
  dual of `add_membership`; it addresses the membership axis only — the
  record tier never removes a structure edge (`token_parent`/`token_child`),
  only rekeys.

## Schema OPEN TEST SLOTS

Referenced by concept, not number (the schema's slot numbering is being revised
as `token_forwarding` and `address_classification` are removed).

- **`token_parent.ordinal` base — RESOLVED here: 0-based.** The first
  constituent is ordinal 0, matching the schema's array-index convention.
  `Constituent`'s ordinal is simply its index in the mint's constituent vector.
- The controller leaves the schema's other open slots as it found them: full
  (non-delta) address storage stands as the schema defines it.

There is **no aliasing/forwarding**: an address IS the identity — the same
construction yields the same address (one token, deduped by mint's SEE on the
PK), and two distinct addresses are never bridged to a common identity (this
is also why `rekey` refuses a `new_id` that already exists). The controller
therefore has no `resolve`/`fold` and touches no forwarding table.

## Build and run

Requires libpq and a local Postgres. Flags come from `pg_config`:

```sh
# from db_kernel/controller/
g++ -std=c++17 -O2 -Wall -Wextra \
    -I. -I../codec -I"$(pg_config --includedir)" \
    controller.cpp controller_test.cpp ../codec/codec.cpp \
    -L"$(pg_config --libdir)" -lpq \
    -o controller_test

./controller_test               # uses ../schema/schema.sql
./controller_test /path/to/schema.sql   # optional override
```

The adversarial harness builds and runs the same way (swap
`controller_test.cpp` for `controller_advtest.cpp`, output binary
`controller_advtest`).

To compile the controller into a larger engine instead of the test, drop
`controller_test.cpp` and link `controller.cpp` (plus the codec) with your own
translation units; the only external link dependency is `-lpq`.

## Test harness

`controller_test.cpp` runs directly against the real **`hcp3_core`** database,
which is disposable and may be overwritten any number of times — no throwaway
or uniquely-named database is used. It never fakes the store:

1. ensures `hcp3_core` exists (created once if absent, never dropped),
2. resets it — `DROP SCHEMA public CASCADE; CREATE SCHEMA public;` — and
   reapplies `../schema/schema.sql`, so each run starts from a clean schema
   regardless of prior contents,
3. exercises the mint path (see/mint/link/wire), the only-follow reads,
   idempotent re-mint, fold-and-wire consistency (`token_child` is the exact
   reverse of `token_parent`'s distinct parents), membership reads/writes
   (`add_membership`, `members_of`, `member_of`), the mutation primitives
   (`rekey`, `delete_token`, `delete_pair`), and the FK-violation rollback
   path.

`controller_advtest.cpp` is a second, adversarial harness over the same
disposable `hcp3_core`. It targets paths the base harness does not:
multi-constituent FK rollback (no orphan link/child rows), controller reuse
after a failed mint, `token_text` derivation (equals the codec dot-join of
`token_id`, never the notation), notation SQL-injection safety, the
distinct-reverse count for a repeated parent, idempotent re-mint leaving
notation/links untouched, and adversarial cases for `rekey` (onto an
existing `new_id`, from a nonexistent `old_id`) and `delete_pair`/
`delete_token` (reciprocal-only removal, FK-blocked delete).

It authenticates over the local unix socket (peer auth, no password). If no
local Postgres is reachable it prints a clear message and exits non-zero
rather than pretending to pass. Output is one `ok`/`FAIL` line per check, a
`PASS`/`FAIL controller_test` summary, and a non-zero exit on any failure.
