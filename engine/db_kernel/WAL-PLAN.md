# WAL subsystem — plan (rev. 5)

**Provenance.** Drawn from `NOTES.md` (the WAL-management topic; G7 delete; the
mass ruling; the governing only-follow / no-reverse-search principle) and the
design discussion of 2026-09-18. **Rev. 4** resolves the adversarial review
(PASS-WITH-FIXES) and Patrick's rulings that day: the WAL manager is a **pure
bookkeeper/observer**; recognition is by **return paths carried in the initial
data**, not inference; there is **no drain**; the swarm side is non-essential to
the primary work list; and mass is a **monitored followup**. Builds on the record
tier (`codec/`, `command/`, `declare/`, `read/`, `update/`, `dispatch/`, `seed/`)
over `hcp3_core`. **Rev. 5** applies a cross-doc consistency validation (findings
F1–F9): the retired WAL-management drift is shed from `NOTES.md`/`PLAN.md`/`API.md`
(bookkeeper, not a scheduler or validation-runner; no drain attributed to it), §5
splits the mass VALUE (rides the FIXED `token.mass`) from the mass calc-DEBT
bookkeeping (booked as part of a change's followup obligations — a delegated call,
made), and F5–F8 polish is applied. F4 (cache-manager runtime model) is **referred
to Patrick**, not built to.

**Standing.** Design only. Wide latitude for code structure; choices touching the
**fundamental data model** are Patrick's — where he has explicitly delegated one
(the mass table, §5) it is taken here and flagged for veto. **Patrick is
authoritative when in doubt**; NOTES is kept from drift by *updating it to match
his rulings*, not by letting stale NOTES text override them.

