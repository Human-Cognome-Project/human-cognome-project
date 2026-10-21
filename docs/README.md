# Documentation

Start with the [current state](../AGENTS.md#current-state) and
[canonical reading paths](../AGENTS.md#reading-paths). This index supplies
references, not another mandatory reading sequence.

## Warm-cache plan development

- [Root-addressed warm knowledge cache layout](warm-knowledge-cache-layout.md) —
  PostgreSQL shared toolbox, host assembly workbench and separate execution
  surface; reconciled with the analyst-owned active-only private boundary.
- [Concrete warm toolbox implementation proposal](warm-toolbox-implementation-plan.md) —
  proposed PostgreSQL publication, persisted initial recipes, source-preserving
  byte exposure and normalized scale, membership preparation and existing
  packed-SoA hot handoff; accepted in #137, with staged work tracked by #139.
- [Stage A native warm storage](../kernels/database/warm/README.md) — accepted
  and merged in #141; supplied normalized recipe storage and 90 PostgreSQL checks.
- [Stage B native letter/byte projection](../engine/docs/LETTER-PROJECTION.md) —
  accepted/merged in #143; 53 pure and 38 native preparation checks.
- [Stage C evidence](../engine/docs/MEMBERSHIP-FIELDS.md), [PR #146](https://github.com/Human-Cognome-Project/human-cognome-project/pull/146) —
  reviewed with no code blockers and all nine checks green, accepted and merged at `43f418a`.
- [Pre-D host workbench design](pre-d-hot-workbench-design.md) — accepted in #147 under
  #145; compilation/value boundaries, private/local active input, portable
  allocation/transfer, restart and dense-field rollups. Bounded D is accepted/merged in #149 under #148.
- [Stage D active workbench](../engine/docs/ACTIVE-WORKBENCH.md) — native shared-only
  recovery, separate supplied local inputs, seeded execution and controlled restart;
  CPU copy/direct-host density evidence, with GPU pinning tracked separately in #145.

## Active implementation plans and built results

- [Prepared particle space and live representation](pooled-space-live-insertion-plan.md) —
  proposed plan for #156: lightweight `0x` declarations, reuse while data is hot,
  prepared native backing, structural recipes and continuous engine-owned updates.
- [Model realignment plan](model-realignment-plan.md) — settled directions,
  built bounded support and remaining work; #123 is held for revision.
- [Language component plan](language-component-unicode-plan.md) — distinct
  linguistic endpoints, shared core parents and reciprocal seed links.
- [First-stage data object model](first-stage-data-object-model.md) — built
  native preparation boundary and its limits.
- [Object-support results](../engine/docs/OBJECT-SUPPORT-RESULTS.md) — what
  the bounded engine slice implements and how it was checked.
- [Effective-motion CPU evidence](../engine/docs/EFFECTIVE-MOTION.md) — #91's
  selective particle/centroid gates, supplied focal origin, demand counts and
  accepted/merged in #150; CPU prerequisite satisfied, GPU evidence under #145.
- [Type-phase simplification #152](https://github.com/Human-Cognome-Project/human-cognome-project/issues/152) — accepted/merged in #153; one native activity set per type, with type-defined phases.
- [Human viewport purpose and plan](viewport-purpose-and-plan.md), [#151](https://github.com/Human-Cognome-Project/human-cognome-project/issues/151) — accepted in #154; [optional implementation](../tools/viewport/README.md) is ready for review with real letter-study captures and bounded native/browser evidence.

Held/deferred PRs: #123 (unaccepted general identity contract), #111 (unresolved
command atomicity/restart), and #75 (optional historical monitor draft). They
are not prerequisites for the current bounded work. See [guidance](../AGENTS.md#active-and-deferred-work--2026-10-19).

## Current system map

- **[address-encoding-transition.md](address-encoding-transition.md)** —
  paired Base62 `text[]` keys, namespace arithmetic, shared-root compression
  and wildcard ranges; explains why the proposed numeric key was reversed.
- **[napier-system-guide.md](napier-system-guide.md)** — coherent cold → warm → hot,
  model, analyst, WAL, and endpoint flow, with built/planned boundaries.
- **[instance-local-data.md](instance-local-data.md)** — reserved private DB
  scope, inward-only deferred work, and unresolved release/encryption design.
- **[database working set and ledger](../kernels/database/WORKING-SET-AND-LEDGER.md)** —
  reciprocal paths, study-rooted warm projection, and calculation scopes.
- **[active field model](../engine/docs/ACTIVE-FIELD-MODEL.md)** — tick order,
  centroid wakeup, brake, and open formula questions.
- **[WAL report to work](../kernels/wal/REPORT-TO-WORK.md)** — report ingress,
  durable deferred work, and privacy boundary.
- **[kernel activation](../network/KERNEL-ACTIVATION.md)** — mailbox priority,
  component scaling, and monitor responsibilities.
- **[napier-system-discussion.md](napier-system-discussion.md)** — dated working
  record preserving the derivation and unsettled alternatives.

## Foundations and recovered notes

Reference material; consult the sections relevant to your component:

1. **[physics-basis.md](physics-basis.md)** — what was determined, the principles the project runs
   on, lineage, and the open problems. The primary research sources are
   [`/research/ledger/`](../research/ledger/) and [`/research/field/`](../research/field/).
2. **[architecture.md](architecture.md)** — the structure of the space (nibble/byte particles),
   arrayed-pair addressing, the flat pool and logical chains, composition and compression, the
   singularity, NAPIER, and the engine substrate.
3. **[data-protocol.md](data-protocol.md)** — intake invariants (no filtering; ordering, flags, and
   provenance instead), the landing lattice, and how previous-era stores are pulled from.
4. **[../engine/docs/README.md](../engine/docs/README.md)** — status/index for the recovered native
   C++ engine, harness, Taichi-vetting and device-measurement record.

Native-engine model records recovered from the local September workspace:

- **[field-physics-and-tick-notes.md](field-physics-and-tick-notes.md)** — field/tick model and
  implementation-facing physics decisions.
- **[bonding-notes.md](bonding-notes.md)** — bonding/composition decisions and unresolved seams.
- **[parent-structure-notes.md](parent-structure-notes.md)** — parent/child/sibling structural
  semantics and data/harness consequences.
- **[particle-geometry-notes.md](particle-geometry-notes.md)** — operative particle geometry and
  constraints.
- **[storage-and-working-split.md](storage-and-working-split.md)** — storage/working/resident split
  and engine-capacity consequences.

These recovered notes carry provenance banners. Their dated statements about what was or was not
built are not current repository status; use the engine documentation index and current code for that.

Reference:

- **[legacy-data-maps/](legacy-data-maps/)** — working maps of older data holdings retained for
  migration/reference until current schema documentation replaces them.
- **[/review/](../review/)** — review records and dispositions from repository recovery/rebase work.
- **[/archive/](../archive/)** — preserved prior generations, superseded implementations and
  historical records.
