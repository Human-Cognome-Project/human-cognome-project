# Legacy database access map

> **Historical migration reference.** This document describes database holdings
> observed during the June 2026 architecture and is retained only to help read or
> migrate older stores. It is not current HCP architecture, configuration, or an
> authority hierarchy.

The original version of this file contained a private LAN address, a development
password, a workstation credential-file path, and language describing the old
`hcp_orchestrator` claim graph as the project's source of truth. Those details
have been deliberately removed from the current tree.

Current code and documentation must obtain database connection details from the
environment or another local secret/configuration mechanism. Do not embed hosts,
passwords, private paths, or credentials in Git.

A conventional PostgreSQL invocation is:

```sh
PGHOST=... PGPORT=... PGUSER=... PGDATABASE=... psql
```

Use `PGPASSWORD` only as a local process environment value when appropriate,
or preferably a local `PGPASSFILE`/credential mechanism. No value in this
document is a credential.

---

## Historical database inventory

The following names were observed in the June 2026 store and are preserved as
migration/provenance information, not as a statement that these databases still
exist or remain canonical:

| Database | Historical role |
|---|---|
| `hcp_core` | universal concepts / cold-resident core |
| `hcp_english` | English text forms |
| `hcp_envelope` | older query/filter workspace definitions |
| `hcp_fic_pbm` | fiction pair-bond maps |
| `hcp_fic_people`, `hcp_fic_places`, `hcp_fic_things` | fiction entity stores |
| `hcp_nf_people`, `hcp_nf_places`, `hcp_nf_things` | non-fiction entity stores |
| `source_english` | older drained/deduplicated English substrate |
| `source_wiktionary` | raw Wiktextract source |
| `hcp_orchestrator` | old cross-linked claim graph used by the previous documentation process |

Historical counts and connection tests from the earlier document should be
treated as dated observations, not current measurements.

See [shards-and-schema.md](shards-and-schema.md) for the corresponding legacy
schema map.

---

## Historical orchestrator claim graph

The old documentation workflow used `hcp_orchestrator` as a cross-session
claim graph with helpers such as `get_current(...)`,
`find_claims(...)`, and a `claim_edges` table.

That graph is **not current architectural authority**. It is part of the
previous documentation/memory system and may be useful only when reconstructing
provenance from older material.

Current architecture and implementation authority comes from the current
repository code plus explicitly current design records. In particular, start
with:

- [../README.md](../README.md)
- [../architecture.md](../architecture.md)
- [../../engine/ARCHITECTURE.md](../../engine/ARCHITECTURE.md)
- [../../engine/docs/README.md](../../engine/docs/README.md)
- [../../AGENTS.md](../../AGENTS.md)

Do not restore old connection details merely to make this historical map
executable.
