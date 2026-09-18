# NOTES currency & coherence review — record tier (2026-09-18)

**Reviewer role:** fresh, independent adversary. **Target:**
`engine/db_kernel/NOTES.md`. **Ground truth:** the built record-tier modules on
branch `dbkernel-design-checkpoint` (`89fd066`..`aefc9e6`) — `schema/schema.sql`,
`controller/controller.h`, `command/command_ir.h` + `command/span_planner.h`,
`declare/declare_core.h`, `read/read_core.h`, `update/update_core.h`,
`dispatch/dispatch.h`, `seed/seed_0x.cpp`, `ingestion/README.md`, and `PLAN.md`.
No file was modified.

**Question asked:** does NOTES accurately and coherently reflect the record tier
as actually built — and what is now stale, contradicted, or unmarked?

**Headline:** the *authoritative* "Current build state" section is accurate and
matches the code table-for-table, primitive-for-primitive. But two end-of-document
**design-era sections still present the finished record tier as pending future
work**, and two of the four **"Open decisions" are actually closed** (one done,
one resolved-and-referencing a deleted struct). A reader who trusts document order
— or who lands in the Handoff / In-flight sections, which are literally the last
and second-to-last headings — is told the build has not happened.

---

## What is CURRENT (verified accurate)

- **`## Current build state` (870–917) is accurate.** Every claim checks against
  code:
  - schema: 5 tables `token` (no `type`, `mass` nullable), `token_parent`,
    `token_child`, `members`, `member_of`; `COLLATE "C"` on the `text[]` address
    columns; PK-only indexes — matches `schema/schema.sql` exactly.
  - controller: reads `token_exists`/`parents_of`/`children_of`/`members_of`/
    `member_of`/`attributes_of`; writes `mint` (no type; optional mass → SQL
    NULL), `add_membership`, `rekey`, `delete_token`, `delete_pair`; `gather`
    (both overloads) — matches `controller/controller.h` exactly.
  - command / declare / read / update / dispatch / seed / ingestion(RETIRED) —
    each matches its header/module.
- **Build-phase rulings — firmed 2026-09-17/18 (684–819)** are consistent with the
  code: terminal-only wildcards, `COLLATE "C"`, gather primitive, MOVE
  source/destination split, READ terminal-wildcard anchor, ADD_CONNECTION both-
  sides cross-product, DELETE local-full-validation + G7→WAL, mass-none-declared —
  all present in the built modules.
- **Relationship model & type — firmed 2026-09-17 (112–179)** governs and matches
  the schema/controller (TYPE dropped, `members`/`member_of` pair, de-overloaded
  structure, net store of 5).
- The supersession **markers** on the pre-rebase prose (DECLARE format REVISED,
  intake-formula REVISED, Type-semantics PARTIALLY SUPERSEDED, `token_child`
  two-write SUPERSEDED, pairwise SETTLED) are present and correctly scoped.

---

## Findings

### C1 — BLOCKER — "Handoff — staged implementation" reads as a pending future mission (930–978)

**Location / quote:** §"Handoff — staged implementation (for the next context)":
> "Mission of the next context: turn the firmed design into a **staged
> implementation plan for Sonnet agents, each stage under adversarial review** …"

and, in "Firmed and ready to build (the record tier)":
> "Address/codec + **4-table schema** + controller door + **path-A ingestion** +
> the **`db_runtime` verb dispatcher** already exist (see Current build state)."

**Stale vs. code:** the mission is EXECUTED — the staged plan is `PLAN.md`, the
agents ran, and the record tier is COMPLETE and committed. The baseline line is
doubly contradicted by the actual Current build state: the schema is **5 tables**
(not 4), **path-A ingestion is RETIRED** (`ingestion/README.md` breadcrumb), and
`db_runtime` is superseded by `dispatch/`. This section is the LAST heading in the
file; an agent reading it as the "where things stand / what's next" pointer is
told to go build what already exists. Actively misleading about built state.

**Narrowest fix:** add an `> **EXECUTED 2026-09-18**` banner at the section top
stating the staged plan (`PLAN.md`) and the record-tier build are complete (see
Current build state), and correct or strike the "4-table schema / path-A
ingestion / `db_runtime` already exist" baseline line (it predates the build).

### C2 — BLOCKER — "In flight — next to lock (design, not built)" asserts built work is unbuilt (844–867)

**Location / quote:** the heading itself — "**In flight — next to lock (design,
not built)**" — over bullets "**DECLARE format firming.**" and "**UPDATE sub-ops —
FIRMED (2026-09-15)…**".

**Stale vs. code:** DECLARE format is built (`command/command_ir.h` +
`declare/declare_core.h`); the four UPDATE sub-ops are built
(`update/update_core.h`). Nothing in this section is "in flight" or "not built."
The `> **SUPERSEDED 2026-09-17**` marker at the top only retracts the *TYPE-gating*
content of the first bullet; it does **not** touch the section's "design, not
built" status framing, which is now false. The word "not built" in a heading is
the most direct possible mis-statement of build state.

