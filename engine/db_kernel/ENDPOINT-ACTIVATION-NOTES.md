# Endpoint-activation cross-kernel model — design notes

**Status: PARTLY BUILT (2026-09-22). Governing model for the cross-kernel command /
coupling layer.** The local activation substrate (`endpoint/`, commit `b97034a`) and the
WAL manager as a monitored-endpoint kernel (Pair 1 — `wal/wal_kernel.{h,cpp}`, commits
`3aca2ac`/`cf7e0c6`) are **BUILT**; the remaining coupling (cache-manager kernel, Pair-2
swarm, the API-pair transport bridge, reload repopulation, the live feed) is still
**direction record — not built.** Its **resolved decisions govern** the ongoing
realignment (they are no longer merely proposed). Scope confirmed with Patrick:
cross-kernel commands — not the analyst layer, cross-kernel field work, or the wider HCP
vocabulary. It reshapes *how components are invoked and coupled*; largely
**non-destructive to the built record-tier cores** — the six verbs survive as reaction
bodies (see *Fit with the built base*).

Canadian English. Prose/draft record. On any conflict with built behaviour, the
code and the existing `API.md` / `NOTES.md` remain the source of truth for what is
*built*; this file records a *direction* still under discussion.

## The shift

The current construct invokes a kernel by **active instruction** (a serialized
command dispatched at it) and reads results by **polling**. Both pull the system
toward API-style serialization as its native idiom — request/response is the shape
of "instruct, then check." That is not optimal for actual data flow.

Move the base to **monitored data-endpoint activation**: a component monitors an
endpoint; **data arriving at the endpoint is the activation.** No instruction, no
poll — presence fires the work. Serialization is not discarded; it is **demoted to
a bridge**, used only where two components do not share a native memory
representation.

## Endpoints are memory boxes, not writes

An endpoint is a **memory allocation box the process monitors** — a virtualized
stdin/stdout location — NOT a physical store write. A box **may refer to a
secondary location if need be**, but the physical write is the exception, not the
substrate. The local path is box-to-box in native memory: no serialization and no
physical write anywhere on it.

- A box is an **ordered queue** (a region holding pending items in order), not a
  single slot.
- **The box queues its work by existing.** There is no enqueue call and no
  separate queue object — an occupied box *is* the queued work; the monitor
  watching it sees occupancy as the activation, handles the contents in order, and
  clears them. Empty = settled. This is the same presence=pending / cleared=done
  accounting the WAL manager runs, seated in memory rather than in DB rows.

## One process shape

Every component is the same shape: **monitor an occupied box → run its body →
emit into an output box.** Nothing is privileged. A record-tier component's body
is DECLARE / READ / UPDATE; a converter's body is serialize-and-hand-off; its
counterpart's body is deserialize-and-place. The scheduler over a set of boxes
does exactly one thing (see *Priority*).

## Serialization as a bridge (the .json/API converter)

For any two of *our own* internal components separated by a representation /
system boundary:

- A converter is **just another monitor-a-box process** — indistinguishable from
  any kernel to everything around it. Its body: serialize whatever it finds in its
  input box and hand it to its counterpart.
- It is a **matched pair**: the counterpart on the other side deserializes and
  places the result into the memory boxes (queues) on that side. From every other
  component's view, the counterpart it talks to is just a local endpoint — no
  component knows whether its counterpart is native or bridged.
- The .json/API form is **internal interchange, not external integration** — the
  neutral form between two of our own components across a boundary, never a surface
  for foreign APIs. Serialization exists at exactly these seams and nowhere else.
- **Conversion and transmission are separate, swappable stages.** The converter
  translates representation; a separate transmission process monitors the
  converter's output box and moves the bytes. Change the format without touching
  the transmitter; change the transport without touching the converter.
- **The return path is activation in reverse, not a poll.** A response flows back
  through the same stage types and lands at the originating component's return
  endpoint, which fires when it arrives. Nothing waits or checks.

## The setup runner IS the command structure

The setup runner wires the topology: **components + boxes, plus a converter-pair
at each representation seam.** That is the cross-kernel command structure now —
**no separate command protocol survives** — the only carried element is the
return-endpoint reference in a request (below). There are only component bodies,
boxes, and converter-pairs at the seams.

