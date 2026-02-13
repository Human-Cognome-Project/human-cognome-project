# PBM Format Specification

**Author:** hcp_pbm (PBM Specialist)
**Date:** 2026-02-13
**Status:** Draft specification — ready for DB specialist review
**Depends on:** pbm-design-notes.md, Decision 005 (decomposed references)

---

## 1. Overview

A PBM (Pattern-Based Memory) is a faithful, lossless encoding of content in HCP's own token representation. This document specifies:

1. How PBM content is physically stored (Section 2)
2. What structural marker tokens are needed (Section 3)
3. How the zA namespace organizes PBM storage (Section 4)
4. What metadata accompanies a PBM (Section 5)
5. The complete database schema for hcp_en_pbm (Section 6)

### Design Principles

- **Content, not format.** PBMs store tokenized content. The source file format is stripped at ingestion; the output format is chosen at reconstruction. The PBM is format-agnostic in both directions.
- **Store what can't be computed.** Inter-word whitespace, positional capitalization, line breaks, and alignment geometry are all deterministic from the token sequence plus structural markers. They are NOT stored.
- **Decomposed references everywhere.** Per Decision 005, every token reference uses the (ns, p2, p3, p4, p5) decomposed pair structure with a generated `token_id` column. No monolithic ID strings. No JSONB arrays. No TEXT[] arrays.
- **Immutable source PBMs.** A source PBM is never modified after creation. Processing or extraction produces new (derived) PBMs linked to the source via metadata.

---

## 2. PBM Content Structure

### 2.1 The Content Stream

A PBM's content is an **ordered sequence of token references**. Each element in the sequence is a reference to either:

- A **content token** from a language shard (e.g., AB.AB.CA.Ec.xn — an English word)
- A **structural marker token** from hcp_core (e.g., AA.AE.AA.AB — paragraph break)
- An **inline formatting token** from hcp_core (e.g., AA.AE.AB.AA — bold start)
- A **character token** from hcp_core (e.g., AA.AA.AA.AA.Bj — a specific byte/character)

The content stream is flat. All hierarchy (chapters, sections, paragraphs) is encoded through structural marker tokens embedded in the sequence at the appropriate positions. There is no tree structure in storage — the tree is implicit in the marker nesting and is reconstructed by the engine.

### 2.2 Storage: Row-Per-Token Junction Table

The content stream is stored as one row per token per position in a junction table:

```
pbm_content
├── PBM reference (decomposed): identifies WHICH document
├── position (INTEGER): identifies WHERE in the sequence
└── Token reference (decomposed): identifies WHAT token
```

Position values are sequential integers starting at 1. Gaps are permitted (to allow insertions in derived PBMs without renumbering) but the canonical ordering is always by position.

### 2.3 Reconstruction from the Content Stream

Given a PBM's content stream (ordered by position), reconstruction proceeds left to right:

1. **Content tokens** emit their surface form (looked up from the language shard)
2. **Structural markers** trigger formatting actions (paragraph break → newline+newline, indent_level_2 → apply indent, etc.)
3. **Inline markers** toggle formatting state (bold_start → begin bold, bold_end → end bold)
4. **Inter-word whitespace** is inserted deterministically between adjacent content tokens (default: single space; suppressed before punctuation, after opening brackets, etc.)
5. **Positional capitalization** is applied by rule (capitalize first alphanumeric after sentence-ending punctuation)

The reconstruction rules are not stored in the PBM. They will live in the engine's rule tables (future work, stored in the databases as tokenized callable formulas per the design notes).

### 2.4 Example: A Simple Paragraph

Source text:
> The **quick** brown fox jumped over the lazy dog. It was *very* fast.

PBM content stream (conceptual):

| Pos | Token | Type |
|-----|-------|------|
| 1 | paragraph_start | structural |
| 2 | `the` | content (AB.AB) |
| 3 | bold_start | inline format |
| 4 | `quick` | content (AB.AB) |
| 5 | bold_end | inline format |
| 6 | `brown` | content (AB.AB) |
| 7 | `fox` | content (AB.AB) |
| 8 | `jumped` | content (AB.AB) |
| 9 | `over` | content (AB.AB) |
| 10 | `the` | content (AB.AB) |
| 11 | `lazy` | content (AB.AB) |
| 12 | `dog` | content (AB.AB) |
| 13 | `.` | content (AB.AB — punctuation token) |
| 14 | `it` | content (AB.AB) |
| 15 | `was` | content (AB.AB) |
| 16 | italic_start | inline format |
| 17 | `very` | content (AB.AB) |
| 18 | italic_end | inline format |
| 19 | `fast` | content (AB.AB) |
| 20 | `.` | content (AB.AB) |
| 21 | paragraph_end | structural |

Notes:
- "The" at position 2 is stored as lowercase `the`. Capitalization is positional (start of paragraph) — computed, not stored.
- "It" at position 14 is stored as lowercase `it`. Capitalization is positional (after sentence-ending `.`) — computed, not stored.
- Whitespace between tokens is not stored. The reconstruction engine inserts spaces by default, suppresses before `.`, etc.
- Bold/italic markers are paired start/end tokens. They carry no content — they're reconstruction instructions.

---

## 3. Structural Marker Token Set

### 3.1 Namespace Allocation

New structural marker tokens for PBM use are allocated under **AA.AE** (a new p2 category in hcp_core). This separates PBM/document structural tokens from the conceptual mesh (AA.AC) and encoding infrastructure (AA.AD).

```
AA.AE                   PBM/Document Structural Tokens
├── AA.AE.AA             Block-level markers
├── AA.AE.AB             Inline formatting markers
├── AA.AE.AC             Annotation markers
├── AA.AE.AD             Alignment and layout markers
└── AA.AE.AE             Non-text content markers
```

### 3.2 Block-Level Markers (AA.AE.AA)

These mark document structure boundaries. They appear in the content stream at positions where structure changes.

