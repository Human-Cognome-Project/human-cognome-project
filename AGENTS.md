# Agent guidance

This file is the operational entry point for AI coding/review agents working on HCP.

## Current state

Last checked: **2026-10-17**.

This is the repository's onboarding status snapshot. Code and executable tests
determine what is built; accepted decisions govern intent. Check the
[open PRs](https://github.com/Human-Cognome-Project/human-cognome-project/pulls)
and their latest review comments before starting work. Update this section when
a merge or review changes these statuses; other entry points link here.

| State | Scope and evidence |
|---|---|
| Built | Native C++ engine and bounded object support: whole-particle response, supplied-layout enclosure, variable-radius contact and fixed-study publication. See [results](engine/docs/OBJECT-SUPPORT-RESULTS.md) and [object model](docs/first-stage-data-object-model.md). General recomposition and group-centroid assembly remain planned. |
| Retained data | [Full UTF-8 snapshot](data/postgres/snapshots/2026-10-03-encoding-tables/MANIFEST.md), merged through #130, covers all valid Unicode scalars. This is separate from acceptance of #111's population/controller code. |
| Built bootstrap data preparation | [Linguistic seed preparation](tools/linguistics/README.md), corrected in #131: 20-language candidate batch, distinct linguistic endpoints, core child additions and read-only validation. Accepted addresses are now retained in the seed ledger. |
| Retained/populated cold data | [#136](https://github.com/Human-Cognome-Project/human-cognome-project/pull/136) merged the [bounded letter writer and recovery checks](docs/letter-population-writer.md) implementing #132. The isolated populated pair retains 2,772 Linguistics tokens and 2,741 additive core child links, with exact read-back, zero-row rerun and paired-restore evidence. [Paired exports](data/postgres/snapshots/2026-10-13-letter-seed-validation/MANIFEST.md) are ready for local handoff; installation into existing user stores is not asserted. |
| Accepted staged plan | [#137](https://github.com/Human-Cognome-Project/human-cognome-project/pull/137) merged at `50bd968`; [#139](https://github.com/Human-Cognome-Project/human-cognome-project/issues/139) governs A–D. A/B/C are accepted and merged (#141/#143/#146). Pre-D design #147 merged at `dfa2fcc`; [#148](https://github.com/Human-Cognome-Project/human-cognome-project/issues/148) explicitly releases bounded D. |
| Stage A accepted/merged | [Native warm storage](kernels/database/warm/README.md), PR #141 merged at `e7acdea`: separate PostgreSQL schema, scoped immutable tools, full supplied initial recipes, transactional publication, read-only cold adapter, epochs and owned pins. [90 native PostgreSQL checks](kernels/database/warm/acceptance.json) pass with unchanged complete cold-data hashes, including explicit cross-writer temporary references and finite-subnormal round-trips. |
| Stage B accepted/merged | [Native letter/byte projection](engine/docs/LETTER-PROJECTION.md), PR #143 merged at `6ccd5a3`: Stage A values, exact UTF-8/ordered parents, distinct occurrence paths, explicit exposure normalization and bounded native input handoff. [53 pure and 38 native preparation checks](engine/tests/letter_projection/acceptance.json) pass. |
| Stage C accepted/merged | [Membership evidence](engine/docs/MEMBERSHIP-FIELDS.md), [#146](https://github.com/Human-Cognome-Project/human-cognome-project/pull/146), head `8508ee3`: contextual memberships per live occurrence, byte-owner inheritance, exact contributions and distinct field kinds through the existing kernel. 25 pure and 44 native checks pass, with CPU seeded response in all exposure profiles; existing regression suites and all nine hosted checks pass. Merged in #146 at `43f418a`. |
| Stage D implemented for review | [Active workbench](engine/docs/ACTIVE-WORKBENCH.md), assigned in #148: shared-only manager gate, fresh-client PostgreSQL recovery, supplied local inputs/context, native seeded execution, equal-count fail-closed refresh and controlled restart. CPU copy/direct-host density experiments are bounded evidence. Maintainer acceptance and hosted CI remain separate from local execution. |
| Pre-D design accepted | [Host workbench design](docs/pre-d-hot-workbench-design.md), #147: separate assembly/compile/runtime inputs and private ownership. [#145](https://github.com/Human-Cognome-Project/human-cognome-project/issues/145) remains open for host pinning/registration and actual GPU measurements. Broader rollups, analyst reasoning and process-failure hosting remain unbuilt. |
| Held | [#123](https://github.com/Human-Cognome-Project/human-cognome-project/pull/123): general view/identity contract needs revision; not an accepted implementation contract. This does not block the separately accepted bounded object-support slice or data bootstrap. |
| Held | [#111](https://github.com/Human-Cognome-Project/human-cognome-project/pull/111): command atomicity, restart validation and branch documentation reconciliation remain unresolved. The extracted snapshot has corrected read-only reproduction guidance. |
| Draft | [#75](https://github.com/Human-Cognome-Project/human-cognome-project/pull/75): field monitor, held pending skip machinery per its current description. |

**Active data priority:** use the retained language-letter/punctuation endpoints,
ordered core parents and membership reciprocals to define the first native
letter/byte assembly and warm-cache handoff. Individual letters are the current
test objects. The initial seed batch is populated; broader coverage and native
cache assembly remain follow-up work. No new analyst implementation or general
physics qualification gate is introduced here.

**Settled roots:** `00` = common computational core; `02` = shared multilingual
Linguistics, with `02.FS.LR.00.*` reserved for language-frame word components.
Current plans assign no new meaning to `01`; the old English `01` /
common `01.00` layout is superseded. The initial seed map is accepted in #132; further allocations require review.
Patrick reserved `y*` for personal data and `z*` for temporary objects on
2026-10-15 in #137. These name first-symbol prefix families, not stored wildcards
or concrete object allocations. Internal subdivisions, delegation and eventual
lifetime remain deferred; no additional expansion blocks are assigned.

**Near-term storage follow-up:** [#134](https://github.com/Human-Cognome-Project/human-cognome-project/issues/134)
tracks exact ranges for reverse child listings, starting with current-data
measurements. Common words may have low traversal value but retain complete
cold references. Range storage is not built and does not change #132's current
row-based population contract. See [the detailed direction](kernels/database/WORKING-SET-AND-LEDGER.md#range-compressed-reverse-child-listings-patrick-2026-10-13).

**Warm-cache implementation:** The [accepted A–D contract](docs/warm-toolbox-implementation-plan.md)
and [layout](docs/warm-knowledge-cache-layout.md) preserve warm as the toolbox and
hot as the host workbench where objects and relationships are assembled; backend
execution memory is where their numerical projection runs together. Stage A
stores supplied recipes; B/C supply projection and contextual fields. D was
released in #148 after #147 acceptance and is implemented for review. It adds
supplied active inputs and controlled session lifetime, not analyst reasoning,
general rollups or process-failure isolation. Ordinary vectors are not pinned;
GPU allocation/transfer qualification remains open in #145.

**Private/local boundary (Patrick, 2026-10-16):** the analyst alone supplies its
private/local information at active hot assembly. The cache manager has no role
in accessing those DB codes/data or publishing, acquiring or refreshing them in
warm storage. Exclude private namespaces/handles from the manager's authorized
request/source path and reject attempted private scopes **before source lookup**;
do not read and then filter, redact or classify. This protects model/entity
private stores; public entity knowledge can remain shared. Joint numerical
reporting returns to the analyst-owned workbench, with no private-source or joint
result propagation through manager reporting. Use supplied private fixtures;
analyst reasoning and real private DB access are not implemented here.

A's isolated scoped-storage tests are built capabilities, not authority to route
private data through the manager. That earlier plan wording is superseded; no
blanket change to `z*` temporary ownership or retained fixtures is authorized.
Warm publication, workbench assembly completeness and execution readiness are
separate outcomes. Native test clients do not implement an analyst or scheduler.
The accepted `y*` / `z*` reservations still require explicitly authorized concrete
bindings, and their internal production layout remains deferred.
[#138](https://github.com/Human-Cognome-Project/human-cognome-project/issues/138)
tracks endpoint sub-classification and numeric/operator coverage/placement as a
separate follow-up, preserving seed identities and adding no gate on that milestone.

## Reading paths

This is the canonical onboarding map, shared by README, CONTRIBUTING and the
documentation index. Read the common path, then the row relevant to your work.
Agents also read this file in full. Component-specific instructions still apply.

| Audience | Read in order |
|---|---|
| Everyone | [Covenant](covenant.md) → [Charter](charter.md) → current state above → [NAPIER system guide](docs/napier-system-guide.md), observing built/planned labels. |
| Agents | This file → [model realignment plan](docs/model-realignment-plan.md) → component README, accepted plan and tests. |
| Engine / geometry / LoD | [Engine architecture](engine/ARCHITECTURE.md) → [active field model](engine/docs/ACTIVE-FIELD-MODEL.md) → [realignment plan](docs/model-realignment-plan.md) → [built results](engine/docs/OBJECT-SUPPORT-RESULTS.md). Check #123's latest disposition for general identity/recomposition work. |
| Data / linguistics | [Language component plan](docs/language-component-unicode-plan.md) → [first-stage object model](docs/first-stage-data-object-model.md) → [working set and ledger](kernels/database/WORKING-SET-AND-LEDGER.md) → component README/tests. Check the status of population proposals before implementing their allocations or writes. |
| Reviewers | Current state above → changed component's accepted plan and code → PR description, latest review findings and applicable acceptance criteria. |
| Repository structure | [Reorganization record](REORGANIZATION.md) → relevant component architecture. Historical folder placement is not an architecture contract. |

## Architecture invariants

- The modified Taichi **runtime/compiler substrate**, HCP **physics engine**, and HCP **engine harness** are distinct concepts.
- The canonical engine path is native **C++**. Taichi's Python frontend is upstream/vendor tooling, not the HCP runtime surface.
- Project Python is authorized only for bulk database population scripts during bootstrap. Their source preparation and validation stay within that population workflow. Offline operation alone does not authorize Python. Do not implement reusable selection/assembly components, physics, database/WAL kernels, endpoint/network infrastructure, or analyst behavior in Python.
- The **engine harness is the control interface for the physics engine**. It is not the DB manager, WAL manager, messaging system, topology system or thread manager.
- **Analyst functions do not exist yet.** Do not invent an analyst architecture or implement speculative analyst behaviour.
- Warm is the shared tool/part box; hot is the analyst-owned host assembly workbench; GPU memory is the execution surface. Private/local inputs enter only through the analyst-owned active input boundary. The cache manager must not access private DB codes/data, including for content inspection or filtering.
- Database/cache/record/WAL kernels exist to support the future analyst and keep its working surfaces current.
- Kernel components communicate through active inbox/outbox endpoints and should remain location-blind: local versus remote placement is a topology/bridge concern.
- The future configuration routine resolves endpoint paths; the future thread manager handles lower-frequency/system-facing bridge activation.
- `kernels/database/`, `kernels/wal/`, and `network/endpoint/` are peers. Do not collapse WAL or shared endpoint infrastructure back under the database/cache hierarchy.

## Do not reintroduce

- Do not infer conceptual scale from raw SNode depth or secondary compression nesting.
- Do not implement the superseded English `01` / per-language database layout.
- Do not substitute a shared core character for a language-letter endpoint: the linguistic endpoint has its own identity and ordered core parents.
- Do not materialize reverse DB child-reference listings as live particles. They support navigation/assembly; ordered parent occurrences and separately prepared fields determine live participation.
- Do not extend Python beyond early bulk database bootstrap population and its preparation/validation. Offline operation alone is no exception; operational selection and assembly are native.
- Do not invent analyst behaviour or treat held #123 proposals as an accepted contract.
- Do not restore universal fixed diameter, beads-on-string geometry, positional parent offsets or rotation.
- Do not reset/drop persistent databases as routine validation.

## Current development state

The structural repository reconciliation is complete on `main`. Native engine recovery restored the C++ workspace and curated Taichi fork; hosted CI builds and tests the native CPU path from source. Further architecture and performance work is tracked separately from the reorganization.

Branch new work from current `main` unless a specific recovery/integration branch is explicitly named for the task.

## Current model realignment (2026-10-09)

Follow the [conversion and continuation plan](docs/model-realignment-plan.md)
for sequencing, dependencies and verification. Its dated decisions supersede
conflicting historical model requirements; code still determines what is built.

- Primary SNode structure expresses conceptual LoD. Attached secondary
  compression structures inherit that scale at **1:1**, including nested
  dedupe; storage depth alone never creates a conceptual scale step.
- Component-derived spherical extent replaces universal fixed diameter.
  Supplied-layout enclosure is built; packing and broader extent questions remain open (#120).
- Parent response is proportional and whole-particle. No internal application
  offsets, rotation, rotary alignment or beads-on-string requirement.
- Corona-disabled variable-radius contact is built in the bounded object-support
  slice. Scaled attraction supplies intended cohesion; broader calibration
  remains empirical.
- Whitespace is recorded as positional gaps; optional model separators have
  only local connecting participation, no structural relationships or gathering
  mass, and no field with other whitespace occurrences.
- Track bridge/identity in #106, distance calibration in #115, whitespace in
  #116, response in #118, contact in #119 and extent in #120.
- Dictionary structures and SNode assembly are the near-term objective.
  Follow the [language component plan](docs/language-component-unicode-plan.md):
  `00` is the common computational core; `02` is the shared multilingual
  Linguistics base, with `02.FS.LR.00.*` reserved for language-frame word
  components. Patrick confirmed this supersedes English `01` / common `01.00`
  on 2026-10-11. No new meaning for `01` is assigned here. Formal cross-store
  writes still require the plan's bounded reference/write implementation.
- Linguistic aggregation uses reciprocal `members` / `member_of` links.
  Shared core character values retain one identity. Each language's letter or
  punctuation endpoint has its own linguistic identity, with ordered core
  character parents and matching additions to those core endpoints' child
  listings (Patrick clarified 2026-10-12). The linguistic endpoints join their
  language and alphabet/punctuation groupings. English variants belong to English, which
  belongs to its broader groups. Each grouping has its own centroid
  representation; alphabet and punctuation groups are distinct. Literal
  `parent` / `child` links describe construction only. Do not duplicate the
  underlying core character values or encode group containment as parents.
  Existing record-integrity findings are not cleared by
  documentation merges; check current PR/issue state before execution.

## Work discipline

- Code and executable tests determine what is built. Design notes determine intent only where they explicitly say a decision is settled.
- Preserve distinctions such as **BUILT**, **planned**, **deferred**, and **experimental**. Do not promote a plan to implementation by paraphrase.
- When moving kernel families, migrate their include paths, build instructions and tests together. Preserve working interfaces before refactoring behaviour.
- Preserve the native engine boundary under `engine/src/`: `engine_support` wraps the Taichi runtime/compiler substrate; `field_core` owns field/tick mechanics; the wider analyst-facing harness remains a distinct control layer. Do not promote the archived Python prototype back into the runtime path.
- Never force-push or rewrite shared history for cleanup. Normal commits, merges and moves keep prior versions recoverable.
- Do not add tests or utilities that reset, drop, or truncate a database or schema as part of routine execution. The core snapshot is retained and may grow; validate it read-only unless a specific data change is authorized.
- Keep credentials and private data out of Git. PostgreSQL reproducibility exports belong under `data/postgres/snapshots/` and use Git LFS for compressed dumps.
- Add or preserve tests for behavioural changes. Large generated run artifacts should not be treated as source code merely because an experiment produced them.

## Contribution flow

Use focused branches and reviewable commits. For structural changes, prefer small commits that do one of:

- history-preserving integration;
- pure structural move;
- documentation correction;
- behaviour refactor;
- test/build repair.

Do not combine all of those in a single cleanup commit.
