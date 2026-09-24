# Adversarial review 2 — PLAN.md against NOTES.md + subdir specs (second pass)

Date: 2026-09-16. Reviewer: fresh, independent adversary, second pass, no stake in
the plan passing. Target (revised): `/opt/project/repo/engine/db_kernel/PLAN.md`.
Governing spec read in full: `NOTES.md`, `schema/README.md`, `schema/schema.sql`
(ground-truthed for nullability/FKs/shapes), `controller/README.md`,
`controller/controller.h`, `ingestion/README.md`, `codec/README.md`. Round-1
review verified against: `review/plan-review-2026-09-16.md` (F1–F13,
PASS-WITH-FIXES).

**Headline.** Round-1 F1–F13 are all folded in faithfully and without
over-extension; the five human-authorized resolutions (G1, G2/G8, G3, F5/F12, G9)
are applied minimally and are spec-consistent where I could ground them. But the
revision's own F3 fix — the type-lens that separates READ's "members" axis from
"reverse-adjacency" over `token_child` — is **incomplete against a firmed
precept it did not account for** (`NOTES.md` line 308–312: a *sub-label*
member's membership is stored as a `token_parent` PARENT edge, not a
`token_sibling_group` SIBLING edge). The lens as written mis-classifies sub-label
members, and the symmetric write-side reduction (all membership → `token_child`
`token_sibling_group`) drops the sub-label case entirely. These are new MAJORs
(N1, N2) sharing one root cause. Verdict: **PASS-WITH-FIXES**.

---

## New findings introduced or surviving in this revision

### N1 — MAJOR — the F3 read-lens mis-classifies sub-label members; `token_parent` is overloaded (composition AND sub-label membership), so "lists anchor via token_parent ⇒ composition" is wrong

- **Location:** §II.4 G3 box, lines 306–308 — *"for each `token_child` counterparty `c`, `c` lists the anchor via `token_parent` ⇒ composition/reverse-adjacency, or via `token_sibling_group` ⇒ membership/member. This is only-follow (direct reads on `c`), no search."* Repeated in §II.5, lines 324–326.
- **Precept contradicted:** `NOTES.md` "Relationships stored BOTH directions", lines 308–312: *"For a label, CHILDREN are its group members — the reciprocal of each member's own membership edge: **a literal member's SIBLING, or a sub-label member's PARENT.** (e.g. … `single hex codes` **PARENTS** `hex value tables` ⟺ `hex value tables` CHILDREN `single hex codes`.)"* And `NOTES.md` Type semantics, lines 194–201: literal `parents (token_parent) = constituents`; label `parents (token_parent) = larger groups it is a sub-group of`. And the lens definition, lines 160–163: the reading is derived from *"endpoints' types + direction"*, not from which table an edge sits in.
- **Why it is a defect (concrete walk):** Take anchor = `hex value tables` (a super-group). `children_of(hex value tables)` returns `single hex codes` (via the containment reciprocal `token_child(hex value tables, single hex codes)`). The plan's lens now asks: does `single hex codes` list the anchor via `token_parent` or `token_sibling_group`? Per NOTES line 310–311 it lists it via **`token_parent`** (`single hex codes` PARENTS `hex value tables`). The plan's rule therefore classifies `single hex codes` as **composition / reverse-adjacency** — but it is in fact a **member** (a sub-label member of the super-group). The lens gives the wrong reading for every sub-label member. `token_parent` is overloaded exactly as `token_child` is: it holds *both* a literal's constituents *and* a sub-label's super-group membership, with no discriminator. Distinguishing them requires the *endpoint types* (is `c` a literal → anchor is its constituent → reverse-adjacency; is `c` a label → anchor is its super-group → `c` is a member), i.e. the actual lens NOTES specifies (`attributes_of(c).type`), not the table the edge lives in. The plan under-implemented the lens.
- **Narrowest correction:** In the G3 box / §II.5, replace the "which table" test with the endpoint-type test NOTES actually licenses: for each counterparty `c`, use `c`'s type (and the anchor's) to read the edge — `token_parent`-linkage from a *literal* `c` = composition (reverse-adjacency); `token_parent`-linkage from a *label* `c` = sub-label membership (member); `token_sibling_group`-linkage = literal membership (member). Cite `NOTES.md` 160–163 + 308–311.