| Token ID | Name | Description |
|----------|------|-------------|
| AA.AE.AA.AA | document_start | Marks the beginning of a PBM content stream |
| AA.AE.AA.AB | document_end | Marks the end of a PBM content stream |
| AA.AE.AA.AC | part_break | Part/volume division |
| AA.AE.AA.AD | chapter_break | Chapter division |
| AA.AE.AA.AE | section_break | Section division (h2 equivalent) |
| AA.AE.AA.AF | subsection_break | Subsection division (h3 equivalent) |
| AA.AE.AA.AG | subsubsection_break | Sub-subsection (h4 equivalent) |
| AA.AE.AA.AH | minor_break | Minor division (h5/h6 equivalent) |
| AA.AE.AA.AI | paragraph_start | Beginning of a paragraph |
| AA.AE.AA.AJ | paragraph_end | End of a paragraph |
| AA.AE.AA.AK | line_break | Forced line break within a paragraph |
| AA.AE.AA.AL | page_break | Page break (when meaningful in source) |
| AA.AE.AA.AM | horizontal_rule | Thematic break / horizontal rule |
| AA.AE.AA.AN | block_quote_start | Beginning of a block quotation |
| AA.AE.AA.AP | block_quote_end | End of a block quotation |
| AA.AE.AA.AQ | list_ordered_start | Beginning of an ordered list |
| AA.AE.AA.AR | list_ordered_end | End of an ordered list |
| AA.AE.AA.AS | list_unordered_start | Beginning of an unordered list |
| AA.AE.AA.AT | list_unordered_end | End of an unordered list |
| AA.AE.AA.AU | list_item_start | Beginning of a list item |
| AA.AE.AA.AV | list_item_end | End of a list item |
| AA.AE.AA.AW | table_start | Beginning of a table |
| AA.AE.AA.AX | table_end | End of a table |
| AA.AE.AA.AY | table_row_start | Beginning of a table row |
| AA.AE.AA.AZ | table_row_end | End of a table row |
| AA.AE.AA.Aa | table_cell_start | Beginning of a table cell |
| AA.AE.AA.Ab | table_cell_end | End of a table cell |
| AA.AE.AA.Ac | table_header_cell_start | Beginning of a table header cell |
| AA.AE.AA.Ad | table_header_cell_end | End of a table header cell |
| AA.AE.AA.Ae | code_block_start | Beginning of a code/preformatted block |
| AA.AE.AA.Af | code_block_end | End of a code/preformatted block |
| AA.AE.AA.Ag | title_start | Document/section title start |
| AA.AE.AA.Ah | title_end | Document/section title end |

**32 block-level markers.**

Division markers (part_break through minor_break) are standalone — they mark a boundary point. Container markers (paragraph, block_quote, list, table, code_block, title) are paired start/end.

### 3.3 Inline Formatting Markers (AA.AE.AB)

These toggle formatting state within a paragraph or block. Always paired start/end.

| Token ID | Name | Description |
|----------|------|-------------|
| AA.AE.AB.AA | bold_start | Begin bold text |
| AA.AE.AB.AB | bold_end | End bold text |
| AA.AE.AB.AC | italic_start | Begin italic text |
| AA.AE.AB.AD | italic_end | End italic text |
| AA.AE.AB.AE | underline_start | Begin underlined text |
| AA.AE.AB.AF | underline_end | End underlined text |
| AA.AE.AB.AG | strikethrough_start | Begin strikethrough text |
| AA.AE.AB.AH | strikethrough_end | End strikethrough text |
| AA.AE.AB.AI | superscript_start | Begin superscript |
| AA.AE.AB.AJ | superscript_end | End superscript |
| AA.AE.AB.AK | subscript_start | Begin subscript |
| AA.AE.AB.AL | subscript_end | End subscript |
| AA.AE.AB.AM | all_caps_start | Begin ALL CAPS rendering |
| AA.AE.AB.AN | all_caps_end | End ALL CAPS rendering |
| AA.AE.AB.AP | small_caps_start | Begin small caps rendering |
| AA.AE.AB.AQ | small_caps_end | End small caps rendering |
| AA.AE.AB.AR | code_inline_start | Begin inline code |
| AA.AE.AB.AS | code_inline_end | End inline code |
| AA.AE.AB.AT | link_start | Begin hyperlink span (URL in metadata) |
| AA.AE.AB.AU | link_end | End hyperlink span |
| AA.AE.AB.AV | highlight_start | Begin highlighted/marked text |
| AA.AE.AB.AW | highlight_end | End highlighted/marked text |

**22 inline formatting markers.**

Inline markers nest. `bold_start italic_start ... italic_end bold_end` is valid. The engine resolves nesting during reconstruction.

### 3.4 Annotation Markers (AA.AE.AC)

These mark source-faithful anomalies, editorial annotations, and reference points.

| Token ID | Name | Description |
|----------|------|-------------|
| AA.AE.AC.AA | sic_start | Begin source-faithful anomaly (misspelling, OCR artifact, etc.) |
| AA.AE.AC.AB | sic_end | End source-faithful anomaly |
| AA.AE.AC.AC | footnote_ref | Inline footnote reference point (ordinal in metadata) |
| AA.AE.AC.AD | footnote_start | Begin footnote content |
| AA.AE.AC.AE | footnote_end | End footnote content |
| AA.AE.AC.AF | endnote_ref | Inline endnote reference point |
| AA.AE.AC.AG | endnote_start | Begin endnote content |
| AA.AE.AC.AH | endnote_end | End endnote content |
| AA.AE.AC.AI | aside_start | Begin parenthetical/aside content |
| AA.AE.AC.AJ | aside_end | End parenthetical/aside content |
| AA.AE.AC.AK | redacted | Marks a known redaction in the source |
| AA.AE.AC.AL | tbd | Unresolved token reference placeholder |
| AA.AE.AC.AM | citation_start | Begin inline citation |
| AA.AE.AC.AN | citation_end | End inline citation |

