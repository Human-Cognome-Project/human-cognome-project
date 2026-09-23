# Plan — API face + record-tier core data functions

> **⚠ HISTORICAL (executed) — point-in-time staging record, not current design.** The
> record tier described here is built; treat specifics as of-their-time. In particular,
> **`RECONCILE` was removed from the dispatch surface (2026-09-22)** — it is no longer a
> face entry / stub / verb; the analyst messages the WAL manager directly. For current
> design see `NOTES.md` (messaging-realignment banner) and `ENDPOINT-ACTIVATION-NOTES.md`
> "RECONCILE".

**Provenance.** Drawn from `NOTES.md` as firmed 2026-09-17 and vetted clean
(`review/notes-review-2-2026-09-17.md`, PASS). This plan supersedes the earlier
pre-rebase plan (built on TYPE-gating / sibling-groups / the `token_child`
membership front-door), which is obsolete.

**Standing.** Design only. This plan is to be vetted by a fresh adversary against
the firm notes **before any implementation**. No coding agent is dispatched until
the plan itself passes the same review cycle. Nothing here is built without
agreement and vetting.

**Discipline.** Absolute precision to the precepts. Every element traces to a
stated precept in `NOTES.md`; where the spec defers or is silent, this plan flags
a seam and does **not** resolve it. Creative latitude is confined to direct code
structures (how a type/function/module is laid out in C++). Semantics, grammar,
field meaning, and command behaviour are transcribed from spec, not interpreted.

---

## 0. Scope

**In scope**

1. **The face of the API** — the request/response surface for all verbs. Full
   grammar for the record tier; the cache tier appears as named entries only.
2. **Core data functions of the fully-defined verbs** = the record tier:
   - `DECLARE_RECORD` — the intake formula (structure + grouping), no `TYPE`;
   - `READ_RECORD` — anchor / direction / depth / exclusions / linearized return
     (raw radial path);
   - `UPDATE_RECORD` — `MOVE_RECORD`, `ADD_CONNECTION`, `DELETE_RECORD`,
     `DELETE_CONNECTION`.
3. **The schema/door rebase** the record tier depends on (§II.0) — new tables and
   the dropped column/table — since the cores cannot be built over the pre-rebase
   store.

**Out of scope — named seams only, not designed, not resolved**

- **Cache tier core** — `RECONCILE`, `UPDATE_CACHE`, `REBASE_CACHE`. Named face
  entries and runtime stubs only.
- **The async runtime split** — file-now / wire-later via the pending list, and
  its drain/RECONCILE forcing. The record-tier cores write **synchronously**
  (§II.4); the deferral is the runtime layer's, out of scope.
- **Deferred items** (`NOTES.md` Handoff "Deferred", Open decisions): the mass /
  aggregation model (sum-vs-centroid, nested aggregation); extrapolation /
  relative-placement rules (the inert `extrapolate_address` seam); notation
  derivation; the prose→token_id swap; external transport/wire shape; next-slot
  mechanism; block boundaries past "hex couplets".

---

## 1. Baseline and the rebase it requires

**Current build state (`NOTES.md`):**

- **`codec/`** — address ↔ token_id, partial element, delta-by-position. Done.
- **`schema/`** — 4 tables (`token`, `token_parent`, `token_sibling_group`,
  `token_child`); `token.type`, `token.mass` present; PK-only indexes. Done.
- **`controller/`** — the door: reads `token_exists` / `parents_of` /
  `children_of` / `groups_of` / `attributes_of`; writes `mint`
  (see-mint-link-wire) / `add_group_membership`. Done.
- **`ingestion/`** — path-A entry + `db_runtime` verb dispatcher; `DECLARE`/`READ`
  functional in the **old flat form**; UPDATE / cache verbs are stubs.

