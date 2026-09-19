# Using the WAL manager

This is the **consumer contract** for anything that reads the WAL manager's
bookkeeping — chiefly the **cache manager**, which is the only real consumer
today. If you are working *on* the WAL manager itself, see `wal/README.md`
instead (charter, file map, build/run). This file is for everyone else.

## What it provides

The WAL manager maintains the **active deferred-work topology**: what
followup work a change has left owed, and whether it is still owed right
now. Two pieces, both in the WAL manager's own `wal_manager` Postgres
database (never `hcp3_core`):

- **The open-obligation relation** (`obligation`, `wal_schema.sql`) — a
  **live relation**. A row exists **iff** that obligation is still open;
  there is no status column, no done-mark, no queue to drain. A `structure`
  obligation `(parent, child)` means "the parent still owes a `token_child`
  return to that child." A `membership` obligation `(member, group)` means
  "the member still owes a `member_of` return to that group." A `mass`
  obligation `(token_id, <empty>)` means "that new token's mass is still
  NULL and hasn't been filled." When the row is gone, the obligation is
  settled — that's the whole signal.
- **History** (`history`, `wal_schema.sql`) — the append-only, durable
  record of every WAL report seen, one row per `(source, lsn)`, each naming
  the single obligation (if any) that report's write settled. This is the
  audit trail behind the live relation, not a second place to look for
  "what's open" — always read the live relation for that.

Where the types live: `Obligation` / `ObligationKind` in `wal_recognize.h`;
`Report` / `OpKind` / `Source` / `Lsn` / `Scope` (plus the `fixture::`
builders used in tests) in `wal_report.h`. Reuse these types directly —
don't define a parallel identity or report shape in consumer code.

## How to navigate it (only-follow)

Every access into the WAL DB is a **bounded PK follow** — there is no
reverse-search index and none should ever be added. `wal_book.h`
(`wal::WalBook`) is the door:

- `is_open(identity)` — a PK lookup: is this exact `(kind, addr_a, addr_b)`
  still owed? Use this to check one specific obligation you already know the
  identity of.
- `list_open(kind)` / `list_open(kind, addr_a)` — reads the live relation,
  unfiltered or filtered on a **PK-leading prefix**: every obligation of one
  `kind` (e.g. "every open mass debt"), or every obligation of one `kind`
  under one `addr_a` (e.g. "everything token X still owes as a parent, or
  as a member, or as itself pending mass" — call with the right `kind` for
  the axis you want). Calling `list_open` with `addr_a` but no `kind`
  throws `std::invalid_argument` — that combination isn't a PK-leading
  prefix, and this door does not expose a path around that.
- There is **no way to filter by `source`** or by any other non-key column,
  and no way to ask "what does token X still owe, across all three kinds,
  in one call" — that would be a reverse/predicate scan. If you need that
  view, issue one `list_open(kind, addr_a)` call per kind you care about;
  three bounded PK-prefix follows, not one scan.
- `open`, `close`, and `record_seen` exist on `WalBook` but are **not for
  consumers to call** — see Contract below.

## The contract / boundary

**The WAL manager is the sole writer of its own DB.** A consumer:

- **Never** calls `WalBook::open`, `WalBook::close`, or
  `WalBook::record_seen`, and never issues a raw INSERT/UPDATE/DELETE
  against `obligation` or `history` directly. Those three calls are the WAL
  manager's own bookkeeping (via `wal_ingest`/`wal_monitor`), driven only by
  observing WAL reports — a consumer opening or closing rows itself would
  make the bookkeeping lie about what's actually been done.
- **Never asks the WAL manager to do, schedule, or prioritize work.** It
  has no such surface, on purpose (WAL-PLAN.md §0: bookkeeper/observer
  only, never a scheduler). If you want something done sooner, do it
  yourself — see RECONCILE below.
- Does **its own primary work** against its own store (for the cache
  manager: reading a report, following its own stored lists, and writing
  the reciprocal or the mass fill to `hcp3_core`). That write becomes the
  next WAL report; the WAL manager observes it and closes the matching
  obligation by identity equality. This is the **read → follow → do →
  repeat** cycle (`WAL-PLAN.md` §3): the consumer's own write *is* the
  proof of completion, and the WAL manager's only job is to notice it
  landed.
- Reads the open-obligation relation to know **what's still owed**, not to
  be told **what to do next in what order** — the WAL manager keeps no
  priority, no queue position, nothing beyond membership. Any ordering
  policy (which open obligation to work next) is the cache manager's own
  decision, made by reading `list_open`, not something the WAL manager
  hands over.

One line, repeated from `wal/README.md` because it's the whole boundary:
**the cache manager writes the primary change and builds its own return
paths; the WAL manager books what returns are owed and watches them land.**

## RECONCILE

RECONCILE (`NOTES.md` "Cache tier") is an analyst-raised **"this deferred
cross-work is priority now"** flag — the analyst expects the pending
cross-processing results (reciprocal returns, derived masses/centroids)
will help their current analysis, so they want it pulled forward instead of
draining in the background. It is a momentary priority boost on **already-
known** deferred work, not new work, and it is acted on entirely by the
**cache manager**: on a RECONCILE, the cache manager navigates the WAL
manager's open-obligation topology (`list_open`) to find what's still owed
and does that work now instead of later.

**The WAL manager's role is unchanged by RECONCILE.** It does not receive
the flag, does not reprioritize anything, and does not drain faster or
slower — it keeps doing exactly what it always does: booking obligations as
they open and closing them as it observes the settling writes land,
whenever they land. RECONCILE changes *when* the cache manager acts, never
what the bookkeeper does.

## Build / link

Link against `wal_book.{h,cpp}` (the door), `wal_recognize.h` (the
`Obligation`/`ObligationKind` types — header-only for the type itself),
and `wal_report.h` (the `Report`/`OpKind` types, header-only) as needed;
`../codec` is a transitive dependency (`codec::Address`). See
`wal/README.md` for the exact `g++` invocations, the **run-FROM-`wal/`**
convention (DB-backed code loads `wal_schema.sql` relative to the current
working directory), and the disposable `wal_manager` database note — that
convention and those build lines are canonical there; this file doesn't
duplicate them.

## Guardrails (recap)

- Don't reach into the `wal_manager` database directly (no raw SQL against
  `obligation`/`history` from consumer code) — go through `WalBook`'s
  read-only surface (`is_open`, `list_open`).
- Don't call `open` / `close` / `record_seen` from consumer code — those
  are the WAL manager's own writes.
- Don't ask the WAL manager to do, schedule, or prioritize work — it has no
  such surface. Do the work yourself and let your own write be observed.
- Don't treat `list_open` as an ordering or priority hint — it is
  membership only. Ordering policy (including RECONCILE) is entirely the
  cache manager's own.
