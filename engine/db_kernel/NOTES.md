# db_kernel — data protocol & address-layout notes

Working notes for the hcp3_core db_kernel set. Prose/design record; the code
and its tests are the source of truth for behaviour. Canadian English.

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

The cache manager (this whole db_kernel) is a **complete, standalone runtime
process**, not library pieces called ad hoc: it owns the DB (only the process
controls the DB) and its primary duty is serving input/output requests. It must
be complete on its own and may be **detachable to ride with the swarm**;
whether it is later bundled with other processes is a separate concern.

> **Build status (2026-09-18):** the record tier writes **synchronously** — every
> reciprocal is written inside the authoring op (`mint`'s WIRE, `add_membership`
> both directions). The async **file-now / wire-later** split described below is
> DEFERRED to WAL management (see *Current build state* → Deferred) — design, not
> built.

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
- **⚠ Revisit (2026-09-18):** the WAL manager's **self-accounting** completion model
  — followup obligations booked from a change's own data and closed by *observing*
  the followup writes, with **no pending-list drain** (see `WAL-PLAN.md`) — bears
  directly on this pending-list / wire-later runtime. Whether self-accounting
  **replaces** the pending-list-that-drains here or only governs the WAL manager's
  view is unresolved (F4) and **must be revisited when the cache-manager runtime is
  designed/built**.
- **Multi-analyst.** The cache manager may serve more than one analyst and needs
  a per-analyst input/response link. Maintained aggregates (own masses, label
  centroids, reciprocal listings) therefore have a single owner — the manager —
  so they stay consistent across async multi-analyst updates.

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
ONLY. The **cache tier** (RECONCILE, UPDATE_CACHE, REBASE_CACHE) is excluded —
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
  the caller must deliberately confirm, not fire-and-forget; (2) **validation
  tags for other instances** — the delete carries tags so peer instances confirm
  before it is made SYSTEMIC (executes tentatively; becomes system-wide only
  once peers validate). Rationale: a record is assumed well-vetted and
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
reports; see `WAL-PLAN.md`), while the cross-network validation **act** is
**deferred swarm-side**, not run by the WAL bookkeeper. (This resolves the old G7
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

Touched only enough to define and defer; mechanisms not built. Axis between the
two view ops is continuity of basis:

- **UPDATE_CACHE** — same basis, incremental: rebalance the working set around
  the current exploration locus and progressively extend the telescoping view's
  reach in a direction. Aim-and-extend in place; continuous.
- **REBASE_CACHE** — new basis, wholesale: change the fundamental basis of the
  analysis and fully rebuild the cache space around the new exploratory
  location. Discontinuous — re-found, not re-aim. Sets the "current position"
  that DECLARE after/override lean on.
- **RECONCILE** — orthogonal to both, and demand-driven: an analyst request for
  IMMEDIATE, prioritized cross-matching — forcing the deferred cross-processing
  (WIRE / cross-links / feedback) to the foreground and FULLY into the cache,
  generally because the result will impact current analysis and cannot wait for
  the pending list to drain on its own. Temporarily inverts the normal
  filing-over-cross-processing precedence, on request. Relevance-triggered, not
  housekeeping.

All cache-tier: global state transitions on the working set, non-arrayable.
Deferred.

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
  to the cores; RECONCILE/UPDATE_CACHE/REBASE_CACHE stub) + arraying executor
  (cross-command ordered stream over the four additive verbs; DELETE structurally
  excluded). Minimal testable surface; external wire deferred (G6). Done.
- `seed/` — `seed_0x` mints the seed floor (`0x` mass 0 + 16 hex atoms mass 1) via
  `mint`'s optional-mass bootstrap channel. Done.
- `ingestion/` — **RETIRED.** The pre-rebase path-A `Ingestor` + one-request-per-
  line `db_runtime` are superseded by `command/`+`declare/` (intake), `dispatch/`
  (routing), `seed/` (floor); `ingestion/README.md` is a supersession breadcrumb.

**Deferred (not built; recorded seams):** G4 next-slot mechanism (also gates
manager-placed mint), G5 block boundaries past hex couplets, G6 external
transport/wire format; **WAL management** (the WAL manager — a bookkeeper/observer
over WAL reports; books the DELETE report, with the cross-network validation act
deferred swarm-side; the file-now/wire-later runtime itself is the cache manager's,
§Process runtime — not a WAL-manager drain; design in `WAL-PLAN.md`, rev.5,
design-ready pending F4 + the NOTES charter entry); the **cache tier** (RECONCILE /
UPDATE_CACHE / REBASE_CACHE — stubs); the mass aggregation model; notation
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

**Cache tier** (RECONCILE, UPDATE_CACHE, REBASE_CACHE) — defined, deferred; a
late stage, after the record tier is solid.
