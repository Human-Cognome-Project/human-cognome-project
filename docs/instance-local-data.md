# Instance-local data boundary

**Design record (2026-09-26).** No personal database, encryption facility, or
release path has been implemented here. This note records the intended boundary
so work on [database views](../kernels/database/WORKING-SET-AND-LEDGER.md),
[WAL routing](../kernels/wal/REPORT-TO-WORK.md), and the future
[swarm](../network/SWARM-NOTES.md) can use the same rule.

## Scope

Each NAPIER instance reserves a private address block for one or more local
databases. Provisional names are `personality.db` and `relationships.db`.
Their exact schema, split, and number are open. They hold the instance's own
history, notes on interactions and relationships, and differentiated aspects
such as preferred expressions. This information is local to the instance;
local records are not automatically shared with the cold shard swarm.

| Dimension | Meaning | Example |
|---|---|---|
| Local vs global | Address scope and allowed visibility. | An instance's relationship note vs a shared encoding-table definition. |
| Cold vs warm vs hot | Persistence, preparation, and current engine residence. | A local record can inform a private warm view without becoming a shared record. |

The instance's willingness to share does not automatically grant permission
to expose information about other people or authorize a global write. A
significant derived result might later be shared under separately specified
guardrails. What counts as a releasable result, how exceptional cases are
handled, and who approves a release are not settled. No ordinary WAL or cache
operation should be treated as that release path.

## Direction of followup work

- Global records and factors can change what an instance needs to consider;
  deferred work may enter that instance's local database.
- Local writes may create local reciprocal or mass work, observed by that
  instance's WAL bookkeeping.
- A local report or obligation does not by itself authorize deferred writes
  into shared global databases or into another instance's databases.
- Entries involving local history in the WAL manager's own PostgreSQL store
  are internal bookkeeping, not automatic content-tracker output.

This is a direction constraint on **where owed work can be written**, not a
claim that local databases are invisible to the local WAL manager. It needs
origin scope and destination/provenance to survive report conversion, durable
obligation bookkeeping, endpoint routing, and recovery. The built
[`wal::Report`](../kernels/wal/wal_report.h) carries `Scope`, but the built
[`wal::Obligation`](../kernels/wal/wal_recognize.h) is an identity without a target
scope; the current fixture-driven handler has a single cache-manager out-box.
The direction rule is not enforced by those components yet.

## Before implementation

Specify private address allocation, physical databases, encryption and key
lifecycle, ownership of instance-local reports and their WAL store, target
scope on pending obligations, and a guarded result-release path. Keep these
questions separate from the immediate local record/WAL/cache loop. The
[NAPIER guide](napier-system-guide.md) shows how this boundary fits the larger
system; the [discussion record](napier-system-discussion.md) retains the
original context.