### N2 — MAJOR — write-side symmetric gap: membership authoring reduces to `token_sibling_group`, dropping the sub-label→`token_parent` form NOTES requires; the member-side table-selection lens and a post-mint `token_parent`-membership door method are missing and unflagged

- **Location:** §II.3, lines 249–251 — *"`MEMBER_OF` … writes the member side via `add_group_membership` **and its reciprocal** (label CHILDREN) via the G3 `token_child` front-door write."* §II.6, lines 335–336 — *"`ADD_CONNECTION` — pairwise add via `add_group_membership` (member side) + the reciprocal front-door write into `token_child` (G3)."* §I.D ADD_CONNECTION, lines 156–158.
- **Precept contradicted:** `NOTES.md` 308–312 (same as N1): a sub-label member joins its super-group by a **PARENT** edge (`token_parent`), not a SIBLING edge (`token_sibling_group`). `add_group_membership` writes **only** `token_sibling_group` (`controller.h:138`, `controller/README.md` line 75–78). NOTES lines 411–423 (grouping-by-nesting) and the label-vs-literal mirror (466–471) make the sub-label tier (`hex value tables` gathering `single hex codes`) a firmed **record-tier** construction, i.e. squarely in scope.
- **Why it is a defect:** Building the firmed running lattice (hex digits → `single hex codes` → `hex value tables`) *requires* authoring a sub-label's membership in a super-group, which per NOTES is a `token_parent` edge on the sub-label plus the `token_child` reciprocal. The plan authors every membership through `add_group_membership` (`token_sibling_group`) only, so the sub-label tier cannot be written correctly, and DELETE_CONNECTION's "remove the reciprocal" (I.D 162–164) inherits the same omission. Worse, there is a **door gap the plan does not flag**: adding a `token_parent` membership edge to an *existing* token post-mint has no controller method — `mint` writes `token_parent` only at creation of a new composite, and `add_group_membership` writes `token_sibling_group`. This is exactly the class of gap the plan *did* flag for G3/G9; here it is silent. The plan neither designs the member-side table selection nor flags it as a seam — it silently reduces two firmed forms to one.
- **Narrowest correction:** State that membership authoring selects the member-side table by the endpoint-type lens — literal member → `token_sibling_group` (via `add_group_membership`); sub-label member → `token_parent` (its PARENT/super-group edge) — with the `token_child` reciprocal in both cases; and surface the required door method (write, and delete for DELETE_CONNECTION, of a `token_parent` membership edge on an existing token) as a new door capability under G9 or a fresh seam, assigning it to Agent 2/3/5. If Patrick's "all connections are pairwise / no connection-kind operand" (NOTES 156–163) is meant to *unify* membership into a single table (overriding 308–311), that is a CONFLICT between two firmed NOTES passages — per the operating rule the plan must **stop and confirm with Patrick**, not silently pick one.

> **Root cause note (N1+N2).** Both trace to one firmed precept the revision's F3
> fix did not fold in: `NOTES.md` 308–311 makes `token_parent` carry two readings
> (literal constituents; sub-label super-group membership), exactly as F3
> recognized for `token_child`. The correct lens is by *endpoint type* (NOTES 160),
> which resolves both tables uniformly. Fixing N1/N2 is one change: read/write the
> membership reading from endpoint types, across both overloaded tables.

### N3 — MINOR — `token_parent.mass` doc/README not updated alongside the G2 nullability change (F11 covers only `token_child`)

- **Location:** §II.3 G2/G8 box, lines 273–275 (schema column → nullable, door → optional) and the F11 doc-consequence note (lines 311–313) which lists only `token_child.child_token_id`.
- **Spec:** `schema/README.md` line 52 documents `token_parent.mass` as *"`integer NOT NULL`, stored as given, never computed"*; `schema.sql:131` `mass integer NOT NULL` with the comment "stores whatever mass is given". G2 makes the column nullable (blank at declare). `controller.h:44` `int mass = 0` comment likewise implies always-present.
- **Why it is a defect:** The plan folds the F11-style doc consequence for `token_child` but not the parallel one for `token_parent.mass`/`Constituent.mass`: the schema README (line 52), the `schema.sql` column comment, and the `controller.h` `Constituent` comment will contradict the new nullable/optional semantics. Same class as F11, left unflagged.
- **Narrowest correction:** Extend the G2/G8 doc consequence to note `schema/README.md` line 52, the `schema.sql:131` comment, and the `controller.h:42–45` comment update to reflect nullable/optional per-element mass (NULL = not-yet-computed).

