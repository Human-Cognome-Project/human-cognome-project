# Quick Reference

A consolidated cheat-sheet. For detail, follow the links.

---

## Engine daemon

| Item | Value |
|------|-------|
| Process | `HCPEngine.HeadlessServerLauncher` |
| Port | TCP **9720** |
| Protocol | length-framed JSON (4-byte big-endian length prefix + UTF-8 JSON) |
| API verbs | `health`, `ingest`, `retrieve`, `list`, `tokenize`, `phys_ingest` |
| Reference client | [`scripts/hcp_client.py`](../../scripts/hcp_client.py) |

```bash
python scripts/hcp_client.py ingest <file|dir>
python scripts/hcp_client.py list
python scripts/hcp_client.py delete <doc_id>
```

Build/run detail: [build-and-run.md](build-and-run.md).

---

## Databases (NAS HAVEN — `192.168.68.60:5435`)

```bash
# Data shards (dev role)
PGPASSWORD=hcp_dev psql -h 192.168.68.60 -p 5435 -U hcp -d hcp_english

# Orchestrator claim-graph (source of truth)
PGPASSFILE=/home/patrick/.creds-hcp-orchestrator \
  psql -h 192.168.68.60 -p 5435 -U hcp_orchestrator_rw -d hcp_orchestrator
```

Shards: `hcp_core`, `hcp_english` (~1.494M), `hcp_envelope`, `hcp_fic_pbm`,
`hcp_fic_{people,places,things}`, `hcp_nf_{people,places,things}`, + `source_english`,
`source_wiktionary`, `hcp_orchestrator`. Detail: [database-access.md](database-access.md).

Claim-graph helpers: `get_current('topic')`, `find_claims('terms')`, `claim_edges`,
`get_supersession_chain(id)`.

---

## Scripts (`scripts/` — Python front-end tooling only)

| Script | Purpose |
|--------|---------|
| `hcp_client.py` | talk to the engine daemon |
| `ingest_texts.py` | batch text ingestion |
| `run_benchmark.py` | ingestion benchmark |
| `load_kaikki_fast.py` / `setup_kaikki_schema.py` | Kaikki load + schema setup |
| `create_*_entities.py`, `mint_multiword_ids.py`, `resolve_relations*.py` | data-build passes |

> **Language policy** (claim 30): `scripts/` is build-time / developer tooling only. **Nothing in
> `scripts/` is an engine component.** All runtime/engine/pipeline logic is C++ in the O3DE Gem.

---

## Migrations (`db/migrations/`)

```bash
db/migrations/run.sh        # apply migrations
```

> **History note:** migration `048_position_normalization` is **superseded by**
> `049_position_arrays` (claim 205) and would wrongly re-run on a from-scratch replay. See
> [`db/migrations/README_position_history.md`](../../db/migrations/README_position_history.md).

---

## Doc map

| If you want… | Read |
|--------------|------|
| the single keystone idea | [../02-architecture/keystone-db-functions.md](../02-architecture/keystone-db-functions.md) |
| the design principles | [../02-architecture/principles.md](../02-architecture/principles.md) |
| the engine's three parts | [../04-engine/overview.md](../04-engine/overview.md) |
| how text becomes tokens | [../04-engine/resolution-chamber.md](../04-engine/resolution-chamber.md) |
| the data substrate | [../05-data-layer/shards-and-schema.md](../05-data-layer/shards-and-schema.md) |
| current status | [../06-status/status.md](../06-status/status.md) |
| what's deferred / in-flux | [../06-status/deferred-and-open.md](../06-status/deferred-and-open.md) |
| the phase plan | [../../ROADMAP.md](../../ROADMAP.md) |
| terminology | [../00-orientation/glossary.md](../00-orientation/glossary.md) |