**14 annotation markers.**

The `sic` pair wraps content that is anomalous in the source and must be preserved exactly. The content between sic_start and sic_end is stored character-by-character (AA.AA character tokens) rather than as word tokens, since it may not match any known word.

The `tbd` marker replaces an unresolvable token reference during ingestion. The PBM's metadata records TBD details (see Section 5.4).

### 3.5 Alignment and Layout Markers (AA.AE.AD)

These encode structural intent for block-level layout. The PBM stores the intent; the output context handles geometry.

| Token ID | Name | Description |
|----------|------|-------------|
| AA.AE.AD.AA | align_left | Explicit left alignment (when overriding a non-left default) |
| AA.AE.AD.AB | align_center | Center alignment |
| AA.AE.AD.AC | align_right | Right alignment |
| AA.AE.AD.AD | align_justify | Justified alignment |
| AA.AE.AD.AE | indent_level_1 | Tab level 1 |
| AA.AE.AD.AF | indent_level_2 | Tab level 2 |
| AA.AE.AD.AG | indent_level_3 | Tab level 3 |
| AA.AE.AD.AH | indent_level_4 | Tab level 4 |
| AA.AE.AD.AI | indent_level_5 | Tab level 5 |
| AA.AE.AD.AJ | indent_level_6 | Tab level 6 |
| AA.AE.AD.AK | indent_level_7 | Tab level 7 |
| AA.AE.AD.AL | indent_level_8 | Tab level 8 |
| AA.AE.AD.AM | hanging_indent | Hanging indent (first line outdented) |

**13 alignment/layout markers.**

Alignment markers appear immediately after a block-start marker (e.g., after paragraph_start) and apply to the entire block. If no alignment marker follows a block-start, the default (left-aligned, no indent) applies.

Indent levels are ordered. What each level means in the original source is recorded in document metadata (see Section 5.3). The PBM always stores "indent level N," never raw tabs or spaces.

Eight indent levels should cover the vast majority of content. If more are needed, additional tokens can be allocated at AA.AE.AD.AN+ following the same pattern.

### 3.6 Non-Text Content Markers (AA.AE.AE)

These mark positions where non-textual content appears. The actual content is stored separately (see Section 6.5).

| Token ID | Name | Description |
|----------|------|-------------|
| AA.AE.AE.AA | image_ref | Reference to an image (blob ID in metadata) |
| AA.AE.AE.AB | figure_start | Begin figure (image + caption container) |
| AA.AE.AE.AC | figure_end | End figure |
| AA.AE.AE.AD | caption_start | Begin caption text |
| AA.AE.AE.AE | caption_end | End caption text |
| AA.AE.AE.AF | embedded_object_ref | Reference to an embedded object (blob ID in metadata) |
| AA.AE.AE.AG | math_start | Begin mathematical expression |
| AA.AE.AE.AH | math_end | End mathematical expression |
| AA.AE.AE.AI | audio_ref | Reference to audio content |
| AA.AE.AE.AJ | video_ref | Reference to video content |

**10 non-text content markers.**

### 3.7 Token Set Summary

| Category | Prefix | Count |
|----------|--------|-------|
| Block-level | AA.AE.AA | 32 |
| Inline formatting | AA.AE.AB | 22 |
| Annotations | AA.AE.AC | 14 |
| Alignment/layout | AA.AE.AD | 13 |
| Non-text content | AA.AE.AE | 10 |
| **Total** | **AA.AE** | **91** |

All 91 tokens are universal (not language-specific) and belong in hcp_core.

---

## 4. zA Namespace Organization

### 4.1 Existing Allocation

From namespace_allocations in hcp_core:

```
z*              Source PBMs (mode)
zA              Source PBMs (Universal) — shard: hcp_en_pbm
├── zA.AA       Source PBMs (Universal Direct) — byte-level computational content
│   └── zA.AA.AA.AA.{count}  Encoding table source PBMs
├── zA.AB       Source PBMs (Text Mode) — text-mode content
│   ├── zA.AB.A*   Tables (A=CSV, B=TSV, C=SSV, etc.)
│   ├── zA.AB.B*   Dictionaries
│   └── zA.AB.C*   Books
```

### 4.2 Addressing Convention

The zA.AB 3rd pair uses **double-duty encoding** (1st character = document type, 2nd character = format variant):

| 1st char | Document type | Examples |
|----------|--------------|----------|
| A | Table-form | CSV, TSV, SSV data |
| B | Dictionary-form | Lexicons, glossaries, reference works |
| C | Book-form | Monographs, textbooks, novels |
| D | Article-form | Journal articles, essays, papers |
| E | Correspondence | Letters, emails, messages |
| F-Z, a-z | Reserved | Future document types |

| 2nd char | Variant meaning |
|----------|----------------|
| A | Primary/canonical form |
| B-Z, a-z | Alternate forms, editions, or format variants |

### 4.3 Document Addressing

A complete PBM token ID:

```
zA.AB.CA.AA.AA
│  │  ││ │  └── Specific document/sub-document within series
│  │  ││ └───── Document series or collection
│  │  │└─────── Format variant (A = primary)
│  │  └──────── Document type (C = book)
│  └─────────── Text mode content
└────────────── Source PBM mode
```

The 4th and 5th pairs address individual documents and subdivisions within a document type + variant. With 2,500 values per pair:
- Pair 4 alone: 2,500 document series per type+variant
- Pairs 4+5: 6.25 million individual documents per type+variant

### 4.4 Derived PBMs

A derived PBM (e.g., extracted article content from an HTML page) gets its own token ID in the zA namespace. It is a first-class PBM — structurally identical to a source PBM. The derivation relationship is recorded in metadata (see Section 5.5), not in the address.

This means a book and its extracted chapter are two separate PBMs with two separate token IDs and two separate content streams. They are connected by a relationship record, not by address hierarchy.

---

## 5. Metadata Schema

### 5.1 Metadata Architecture

