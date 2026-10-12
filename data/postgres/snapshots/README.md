# PostgreSQL state snapshots

This directory is reserved for reproducibility snapshots exported from development PostgreSQL instances.

The current retained `hcp_core` export is the
[2026-10-03 full UTF-8 encoding snapshot](2026-10-03-encoding-tables/MANIFEST.md),
extracted from PR #111. It contains all valid Unicode scalar endpoints.
The [2026-09-28 encoding floor](2026-09-28-encoding-floor/MANIFEST.md) remains
historical evidence of the initial nibble/byte state. Snapshots record the
store's growth; none of these files authorizes resetting the live store.

Use dated subdirectories and keep state dumps separate from source schema/migrations and from runtime kernel code. A snapshot set should normally include:

- compressed SQL dump(s);
- SHA-256 checksum(s);
- a short manifest describing the database names, export date, relevant schema/version context, and any known dependencies.

Compressed `*.sql.gz` snapshots under this tree are tracked through Git LFS.

Do not commit credentials, connection strings containing secrets, private user data, or machine-specific authentication material.

The existing `/db` tree is retained unchanged during the initial reorganization because it contains legacy database tooling/dumps that still need separate disposition.
