# Adversarial review — NOTES.md revision of 2026-09-17

Reviewer: fresh independent adversary. No stake in the document passing.
Target: `/opt/project/repo/engine/db_kernel/NOTES.md` (revised to fold in the
2026-09-17 firmed decisions).
Ground-truth cross-checked: `schema/schema.sql`, `schema/README.md`,
`controller/controller.h`, `controller/README.md`, `codec/README.md`,
`ingestion/README.md`.

Scope of the check: is the revision COHERENT, PRECISE to the firmed decisions,
and COMPLETE, and did it destroy or over-reach on correct prior content. Read
in full, section by section.

---

## Summary of verdict

**PASS-WITH-FIXES.** The new authoritative section *Relationship model & type —
firmed 2026-09-17* is itself precise and complete against all eight firmed
decisions, and the four sections it names as superseded (*Type semantics*,
*DECLARE format*, *Relationships stored BOTH directions*, *In flight — next to
lock*) each carry an adequate supersession marker. The defect is a **hole in
the supersession trail**: the single most build-critical subsection — **`###
The literal intake formula`** (and its label-dual continuation) — is NOT in the
enumerated supersession list and carries NO local marker, yet it still presents
`TYPE literal` as a required gating field and still calls `MEMBER_OF` "the
sibling-group edge" with reciprocal "label CHILDREN". A plan derived by reading
that subsection top-to-bottom would encode the dropped TYPE gate. The global
"governs on conflict" clause resolves the conflict in principle, but the local
text is unreconciled and misleading exactly where a builder will look. Must-fix
before a plan is built: **F1, F2**.

---

## Findings (itemized)

### F1 — BLOCKER — `### The literal intake formula` still mandates TYPE gating, unmarked

- **Location:** section `### The literal intake formula (firmed 2026-09-15 15:12 MDT)`.
  - Line 442: "Fields are order-free; **TYPE gates them.** Applies per node."
  - Lines 444–450, the canonical DECLARE_RECORD template:
    ```
    DECLARE_RECORD
      TYPE      literal
      PARENTS   [ p0, p1, ... p{N-1} ]        # THE member array — sets N
      ...
    ```
- **Violates:** firmed decision **3** ("`TYPE` is dropped … DECLARE is no longer
  type-gated — validation is STRUCTURAL") and decision **7** (DECLARE fields are
  `ADDRESS`, `NOTATION`, `PARENTS`, `MEMBERS`, `MEMBER_OF`; **no `TYPE`**). The
  new section states this at lines 144–147 and 149–153.
- **Why it is a defect:** This subsection is *the* build spec for DECLARE — the
  handoff (line 776) points a plan-builder straight at "the literal intake
  formula" as the DECLARE specification. It has its own `###` header and is a
  sibling of `### DECLARE format`, so the REVISED marker at the head of *DECLARE
  format* (lines 337–341) does **not** reach it. It is also conspicuously ABSENT
  from the firmed section's own enumerated supersession list (lines 114–118),
  which names *Type semantics*, *DECLARE format*, *Relationships stored BOTH
  directions*, and *In flight* — but not the intake formula. A reader is thereby
  led to treat the intake formula as still-current, and it says `TYPE literal`
  is a required field that "gates them". That is precisely the expensive latent
  contradiction this review exists to catch. (Contrast line 254, "intake gates
  TYPE per node", which IS covered — it sits under the *Type semantics* marker at
  212. The intake-formula copies are not.)
- **Narrowest correction:** (a) add a supersession/rebase marker at the head of
  `### The literal intake formula` pointing to *Relationship model & type —
  firmed 2026-09-17*, stating that `TYPE` is dropped and validation is
  structural (which downward field is present: `PARENTS` ⇒ structure, `MEMBERS` ⇒
  grouping) while the PARENTS/ADDRESS-span/NOTATION/array mechanics stand; and
  (b) add `### The literal intake formula` (and its label-dual) to the
  enumerated list at lines 114–118 so the trail is complete.

### F2 — MAJOR — Intake formula + label-dual: `MEMBER_OF` called "sibling-group edge", label down-field called "CHILDREN", unmarked