**Narrowest fix:** retitle the heading (e.g. "In flight — next to lock —
**BUILT 2026-09-18**") or add an EXECUTED banner pointing to Current build state;
the array-validation detail can stay as historical record beneath it.

### C3 — MAJOR — "Open decisions": "Input shape for ingestion" is resolved and names a deleted struct (926–927)

**Location / quote:**
> "- **Input shape for ingestion** — keep the in-process `dbk::DataPoint` struct,
> or add an external file/wire presentation (and what shape)."

**Stale vs. code:** resolved by "Request transport (firmed 2026-09-18)" (812–819)
and executed in `dispatch/`: the in-process **IR** is the surface now, external
wire/framing is the deferred **G6** seam. The `dbk::DataPoint` struct no longer
exists — path-A ingestion (which owned it) is RETIRED. So this is listed
open-but-resolved AND points at a defunct type.

**Narrowest fix:** strike the bullet, or replace it with a one-line pointer:
"RESOLVED — in-process IR is the record-tier surface (`dispatch/`); external wire
deferred to G6 (see *Request transport — firmed 2026-09-18*)."

### C4 — MAJOR — "Open decisions": "Re-validate … and commit the set" is done (928)

**Location / quote:**
> "- **Re-validate** the changed schema/controller pieces, and **commit** the set."

**Stale vs. code:** the schema/controller pieces were re-reviewed
(`review/build-schema-review-*`, `build-collation-review-*`) and committed; the
whole record tier is committed through `aefc9e6`. This is a finished action item
still listed as an open decision.

**Narrowest fix:** remove the bullet (it is executed), leaving only the two
genuinely-open items (Next-slot G4, Block boundaries G5).

### C5 — MINOR — "API command vocabulary" status tags are design-era for the record-tier verbs (315–338)

**Location / quote:** "DECLARE_RECORD … *Functional.*"; "UPDATE_RECORD … *FIRMED
(2026-09-15) into four sub-ops …*"; "READ_RECORD … *Functional in the raw radial
path …*".

**Stale vs. code:** these record-tier verbs are now BUILT/complete
(`declare/`, `update/`, `read/`), not merely "functional" or "firmed." The
cache-tier tags (`*Stub*`, `*Stub (deferred)*`) remain correct (dispatch stubs).
Not contradicted by Current build state, but the italic status tags read as
pre-build.

**Narrowest fix:** bump the three record-tier tags to "*built (record tier)*"
(or add a one-line "status tags below predate the build; see Current build
state" note under the section head).

### C6 — MINOR — "DECLARE addressing modes": `from`/`after` still tagged "*To build.*" (411–424)

**Location / quote:** "**declare from** … *To build.*" and "**declare after** …
*To build.*".

**Stale vs. code:** `FROM` is built (`command/span_planner.h` successor +
`plan()`, exercised by `declare/` and `seed/`); `AFTER` is built structurally and
reports `kPendingSeam` pending the G5 trunk map. Neither is "to build" any longer.
The section carries a "Superseded framing (2026-09-15)" note but that addresses
mode-vs-segment framing, not the build status.

**Narrowest fix:** change `from`'s tag to "*built*" and `after`'s to "*built,
origin pending G5 (trunk map)*".

### C7 — OBSERVATION — "Process runtime" implies async work-balancing is present (181–213)

**Location / quote:** "Work balancing is **foundational, not deferred** …" and
"Today `mint` does link+wire synchronously; the runtime splits this into
**file-now / wire-later-via-pending-list** …".

**Stale vs. code:** the record tier writes **synchronously** (PLAN §II.6; every
reciprocal written inside the authoring op — `add_membership`, `mint`'s WIRE).
The file-now/wire-later split is deferred to WAL management (Current build state
"Deferred"). The section has no marker distinguishing built-now (synchronous)
from deferred (async split), so a reader could take the async runtime as built.

**Narrowest fix:** add a one-line "record tier writes synchronously; the async
file-now/wire-later split is deferred to WAL management (see Current build state)"
under the section head.

### C8 — OBSERVATION — "Open concerns": mass column is a completed to-do (288–298)

**Location / quote:** "A token's own mass — **FIXED: stored on `token`.** … Schema:
add a mass column to `token`."

Correctly tagged FIXED, but "Schema: add a mass column" is a to-do phrasing whose
work is done (`token.mass` present). No action strictly required — the FIXED tag
carries the resolution — but the trailing imperative could be reworded to past
tense for cleanliness.

### C9 — OBSERVATION — pre-rebase DECLARE-format / intake-formula bodies still show `TYPE`/`CHILDREN` (348–354, 459–464)

The retained body text ("`TYPE` (non-optional; gates the rest)", "`CHILDREN`",
the `TYPE literal` template line) contradicts the built code, **but** each is
covered by an explicit `> **REVISED 2026-09-17**` marker directing the reader to
the governing section — consistent with the document's own supersession
convention. No fix required; recorded for completeness so a future sweep does not
mistake the markers for missing.

---

## Currency checklist

| Check | Result |
|---|---|
| Current-build-state section accurate to the code? | **YES** — tables, primitives, module names all verified. |
| Handoff / In-flight / status-tags current? | **NO** — Handoff (C1) and In-flight (C2) present built work as pending; API-vocabulary tags (C5) and addressing-mode tags (C6) are design-era. |
| Open-decisions correctly sorted (resolved vs. open)? | **NO** — 2 of 4 closed: "Input shape" resolved + names a deleted struct (C3); "Re-validate/commit" done (C4). Next-slot (G4) and Block boundaries (G5) are correctly still open. |
| No unmarked pre-rebase contradiction? | **MOSTLY** — the intake/DECLARE-format bodies are properly marked (C9); the unmarked drift is the *status/mission* framing (C1, C2, C7), not the type model. |

---

## Verdict

**NEEDS-FIXES.**

**Must-fix:** **C1, C2** (BLOCKER — the file's two closing sections tell a reader
the record tier is unbuilt), **C3, C4** (MAJOR — closed "Open decisions", one
naming a deleted struct).

C5–C7 should be cleaned in the same pass (cheap, and they compound the same
"design-era text left in place" impression); C8–C9 are optional tidy-ups.

The knowledge itself is not stale — the firmed/build-phase sections and Current
build state are correct and coherent. What is stale is the **document's
forward-looking scaffolding**: the handoff mission, the "in flight / not built"
framing, and two open-decision bullets were never retired after the build they
anticipated actually landed.
