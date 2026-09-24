# Adversarial review (round 2) — NOTES.md revision of 2026-09-17

Reviewer: fresh, independent second-pass adversary. No stake in the document
passing. A build plan will be derived from these notes only once they pass, so
latent contradictions are expensive and are hunted for accordingly.

Target: `/opt/project/repo/engine/db_kernel/NOTES.md` (revised after round 1 to
apply fixes for F1–F7).
Prior review verified against:
`/opt/project/repo/engine/db_kernel/review/notes-review-2026-09-17.md`.
Ground-truth cross-checked: `schema/schema.sql`, `schema/README.md`,
`controller/controller.h`, `controller/README.md`, `codec/README.md`,
`ingestion/README.md`.

Read in full, section by section, then re-audited the supersession trail
independently (not trusting round 1 to be exhaustive) and grep-swept for every
stale term (`sibling`, `TYPE`/`gates`, `token.type`, `CHILDREN`, `per-type`).

---

## Summary of verdict

**PASS.** The round-1 blocker/major (F1/F2) — the unmarked supersession hole over
`### The literal intake formula` — is now closed: the subsection carries a local
REVISED marker that explicitly neutralizes both the `TYPE literal` gate and the
"sibling-group edge"/"label CHILDREN" wording while retaining every mechanic, and
the subsection (with its label-dual) is now named in the firmed section's
enumerated supersession list. F3–F7 are all fixed correctly. No new contradiction,
imprecision, or broken cross-reference was introduced by the fixes. The full
independent supersession-trail re-audit finds **no remaining unmarked
old-model statement** anywhere in the document. All eight firmed decisions are
CAPTURED and no retained precept was broken. A plan may now be built from these
notes.

---

## F1–F7 verification table

| ID | R1 severity | Status | Evidence (current text + location) |
|---|---|---|---|
| F1 | BLOCKER | **FIXED-CORRECTLY** | `### The literal intake formula` now opens (lines 446–454) with: "**REVISED 2026-09-17 — read through *Relationship model & type — firmed 2026-09-17*.** `TYPE` is dropped: the `TYPE literal` line in the template below, and \"TYPE gates them\", no longer apply — validation is **structural** (which downward field is present: `PARENTS` ⇒ structure, `MEMBERS` ⇒ grouping)." AND the firmed section's enumerated list now names it (line 118): "…the literal/label parents-children-siblings breakdown in **Type semantics**, **DECLARE format**, **The literal intake formula** (and its label-dual continuation), **Relationships stored BOTH directions**, and **In flight — next to lock**." Both halves of the R1 correction (local marker + enumerated entry) applied. Retained mechanics untouched (marker's closing sentence). |
| F2 | MAJOR | **FIXED-CORRECTLY** | Same marker, lines 451–453: "In the field descriptions, `MEMBER_OF` is the member-side of the **membership** pair (`members`/`member_of`), NOT a \"sibling-group edge\"; its reciprocal is the label's **`MEMBERS`** roster, NOT \"label CHILDREN\"." This explicitly names and neutralizes the exact stale phrases still present in-body at lines 502–503 ("Member-side of the sibling-group edge; the reciprocal (label CHILDREN)…") and line 559 ("…vs label CHILDREN"). R1 offered marker-restatement as an accepted correction path; the author took it. The reciprocity asserted is correct: `MEMBER_OF` (member→group) reciprocates the label's `MEMBERS` roster (group→members). |
| F3 | MINOR | **FIXED-CORRECTLY** | `## Analyst command semantics`, lines 92–94: "…records their membership in G (`members`/`member_of` — see *Relationship model & type — firmed 2026-09-17*)." The retired "sibling-group membership" wording is gone and a pointer to the firmed section is added. |
| F4 | MINOR | **FIXED-CORRECTLY** | `## Process runtime`, line 194–196: "cross-processing = the deeper cross-linking (the WIRE / `token_child` structure-reverse **and the `members`/`member_of` membership reciprocal**) and feedback." And lines 207–209: "…the reverse indexes (`token_child`, **and likewise the `members`/`member_of` reciprocal**) being eventually-consistent while deferred work drains." The membership reciprocal is now explicitly folded into the file-now/wire-later pending workstream, resolving the completeness gap. |
| F5 | MINOR | **FIXED-CORRECTLY** | `## Handoff` → Deferred, line 816: "**Group/boundary senses (#23)** — emergent kind-partition (sparsity) vs **stored membership (`members`/`member_of`)** vs the trunk-map boundary." Retired store name replaced. |
| F6 | OBSERVATION | **FIXED-CORRECTLY** | Firmed section, lines 148–150: "DECLARE is no longer `TYPE`-gated: validation is **structural** — which downward field is present (`PARENTS` ⇒ structure, `MEMBERS` ⇒ grouping); what is declared is stored, the readings emergent." The crisp "which downward field is present" definition is now hoisted into the authoritative section, matching the In-flight marker (lines 710–711); the section is self-contained. |
| F7 | OBSERVATION | **FIXED-CORRECTLY** | `### READ`, line 658: "…data path parents / members / both (**the per-axis downward reads** — parents = structure-down, members = membership-down)…". "per-type" → "per-axis"; the TYPE-lens vestige is removed. |

