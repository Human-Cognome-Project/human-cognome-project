# Adversarial review — PLAN.md against NOTES.md + subdir specs

Date: 2026-09-16. Reviewer: fresh adversary, no prior stake. Target:
`/opt/project/repo/engine/db_kernel/PLAN.md`. Governing spec read in full:
`NOTES.md`, `schema/README.md`, `schema/schema.sql` (ground-truth), `controller/README.md`,
`controller/controller.h`, `ingestion/README.md`, `codec/README.md`.

---

## Findings (itemized)

### F1 — MAJOR — MOVE/DELETE cores silently require door mutation primitives the controller does not have

- **Location:** Part II §II.6 — *"`MOVE_RECORD` — re-key token_id + cascade-repoint every referencing edge …"* and *"`DELETE_RECORD` / `DELETE_CONNECTION` — local active-confirmation gate, tag-carrying shape, specific-id-only …"*; and Part I §II baseline line 61–62 (*"except where a core function needs a door/schema capability the baseline does not provide (surfaced in Part V)"*).
- **Precept/spec contradicted:** `controller.h` exposes exactly two write methods — `mint(...)` and `add_group_membership(...)`. There is **no UPDATE, no DELETE, no re-key** primitive anywhere in the door. `controller/README.md` confirms the write side is "see-mint-link-wire" only. NOTES §UPDATE requires MOVE to "re-key … and cascade-repoint every edge" and DELETE to "remove a whole token" / "remove one specific pair."
- **Why it is a defect:** Three of the four in-scope UPDATE ops (MOVE_RECORD, DELETE_RECORD, DELETE_CONNECTION) cannot be built over the existing door — they need SQL UPDATE/DELETE primitives that do not exist. The plan itself enumerates the door's write side accurately in Part 1 (only `mint`, `add_group_membership`), and it *does* flag one needed door addition (G3, the token_child front-door write), which makes the omission of the delete/update/re-key primitives a genuine unflagged gap, not an oversight excusable as latitude. Part III then claims "Agents 5 and 6 build their full local behaviour now," which is false for Agent 5 as written.
- **Narrowest correction:** Add a Part V seam (or an explicit "new door capability" note under II.6, parallel to G3) stating that MOVE requires a re-key/cascade-repoint write primitive and the DELETE pair requires row-delete primitives on `token`/`token_parent`/`token_sibling_group`/`token_child`, none of which the controller currently provides; make Agent 5 depend on that door addition.

### F2 — MAJOR — G2's "nullable" consequence is incomplete: the controller `Constituent`/`mint` LINK path is not covered

