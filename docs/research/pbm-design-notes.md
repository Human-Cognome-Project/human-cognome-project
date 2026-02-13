# PBM Design Notes

**From:** Orchestrator (discussion with Project Lead)
**Date:** 2026-02-13
**Status:** Pre-specialist planning notes

---

## What a PBM Is

A PBM (Pattern-Based Memory) is a **faithful, lossless encoding of content** in HCP's own token representation. It is a strict formula for exact reproduction of the source content. Nothing more.

- A PBM does not interpret, analyze, or extract meaning
- A PBM does not store the source file format — only the content
- A PBM is format-agnostic in both directions (any format in, any format out)
- The same PBM can be reconstructed into formats that didn't exist at ingestion time

## Three-Layer Architecture

1. **PBM** — the raw content map. Token references and inline markers. Deterministic reproduction of exact content.
2. **Metadata** — contextual connections. Provenance, relationships to other PBMs, classification, document-specific formatting conventions. Metadata says "poll this PBM for these contexts."
3. **Engine processing** — meaning. Computed from the PBM using force patterns, concept mesh, and metadata. Never stored in the PBM itself.

## What PBMs Store vs. Compute

### Stored in the PBM
- Word/content tokens (references into language shard)
- Inline formatting markers (markdown-style: bold, italic, underline, sub/superscript, all-caps)
- `<sic>` tags for source-faithful anomalies (misspellings, OCR artifacts, non-standard spellings)
- Non-text content references (images, embedded objects — preserved even if not yet interpretable)

### Computed by Reconstruction Rules (not stored)
- Inter-word whitespace (deterministic from token sequence)
- Positional capitalization (structural — "after closing punctuation, capitalize first alphanumeric of next word")
- Line breaks, paragraph breaks, page breaks (structural markers, not raw characters)
- Right-justification (calculable from content and alignment marker)
- Centering (calculable from content and alignment marker)

### Structural Markers in the PBM
- Indent level markers (tab level N — never raw spaces or tab characters)
- Alignment markers (center, right-align) — block-level, output context handles geometry
- The PBM stores structural intent, not spatial rendering. All geometry is deferred to output.

### Indentation / Tab Level Handling

Two cases at ingestion:
- **Source has native tab characters**: tab level 1 = "tab" (default). Metadata notes native tabs.
- **Source has space-based indentation**: encoder detects the pattern (e.g., 4 spaces = level 1, 8 = level 2), creates ordered tab levels. Metadata records "tab level 1 = 4 spaces, tab level 2 = 8 spaces" for exact-original reproduction if needed.

The PBM always stores "tab level N" — never raw tabs or spaces. Levels are ordered so nesting makes sense. Metadata preserves what each level meant in the original source. Reconstruction uses metadata for exact reproduction, or ignores it when outputting to a different context.

### Stored in Document Metadata (not in PBM)
- Tab level definitions (what each level meant in the original source)
- Formatting conventions (alignment defaults, other rendering context)
- Provenance (source URL, file path, acquisition date)
- Contextual relevance pointers ("poll this PBM for these contexts")
- Relationships to other PBMs (e.g., "extracted content PBM" → "source HTML PBM")
- Document-specific reconstruction rules

## Ingestion / Storage / Output Separation

- **Ingestion**: Format-specific encoders read source files (HTML, PDF, plain text, DOCX, etc.). The encoder strips the file format container and produces a PBM in HCP's token representation. Format compatibility is an ingestion concern only.
- **Storage**: Format-agnostic. Content stored as HCP token sequences. The file format is gone — only content remains.
- **Output**: Reconstruction targets any format needed. Same content can be rendered as HTML, plain text, PDF, or anything else. Output format is an output concern only.

The PBM is the content in its purest tokenized form. What carried it in and what carries it out are both external concerns.

## Long-Term Vision

Everything lives in the databases. The long-term goal is an incredibly simple download — the engine binary and the databases. That's it. No config files, no asset bundles, no templates.

- Reconstruction rules → stored in the DBs as tokenized callable formulas
- Force patterns, encoder logic, formatting conventions → all in the DBs
- Common routines shared in core, domain-specific rules in appropriate shards
- The engine is generic; the databases define what it does

The PBM specialist focuses on the storage format and basic encoding. The rule-storage architecture grows as patterns emerge from real encoding work.

## Key Distinction: Source vs. Derived PBMs

A PBM always represents ONE specific thing as it actually exists.

- **PBM of a web page** = the raw HTML source, token for token
- **PBM of the article content** = the extracted text content (a separate, derived PBM)
- Metadata links them: "this content PBM was extracted from that source PBM"

Processing/extraction produces new PBMs. The source PBM is never modified.

## Capitalization Handling

- **Positional capitalization** (sentence-initial, after punctuation, etc.): reconstruction rules, not stored. Pattern logic: "after X, do Y on next character."
- **Label tokens** (proper nouns, names): capitalized form is the primary spelling in the token itself. PBM just references the token.
- **Non-structural capitalization** (ALL CAPS, brand names like "iPhone"): inline markers. ALL CAPS gets its own marker convention. Mixed-case brand names stored as their token form.
- **Misspellings / anomalies**: `<sic>` tag. Faithful reproduction with explicit anomaly marking.

## Formatting Markers

Inline formatting uses markdown-style conventions within the PBM:
- Bold, italic, underline, sub/superscript — standard markdown or extended conventions
- ALL CAPS — dedicated marker (convention TBD)
- Alignment (center, right-justify) — structural marker at block level, stored in PBM or document metadata depending on scope

These are reconstruction instructions, not content. The engine can strip them for analysis or factor them in as emphasis/formatting signals.

## Namespace Allocation

- zA namespace already allocated for source PBMs
  - zA.AB.C* = Books
  - zA.AB.A* = Tables
  - zA.AB.B* = Dictionaries
- PBM tokens reference content tokens from language shards (AB namespace for English)
- Inline markers and structural tokens from core (AA namespace)
- Proper noun resolution connects to label tokens in language shards
- y* namespace reserved for proper nouns/abbreviations — future per-expression-mode expansion

## Open Questions for PBM Specialist

1. **Exact PBM storage format**: How are token sequences physically stored? Row-per-token in a table? Packed binary? What's the schema for the zA shard?
2. **Non-text content**: How to preserve images, embedded objects, binary content in a PBM. Raw blob references? Separate storage with position markers?
3. **Inline marker token allocation**: Which AA.AB tokens exist for formatting markers? What needs to be added?
4. **`<sic>` and annotation token family**: Define the set of inline annotation tokens. `<sic>`, `<emph>`, others?
5. **Reconstruction rule engine**: Where do reconstruction rules live? Per-document metadata? Per-format defaults? How are they versioned?
6. **Derived PBM lineage**: Schema for tracking source → derived PBM relationships
7. **Encoder framework**: Architecture for pluggable format-specific encoders. What's the common interface?
8. **Scale considerations**: PBM shard sizing. 2GB soft target per shard — how many PBMs fit? What triggers a new shard?

## Specialist Staging (Suggested)

**Phase 1**: PBM schema design — define the storage tables in zA, the metadata schema, the relationship model between PBMs
**Phase 2**: Text encoder — plain text ingestion producing PBMs with reconstruction verification (round-trip: text → PBM → text, exact match)
**Phase 3**: Reconstruction rules engine — whitespace, capitalization, indentation, formatting
**Phase 4**: Markup encoder — HTML/markdown ingestion, content extraction, source vs. derived PBM linking
**Phase 5**: Integration — connect PBM metadata to force patterns and concept mesh for engine polling
