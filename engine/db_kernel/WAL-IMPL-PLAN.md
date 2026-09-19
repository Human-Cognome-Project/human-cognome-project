# WAL manager — implementation plan (for coding agents)

> **STATUS: IMPLEMENTED** as `wal/` (2026-09-19) — W-1…W-6 all built and green
> (see acceptance criteria per task below, all met), package-vetted
> primary↔adversary, committed through `a899970`. See `wal/README.md` (charter,
> file map, build/run) and `wal/USAGE.md` (consumer contract for the cache
> manager and anything else that reads this kernel set's obligation topology).
> Bucket B/C below are unchanged by the build — still design-ahead / NEEDS-
> PATRICK, not resolved. One item package review raised (W-5 ingest's
> `close()`+`record_seen()` are not one transaction) is **resolved, dropped as
> a WAL-manager item** (Patrick, 2026-09-19) — no transaction surface is added
> here; the real underlying gap becomes a cache-manager-runtime item instead.
> See `WAL-PLAN.md`'s status blockquote for the full trace.

**Source.** `WAL-PLAN.md` (rev. 6, reconciled/spec-compliant) and the 7-point
ruling set (Patrick, 2026-09-18). This plan turns that design into
**Sonnet-sized build tasks**. It plans **only the WAL manager (bookkeeper)** and
**only the parts buildable against the code that exists today**; everything that
needs the async reciprocal, the cache manager, or a Patrick decision is named and
**not planned to build**.

---

## 0. Charter carried down from the design (do not violate)

- **Bookkeeper/observer only.** The WAL manager reads WAL reports, derives what
  returns each change owes, and monitors for those returns. It **never writes the
  primary change** (never touches `hcp3_core`), **never reads the command string**,
  and **never designs or drives the cache manager**. *(Spec 1.)*
  - Nuance to encode honestly: it **does** write to **its own WAL DB** (the
    obligation relation + History) — that is its bookkeeping, not a primary-store
    write. "Never performs a write" in `WAL-PLAN.md` §0 means the **core store**.
- **No drain / no consumed queue.** Completion is **self-accounting**: an
  obligation is a **row inserted on open** and **PK-DELETED on close**; "still
  open" is **membership** of the relation — **no status column, no scan, no
  done-mark**. *(Spec 2.)*
- **Recognition = return paths in the change's own initial data.** `parent_id`s
  owe `token_child`; a `members` declaration owes `member_of`; same all around.
  Match followup→obligation by **equality on the shared axis-appropriate
  identity** — `(parent, child)` for structure, `(member, group)` for membership.
  **only-follow, no reverse index, no store read** (for a DECLARE). *(Spec 3.)*
- **Mass = cache-manager followup work; WAL manager only monitors.** Booked **with**
  the change's followup obligations (not a separate table). Value lives in the
  FIXED `token.mass` (NULL = not-yet-computed). The NULL→non-NULL signal is used
  **only for a new-token DECLARE**. *(Spec 4.)*
- **Per-source ordering only.** Each source's native `lsn` orders that source; **no
  cross-source global order** is assumed or built. *(Spec 6.)*
- **Own standalone kernel set.** Same discipline as the record-tier modules:
  each part **builds, runs, and tests on its own** (C++17, libpq, tiny in-file
  check harness, prints `PASS <name>`, exits 0), and **every task ships with
  tests** (project rule). Reuse `codec` for address↔token_id; do **not** rebuild
  it, and do **not** reuse `gather` (it reads `token` only) — reuse the follow
  *discipline*, not the function.

---

## 1. Honest staging — what actually exists to build against

> The record tier writes reciprocals **synchronously** today. The async
> "file-now / wire-later" return the monitor is designed to watch for **does not
> exist yet** — that is the cache manager's runtime, which is out of scope. The
> bookkeeping mechanism is nonetheless buildable and testable now: the open→close
> machinery works whether the return arrives in the same batch or a later one, so
> we build it and **drive it from fixture WAL reports** that cover both gaps.

Three buckets. Only bucket A is planned into build tasks.

### A. BUILDABLE-NOW (planned below, tasks W-1…W-6)
- The **WAL DB schema**: the live-relation obligation store (open = membership,
  PK identity, no status column) + the append-only **History**.
- **Recognition** as a pure function: decoded report → owed obligations
  (axis-appropriate identity; mass obligation only for a new-token DECLARE).
- The **bookkeeping door**: open (insert), close (PK-delete by identity),
  list-open (read relation), History append — all bounded PK follows.
- **Ingest from a decoded fixture report** into the door.
- The **monitor loop** over an ordered per-source fixture stream: recognize →
  open; match followup writes → close; advance per-source `lsn`.
- Tests at every layer against **fixture WAL reports** (no live replication slot).

### B. DESIGN-AHEAD / BLOCKED (named, not built)
- **Async reciprocal split.** The genuine "later" return is produced by the cache
  manager's file-now/wire-later runtime, which does not exist. We do **not** build
  cache-manager behaviour; we only make the monitor tolerant of the gap via
  fixtures.
- **Live Postgres logical-decoding ingest.** `WAL-PLAN.md` §4 assumes a decodable
  report and **does not design the decoding wire form**. Consuming a real
  replication slot is blocked on that wire-form decision. Buildable-now ingest is
  from an in-process **decoded report struct**, not a live slot.
- **Mass-fill write.** There is **no `token.mass` UPDATE primitive** today
  (`mint()` writes mass once at NULL; `rekey()` carries it over), and the non-NULL
  fill is the cache manager's deferred aggregation output, so the **mass-close
  write is design-ahead** — exercised only from the W-2 mass-fill fixture, never by
  observing the record tier emit it.
- **MOVE_RECORD / rekey of open obligations.** A MOVE changes `token_id`s that
  open obligations key on. MOVE is atomic today (nothing owed); re-keying live
  obligations is design-ahead (`WAL-PLAN.md` §7.5). Named, not designed — and note
  it is the one operation that could demand a reverse index, so it stays out.

### C. NEEDS-PATRICK (do not plan or build)
- **Connection mass-recompute signal** — how a dirtied (existing-token) mass is
  marked for recompute; part of the deferred aggregation model (`WAL-PLAN.md` §5).
- **F4** — whether self-accounting *replaces* the cache manager's own pending-list
  runtime or only governs the WAL manager's view (`WAL-PLAN.md` §10; flagged in
  `NOTES.md` to revisit when the cache-manager runtime is built).
