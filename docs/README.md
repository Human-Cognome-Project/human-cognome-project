# Documentation

## Current system map

- **[address-encoding-transition.md](address-encoding-transition.md)** —
  new RFC 4648 §5 URL-safe Base64 alphabet decision, namespace arithmetic,
  and migration seams; existing record-tier code is still base-50.
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

Read in order:

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