- **Location:** §II.3 / G2·G8 box — *"`token_parent.mass` is `NOT NULL` today — it must … become **nullable** … Agent 2 carries this schema adjustment and its test."*
- **Precept/spec:** Ground-truthed — `schema.sql:131` `token_parent.mass integer NOT NULL` (plan's premise is correct; not over-reach), and `token.mass` is already nullable (`schema.sql:87`). BUT `controller.h:42-45` `struct Constituent { codec::Address address; int mass = 0; }` is a **non-optional** int, and `mint`'s LINK step (`controller.h:121-123`) "insert token_parent rows … each with its ordinal and per-element mass" writes that int. NOTES §DECLARE/Mass requires no mass at intake and NOTES/schema require NULL ("not yet computed") kept distinct from a real 0.
- **Why it is a defect:** Making the column nullable is necessary but not sufficient. To write blank mass, the door's `Constituent.mass` must become `std::optional<int>` (or `mint` must gain a no-mass LINK mode) so the LINK inserts SQL NULL rather than `0`. Writing `0` would violate the NOTES/schema "0 ≠ not-yet-computed" rule. The plan presents G2 as stating the "direct" structural consequence but stops at the schema column.
- **Narrowest correction:** Extend G2 to add the door consequence: `Constituent.mass` → optional and `mint` LINK writes NULL for an absent per-element mass; assign it to Agent 2 alongside the schema change.

### F3 — MAJOR — READ's "members" axis and label CHILDREN conflate with literal reverse-adjacency in `token_child`; the plan glosses the required type-lens disambiguation

- **Location:** §II.5 — *"The label-`CHILDREN` axis follows `token_child` (G3, resolved)"*; G3 box — *"label `CHILDREN` reads then follow `token_child` exactly as literal children do."*
- **Precept/spec:** NOTES "Why the label MUST exist as a literal" + Addressing rule: a label shares **one token_id with its naming literal** ("borrowed by identity … the same token_id the LoD rollup rides"). NOTES "All connections are pairwise": edges carry **no connection-kind operand**; the reading is "lens-derived (endpoints' types + direction give the reading)." `schema.sql:204-213` / `controller.h:97`: `token_child(token_id, child_token_id)` stores no kind/type discriminator; `children_of` returns every row keyed on the id.
- **Why it is a defect:** For a naming-literal token_id (the normal case for any label), `children_of(id)` returns **both** its literal-composition reverse edges (composites that use it as a constituent — NOTES/READ "reverse orientation") **and** its label members (NOTES/READ "members" axis), with no stored way to tell them apart. READ's firmed direction narrowing (parents / members / both / reverse) therefore cannot separate the "members" axis from the "reverse-adjacency" axis by following `token_child` alone — it needs the per-counterparty type lens (e.g. does `child` list `id` via `token_parent` vs via `token_sibling_group`). "Exactly as literal children do" understates this and leaves a core, in-scope READ behaviour undesigned.
- **Narrowest correction:** In §II.5 note that the "members" vs "reverse-adjacency" narrowing over `token_child` is resolved by the endpoint type-lens (per counterparty), not by the table alone; keep it inside READ's spec-bound behaviour rather than asserting the follow is identical.

### F4 — MINOR — Agent 4 (READ) dependency omits Agent 3

- **Location:** Part III table — `| 4 | II.5 READ raw radial | 1 | — |`.
- **Spec/plan:** §II.5 itself ties the label-CHILDREN axis to G3, which Agent 3 builds. Project rule "tests on everything."
- **Why it is a defect:** Agent 4's read code compiles over baseline `children_of`, but exercising/testing the label-CHILDREN axis needs label-containment rows, which only the G3 front-door write (Agent 3) produces. As listed, Agent 4 depends on Agent 1 only, so its label-axis test cannot run in dependency order without hand-forged fixtures.
- **Narrowest correction:** Add Agent 3 (or the G3 door method) as an Agent 4 dependency for the label-axis test, or state that Agent 4's label-axis test uses fixtures produced by Agent 3.

### F5 — MINOR — MEMBER_OF's reciprocal (label CHILDREN into `token_child`) is authored by Agent 2's op but built by Agent 3

- **Location:** §II.3 — *"`MEMBER_OF` → `add_group_membership`"* (Agent 2); G3 front-door write (Agent 3).
- **Spec:** NOTES literal-intake MEMBER_OF: "Member-side of the sibling-group edge; the reciprocal (label CHILDREN) is manager-maintained." NOTES "Relationships stored BOTH directions."
- **Why it is a defect:** `add_group_membership` writes only `token_sibling_group` (member side). The reciprocal label-containment row in `token_child` is the G3 method Agent 3 builds. The plan does not state whether MEMBER_OF's reciprocal is written synchronously (needing Agent 3's method inside Agent 2's op) or deferred to the pending-work list. As written, Agent 2 (depends on 1 only) authors an edge whose reciprocal it has no method for.
- **Narrowest correction:** State that the MEMBER_OF reciprocal is the same G3 front-door write, and either add the Agent 2→3 dependency or explicitly route the reciprocal to pending-work (consistent with the file-now/wire-later precept).

### F6 — MINOR — DELETE_CONNECTION reciprocal removal not stated

- **Location:** §I.D / §II.6 — *"`DELETE_CONNECTION` — remove one specific pair."*
- **Spec:** NOTES "Relationships stored BOTH directions … the reciprocal is maintained"; ADD_CONNECTION "Reciprocal maintained." A stored edge exists in both directions (`token_sibling_group` + `token_child`, or `token_parent` + `token_child`).
- **Why it is a defect:** If ADD maintains both directions, DELETE_CONNECTION must remove both, or the reverse index is left dangling. The plan says "one specific pair" without noting the reciprocal edge must also be removed.
- **Narrowest correction:** Add that DELETE_CONNECTION removes the named edge and its stored reciprocal (both-direction removal), mirroring ADD_CONNECTION.

### F7 — MINOR — DELETE_RECORD referential-integrity policy on dependents is unspecified and unflagged

- **Location:** §I.D / §II.6 — *"`DELETE_RECORD` — remove a whole token."*
- **Spec:** `schema.sql` FKs: `token_parent.parent_token_id`, `token_sibling_group.group_token_id`, `token_child.child_token_id` all `REFERENCES token`. NOTES specifies only the two gates and "removes a whole token," nothing about dependents. NOTES "no invented complication."
- **Why it is a defect:** Deleting a token that other composites reference as a constituent/group will hit an FK; the plan neither defines a policy (block / cascade / repoint) nor flags it as an open seam — while it does flag the peer-validation transport (G7). This is a real seam glossed. (Correctly, the plan should *not* invent a cascade rule — it should flag that dependent-handling is unspecified in NOTES.)
- **Narrowest correction:** Add a surfaced gap: DELETE_RECORD's behaviour when the token is still referenced (FK dependents) is unspecified in NOTES; flag rather than resolve.

### F8 — MINOR — span planner needs an address-successor capability the codec does not expose

- **Location:** §II.2 — *"`plan(span, N) → [address per slot] + validity`. … Uses the codec. Fill = analyst-supplied-start form."*
- **Spec:** `codec/README.md` / codec public interface exposes couplet↔code, address↔token_id, delta/reconstruct — **no** whole-address increment / sequential-successor / trunk-deepening function. NOTES "sequential, contiguous addresses … deterministic … sequential placement."
- **Why it is a defect:** Sequential cover-N fill from a supplied start requires an address-increment primitive (base-50 successor, with depth expansion) that the codec does not provide. "Uses the codec" implies the capability exists; it does not, and the trunk-deepening extent touches G5.
- **Narrowest correction:** Note that the planner adds a sequential-successor helper over the codec primitives (code-structure latitude), and that depth-expansion beyond a single ring leans on the deferred trunk map (G5).

### F9 — OBSERVATION — READ direction flattened

- **Location:** §I.C — *"Direction … a narrowing to parents / members / both / reverse."*
- **Spec:** NOTES READ: "parents / members / both (the per-type downward reads), reverse orientation likewise selectable" — reverse is an *orientation modifier* on the axes, not a peer fourth value.
- **Why minor:** Slight shape shift; does not change scope. Correction: phrase reverse as an orientation applicable to the axes.

### F10 — OBSERVATION — READ (and DECLARE) mislabelled "additive ops"

- **Location:** §I.F — *"**Additive** ops are arrayable (`DECLARE`, `READ`, `MOVE_RECORD`, `ADD_CONNECTION`)."*
- **Spec:** NOTES "Arraying is universal": "every record-tier command — DECLARE, READ, and the *additive* UPDATE sub-ops (MOVE_RECORD, ADD_CONNECTION)". NOTES lists READ alongside, not as an additive op.
- **Why minor:** Terminology only; the arrayable set is correct. Correction: "the arrayable record-tier ops (DECLARE, READ, and the additive UPDATE sub-ops MOVE_RECORD, ADD_CONNECTION)."

### F11 — OBSERVATION — `token_child.child_token_id` documentation no longer matches its dual use under G3

- **Location:** G3 box (consequence unflagged).
- **Spec:** `schema.sql:210` / `controller.h:97` document `child_token_id` as "a token that directly lists token_id as one of its parents" — the literal-composition reading only. G3 makes `token_child` also carry label containment (member not a parent).
- **Why minor:** Doc-consistency consequence of the authorized G3 fold; the schema/controller comments understate the column. Correction: note that the `token_child` column semantics broaden to "child by either reading (composition or containment)."

### F12 — OBSERVATION — the "file-now / wire-later" foundational precept is engaged only for mass

- **Location:** §I.B / §II.3 use the pending-work list for mass; the G3 `token_child` writes are treated as synchronous throughout.
- **Spec:** NOTES "Process runtime … Work balancing is foundational, not deferred": the `token_child` reverse adjacency is deferred cross-processing on the pending list, eventually-consistent; RECONCILE (cache tier, out of scope) forces the drain.
- **Why observation, not a contradiction:** Synchronous writes are a defensible record-tier choice (mint already wires synchronously), with the async deferral legitimately owned by the out-of-scope RECONCILE/runtime layer. The defect is only that the plan does not *say* it is building synchronous writes and consciously leaving the (firmed-foundational) work-balancing deferral to the runtime/RECONCILE layer. Correction: one line stating this.

### F13 — OBSERVATION — mass-model wording should be read as naming pending work, not resolving the deferred sum-vs-centroid question

- **Location:** §I.B — *"a literal's mass = sum of parents' stored masses …; a label's = the maintained centroid."*
- **Spec:** NOTES Handoff flags the mass model deferred, including "sum vs centroid for label mass (the notes use 'centroid' for what they example as a sum)."
- **Why observation:** The plan uses NOTES' own term ("centroid") and attributes the computation to the pending workstream, so it does not actually resolve the ambiguity — but the wording is close enough to read as a resolution. Correction: mark it explicitly as "per the deferred aggregation model (unresolved sum-vs-centroid)."

---

## Completeness interrogation — firmed-precept checklist

**Literal intake**
- PARENTS sets N — **COVERED** (I.B, II.1, II.3).
- `>=2` anti-alias floor — **COVERED** (I.B, II.3; "one constituent resolves to that constituent's own address" transcribed correctly).
- ADDRESS alphabet FROM/AFTER/TO/direct/@/undeclared-hook — **COVERED** (I.E; matches NOTES §"The literal intake formula" in substance).
- cover-N + elastic-last-only + interior self-delimiting + nested-declare-as-one-slot — **COVERED** (I.E, II.2).
- NOTATION positional-partial, blanks legal, reads blank until derivation — **COVERED** (I.B, II.3).
- MEMBER_OF two sections split by `|` (broadcast | sparse per-member) — **COVERED at face** (I.B); **PARTIAL at core** (II.3 reduces it to "→ add_group_membership" without restating the section-2 sparse fold — see F5 for the reciprocal seam).

**Label dual**
- CHILDREN non-empty floor `>=1`; `>=2` does not transfer — **COVERED** (I.B, II.4).
- label has no own address; placed via naming literal — **COVERED** (II.4).
- naming-literal identity; crude prose construction-post; membership keyed on members referencing group by naming-literal token_id — **COVERED** (II.4).
- label-vs-literal mirror — **COVERED** (cited as spec anchor in II.4 / Part IV).

**READ**
- anchor / radial-default / depth-LoD / named-branch exclusions only / linearized once-per-path no dedup — **COVERED** (I.C, II.5), except the members-axis conflation is glossed (**F3**); direction shape slightly flattened (**F9**).

**Four UPDATE ops**
- MOVE = bookkeeping / topology-invariant / re-key+cascade — **COVERED at face; door primitive MISSING/unflagged (F1)**.
- ADD_CONNECTION = label-leads / pairwise / no-kind-operand / endpoints-pre-exist / reciprocal — **COVERED** (I.D, II.6).
- DELETE pair = single-only / never-arrayed / active-confirmation + peer-validation tags / specific-id-no-wildcard — **COVERED at face**; door delete primitive **MISSING/unflagged (F1)**; reciprocal removal (F6) and dependent-FK policy (F7) unstated.

**Cross-cutting**
- Arraying universal (serial ordered; additive arrayable; destructive not; cache excluded) — **COVERED** (I.F, II.7); minor label slip (F10).
- Only-follow / never-search — **COVERED** and correctly leaned on (F1's cascade is only-follow-able via `parents_of`/`children_of`/`groups_of`).
- Address-is-identity / no aliasing — **COVERED**.
- No invented complication — **COVERED**; the plan is disciplined about not resolving deferred items.

**Authorized folded exceptions**
- **G1** (programmatic arrangement as latitude) — applied faithfully; no semantics smuggled in.
- **G2/G8** (mass blank; `token_parent.mass` nullable; seed-floor exception preserved) — premise verified against `schema.sql:131` (NOT NULL today) and the seed exception is preserved (I.B, II.3); but the consequence is **incomplete** (F2, door side).
- **G3** (inverse path front-door into `token_child`; supersedes schema OPEN TEST SLOT 4) — verified against NOTES "token_child two write sources by type — RESOLVED (2026-09-15)" and "Relationships stored BOTH directions"; the supersession of SLOT 4 is justified by NOTES, not plan invention; but the read-side conflation it enables is glossed (F3) and its doc consequence unflagged (F11).

**Nothing else silently resolved:** mass sum-vs-centroid, nested aggregation, label-vs-naming-literal mass slot, notation-derivation, prose→token_id swap, next-slot (G4), block boundaries (G5), transport (G6), delete peer-validation transport (G7), identity-vs-relocation prose — all correctly left deferred. No out-of-scope cache-tier core mechanics were designed (RECONCILE/UPDATE_CACHE/REBASE_CACHE remain named stubs).

---

## Verdict

**PASS-WITH-FIXES.**

The plan holds to absolute precision to precepts across the great majority of its stated scope: the face grammar, the literal formula and its label dual, READ, arraying, and the hard constraints are transcribed faithfully; the four authorized exceptions are applied without over-extension; and the deferred/open items are correctly held open rather than resolved. It does not invent semantics or import unrequested machinery.

It is **not ready as written** because it under-specifies real door/schema seams that in-scope core functions depend on.

**Must-fix before implementation:**
- **F1** — surface the missing controller mutation primitives (re-key/cascade for MOVE; row-delete for the DELETE pair) as a required door capability, and correct Agent 5's "buildable now" claim/dependencies.
- **F2** — complete G2 with the door-side consequence (`Constituent.mass` optional / `mint` LINK writes NULL), not just the schema column.
- **F3** — design (or explicitly bind to the type-lens precept) how READ's "members" axis and label CHILDREN separate from literal reverse-adjacency in the shared `token_child`.

**Should-fix:** F4–F8 (delegation/dependency and reciprocal/FK/codec-successor seams). **Optional:** F9–F13 (wording/documentation precision).