- **Cross-source global-order basis** (`WAL-PLAN.md` §4/§8.3) — swarm-side; only
  if/when the swarm side is built.
- **Optional canonical NOTES WAL-management section** (`WAL-PLAN.md` §7.1/§8.1) —
  scope already recorded across NOTES/PLAN/API; consolidation is at Patrick's
  discretion, not a build item.
- **Swarm side entirely** (`WAL-PLAN.md` §2/§6/§7.2) — unspooling peers' compressed
  WAL patterns, the cross-network validation act, drift control. Deferred.

---

## 2. Kernel set layout (mirrors the record-tier module convention)

A new sibling directory `wal/` holding the set — each part standalone-buildable
and self-testing, same as `codec/`, `declare/`, `read/`:

```
wal/
  wal_schema.sql        -- the WAL DB schema (its OWN Postgres instance)
  wal_verify.sql        -- structural assertions on the schema (cf. schema/verify.sql)
  wal_schema_tests.md   -- what wal_verify.sql proves (cf. schema/tests.md)
  wal_report.h          -- the decoded-report struct + fixture builders (the ingest seam)
  wal_recognize.{h,cpp} -- pure: decoded report -> owed obligations
  wal_recognize_test.cpp
  wal_book.{h,cpp}      -- the bookkeeping door over the WAL DB (open/close/list/History)
  wal_book_test.cpp
  wal_ingest.{h,cpp}    -- one-report step: recognize -> open, and match return -> close
  wal_ingest_test.cpp
  wal_monitor.{h,cpp}   -- the ordered per-source loop driving wal_ingest over a stream
  wal_monitor_test.cpp
  README.md             -- scope, the bookkeeper charter, build/run lines, decisions
```