### N4 — OBSERVATION — "pending list, drained by RECONCILE" is imprecise

- **Location:** §II.3 F5/F12 box, line 262 — *"the file-now / wire-later deferral (pending list, drained by RECONCILE)"*.
- **Spec:** `NOTES.md` lines 127–129: the pending list drains **asynchronously on its own**, "behind the primary input/output request handling." RECONCILE (lines 600–606) is the *on-demand, prioritized* forcing of that drain, "generally because the result will impact current analysis and cannot wait for the pending list to drain on its own." So RECONCILE is not the drainer; it is the demand-forced expedite.
- **Why minor:** Wording only; does not change the (correct) synchronous-now decision. Correction: "pending list (drains asynchronously; RECONCILE forces an immediate, prioritized drain)."

### N5 — OBSERVATION — G10 "does not block the in-process cores" slightly overstates DELETE_RECORD's completeness

- **Location:** Part III line 406–410 and Part V G10 — *"none blocks the in-process cores"*; G10 flags the FK-dependent policy open.
- **Spec:** `schema.sql` FKs (`token_parent.parent_token_id`, `token_sibling_group.group_token_id`, `token_child.child_token_id` all `REFERENCES token`). A row-delete of an FK-referenced token *fails at the DB* until a block/cascade/repoint policy is chosen (G10).
- **Why observation:** The primitive + gate are buildable now (correct), but `DELETE_RECORD` is not fully functional for referenced tokens until G10 is decided — "does not block the cores" is true only for the unreferenced case. The plan already flags G10 as needing Patrick, so this is a precision note, not a new gap. Correction: "the local gate + primitive are buildable now; DELETE_RECORD is functional for unreferenced tokens, its referenced-token behaviour pending G10."

### N6 — OBSERVATION — MOVE re-key rides plain FKs (no `ON UPDATE CASCADE`); the G9 primitive must own FK-safe ordering

- **Location:** §II.6 / G9 box, lines 331–334, 350–352 (re-key + cascade-repoint).
- **Spec:** `schema.sql` FKs are plain `REFERENCES token (token_id)` with no `ON UPDATE CASCADE`; the schema forbids triggers/functions (`schema/README.md` line 5–6). A PK (`token_id`) re-key therefore cannot lean on DB cascade — the door primitive must repoint every referencing row itself, in one transaction, in FK-safe order (insert-new / repoint / delete-old, or deferred constraints).
- **Why observation:** The plan correctly names the primitive as needed (G9); it does not state the cascade must be hand-ordered because the schema has no `ON UPDATE CASCADE`. Design-level note. Correction: one line under G9 that the re-key primitive owns FK-safe repoint ordering (no DB-level cascade exists).

### N7 — OBSERVATION — the "work balancing is foundational, not deferred" precept is stronger than the F5/F12 authorization it is leaned on to defer

- **Location:** §II.3 F5/F12 box, lines 258–262; Part V F5/F12.
- **Spec:** `NOTES.md` lines 119–120: *"Work balancing is **foundational, not deferred** — the temporary Python loader is real I/O and must be handled right from the base."* The F5/F12 authorization is specifically about reciprocal write *timing* (synchronous vs async).
- **Why observation:** Deferring reciprocal *timing* to synchronous is faithful (mint already wires synchronously — `controller.h:123–126`), and the plan states it consciously. But the plan leans on that ruling to defer the *whole* file-now/wire-later + pending-list architecture to "the out-of-scope runtime/cache layer," which reads slightly broader than the timing ruling and brushes a precept explicitly marked "not deferred." This is Patrick's to confirm; flagging so it is a conscious scope call, not an inferred one. Correction: note that synchronous timing is the authorized record-tier baseline and confirm the pending-list architecture's deferral is within scope.

---

## Round-1 findings F1–F13 — verification