## Priority — coarse, placed, never computed

Priority lives at **box granularity, not per item.** Within a box, FIFO — no sort,
no per-item priority field, nothing to search. The scheduler does one thing:
**drain the highest-priority occupied box, take its head.** Which box a piece of
work lands in *is* its priority, so routing/placement carries the priority meaning
— the same instinct as meaning living in address layout instead of being
recomputed.

- Any priority class = a box pinned at that level, occupied on demand. **N tiers =
  N pinned boxes** (purpose-pinned tier boxes, separate from a component's own input
  box); you invoke a tier by routing work to it.
- **High priority means "do this next," never "interrupt."** The scheduler picks the
  highest-priority occupied box *at the point it selects the next unit* — it does not
  preempt a unit already running. A unit runs to completion; priority only orders what
  is chosen next. An **ordered/arrayed stream is one unit** (a compressed serial
  command), drained in order to completion by its handler — so dependent ordering
  (a MOVE after the DECLARE it depends on) is preserved intrinsically, with nothing to
  cut in mid-stream. Nothing is interrupted mid-execution.

## RECONCILE = a pinned, normally-empty endpoint, fed by the WAL manager

RECONCILE is not a verb and not a freeze. **It is not a db/cache-manager verb at all**
— the `dispatch::Reconcile` stub was removed from the dispatch surface (2026-09-22,
adversary-vetted CLEAN). Instead **the analyst messages the WAL manager directly**, and
there is a **dedicated reconcile box pinned at highest priority and normally
unoccupied.** A RECONCILE **routes through the WAL manager** (not the cache manager
directly): on a reconcile request, the WAL manager **moves the relevant existing pending
work into the reconcile box**, and it then executes first-priority. The db/cache manager
never receives or interprets a reconcile verb — it only *drains* the priority box the WAL
manager fills.

- **Scope flag — `local` vs `all` (tentative).** The reconcile request carries a scope.
  `local` reconciles this instance's pending work (the WAL open-obligation topology only);
  `all` broadens it to **swarm-sourced relevant work as well** — which, from the WAL manager's
  side, simply arrives as new work in (Pair-2 inbox → cache-manager work via the extra box); the
  coverage request itself is an analyst / cache-manager act (forward), not the WAL manager's. So
  **`all` ⊇ `local`** — it necessarily incurs the local reconcile too. The flag rides on the
  request (a field on the placement).
- **Selection is only-follow, not a scan.** The pending work already lives as the WAL
  manager's open-obligation topology; on a reconcile the WAL manager navigates that
  topology (PK / PK-prefix follows — its existing surface) to move the relevant set
  into the box. It moves what it already tracks — no predicate scan over pending work.
- **Pending work's home stays the WAL topology; the box is transient execution
  staging.** The reconcile box does not hold pending work in general — it is filled
  only when a reconcile happens, drained first, then empty again. (Resolves the
  earlier draft's mis-location of pending work into memory boxes.)
- **"First priority" = do-next, not preempt** (per the Priority section): the reconcile
  box wins the next selection whenever it holds anything; nothing running is
  interrupted.
- Termination / the "consistent point": for **`local`**, the moved set is finite and only a
  reconcile fills the box, so the consistent point *is* the moment it empties. For **`all`**,
  the swarm-sourced work arrives asynchronously as new work in, so its consistent point is when
  those obligations **close** (self-accounting) — the async settlement, not the local box-empty
  moment.
- This is the clean form of "temporary promotion": priority is the box's fixed
  identity, "temporary" is only its occupancy. (Supersedes the earlier
  freeze-the-endpoint / drain-boundary framing — no freeze, marker, or relaxation
  logic needed.)
- **Role note (a deliberate rebase, carry into the change plan):** this extends the
  WAL manager from the built "passive topology the cache manager *reads*"
  (`wal/USAGE.md`: no schedule/prioritize surface) to **actively staging reconcile
  work into a box**. That is a conscious revision of the built bookkeeper boundary,
  not an oversight. The WAL manager still only *selects and places*; whoever drains
  the box does the work.

## What the rebase keeps and replaces

**Still governs** (unchanged): only-follow / never-search; address IS identity; no
imported machinery / no invented complication; the cost hierarchy; SEE idempotency.

**Replaced:** active-instruction dispatch → monitored-endpoint activation; polling →
completion acks at a return endpoint; synchronous reciprocal writes → same-boundary
vs cross-boundary activation (same-memory is immediate, cross-kernel is the same
activation with latency), which makes the async reciprocal the natural shape. This
bears on **F4** (self-accounting vs a drained pending-list): the pending list being denied
is the **cache manager's own drainable list** — the durable open-obligation relation still
lives in the WAL manager and closes by observing the followup write. Points toward F4
affirmative; confirm when the cache-manager runtime is designed.

**Durability is not a box property:** boxes are volatile and a crash loses in-flight
boxes, by design. Completion is an **explicit ack to the requester** (payload
optional); the durable source of truth stays the **store (Postgres/WAL)**, from which
pending work is re-established on restart, lost in-flight work is **re-driven** (an
unacked request is still owed), and idempotency (SEE) makes redo safe. At-least-once =
ack + reload + idempotent redo, never persisted mailboxes.

**Reused as-is:** the WAL manager's presence=pending obligation topology (the durable
home of pending work); the six verbs, now the reaction bodies a monitor runs rather
than dispatched commands.