Build/run convention per part (as `codec/README.md` documents):
`g++ -std=c++17 -O2 -Wall -Wextra <part>.cpp <part>_test.cpp -lpq -o /tmp/<part>_test && /tmp/<part>_test`
→ prints `PASS <part>_test`, exits 0. DB-backed tests connect to a **disposable
`wal_manager` database** (create-if-absent, same pattern as
`read/read_core_test.cpp`); the WAL DB is **separate from `hcp3_core`**.

---

## 3. Build tasks (Sonnet-sized; each ships tests)

### W-1 — WAL DB schema (`wal_schema.sql`, `wal_verify.sql`, `wal_schema_tests.md`)
Define the WAL manager's own Postgres schema. **No reverse-search indexes; only PK
indexes.**
- **`obligation`** — the live open-obligation relation. **open = membership**, so a
  row exists **iff** the obligation is open; closing is a **PK DELETE**. Columns:
  the **axis-appropriate identity** as the PK, plus enough to identify the axis and
  source:
  - a discriminator for the obligation kind/axis (`structure` `(parent, child)`,
    `membership` `(member, group)`, `mass` `(token_id)`),
  - the identity address columns as a **FIXED two-slot PK** `(kind, addr_a,
    addr_b)`, `text[]`, **COLLATE "C"** (matching `schema/schema.sql`'s
    address-column pin so PK order = address order). Structure = `(parent, child)`,
    membership = `(member, group)`, mass = `(token_id, ·)`. Because **Postgres PK
    columns are NOT NULL**, the absent second axis of a mass obligation uses the
    **sentinel `addr_b = '{}'`** (an empty, non-null `text[]`), **not** NULL — this
    is what keeps mass in the *same* relation without a separate mass table (Spec 4
    / `WAL-PLAN.md` §5). Never reach for a separate mass table. The two-slot PK and
    `'{}'` sentinel are **WAL-DB own-schema latitude** (`WAL-PLAN.md` §Standing,
    "wide latitude for code structure"), **not** a core-model change — the mass
    *value* still rides the FIXED `token.mass`, untouched.
  - `source` (core / a language shard / personality DB) and the opening `lsn`.
  - **No `status` column, no `done` flag, no `seq`.** (Encodes Spec 2.)
  - Mass obligations live **in this same relation** under the `mass` kind — booked
    **with** the change's other followup obligations, **not** a separate table
    (Spec 4 / `WAL-PLAN.md` §5).
- **`history`** — append-only; the durable audit of reports seen and the
  obligations each settled (`WAL-PLAN.md` §4). This is where per-source ordering
  lives, **not** a second index on `obligation`. Append identity is
  **`(source, lsn)`, one row per report** — see W-4: `record_seen` is the sole
  history append, and it *records* any obligation the report settled in that same
  row; `close()` only PK-deletes, it does **not** append a second row (which would
  collide on `(source, lsn)`). Per-source `lsn` orders each source; **no
  cross-source global `seq`** is introduced.
