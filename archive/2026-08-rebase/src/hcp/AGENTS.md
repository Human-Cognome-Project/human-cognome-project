# Python Tools — Agent Guide

## Role
Python code here is **reference and tooling**, not the runtime engine. The authoritative engine is C++ at `hcp-engine/Gem/Source/`. Python implementations should match C++ decisions, not drive them.

## Module Ownership

| Module | Primary Owner | Notes |
|--------|--------------|-------|
| `core/` | Infrastructure specialist | Stable, rarely changes |
| `db/` | DB specialist | Connectors only — schema lives in `db/migrations/` |
| `ingest/` | Librarian specialist | Ingestion pipeline for populating databases |
| `engine/` | Engine specialist (reference) | Must stay in sync with C++ Gem |
| `cache/` | DB specialist (reference) | Must match C++ resolver spec |
| `reconstruction/` | Engine specialist | Spacing rules |

## Key Conventions
- All DB connectors use **psycopg v3** (`import psycopg`) — NOT psycopg2
- Token IDs use **base-50** pair encoding (not base-20)
- NVIDIA Warp kernels MUST be in `.py` files on disk (JIT needs `inspect.getsource()`)
- Virtual environment: `/opt/project/repo/.venv/`

## Adding a Format Builder
The simplest way to contribute. All format builders follow one pattern:

1. Extract plain text from the source format
2. Feed text to existing tokenizer pipeline
3. Store results via existing storage module

See [hcp-engine/AGENTS.md](../../hcp-engine/AGENTS.md) "Adding Format Builders" section for the full guide.