No finding is NOT-FIXED, FIXED-BUT-IMPERFECT, or REGRESSED.

**Non-blocking note on F2 (not a defect):** the neutralized stale phrases
("sibling-group edge", "label CHILDREN") still physically appear in the intake
body (502–503, 559) and are corrected only by the head marker. This is the exact
convention the document already uses for its four other marked sections (Type
semantics, DECLARE format, In flight, Open concerns) — marker at the head,
superseded prose retained beneath — so it is consistent and acceptable, not an
imperfection. Flagged only for transparency.

---

## New-defect hunt (defects introduced by the round-2 edits)

Walked every edited location for newly created contradiction, imprecision, or
broken cross-reference. **None found.** Specifics checked:

- **Enumerated-list insertion (line 118).** Adding "**The literal intake formula**
  (and its label-dual continuation)" to the mid-sentence list reads grammatically
  and every named section exists verbatim (Type semantics @215, DECLARE format
  @340, The literal intake formula @444, Relationships stored BOTH directions
  bullet @388, In flight — next to lock @707). No dangling reference.
- **F4 edit (194–196, 207–209).** Consistent with the firmed section's rule that
  membership "stores both directions (write one side, the reciprocal is
  maintained)" (lines 135–137) and with the eventually-consistent treatment of
  `token_child`. No new contradiction; it closes a gap rather than opening one.
- **F6 edit (148–150).** Does not conflict with the In-flight marker (710–711) —
  identical `PARENTS ⇒ structure, MEMBERS ⇒ grouping` mapping in both. No
  duplication conflict.
- **Intake-formula marker (446–454).** Every claim it makes is accurate: `>=2`
  literal floor (line 468) and `>=1` label floor (lines 555–556) do stand;
  grounding rule (520–528) and label-has-no-address (540–556) do stand; the
  MEMBER_OF↔MEMBERS reciprocity is correct. It does not over-reach (it does not,
  e.g., claim to retain anything that was in fact dropped).
- **F7 edit (658).** The two axes named (`parents` = structure-down, `members` =
  membership-down) are exactly the new model's downward reads; the rename is
  purely cosmetic and correct.

---

## Independent supersession-trail re-audit (every location touching the old model)

Whole-document sweep, not limited to round-1 findings. Every occurrence of the
old model is listed with its marking status.