PBM metadata lives in **separate tables**, not in the PBM content stream. This follows the three-layer architecture: PBM = content, metadata = context, engine = meaning.

Metadata is organized into:

1. **Document provenance** — where the content came from
2. **Formatting conventions** — how the source was formatted
3. **Tab level definitions** — what indent levels meant in the original
4. **TBD resolution log** — unresolved references during ingestion
5. **Document relationships** — links between PBMs
6. **Position-anchored metadata** — metadata attached to specific content positions (link URLs, footnote ordinals, blob IDs for non-text content)

### 5.2 Document Provenance

Stored per document. One row per provenance fact.

| Field | Description | Example |
|-------|-------------|---------|
| source_type | How the content was acquired | `file`, `url`, `api`, `manual` |
| source_path | Original file path or URL | `/data/books/analysing-sentences.pdf` |
| source_format | Original file format | `pdf`, `html`, `txt`, `docx` |
| acquisition_date | When the content was acquired | `2026-02-13` |
| source_checksum | SHA-256 of the original source file | `a1b2c3...` |
| encoder_version | Version of the encoder that produced this PBM | `plaintext-v1` |
| content_language | Primary language of the content | References a token in AA or AB namespace |
| rights_status | Copyright/licensing status | `public_domain`, `fair_use`, `licensed`, `unknown` |

### 5.3 Tab Level Definitions

Per-document mapping of indent levels to their original source representation. Stored in a dedicated table (not JSONB).

| Field | Description |
|-------|-------------|
| document (decomposed ref) | Which PBM this applies to |
| indent_level | Integer 1-8 (matches indent_level_N markers) |
| original_representation | What this level meant in the source: `tab`, `2_spaces`, `4_spaces`, etc. |
| semantic_role | Optional: `code_indent`, `quote_nesting`, `list_nesting`, etc. |

For exact-original reproduction, the reconstruction engine consults this table. For generic output, it ignores it and uses context-appropriate indentation.

### 5.4 TBD Resolution Log

When ingestion encounters a reference it cannot resolve to an existing token, it inserts a `tbd` marker at that position and logs the unresolved reference.

| Field | Description |
|-------|-------------|
| document (decomposed ref) | Which PBM |
| position | Position in the content stream where the TBD marker sits |
| tbd_index | Sequential index within the ingestion run (TBD1, TBD2, ...) |
| original_text | The original surface text that couldn't be resolved |
| resolution_status | `unresolved`, `resolved`, `abandoned` |
| resolved_to (decomposed ref) | Token ID when resolved (NULL until resolution) |
| resolved_date | When resolution occurred |

When a resolving source is later ingested, a sweep replaces the `tbd` marker in the content stream with the resolved token and updates this log.

### 5.5 Document Relationships

Links between PBMs recording derivation, extraction, and other structural relationships.

| Field | Description |
|-------|-------------|
| source_pbm (decomposed ref) | The source PBM |
| target_pbm (decomposed ref) | The derived/related PBM |
| relationship_type | `extracted_from`, `derived_from`, `revision_of`, `translation_of`, `references`, `part_of` |
| extraction_range_start | Start position in source (for `extracted_from`) |
| extraction_range_end | End position in source (for `extracted_from`) |
| notes | Free-text description of the relationship |

### 5.6 Position-Anchored Metadata

Some structural markers require additional data tied to a specific position in the content stream. Examples:
- `link_start` needs a URL
- `footnote_ref` needs an ordinal or label
- `image_ref` needs a blob reference
- `code_block_start` may need a language identifier

This is stored in a position-anchored metadata table:

| Field | Description |
|-------|-------------|
| document (decomposed ref) | Which PBM |
| position | Position in the content stream |
| key | Metadata key (`url`, `ordinal`, `blob_id`, `language`, `alt_text`, etc.) |
| value | Metadata value (TEXT) |

This avoids putting metadata into the content stream or using JSONB. One row per key-value pair per position.

---

## 6. Database Schema for hcp_en_pbm

### 6.1 Database Creation

The `hcp_en_pbm` database does not yet exist. It needs to be created and registered in the shard_registry (which already has a zA entry pointing to `hcp_en_pbm`).

### 6.2 Table: tokens (Document Registry)

Every PBM is a token in the zA namespace. This table follows the standard HCP token table pattern.

```sql
CREATE TABLE tokens (
    ns          TEXT NOT NULL,
    p2          TEXT,
    p3          TEXT,
    p4          TEXT,
    p5          TEXT,
    token_id    TEXT NOT NULL GENERATED ALWAYS AS (
                    ns || COALESCE('.' || p2, '')
                       || COALESCE('.' || p3, '')
                       || COALESCE('.' || p4, '')
                       || COALESCE('.' || p5, '')
                ) STORED,
    name        TEXT NOT NULL,           -- Human-readable document title
    category    TEXT,                     -- Document type: 'book', 'article', 'table', 'dictionary', etc.
    subcategory TEXT,                     -- Sub-type: 'textbook', 'novel', 'reference', etc.
    metadata    JSONB NOT NULL DEFAULT '{}'::jsonb,  -- Lightweight properties (kept minimal)

    CONSTRAINT tokens_pkey PRIMARY KEY (token_id)
);

CREATE INDEX idx_tokens_ns ON tokens (ns);
CREATE INDEX idx_tokens_ns_p2 ON tokens (ns, p2);
CREATE INDEX idx_tokens_prefix ON tokens (ns, p2, p3, p4, p5);
CREATE INDEX idx_tokens_name ON tokens (name);
CREATE INDEX idx_tokens_category ON tokens (category);
```

**Note:** The `metadata` JSONB column on the tokens table is for lightweight, rarely-queried document properties only (e.g., page count, word count estimates). All structured metadata goes in the dedicated metadata tables below.

### 6.3 Table: pbm_content (The Content Stream)

The main table. Stores the ordered token sequence for every PBM. This will be the largest table in the shard.

