# Contributor Execution Guide

## Prerequisites

- Python 3.12+
- PostgreSQL 16+ (for DB-dependent code)

## Virtual Environments

There are two separate venvs. Use the right one for your task.

### HCP dev (pip venv)

For all core HCP work — token encoding, DB operations, ingestion, tests.

```bash
source /opt/project/repo/.venv/bin/activate
# or create fresh:
python3 -m venv .venv && source .venv/bin/activate && pip install -e ".[dev]"
```

Python 3.12, psycopg v3, pytest, lmdb, sqlalchemy, warp-lang, fastapi, msgpack.

### O3DE (Open 3D Engine)

Game engine with **PhysX 5 built in**. Installed system-wide.

```bash
# CLI (headless, no display needed):
/opt/O3DE/25.10.2/scripts/o3de.sh <subcommand>

# O3DE's bundled Python:
/opt/O3DE/25.10.2/python/python.sh script.py
```

- O3DE 25.10.2 installed at `/opt/O3DE/25.10.2/`
- PhysX 5 is native — no plugins or extensions needed
- C++ primary language, Python scripting available
- Requires display for Editor; CLI and builds work headless

### GPU Layout

| GPU | Card | VRAM | CC | Role |
|-----|------|------|----|------|
| cuda:0 | GTX 1070 | 8 GB | 6.1 | **Primary compute** (PhysX 5 / Warp) |
| cuda:1 | GTX 750 Ti | 2 GB | 5.0 | Engine render (if needed) |

**NVIDIA Warp** (v1.11.1) is in the HCP dev venv — bundles CUDA 12 runtime, needs driver 525+
(current driver: 535). Both GPUs work with Warp but cuda:0 is preferred (cuda:1 lacks mempool).

Warp kernels must be in `.py` files on disk (JIT needs `inspect.getsource()`).

## Active Code Tree

```
src/hcp/                    ← sole runtime package
├── core/
│   ├── token_id.py         ← base-50 Token ID encoding/decoding
│   └── byte_codes.py       ← byte classification (256 values)
├── db/
│   ├── postgres.py         ← hcp_core connection + schema
│   ├── english.py          ← hcp_english connection
│   ├── names.py            ← hcp_names connection (shard merged into hcp_english)
│   ├── kaikki.py           ← Wiktionary data loader
│   └── pbm.py              ← PBM database interface
├── ingest/                 ← data ingestion scripts
│   ├── atomization.py      ← Unicode byte atomization
│   ├── byte_codes.py       ← byte code ingestion to DB
│   ├── encoding_tables.py  ← encoding table setup
│   ├── gutenberg_*.py      ← Project Gutenberg pipelines
│   ├── nsm_*.py            ← NSM primitive/molecule loading
│   ├── names.py            ← name component ingestion
│   ├── words.py            ← word token ingestion
│   └── abbreviations.py    ← abbreviation data
└── reconstruction/
    └── spacing.py          ← spacing rules for text reconstruction
```

All database connectors use **psycopg v3** (`import psycopg`).

## Running Tests

```bash
# All tests (no DB required)
pytest

# Verbose
pytest -v

# Single module
pytest tests/test_token_id.py
```

### Which tests need a database?

Tests marked with `@pytest.mark.db` require a running PostgreSQL instance. Currently all tests in `tests/` are pure-Python and run without a database.

DB connection details (dev environment):
- Host: `localhost`, Port: `5432`
- User: `hcp`, Password: `hcp_dev`
- Databases: `hcp_core`, `hcp_english`, `hcp_en_pbm`

## Lint

No linter is currently configured in pyproject.toml. To run basic checks:

```bash
python -m py_compile src/hcp/core/token_id.py   # syntax check any file
```

## Key Concepts

- **Token IDs** use base-50 pair encoding (alphabet: A-N, P-Z, a-n, p-z). Each pair encodes 0–2499. IDs are 1–5 dot-separated pairs.
- **Namespaces**: `AA.*` = core, `AB.*` = English text, `zA.*` = source PBMs.
- **PBM** (Pair-Bond Map): frequency-ranked token adjacency data.
- Database schemas live in `db/dumps/` as SQL files. Migrations are in `db/migrations/`.

## What NOT to Touch

- `db/` directory — managed by the DB specialist
- `docs/research/` — managed by the linguistics specialist
- `docs/decisions/` — append-only decision records
