# Legacy data maps

Working documentation of the previous era's data holdings, carried forward **because extraction
still navigates by them** (everything else from that documentation tree is in
[`/archive/2026-08-rebase/docs/`](../../archive/2026-08-rebase/docs/)). These describe read-only
sources; they are replaced, not updated, when the new schema documentation lands.

- [shards-and-schema.md](shards-and-schema.md) — the shard inventory and table shapes.
- [kaikki-pipeline.md](kaikki-pipeline.md) — the Wiktionary → Kaikki → source_* → hcp_english lineage.
- [tokenization-policies.md](tokenization-policies.md) — what shapes the stored data has (needed to pull it correctly).
- [database-access.md](database-access.md) — where the live stores are and how to read them.

Live-store ground truth as of 2026-08-30 (row counts, address-column state, sentinel registry):
[/review/pass-04-data.md](../../review/pass-04-data.md).