**Form.** A separate bookkeeper, so its **own small kernel set** — standalone-
buildable and self-testing like the record-tier modules ("builds, runs, tests on
its own"). Own binary vs. linked into larger binaries is left open, to follow the
structure.

**Anchoring honesty.** The bookkeeper-only charter (§0), the multi-instance model
(§1), the two-layer split (§2), and the recognition mechanism (§3) rest on the
**2026-09-18 discussion, authoritative from Patrick** — NOTES currently states the
"WAL management" *topic* more broadly (work scheduler + cross-network validation),
so **NOTES is to be updated** to record this bookkeeper scope (§7.1). "Self-
interpreting delta stream" is standing-memory phrasing, not NOTES text.

---

## 0. What this subsystem is — and the boundary

The **WAL manager** is a bookkeeper/observer over **WAL reports**. Reading a
change's own data, it knows the **reciprocal return paths** that change owes, and
it **monitors the stream for those returns to arrive** (§3). It works from the
**data — never the command string** — and never performs a write.

**In scope (only this):** the **WAL DB** (its own Postgres instance, §4);
ingesting WAL reports; deriving each change's owed return paths **from the data it
carries**; **monitoring** for the followup writes that settle them; tracking mass
followups (§5); per-source sequencing and replication state.

**Out of bounds (the cache manager's, neither designed nor described):** the
command string; all **primary execution** (filing, wiring, moving, deleting, and
the mass calculation/insertion itself); and **manager policy** (promotion/consent,
delete validation).

One line: **the cache manager writes the primary change and builds its return
paths; the WAL manager books what returns are owed and watches them land.**

---

## 1. The instances (context, not this subsystem's design)

*(2026-09-18 discussion — §7.1.)* A running instance connects to ≥3 DB instances:
**core** (`hcp3_core` — anchor, most-shared, slowest), **language DB(s)** (a
sharded, partitioned domain), and the **personality-and-relationship DB** (fastest,
least-shared; identical format/handling, **local addressing only**). Volatility and
fabric-visibility are inverted by design: the churny data is private, so it cannot
drive fabric-wide drift.

---

## 2. Two layers — and the swarm side is secondary

**Layer 1 — Postgres-native replication (configured, not built).** Each local
Postgres instance's **WAL + logical replication + internal compression** carries
shared data to subscribers. This is why Patrick chose Postgres; compressed delta
transmission is **Postgres-native, not built here.**

**Layer 2 — the WAL DB (this subsystem).** A bookkeeping store over WAL reports; it
moves no bytes.

**Swarm side is non-essential to the primary work list.** The primary work list is
the *local* return-path monitoring (§3). Cross-instance interaction is secondary and
deferred, and will **mostly consist of unspooling other instances' compressed WAL
patterns for cache-manager and analyst incorporation** — an inbound expansion role,
not a validation engine this subsystem runs (§7).

---

## 3. Recognition — return paths carried in the initial data

Each WAL report is self-interpreting, and **the followup that will satisfy a
change's obligation is part of the change's own initial data** — that is what the
WAL manager monitors for (Patrick, 2026-09-18):

- **When a token_id is created it is given its `parent_id`s as part of its
  construction.** Each parent then owes a **matching child write at a later time** —
  a `token_child` return (parent → this new token).
- **A `members` declaration** (group → member) owes a matching **`member_of`
  return** (member → group). **Same all around** for every reciprocal relationship.

This followup work is primarily the cache manager **constructing its own data-
traversal paths back through the data** — the reverse follow-lists that make the
graph walkable (only-follow) in both directions. The forward relationship is
declared with the change; the **return path** is built later; the WAL manager
monitors it lands.

So recognition needs **no decomposition rule and no inference**: the return to
expect is handed over in the change's own data. The obligation and the followup
that settles it **share one identity** — the return write's own key, **axis-
appropriate**: a `(parent, child)` pair for a structure return, a `(member, group)`
pair for a membership return — present in the initial data and again in the later
return write. Matching them is a **direct equality on that shared key**: only-
follow, no reverse index, and (for a DECLARE) no store read.

**Completion is self-accounting.** The cache manager **reads and does**; its return
write **is** a WAL report; that report is the proof it was done. An obligation opens
when a change is seen and closes when its return write of the same identity is seen.
There is **no drain** and **no done-mark** — settlement is intrinsic to the stream,
which is why no searched status flag is ever needed. (Deferred realization: today
the record tier writes reciprocals *synchronously*, so the "later" return is design-
ahead of the current cores — §7.)

**read → follow → do → repeat** is the cache manager's cycle (read a report, follow
stored lists, do the write, which is the next report). This plan **relies on** that
discipline; it does not design or guarantee it — the cache manager is out of bounds.

---

## 4. The WAL DB — substrate and shape

**Substrate.** Its **own Postgres instance**, uniform with the other locals.

**Ingest (explicit).** Obligations are derived from a report's decoded data — the
WAL feed is **Postgres logical-decoding output (Layer 1)**, footprint decoded via
`codec` token_ids. This is distinct from **G6**, which is the external
*request/command* transport — a different seam, not the WAL feed. This plan assumes
a decodable report and does not design the decoding wire form.

**Shape (only-follow, no reverse-search index).** Every access is a bounded PK
follow — no predicate scan on a non-key column, no reverse index:

- **Return-path obligations** — expected returns read from a change's initial data
  (`parent_id`s owe `token_child`; a `members` declaration owes `member_of`), keyed
  by the return write's own **axis-appropriate identity** (structure `(parent,
  child)`; membership `(member, group)`, §3); an obligation **closes when a followup
  write of that exact identity is observed** (§3). No drain, no queue consumption.
- **Mass obligations** — §5; part of a change's **followup obligations** (same bundle
  as its return paths), keyed by `token_id`; opened at `token.mass = NULL`, closed
  when a non-NULL `token.mass` write lands. Mass *value* stays in the FIXED core
  `token.mass`.
- **History** — append-only per source `lsn`; the durable record of reports seen and
  obligations settled.
- **Outstanding-obligation read-out** — the observer lists still-open obligations by
  a **bounded PK follow** (by `source`/`lsn`, or by identity to probe one specific
  obligation), never a scan.

Per-row: `source`, `lsn` (that source's native LSN — per-source progress/rebase),
the footprint (full `codec` token_ids; Layer 1 handles wire compression), and the
address scope (local/global).

**No stored `seq`.** Per-source `lsn` orders each source. A *cross-source* global
order is **not claimed** — native LSNs across sources are not mutually comparable —
and is a **swarm-side** concern whose basis Patrick settles (§7); the primary work
list needs only per-source order. **No `gather` reuse** (`gather` reads `token`
only); the follow *discipline* is reused, not the function.

---

## 5. Mass — a monitored calculation debt

A **DECLARE or a connection** incurs a **mass calculation debt**: a mass the cache
manager must compute and insert (Patrick, 2026-09-18). Mass is **not passed from the
analyst** — the cache manager would have to validate any asserted mass anyway, so it
just computes and inserts it (the NOTES mass ruling). The WAL manager does **not**
compute mass; it **books the debt and monitors that the mass write lands**. The
**mass-aggregation model** (how mass is computed) stays deferred and is the cache
manager's (§7).

**The mass VALUE rides the built `token.mass` — no new core schema.** A token's own
mass is a **FIXED, built** decision: stored on `token` (`NOTES.md` "own mass —
FIXED: stored on `token`"; `token.mass` nullable, **NULL = not-yet-computed**,
distinct from a real 0). That NULL state is the monitor signal: a declare/connection
report with `token.mass = NULL` **opens** the debt; a later non-NULL `token.mass`
write for that `token_id` **closes** it.

**Booking the debt — part of the change's followup obligations (my delegated call).**
The mass debt is one of the cache manager's **followup-work** items for a change,
alongside the return paths — so the WAL manager books it **with** that change's
followup obligations (the "original section"), not as a separate special table. Each
item closes on its own followup write's identity — a return write, or the
`token.mass` fill — matched by equality (only-follow). This is the WAL-DB *debt*
bookkeeping, **not** a second copy of the mass value: the value stays in the FIXED
`token.mass`, so there is no core-schema change. (An earlier note conflated the debt
bookkeeping with a core mass-value table; they are different things.)

---

## 6. The DELETE report (record-side only)

NOTES relocates G7's cross-network validation into "WAL management"; under the
bookkeeper scope that **validation act is swarm-side and not essential to the
primary work list** (§2), so it is **not** designed here. This subsystem's part is
only to ingest the delete's report and note the reciprocal **removals** it owes
(door-idempotent, per the record tier). The tentative→systemic validation is
deferred swarm-side work (§7), not cache-manager business.

---

## 7. Deferred (named, not designed)

1. **NOTES charter update** — record the bookkeeper scope of WAL management in NOTES
   (Patrick authoritative; controls drift).
2. **Swarm side** — unspooling other instances' compressed WAL patterns for cache-
   manager/analyst incorporation; the cross-network validation act (§6); the
   cross-source global-order basis (§4); drift control. Non-essential to the primary
   work list.
3. **Mass-aggregation model** (the computation, §5) and **feedback** — unresolved
   models; the WAL manager monitors their writes, it does not compute them.
4. **Async reciprocal** — the record tier writes returns synchronously today; the
   "later" return (§3) is design-ahead.

Relevant open seam: **G10** (`DELETE_RECORD` FK policy). *(G6 is the external
request/command transport — NOT the WAL feed, which is Postgres logical decoding,
§4. G4 next-slot and G5 block-boundaries are DECLARE-placement seams the bookkeeper
never touches. None are WAL touch points.)*

---

## 8. Reserved for Patrick

1. **NOTES charter update** (§7.1) — his authority; the plan proceeds on the
   bookkeeper scope meanwhile.
2. **Mass — decided (your delegation).** Value stays in FIXED `token.mass`; the calc
   debt is booked as part of the change's followup obligations (§5) — not a separate
   table, not a core-schema change. Flag if you'd rather it be a separate set.
3. **Cross-source global-order basis** (§4) — swarm-side; needed only if/when the
   swarm side is built.

---

## 9. Verification (tests ship with the build)

- **return-path recognition** — a created token's `parent_id`s yield exactly the
  owed `token_child` returns; a `members` declaration yields the owed `member_of`
  return — read from the data, never the command string.
- **reconciliation** — an obligation settles when a followup write of its exact
  axis-appropriate identity (`(parent, child)` structure, `(member, group)`
  membership) appears; matched by equality, no reverse index.
- **mass followup** — a report with `token.mass = NULL` opens a `token_id`-keyed
  mass obligation that settles when a non-NULL `token.mass` write for it is observed.
- **only-follow access** — every ledger access is a bounded PK follow; no non-key
  predicate scan, no reverse index.
- **scope recording** — a report's local/global scope is booked; private content is
  not promoted absent the cache manager's act.
- **per-source order** — each source's `lsn` orders it; no stored `seq`; no cross-
  source global order is assumed.

---

## 10. Open decisions

- The §8 reservations (NOTES charter update; cross-source basis).
- The deferred set in §7, owned by Patrick.