```sql
CREATE TABLE pbm_content (
    -- Which PBM this content belongs to (decomposed reference)
    doc_ns      TEXT NOT NULL,
    doc_p2      TEXT,
    doc_p3      TEXT,
    doc_p4      TEXT,
    doc_p5      TEXT,
    doc_id      TEXT NOT NULL GENERATED ALWAYS AS (
                    doc_ns || COALESCE('.' || doc_p2, '')
                             || COALESCE('.' || doc_p3, '')
                             || COALESCE('.' || doc_p4, '')
                             || COALESCE('.' || doc_p5, '')
                ) STORED,

    -- Position in the content stream
    position    INTEGER NOT NULL,

    -- What token is at this position (decomposed reference)
    ns          TEXT NOT NULL,
    p2          TEXT,
    p3          TEXT,
    p4          TEXT,
    p5          TEXT,
    token_id    TEXT NOT NULL GENERATED ALWAYS AS (
                    ns || COALESCE('.' || p2, '')
                       || COALESCE('.' || p3, '')
                       || COALESCE('.' || p4, '')
                       || COALESCE('.' || p5, '')
                ) STORED,

    CONSTRAINT pbm_content_pkey PRIMARY KEY (doc_ns, doc_p2, doc_p3, doc_p4, doc_p5, position)
);

-- For reading a document's content in order
CREATE INDEX idx_pbm_content_doc ON pbm_content (doc_ns, doc_p2, doc_p3, doc_p4, doc_p5, position);

-- For finding all PBMs that reference a specific token
CREATE INDEX idx_pbm_content_token ON pbm_content (ns, p2, p3, p4, p5);

-- For the generated doc_id (cross-shard joins)
CREATE INDEX idx_pbm_content_doc_id ON pbm_content (doc_id);
```

**Scale estimate:** A 300-page book averages ~80,000 words. With structural markers, ~100,000 rows per book. The 2GB shard target supports roughly 3,000-5,000 books before needing a new shard, depending on structural complexity.

### 6.4 Table: document_provenance

```sql
CREATE TABLE document_provenance (
    id              SERIAL PRIMARY KEY,

    -- Which PBM (decomposed reference)
    doc_ns          TEXT NOT NULL,
    doc_p2          TEXT,
    doc_p3          TEXT,
    doc_p4          TEXT,
    doc_p5          TEXT,
    doc_id          TEXT NOT NULL GENERATED ALWAYS AS (
                        doc_ns || COALESCE('.' || doc_p2, '')
                                 || COALESCE('.' || doc_p3, '')
                                 || COALESCE('.' || doc_p4, '')
                                 || COALESCE('.' || doc_p5, '')
                    ) STORED,

    source_type     TEXT NOT NULL,        -- 'file', 'url', 'api', 'manual'
    source_path     TEXT,                 -- Original file path or URL
    source_format   TEXT,                 -- 'pdf', 'html', 'txt', 'docx', etc.
    acquisition_date DATE,
    source_checksum TEXT,                 -- SHA-256 of original source
    encoder_version TEXT,                 -- Encoder that produced this PBM

    -- Content language (decomposed token reference)
    lang_ns         TEXT,
    lang_p2         TEXT,
    lang_p3         TEXT,
    lang_p4         TEXT,
    lang_p5         TEXT,
    lang_token_id   TEXT GENERATED ALWAYS AS (
                        lang_ns || COALESCE('.' || lang_p2, '')
                                 || COALESCE('.' || lang_p3, '')
                                 || COALESCE('.' || lang_p4, '')
                                 || COALESCE('.' || lang_p5, '')
                    ) STORED,

    rights_status   TEXT                  -- 'public_domain', 'fair_use', 'licensed', 'unknown'
);

CREATE INDEX idx_provenance_doc ON document_provenance (doc_ns, doc_p2, doc_p3, doc_p4, doc_p5);
CREATE INDEX idx_provenance_doc_id ON document_provenance (doc_id);
```

### 6.5 Table: non_text_content

Binary storage for images, embedded objects, and other non-textual content referenced by non-text markers.

```sql
CREATE TABLE non_text_content (
    id              SERIAL PRIMARY KEY,
    blob_id         TEXT NOT NULL UNIQUE,  -- Deterministic reference ID (e.g., SHA-256 of content)
    content_type    TEXT NOT NULL,         -- MIME type: 'image/png', 'application/pdf', etc.
    content         BYTEA NOT NULL,        -- The raw binary content
    original_name   TEXT,                  -- Original filename if known
    width_px        INTEGER,              -- For images: width in pixels
    height_px       INTEGER,              -- For images: height in pixels
    size_bytes      BIGINT NOT NULL       -- Size of the content in bytes
);

CREATE INDEX idx_non_text_blob_id ON non_text_content (blob_id);
```

Binary content is deduplicated by blob_id (content hash). Multiple PBMs can reference the same blob. The `image_ref`, `embedded_object_ref`, `audio_ref`, and `video_ref` markers in the content stream connect to this table via position-anchored metadata (key=`blob_id`).

**Shard sizing note:** Large binary content (high-resolution images, video, audio) will consume shard space quickly. For PBMs with heavy binary content, consider creating dedicated shards (e.g., zA.AB.CA for text-heavy books, a separate shard for image-heavy books). The shard_registry in hcp_core handles routing.

### 6.6 Table: tab_definitions

```sql
CREATE TABLE tab_definitions (
    -- Which PBM (decomposed reference)
    doc_ns                  TEXT NOT NULL,
    doc_p2                  TEXT,
    doc_p3                  TEXT,
    doc_p4                  TEXT,
    doc_p5                  TEXT,
    doc_id                  TEXT NOT NULL GENERATED ALWAYS AS (
                                doc_ns || COALESCE('.' || doc_p2, '')
                                         || COALESCE('.' || doc_p3, '')
                                         || COALESCE('.' || doc_p4, '')
                                         || COALESCE('.' || doc_p5, '')
                            ) STORED,

    indent_level            SMALLINT NOT NULL,  -- 1-8 (matches indent_level_N markers)
    original_representation TEXT NOT NULL,       -- 'tab', '2_spaces', '4_spaces', etc.
    semantic_role           TEXT,                -- 'code_indent', 'quote_nesting', 'list_nesting', etc.

    CONSTRAINT tab_definitions_pkey PRIMARY KEY (doc_ns, doc_p2, doc_p3, doc_p4, doc_p5, indent_level)
);
```

