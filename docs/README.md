# Documentation

Start with the [current state](../AGENTS.md#current-state) and
[canonical reading paths](../AGENTS.md#reading-paths). This index supplies
references, not another mandatory reading sequence.

## Warm-cache plan development

- [Root-addressed warm knowledge cache layout](warm-knowledge-cache-layout.md) —
  proposal for PostgreSQL multi-writer ownership, temporary/personal integration,
  and native letter/byte assembly. Implementation details remain for review.
- [Concrete warm toolbox implementation proposal](warm-toolbox-implementation-plan.md) —
  proposed PostgreSQL publication, persisted initial recipes, source-preserving
  byte exposure and normalized scale, membership preparation and existing
  packed-SoA hot handoff; accepted in #137, with staged work tracked by #139.
- [Stage A native warm storage](../kernels/database/warm/README.md) — implemented
  for review under #140; supplied normalized recipe storage and 69 PostgreSQL
  acceptance checks. Projection, membership fields and hot materialization remain
  subsequent stages.

## Active implementation plans and built results

- [Model realignment plan](model-realignment-plan.md) — settled directions,
  built bounded support and remaining work; #123 is held for revision.
- [Language component plan](language-component-unicode-plan.md) — distinct
  linguistic endpoints, shared core parents and reciprocal seed links.
- [First-stage data object model](first-stage-data-object-model.md) — built
  native preparation boundary and its limits.
- [Object-support results](../engine/docs/OBJECT-SUPPORT-RESULTS.md) — what
  the bounded engine slice implements and how it was checked.

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
