# PostgreSQL state snapshots

This directory is reserved for reproducibility snapshots exported from development PostgreSQL instances.

Use dated subdirectories and keep state dumps separate from source schema/migrations and from runtime kernel code. A snapshot set should normally include:

- compressed SQL dump(s);
- SHA-256 checksum(s);
- a short manifest describing the database names, export date, relevant schema/version context, and any known dependencies.

Compressed `*.sql.gz` snapshots under this tree are tracked through Git LFS.

Do not commit credentials, connection strings containing secrets, private user data, or machine-specific authentication material.

The existing `/db` tree is retained unchanged during the initial reorganization because it contains legacy database tooling/dumps that still need separate disposition.
