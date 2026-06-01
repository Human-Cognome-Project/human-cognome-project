# Deferred Deep Engine Pages

> **These are NOT stale — they are deferred.** Distinct from the rest of `_archive/` (which holds
> superseded legacy docs), these three pages are accurate-as-written but were **intentionally deferred**
> from the active docs per Patrick-direct **claim 292**: documenting the engine's forward-looking
> mechanics (claims 265–288) and rework-pending Gem internals (claims 201/239) in depth now would be
> documenting a **moving target.**

The active engine section keeps only a **light overview** plus the **built** state. These deep pages
are parked here until the engine rework settles, at which point they can be revised against the
then-current implementation and promoted back.

| Page | Covers | Claims |
|------|--------|--------|
| `cognitive-cycle.md` | the 11 ms reconciliation beat, modality streams, deeming policy | 265/267/268/269/270/274/275/276/277 |
| `resolution-furnace.md` | GPU PBD substrate channels, articulation trees, throughput thesis | 281/216/214/213/215/288 |
| `reconciliation-loop.md` | two-LMDB split, ring buffer, stale-work check, loop optimization | 119/121/282/283/284 |

The content is summarized at a high level in
[../../04-engine/overview.md](../../04-engine/overview.md), and the underlying claims remain in the
orchestrator claim-graph (the source of truth). Nothing is lost — this is a prioritization move, not a
supersession.

> Note: links *inside* these three archived pages may point to siblings that are also archived or to
> active pages; they are preserved as-authored and are not maintained while parked here.
