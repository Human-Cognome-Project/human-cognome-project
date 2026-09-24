# Documentation

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
