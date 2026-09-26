# From database reports to deferred work

**Status (2026-09-26):** Component-level guide to the intended report ingress
and work loop in the [NAPIER system](../../docs/napier-system-guide.md).
The [WAL README](README.md), [USAGE.md](USAGE.md), and current C++/SQL specify
the built fixture-fed bookkeeper. A live PostgreSQL logical-decoding adapter,
the cache-manager pending-work consumer, and analyst RECONCILE staging are
not implemented.

## Responsibilities and sequence

The WAL manager is an independent bookkeeper. Database changes supply its
ongoing input; **RECONCILE is the only command the future analyst sends to
it**. It never writes the primary cold-store change and does not read the
command that caused it. The manager's own `wal_manager` PostgreSQL database
holds the durable obligations and report history. The cache manager, rather
than the WAL manager, carries out outstanding reciprocal/mass writes.

| Step | Input/output contract | State |
|---|---|---|
| Database change | A primary write becomes part of its database's WAL. Each source has its own LSN ordering; unrelated sources have no global order. | PostgreSQL records exist; live capture is not wired to this kernel. |
| Ingress bridge | Decode a source report to [`wal::Report`](wal_report.h): source, native position, local/global scope, operation, addressed footprint, relevant mass. Route it to that source's inbox, preserving per-source order. Recognition uses decoded change data, not the initiating verb. | Report struct and fixtures built; live decode/wire form, durable delivery and replay to define. |
| Bookkeeping | Recognize the returns owed, record History, open keyed obligations in the manager's own database; close them when matching return reports appear. | Built for fixture reports, with per-source high-water checking. |
| Work notification | Emit owed identities to the appropriate cache-manager pending-work inbox. An occupied box activates its handler; the obligation remains durable until a followup report settles it. | Built as a fixture-driven WAL endpoint handler with **one** out-box; cache-manager tier-3 consumer and reload re-emission still to build. |
| RECONCILE | Analyst asks WAL to select relevant *existing* obligations and place them in the cache manager's normally empty top-priority box. Work runs next; no running unit is interrupted. | Design only; scope/relevance and delivery to implement. |
| Content tracker | Eligible WAL-manager entries may eventually provide the source records/manifest for swarm content tracking. | Deferred, lowest priority; privacy filtering and exact artifact/manifest shape unchosen. |

The **queue** means the manager's persistent open-obligation relation plus
transient endpoint delivery. It is not a consumed FIFO or a second durable
pending list in the cache manager. `obligation` is keyed by owed structure,
membership, or mass identity; `history` holds the decoded footprint and
bookkeeping fields rather than a full original WAL record. `wal::WalKernel`
feeds a fixture `Report` through the per-source monitor and pushes each owed
`Obligation` as an arena handle to its out-box. The built code has no real
source-to-inbox decoder, no consumer write and no reconcile handler.

An initial primary `token_parent` occurrence owes a `token_child` reverse
walk for its constituent/composite pair. A `members` forward group declaration
owes `member_of` on its member; new-token mass fill can also be owed. These
stored returns are the cold graph's future n-dimensional traversal paths.
The cache manager's **built** `Controller::mint` and
`Controller::add_membership` currently write reciprocals in the same call.
The proposed file-now/wire-later route must reconcile this eager behaviour
before delayed work can be the source of those writes. See
[working set and ledger](../database/WORKING-SET-AND-LEDGER.md).

## Ingress wireframe still to specify

Patrick proposed entry into the WAL manager's own PostgreSQL database as a
durable handoff for decoded reports and a later content-tracker source. The
current code accepts in-memory reports from fixtures and then writes History
and obligation rows; direct database entry before endpoint activation is a
possible future integration, **not its current input contract**. A focused
adapter design should pin down the logical-decoding source and slot/LSN
representation, report-to-`OpKind` mapping, source identity, which record is
durably accepted before acknowledging a feed position, delivery/replay across
a restart, and how an entry activates the source inbox. The manager must
recognize changes from decoded data without inferring the author's command.
There is no assumption of a total ordering across different databases.

The content tracker cannot treat every History entry as shareable. The swarm
notes propose WAL-derived indexes/piece references, but the exact source
files and manifest are future network design. Work on local ingress and
returns is the immediate priority; [swarm notes](../../network/SWARM-NOTES.md)
retain the later direction.

## Direction at the private boundary

An instance's local databases may be WAL sources for **that instance's**
bookkeeping. Global changes may create local deferred followups; local
changes do not ordinarily cause a deferred write to a shared global database
or another instance. Local reports, obligations and History records are not
implicitly publishable. Any separately shared derived result needs the
[instance-local release design](../../docs/instance-local-data.md).

`Report::scope` and the stored History record local/global provenance. The
built `Obligation` identity and single outbound box do not yet carry/check
the work's target scope. Live conversion, durable work representation,
consumer selection and replay must retain enough origin/target information
to enforce this direction. Keeping local report processing inside the
instance is compatible with the privacy rule; silently feeding its entries
to a tracker is not.

See [kernel activation](../../network/KERNEL-ACTIVATION.md) for priority,
mailbox and runtime responsibilities. `wal::WalMonitor` here refers to
per-source LSN bookkeeping, not the network scheduler monitor or the human
browser view of the field simulation.