- **Location:** same unmarked subsection.
  - Lines 486–488 (MEMBER_OF field description): "… **Member-side of the
    sibling-group edge**; the reciprocal (**label CHILDREN**) is
    manager-maintained."
  - Line 544 (label-vs-literal mirror): "required downward field — literal
    PARENTS (constituents, each >=2) vs **label CHILDREN** (members, non-empty)".
- **Violates:** decision **2** (membership is the `members`/`member_of`
  reciprocal pair — `token_sibling_group` is dropped; there is no "sibling-group
  edge") and decision **7** (the label down-field is **`MEMBERS`**, which
  replaces the old `CHILDREN`). New section lines 128–130, 149–151.
- **Why it is a defect:** the intake formula is where a builder reads the field
  semantics; "sibling-group edge" and "label CHILDREN" name a store and a field
  that no longer exist. Same unmarked-subsection root cause as F1 but a distinct
  substantive error (membership store identity + field name, not the TYPE gate).
- **Narrowest correction:** under the F1 marker, restate: `MEMBER_OF` is the
  member-side of the **membership** pair (`members`/`member_of`); its reciprocal
  is the label's **`MEMBERS`** roster; drop the words "sibling-group edge" and
  "label CHILDREN". (The `MEMBER_OF` *field name itself* is retained by decision
  7, so only its described semantics need the edit.)

### F3 — MINOR — `Analyst command semantics` still says "sibling-group membership", unmarked

- **Location:** `## Analyst command semantics (the relative assignment rule)`,
  line 91: "The system lays the points into trunk L … and records their
  **sibling-group membership** in G."
- **Violates:** decision **2** (the membership store is `members`/`member_of`;
  `token_sibling_group` is dropped). The membership *concept* survives; only the
  dropped store's name is stale.
- **Why it is a defect:** small, but it is an unmarked use of the retired store
  name in an operative (relative-assignment) section, with no pointer to the
  2026-09-17 model.
- **Narrowest correction:** reword to "records their membership in G
  (`members`/`member_of`)", or add a one-line pointer to the firmed section.

### F4 — MINOR — Process-runtime file-now/wire-later names only `token_child`; membership reciprocal maintenance not accounted for

- **Location:** `## Process runtime — the cache manager is a complete runtime`.
  - Line 192: "cross-processing = the deeper cross-linking (the WIRE /
    `token_child` reverse adjacency) and feedback."
  - Line 204: "the reverse index (`token_child`) being eventually-consistent
    while deferred work drains."
- **Issue:** the new model adds a second stored reciprocal — `members` ⇄
  `member_of` — which per lines 132–134 is "write one side, the reciprocal is
  maintained, exactly like structure". The file-now/wire-later description names
  only the `token_child` structure-reverse as the deferred/eventually-consistent
  reverse; it does not say the membership reciprocal joins the same pending-work
  workstream. Line 208 ("reciprocal listings") gestures at it generically but
  the concrete WIRE bullet does not.
- **Why it is a defect:** a completeness gap for the runtime stage — a builder
  needs to know the membership reciprocal has the same file-now/maintain-later
  treatment (or an explicit statement that it is maintained synchronously).
- **Narrowest correction:** extend the WIRE/deferred bullets to include the
  `members`/`member_of` reciprocal alongside `token_child`, or state its
  consistency policy explicitly.

### F5 — MINOR — Handoff "Deferred" list references "stored sibling-group membership", unmarked

- **Location:** `## Handoff — staged implementation`, "Deferred" list, line 799:
  "**Group/boundary senses (#23)** — emergent kind-partition (sparsity) vs
  **stored sibling-group membership** vs the trunk-map boundary".
- **Violates:** decision **2/8** (store dropped/renamed to `members`/`member_of`).
  The REBASED marker at lines 766–771 covers only the "Firmed and ready to build"
  list *above* it, not this Deferred list, so this reference is unmarked.
- **Narrowest correction:** "stored membership (`members`/`member_of`)".

### F6 — OBSERVATION — Structural-validation definition is split; the firmed section's phrasing is looser than the marker's

