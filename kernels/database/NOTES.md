# database/cache — data protocol & address-layout notes

> **⚠ MESSAGING REALIGNMENT — governing (2026-09-22).** The **entire messaging of this
> kernel is being realigned to the monitored-endpoint activation format**
> (`network/ENDPOINT-ACTIVATION-NOTES.md` is the messaging model). **This is the
> plan for that realignment, not a build history.** Where a thing is being redone it is
> **SUPERSEDED here, not left marked DONE** — a stale "BUILT/COMPLETE" on a redone piece
> drags the old precepts (synchronous verb-dispatch, "issue a command at the kernel,"
> poll-for-result) back into current work.
>
> **Supersession boundary (scoped to the messaging/invocation layer):**
> - **SUPERSEDED** — the invocation/coupling model: the synchronous `dispatch_one` /
>   `dispatch_stream` call-and-return; "`db_runtime` dispatches on a leading verb"; the
>   command-protocol / external-wire framing as the delivery model; polling for results.
>   (The `dispatch/` *logic is unchanged and reused verbatim* as the reaction-body core —
>   see tier 2 / `dbmanager/`; only its role as the external call-and-return entry point is
>   superseded.)
>   Replaced by: monitored-endpoint activation — every function is a **reaction body** on a
>   box, data-arriving-is-the-activation, results returned to a **caller-supplied return
>   endpoint**, "the setup runner IS the command structure; no separate command protocol
>   survives."
> - **SURVIVES (still BUILT)** — the six verb **cores' logic** (DECLARE / READ / MOVE /
>   ADD_CONNECTION / DELETE_RECORD / DELETE_CONNECTION against the store), now run *as
>   reaction bodies*; arraying-as-one-ordered-unit (maps directly to "an arrayed stream is
>   one unit drained to completion"); the schema, `codec/`, `controller/` (the DB door),
>   `seed/`; and the whole data model (address-is-identity, pairwise, literal/label, mass).
>   The realignment reshapes *how these are invoked and coupled*, not what they compute.
>
> **Pinned so far (2026-09-22):**
> - **The db/cache manager is one role.** Its *current* functions are the record-tier
>   surface; the warm-cache shape is the same role's still-unfleshed aspect.
> - **Two channels in.** *Current work* flows **direct with the agent process** (not via the
>   WAL manager); *deferred + outside (swarm) work* is stored in the **WAL manager**, which is
>   the db/cache manager's **persistent work store**. Consequence — **F4 resolves affirmative**:
>   the manager keeps **no durable pending-list of its own**; the WAL open-obligation relation
>   IS its pending work, and it repopulates outstanding work from `list_open` on reload.
> - **RECONCILE is no longer a db/cache-manager verb** — the `dispatch::Reconcile` stub was
>   **removed** (adversary-vetted CLEAN, `dispatch_test` 31/31 green). The analyst messages the
>   **WAL manager directly**, which promotes the relevant pending queue into a **priority
>   in-box** the manager drains.
> - **WAL manager Pair-1 push is BUILT** (`kernels/wal/wal_kernel.{h,cpp}`, `kernels/wal/WAL-INTEGRATION-PLAN.md`):
>   source-blind (=location-blind, knows its counterpart), emitting owed reciprocal work to the
>   cache-manager out-box, fixture-fed. Local activation primitives BUILT (commit `b97034a`).
> - **Tier 2 (analyst reaction body) is BUILT** as `dbmanager/` (`db_manager_kernel.{h,cpp}`
>   + test + README; `TIER2-PLAN.md`, adversary-vetted plan + build): `DbManagerKernel` runs
>   the record-tier verbs as reaction bodies over the shared `Controller`, reusing `dispatch/`
>   verbatim, returning each `Result` to the request's caller-supplied `reply_to`. Fixture-fed;
>   endpoint advertising + tiers 1/3/4 deferred. `PASS`, 38/38, ASan/UBSan clean.
>
> **Open / still being designed (the plan fills in as we pin it):** the agent-facing
> reframe of the record-tier surface onto boxes + caller-supplied return endpoints; the warm
> cache shape; ingest-atomicity ownership on the box model; the still-deferred WAL seams
> (reload repopulation, the serialization/"API-pair" split-mode shuttle, the live report feed,
> Pair-2/swarm coupling, the cache-manager consumer). **The wider project needs several doc
> updates after this kernel is realigned.**

Working notes for the hcp3_core database/cache kernel set. Prose/design record; the code
and its tests are the source of truth for behaviour. Canadian English.

> **Available work streams for the next session → see `HANDOFF.md`.** The record-tier
> **cores** and the WAL manager are built; the **kernel messaging around them is being
> realigned** onto the activation format (governing banner above) — so the messaging/
> invocation layer is SUPERSEDED, not DONE. The db/cache-manager realignment is the active
> stream, with the swarm/p2p layer preliminary and gated behind it. `HANDOFF.md` is the
> reload pointer — read it first.

## Governing principle

The DB is the **strictest form of the data protocol** — any system built on it
takes that protocol as its focus. The cost hierarchy that shapes every choice,
in concrete terms:

- **Declaration touches 1 data point.** Near free — give a point an address /
  a group and you are done.
- **Calculation touches a minimum of 3 data points, plus the resource to
  calculate.** So the CPU **only follows stored lists, it never searches** (no
  reverse-search indexes anywhere in the schema).
- **Allocation costs a calculation plus the work that predicated it.** So
  address space is spent deliberately, in whole units, and its layout is made
  to carry meaning rather than be recomputed.

Everything below is a consequence of that hierarchy.

**Computational tax is paid exactly once.** Anything derivable — reverse edges,
relative parents, masses, centroids — is computed once (at write/reconcile) by
its owner and STORED, then read as a pure follow. "The cache manager owns X"
means it is *who* computes X and *how* X is stored — NEVER that X is left
unstored to be derived on demand, which would force a search on every
non-downward action. Store the tax; never re-pay it.

## Address space

An address is an ordered sequence of base-50 couplets (alphabet `A–Z`,`a–z`
minus `o`,`O`; see `codec/`). Treat each single character as a digit: freeing
one more digit multiplies the reachable set ×50 — nested **rings** off a shared
prefix.

Off the `AA.AA.AA.AA.` prefix:

| Freed | Reachable | Example |
|---|---|---|
| last char (`A*`) | 50 | `AA.AA.AA.AA.A*` |
| whole 5th couplet | 2,500 | `AA.AA.AA.AA.` |
| one digit deeper | 125,000 | `AA.AA.AA.A` |

A **trunk** is a leading-character subtree at a given ring — `AA.AA.AA.AA.A*`,
`…B*`, `…C*`, … A trunk is not a fixed width: it grows **deeper** (adds rings)
to hold however many members its kind needs. ("**block**" is used
interchangeably for a trunk in the DECLARE / addressing / `AFTER` sections — the
same unit; `AFTER:b`'s `b` is a trunk.)

## Per-kind trunk allocation

Kinds are laid out **trunk-aligned**, one kind per trunk (a kind may span more
than one), sized to demand:

- A kind rounds **up to the next whole trunk letter**. It fills part of its
  trunk(s); the unused tail is left empty as deliberate **sparse headroom**.
- The next kind starts at the **next unoccupied trunk**, never butted against
  the previous kind's last used slot. If a kind finishes anywhere in the `D*`
  range, the next kind starts at `E*` — `D`'s remainder stays sparse.

Running example (root namespace `AA.AA.AA.AA.*`):

- **single hex codes + `0x`** — the `A` trunk (`AA.AA.AA.AA.A*`). Small; the
  whole kind fits in one trunk. (`0x` = `AA.AA.AA.AA.AA`.)
- **hex couplets (256-byte codes)** — start at `AA.AA.AA.AA.BA`, taking as much
  of the `B` trunk as 256 requires (expanding into depth past B's first 50).
- **next kind** (3- or 4-couplet set — **undecided**) — the next clean trunk
  (`C`/…), leaving B's tail sparse.

The assignment layer carries a **trunk → kind map**, extended one trunk at a
time. Boundaries past "hex couplets" are deliberately unfixed, so nothing may
hardcode them.

## Division by sparsity — a property of the data, NOT code

Because each kind sits in its own trunk(s) with an empty tail, the CPU runner
can sweep a trunk range as one contiguous **spread**, and the sparse gap is
itself the boundary between kinds. There is **no delimiter, no boundary check,
no grouping logic to write** — the division falls out of where the points sit.
The assignment side's only job is to place kinds in whole trunks and leave the
gaps. **Do not implement the division; it is emergent from the layout.**

## Analyst command semantics (the relative assignment rule)

The normal ingestion command is group-level, not point-level:

> "Enter these N points as **group G** at **level/trunk L**."

The system lays the points into trunk L at sequential, contiguous addresses and
records their membership in G (`members`/`member_of` — see *Relationship model &
type — firmed 2026-09-17*). The analyst designates the trunk;
the system does the sequential fill and the grouping. The fill is deterministic
**from the analyst-designated trunk** — sequential placement, no search. (The next-free mechanism — analyst-supplied
starting address vs a per-trunk cursor the controller advances — is an OPEN
decision; see Open decisions. This section assumes the analyst-supplied-start
form, which needs no cursor.) Per-point provisional addresses
remain available for exceptions.

## Hard constraints

- **Only-follow, never search** — resolve a direct address, follow stored PK
  lists; no reverse-search index exists.
- **Address IS identity** — same construction ⇒ same address ⇒ one token
  (deduped by `mint`'s SEE — the PK-existence probe: look up the token_id, and if
  it already exists, no-op / link-only instead of a second insert); a different address is a different
  token. **No aliasing/forwarding**; two distinct addresses are never bridged
  to a common identity, and nothing "papers over" a duplicate.
- **No invented complication** — compute with no definable direct purpose is
  garbage. No imported machinery, no anticipatory abstraction.

## Relationship model & type — firmed 2026-09-17 (governs on conflict)

Firmed this session with Patrick. Where it conflicts with anything below, **this
section governs**: it supersedes the `token_sibling_group` / `SIBLINGS`
membership design, the declared-`TYPE` gating, the `token_child` "two write
sources by type" front-door for membership, and the literal/label
parents-children-siblings breakdown in **Type semantics**, **DECLARE format**,
**The literal intake formula** (and its label-dual continuation),
**Relationships stored BOTH directions**, and **In flight — next to lock**.

**Two orthogonal relationship axes, each a reciprocal pair, each a one-level
arrayed follow.**

- **Structure — `parent` / `child`.** A token's ordered constituents (`parent`)
  and, reciprocally, the tokens that use it in their composition (`child`).
  Structure is literal composition ONLY; it no longer carries membership. This
  de-overloads `token_parent` (it stopped meaning both constituents and a
  sub-label's super-group).
- **Membership — `members` / `member_of`.** What a grouping contains (`members`)
  and, reciprocally, the groups a token joins (`member_of`). This REPLACES the
  sibling-group in full: `token_sibling_group` is dropped.

Both pairs return a single one-level list per follow (the same shape the old
parent/child returned). Membership stores both directions (write one side, the
reciprocal is maintained), exactly like structure — only-follow, never search.

**Type is emergent and LoD-relative — `TYPE` is dropped.**

- A token reads as a **label / grouping** when it has `members` in scope, and as
  a **terminal literal** when it has **no members relevant to the current LoD and
  scope**. Terminality is NOT absolute: the same token_id is a coarse-LoD
  terminal and a finer-LoD label — the literal↔label ladder (LoD *k*'s labels are
  LoD *k+1*'s literals), already described under the LoD rollup.
- Because the reading flips with the read's LoD/scope, type CANNOT be a stored
  branch. The declared `TYPE` field and the `token.type` column are both dropped.
  Type is a pure read-lens over the members-in-scope reality. DECLARE is no
  longer `TYPE`-gated: validation is **structural** — which downward field is
  present (`PARENTS` ⇒ structure, `MEMBERS` ⇒ grouping); what is declared is
  stored, the readings emergent.

**DECLARE fields (revised):** `ADDRESS`, `NOTATION`, `PARENTS` (structure
constituents), `MEMBERS` (grouping — replaces the old label `CHILDREN`),
`MEMBER_OF` (membership, up). No `TYPE`. The structure `>=2` anti-alias floor and
the membership `>=1` floor still hold, now selected by which field is present
rather than by a declared type.

**Pairwise is the substrate, not a constraint to reconcile against.** Every
calculation the engine does is pairwise — that is the entire point of the DB. A
pairwise base calc, once recorded (combined masses + connections), lets the model
rebase by a **read and alignment over nominal ticks**: new data recomputes
minimally off the established relationships, which is what permits the proper
mathematical **exclusion of state space**. "All connections are pairwise" IS this
substrate; the structure/membership split is storage detail beneath the one
pairwise primitive, never a competing category.

**"No connection-kind operand / nothing stored as a category"** was a guardrail
against one specific error: a hidden **equivalence table** declaring distinct
tokens equivalent through a hidden connection — forced equivalence / covert
aliasing, which violates **address-IS-identity / no-forwarding** (the same reason
`token_forwarding` was removed and MERGE is forbidden). It is NOT a ban on
distinct relationship stores. The relationship "kind" is encoded in the
memberships/structure themselves and read from the axis an edge sits on — never a
stored kind label, never an equivalence bridge.

**Schema consequence (to build):** `token` drops its `type` column;
`token_sibling_group` is dropped; `members` and `member_of` are added as the
membership reciprocal (mirroring `token_parent` / `token_child`). Net store:
`token`, `token_parent`, `token_child`, `members`, `member_of`.

## Process runtime — the cache manager is a complete runtime

The cache manager (this database/cache kernel family) is a **complete, standalone runtime
process**, not library pieces called ad hoc: it owns the DB (only the process
controls the DB) and its primary duty is serving input/output requests. It must
be complete on its own and may be **detachable to ride with the swarm**;
whether it is later bundled with other processes is a separate concern.

> **Build status (2026-09-19):** the record tier writes **synchronously** — every
> reciprocal is written inside the authoring op (`mint`'s WIRE, `add_membership`
> both directions). The async **file-now / wire-later** split described below
> remains the **cache manager's own runtime to build** — it is NOT implemented by
> the WAL manager, which is now BUILT (`kernels/wal/`, see *Current build state*) as a
> pure bookkeeper: it books the return-path/mass obligations a change owes and
> monitors for their followup writes (tolerating the same-batch-today /
> later-batch-future gap via fixtures), but it does not perform, drive, or design
> this file-now/wire-later split itself.

Work balancing is **foundational, not deferred** — the temporary Python loader
is real I/O and must be handled right from the base:

- Under a fast bulk load (e.g. the Python loader dumping definitions), **filing
  the incoming data takes precedence over cross-processing it.** Filing =
  recording the token and its declared data; cross-processing = the deeper
  cross-linking (the WIRE / `token_child` structure-reverse **and the
  `members`/`member_of` membership reciprocal**) and feedback.
- The process carries a **pending work list**: deferred cross-processing is
  queued and done asynchronously, behind the primary input/output request
  handling.
- Postgres is the swarm DB partly because its **WAL records are p2p-networking
  friendly** for the swarm.
- The cross-processing workstream **may be detachable** — riding with the swarm
  (picking up deferred work off the p2p-friendly WAL) rather than living in the
  core process at all. The core process would then stay lean on primary I/O,
  with cross-linking/feedback carried as a distributed swarm workstream.
- Today `mint` does link+wire synchronously; the runtime splits this into
  **file-now / wire-later-via-pending-list**, the reverse indexes (`token_child`,
  and likewise the `members`/`member_of` reciprocal) being eventually-consistent
  while deferred work drains.
- **⚠ Revisit (2026-09-19):** the WAL manager (now BUILT, `kernels/wal/`) implements a
  **self-accounting** completion model — followup obligations booked from a
  change's own data and closed by *observing* the followup writes, with **no
  pending-list drain** (see `kernels/wal/WAL-PLAN.md`, `kernels/wal/README.md`) — bears directly on
  this pending-list / wire-later runtime. Whether self-accounting **replaces**
  the pending-list-that-drains here or only governs the WAL manager's own view is
  unresolved (**F4, NEEDS-PATRICK**) and **must be revisited when the
  cache-manager runtime is designed/built**. Sitting alongside F4 in that same
  revisit: **ingest atomicity across the observe→book cycle** — dropped as a
  WAL-manager item (Patrick, 2026-09-19; the WAL manager's door gets no
  transaction surface), but the underlying gap (a crash between the WAL
  manager's `close()` and `record_seen()` can silently mis-record a
  settlement) is real and becomes this runtime's problem to solve, since
  cross-kernel-cycle durability belongs to whichever runtime orchestrates the
  cycle, not the bookkeeping door on one end of it. See `kernels/wal/WAL-PLAN.md`'s status
  blockquote for the full trace.
- **Multi-analyst.** The cache manager may serve more than one analyst and needs
  a per-analyst input/response link. Maintained aggregates (own masses, label
  centroids, reciprocal listings) therefore have a single owner — the manager —
  so they stay consistent across async multi-analyst updates.

## db/cache-manager kernel — box & priority structure (messaging realignment, 2026-09-22)

The db/cache manager is **one monitored-endpoint kernel**. Its scheduler drains the
**highest-priority occupied box, head-first** — do-next, never preempt (activation model);
priority is box-granular, placement carries the priority meaning. The tiers, highest →
lowest:

1. **High-priority reconcile box** — pinned top, normally empty. Filled *only* by the WAL
   manager promoting already-tracked pending work into it on an analyst reconcile (the
   analyst messages the **WAL manager directly**; not a db/cache-manager verb). Drained
   first; empty = nothing to reconcile. (Substrate reconcile-box pattern, not a verb.)
2. **Analyst-command box(es)** — current work. **All analyst requests are one stream** — the
   verbs are NOT split into priority-distinct boxes (READ / DECLARE / MOVE / ADD_CONNECTION /
   DELETE share the stream; an arrayed stream is one ordered unit drained to completion).
   **Possibly multiple analysts**, each its own box **at this same tier** — the per-analyst
   input/return link IS the caller-supplied return endpoint. Each result returns to its
   request's return endpoint.
3. **Pending-work box** — deferred cross-work. Fed by the WAL manager's **Pair-1 push** (owed
   reciprocal work: the WIRE / `token_child` structure-reverse, the `members`/`member_of`
   reciprocal, mass). Drains **behind** analyst current work — the **filing-over-cross-processing
   precedence, now expressed as tier-2 > tier-3**, not a runtime pending-list discipline. Each
   write here becomes the next WAL report; the WAL manager closes the obligation by observing it
   (self-accounting). Reconcile is the mechanism that lifts items from this tier up to tier 1.
4. **Standing maintenance commands** — background upkeep (background cache updates, etc.).
   Lowest priority; standing box(es) that run only when nothing above is occupied. The
   cache-tier ops (`UPDATE_CACHE` / `REBASE_CACHE`) plausibly live here as standing
   box-priority operations rather than dispatched verbs (consistent with
   `network/ENDPOINT-ACTIVATION-NOTES.md`'s "UPDATE_CACHE / REBASE_CACHE plausibly follow RECONCILE
   into box-priority operations").

**Consequences:** filing > cross-processing is the tier-2 > tier-3 ordering; reconcile =
moving relevant tier-3 work into tier-1 (do-next), the WAL relation staying the durable home
(F4-affirmative — the manager keeps no durable pending-list of its own); the six verb cores
run as **reaction bodies** on tier-2 (analyst) and tier-3 (deferred owed-work); the `dispatch/`
synchronous router is **superseded** by the scheduler + handlers.

**Open (minor):** fairness/ordering *among equal-tier analyst boxes* when several analysts are
occupied at tier 2 (the scheduler's within-tier selection rule) — not yet pinned; a scheduling
detail, not a structural one.

### Tier 2 — analyst reaction body (design pinned 2026-09-23; build plan `TIER2-PLAN.md`)

The analyst-facing current-work surface, re-expressed on the activation substrate. A rehome
following the `WalKernel` precedent — **the six verb cores and the `dispatch/` logic do not
change**; only invocation and result-return do.

- **The analyst is a peer kernel composing per its own ruleset, NOT a human at a command
  line.** Requests are therefore **well-formed by construction** (the return endpoint and the
  arena handle are part of what it emits). The manager does **not** validate or reject
  peer-kernel requests; a malformed envelope is a programming/wiring bug that fails loud like
  any other (as `WalKernel` lets a bad handle throw), not an analyst-facing rejection path. The
  only "rejection" tier 2 has is the **core's own semantic `Result`** on a well-formed request.
- **One `DbManagerKernel` instance, one handler, registered on each analyst box** (tier 2),
  all sharing the one `Controller` (the DB has a single owner — the manager). Multiple analysts
  = multiple boxes at this tier.
- **Message = `{ payload = command-arena handle, reply_to = return endpoint }`.** A
  `command::Command` is not serialized into the opaque box payload (serialization is the
  deferred G6 wire seam); the payload is an **arena handle** into a side arena of request
  values (the `WalKernel`/`Report` pattern), fixture-fed by the driver first. `reply_to` is the
  **caller-supplied return endpoint** — the manager is **advertising-agnostic**: it sends the
  result to whatever `reply_to` the request carries and never resolves, discovers, or allocates
  an endpoint. **Endpoint advertising / cross-connection is the (not-yet-built) system
  configuration routine** (the setup runner establishing + cross-connecting advertised
  endpoints); until it exists the fixture-fed driver stands in for it, supplying return
  endpoints directly. That caller-supplied link IS the multi-analyst per-analyst I/O link.
- **Handler body reuses `dispatch/` verbatim:** resolve handle → request → run `dispatch_one`
  (a single `Command`, any verb incl. DELETE) or `dispatch_stream` (an ordered `AdditiveCommand`
  stream, one unit to completion) over the `Controller` → append `Result`(s) to a result-arena →
  `sender.send` that handle to `reply_to`. **Both request forms kept** (DELETE cannot ride a
  stream structurally, so the single-`Command` form stays regardless).
- **Core-non-destructive — no WAL emission here.** Reports reach the WAL manager from the
  Postgres logical-decoding feed (deferred seam), NOT from this handler; READ mutates nothing,
  emits nothing, opens no obligation — its result just lands at `reply_to`. The file-now/wire-
  later split is the forward tier-3 consumer's concern, not tier 2.
- **Deferred seams (named, not built):** the analyst-side converter / live feed (fixture-fed
  first cut); **endpoint advertising / cross-connection = the system configuration routine**
  (above); component-originated sub-requests (the analyst's READ→sim→DECLARE loop is the
  analyst kernel's business, modelled as the external driver, per the substrate's deferred
  sub-request seam).

## Type semantics — literal vs label

> **PARTIALLY SUPERSEDED 2026-09-17** — see *Relationship model & type — firmed
> 2026-09-17*. The literal↔label distinction and the LoD-rollup reasoning stand,
> but membership is now the `members`/`member_of` pair (not siblings), structure
> is `parent`/`child` (de-overloaded), and `TYPE` is dropped (emergent,
> LoD-relative). Read the paragraphs below through that lens; where the "siblings"
> / declared-type wording conflicts, the 2026-09-17 section governs.

**Governing distinction (firmed 2026-09-15):** literal chains are *pure
structural composition rules* — a literal is fully defined by its construction,
closed and self-describing. Labels are *what those compositions mean outside of
literal construction* — the interpretation the structure cannot yield on its
own, assigned and anchored down onto a naming literal. Structure (literal) is
the substrate the store computes on; meaning (label) is the semantic layer laid
over it. This predicts the asymmetries below: notation derives for a literal but
is assigned for a label; mass sums constituents for a literal but centroids
members for a label; and TYPE is a read-lens on one uniform graph, not a storage
branch — because structure and meaning are two readings of the same edges.

**All connections are pairwise (firmed 2026-09-15).** Every connection in the
store is a *pair*, full stop. Constituency, membership, child, grouping are not
distinct edge classes — they are different *readings* of the one pairwise
primitive through the type lens (endpoints' types + direction give the reading;
nothing is stored as a category). The store holds one thing — pairs — and higher
structure (groups, compositions, the LoD ladder) is emergent from accumulated
pairs. Consequences: edge ops carry no connection-kind operand; pairwise is not a
special class but the universal substrate — the lens defines the reading.

**Why the label MUST exist as a literal — the LoD rollup (firmed 2026-09-15).**
The computational/gaming reason grounding the naming-literal rule: when the
level of detail changes, the label BECOMES the actual particle. The label's
naming literal *is* the coarse-LoD particle — the same token_id, read as a label
(members/meaning) at fine LoD and as a literal (structure/mass) when it
participates in composition at coarse LoD. Consequences: (1) LoD rollup is O(1),
a representation swap not a computation — the coarse particle already exists,
its centroid mass already stored, so zoom-out is a single follow with no
re-aggregation and no search; (2) this IS the no-explosion funnel — a coarse
node lists a few sub-group tokens, each a label-as-particle for its whole set;
(3) stored centroid mass is precisely the coarse particle's weight, pre-computed
so it is live the instant LoD flips. And the deeper shape: the literal→label
inversion is a **ladder, not a single flip** — a label rolled up becomes a
literal, which is labelled again at the next-coarser scale; structure and
meaning alternate up the rungs (LoD *k*'s labels are LoD *k+1*'s literals). That
is why intake gates TYPE **per node** and mints several strata in one statement:
it is building multiple rungs of that ladder at once.

Each token carries a **type** that acts as a *read lens on the same edges* —
storage is uniform; the engine does not branch on leaf-vs-group. Every distinct
token_id is a self-contained grouping, so a leaf is a member/child exactly like
a sub-group. "Exact token_ids always call each other" — a reference resolves to
precisely itself (the address-is-identity rule).

**Literal** (e.g. a hex digit, a composed string):
- parents (`token_parent`) = the ordered tokens that constitute it.
- mass = the sum of those parents' masses. Base atoms with no parent take the
  one defined reading, 1 (the only explicitly-defined mass; everything above
  aggregates).
- children (`token_child`) = tokens that use it in *their* composition.
- siblings (`token_sibling_group`) = groups it joins as a whole mass.

**Label** (e.g. `single hex codes`):
- represents a grouping other tokens are contained within.
- parents (`token_parent`) = larger groups it is a sub-group of.
- children (`token_child`) = the tokens it contains — sub-groups and leaf
  members alike.
- stored mass = the **aggregate (centroid) of its member masses** (e.g. 16 hex
  digits x 1 = 16), NOT the label's own intrinsic mass (that comes from word
  associations, later).

Mass aggregates in the direction the type reads *down*: a literal sums over its
parents (constituents); a label sums over its children (members).

## Open concerns (this layer — not resolved)

- **A token's own mass — FIXED: stored on `token`.** Decided: the token's own
  mass is a stored value on `token` — one value per token, holding a literal's
  sum-of-parents and a label's centroid-of-members alike — to avoid re-summing
  on every touch. Maintained by the aggregation / pending-work workstream
  (eventually-consistent while it drains). It stays derivable (every
  `token_parent` row carries a mandatory mass; the sum grounds on the base atoms
  whose mass-1 is the explicit seed), so for composites the stored value is a
  maintained cache of that sum; for base atoms it *is* the seed. Schema: add a
  mass column to `token`.
- **Nested aggregation.** When a label's members include sub-group labels, does
  the centroid recurse to leaf masses or sum the sub-groups' stored centroids?
  Defines one-level vs transitive roll-up and avoids double-counting.
- **`token_child` two write sources by type — SUPERSEDED 2026-09-17.** Membership
  no longer rides `token_child`; it has its own `members`/`member_of` reciprocal
  (see *Relationship model & type — firmed 2026-09-17*). `token_child` reverts to
  pure structure-reverse (WIRE from `token_parent`). The old resolution
  (label containment as a `token_child` front-door write) no longer applies.
- **Pairwise / cross-field connections** — SETTLED, not an open concern: all
  connections are pairwise; the type lens defines the reading. See "All
  connections are pairwise" under Type semantics. (Do not resurface this as
  open.)

## API command vocabulary

> **⚠ SUPERSEDED as a *delivery* model (2026-09-22) — see the governing messaging-realignment
> banner at the top.** "Dispatches on a leading verb / formulaic calls issued at the kernel" is
> the old synchronous invocation model, now replaced by monitored-endpoint activation: these
> verbs become **reaction bodies** on boxes, invoked by data arriving, with results returned to
> a caller-supplied return endpoint — not commands dispatched at a runtime and polled. **The
> verb *vocabulary and semantics* below still stand** (what each verb means and validates is
> unchanged); only the framing of them as dispatched-and-polled commands is superseded.

The cache-manager runtime (`db_runtime`) dispatches on a leading verb — the
"formulaic calls" other routines (the analyst process, the swarm) issue.
Commands can nest (a set contains sub-declares; modes compose) and can drive
reconciliation patterns. Two tiers, record-level and cache-level:

> **Status note (2026-09-18):** the record-tier verbs (DECLARE_RECORD, READ_RECORD,
> and the four UPDATE sub-ops) are now **BUILT** — the italic "*Functional*" /
> "*FIRMED*" tags below predate the build; see *Current build state*. The
> cache-tier `*Stub*` tags remain correct (dispatch stubs).

- **DECLARE_RECORD** — official addressing: commit a proven-useful particle
  passed down from the analyst's virtual sim into the store. *Functional.* Two
  forms differing by specificity of addressing — a single particle, and a
  particle set (single is the degenerate one-member case, subsumed by the set
  but understood on its own).
- **UPDATE_RECORD** — change an existing token's relationships (params deferred).
  *FIRMED (2026-09-15) into four sub-ops — MOVE_RECORD, ADD_CONNECTION
  (arrayable), DELETE_RECORD, DELETE_CONNECTION (single, confirmed,
  peer-validated); see UPDATE ops.* The old umbrella RELATE (group membership,
  containment, cross-links) is now the ADD_CONNECTION / DELETE_CONNECTION writes;
  the scalar param-change (notation prose→token_id, formerly SET_FIELD) is the
  one deferred bookkeeping item, outside the firmed four.
- **READ_RECORD** — direct exploratory read: probe potential connections without
  a formal declaration. *Functional in the raw radial path; cache-shaping,
  exclusions and depth firm alongside the cache tier.* Cache-FREE by default (the
  explicit bypass — the cache is the normal data path), with an OPTIONAL
  cache-shaped mode that scopes the read by the working set.
- **RECONCILE** — settle deferred cross-processing to a consistent point. *Stub.*
- **UPDATE_CACHE** — realign the working set to current scope. *Stub (deferred).*
- **REBASE_CACHE** — shift the exploration root. *Stub (deferred).*

### DECLARE format

> **REVISED 2026-09-17** — `TYPE` is dropped (emergent, not gating). The fields
> are `ADDRESS`, `NOTATION`, `PARENTS` (structure), `MEMBERS` (grouping — replaces
> `CHILDREN`), `MEMBER_OF` (membership; no longer a *sibling-group* edge — the
> membership pair is `members`/`member_of`). See *Relationship model & type —
> firmed 2026-09-17*. The addressing / array / nesting mechanics below stand.

Named fields, order-free ("position" is only a pointer, not a fixed slot):
`ADDRESS` (a span expression — see the intake formula's alphabet: `@`/direct
pin, `FROM`, `AFTER`, `TO`, and the undeclared→cross-connection hook), `TYPE`
(non-optional; gates the rest), `NOTATION` (quoted; may contain spaces, e.g.
`"single hex codes"`), `PARENTS`, `MEMBER_OF` (the member-side sibling-group
edge — the firmed formula's name for what this list earlier called `SIBLINGS`),
`CHILDREN`.

- **TYPE gates validity** — parse everything, then validate against the type. A
  `literal` expects ordered constituents (parents; mass sums from them); a
  `label` expects members/containment (children; mass = centroid; parents =
  super-groups). A literal cannot carry label-containment fields and vice versa.
- **Mass is manager-owned, not declared.** DECLARE carries no mass: a literal's
  own mass = sum of its parents (derived); its constituent masses = each
  parent's own mass (also derived), so PARENTS holds references only, no
  per-position mass; a label's centroid = maintained by the manager. Label mass
  sits with the manager specifically because it is a maintained aggregate that
  async updates from *multiple analysts* would churn — it needs one consistent
  owner, not analyst-asserted values. The one exception is the **seed layer**:
  the 16 mass-1 hex atoms (and `0x` at mass 0) carry their mass as a *declared
  entry* — we put the value because everything needs a value to start, and there
  is nothing for the manager to compute at the floor (no parents to sum). That
  seed layer is the ONLY declared mass; every composite mass above is
  manager-built from these 16.
- **Single vs set = scalar vs array** (no separate span mechanism). The set/span
  form is just whether any field carries an array. Scalar fields are the shared
  frame and broadcast to every member; an array field is the per-member
  differentiator, its length the member count — e.g. `NOTATION:["0"…"F"]` with
  everything else scalar → 16 particles inheriting the frame, walked
  sequentially by the `from` address. Scalar-only = the degenerate single.
  Multiple arrays **co-index** (zipped, equal length, one member per index — not
  a cartesian product), the index lining up with the `from` sequential slot.
  Relationship fields may be arrays too (`CHILDREN:[…]` = a label's member
  roster / an inline set of member declares).
- **Recursion** — any relationship field (parents/siblings/children) may be a
  reference OR a full nested DECLARE of the same shape, so one call builds a
  whole connected sub-structure (e.g. hex `"1"` + inline `single hex codes`
  sibling + inline `hex value tables` super-group). Nested declares of an
  existing token are idempotent (SEE): ensure-plus-link — only the new edge is
  added, so the same label can be nested by all 16 hex particles safely.
- **Relationships stored BOTH directions** (denormalized) for follow speed (NOT
  search — the reverse is stored so the CPU FOLLOWS it) —
  children are spelled out/stored, same principle as stored summed masses:
  store the reverse so the CPU follows it, never searches. Declarable from
  whichever side is efficient; the reciprocal is maintained. For a label,
  **CHILDREN are its group members** — the reciprocal of each member's own
  membership edge: a literal member's SIBLING, or a sub-label member's PARENT.
  (e.g. hex `"1"` SIBLINGS `single hex codes` ⟺ `single hex codes` CHILDREN
  hex `"1"`; `single hex codes` PARENTS `hex value tables` ⟺ `hex value
  tables` CHILDREN `single hex codes`.)
- **No data explosion** — as aggregation deepens, direct listings per level
  *decrease*: finer cross-connections amalgamate members into sub-group
  token_ids, so a mature node lists a few sub-groups (each a single token_id
  standing for its whole set), not thousands of leaves. Self-compressing with
  depth — the funnel: sparse/direct at the rim, amalgamated toward the throat.

### DECLARE addressing modes

> Superseded framing (2026-09-15): these are no longer separate "modes" but
> composable segments of one ADDRESS span expression — see **The literal
> intake formula** below, which folds `from`/`after` in and adds `TO`. The
> per-mode notes here (esp. `@`-override ↔ reconciliation) are retained.

- **declare @** — pin to a defined address (single particle). A *request*, not a
  guarantee: the cache manager may override the placement. Today's
  provisional-accept path is this mode without the override.
- **declare from** — number sequentially from a given point (single or many).
  The group-at-trunk sequential fill. *Built (span planner `FROM` + successor);
  analyst-supplied-start fill, per-trunk-cursor pending the G4 next-slot decision.*
- **declare after** — begin at the next defined block after a given reference
  (trunk-aligned; honours the gap — finish in `D*`, next starts `E*`). **Firmed
  as `AFTER:b`**: takes an EXPLICIT block `b` as its reference (see the intake
  formula); it does NOT key off cache "current position". Leans on the block/trunk
  map. *Built structurally; concrete `AFTER` origin pending G5 (trunk map) —
  the span planner reports `kPendingSeam` for it.*
- **undeclared → address from cross-connections** — a particle with no address
  is assigned one derived from its cross-connections. This is the extrapolate
  hook (inert now); exact method TBD.

Consequences:
- **Placement authority ↔ reconciliation.** `@`-override and cross-connection
  derivation both mean the address is not final at declare time; that
  placement/relocation is what generates RECONCILE work.
- **DECLARE returns the ACTUAL assigned address**, not an echo of the requested
  one — caller proposes, manager disposes (`minted <address>` already returns
  what landed).
- **`@`-override couples DECLARE to cache state** — the current-position/override
  that REBASE_CACHE sets. (`AFTER:b` does NOT — it takes an explicit block, per
  the firmed formula, superseding the earlier "after leans on current position"
  note.)
- **Relocation is free because nothing holds addresses.** The analyst is its own
  set of C++ kernels with NO attachment to any data point — it navigates by
  connection, not by a held address. So the cache manager changing a record's
  address is immaterial as long as the connection is correct: **connection
  correctness is the invariant; the address is manager-assigned and fluid.**
  This is what makes `@`-override, `from`/`after` reflow, and back-correction
  safe — no stale handle exists to break.

### The literal intake formula (firmed 2026-09-15 15:12 MDT)

> **REVISED 2026-09-17 — read through *Relationship model & type — firmed
> 2026-09-17*.** `TYPE` is dropped: the `TYPE literal` line in the template below,
> and "TYPE gates them", no longer apply — validation is **structural** (which
> downward field is present: `PARENTS` ⇒ structure, `MEMBERS` ⇒ grouping). In the
> field descriptions, `MEMBER_OF` is the member-side of the **membership** pair
> (`members`/`member_of`), NOT a "sibling-group edge"; its reciprocal is the
> label's **`MEMBERS`** roster, NOT "label CHILDREN". The PARENTS / ADDRESS-span /
> NOTATION / array / nesting mechanics, the grounding rule, label-has-no-address,
> and the `>=2` / `>=1` floors all stand unchanged.

The literal `DECLARE_RECORD`, set form (single = the degenerate N=1). Fields are
order-free; TYPE gates them. Applies per node.

    DECLARE_RECORD
      TYPE      literal
      PARENTS   [ p0, p1, ... p{N-1} ]        # THE member array — sets N
      ADDRESS   <span covering the N slots, in member order>
      [NOTATION n0,n1,...,n{N-1}]             # optional
      [MEMBER_OF <shared> | s0,s1,...,s{N-1}]  # optional

- **PARENTS is the member array and sets N.** Outer length = the number of
  declared literals. *Consistent* — length-N exactly, no partial. Each member
  `pi` is itself a list of constituent references, `|pi| >= 2`, unbounded above.
  The >=2 floor is the anti-alias rule made structural: one constituent would
  resolve to that constituent's own address, not a new token. References ONLY —
  no per-position mass. Base-atom seed layer is the floor exception (literal, 0
  parents, declared mass).
- **ADDRESS covers the N slots.** A span expression; members in addressing order
  (index = slot). Alphabet: `FROM:x` (start at x), `AFTER:b` (start at the next
  block after defined block b — the trunk-shift, honours the sparse gap),
  `TO:y` (terminal bound; a *condition on the preceding origin*, never
  standalone), direct values, the `@`-pin (a *requested* address the manager may
  override, not a guarantee), and — for a slot left undeclared — the inert
  undeclared→cross-connection hook. Segments mix arbitrarily; valid iff, laid end
  to end, they cover N. An open (un-`TO`'d) origin is elastic and may be the
  *last* segment only (⇒ array may be partial); every interior segment is
  self-delimiting (direct = 1 slot, origin+`TO` = fixed count). A nested declare
  mints one item → one address → a direct value, so it is always a single
  self-delimiting slot, legal anywhere incl. interior. Internal complexity
  collapses to addresses — the span never sees the machinery. Purpose: gather
  disconnected address sets under groups.
- **Mass: none declared.** Absent from intake by construction. DB-derived (a
  literal's mass = sum of its parent-list's own stored masses, grounding on the
  mass-1 atoms) AND stored (storage cheap, calculation slow — store the tax;
  keeps the DB fully traversable so runtime reads never search). Only the seed
  floor writes a mass literal (`0x` = 0/undefined; 16 hex = 1). The earlier
  per-parent `:mass` and analyst `--mass` are dropped from the literal intake.
- **NOTATION: optional, positional-partial.** If present, comma-delimited over
  the N slots; blanks (successive commas) are legal placeholders. A blank or an
  absent field → surface derived by concatenating parent values down to
  word/character level, derived AND stored by the cache manager (NOT
  read-time). Derivation not in place yet — a blank reads blank until it lands.
- **MEMBER_OF: optional (tentative — may become required; decide by practice).**
  Two sections split by `|`. Section 1 broadcasts to all N — the common
  group(s) every member joins. Section 2 (optional) is a *sparse* positional
  array of *additional* per-member memberships; member i's total = shared ∪ its
  sparse slot. Member-side of the sibling-group edge; the reciprocal (label
  CHILDREN) is manager-maintained.

**Grouping by nesting.** A full set = a flat literal enumeration (stratum 1:
every used codepoint, once, each with its parents) plus nested label sub-declares
over it (stratum 2: tables gather codepoints, endpoints/families gather tables —
the whole Unicode-Tables lattice built in one statement). Membership authored
from whichever side is cheap (member-side MEMBER_OF or group-side nested
grouping — two directions onto one edge, reciprocal maintained). SEE dedups: a
codepoint/couplet shared across many tables is enumerated once and re-referenced
(ensure-plus-link, no explosion). The literal→label inversion happens per node,
mid-statement: literal floor, label lattice, the sub-declare the hinge each time.
[Example-domain note: the Unicode-Tables case (codepoints → tables →
endpoints/families) is the SAME mechanism as the hex running example (hex digits
→ single hex codes → hex value tables), generalized to a second, deeper domain —
"endpoints/families" and "Unicode-Tables lattice" are that domain's
sub-group/super-group tiers, not new primitives.]

**Label intake is the dual — next to firm.** Grounding rule (firmed
2026-09-15): a label CANNOT exist without a literal that represents what it is —
its name-as-a-literal (its *identity*, distinct from its members). Labels bottom
out on literals twice: once for membership (what they group, read down via
children) and once for identity (the naming literal). No free-floating label. In
the crude early stage the naming literal is stood in for by the temporary
`notation` prose string; it is replaced by a proper literal reference (the
composed-string token) as those terms are incorporated elsewhere — the
`notation prose → token_id` SET_FIELD swap under UPDATE sub-ops.

Bootstrap / construction posts (firmed 2026-09-15): to build anything you need
labels; a label needs members AND a naming literal; the naming literal is itself
a literal you must build (from char/word literals that may not exist yet) — a
circular dependency at bootstrap. Temp labels (crude prose notation) break the
cycle: they are deliberate **construction posts** — a placeholder identity you
build the real structure against, then pull once the genuine naming literal
exists. So the crude-storage stage is an intentional construction phase, NOT
incompleteness or debt; pulling the posts is precisely the deferred prose→
token_id swap. Scaffolding and its removal are both part of the design.

Addressing rule (firmed 2026-09-15): **a label is not given an address of its
own** — it exists *across* other token_ids (a distributed relation over its
members), so there is no single point to place. Its only address is its naming
literal's, borrowed by identity (the same token_id the LoD rollup rides). So the
naming literal is the label's sole handle into address space; a label with no
naming literal is unaddressable — unreferenceable, unable to roll up to a
particle. Therefore label intake carries **no independent ADDRESS placement**
like the literal set does: what gets placed is the naming literal (via the
literal path); the label is realized as membership edges keyed on the members,
referencing the group by the naming-literal token_id. The crude stage references
a placeholder (prose notation); the prose-to-token_id swap is when the label
acquires its real literal-derived address. Corollary: because a label is not
addressed by composing its members, the literal >=2 anti-alias floor does NOT
transfer. Minimum LOCKED at >=1 (non-empty, no higher floor): a single-member
label is valid, its centroid being that one member's mass (sole-item centroid =
the member's own mass). Rare in long-term practice but legitimate when building
analytical systems — a grouping stood up with one member that later accretes.

Label-vs-literal mirror (same machinery, opposite flow): required downward field
— literal PARENTS (constituents, each >=2) vs label CHILDREN (members,
non-empty); floor — literal bottoms at the 16 hex seed atoms (parentless) vs
label bottoms on literal leaves (no label-side exception); identity —
self-describing (derived notation) vs must carry a naming literal; mass — sum of
parents vs centroid of members; flow — bottom-up (enumerate leaves, nest groups)
vs top-down (declare group, nest members). Everything else (span arrays,
consistent-vs-sparse, nesting collapses to a direct slot, membership declarable
from whichever side is cheap) is identical.

### Arraying is universal — a compressed serial command stream (firmed 2026-09-15)

Any complete argument/command can be arrayed, because an array IS just a
compressed **serial** (ordered, in-sequence) sequence of single commands sharing
the scalar frame — NOT an unordered batch. Ordering is semantic: inter-dependent
ops resolve because they execute in order (a MOVE targeting a slot an earlier
MOVE placed works for the same reason intake fills in addressing order). The
decompression is exact via (1) frame broadcast — scalar operands replay
identically into each of the N; (2) SEE idempotency — a shared nested product
collapses to declared-once + N links, never double-minted. The array form also
enforces cross-checks the naive serial form would not (co-index equal length,
span covers N), but the executed result is identical. Consequence: every **record-tier**
command — DECLARE, READ, and the *additive* UPDATE sub-ops (MOVE_RECORD,
ADD_CONNECTION) — inherits its set/batch form for free; there is no separate
batch mechanism to design. The *destructive* UPDATE ops (DELETE_RECORD,
DELETE_CONNECTION) are the exception — single-instruction only, never arrayed
(see UPDATE ops). Arraying is also the
**precondition for label-start (top-down) entry**: without it you would be
forced to single-structure definitions begun at one point (bottom-up only); a
group's members ARE an array, so the label-end flow exists only because arraying
does. Scope: record tier
ONLY. The **cache tier** (UPDATE_CACHE, REBASE_CACHE) is excluded —
those are global state transitions on the working set as a whole, not
element-wise command streams, so there is nothing to compress into a serial
array (a REBASE is not N little rebases).

### UPDATE — record-tier ops (firming 2026-09-15)

Record-tier, so arrayable (see arraying above). Only a few options are
necessary. Firmed so far:

- **MOVE_RECORD.** A range from->to relocation reusing the intake placement
  variables (FROM/AFTER/TO/direct — the SAME span grammar as DECLARE ADDRESS).
  Strictly BOOKKEEPING: triggered when a more correct structural alignment has
  been derived, it re-places the record(s) at better-aligned addresses. Changes
  NO relationships, only pointers — the graph topology is invariant (same
  parents, children, memberships); MOVE rewrites the address-labels on the moved
  nodes and cascade-repoints every edge that referenced the old token_id to the
  new one. No edge added or removed, nothing wired or unwired. Safe because
  nothing holds addresses — connection is the invariant, the address is
  manager-assigned and fluid, so no stale handle breaks. This relocation IS the
  reconciliation generator (the cascade repoint may drain via the
  pending/RECONCILE path).
- **ADD_CONNECTION `<label> <element...>`.** Assert membership: add one or more
  elements to a label. MUST begin with a label — that is the anchor, because the
  op is "add >=1 elements as members of this label." Label = scalar frame
  (broadcast); elements = the arrayable side (>=1); each addition is a
  pair(label, element). Declares/mints no terms (that is DECLARE's job); all
  endpoints must pre-exist. No connection-kind operand — all connections are
  pairwise, the reading is lens-derived. Reciprocal maintained (both-direction
  store). This is the GROUP-SIDE membership-authoring op — the dual of MEMBER_OF
  (member-side, at declare); same edge, authored from whichever side is cheaper,
  ADD_CONNECTION being the post-mint from-the-label side.
- **DELETE_RECORD / DELETE_CONNECTION — the destructive pair.** DO NOT ARRAY at
  all: single instruction only, no batch form (the opposite of the compressed
  serial stream — non-arrayability IS part of the friction). Two deliberate
  gates: (1) **active confirmation before execution** — local, pre-execution,
  the caller must deliberately confirm, not fire-and-forget; (2) **[SUPERSEDED
  2026-09-18 — see DELETE gates ruling below]** originally "validation tags for
  other instances," but **no local validation-tag machinery is built**: the delete
  executes locally now, its report is **booked by the WAL manager** (built —
  `kernels/wal/`), and the peer/cross-network tentative→systemic validation is
  **deferred swarm-side**.
  Rationale: a record is assumed well-vetted and
  reasonably supported before it reaches the DB, so destruction contradicts the
  base assumption and must be very deliberate — vetting lives on entry, deletion
  is the rare gated exception. Scope: DELETE_RECORD removes a whole token;
  DELETE_CONNECTION removes one specific pair (matches the specific edge).
  **Specific IDs and connections ONLY — no wildcards.** You delete exactly what
  you name; there is no pattern/wildcard match (consistent with only-follow /
  never-search — there is no scan to select targets). This
  settles the old RETIRE_RECORD / "do records only ever move?" question: records
  CAN be removed, but only as a maximally-gated exception, never a normal path.
  No MERGE (forced equivalence forbidden) — unchanged.

That is the full UPDATE set: MOVE_RECORD, ADD_CONNECTION (both arrayable),
DELETE_RECORD, DELETE_CONNECTION (both single, confirmed, peer-validated).

### READ — record-tier exploratory read (firming 2026-09-15)

The direct exploratory probe — **cache-FREE by default** (the explicit cache
bypass; the cache is the normal data path), with an OPTIONAL cache-shaped mode
(see below). Only-follow from the anchor; never searches. **Purpose:** the analyst's construction instrument —
reading one rich node's structure informs the majority of the variables for a
new prospective token; it is the front of the declare loop (READ → prove in sim
→ DECLARE), not a separate utility. Parameters:

- **Anchor** — a single token_ID to begin from.
- **Direction — OPTIONAL; default RADIAL.** A token is both label and literal,
  so it is a dense pairwise hub (dozens of lines: constituency, membership,
  reverse-adjacency, super-groups). Unset, the read expands radially — all
  directions outward. Direction is a *narrowing* applied when one axis is
  wanted: data path parents / members / both (the per-axis downward reads —
  parents = structure-down, members = membership-down), reverse orientation
  likewise selectable.
- **Depth (by factor)** — the LoD dial: depth 0 = the rolled-up node (coarse
  particle), each rung expands one level down the ladder.
- **Exclusions** — prune named branches (specific token_ids ONLY, NOT property
  filters — a predicate would force a search). Exact mechanism TBD (later).
- **Cache data shaped** — shape/scope the read by the current cache's data
  shape. The scalable relevance tool: vast swaths of a hub's pairwise fan are
  irrelevant at any moment and cannot be hand-pruned, so cache-shaping prunes
  wholesale to the currently-relevant subset. Complements exclusions (automatic
  relevance-by-working-set vs surgical named prune). Gives READ two poles: raw
  radial (pure cache-free probe) vs cache-shaped (relevance-bounded). The
  cache-shaped form couples READ to cache state, so its exact shape firms
  alongside the cache tier (as after/override coupled DECLARE to it).
- **Re-convergence — RULED (2026-09-15): once per path, NO dedup.** The READ
  return is an arrayed (linearized) data return; a shared node recurring at
  different points is NOT redundancy — the fact that it repeats, and where, IS
  part of the data structure (it carries the convergence/sharing losslessly).
  Because the read is a pure follow, linearization is deterministic — identical
  values fall at predictable positions, so repeats are expected and legible, not
  anomalies. Dedup would destroy the structural information the read exists to
  surface. Same shape as arrayed intake: an ordered array where position AND
  repetition encode the tree; consistent with only-follow (every referenced path
  is followed) and address-is-identity (same token_id at many positions = one
  identity at many structural points).

### Build-phase rulings — firmed 2026-09-17 (record-tier; wildcards, MOVE, mass)

Rulings made with Patrick during the record-tier build. These GOVERN and are
recorded here so code traces to spec, not to chat.

**Wildcards are TERMINAL ONLY — no inline wildcards.** A wildcard is a partial
*trailing* address element (the codec's partial element, e.g. `A*`), naming a
contiguous trunk/subtree region. **The terminal restriction is what makes a
wildcard a GATHER, not a SEARCH:** fixing the entire prefix and freeing only the
tail yields a single contiguous, address-ordered region (a trunk/subtree) that is
*walked* (only-follow). An inline (non-terminal) wildcard would free an interior
position while pinning later ones — a non-contiguous pattern match, i.e. a
search — which is forbidden. A wildcard may therefore only be the last element.
(Consistent with the codec: a partial element is valid only as an address's last
element.)

**Grouping-node naming literal (label DECLARE).** A label DECLARE (MEMBERS
present) establishes its naming literal ONE of two ways, and ADDRESS is PERMITTED
on a grouping node — it names the naming literal's address (this REFINES "a label
has no own address": the label borrows the naming literal's address, it has none
independently):
- **Use-provided-ID:** an existing address is given (PARENTS absent) → reference
  that pre-existing naming literal.
- **Mint:** no address, or only a target address → mint a NEW naming literal from
  PARENTS (its *required* constituents only — the ≥2 floor, lean; no "must be
  complete" over-requirement; constituents may be existing-address references,
  needing at most one nested declare to begin); placed at the target address if
  given, else manager-placed.
- A grouping node with NEITHER PARENTS nor ADDRESS is invalid → reject.

The mixed PARENTS+MEMBERS node IS the mint case — one token that is both the
literal (its composition) and the label (its members): the
naming-literal-IS-the-particle duality. NOTATION is NOT the handle — it is
optional and human-facing only (a reviewer aid; token_ids are hard to eyeball),
never a functional identity/reference. Edge cases, reject-until-ruled: N>1 for a
mixed node (a naming-literal mint is a single literal, N=1); an ADDRESS-span
nested-declare colliding with a slot's own PARENTS composition (would silently
drop that slot's constituents).

**Address column collation (firmed 2026-09-18) — `COLLATE "C"` on the `text[]`
address columns.** `token_id` and the FK address columns (`parent_token_id`,
`child_token_id`, `member_token_id`, `group_token_id`) are pinned `COLLATE "C"` so
element comparison is **byte order = `codec::kAlphabet`'s order** (A–N,P–Z,a–n,
p–z). Without the pin the columns inherit the DB default collation
(case-interleaved, `aAbB…`), so the PK btree order does NOT match address order —
a contiguous trunk is NOT a contiguous PK range, breaking gather / ranges /
sequential fill (a bare range scan silently drops members; a query-side
`COLLATE "C"` degrades to a full index walk, the forbidden search cost). This is a
**pure collation pin, not an alphabet change** (byte order already equals the
codec's documented order) and it **preserves the arrayed `text[]` storage** —
chosen for address compression and to avoid re-parsing a dot-joined string on
every action — changing only sort order. Equality / point reads are unaffected
(collation-independent); ordered ranges now ride a genuine bounded PK `Index
Cond`. Prerequisite for the gather primitive.

**Gather primitive (firmed 2026-09-18) — the terminal-wildcard / range
enumerator.** Resolving a terminal-wildcard prefix or a `FROM..TO` range to the
*existing* tokens it covers (MOVE's bulk source, ADD_CONNECTION's wildcard
expansion, READ's wildcard anchor) is a **GATHER**, realized as a **PK range scan
on `token_id`**: the fixed prefix (or the range bounds) delimits a *contiguous*
interval in the PK's btree order, which is walked on the PK index. This is
only-follow — walking a contiguous key range on the existing PK — NOT a
reverse-search: no new/secondary index, no sequence scan, no predicate on a
non-key column. It is independent of G4/G5 (it *enumerates* existing tokens; it
does not *place* new ones). It is a new door **read** primitive; the execution-time
wildcard resolution in the UPDATE/READ cores runs over it.

**MOVE — "from→to" is the relocation RELATIONSHIP, not a command form.** The
"range from→to relocation" wording conveys the source→destination relationship;
**source-selection and destination-placement are DISTINCT operations** that read
naturally as from→to:
- **Source (a selection op):** explicit token(s), a `FROM..TO` range, or a
  **terminal-wildcard prefix** selecting a whole trunk/branch (MOVE's primary use:
  bulk correction of a misclassified branch). Only-follow — a prefix/range names a
  contiguous addressed region, *walked*, not predicate-searched. A wildcard/range
  source's unit count N is store-resolved (execution-time).
- **Destination (a placement op):** an ADDRESS span; cover-N with N = the source
  unit count. All-explicit source ⇒ N known ⇒ cover-N at IR; any range/prefix
  source ⇒ N store-dependent ⇒ cover-N deferred to the UPDATE core.

**READ — terminal wildcards permitted (nominal, tree-constrained).** The anchor
and exclusions MAY be terminal wildcards: an analyst reading a range of tokens (a
full construct) selects a contiguous tree region. This is a **nominal** search —
constrained to walk connected factors, results computed *under a tree* — and is
permitted, distinct from the forbidden arbitrary property-predicate (an unbounded
scan). This refines the earlier "exclusions = specific token_ids only": specific
token_ids OR a terminal-wildcard tree region; never a property predicate.

**ADD_CONNECTION — terminal wildcards, either side.** The group and/or the
elements may be a terminal wildcard/range, applying in either direction: "add all
[wildcard-selected members] to this group," or "add this member to all
[wildcard-selected groups]." Example: the `01` hex couplet and its translation
across tables — the tables occupy a definable address range, so one statement with
the right terminal wildcard sums them. Endpoints still must pre-exist; the wildcard
resolves to the definable address range at execution (UPDATE core), which
enumerates the pairs. **Both sides wildcard (firmed 2026-09-18): full
cross-product.** When BOTH the group side and the element side are wildcards, the
result is the full cross-product — every gathered member joins every gathered group
(M groups × N members = M×N edges). A legitimate bulk operation (may be helpful in
some cases); endpoints still pre-exist; idempotent.

**DELETE (RECORD / CONNECTION) — explicit only, no wildcards.** Unchanged stated
hard rule: specific ids/pairs only (the destructive gate). DELETE_CONNECTION
targets the **membership axis only**; structure removal is via DELETE_RECORD
(whole-record deletion) — a constituent is not surgically removed from a
composition. [d2]

**DELETE gates (firmed 2026-09-18): local FULL validation + local execution now;
peer/cross-network validation deferred to WAL management.** The local active
confirmation is a **full, specific-target validation** — the op echoes the exact
thing to be deleted ("did you mean to delete this specific thing?") and requires
explicit confirmation of THAT target before it executes; never a
bare/fire-and-forget delete, never a blanket confirm flag. It then executes
LOCALLY against the store. The peer / cross-network validation path
(tentative→systemic across instances) is NOT built at the record tier — the
delete's report is **booked by the WAL manager** (a bookkeeper/observer over WAL
reports, now BUILT — `kernels/wal/`; see `kernels/wal/WAL-PLAN.md`, `kernels/wal/README.md`), while the
cross-network validation **act** is **deferred swarm-side**, not run by the WAL
bookkeeper. (This resolves the old G7
delete-peer-validation seam; WAL-manager-is-bookkeeper reframed 2026-09-18.)
No local validation-tag machinery is built now.

**Mass — why none is declared.** An analyst-asserted mass would have to be
validated against the asserted parents anyway (i.e. recomputed), so passing it is
redundant work on both sides of the transaction. The analyst computes mass in its
model but transmits nothing; the store derives-and-stores it (blank at declare,
filled by the pending-work workstream). The ONLY declared masses are the seed
floor (16 hex atoms = 1, `0x` = 0/undefined), inserted manually via `mint`'s
optional mass parameter — a bootstrap channel, NOT the DECLARE verb.

**Request transport (firmed 2026-09-18) -- external wire deferred; nested/arrayed
is the intent.** The request was NEVER one-request-per-line -- the nested and
arrayed nature was intended from the start. The external wire/framing format
stays deferred (G6). For the record-tier build the verbs are dispatched over the
in-process IR + arraying executor with a **minimal testable surface**; a real
external request encoding is a later, focused pass. Bar for this stage: the verbs
are testable through the dispatch layer. The old `db_runtime` one-request-per-line
flat form is superseded (it was never the intent).

### Cache tier — loosely defined, deferred (2026-09-15)

> **Elaborated by "Cache manager — view composer (design forming, 2026-09-23)" below**
> — the two view ops are now framed on the n-dimensional / view-composer model. The
> continuity-of-basis axis below still holds; read it through that section.

Touched only enough to define and defer; mechanisms not built. Axis between the
two view ops is continuity of basis:

- **UPDATE_CACHE** — same basis, incremental: rebalance the working set around
  the current exploration locus and progressively extend the telescoping view's
  reach in a direction. Aim-and-extend in place; continuous.
- **REBASE_CACHE** — new basis, wholesale: change the fundamental basis of the
  analysis and fully rebuild the cache space around the new exploratory
  location. Discontinuous — re-found, not re-aim. Sets the "current position"
  that DECLARE after/override lean on.
- **RECONCILE** — **SUPERSEDED as a cache-tier verb (2026-09-22): RECONCILE is
  not a db/cache-manager verb at all** — it was removed from the dispatch surface;
  the analyst messages the **WAL manager directly**, which promotes the relevant
  pending work into a pinned priority box the cache manager drains. See the
  messaging-realignment banner at the top of this file and
  `network/ENDPOINT-ACTIVATION-NOTES.md` "RECONCILE". Only `UPDATE_CACHE`/`REBASE_CACHE`
  remain named-but-unbuilt cache-tier stubs. *(The original characterization,
  retained for provenance:)* orthogonal to both, and demand-driven: an analyst
  request for IMMEDIATE, prioritized cross-matching — forcing the deferred
  cross-processing (WIRE / cross-links / feedback) to the foreground, generally
  because the result will impact current analysis and cannot wait for the pending
  work to drain on its own. Relevance-triggered, not housekeeping.

Both view ops (`UPDATE_CACHE` / `REBASE_CACHE`): global state transitions on the
working set, non-arrayable. Deferred. (RECONCILE is no longer among them — see its
bullet above.)

### Cache manager — view composer (design forming, 2026-09-23)

> **DESIGN FORMING, not pinned, not built.** Captured from the cache-operations
> discussion so it is not lost; will firm before any build. Game mechanics are the
> operative frame; the Taichi SNode-tree resemblance is **structural, not functional**.

The store is **n-dimensional** — all connections across all axes; it is not itself a
tree. A tree / composition appears only when an analysis **projects onto chosen relevant
dimensions**. The cache manager is the **view composer** that does that projection.

- **View spec = system configuration + analyst request, combined.** Together they define
  *which axes of study* (the relevant-dimension projection) and *what levels of rollup*
  (LoD depth, **per area, not one global level**) are appropriate. Neither input alone
  sets the view.
- **Structural, not functional, SNode resemblance.** A token is an SNode-composable
  element (defined-by-parts = literal; nesting = LoD). But a Taichi SNode tree is a
  *fixed* field layout for compute, whereas the cache is **re-composed per study** — a
  projection, not a cutout of cold storage.
- **Rollup is dimension-parametric** — along the studied axis. Rolling up *word
  structures* is relevant only when words are the object of study; along another axis the
  rollup composes entirely different coarse particles. (Corrects the earlier word/hex
  examples being read as the general case.)
- **The walk (all modes):** only-follow (address IS identity; PK / PK-prefix follows — no
  predicate scan), **LoD-bounded per area** — take the coarse **label-as-particle** (its
  centroid already stored, the O(1) swap; the truncation / LoD dial) wherever the view's
  rollup level says stop; descend only where detail is wanted. Output = the **compound
  structures** for that view (the warm working cache content).
- **Three update behaviours:**
  1. **Passive background update** — the manager keeps the current view current as it
     processes other work; not analyst-driven. This is the **tier-4 standing maintenance**
     ("background cache updates") of the box-priority structure. **This stream drives the
     local deviation monitor** (minor, later): local deviation = how much the manager has
     passively updated that is relevant to the current projection + how much pending work
     touches the area of study. Sharpens the activation-notes "Analyst deviation gauge"
     (local-deviation) — it is grounded in this passive-update stream, relevance-scoped to
     the projection.
  2. **`UPDATE_CACHE` — active refocus, edge-driven** — same basis (same axes/rollup
     projection); as the analyst's focus moves **toward the edge of the study area**,
     aim-and-extend the view's reach in that direction. The telescoping reach, triggered by
     edge-approach.
  3. **`REBASE_CACHE` — new basis, wholesale** — a new (config+request) view spec →
     re-walk, re-compose from scratch.
- **Provisional analyst addresses** — an address the analyst proposes is a position *within
  this composed view*, not an absolute store coordinate; the manager disposes the actual
  cold-store placement (caller-proposes / manager-disposes).

**Open (to pin):** the **combination rule** — how system configuration and the analyst
request merge into one view spec (the part that reaches back into the controller/analyst
layer); the **edge-approach trigger** for `UPDATE_CACHE` — analyst-requested vs
manager-detected/anticipatory.

**Parked — storage granularity (for the cache-structure build-out, 2026-09-23):** a great
deal of the store will **never be used at the granularity it contains** — coarse-by-default
is the norm, fine descent rare and local (why the walk is LoD-bounded). Two-part granularity
strategy to design when the cache structure is built:
- **Explicit base:** the **UTF table + character sets get an explicit mapping** (the base
  atoms / seed floor / codec alphabet ground — must be exact; everything composes up from it).
- **Batch-rolled middle:** many of the **detailed intermediate composition steps** between
  the explicit base and the meaningful coarse tokens can be **rolled up in batches** rather
  than each fully materialized — coarse-by-default storage, fine on demand (the local echo of
  the swarm's "distillation + pull-coverage-on-request"). *(Reading to confirm later: "batch
  rollup" as a storage/build representation of the detailed middle, not a per-request compose
  economy.)*

## In flight — next to lock — BUILT 2026-09-18

> **EXECUTED 2026-09-18.** DECLARE format and the four UPDATE sub-ops below are
> BUILT (see *Current build state* — `command/` + `declare/`, `update/`). The text
> is retained as historical design record; the array-validation standards stand.
>
> **SUPERSEDED 2026-09-17** for the TYPE-gating bullet — `TYPE` is dropped;
> validation is **structural** (which downward field is present: `PARENTS` ⇒
> structure, `MEMBERS` ⇒ grouping), not type-gated. See *Relationship model &
> type — firmed 2026-09-17*. The array-validation standards below stand.

- **DECLARE format firming.** TYPE gates the REQUIRED downward field: literal ⟹
  PARENTS (constituents), label ⟹ CHILDREN (members). The other/reciprocal side
  is manager-computed-**and-stored** (tax paid once), never declared. Base atom
  = floor exception (literal, no parents, declared seed mass). Plus array
  validation standards for clean rejection: co-index equal-length arrays
  (= member count, aligned to the `from` slot), required-by-type present,
  forbidden-by-type absent, address mode well-formed, nested declares recurse.
  TYPE-gating and validation apply PER NODE — a typical nested statement mixes
  literal and label nodes, so both modes are active in most declarations; the
  checks recurse node by node, not once per statement.
- **UPDATE sub-ops — FIRMED (2026-09-15), see "UPDATE — record-tier ops"
  above.** The necessary set is four: MOVE_RECORD and ADD_CONNECTION (arrayable),
  DELETE_RECORD and DELETE_CONNECTION (single, confirmed, peer-validated,
  specific-id-only, no wildcards). RETIRE_RECORD is resolved into DELETE_RECORD;
  No MERGE (forbidden). DEFERRED (bookkeeping, handle later, per
  Patrick): the notation prose→token_id swap for temporary labels when the
  naming literal mints. Not a design gap — a bookkeeping item parked on purpose.

## Current build state

**Record tier COMPLETE** (built 2026-09-17/18; each stage built by a Sonnet agent,
adversary-reviewed to a clean PASS, and lead-confirmed; on branch
`dbkernel-design-checkpoint`). All modules test green against a live disposable
`hcp3_core`.

- `codec/` — address ↔ token_id transforms. Done.
- `schema/` — **5 tables**: `token` (no `type`; `mass` nullable), `token_parent`,
  `token_child`, `members`, `member_of`. The `text[]` address columns are pinned
  `COLLATE "C"` so PK order = codec byte order (required for gather/ranges).
  PK-only indexes; no reverse-search index. Done.
- `controller/` — the single door: only-follow reads (`token_exists`,
  `parents_of`, `children_of`, `members_of`, `member_of`, `attributes_of`); writes
  `mint` (no type; optional mass → SQL NULL), `add_membership` (idempotent, both
  directions), `rekey` (FK-safe cascade + `token_text` re-derive), `delete_token`,
  `delete_pair`; and `gather` (contiguous PK-range enumerator for
  terminal-wildcard / range). Done.
- `command/` — command IR + structural (no-TYPE) validation + ADDRESS span planner
  (with a base-50 successor). Done.
- `declare/` — DECLARE core: literal structure (mint ordered constituents) +
  grouping (naming literal by use-provided-ID or mint-from-PARENTS), nesting, SEE
  idempotency; mass blank (pending work); manager-placed mint rejected pending G4.
  Done.
- `read/` — READ core: only-follow raw-radial; per-axis (structure vs membership)
  + reverse; depth/LoD; named + terminal-wildcard-region exclusions; terminal-
  wildcard anchor via gather; linearized once-per-path, no dedup. Done.
- `update/` — UPDATE core: MOVE (explicit + gather-resolved wildcard/range,
  cover-N, rekey cascade, mint-bearing destination rejected), ADD_CONNECTION
  (idempotent; wildcard either side + M×N cross-product), DELETE_RECORD /
  DELETE_CONNECTION (full specific-target confirmation + local execution;
  only-follow reject-on-referenced G10 probe). Done.
- `dispatch/` — verb dispatch over the in-process IR (routes DECLARE/READ/UPDATE
  to the cores; UPDATE_CACHE/REBASE_CACHE stub — RECONCILE removed 2026-09-22,
  now an analyst → WAL-manager message) + arraying executor
  (cross-command ordered stream over the four additive verbs; DELETE structurally
  excluded). Minimal testable surface; external wire deferred (G6). Done.
- `seed/` — `seed_0x` mints the seed floor (`0x` mass 0 + 16 hex atoms mass 1) via
  `mint`'s optional-mass bootstrap channel. Done.
- `ingestion/` — **RETIRED.** The pre-rebase path-A `Ingestor` + one-request-per-
  line `db_runtime` are superseded by `command/`+`declare/` (intake), `dispatch/`
  (routing), `seed/` (floor); `ingestion/README.md` is a supersession breadcrumb.

**WAL manager COMPLETE** (built 2026-09-19; `kernels/wal/` kernel set, tasks W-1…W-6,
package-vetted primary↔adversary, committed through `a899970`; its own standalone
Postgres DB `wal_manager`, always separate from `hcp3_core`). A pure
bookkeeper/observer over WAL reports — it never writes `hcp3_core`, never reads
the command string, and never drives the cache manager. What it maintains is the
**active deferred-work topology**: the live open-obligation relation (open =
membership, PK-delete on close, no status column) plus an append-only History —
the topology the cache manager navigates to find what followup work (reciprocal
returns, mass) is still owed. Door surface: `open` / `close` / `is_open` /
`list_open` / `record_seen`, every access a bounded PK follow (only-follow, no
reverse index). See `kernels/wal/README.md` (charter, file map, build/run) and
`kernels/wal/USAGE.md` (how a consumer — chiefly the cache manager — navigates it and the
door/scope contract). **RECONCILE** (an analyst-raised "this deferred cross-work
is priority now" flag) **routes through the WAL manager, not the cache manager
directly** (revised 2026-09-22 — supersedes the earlier framing here that had the
cache manager navigate the topology and the WAL manager do nothing): the analyst
messages the **WAL manager directly**, which navigates its own open-obligation
topology (only-follow) to **move the relevant pending work into a pinned priority
box**; the cache manager only *drains* that box. This is a conscious revision of the
built bookkeeper boundary — the WAL manager now *selects and places* reconcile work
(it does not do the work). Its open→close obligation ledger is otherwise unchanged,
still driven by the reciprocal write landing. See `network/ENDPOINT-ACTIVATION-NOTES.md`
"RECONCILE" and the messaging-realignment banner at the top of this file.
**Ingest atomicity — resolved as out-of-scope here (Patrick, 2026-09-19):**
package review found `close()` + `record_seen()` are two separate writes, not
one transaction, so a crash between them makes a resume-from-History-`max(lsn)`
redeliver the report, land `is_open() == false`, and silently write
`settled = NULL` for a report that did settle something — a real gap, not
healed by Postgres's own durability (which guarantees the row written, not
its value). Patrick's ruling: **dropped from the WAL manager specifically** —
no transaction surface is added to `WalBook`, `ingest()` stays as-is — because
atomicity across the whole observe→book cycle is the job of whichever runtime
orchestrates that cycle, not this bookkeeping door. It becomes a
**cache-manager-runtime** open item, to revisit alongside F4 when that runtime
is built (see `kernels/wal/WAL-PLAN.md`'s status blockquote for the full trace).
Still deferred within/around this layer (bucket B/C, `kernels/wal/WAL-IMPL-PLAN.md` §1): the
async reciprocal split, live logical-decoding ingest, and the mass-fill write
(the cache manager's own runtime, not built); MOVE/rekey of open obligations; and
NEEDS-PATRICK — the connection mass-recompute signal, F4, the cross-source
global-order basis, and the swarm side entirely.

**Deferred (not built; recorded seams):** G4 next-slot mechanism (also gates
manager-placed mint), G5 block boundaries past hex couplets, G6 external
transport/wire format; the **cache-manager runtime** itself (the file-now /
wire-later split, §Process runtime — the WAL manager above only books/monitors
the obligations that split would create, never drains it) and the swarm-side
cross-network DELETE validation act; the **cache tier** (UPDATE_CACHE /
REBASE_CACHE — stubs; RECONCILE removed 2026-09-22, now analyst →
WAL-manager); the mass aggregation model; notation
derivation; the prose→token_id swap; extrapolation / relative-placement rules.

## Open decisions (Patrick's)

- **Next-slot mechanism** (only-follow-safe): analyst supplies the trunk's
  starting address (deterministic sequential fill) **or** each trunk carries a
  next-free cursor the controller follows and advances.
- **Block boundaries past "hex couplets"** — whether the next encoded set is 3
  or 4 couplets (trunk map extends when decided).

*(Resolved 2026-09-18, removed from this list: "Input shape for ingestion" — the
record-tier surface is the in-process IR (`dispatch/`), external wire deferred to
G6, and the old `dbk::DataPoint`/path-A is retired; "Re-validate + commit the set"
— done, re-reviewed per stage and committed through `aefc9e6`.)*

## Handoff — staged implementation (EXECUTED 2026-09-18)

> **EXECUTED 2026-09-18.** This mission is DONE: the staged plan is `PLAN.md`, the
> Sonnet agents built each stage under adversarial review, and the record tier is
> COMPLETE and committed (see *Current build state*). The text below is retained
> as the historical handoff record, not a pending instruction.

This document is the FULL reference; read it end to end. Mission of the next
context: turn the firmed design into a **staged implementation plan for Sonnet
agents, each stage under adversarial review** (the same completeness-gated
adversarial discipline just applied to these notes — complete review required,
no summarization accepted, completeness interrogated, partial/lumped returns
refused).

**Firmed and ready to build (the record tier):**

> **REBASED 2026-09-17** — the type/membership model changed: `TYPE` dropped
> (emergent, LoD-relative); membership is `members`/`member_of`, structure is
> `parent`/`child`; schema restructures (drop `token_sibling_group` + `token.type`,
> add `members`/`member_of`). Read this list through *Relationship model & type —
> firmed 2026-09-17*: "label dual" ⇒ the grouping read; "MEMBER_OF" is the
> membership pair; "label CHILDREN" ⇒ `MEMBERS`.

- Address/codec + schema + controller door existed at handoff. **(Now built out:
  5-table schema (+`COLLATE "C"`), the full door incl. `gather`, and the
  DECLARE/READ/UPDATE cores + `dispatch/`; path-A ingestion is RETIRED and the old
  `db_runtime` flat form superseded by `dispatch/` — see Current build state.)**
- DECLARE — the literal intake formula (member array = PARENTS sets N; ADDRESS
  span alphabet; NOTATION positional-partial; MEMBER_OF; grouping-by-nesting)
  and its label dual (mirror; grounding rule; label has no own address).
- READ — anchor, optional-direction/radial-default, depth (LoD), exclusions
  (named-branch only), cache-shaped mode; linearized once-per-path return.
- UPDATE — the four ops: MOVE_RECORD, ADD_CONNECTION (arrayable), DELETE_RECORD,
  DELETE_CONNECTION (single, confirmed, peer-validated, specific-id, no wildcard).
- Arraying is universal across the record tier (serial, ordered).

**Deferred — do NOT block the staged build, do NOT invent resolutions.** Patrick
indicates these are covered in prior contexts / the graph; validate there rather
than guess. Flagged by the 2026-09-15 adversarial review of these notes:
- **Mass model** — sum vs centroid for label mass (the notes use "centroid" for
  what they example as a sum); which mass is stored when one token_id is both a
  label and its naming literal (two values, one slot); nested aggregation
  (recurse-to-leaf vs sum-stored, double-counting); `0x` = 0 vs undefined. The
  record tier does not depend on these: the analyst declares NO mass; the stored
  mass is a maintained aggregate column filled by the deferred workstream, so the
  exact aggregation rule is a later stage.
- **Identity vs relocation (#1/#2)** — one authored statement of what persists
  across a MOVE and what "address IS identity" precisely means. MOVE is
  implementable as a pointer/re-key regardless of the prose wording; the wording
  is Patrick's to settle.
- **Group/boundary senses (#23)** — emergent kind-partition (sparsity) vs stored
  membership (`members`/`member_of`) vs the trunk-map boundary; distinguish in prose, not a
  real contradiction. Confirm with Patrick before rewording.

**Cache tier** (UPDATE_CACHE, REBASE_CACHE) — defined, deferred; a
late stage, after the record tier is solid. (**RECONCILE removed from the
dispatch surface 2026-09-22** — it is no longer a db/cache-manager verb but
an analyst → WAL-manager message; the WAL manager promotes the relevant
pending queue into a priority in-box. See `network/ENDPOINT-ACTIVATION-NOTES.md`
"RECONCILE".)