| Location | Old-model content | Marked? | Disposition |
|---|---|---|---|
| Firmed section 112–179 | states the NEW model + enumerates what it drops | n/a (authoritative) | Correct |
| `## Analyst command semantics`, 92 | (was "sibling-group membership") | **Fixed** | Now "membership in G (`members`/`member_of` — see firmed section)" — F3 |
| `## Type semantics — literal vs label`, 224–286 (incl. "siblings" 274, "children = members" 280, "parents = super-groups" 279, "intake gates TYPE per node" 259, "type…read lens" 263–266, "mass reads down" 285) | siblings store; declared type as lens; label CHILDREN = members; token_parent = super-group; token_child = containment | **Yes** — PARTIALLY SUPERSEDED marker 217–222 names members/member_of, parent/child de-overload, TYPE dropped | Correctly marked |
| `### DECLARE format`, 348–397 (TYPE "non-optional; gates the rest" 351/356; `MEMBER_OF` "member-side sibling-group edge" 352–353; `CHILDREN` field 354, 380, 388–397; token_parent/child containment 396–397) | declared gating TYPE; CHILDREN as label field; sibling-group edge | **Yes** — REVISED marker 342–346 gives new field set + firmed-section pointer; the "Relationships stored BOTH directions" bullet is additionally named in the enumerated list (118) | Correctly marked |
| `## Open concerns` → "`token_child` two write sources by type", 302–306 | membership riding `token_child` | **Yes** — in-line SUPERSEDED 2026-09-17 marker; reverts token_child to pure structure-reverse | Correctly marked |
| **`### The literal intake formula`, 457–460** ("TYPE gates them"; `TYPE literal` template line) | declared gating TYPE | **Yes (newly)** — head marker 446–454 + enumerated list 118 | **Now correctly marked — F1 closed** |
| **`### The literal intake formula`, 502–503; label-dual 559** ("sibling-group edge"; "label CHILDREN") | dropped store + field name | **Yes (newly)** — head marker 451–453 explicitly neutralizes both phrases | **Now correctly marked — F2 closed** |
| Intake-formula label-dual 505–566 (grounding rule, naming literal, `>=1` floor, "MEMBER_OF"/"CHILDREN" wording) | siblings/children terminology | **Yes** — under head marker; retained precepts (grounding, label-has-no-address, floors) stand by design | Correctly marked |
| `### UPDATE — record-tier ops`, ADD_CONNECTION 611–620 ("membership", "dual of MEMBER_OF") | — | n/a | **No contradiction** — maps cleanly to `members`/`member_of` group-side authoring; names no dropped store |
| `### READ`, 658 | (was "per-type downward reads") | **Fixed** | Now "per-axis" — F7 |
| `## In flight — next to lock`, 714–721 (TYPE gates the required downward field) | declared gating TYPE | **Yes** — SUPERSEDED marker 709–712 with `PARENTS`/`MEMBERS` structural mapping | Correctly marked |
| `## Current build state`, 736 ("4 tables … `token_sibling_group` …") | current 4-table reality | n/a — states present reality, THEN the "REBASE 2026-09-17 (to build)" delta (739–742) drops `token_sibling_group` + `token.type`, adds `members`/`member_of` | Correct (statement of fact + forward action, matches schema.sql) |
| `## Handoff` → "Firmed and ready to build", 782–798 ("label dual", "MEMBER_OF", "label CHILDREN") | old terms | **Yes** — REBASED marker 782–788 with explicit term remapping | Correctly marked |
| `## Handoff` → "Deferred", 816 | (was "stored sibling-group membership") | **Fixed** | Now "stored membership (`members`/`member_of`)" — F5 |

**Result:** every old-model statement in the document is now either inside the
authoritative firmed section (as an explicit drop), under a supersession marker,
or corrected in place. The single structural hole round 1 identified (the intake
formula) is closed on both counts (local marker + enumeration). No location
round 1 missed was found unmarked.

---

## Completeness walk — firmed decisions 1–8

| # | Decision | Status | Evidence |
|---|---|---|---|
| 1 | Structure = `parent`/`child`, literal composition ONLY; `token_parent` de-overloaded | **CAPTURED** | 125–130: "Structure is literal composition ONLY; it no longer carries membership. This de-overloads `token_parent` (it stopped meaning both constituents and a sub-label's super-group)." |
| 2 | Membership = `members`/`member_of`, reciprocal pair, one-level arrayed follow, both directions, replaces sibling group | **CAPTURED** | 130–137: reciprocal pair; "Both pairs return a single one-level list per follow"; "stores both directions (write one side, the reciprocal is maintained)"; "This REPLACES the sibling-group in full: `token_sibling_group` is dropped." |
| 3 | `TYPE` dropped; literal/label EMERGENT from members-in-scope; LoD/scope-relative; `token.type` dropped; not type-gated; validation structural | **CAPTURED** — and no longer contradicted anywhere unmarked | 138–150 fully state it. R1's contradiction (intake formula) is now marked (F1). |
| 4 | Terminality NOT absolute — "no members relevant to the current LoD and scope" | **CAPTURED** | 139–144: "a **terminal literal** when it has **no members relevant to the current LoD and scope** … Terminality is NOT absolute." |
| 5 | Pairwise is the substrate; base calc → rebase by read+alignment over nominal ticks → exclusion of state space; split is storage detail beneath the one primitive | **CAPTURED** | 158–165, verbatim to the decision incl. "never a competing category." Reinforced by the SETTLED note at 307–310. |
| 6 | "No connection-kind operand" = anti-guardrail vs a hidden EQUIVALENCE table, NOT a ban on distinct relationship stores; kind read from the axis | **CAPTURED** | 167–174: "It is NOT a ban on distinct relationship stores. The relationship 'kind' is encoded in the memberships/structure themselves and read from the axis an edge sits on." |
| 7 | DECLARE fields: `ADDRESS`, `NOTATION`, `PARENTS`, `MEMBERS` (replaces `CHILDREN`), `MEMBER_OF`; no `TYPE` | **CAPTURED** — and no longer contradicted anywhere unmarked | 152–156 correct; DECLARE-format marker 342–346 and intake marker 446–454 close the former holes (F1/F2). |
| 8 | Schema: `token` (no `type`), `token_parent`, `token_child`, `members`, `member_of`; `token_sibling_group` dropped | **CAPTURED** | 177–179 and 739–742 list the net store identically; `token.mass` correctly retained (Open concerns 291–302). Matches `schema.sql` current state + stated delta. |