**Open (from `SWARM-NOTES.md`):** converter:transmission need not be 1:1 — a converter
is per-remote-counterpart, but one transmission process may multiplex many converters
onto shared peer machinery.

## Cost model / implementation constraints

Payloads are small (token_ids = a handful of base-50 couplets; notation = short
text), and a box may hold a **reference** into a shared arena rather than the
payload (the "secondary location if need be"), so per-message and per-box *memory*
overhead is nominal. What governs whether **box proliferation** is cheap is two
constraints the change plan must honour — the payload size is not the issue:

1. **Monitoring is readiness/notify-driven, never per-box polling.** Producers
   signal a readiness mechanism; the monitor services only occupied boxes (cost
   O(occupied), not O(total)). This is "monitor, never poll" applied to the
   substrate itself — idle boxes then cost essentially nothing and may exist in
   large numbers.
2. **OS-level resources stay at the bridge seams only.** Local boxes are
   lightweight in-memory queues (tens of thousands are fine). A box becoming an OS
   object each (pipe/socket/shm) would hit fd / kernel-memory ceilings — so OS
   channels appear only at converter/transmission seams, where they are pooled and
   **multiplexed** (many converters : one transmitter).

The proliferating class is the **ephemeral per-request return endpoints**: their
live count = **in-flight concurrency, not data volume** (allocated, filled once,
drained, recycled from a freelist). That count is a **steady state** — roughly
arrival rate × mean drain latency (Little's law) — so its RAM tracks *how long work
waits to be serviced*, not throughput or total data. Given the tiny payloads it
takes on the order of ~1M simultaneously-outstanding un-drained ops to reach ~1 GB
(2–3 orders of magnitude past any realistic in-flight count on a 2–4 GB machine).
Unbounded growth therefore only comes from a **monitor that stops draining** (a
stall or a leak of un-recycled boxes) — a **liveness signal to detect, not a
capacity ceiling**. Standing boxes (component inputs, the pinned
reconcile box) are few and fixed. The one genuine per-op cost is **concurrent-append
synchronization** on a shared input box under the multi-connection case (a CAS/lock
per append) — cheap for small messages, but that is where the cycles go, not in box
setup or existence.

## Endpoint identity & the dupe-address rule (tentative, 2026-09-20)

**Endpoint identity reuses the one address space — nothing new is invented.** A
node / component is addressed in the **existing token_id space** (address IS
identity; base-50 couplets; no parallel node namespace, no assignment authority).
An endpoint reference is therefore just:

- **the node/component's token_id-space address**, plus
- **a transient local mailbox locator** — which in-memory box on that instance: a
  runtime slot (`slot`, `generation`), discarded on drain, NOT a stored identity and
  NOT part of DB addressing.

Resolution: a **local** target → resolve the slot directly → append; a **remote**
target → hand the payload (address and all) to the converter the setup runner wired
at that seam → the far side resolves its own local slot. **No routing table, no
multi-hop, no node namespace** — reaching an address across the swarm is the
content-addressed pull the swarm design already provides, not a layer the endpoints
supply. *(Earlier drafts of this raised a node-id assignment scheme and multi-hop
routing — both RETRACTED as imported machinery; the base already answers node
identity and reachability.)*

**Why identity needs no authority — the dupe-address rule (Patrick, 2026-09-20).**
Addresses are suggestions/derivations that self-resolve, so no central assignment is
needed:

- **Local (same store).** A DECLARE / MOVE address is the analyst's *suggestion*,
  born of how the data connects; the analyst does not care what the final store
  address is. The **cache manager disposes** — it uses the request as given, or the
  **closest appropriate range** if that space is occupied. (This is the existing
  `@`-override / caller-proposes-manager-disposes / relocation-is-free rule; the
  "address range occupied" routines are **noted but currently incomplete** —
  flagged.)
- **Across the p2p network — two cases, one rule set, self-converging.**
  - *Same data, same address:* the **winner is picked by the one rule below (least
    conflicting, oldest if equal)**; a
    tracker encountering the dupe incorporates the deprecated one as a **delete or
    move**, **sanctioned by its replacement with the winning address**.
  - *Different data, same address (a true collision):* systems encountering it
    **separate the two** and issue a **combined ledger update** describing the
    separation; the **same address rules then apply** to the separated particles
    (re-addressed by the standard content derivation, and any further dupe that
    creates resolves the same way — recursive).

  Detection is a direct comparison of the token's actual content — its **connections
  and masses** (all the DB holds): same content = the same token (dedup); different
  content at one address = a collision. No address-hashing, no content-hash scheme.
  This is the torrent mapping: the **address replaces the piece-hash** as the manifest
  key, the **payload is the data packet** for that address, and **sections of the swarm
  are a torrent file-selection tree** (a trunk/subtree is a selectable region to pull).
  Nothing is hashed — the address already *is* the key. Individual packets are **tiny**
  (a token's connections + mass) even when the receiver assembles many into a larger
  local DB — the transferred pieces stay small as the assembled store grows. The rules
  then make resolution automatic — no minter, no authority. **A given address may flux across the network at first
  but normalizes rapidly:** the rules are deterministic and holders derive the same
  resolution independently, so the combined ledger update propagates and every holder
  converges — the transient disagreement damps as it spreads. Flux is **safe**, not
  merely tolerated: nothing holds an address (the analyst navigates by connection;
  relocation is free), so a momentarily-wrong address is corrected by a move with no
  stale handle to break.
  - **Convergence is by iteration of one rule, not first-time-identical separation
    (Patrick, 2026-09-20).** The single rule — **least other conflicting change wins,
    oldest if equal** — applies to *whatever conflicts exist*, including a conflict
    created by two holders separating the same collision *differently*: that
    divergence is itself just another conflict the same rule then settles, so the
    system iterates to a fixpoint. **"Oldest" is a plain timestamp carried in each package** — a local
    value comparison, no global clock. Everyone compares the same carried value and
    picks the same winner; that determinism is what converges. There are therefore **not** two resolution modes: local disposition
    ("closest appropriate range") merely *proposes* an address; any conflict that
    proposal creates on the network reconciles by the very same rule, exactly as any
    other conflict does. (Corrects an earlier draft here that posited a separate
    content-deterministic network mode — unnecessary.)

**Residual (small, local):** recycled-slot safety — a late/duplicate reply must not
land in a mailbox slot since recycled to another request. The `generation` stamp on
the local slot handles it (a reply carrying a stale generation is dropped). Purely
local runtime plumbing.

## WAL-manager comms wiring + swarm indexing (2026-09-21 — WAL-integration design, in progress)

Patrick-driven, the first WAL-manager integration onto the activation substrate. The
WAL manager's coupling is a set of boxes.

> **⚠ Update (2026-09-22) — Pair 1 BUILT, and narrowed by the source-blind reframe.**
> Pair-1's steady-state push is now built as `wal/wal_kernel.{h,cpp}`
> (`WAL-INTEGRATION-PLAN.md`, adversary-vetted plan + build). Clarifications from Patrick
> (2026-09-22) sharpen the wording below: (1) **source-blind = LOCATION-blind, not
> identity-blind.** A kernel does not know or care *where* a counterpart physically sits
> (local vs remote); it **is** aware of *who* it receives from and *who* it assigns work to.
> **Every outbox is a specific counterpart's inbox** — the box IS the addressing; **each
> kernel has multiple in/out boxes** (an interactive internal-state mail system). So "emits
> them to *that originator's inbox*" is right — the WAL manager assigns reciprocal work to a
> known counterpart (the cache manager) by choosing its out-box. It is special only in that
> its **primary stream is internally-generated reports** for **internal** work (→ cache
> manager); **external** work is fed by/to the **swarm manager** (Pair 2). This pass has one
> cache-manager counterpart, so one reciprocal out-box and no selection yet; selection-by-who
> (several counterparts) and **reload repopulation** (each thread re-derives the work it holds
> for its counterparts from durable state — WAL re-emits from `list_open`) are deferred.
> (2) Serialization / the **"API pair"** is a **separate matched-pair bridge**, built
> separately, that shuttles content between a local and a remote box in **split mode** — not
> part of this Pair-1 wiring. Still deferred: reload repopulation, that shuttle, the live feed,
> Pair 2 + the extra swarm box, and the cache-manager consumer.

**Pair 1 — internal (local db → cache manager).**
- **Inbox: a per-source SET** of local WAL-record feeds (core, each language shard, the
  personality DB). Per-source ordering is the box FIFO; cross-source independence is just
  the boxes being separate — no global order needed. (The box FIFO is a *volatile view* of
  the durable per-source order, which lives in the WAL manager's History.)
- **Outbox: the reciprocal work list** → the cache manager that created the initiating
  entry. Instead of the cache manager polling what's owed, the WAL manager recognises the
  owed reciprocals from the record's own data and emits them to that originator's inbox
  (which cache manager falls out of the initiating entry's own origin).
  - **Volatile transport, durable relation.** The outbox is *notification/transport only* —
    the durable obligation stays in the WAL open-obligation relation; a lost outbox entry is
    re-driven on reload (durability ruling above). This does NOT re-locate pending work into
    a box: the box notifies, the relation persists, and close is still by observing the
    followup write come back around the WAL feed.
  - **Deliberate rebase (carry into the change plan).** This PUSH supersedes the built
    PULL/observer contract — `wal/USAGE.md`'s "not to be told what to do next," "no
    schedule/prioritize surface," "never drives the cache manager." On the activation
    substrate everything is push; polling `list_open` is the demoted idiom. Close semantics
    are unchanged (still self-accounting). Flag `wal/USAGE.md` / `WAL-PLAN.md` / `NOTES.md` /
    `HANDOFF.md` for supersession — same treatment the RECONCILE section gives its rebase.

**Pair 2 — with the swarm manager (self-contained, both ways).**
- The **swarm manager is the p2p component — NOT yet addressed** (a forward reference, like the
  cache-manager boxes). This pass wires only the WAL manager's *coupling* to it, not its
  internals; its design is the p2p / swarm section, still gated on the WAL-data-shape exam.
- **Inbox:** incoming change data from the swarm manager, to **unpack**.
- **Outbox:** composed **internal delta packets** passed back to the swarm manager — the
  outbound prep (local change → manifest/packets for peers).
- This **activates the WAL manager's swarm-side facet**, which `WAL-PLAN.md` §2 and
  `SWARM-NOTES.md` mark deferred — a deliberate design-discussion rebase; cross-ref those.

**Extra box — unpacked swarm change → cache-manager work list.** The unpacked inbound
change (pair 2's inbox) becomes cache-manager work, its own outbox to the cache manager,
separate from the swarm-manager pair. So **two outboxes point at the cache manager**
(pair-1 reciprocals + this unpacked swarm work). Because a box is natively multi-connection,
both can already feed the cache manager's one input box; the **shared-outbox extension**
(deferred) is only the further question of whether they stay two priority-distinct boxes or
collapse to one ordered box (priority is box-granular).

**Fan:** the pair-1 local WAL feed drives **two** outputs — reciprocal work → cache
manager, and composed delta packets → swarm manager. That fan is where composing sits.

**Swarm indexing (under the trackers) — design-discussion; still GATED on examining the WAL
data shape first (`SWARM-NOTES.md`, `HANDOFF.md`). The reconcile-vs-extend split *informs* the
delta-span (sync) vs state-range (bulk-coverage) fork but does not resolve it — the fork stays
formally parked pending the data-shape exam.**
- The **address tree IS the index** the trackers carry (the manifest = the address tree; the
  address is the manifest key — no hashing).
- **Address is identity/key.** A packet's **content (connections + masses) + when it was
  set** — the **set-time is the immutable timestamp carried in the package** (the same stamp
  the dupe rule's oldest-wins uses; a local value comparison, no global clock) — are what
  drive dedup / collision / versioning: content compared for same-vs-different, set-time the
  oldest-wins tiebreak. Identity is the address; content + set-time are the version at it.
- Each tracker holds **only its own current state** — full detail where it has coverage;
  where truncated, it holds the **existing coarse label token** (its already-stored mass +
  connection indicators = the core distillation / LoD rollup), **NOT a newly stored or
  computed aggregate**. Consistent with NOTES' "centroid not stored — had by having the
  label": the coarse node already exists, so a truncated area is that coarse token shown
  instead of descended.
- **Truncation is the LoD dial applied to coverage:** a truncated node is the tree seen at
  coarse LoD (its coarse token); pulling coverage descends it into detail. A tracker is
  coarse where it has not pulled, fine where it has — the existing rollup reused, nothing new
  to build.
- A tracker's partial coverage is a **coarse-LoD SUBSET of the ONE shared address tree** (the
  address is the swarm-wide manifest key), never a replicated fork — one data space, not many.

**Pull / reconcile vs extend, and the analyst's deviation signal (2026-09-21).**
- The **swarm manager** keeps **semi-contemporaneous** records of what's available across the
  swarm — the tracker catalogue **is the held core distillation** (SWARM-NOTES' connection
  indicators), not a new store; roughly current, not perfectly live.
- **For the WAL manager, swarm activity is just new work coming in** (Pair-2 inbox → composed
  into cache-manager work via the extra box). It composes whatever arrives; it does not itself
  distinguish extend- vs reconcile-prompted, nor originate swarm coverage requests — requesting
  new coverage (extend) and raising reconcile-`all` are cache-manager / analyst-layer acts
  (forward).
- **Reconcile** settles changes to coverage the instance **already holds**: the WAL manager,
  from the **locally-defined dataset**, determines which aspects to pull/reconcile, and a
  change within the existing (locally-held) tree is **auto-captured** — no request needed.
  (`local` vs `all` on the reconcile request scopes this to the instance or the swarm.)
- **Extend** is the distinct operation — pulling **new granularity** the instance does NOT
  hold (descending a truncated/coarse node into detail): an explicit **cache-manager →
  swarm-manager** request, not automatic (the cache manager decides what to attach / at what
  granularity — SWARM-NOTES division of labour). **This is a CACHE-MANAGER box — a forward
  reference, NOT part of the WAL-manager wiring above; it gets defined when we address the
  cache-manager boxes (this pass is scoped to the WAL manager).**
- **Analyst deviation gauge — analyst-facing context; derived from work already tracked, NOT a
  new stored statistic.** Two indicators, **both relevance-scoped to the current focus**:
  - **Local deviation** = the cache manager's owed **cross-pollination (reciprocal) backlog**
    that bears **directly on the current primary focus**. A "side" connection may matter to
    others but is not counted if it's not relevant to the local working cache.
  - **Global deviation** = how much **pending work from p2p swarm updates** is relevant to
    what's being examined — **NOT a swarm-wide total**; it is swarm-sourced pending work filtered
    by the active focus (derivable from incoming swarm work × relevance, no cross-node aggregate).
  Both gauge how much relevant work is outstanding and how directly it touches the active set —
  the relevance trigger RECONCILE leans on. (Analyst-layer indicator, grounded in the WAL/cache
  work topology; wired with the analyst/cache-manager boxes later.)

**Forward (named focused discussions, not this WAL pass) (Patrick, 2026-09-21):**
- **WAL-report → WAL-inbox translation** — how raw WAL records (Postgres logical-decoding
  output) become inbox messages. Its own focused discussion; relates to the live-feed /
  wire-form (G6) gate.
- **`verify` — the analyst's integrity gate** (analyst-layer, forward). **In essence it asks:
  "does our model allow for this to be true?"** — a model-consistency check (is the claim
  supported by the established connections/masses and the swarm's version?), not an authority
  check. It guards **any delete action or significant connection assertion** (the destructive /
  high-impact operations) and is
  the system's **self-defence against attempted forced bias** — distorting the model by forcing
  deletes or assertions. It is **swarm-informed**: the link to swarm conflict data is the
  leverage — a genuinely well-supported change does not conflict, whereas a forced-bias attempt
  **diverges from what the network holds** and surfaces as conflict for `verify` to catch and
  resist (the content-addressed / oldest-wins consensus turned into a defence: bias shows up as
  divergence). Realizes the base's delete-gating / no-forced-equivalence stance and the old
  cross-network (G7) validation. The WAL core holds no confirmation logic — it only books and
  routes.

## Open / to pin

- **READ / all functions move to activation — RESOLVED (Patrick, 2026-09-19): the
  model is fully uniform, no synchronous carve-out.** The analyst is itself a
  monitored-endpoint kernel: READ → prove-in-sim → DECLARE becomes
  emit-read-request → result lands in a return box → sim → emit-declare,
  event-driven, no blocking wait. READ mutates nothing, so it emits no WAL report
  and opens no obligation — its completion is just its result landing in the return
  box (the substrate's own presence=done signal). Clean split: **boxes = universal
  activation/coupling for everything; WAL = the write-obligation accounting layered
  on top only for deferred cross-work** — read-shaped / non-mutating calls never
  involve the WAL manager.
- **Request→return correlation — RESOLVED (Patrick, 2026-09-19): the request names
  its own return endpoint.** The requester designates a return box and carries that
  endpoint in the request; the serving component emits its result there; the box
  *is* the correlation — no correlation token, no matching table, no search
  (consistent with only-follow / placement-carries-meaning). Two consequences to
  carry: (1) return destinations are **caller-supplied and dynamic**, so not every
  edge is static setup-runner wiring — the runner wires the standing topology, the
  return endpoint is supplied per request; (2) a return endpoint must be
  **nameable** (a stable logical reference, not a raw memory pointer) so it survives
  a converter crossing — the converter-pair resolves the named endpoint to boxes on
  each side, same as any payload. **Endpoint naming/addressing is resolved below
  (see *Endpoint identity*):** a token_id-space node address + a local mailbox slot;
  only that name crosses a converter, never a raw pointer. (3) **Any point is natively multi-connection.** Because each
  requester carries its own return endpoint, a component's input box can be fed by
  many requesters at once and each answer routes back independently — no held
  connections, no connection state to manage; a component monitors a box anyone can
  fill rather than holding channels. This **subsumes the existing multi-analyst
  "per-analyst input/response link" requirement** (`NOTES.md` "Process runtime"):
  that link IS the caller-supplied return endpoint. (The other half of that NOTES
  clause — consistency of "maintained aggregates" — is moot: the DB retains only
  **connections and token masses**; a label's **centroid is not stored**, it is had by
  having the label, derived from its member connections and those members' masses.
  Nothing to keep consistent at the DB level; connected-dataset overlap is a
  **cache-update** matter, out of scope here.) No-held-connection mirrors the
  base's no-held-address rule — the return endpoint is the invariant, the connection
  itself is fluid. Independent requests interleave FIFO in the shared input box and
  are safe precisely because correlation is by return endpoint, not by position
  (the same stance as WAL's per-source-order / no-cross-source-global-order).
- **Converter:transmission topology** — per-remote vs shared multiplexer (above).
- **UPDATE_CACHE / REBASE_CACHE** — plausibly follow RECONCILE into
  box-priority / scope operations rather than distinct verbs; check when reached.
- **Secondary-location referral** — box occupancy itself is resolved (ordered
  queues, drained by the monitor). When/what triggers a box to refer out to a
  physical/secondary location (persistence, oversize, cross-machine) is unpinned.