- **Location:** firmed section lines 146–147: "validation is **structural**
  (what is declared is stored; the readings are emergent)" — versus the *In
  flight* marker, lines 694–696: "validation is **structural** (which downward
  field is present: `PARENTS` ⇒ structure, `MEMBERS` ⇒ grouping)".
- **Note:** not a contradiction — the firmed section carries the precise
  "selected by which field is present" idea at lines 152–153 (the `>=2` / `>=1`
  floors). But the crisp definition of *structural validation* lives in the
  marker, not in the governing section. Consider hoisting the "which downward
  field is present" phrasing into the firmed section (lines 146–147) so the
  authoritative section is self-contained. Coherent as-is.

### F7 — OBSERVATION — READ "per-type downward reads" wording survives TYPE-drop

- **Location:** `### READ`, line 641: "data path parents / members / both (**the
  per-type downward reads**), reverse orientation likewise selectable."
- **Note:** the reads named (`parents` = structure-down, `members` =
  membership-down) are the correct axes under the new model; only the adjective
  "per-type" is a vestige of the dropped TYPE lens. Harmless (READ is unmarked
  but not contradictory). Optional: reword "per-type" → "per-axis".

---

## Completeness checklist — firmed decisions 1–8

| # | Decision | Status | Evidence |
|---|---|---|---|
| 1 | Structure = `parent`/`child`, literal composition ONLY; `token_parent` de-overloaded | **CAPTURED** | Lines 123–127: "Structure is literal composition ONLY; it no longer carries membership. This de-overloads `token_parent` (it stopped meaning both constituents and a sub-label's super-group)." |
| 2 | Membership = `members`/`member_of`, reciprocal pair, one-level arrayed follow, both directions stored; replaces sibling group | **CAPTURED** | Lines 128–134: reciprocal pair defined; "Both pairs return a single one-level list per follow"; "stores both directions (write one side, the reciprocal is maintained)"; "This REPLACES the sibling-group in full: `token_sibling_group` is dropped." |
| 3 | `TYPE` dropped; literal-vs-label EMERGENT from members-in-scope; LoD/scope-relative; `token.type` dropped; DECLARE not type-gated; validation structural | **CAPTURED** (in firmed section) — but **CONTRADICTED unmarked** in the intake formula | Lines 136–147 fully state it. Defect: lines 442/445 still show `TYPE literal` gating (F1). |
| 4 | Terminality NOT absolute — "no members relevant to the current LoD and scope" | **CAPTURED** | Lines 137–142: "a **terminal literal** when it has **no members relevant to the current LoD and scope** … Terminality is NOT absolute." |
| 5 | Pairwise is the substrate, not a conflict; base calc → rebase by read+alignment → exclusion of state space; split is storage detail beneath the one primitive | **CAPTURED** | Lines 155–162, verbatim to the decision, incl. "never a competing category." Reinforced by line 302–305 marking the old "open concern" SETTLED. |
| 6 | "No connection-kind operand" was an anti-guardrail vs a hidden EQUIVALENCE table (covert aliasing), NOT a ban on distinct relationship stores; kind read from the axis | **CAPTURED** | Lines 164–171, precise: "It is NOT a ban on distinct relationship stores. The relationship 'kind' is encoded in the memberships/structure themselves and read from the axis an edge sits on." |
| 7 | DECLARE fields: `ADDRESS`, `NOTATION`, `PARENTS`, `MEMBERS` (replaces `CHILDREN`), `MEMBER_OF`; no `TYPE` | **CAPTURED** (firmed section + DECLARE-format marker) — **CONTRADICTED unmarked** in the intake formula | Lines 149–153 and 337–341 correct. Defect: lines 445/486–488/544 still use `TYPE`/"label CHILDREN"/"sibling-group edge" (F1, F2). |
| 8 | Schema: `token` (no `type`), `token_parent`, `token_child`, `members`, `member_of`; `token_sibling_group` dropped | **CAPTURED** | Lines 173–176 and 719–726 both list the net store identically; `token.type` drop + `token_sibling_group` drop + `members`/`member_of` add stated; `token.mass` correctly retained (Open concerns 285–296). |

