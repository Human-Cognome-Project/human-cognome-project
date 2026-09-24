# Data sources

This directory holds manifests and lightweight acquisition helpers for external source material used by HCP. Large downloaded data belongs under `sources/data/` and is gitignored.

This is a **source inventory**, not an engine or schema specification. Historical source ideas may survive longer than the storage structures that first motivated them; current ingestion/storage rules are documented in `docs/data-protocol.md` and current database code lives under `kernels/database/`.

## Source families

### Kaikki / Wiktionary extracts
- **What:** machine-readable Wiktionary-derived lexical data.
- **Potential use:** definitions, senses, etymology, cross-lingual relationships and other explicit lexical structure.
- **Source:** Kaikki/Wiktionary.
- **Status:** established project source family; ingestion into any current schema must follow the current data protocol rather than older bonding-table assumptions.

### Text corpora
- **What:** authored text with recoverable provenance, including the Gutenberg holdings under `data/gutenberg/`.
- **Use:** language/content ingestion and reconstruction/reading experiments.
- **Status:** source material, not a pre-tokenized authority.

### Frequency/statistical reference data
- **What:** language-frequency datasets where useful.
- **Use:** optional observational/reference evidence.
- **Status:** no current source is canonically selected; statistical frequency is not a substitute for explicit structure or field calculation.

### Natural Semantic Metalanguage reference
- **What:** NSM semantic primes/molecules and cross-linguistic reference material.
- **Use:** semantic decomposition/reference work.
- **Status:** source/reference domain; no obsolete namespace assignment is implied by this page.

### Encoding standards
- **What:** Unicode/UTF-8 and related character/encoding tables.
- **Use:** exact representation and boundary interpretation.
- **Status:** standards/reference material.

## Rules

- Preserve provenance and ordering.
- Do not embed credentials or private source locations in source manifests.
- Do not turn a source-specific representation into a canonical HCP structure merely because an importer uses it.
- Acquisition helpers may use Python or shell where convenient; source acquisition is offline/bootstrap I/O, not an engine hot path.