- **F1 (MAJOR — MOVE/DELETE door primitives missing/unflagged) — FIXED-CORRECTLY.** G9 box (lines 340–354) + Part V G9 authorize re-key/cascade-repoint and row-delete on all four tables; Agent 5 depends on the addition (Part III table + note); the "buildable now" claim is corrected to "local ops + gates + primitives now, swarm verification later" (line 353). Verified the door genuinely lacks them: `controller.h` exposes only `mint` and `add_group_membership`. No over-reach (MOVE not collapsed into DELETE gating; DELETE not made arrayable). *Caveat:* the door-gap enumeration omits the `token_parent`-membership write/delete method (N2) — a distinct gap from F1's re-key/row-delete, still unflagged.
- **F2 (MAJOR — `Constituent.mass` optional / `mint` LINK writes NULL) — FIXED-CORRECTLY.** G2/G8 box lines 273–276 adds the door consequence explicitly and assigns it to Agent 2. Ground-truthed: `Constituent.mass` is `int = 0` (`controller.h:42–45`), `token_parent.mass` is `NOT NULL` (`schema.sql:131`), `token.mass` already nullable (`schema.sql:87`). Correctly targets per-element `Constituent.mass` (not the already-optional token-level `mint(... mass)` param). Seed-floor exception preserved (lines 130, 277–278).
- **F3 (MAJOR — READ members vs reverse-adjacency lens) — FIXED-BUT-IMPERFECT.** The lens is added (G3 box 300–308, §II.5 322–326) and is correct and only-follow **for literal members** (SIBLING via `token_sibling_group`). It is **wrong for sub-label members** (PARENT via `token_parent`), which the lens mis-reads as composition — see **N1**. The intent is right; the implementation of the lens (by table, not by endpoint type) is under-specified against NOTES 308–311 / 160–163.
- **F4 (MINOR — Agent 4 depends on Agent 3) — FIXED-CORRECTLY.** Part III table: Agent 4 depends "1, **3** (label-axis fixtures/rows, F4)."
- **F5 (MINOR — MEMBER_OF reciprocal timing/dependency) — FIXED-CORRECTLY.** F5/F12 box declares reciprocals synchronous; Agent 2 depends on Agent 3 for the G3 method; the dependency note (402–405) justifies the 3-before-2 ordering and names the rejected async alternative. (Scope of the reciprocal still incomplete for sub-labels — N2 — but the *timing/dependency* question F5 raised is resolved.)
- **F6 (MINOR — DELETE_CONNECTION reciprocal removal) — FIXED-CORRECTLY.** §I.D lines 162–164: removes the named edge "and its stored reciprocal (both-direction removal), mirroring ADD_CONNECTION." (Same sub-label table caveat as N2 for which reciprocal table.)
- **F7 (MINOR — DELETE_RECORD FK-dependent policy) — FIXED-CORRECTLY.** Gap G10 (361–366, Part V) flags the FK-dependent behaviour as unspecified in NOTES and explicitly does **not** invent a cascade rule ("inventing a cascade rule would breach 'no invented complication'"). Correctly flagged, not resolved.
- **F8 (MINOR — codec lacks address-successor) — FIXED-CORRECTLY.** §II.2 Note F8 (238–243) adds a base-50 successor helper over codec primitives as code-structure latitude, and ties depth-expansion to the deferred trunk map (G5). Ground-truthed: `codec/README.md` public interface has couplet↔code, address↔token_id, delta/reconstruct — no successor/increment.
- **F9 (OBS — READ direction flattened) — FIXED-CORRECTLY.** §I.C 137–139: reverse is "selectable as an *orientation* on those axes (not a peer fourth value) — per NOTES 'reverse orientation likewise selectable'."
- **F10 (OBS — READ/DECLARE mislabelled additive) — FIXED-CORRECTLY.** §I.F 191–194: "arrayable record-tier ops are `DECLARE`, `READ`, and the *additive* UPDATE sub-ops … (READ is arrayable but is not itself an 'additive op')."
- **F11 (OBS — `token_child` doc broadens under G3) — FIXED-CORRECTLY.** G3 box 311–313 notes `token_child.child_token_id` semantics broaden to "child by either reading" and that `schema.sql`/`controller.h` comments are to be updated. (The parallel `token_parent.mass` doc update is missed — N3.)
- **F12 (OBS — file-now/wire-later engaged only for mass) — FIXED-CORRECTLY** (with N4/N7 precision notes). The plan now states synchronous writes consciously and assigns the async deferral to the out-of-scope layer.
- **F13 (OBS — mass wording reads as resolving sum-vs-centroid) — FIXED-CORRECTLY.** §I.B 127–128: "the unresolved sum-vs-centroid question is NOT settled here … the record tier declares no mass, so it does not depend on the resolution."