Net: 6/8 fully clean; decisions **3 and 7** are correct in the authoritative
section but left with **unmarked contradictions** in the intake formula
subsection (F1/F2). No decision is MISSING.

---

## Supersession-trail audit (every location still stating the old model)

| Location | Old-model content | Marked? | Disposition |
|---|---|---|---|
| `## Analyst command semantics`, line 91 | "sibling-group membership" | **No** | Unmarked contradiction → **F3** (MINOR) |
| `## Type semantics — literal vs label`, lines 219–281 (incl. 227, 254, 257–281; "siblings" 269, "children = members" 276, "parents = super-groups" 265) | siblings store; declared type as read-lens; label CHILDREN=members; token_parent=super-group | **Yes** — PARTIALLY SUPERSEDED marker 212–217, explicitly names members/member_of, parent/child de-overload, TYPE dropped | Correctly marked |
| `### DECLARE format`, lines 344–397 (TYPE "gates the rest" 345/351; `CHILDREN` 349/388–392; `MEMBER_OF` "sibling-group edge" 347) | declared gating TYPE; CHILDREN as label field; sibling-group edge | **Yes** — REVISED marker 337–341, gives new field set, points to firmed section | Correctly marked |
| `### DECLARE format` → "Relationships stored BOTH directions" bullet, 383–392 | label CHILDREN = members; literal SIBLING / sub-label PARENT | **Yes** — under DECLARE-format marker AND named in the enumerated list (line 117) | Correctly marked |
| `## Open concerns` → "`token_child` two write sources by type", 297–301 | membership riding `token_child` | **Yes** — SUPERSEDED 2026-09-17 marker in-line; reverts token_child to pure structure-reverse | Correctly marked |
| **`### The literal intake formula`, lines 442, 444–450** | **`TYPE gates them` / `TYPE literal` required field** | **No** | **Unmarked contradiction → F1 (BLOCKER)** |
| **`### The literal intake formula`, lines 486–488; label-dual line 544** | **`MEMBER_OF` = "sibling-group edge"; "label CHILDREN"** | **No** | **Unmarked contradiction → F2 (MAJOR)** |
| `### The literal intake formula` misc (parents/siblings/children wording, e.g. 490–498, 505–551 grounding rule) | "siblings"/"children" terminology | **No** (same unmarked subsection) | Folded into F1/F2 fix (add the head marker; retained precepts — grounding rule, label-has-no-address, >=1 floor — themselves stand) |
| `### UPDATE — record-tier ops`, ADD_CONNECTION 597–605 | "membership"/"dual of MEMBER_OF" | n/a | **No contradiction** — maps cleanly onto `members`/`member_of` (group-side authoring); does not name the dropped store. Home exists. |
| `### READ`, line 641 | "per-type downward reads" | **No** | **F7 (OBSERVATION)** — axes correct, adjective vestigial |
| `## In flight — next to lock`, 698–707 (TYPE gates the required downward field) | declared gating TYPE | **Yes** — SUPERSEDED marker 693–696, gives structural rule with `PARENTS`/`MEMBERS` mapping | Correctly marked |
| `## Handoff` → "Firmed and ready to build" list, 764–782 | "label dual", "MEMBER_OF", "label CHILDREN" | **Yes** — REBASED marker 766–771 with explicit term remapping | Correctly marked |
| `## Handoff` → "Deferred" list, line 799 | "stored sibling-group membership" | **No** — REBASED marker covers only the list above it | Unmarked contradiction → **F5** (MINOR) |