### 6.7 Table: tbd_log

```sql
CREATE TABLE tbd_log (
    id                  SERIAL PRIMARY KEY,

    -- Which PBM (decomposed reference)
    doc_ns              TEXT NOT NULL,
    doc_p2              TEXT,
    doc_p3              TEXT,
    doc_p4              TEXT,
    doc_p5              TEXT,
    doc_id              TEXT NOT NULL GENERATED ALWAYS AS (
                            doc_ns || COALESCE('.' || doc_p2, '')
                                     || COALESCE('.' || doc_p3, '')
                                     || COALESCE('.' || doc_p4, '')
                                     || COALESCE('.' || doc_p5, '')
                        ) STORED,

    position            INTEGER NOT NULL,         -- Position in content stream
    tbd_index           INTEGER NOT NULL,         -- Sequential within ingestion run
    original_text       TEXT NOT NULL,             -- The unresolvable surface text
    resolution_status   TEXT NOT NULL DEFAULT 'unresolved',  -- 'unresolved', 'resolved', 'abandoned'

    -- Resolved token (decomposed reference, NULL until resolved)
    resolved_ns         TEXT,
    resolved_p2         TEXT,
    resolved_p3         TEXT,
    resolved_p4         TEXT,
    resolved_p5         TEXT,
    resolved_token_id   TEXT GENERATED ALWAYS AS (
                            resolved_ns || COALESCE('.' || resolved_p2, '')
                                         || COALESCE('.' || resolved_p3, '')
                                         || COALESCE('.' || resolved_p4, '')
                                         || COALESCE('.' || resolved_p5, '')
                        ) STORED,

    resolved_date       TIMESTAMP WITH TIME ZONE
);

CREATE INDEX idx_tbd_log_doc ON tbd_log (doc_ns, doc_p2, doc_p3, doc_p4, doc_p5);
CREATE INDEX idx_tbd_log_status ON tbd_log (resolution_status);
```

### 6.8 Table: document_relationships

```sql
CREATE TABLE document_relationships (
    id                      SERIAL PRIMARY KEY,

    -- Source PBM (decomposed reference)
    source_ns               TEXT NOT NULL,
    source_p2               TEXT,
    source_p3               TEXT,
    source_p4               TEXT,
    source_p5               TEXT,
    source_id               TEXT NOT NULL GENERATED ALWAYS AS (
                                source_ns || COALESCE('.' || source_p2, '')
                                           || COALESCE('.' || source_p3, '')
                                           || COALESCE('.' || source_p4, '')
                                           || COALESCE('.' || source_p5, '')
                            ) STORED,

    -- Target PBM (decomposed reference)
    target_ns               TEXT NOT NULL,
    target_p2               TEXT,
    target_p3               TEXT,
    target_p4               TEXT,
    target_p5               TEXT,
    target_id               TEXT NOT NULL GENERATED ALWAYS AS (
                                target_ns || COALESCE('.' || target_p2, '')
                                           || COALESCE('.' || target_p3, '')
                                           || COALESCE('.' || target_p4, '')
                                           || COALESCE('.' || target_p5, '')
                            ) STORED,

    relationship_type       TEXT NOT NULL,  -- 'extracted_from', 'derived_from', 'revision_of',
                                           -- 'translation_of', 'references', 'part_of'
    extraction_range_start  INTEGER,        -- Start position in source (for 'extracted_from')
    extraction_range_end    INTEGER,        -- End position in source (for 'extracted_from')
    notes                   TEXT
);

CREATE INDEX idx_docrel_source ON document_relationships (source_ns, source_p2, source_p3, source_p4, source_p5);
CREATE INDEX idx_docrel_target ON document_relationships (target_ns, target_p2, target_p3, target_p4, target_p5);
CREATE INDEX idx_docrel_type ON document_relationships (relationship_type);
```

### 6.9 Table: position_metadata

```sql
CREATE TABLE position_metadata (
    -- Which PBM (decomposed reference)
    doc_ns      TEXT NOT NULL,
    doc_p2      TEXT,
    doc_p3      TEXT,
    doc_p4      TEXT,
    doc_p5      TEXT,
    doc_id      TEXT NOT NULL GENERATED ALWAYS AS (
                    doc_ns || COALESCE('.' || doc_p2, '')
                             || COALESCE('.' || doc_p3, '')
                             || COALESCE('.' || doc_p4, '')
                             || COALESCE('.' || doc_p5, '')
                ) STORED,

    position    INTEGER NOT NULL,      -- Position in content stream
    key         TEXT NOT NULL,          -- 'url', 'ordinal', 'blob_id', 'language', 'alt_text', etc.
    value       TEXT NOT NULL,          -- The metadata value

    CONSTRAINT position_metadata_pkey PRIMARY KEY (doc_ns, doc_p2, doc_p3, doc_p4, doc_p5, position, key)
);

CREATE INDEX idx_posmeta_doc ON position_metadata (doc_ns, doc_p2, doc_p3, doc_p4, doc_p5);
```

### 6.10 Schema Summary

| Table | Purpose | Expected scale |
|-------|---------|----------------|
| tokens | Document registry | 1 row per PBM (thousands) |
| pbm_content | Content token sequences | ~100K rows per document (millions total) |
| document_provenance | Source tracking | 1-few rows per PBM |
| non_text_content | Binary blobs | Varies — deduplicated by hash |
| tab_definitions | Indent level meanings | 0-8 rows per PBM |
| tbd_log | Unresolved reference tracking | Sparse — ideally near zero |
| document_relationships | PBM-to-PBM links | 0-few rows per PBM |
| position_metadata | Per-position key-value data | Sparse — only where markers need extra data |