No round-1 finding is NOT-FIXED or REGRESSED. F3's fix is the only one imperfect, and its imperfection is a genuine new completeness gap (N1) rather than a botched application.

---

## Human-authorized resolutions — faithfulness check

- **G1 (programmatic arrangement = code-structure latitude) — FAITHFUL, minimal.** I.A box (94–100) + Part V confine latitude to layout/dispatch attachment and explicitly hold the four ops' grammar/behaviour spec-bound. No semantic decision smuggled in.
- **G2/G8 (mass blank on declare) — FAITHFUL, spec-consistent.** Consequences verified against ground truth (see F2). Seed-floor exception preserved; the deferred sum-vs-centroid model is *not* resolved (F13). One doc consequence missed (N3).
- **G3 (extrapolate inverse path into `token_child`; supersede schema OPEN TEST SLOT 4) — FAITHFUL where it goes, INCOMPLETE.** Justified by `NOTES.md` 223–227 ("token_child two write sources by type — RESOLVED") and 303–312, not plan invention; the supersession of schema OPEN TEST SLOT 4 (`schema/README.md` 79–83, which anticipated "its own explicit stored structure … analogous to `token_child`") is legitimate — NOTES resolves it by reusing `token_child` itself. But the fold stops at the literal-member case; the sub-label case (N1/N2) is dropped.
- **F3 fix (per-counterparty type lens) — see N1: correct for literal members, wrong for sub-label members.** It IS only-follow (direct `parents_of(c)`/`groups_of(c)` reads on each counterparty, no search) and is not otherwise invented machinery — but it is licensed by the *endpoint-type* lens (NOTES 160), which the plan collapsed to a "which table" test that the overloaded `token_parent` defeats.
- **F5/F12 (synchronous reciprocals) — FAITHFUL, defensible.** Matches `mint`'s current synchronous `token_child` wiring; stated consciously; async deferral placed with the out-of-scope layer. See N4/N7 for two precision notes; neither contradicts the ruling.
- **G9 (MOVE/DELETE mutation permitted) — FAITHFUL, minimal, spec-consistent.** Verified: door lacks re-key/cascade/row-delete today; MOVE retains its ranged/arrayable form (I.D 165–166, I.F 190–197, II.6 334) and is not collapsed into DELETE's single-only gating; primitives attributed to Agent 5 with corrected dependencies. MOVE's mutation scoped as reassignment (topology-invariant); DELETE's as individual/specific/confirmed/peer-validated. See N6 (FK-ordering note).

**Dependency table — acyclicity re-checked (task-flagged).** Edges: 1←(baseline); 3←1; 2←{1,3}; 4←{1,3}; 5←{1,2,3}; 6←{2,3,4,5}. Agent 2→3 is new; 3 depends only on 1, so 2→3→1 introduces **no cycle**. A valid topological order exists: **1, 3, 2, 4, 5, 6**. The table is consistent and acyclic. The reordering note (402–405) is sound. No agent depends on an unbuilt capability *within the table*, but Agents 2/3/4/5 all inherit the N1/N2 gap (member-side/table lens + `token_parent`-membership door method), so several "buildable now" claims are qualified by that unaddressed capability.

---

## Completeness walk — firmed record-tier precepts

**Literal intake**
- PARENTS sets N — **COVERED** (I.B, II.1, II.3).
- `>=2` anti-alias floor — **COVERED** (I.B, II.3).
- ADDRESS span alphabet (`FROM`/`AFTER`/`TO`/direct/`@`/undeclared-hook) — **COVERED** (I.E; matches NOTES 383–390).
- cover-N + elastic-last-only + interior self-delimiting + nested-declare-as-one-slot — **COVERED** (I.E, II.2).
- NOTATION positional-partial, blanks legal, reads blank until derivation — **COVERED** (I.B, II.3).
- MEMBER_OF two-section split (broadcast `|` sparse per-member) — **COVERED at face and now restated at core** (I.B; II.3 249–251 restates both sections — round-1 F5-partial improved); **PARTIAL on the member-side table**: reduces to `add_group_membership` (`token_sibling_group`) only, dropping the sub-label→`token_parent` form (**N2**).