**Rebase required by the new model (`NOTES.md` "Relationship model & type — firmed
2026-09-17"; Current build state REBASE note):**

- **Drop** `token_sibling_group` and the `token.type` column.
- **Add** `members` and `member_of` — the membership reciprocal, mirroring
  `token_parent` / `token_child`: PK-keyed, one-level arrayed follows, PK-only
  indexes (no reverse-search index — the reciprocal is a stored follow).
- **`token_parent` becomes structure-only** (constituents); `token_child` reverts
  to pure structure-reverse. Neither carries membership any longer.
- **`token_parent.mass` → nullable** (mass blank at declare; see §II.3).
- **Net store:** `token` (no `type`; `mass` retained, nullable), `token_parent`,
  `token_child`, `members`, `member_of`.

---

## 2. The gap this plan closes

Current face (ingestion `README`): the flat, `--type`/`--mass`-carrying
`DECLARE_RECORD`. Firm face (`NOTES.md`): named-field, order-free, **no `TYPE`**,
mass-free, span-addressed, arrayable, nestable; two orthogonal relationship axes
(**structure** `parent`/`child`, **membership** `members`/`member_of`); literal
vs label **emergent and LoD-relative**, never stored. The face is brought to the
firm formula; the core functions implement its semantics over the rebased store.

---

## Part I — The face of the API

### I.A Verb surface (all verbs named)

Record tier — full grammar: `DECLARE_RECORD`, `READ_RECORD`, and the four UPDATE
sub-ops. Cache tier — named entries, remain stubs: `RECONCILE`, `UPDATE_CACHE`,
`REBASE_CACHE`. Programmatic arrangement (whether the four UPDATE ops are
top-level verbs or sub-commands, dispatch attachment, module layout) is the
implementing agents' code-structure latitude; the plan fixes grammar and
behaviour only.

### I.B `DECLARE_RECORD` grammar — no `TYPE`

Named fields, order-free (`NOTES.md` DECLARE format REVISED marker; The literal
intake formula REVISED marker; firmed section):

- `ADDRESS` — a span expression (I.E).
- `NOTATION` — quoted; positional-partial, comma-delimited over the N slots;
  blanks legal (a blank/absent field → surface derived later, reads blank now).
- `PARENTS` — **structure** constituents; the member array, sets N; each member a
  list of `>= 2` constituent references (anti-alias floor); references only, no
  per-position mass.
- `MEMBERS` — **grouping** roster (replaces the old label `CHILDREN`); non-empty
  (floor `>= 1`). What this token contains as a group.
- `MEMBER_OF` — **membership, up**: the groups this token joins; two sections
  split by `|` (section 1 broadcasts to all N; section 2 a sparse per-member
  array). Member-side of the `members`/`member_of` pair.

**No `TYPE` field, no type gating.** Validation is **structural**: which downward
field is present — `PARENTS` ⇒ structure, `MEMBERS` ⇒ grouping — plus the field
constraints (structure `>= 2`, grouping `>= 1`). A node may carry structure
and/or grouping; the literal-vs-label reading is emergent, never asserted. Applies
per node (a nested statement mixes structure and grouping nodes).

**Grouping-node naming literal.** A grouping node (`MEMBERS` present) establishes
its naming literal one of two ways, and **`ADDRESS` is permitted** on it (naming
the naming literal's address — the label borrows that address; it has none
independently). (1) **Use-provided-ID:** an existing address is given (`PARENTS`
absent) → reference that pre-existing naming literal. (2) **Mint:** `PARENTS`
present ⇒ mint a new naming literal from that composition (required constituents
only, ≥2 floor, lean), placed at the target `ADDRESS` if given, else
manager-placed. A grouping node with **neither** `PARENTS` nor `ADDRESS` is
rejected. `NOTATION` is optional/human-facing, **never** the handle. The naming
literal must resolve to a live `token` row (`add_membership` FKs both endpoints);
the mixed `PARENTS`+`MEMBERS` node is the mint case. See NOTES *Build-phase
rulings — firmed 2026-09-17*. (Reject-until-ruled edges: N>1 mixed node; an
ADDRESS-span nested-declare colliding with a slot's own `PARENTS`.) What is placed is the naming
literal (via the structure path); the grouping is realized as membership edges
keyed on its members, referencing the group by the naming-literal token_id.

**Mass.** None declared. Declare writes mass **blank**; the assignment is recorded
as pending work and filled by the manager's aggregation workstream **per the
deferred aggregation model (sum-vs-centroid unresolved — not settled here)**. The
seed floor is the only declared mass (`0x` = 0/undefined; the 16 hex atoms = 1).

### I.C `READ_RECORD` grammar

- **Anchor** — a single token_id, or a **terminal wildcard** to read a range /
  full construct (a nominal, tree-constrained read — see NOTES *Build-phase
  rulings — firmed 2026-09-17*).
- **Direction** — optional, default RADIAL; narrowing to the per-axis downward
  reads: **parents** (structure-down), **members** (membership-down), or both,
  with **reverse** selectable as an orientation on those axes (not a peer value).
- **Depth (by factor)** — the LoD dial; depth 0 = the rolled-up node. Governs
  whether `members` are in scope (terminal/literal reading at coarse LoD,
  expands to the grouping reading deeper — the literal↔label ladder).
- **Exclusions** — specific token_ids **or a terminal-wildcard tree region** (a
  nominal, tree-constrained selection); no property predicates (an unbounded
  scan). Wildcards are terminal-only, never inline.
- **Cache-shaped** — named, OPTIONAL; deferred with the cache tier. The face
  names it; the core function is raw-radial only.
- **Return** — linearized, once-per-path, **no dedup** (repetition + position
  encode the structure losslessly).

### I.D UPDATE grammar (four ops)

- **`MOVE_RECORD`** — a `from→to` relocation. `from→to` is the relocation
  *relationship*, not a command form: **source-selection** and
  **destination-placement** are distinct ops (NOTES firmed 2026-09-17).
  - *Source (selection):* explicit token(s), a `FROM..TO` range, or a
    **terminal-wildcard prefix** selecting a whole trunk/branch — MOVE is the one
    op whose source may be a wildcard/range (its primary use: bulk correction of a
    misclassified branch). Only-follow over a contiguous region, not a search.
  - *Destination (placement):* an ADDRESS span, cover-N with N = the source unit
    count (all-explicit ⇒ N known ⇒ cover-N at IR; any range/prefix ⇒ deferred).
  Bookkeeping only: topology invariant; re-key the moved token(s) and
  cascade-repoint every referencing edge. No edge added/removed. **Ranged /
  arrayable** — a reversible reassignment, so unlike DELETE it is safe to array.
- **`ADD_CONNECTION <group> <element...>`** — assert membership: add `>= 1`
  elements as members of the group. Group is the scalar frame (must lead);
  elements are the arrayable side. Each addition is a pair; no connection-kind
  operand (the kind is read from the axis, not stored). Mints nothing (endpoints
  must pre-exist). Writes the `members`/`member_of` reciprocal. Group and/or
  elements may be a **terminal wildcard/range**, applying either direction (add
  wildcard-members to a group, or a member to wildcard-groups); the wildcard
  resolves to its address range at execution (Agent 5). Arrayable.
- **`DELETE_RECORD`** — remove a whole token. **Single instruction only, never
  arrayed.** Gates: (1) active pre-execution confirmation (full-target) + local
  execution; peer/cross-network validation is **deferred swarm-side** — the delete's
  report is booked by the WAL manager, and **no local validation-tag machinery** is
  built (firmed G7, 2026-09-18; see §II.5). Specific id only, no wildcard.
- **`DELETE_CONNECTION`** — remove one specific pair; single only; same gates;
  specific, no wildcard. Targets the **membership axis only** (structure removal
  is via DELETE_RECORD). Removes the named edge **and its stored reciprocal**
  (both-direction removal), so no reverse edge dangles.

### I.E `ADDRESS` span expression

Alphabet: `FROM:x` (start at x); `AFTER:b` (start at the next block after defined
block b — trunk-shift, honours the sparse gap); `TO:y` (terminal bound; a
condition on the preceding origin, never standalone); direct values; the `@`-pin
(a *requested* address the manager may override); the inert `undeclared →
cross-connection` hook for a left-undeclared slot.

Rules: segments mix; valid iff laid end to end they cover N. An open (un-`TO`'d)
origin is elastic and may be the **last** segment only. Interior segments are
self-delimiting. A nested declare mints one item → one address → one
self-delimiting slot, legal anywhere.

> **G4/G5 (open — Patrick's).** `FROM`/`AFTER` placement resolution leans on the
> next-slot mechanism (G4) and the trunk→kind map whose boundaries past "hex
> couplets" are unfixed (G5). The span grammar and cover-N validation are
> designable now; the fill uses the analyst-supplied-start form (needs no cursor)
> pending G4.

### I.F Arraying (universal, record tier)

An array **is** a compressed *serial* (ordered) command stream sharing the scalar
frame — not an unordered batch. Decompression is exact via frame broadcast + SEE
idempotency; the array form enforces co-index (equal length = member count,
aligned to the `from` slot) and cover-N. **Arrayable record-tier ops:** `DECLARE`,
`READ`, and the *additive* UPDATE sub-ops `MOVE_RECORD` and `ADD_CONNECTION`. The
**destructive** ops are not arrayable (`DELETE_RECORD`, `DELETE_CONNECTION`) — the
non-arrayability is part of the friction. Cache tier excluded.

### I.G Return contract

- `DECLARE_RECORD` → the **actual assigned address(es)** (not an echo); arrayed
  input → an ordered array of results; a rejected point → `un-ingested: <reason>`.
  A grouping-only node returns its **naming-literal token_id** (the borrowed
  handle), not a freshly placed address.
- `READ_RECORD` → the linearized once-per-path return.
- UPDATE ops → a per-op result.

### I.H Transport / framing — OPEN (Patrick, G6)

`db_runtime` today reads one request per line; the firm `DECLARE` is multi-field,
nestable, arrayable, and may not fit a line. The external command/wire shape is
Patrick's open decision (G6). This plan designs the logical grammar and the
in-process request IR (the parsed form the cores consume); a text→IR parser is
designed against the grammar, but the serialization/framing is a named seam, not
resolved.

---

## Part II — Core data functions (record tier)

Modules over the rebased door. Each states responsibility, design-level shape,
spec anchor, and **ships with tests** (project rule: tests on everything).

### II.0 Schema + door rebase (foundation)

- **Schema:** drop `token_sibling_group` and `token.type`; add `members` /
  `member_of` (PK-keyed reciprocal, PK-only indexes); `token_parent.mass` →
  nullable. **`COLLATE "C"`** on the `text[]` address columns (`token_id` + the FK
  columns) so PK/element order = codec byte order — prerequisite for gather/ranges
  (a pure collation pin; arrayed storage unchanged; equality reads unaffected).
  Net store per §1.
- **Door reads:** replace `groups_of` with `members_of(id)` (follow `members`)
  and `member_of(id)` (follow `member_of`); `attributes_of` drops `type`, keeps
  `mass`. Structure reads (`parents_of` / `children_of`) unchanged. Both new reads
  return a **deterministic (sorted) order**, matching `children_of`, so READ
  linearization is stable.
  - **gather** (new, firmed 2026-09-18) — the terminal-wildcard / `FROM..TO`-range
    enumerator: a contiguous **PK range scan on `token_id`** returning the existing
    tokens under a prefix or in a range, in PK order. Only-follow (walks a
    contiguous key interval on the existing PK — no new index, no seq scan, no
    reverse-search). Backs the wildcard/range resolution in the UPDATE and READ
    cores; independent of G4/G5 (it enumerates existing tokens, does not place new
    ones).
- **Door writes:**
  - `mint` — structure only (see-mint-link-wire into `token_parent` /
    `token_child`); drops its `type` parameter and stops writing `token.type`
    (the column is gone; `mint` no longer records a kind); `Constituent.mass` →
    `std::optional<int>` and LINK writes SQL NULL for blank mass (not `0` — "0 ≠
    not-yet-computed").
  - `add_membership(member, group)` — writes `member_of` **and** its `members`
    reciprocal (both directions); replaces `add_group_membership`. No mass, no
    kind operand.
  - **Mutation primitives** — `rekey(old_id, new_id)` re-keys `token`,
    re-derives `token.token_text` via the codec (the dot-join follows `token_id`,
    as `mint` does), and cascade-repoints every referencing row across
    `token_parent`, `token_child`, `members`, `member_of` in one FK-safe
    transaction (the schema has no `ON UPDATE CASCADE` and forbids triggers, so
    the primitive owns the ordering).
  - **Delete primitives** — `delete_token(id)` and `delete_pair(a, b)` (with its
    reciprocal), single, gated at the op layer.

### II.1 Command IR + validation (the parsed face)

Typed request structures per record-tier verb; the parsed form the cores consume.
**Structural** validation (no TYPE): required downward field present, structure
`>= 2` / grouping `>= 1`, span cover-N, nested declares recurse. **Co-index** rule:
equal-length applies to the member-*differentiator* arrays (length = member count,
aligned to the `from` slot); it does NOT apply to the two intra-field forms —
`NOTATION` is length-N with legal blank slots (successive commas), and `MEMBER_OF`
section 2 is a *sparse* (≤N) per-member array. Neither is a co-index violation.
*Spec:* DECLARE format REVISED; The literal intake formula (NOTATION, MEMBER_OF);
In flight — next to lock (structural validation).

### II.2 Address span planner

`plan(span, N) → [address per slot] + validity`. Cover-N, elastic-last-only,
interior self-delimiting, nested-declare-as-one-slot. Adds a **base-50
address-successor** helper over the codec (the codec exposes none) for sequential
fill; depth expansion leans on the deferred trunk map (G5). Fill =
analyst-supplied-start form pending G4. *Spec:* The literal intake formula
(ADDRESS).

### II.3 DECLARE core

`PARENTS` sets N; per node, `mint` structure via the controller (SEE-dedup
idempotent, ensure-plus-link). `MEMBERS` (grouping) and `MEMBER_OF` (membership)
author the `members`/`member_of` reciprocal via `add_membership` (idempotent in
the door — no caller-side guard needed). A grouping node's naming literal is
either **used by provided-ID** (an existing `ADDRESS`) or **minted from PARENTS**
(placed at the target `ADDRESS`, else manager-placed); the label is keyed on that
naming-literal token (see I.B). `NOTATION` stored positional-partial (blanks read
blank until derivation lands); optional/human-facing, never the handle. Mass blank
(pending work); seed floor is the exception. No TYPE — structural validation.
*Spec:* The literal intake formula + firmed section; label dual (grounding rule,
label-has-no-own-address — a group is realized as membership edges keyed on
members, referencing the group by its naming-literal token_id).

### II.4 READ core (raw radial)

Only-follow from the anchor. **Structure axis** follows `parents_of` /
`children_of`; **membership axis** follows `members_of` / `member_of`. Because
structure and membership are separate stores, the axes are distinct follows — no
type-lens over a shared table. Direction narrowing; depth (LoD, governs
members-in-scope); named-branch exclusions; linearized once-per-path, no dedup.
Cache-shaped mode deferred. *Spec:* READ — record-tier exploratory read.

### II.5 UPDATE core (four ops)

- **`MOVE_RECORD`** — `rekey` + cascade-repoint across all four relationship
  stores (all reverses are stored follows, not searches). Ranged/arrayable. For a
  **wildcard/range source**, resolve it via the **gather** primitive (the
  contiguous PK-range walk, §II.0) to its occupied token set, then enforce cover-N
  (destination covers the resolved N). The UPDATE core must **reject** a MOVE
  destination containing mint-bearing nested-declare or undeclared-hook segments
  (a relocation mints nothing; the IR shape-check permits them, Agent 5 rejects —
  the deferred half of adversary F-1).
- **`ADD_CONNECTION`** — pairwise add via `add_membership` (idempotent, both
  directions). Arrayable. A **terminal-wildcard** group/elements resolves via the
  **gather** primitive to its address range and enumerates the pairs (either
  direction — wildcard members into a group, or a member into wildcard groups).
  **Both sides wildcard → full cross-product** (M gathered groups × N gathered
  members = M×N edges), blessed 2026-09-18 as a legitimate bulk op.
- **`DELETE_RECORD` / `DELETE_CONNECTION`** — `delete_token` / `delete_pair`
  behind a **full specific-target confirmation**: the op echoes the exact target
  ("did you mean to delete this specific thing?") and requires confirming THAT
  target — no bare/fire-and-forget delete, no blanket flag — then executes
  **locally**. Specific-id only (no wildcard); non-arrayed; DELETE_CONNECTION
  removes the reciprocal too (door-idempotent) and targets the membership axis
  only (structure removal is via DELETE_RECORD).

> **G7 — resolved into WAL management (firmed 2026-09-18).** The peer /
> cross-network validation of a delete (tentative→systemic across instances) is
> NOT built at the record tier; the delete's report is **booked by the WAL manager**
> (a bookkeeper/observer over WAL reports), while the cross-network validation
> **act** is **deferred swarm-side**, not run by the WAL bookkeeper. The record tier
> builds only the local full-validation + local execution. No local validation-tag
> machinery. (WAL-manager-is-bookkeeper, reframed 2026-09-18.)

> **G10 (open).** `DELETE_RECORD`'s behaviour when the token is still referenced
> by dependents (the four stores' FKs to `token`) — block / cascade / repoint —
> is unspecified in NOTES. Flagged, not resolved (inventing a cascade rule would
> breach "no invented complication").

### II.6 Reciprocal timing (synchronous)

Every reciprocal (`token_child` structure-reverse, and `members`/`member_of`
membership) is written **synchronously** within the authoring op, matching
`mint`'s current wiring. The firmed-foundational file-now / wire-later deferral
(pending list, drained on its own; RECONCILE forces an immediate prioritized
drain) is consciously left to the out-of-scope runtime/cache layer. *Spec:*
Process runtime; firmed section (reciprocal maintained).

### II.7 Arraying executor (cross-cutting layer)

Wraps the single-op cores: frame broadcast, co-index zip, ordered execution, SEE
collapse. Additive ops only. *Spec:* Arraying is universal.

### II.8 Dispatcher wiring

Wire the record-tier cores behind a **verb dispatch over the in-process IR**
(nested/arrayed — never one-request-per-line, which is superseded as never the
intent): DECLARE→declare core, READ→read core, UPDATE ops→update core, all through
the arraying executor for the additive verbs; cache verbs stay stubs. The external
wire/framing format is DEFERRED (G6, firmed 2026-09-18) — build only a **minimal
testable surface**; the bar for this stage is that the verbs are testable through
the dispatch layer. Also reconcile the pre-rebase entry: rewire `seed/seed_0x.cpp`
to the rebased door (`mint` optional-mass bootstrap channel) and supersede/rework
the old flat-form `ingestion/` path that calls the removed door surface.

---

## Part III — Delegation intent (Sonnet agents)

Progressive, dependency-ordered. Each agent is precept-bounded (no creative
interpretation beyond code structure) and **ships tests**. Model budget is not a
constraint — agents are used freely.

| Agent | Builds | Depends on |
|---|---|---|
| 1 | II.0 schema + door rebase (new tables, dropped column/table, `member_of`/`members` read+write, mutation/delete primitives, `Constituent.mass` optional) | baseline |
| 2 | II.1 Command IR + structural validation, II.2 span planner (+ base-50 successor) | baseline |
| 3 | II.3 DECLARE core (structure + grouping, synchronous reciprocal) | 1, 2 |
| 4 | II.4 READ raw radial (structure + membership axes) | 1, 2 |
| 5 | II.5 UPDATE four ops (over the II.0 primitives + gates) | 1, 2 (Agent 3 for test fixtures only — the UPDATE cores build on the II.0 door + II.2 span grammar, not on the DECLARE core) |
| 6 | II.7 arraying executor + II.8 dispatcher wiring | 3, 4, 5 |

Acyclic; a valid order is 1, 2, 3, 4, 5, 6 (1 and 2 are independent and may run in
parallel). No agent depends on an unbuilt capability: the schema/door rebase
(Agent 1) is the foundation every core sits on, so it leads. **II.6 (synchronous
reciprocal) is not a separate agent** — it is enforced by `add_membership` writing
both directions (Agent 1) and by the DECLARE/UPDATE cores calling it synchronously
(Agents 3, 5). The **gather** primitive (§II.0, firmed 2026-09-18) is a build-1
door follow-up that Agent 5 (and a READ wildcard-anchor follow-up) build on. Open
seams: G4/G5 use the analyst-supplied-start form (G4 also gates manager-placed
mint); G6 leaves transport parked behind the IR; **G7 is resolved into WAL
management** (local full-validation + local execution now); G10 holds
DELETE_RECORD to unreferenced tokens (reject-on-referenced) until decided.

---

## Part IV — Spec-conformance map (for the adversary)

| Module | Governing precepts (`NOTES.md`) |
|---|---|
| II.0 | Relationship model & type — firmed 2026-09-17; Current build state REBASE; Hard constraints (only-follow) |
| I.B / II.1 / II.3 | The literal intake formula (REVISED); firmed section (DECLARE fields, structural validation, mass blank); label dual (grounding rule, no own address) |
| I.E / II.2 | The literal intake formula (ADDRESS); DECLARE addressing modes; Open decisions (next-slot) |
| I.C / II.4 | READ — record-tier exploratory read; Re-convergence; firmed section (per-axis reads, LoD-relative) |
| I.D / II.5 | UPDATE — record-tier ops (all four); firmed section (membership pair) |
| II.6 | Process runtime; firmed section (reciprocal maintained) |
| I.F / II.7 | Arraying is universal |
| I.G / II.8 | DECLARE addressing modes (returns actual address); ingestion runtime request surface |
| I.H | Open decisions (input shape) |
| Cross-cutting | Governing principle; Hard constraints (only-follow, address-is-identity/no-forwarding, no invented complication); firmed section (pairwise substrate, no-equivalence-table, emergent LoD-relative type) |

---

## Part V — Open seams (Patrick's; none block the in-process cores)

- **G4 — Next-slot mechanism.** Analyst-supplied start vs per-trunk cursor —
  governs `FROM`/`AFTER` fill. Span planner uses analyst-supplied-start meanwhile.
- **G5 — Block boundaries past "hex couplets".** Trunk→kind map extent.
- **G6 — External transport/framing.** Whether/what external wire/file
  presentation for the multi-field/nested/arrayed request. IR + parser designed
  regardless.
- **G7 — RESOLVED into WAL management (2026-09-18).** The delete peer /
  cross-network validation (tentative→systemic) is **booked by the WAL manager** (a
  bookkeeper/observer over WAL reports); the delete's report is booked and the
  cross-network validation **act** is **deferred swarm-side**, not run by the
  bookkeeper. The record tier builds the local full-target confirmation + local
  execution only — no local tag machinery.
- **G10 — `DELETE_RECORD` FK-dependent policy.** Block/cascade/repoint on a still
  referenced token — unspecified in NOTES; flagged, not resolved (held as
  reject-on-referenced, no invented cascade).

**Resolved (2026-09-18):** the terminal-wildcard / range enumeration for MOVE's
bulk source, ADD_CONNECTION's wildcard expansion, and READ's wildcard anchor is
built via the **gather** primitive (§II.0) — a contiguous PK-range walk,
only-follow. (READ's earlier wildcard-anchor deferral is now fillable over it.)

**Deferred models (named, not designed):** mass aggregation (sum-vs-centroid,
nested aggregation); notation derivation; prose→token_id swap; extrapolation /
relative-placement rules. **Still open:** G4 (next-slot — also gates
manager-placed mint), G5 (block boundaries), G6 (external transport).