- **Per-row on `history`:** `source`, `lsn`, footprint (full `codec` token_ids),
  **address scope** (local/global, read from the report's address range), and the
  **settled-identity** reference (null/empty when the report opened work but
  settled nothing).
- **Settlement is at most one per report** — the per-write report model of
  `WAL-PLAN.md` §3 ("its return write *is* a WAL report"), which is what lets the
  single settled-identity column and the close()-does-not-append rule hold. Multi-
  *open* per report is fine (a 3-parent DECLARE opens 3 obligation rows under one
  history row); the one-per-report constraint is only on *settlement*. A future
  (bucket-B) wire form that **bundles multiple settling writes into one report**
  would revisit the single settled-identity column — named here, not designed.
- `wal_verify.sql` asserts: PK present on `obligation`; the mass `addr_b` sentinel
  is non-null; **no non-PK/ reverse index**; address columns are `text[] COLLATE
  "C"`; `history` PK is `(source, lsn)` and append-only shaped. `wal_schema_tests.md`
  states what each assertion proves.
- **Acceptance:** `psql -f wal_schema.sql` on a fresh DB succeeds; `wal_verify.sql`
  passes; grep shows zero `CREATE INDEX` other than PKs.

### W-2 — decoded-report seam + fixtures (`wal_report.h`)
Define the **in-process decoded WAL report** struct the rest of the set consumes —
the explicit stand-in for logical-decoding output (bucket B: the live wire form is
not designed). One report carries: `source`, `lsn`, `scope`, the op kind
(DECLARE / ADD_CONNECTION / membership write / DELETE / the reciprocal return
writes / the **mass-fill**), and the **decoded footprint as `codec` token_ids**
(reuse `codec`, do not re-decode). Provide **fixture builders** for each op so
downstream tests construct streams without a live DB.
- The **mass-fill** op is a distinct fixture, named apart from the reciprocal
  return writes: it is an **UPDATE of an existing token's `token.mass` from NULL to
  non-NULL**, not a reciprocal insert. It has no record-tier emitter today (there
  is **no `token.mass` UPDATE primitive** — `mint()` writes mass once, `rekey()`
  carries it over; see §1 bucket B), so this fixture is the **only** way to
  exercise the mass-close path — which is exactly what bucket A's fixture charter is
  for.
- **Acceptance:** header compiles standalone; a tiny `wal_report_test.cpp`
  constructs one of each fixture op and round-trips its footprint through `codec`.
- **Tests:** fixture builders produce the exact token_ids expected; a DECLARE
  fixture exposes `token.mass = NULL`; a mass-fill fixture carries a non-NULL
  `token.mass` for an existing `token_id`.

### W-3 — recognition (pure) (`wal_recognize.{h,cpp}`)
Pure function `owed(report) -> vector<Obligation>` — **no DB, no store read, no
command string**. Implements Spec 3:
- a created token's `parent_id`s ⇒ one `structure` obligation per parent, identity
  `(parent, child)`;
- a `members` declaration ⇒ one `membership` obligation per member, identity
  `(member, group)`;
- a **new-token DECLARE** with `token.mass = NULL` ⇒ one `mass` obligation, identity
  `(token_id)` — **only** for a new-token DECLARE (an ADD_CONNECTION targets an
  existing token and emits **no** mass obligation here — recompute is NEEDS-PATRICK).
- a **DELETE report** ⇒ **no obligation** — it is **History-only** (`record_seen`).
  `WAL-PLAN.md` §6's "note the reciprocal removals it owes" is satisfied by
  History-recording the delete. Opening a live *removal* obligation is **not** done
  here, for two reasons: (i) removal reciprocals are **synchronous today**
  (`delete_pair()` removes members+member_of atomically; `delete_token()` is a
  single-row op), so a removal obligation would open and close in the same batch and
  buys nothing now; (ii) a removal obligation would need a **direction/polarity**
  the axis discriminator does not carry (a `member_of` INSERT and a `member_of`
  DELETE compute the same identity), which is design-ahead of any async removal
  (`WAL-PLAN.md` §7.6) — not built. The cross-network validation act is swarm-side
  and out of scope (§6, bucket C).
- **Acceptance:** deterministic obligation set for each fixture op; empty for ops
  that owe nothing; a connection yields **no** mass obligation; a **DELETE opens no
  obligation** (History-only) and yields **no** validation-act obligation.
- **Tests (`wal_recognize_test.cpp`):** each recognition rule (structure,
  membership, mass); a DELETE yields an empty obligation set; the mass rule fires
  only for new-token DECLARE; nothing reads a command string.

### W-4 — bookkeeping door (`wal_book.{h,cpp}`)
The libpq door over the WAL DB. Every access a **bounded PK follow**:
- `open(Obligation)` — INSERT the live row (idempotent on the identity PK).
- `close(identity)` — **PK DELETE** the row. It does **not** append its own history
  row; the settling report's single `record_seen` append records the settlement (F-3
  / W-1). Closing an **absent** identity is a **no-op** (not an error) — the
  report is still History-recorded by `record_seen`.
- `list_open(...)` / `is_open(identity)` — read the relation (membership), **no
  scan**. `is_open` is a PK lookup. `list_open` is either **unfiltered** (read the
  whole relation, permitted per `WAL-PLAN.md` §4) or filtered **only on a
  PK-leading-column prefix** (i.e. by `kind`, or a `kind`+`addr_a` prefix) so it
  stays a bounded PK-range follow. It **never** filters on `source` or any other
  non-key column — that would be the forbidden predicate scan.