Net: **8/8 CAPTURED, 0 PARTIAL, 0 MISSING.** (Round 1 had 3 and 7 "CAPTURED but
contradicted unmarked"; those contradictions are now marked, so both are clean.)

---

## Retained-precept consistency (did round-2 edits break or over-reach anything?)

- **Address-IS-identity / no-forwarding / no-aliasing** (Governing principle
  23–28; Hard constraints 100–110): intact; firmed section 167–174 re-anchors the
  no-equivalence-table guardrail to it. Not broken.
- **Only-follow, never-search** (16, 102–103): intact; the membership pair is
  explicitly "only-follow, never search" (line 137). F4's addition keeps the
  reverse a stored follow, not a search. Not broken.
- **LoD rollup / label-becomes-particle** (244–260): stands; firmed section
  re-grounds it TYPE-free via the literal↔label ladder (140–144). Not broken.
- **Label has no own address; placed via naming literal** (540–556): intact;
  the intake marker explicitly lists "label-has-no-address" among retained
  precepts. Not broken.
- **`>=2` structure anti-alias floor / `>=1` membership floor** (154–156, 468,
  555–556): preserved and re-hung on "which field is present" (line 156); the
  intake marker confirms both stand. Not broken.
- **Mass on `token`** (Open concerns 291–302; schema 79–87 keeps `token.mass`):
  consistent with the rebase (type dropped, mass kept). Not broken.
- **Arraying-is-universal, UPDATE ops (MOVE/ADD_CONNECTION arrayable;
  DELETE pair single/confirmed/peer-validated/no-wildcard), no-MERGE**
  (568–641): untouched and consistent. Not broken.
- **Runtime reciprocal-maintenance policy** (round-1 soft gap F4): now explicit —
  `members`/`member_of` shares the file-now/wire-later pending workstream
  (194–196, 207–209). Gap closed.

No correct prior content was destroyed or over-reached by this round of edits.

---

## Ground-truth cross-check

- **`schema.sql` still defines exactly 4 tables** — `token` (54),
  `token_parent` (113), `token_sibling_group` (157), `token_child` (204), with
  `token.type` (77) and `token.mass` (87) present. This matches NOTES "Current
  build state" (line 736) and the "REBASE (to build)" delta (739–742): the notes
  correctly describe the pre-rebase files as still current and the rebase as a
  forward action, not as already done. No mis-statement.
- `schema/README.md`, `controller/controller.h`, `controller/README.md`,
  `codec/README.md`, `ingestion/README.md` still describe the pre-rebase world —
  expected and correctly represented by NOTES (rebase is "to build").

---

## Overall verdict

**PASS.** The document is coherent, precise to all eight firmed decisions, and
complete. Round 1's blocker and major (F1/F2) are closed by a correctly scoped
local marker plus the enumerated-list entry, F3–F7 are fixed correctly, the
round-2 edits introduced no new defect, the independent whole-document
supersession-trail re-audit finds no remaining unmarked old-model statement,
all 8 decisions are CAPTURED, and every retained precept remains internally
consistent. A staged build plan may now be derived from these notes.

- **Clean per category:** F1–F7 verification (all fixed), new-defect hunt (none),
  supersession trail (no unmarked contradiction), decisions 1–8 (8/8 captured),
  retained precepts (none broken), ground-truth (accurate).
- **Must-fix:** none.
- **Optional (author's discretion, non-blocking):** if desired, physically
  replace the two neutralized stale phrases in the intake body (502–503, 559)
  to reduce reliance on the head marker — purely cosmetic; the marker convention
  is already consistent with the rest of the document.
