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
| Cold vs warm vs hot | Source persistence, shared toolbox preparation and active assembly. | Analyst-local data joins shared tools only at the hot workbench, outside the cache manager's warm path. |

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

## Active-only private input clarification — 2026-10-16

Patrick's clarification in #145/#139 supersedes private warm-view wording:
private/local information belongs to the analyst and enters consideration only
at the active hot workbench. The cache manager has no role in handling it.
Exclude private DB codes/handles from the manager's authorized request/source
path and reject attempted private scopes before lookup, without inspecting,
classifying, filtering, redacting or logging private contents/code values.
This protects both model/entity private stores; public entity knowledge can
still belong in the shared cache.

The analyst combines supplied local inputs with shared cached tools on its
workbench. No manager warm publication/acquisition/refresh or manager reporting
path carries private data or joint results. The original private source DB can
remain persistent. Existing generic scoped-storage capability and `y*` / `z*`
reservations confer no private-data access authority on the manager.

The [pre-D design](pre-d-hot-workbench-design.md) specifies scoped provenance,
active buffers, separate reporting/restart ownership and supplied-input tests.
The bounded [D adapter](../engine/docs/ACTIVE-WORKBENCH.md), released in #148,
implements that access gate and supplied local fixtures, accepted in #149; no analyst
reasoning or real private DB integration is included. Earlier WAL notes above describe
unbuilt routing seams, not permission to forward private work to a cache-manager
out-box.