- `record_seen(report)` — the **sole** history append: one row per report (source,
  lsn, footprint, scope, settled-identity-if-any).
- Writes go **only** to the WAL DB — never `hcp3_core`.
- **Acceptance:** open then close leaves zero rows for that identity; `is_open` is
  true only between them; absent-identity close is a no-op that still History-records
  the report; every query plan is a PK lookup or PK-prefix range (no seq scan on a
  non-key column — assert with `EXPLAIN` in an adv test).
- **Tests (`wal_book_test.cpp`, DB-backed, disposable `wal_manager`):** open/close
  round-trip; double-open idempotent; close of an absent identity is a no-op **and**
  the report is still in History; History grows append-only with `(source, lsn)`
  unique. Adv test: `EXPLAIN` shows PK-only / PK-prefix access, and a
  `source`-filtered `list_open` is rejected as a scan.

### W-5 — ingest (`wal_ingest.{h,cpp}`)
A **discrete, standalone-buildable** module (its own `wal_ingest_test.cpp`, §2)
that links `wal_recognize` + `wal_book`. The single-report step: `record_seen` →
`owed(report)` → `open` each obligation; **and** if the report **is** a followup
that settles work — a `token_child`/`member_of` return write or a **mass-fill** —
`close` the matching open obligation by **equality on the shared identity**. Match
is only-follow: build the identity from the settling write and PK it — **no reverse
index**. (A DELETE opens nothing and settles nothing here — it is History-only,
W-3.)
- **Acceptance:** a forward change opens its obligations; the matching settling
  write closes exactly those and no others; a settling write with no open obligation
  is recorded in History but closes nothing (no-op close, per W-4).
- **Tests (`wal_ingest_test.cpp`):** forward-then-return in the **same batch**
  closes; forward-then-return in a **later batch** also closes (proves the
  gap-tolerance of the bucket-A mechanism without building bucket-B async); a
  mass-fill closes a mass obligation; an unmatched settling write is inert.

### W-6 — monitor loop (`wal_monitor.{h,cpp}`)
Drive an **ordered per-source** fixture stream: for each source, consume reports in
`lsn` order, run **`wal_ingest`** (W-5) per report, and track **per-source `lsn`
progress** (for rebase). **No cross-source ordering** — sources advance
independently.
- **Acceptance:** given a fixture stream, ends with exactly the still-open
  obligations expected (open minus closed); per-source `lsn` high-water is correct;
  interleaving two sources does not couple their progress or invent a global order.
- **Tests (`wal_monitor_test.cpp`):** a scripted multi-source stream reconciles to
  the expected open set; per-source progress independent; replay from History is
  consistent; a DELETE report is **History-booked only** — no removal obligation is
  opened — with the cross-network validation act **absent** (swarm-side, not built —
  `WAL-PLAN.md` §6).

> Task order / parallelism: **W-1, W-2, W-3 are independent** and can go in
> parallel. **W-4** depends on W-1. **W-5** depends on W-2/W-3/W-4. **W-6** depends
> on W-5. Each merges only when its own `PASS` build is green.

---

## 4. Test strategy (shared)

- **Fixtures, not a live slot.** All streams are built through `wal_report.h`
  builders. This is deliberate (bucket B): the logical-decoding wire form is not
  designed, and the async reciprocal does not exist. Fixtures cover both the
  same-batch return (today's synchronous reality) and the later-batch return
  (the mechanism's forward compatibility) **without** modelling cache-manager
  internals.
- **DB-backed tests** use a **disposable `wal_manager` database**, created if
  absent exactly as `read/read_core_test.cpp` does for `hcp3_core`, and reset per
  run. Never point them at `hcp3_core`.
- **only-follow assertion** is a first-class test (W-4 adv): `EXPLAIN` must show
  PK access for every obligation query; a reverse/seq scan is a failure.

---

## 5. Explicitly NOT in this plan

Cache-manager runtime (pending-list, file-now/wire-later, RECONCILE); mass
computation/aggregation; any write to `hcp3_core`; the DELETE cross-network
validation act; live replication-slot consumption / wire-form decode; MOVE/rekey
of live obligations; cross-source global ordering; the connection mass-recompute
signal; the optional canonical NOTES section. (Buckets B and C above.)