**Label dual**
- CHILDREN floor `>=1`; `>=2` does not transfer — **COVERED** (I.B, II.4).
- label has no own address; placed via naming literal — **COVERED** (II.4).
- naming-literal identity; crude prose construction-post; membership keyed on members referencing group by naming-literal token_id — **COVERED** (II.4).
- label-vs-literal mirror — **COVERED** (II.4 / Part IV).
- label's own PARENTS = super-groups (the sub-label's `token_parent` edge) — **MISSING/UNADDRESSED**: neither DECLARE core (II.3/II.4) states how a label's super-group membership is written (it is a `token_parent` edge per NOTES 200–201, 310–311), and the read of that axis is mis-lensed (**N1, N2**).

**READ**
- anchor / radial-default / depth-LoD / named-branch-exclusions-only / linearized once-per-path no dedup — **COVERED** (I.C, II.5).
- direction, reverse-as-orientation — **COVERED** (I.C, F9 fix).
- members vs reverse-adjacency separation over the shared reverse tables — **PARTIAL/INCORRECT**: correct for `token_child` literal members, wrong for sub-label members and silent on the `token_parent` dual reading (**N1**).

**Four UPDATE ops**
- MOVE = bookkeeping / topology-invariant / re-key+cascade / ranged-arrayable — **COVERED**; door primitive now flagged (G9); FK-ordering note (N6).
- ADD_CONNECTION = label-leads / pairwise / no-kind-operand / endpoints-pre-exist / reciprocal / arrayable — **COVERED at face**; member-side table reduced to one form (**N2**).
- DELETE_RECORD / DELETE_CONNECTION = single-only / never-arrayed / active-confirmation + peer-validation tags / specific-id-no-wildcard / reciprocal removal — **COVERED**; door row-delete flagged (G9); FK-dependent policy flagged (G10); reciprocal removal inherits the N2 table gap.

**Cross-cutting**
- Arraying universal (serial ordered; additive arrayable; destructive not; cache excluded) — **COVERED** (I.F, II.7; F10 fix).
- Only-follow / never-search — **COVERED** and correctly leaned on (the N1 lens is only-follow even when corrected to endpoint-type).
- Address-is-identity / no aliasing — **COVERED**.
- No invented complication — **COVERED**; the plan is disciplined about not inventing cascade rules (G10) or resolving deferred items.

**Nothing silently over-resolved.** G4, G5, G6, G7, G10 remain open (Part V). Mass sum-vs-centroid, nested aggregation, label-vs-naming-literal mass slot, notation-derivation, prose→token_id swap, external transport, next-slot, block boundaries — all correctly deferred. Cache-tier core (RECONCILE/UPDATE_CACHE/REBASE_CACHE) remains named stubs, undesigned. The G3 supersession of schema OPEN TEST SLOT 4 is NOTES-justified, not a plan-invented closure.

---

## Verdict

**PASS-WITH-FIXES.**

The revision folds all thirteen round-1 findings faithfully and applies the five
authorized resolutions minimally and (where groundable) spec-consistently, without
inventing semantics, importing machinery, or resolving still-deferred items. The
dependency graph is acyclic and runnable.

It is **not ready as written** because the F3 fix — the one substantive new
design element in this revision — did not account for a firmed precept
(`NOTES.md` 308–311): a sub-label's membership is a `token_parent` edge, so
`token_parent` is overloaded exactly as `token_child` is. The read lens
mis-classifies sub-label members and the write path drops them, and a required
`token_parent`-membership door method is unflagged.

**Must-fix before implementation:**
- **N1** — correct the type lens to read by *endpoint type* (NOTES 160), resolving both overloaded reverse tables; a `token_parent` link from a *label* counterparty is membership, not composition.
- **N2** — design (or flag as a Patrick seam) the member-side table selection (literal member → `token_sibling_group`; sub-label member → `token_parent`) and surface the missing post-mint `token_parent`-membership write/delete door method; if the "all connections pairwise" precept is intended to unify these into one table, that is a NOTES-internal conflict to stop-and-confirm, not to resolve silently.

**Should-fix:** N3 (`token_parent.mass` doc/README parallel to F11).

**Optional:** N4 (RECONCILE-vs-async wording), N5 (G10/DELETE completeness phrasing), N6 (MOVE FK-ordering note), N7 (scope of the F5/F12 deferral vs "work balancing is foundational").
