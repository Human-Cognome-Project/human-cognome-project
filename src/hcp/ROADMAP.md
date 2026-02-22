# Python Tools Roadmap

Python code in `src/hcp/` serves as reference implementations, ingestion tooling, and test infrastructure. The primary engine is C++ (O3DE + PhysX 5). Python is NOT the runtime — it's tooling.

## Current Modules

| Module | Purpose | Status |
|--------|---------|--------|
| `core/` | Base-50 Token ID encoding, byte codes | Stable |
| `db/` | PostgreSQL connectors (core, english, names, pbm) | Stable |
| `ingest/` | Kaikki dictionary ingestion, Gutenberg text ingestion, batch encoding | Working |
| `engine/` | Reference pipeline (tokenizer, vocab, storage, disassemble, reassemble, validate) | Reference only — C++ Gem is authoritative |
| `cache/` | Cache miss resolver (Python reference for C++ implementation) | Reference only |
| `reconstruction/` | Spacing rules for text reconstruction | Working |

## Phases

### Phase 1 (current): Support Engine Development
- Keep Python reference implementations in sync with C++ Gem decisions
- Ingestion scripts for populating Postgres databases
- Test tooling for validating engine output

### Phase 2: Contributor Tooling
- Format-specific text extractors (PDF, EPUB, HTML, Markdown)
- Batch processing scripts for large document sets
- Validation and comparison tools

### Phase 3: Standalone Tools
- SQLite-backed vocabulary tools (no Postgres dependency)
- Document inspection and debugging utilities
- PBM visualization helpers

## Contributor Expansion Points

| Task | Difficulty | What You'll Learn |
|------|-----------|------------------|
| **PDF text extractor** | Easy | pdftotext + existing tokenizer pipeline |
| **EPUB text extractor** | Easy | XML parsing + existing pipeline |
| **HTML text extractor** | Easy | Beautiful Soup + existing pipeline |
| **Markdown text extractor** | Easy | Minimal parsing + existing pipeline |
| **Wikipedia dump builder** | Medium | XML streaming + existing pipeline |
| **Test coverage expansion** | Medium | Edge cases for tokenizer + reconstruction |
| **Batch ingestion optimization** | Medium | PostgreSQL bulk operations |

All format builders follow the same pattern: extract plain text → feed to existing tokenizer pipeline. See [hcp-engine/AGENTS.md](../hcp-engine/AGENTS.md) for the format builder guide.
