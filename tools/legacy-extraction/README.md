# Legacy extraction toolkit

This is a retained **read-only migration/extraction toolkit** from earlier HCP storage generations. It is not runtime code.

The toolkit was carried forward from `src/hcp/` and `scripts/wiktionary/` during the August 2026 rebase. Provenance is recorded in [review/pass-03-code.md](../../review/pass-03-code.md) and [review/pass-05-tooling.md](../../review/pass-05-tooling.md); originals remain under [archive/2026-08-rebase/](../../archive/2026-08-rebase/).

## Contents

- **`token_id.py`** + **`tests/test_token_id.py`** — legacy arrayed-pair address codec used when reading older stores.
- **`postgres.py`, `english.py`, `names.py`** — connectors for earlier shard generations. Connection settings are environment-driven; do not treat these modules as the current database interface.
- **`kaikki.py`** — Kaikki/Wiktionary intake tooling from the earlier substrate.
- **`gutenberg_fetch.py`** — Gutendex source fetcher with provenance metadata.

## Rules of use

1. Treat old stores as read-only migration sources.
2. O/o legacy-address correction is an extraction concern only; current address rules live with the active database codec under `../../kernels/database/codec/`.
3. Sentinels from old stores are migration flags, not current addresses.
4. Do not add new runtime dependencies on this toolkit.
5. Python is acceptable here because this is offline migration/source tooling, not a hot path.

If a migration utility becomes necessary for the active system, promote that function deliberately into an appropriate current tools area rather than treating this directory as canonical application code.