---

## 7. Ingestion Workflow

This section describes the logical flow for creating a PBM. Encoder implementation is Phase 2 work; this defines the contract that encoders must satisfy.

### 7.1 Steps

1. **Allocate a PBM token ID** in the zA namespace, following the addressing convention (Section 4).
2. **Strip the file format container.** The encoder reads the source file and extracts content. The file format is an ingestion concern only.
3. **Tokenize content.** Each word, punctuation mark, and structural element is resolved to an existing token in the appropriate shard (AB.AB for English words, AA.AE for structural markers, AA.AA for characters).
4. **Insert structural markers.** Paragraph breaks, section breaks, formatting toggles, alignment markers — all inserted as tokens at the appropriate positions in the content stream.
5. **Handle anomalies.** Unrecognizable words get wrapped in `sic_start`/`sic_end` and stored character-by-character. Unresolvable references get `tbd` markers.
6. **Write the content stream.** Insert all rows into `pbm_content` in a single transaction.
7. **Write metadata.** Provenance, tab definitions, position-anchored metadata, TBD log entries.
8. **Register the document.** Insert the PBM token into the `tokens` table.

### 7.2 Round-Trip Verification

Every encoder MUST support round-trip verification: `source → PBM → reconstructed output` should produce content-identical output (ignoring format-specific differences like HTML tags vs. plain text rendering). This is the primary correctness test.

"Content-identical" means: the same words in the same order with the same formatting intent. Whitespace normalization, capitalization reconstruction, and format-specific rendering are handled by the reconstruction engine and verified separately.

---

## 8. Open Design Questions

These are identified but deferred to later phases:

1. **Reconstruction rule storage.** The design notes specify rules will be "stored in the DBs as tokenized callable formulas." The table structure and formula language are Phase 3 work.

2. **Cross-shard PBM content.** A PBM references tokens from multiple shards (AA for markers, AB for words). The content table stores references — it does not duplicate token data. Cross-shard resolution is a read-time concern. The engine needs a shard routing layer (read ns prefix → look up shard_registry → query correct DB).

3. **Shard splitting.** When hcp_en_pbm exceeds the 2GB target, how is it split? Options: by document type (books vs. articles), by content volume, by time period. Deferred until real data volume requires it.

4. **Concurrent ingestion.** Multiple encoders running simultaneously need token ID allocation coordination. Options: sequence-based allocation with advisory locks, pre-allocated ranges per encoder. Deferred to Phase 2.

5. **PBM versioning.** Source PBMs are immutable. But if an encoder bug is found and PBMs need re-ingestion, how is the old PBM superseded? The `revision_of` relationship type handles this — the new PBM links to the old one. The old PBM is not deleted (it may have derived PBMs pointing to it).

---

## 9. DB Specialist Action Items

The following actions are needed from the DB specialist to implement this specification:

### Phase 1: Foundation (immediate)
1. **Create the `hcp_en_pbm` database** (owner: hcp)
2. **Create all 8 tables** per the schemas in Section 6
3. **Insert the 91 structural marker tokens** (Section 3) into `hcp_core.tokens`
4. **Register the new AA.AE namespace** in `hcp_core.namespace_allocations`
5. **Verify shard_registry** entry for zA → hcp_en_pbm is correct

### Phase 2: Validation (after text encoder is built)
6. **Load a test PBM** (plain text document) and verify round-trip reconstruction
7. **Performance baseline** — measure insert throughput and sequential read speed for pbm_content
8. **Index tuning** — adjust indexes based on actual query patterns

---

## Appendix A: Namespace Allocation Registration

New entries needed in `hcp_core.namespace_allocations`:

```sql
INSERT INTO namespace_allocations (pattern, name, description, alloc_type, parent) VALUES
('AA.AE',       'PBM Structural Tokens',  'Structural marker tokens for PBM document encoding', 'category', 'AA'),
('AA.AE.AA',    'Block-Level Markers',     'Document structure: paragraphs, sections, lists, tables', 'subcategory', 'AA.AE'),
('AA.AE.AB',    'Inline Format Markers',   'Text formatting: bold, italic, underline, etc.', 'subcategory', 'AA.AE'),
('AA.AE.AC',    'Annotation Markers',      'Anomalies, footnotes, citations, editorial marks', 'subcategory', 'AA.AE'),
('AA.AE.AD',    'Alignment/Layout Markers', 'Alignment, indentation, and layout intent', 'subcategory', 'AA.AE'),
('AA.AE.AE',    'Non-Text Content Markers', 'References to images, figures, embedded objects, math', 'subcategory', 'AA.AE');
```

## Appendix B: Full Structural Token INSERT

All 91 tokens for `hcp_core.tokens`. Category is `pbm_marker`, subcategory identifies the marker type.