The four sections the firmed section *claims* to supersede (114–118) are all
correctly marked. The trail's single structural hole is the **omission of `###
The literal intake formula`** from that enumeration, matched by the absence of a
local marker on it (F1/F2).

---

## Retained-precept consistency (did the restructure break anything correct?)

- **Address-IS-identity / no-forwarding / no-aliasing** (Governing principle
  22–28; Hard constraints 99–109): intact, and the firmed section reinforces it
  (lines 166–168 tie the no-equivalence-table guardrail to it). Not broken.
- **Only-follow, never-search** (16, 100–102): intact; the new membership pair is
  explicitly "only-follow, never search" (line 134). Not broken.
- **LoD rollup / "label BECOMES the particle"** (239–255): stands, and the firmed
  section explicitly re-grounds it TYPE-free via the literal↔label ladder (lines
  140–142). Coherent under TYPE-dropped. Not broken.
- **Label has no own address; placed via its naming literal** (526–541): still
  coherent — a label is realized as membership edges keyed on members,
  referencing the group by the naming-literal token_id; under `member_of` those
  edges carry the group token_id. No read/write path is orphaned by the
  rename. Not broken.
- **>=2 anti-alias structure floor / >=1 membership floor** (151–153, 453–459,
  538–541): preserved and correctly re-hung on "which field is present" rather
  than declared type (line 152). Not broken.
- **Arraying-is-universal, UPDATE ops, cost hierarchy** (6–28, 553–626): untouched
  and consistent; DELETE gating, no-MERGE, no-wildcards all stand.
- **Mass model** (Open concerns 285–296; retained on `token`): consistent with
  schema rebase (type dropped, mass kept). Not broken.
- **Read/write-path homes after dropping the sibling store:** mass centroid over
  members → computed over `members`; `MEMBER_OF` authoring → `member_of` table;
  `ADD_CONNECTION` group-side → `members`/`member_of` (601–605, no store named,
  maps cleanly). No path left homeless. One soft gap: runtime reciprocal-
  maintenance policy for `members`/`member_of` is not spelled out (F4).

No correct prior content was destroyed or over-reached. The retained precepts
remain internally consistent with the new model.

---

## Ground-truth cross-check (Current build state vs schema.sql)

- **"4 tables" claim (line 720):** CORRECT. `schema/schema.sql` defines exactly
  four: `token` (54), `token_parent` (113), `token_sibling_group` (157),
  `token_child` (204). PK-only indexes confirmed (no secondary indexes; reverse
  traversal is the materialized `token_child`). `token_forwarding` /
  `address_classification` are indeed absent.
- **Rebase delta (lines 723–726):** ACCURATE. `schema.sql` has `token.type`
  (71–77) and `token.mass` (79–87). The stated delta — drop `token_sibling_group`
  + `token.type`, keep `token.mass`, add `members` + `member_of`, net store
  `token, token_parent, token_child, members, member_of` — is the correct
  transform and matches the firmed section (176). `token.mass` is correctly kept
  (Open concerns "FIXED: stored on `token`").
- **Note (not a NOTES defect):** `schema/schema.sql`, `schema/README.md`,
  `controller/controller.h` and `controller/README.md` still describe the
  pre-rebase world (`token.type`, `token_sibling_group`, `groups_of`,
  `add_group_membership`, "three data groups"). This is expected — NOTES marks
  the schema/controller rebase as "to build", and does not claim those files are
  updated. The pre-rebase state is stated correctly in NOTES; the rebase is a
  forward action, not a mis-statement.

---

## Overall verdict

**PASS-WITH-FIXES.**

The authoritative 2026-09-17 section is coherent, precise, and complete against
all eight firmed decisions, and it correctly marks the four sections it names.
The revision did not destroy or over-reach on correct prior content; retained
precepts stay consistent under the new model, and the ground-truth build state
is stated accurately.

The block on building a plan is the **supersession-trail hole over `### The
literal intake formula`**: the DECLARE build spec a plan will be derived from
still presents `TYPE literal` as a gating field (F1) and `MEMBER_OF` as the
"sibling-group edge"/"label CHILDREN" (F2), with no local marker and no entry in
the enumerated supersession list. The global "governs on conflict" clause makes
the intent unambiguous, so this is fixable rather than fatal — but it MUST be
fixed before a plan is built, because a plan-builder reading that subsection
directly would encode the dropped TYPE gate and the dropped store.

**Must-fix before a plan is built:** F1, F2.
**Should-fix (small, low-risk trail cleanup):** F3, F4, F5.
**Optional:** F6, F7.
