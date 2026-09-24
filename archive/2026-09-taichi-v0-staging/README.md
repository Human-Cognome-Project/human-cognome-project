# Taichi v0-staging generation — archived September 2026

This directory preserves the parallel Taichi development generation built on the
`engine/v0-staging` lineage after the 2026-08-30 repository rebase.

## Why it is archived

This line implemented the earlier two-half field model:

- `engine/kernel/` — amplitude/diffusion/binding physics and corpus-pour experiments;
- `engine/timestep/` — readout, compliance, clock/persistence and early write-back experiments;
- `engine/storage/` — SNode/compositional-address and generated-chain experiments;
- `engine/ingest/` — full-file/database-ingest experiments;
- `engine/SEAM.md` — the contract joining those halves.

The later September field implementation now under `/engine/field/` was developed
on the other active lineage and does not import or execute these modules. It moved
to a different configuration-field model with general pull, identity pull,
exclusion and condensation. The DB/cache/WAL/kernel-network work also developed
independently of this v0-staging tree.

The old line is therefore preserved as a predecessor and experimental record rather
than left in the active engine namespace.

## Additional material from the same lineage

- `docs/` contains the v0-staging data plan, fresh-DB proposal and language-grid artifacts.
- `extraction/pull_english_fresh_v0.sql` is the corresponding fresh-pull experiment.

These may still contain useful experiments, measurements or implementation ideas.
Archiving means **not current runtime**, not "worthless" or "never reuse."

## History

The branch history remains intact in Git. No commits were squashed or rewritten.
The integration branch joined `engine/v0-staging` with the later kernel-network
line using a normal two-parent merge before this move.

Several development-machine database defaults were sanitized during the repository
reorganization before the current-tree archive was made. The exact original bytes
remain recoverable from Git history and the original branch.