```sql
-- Block-level markers (AA.AE.AA) — 32 tokens
INSERT INTO tokens (ns, p2, p3, p4, name, category, subcategory) VALUES
('AA', 'AE', 'AA', 'AA', 'document_start',          'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AB', 'document_end',            'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AC', 'part_break',              'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AD', 'chapter_break',           'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AE', 'section_break',           'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AF', 'subsection_break',        'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AG', 'subsubsection_break',     'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AH', 'minor_break',             'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AI', 'paragraph_start',         'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AJ', 'paragraph_end',           'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AK', 'line_break',              'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AL', 'page_break',              'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AM', 'horizontal_rule',         'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AN', 'block_quote_start',       'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AP', 'block_quote_end',         'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AQ', 'list_ordered_start',      'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AR', 'list_ordered_end',        'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AS', 'list_unordered_start',    'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AT', 'list_unordered_end',      'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AU', 'list_item_start',         'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AV', 'list_item_end',           'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AW', 'table_start',             'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AX', 'table_end',               'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AY', 'table_row_start',         'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AZ', 'table_row_end',           'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'Aa', 'table_cell_start',        'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'Ab', 'table_cell_end',          'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'Ac', 'table_header_cell_start', 'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'Ad', 'table_header_cell_end',   'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'Ae', 'code_block_start',        'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'Af', 'code_block_end',          'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'Ag', 'title_start',             'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'Ah', 'title_end',               'pbm_marker', 'block');

-- Inline formatting markers (AA.AE.AB) — 22 tokens
INSERT INTO tokens (ns, p2, p3, p4, name, category, subcategory) VALUES
('AA', 'AE', 'AB', 'AA', 'bold_start',          'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AB', 'bold_end',            'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AC', 'italic_start',        'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AD', 'italic_end',          'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AE', 'underline_start',     'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AF', 'underline_end',       'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AG', 'strikethrough_start', 'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AH', 'strikethrough_end',   'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AI', 'superscript_start',   'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AJ', 'superscript_end',     'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AK', 'subscript_start',     'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AL', 'subscript_end',       'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AM', 'all_caps_start',      'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AN', 'all_caps_end',        'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AP', 'small_caps_start',    'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AQ', 'small_caps_end',      'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AR', 'code_inline_start',   'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AS', 'code_inline_end',     'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AT', 'link_start',          'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AU', 'link_end',            'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AV', 'highlight_start',     'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AW', 'highlight_end',       'pbm_marker', 'inline');

-- Annotation markers (AA.AE.AC) — 14 tokens
INSERT INTO tokens (ns, p2, p3, p4, name, category, subcategory) VALUES
('AA', 'AE', 'AC', 'AA', 'sic_start',      'pbm_marker', 'annotation'),
('AA', 'AE', 'AC', 'AB', 'sic_end',        'pbm_marker', 'annotation'),
('AA', 'AE', 'AC', 'AC', 'footnote_ref',   'pbm_marker', 'annotation'),
('AA', 'AE', 'AC', 'AD', 'footnote_start', 'pbm_marker', 'annotation'),
('AA', 'AE', 'AC', 'AE', 'footnote_end',   'pbm_marker', 'annotation'),
('AA', 'AE', 'AC', 'AF', 'endnote_ref',    'pbm_marker', 'annotation'),
('AA', 'AE', 'AC', 'AG', 'endnote_start',  'pbm_marker', 'annotation'),
('AA', 'AE', 'AC', 'AH', 'endnote_end',    'pbm_marker', 'annotation'),
('AA', 'AE', 'AC', 'AI', 'aside_start',    'pbm_marker', 'annotation'),
('AA', 'AE', 'AC', 'AJ', 'aside_end',      'pbm_marker', 'annotation'),
('AA', 'AE', 'AC', 'AK', 'redacted',       'pbm_marker', 'annotation'),
('AA', 'AE', 'AC', 'AL', 'tbd',            'pbm_marker', 'annotation'),
('AA', 'AE', 'AC', 'AM', 'citation_start', 'pbm_marker', 'annotation'),
('AA', 'AE', 'AC', 'AN', 'citation_end',   'pbm_marker', 'annotation');

-- Alignment/layout markers (AA.AE.AD) — 13 tokens
INSERT INTO tokens (ns, p2, p3, p4, name, category, subcategory) VALUES
('AA', 'AE', 'AD', 'AA', 'align_left',      'pbm_marker', 'layout'),
('AA', 'AE', 'AD', 'AB', 'align_center',    'pbm_marker', 'layout'),
('AA', 'AE', 'AD', 'AC', 'align_right',     'pbm_marker', 'layout'),
('AA', 'AE', 'AD', 'AD', 'align_justify',   'pbm_marker', 'layout'),
('AA', 'AE', 'AD', 'AE', 'indent_level_1',  'pbm_marker', 'layout'),
('AA', 'AE', 'AD', 'AF', 'indent_level_2',  'pbm_marker', 'layout'),
('AA', 'AE', 'AD', 'AG', 'indent_level_3',  'pbm_marker', 'layout'),
('AA', 'AE', 'AD', 'AH', 'indent_level_4',  'pbm_marker', 'layout'),
('AA', 'AE', 'AD', 'AI', 'indent_level_5',  'pbm_marker', 'layout'),
('AA', 'AE', 'AD', 'AJ', 'indent_level_6',  'pbm_marker', 'layout'),
('AA', 'AE', 'AD', 'AK', 'indent_level_7',  'pbm_marker', 'layout'),
('AA', 'AE', 'AD', 'AL', 'indent_level_8',  'pbm_marker', 'layout'),
('AA', 'AE', 'AD', 'AM', 'hanging_indent',  'pbm_marker', 'layout');

-- Non-text content markers (AA.AE.AE) — 10 tokens
INSERT INTO tokens (ns, p2, p3, p4, name, category, subcategory) VALUES
('AA', 'AE', 'AE', 'AA', 'image_ref',           'pbm_marker', 'non_text'),
('AA', 'AE', 'AE', 'AB', 'figure_start',        'pbm_marker', 'non_text'),
('AA', 'AE', 'AE', 'AC', 'figure_end',          'pbm_marker', 'non_text'),
('AA', 'AE', 'AE', 'AD', 'caption_start',       'pbm_marker', 'non_text'),
('AA', 'AE', 'AE', 'AE', 'caption_end',         'pbm_marker', 'non_text'),
('AA', 'AE', 'AE', 'AF', 'embedded_object_ref', 'pbm_marker', 'non_text'),
('AA', 'AE', 'AE', 'AG', 'math_start',          'pbm_marker', 'non_text'),
('AA', 'AE', 'AE', 'AH', 'math_end',            'pbm_marker', 'non_text'),
('AA', 'AE', 'AE', 'AI', 'audio_ref',           'pbm_marker', 'non_text'),
('AA', 'AE', 'AE', 'AJ', 'video_ref',           'pbm_marker', 'non_text');
```
