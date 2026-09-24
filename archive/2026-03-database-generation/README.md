# March 2026 database generation

This directory preserves the pre-rebase PostgreSQL/LMDB database generation that previously lived at the repository root as `db/`.

It is **historical implementation and data**, not the current database/cache architecture.

The preserved tree includes:

- the earlier shard schemas and migrations;
- SQL dump artifacts and checksums;
- database tooling;
- the March 2026 roadmap/TODO/agent guidance that governed that generation.

Current database/cache kernel work lives under `/kernels/database/`. Current WAL work lives under `/kernels/wal/`. Reproducibility exports from the present development PostgreSQL systems belong under `/data/postgres/snapshots/`.

The exact original layout and previous versions remain recoverable from Git history.
