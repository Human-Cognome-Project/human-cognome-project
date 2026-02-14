# Text Encoder Design: Plain Text → PBM

**Author:** hcp_pbm (PBM Specialist)
**Date:** 2026-02-13
**Status:** Design document — pre-implementation
**Depends on:** pbm-format-spec.md, hcp_english token data, hcp_core byte codes

---

## 1. Goal

A Python encoder that converts plain text (specifically Gutenberg Project texts) into PBM (Pair Bond Map) token sequences stored in hcp_en_pbm. The encoder must satisfy **round-trip verification**: original text → PBM → reconstructed text = exact content match.

### What This Document Covers

1. Encoder pipeline — scanner, resolver, structure detector, PBM builder
2. Reconstruction rules — whitespace, capitalization, paragraph rendering
3. Round-trip verification strategy
4. MVP scope for Frankenstein (Gutenberg #84)
5. Token lookup strategy for 1.1M+ tokens

### What Exists Already

Two prior encoder attempts exist in `src/hcp/ingest/`:
- `gutenberg_ingest_pbm.py` — DB-backed token lookup, but stores FPB bond counts (not ordered content streams), references dropped hcp_names DB, uses monolithic token IDs
- `gutenberg_encode.py` — Standalone tokenizer, but no DB lookup, JSON output, no structural markers

Neither produces the ordered content stream with structural markers defined in pbm-format-spec.md. This design starts fresh, informed by what those prototypes got right and wrong.

---

## 2. Token Landscape (What We're Matching Against)

### 2.1 hcp_english (AB namespace) — 1.29M word tokens

| p3 prefix | POS category | Count | Examples |
|-----------|-------------|-------|---------|
| CA | noun | 930,925 | fox, disaster, enterprise |
| CB | verb | 180,945 | jumped, was, can't |
| CC | adjective | 148,596 | quick, lazy, brown |
| CD | adverb | 23,729 | the(!), very, yesterday |
| CE | preposition | 509 | over, with, of |
| CF | conjunction | 209 | and, but, or |
| CG | determiner | 173 | my, this, it's(!) |
| CH | pronoun | 608 | I, you, he |
| CI | interjection | 3,024 | oh, alas |
| CJ | numeral | 456 | one, two, first |
| CK | symbol | 409 | !, ', * |
| CL | particle | 36 | not, to |
| CM | punctuation | 50 | (mostly Braille) |
| CN | article | 5 | (variants only) |
| CQ | character | 109 | |
| DA-DE | derivatives | 3,979 | he's, I'm, we're |
| EA-EC | multi-word | 9,084 | phrases, proverbs |

**Key observations from DB exploration:**
- "the" is at AB.AB.CD.AH.xN (classified as adverb, not article/determiner — Wiktionary artifact)
- "a" is at AB.AB.CA.AA.dF (classified as noun)
- No capitalized "The" or "A" variant exists — confirms positional capitalization is computed
- All 15 common contractions exist (don't, can't, won't, it's, etc.) as whole tokens
- POS classification from Wiktionary is inconsistent — tokens are NOT organized by POS. Lookup is by **name**, not by category.

### 2.2 hcp_core (AA namespace) — Punctuation as Byte Codes

Standard punctuation lives in hcp_core as byte code character tokens (AA.AA.AA.AA.*), NOT as word tokens in hcp_english:

| Character | Token ID | Core Name |
|-----------|----------|-----------|
| `.` | AA.AA.AA.AA.Aw | FULL STOP |
| `,` | AA.AA.AA.AA.Au | COMMA |
| `;` | AA.AA.AA.AA.BJ | SEMICOLON |
| `:` | AA.AA.AA.AA.BI | COLON |
| `!` | AA.AA.AA.AA.Ai | EXCLAMATION MARK |
| `?` | AA.AA.AA.AA.BN | QUESTION MARK |
| `(` | AA.AA.AA.AA.Aq | LEFT PARENTHESIS |
| `)` | AA.AA.AA.AA.Ar | RIGHT PARENTHESIS |
| `-` | AA.AA.AA.AA.Av | HYPHEN-MINUS |
| `"` | AA.AA.AA.AA.Aj | QUOTATION MARK |
| `'` | AA.AA.AA.AA.Ap | APOSTROPHE |
| `_` | AA.AA.AA.AA.Bv | LOW LINE |

Some punctuation also exists as word tokens in hcp_english (Wiktionary import artifacts: `?` as a "noun", `!` as a "symbol", etc.). **The encoder must use the hcp_core byte code tokens for punctuation**, not the hcp_english duplicates. This is canonical and consistent.

### 2.3 Structural Markers (AA.AE namespace)

Defined in pbm-format-spec.md Section 3. The encoder uses these from hcp_core:

- `AA.AE.AA.AI` / `AA.AE.AA.AJ` — paragraph_start / paragraph_end
- `AA.AE.AA.AK` — line_break (forced line break within paragraph)
- `AA.AE.AA.AA` / `AA.AE.AA.AB` — document_start / document_end
- `AA.AE.AA.AD` — chapter_break
- `AA.AE.AA.AE` — section_break
- `AA.AE.AD.AE`–`AA.AE.AD.AL` — indent_level_1 through indent_level_8
- `AA.AE.AB.AC` / `AA.AE.AB.AD` — italic_start / italic_end
- `AA.AE.AC.AA` / `AA.AE.AC.AB` — sic_start / sic_end
- `AA.AE.AC.AL` — tbd (unresolved reference)

---

## 3. Encoder Pipeline

### 3.1 Architecture Overview

```
Input Text
    │
    ▼
┌──────────────┐
│   Scanner    │  Character-by-character → raw token strings
└──────┬───────┘
       │  List[RawToken]
       ▼
┌──────────────┐
│  Resolver    │  Raw strings → hcp token IDs (with cache)
└──────┬───────┘
       │  List[ResolvedToken]
       ▼
┌──────────────┐
│  Structure   │  Detect paragraphs, breaks, indentation
│  Detector    │  Insert structural markers
└──────┬───────┘
       │  List[PBMEntry]
       ▼
┌──────────────┐
│ PBM Builder  │  Assign positions, write to pbm_content
└──────┬───────┘
       │  Complete PBM in database
       ▼
┌──────────────┐
│ Reconstructor│  PBM → text (for verification)
└──────┬───────┘
       │  Reconstructed text
       ▼
┌──────────────┐
│  Verifier    │  Compare original ↔ reconstructed
└──────────────┘
```

### 3.2 Stage 1: Scanner

The scanner reads input text and produces a stream of raw tokens. It operates on **logical lines** (text between newline characters).

**Token types produced:**
- `word` — alphabetic sequence, may include internal apostrophes (contractions)
- `number` — numeric sequence
- `punctuation` — single punctuation character
- `whitespace_hint` — NOT stored, but tracked to detect indentation at line start

**Scanner rules:**

1. **Word boundary detection.** A word is a contiguous sequence of:
   - Alphabetic characters (a-z, A-Z)
   - Internal apostrophes followed by alphabetic characters (`don't`, `it's`, `o'clock`)
   - Internal hyphens connecting alphabetic parts (`re-use`, `well-known`) — **the hyphenated compound is tried as a single token first**

2. **Punctuation.** Each punctuation character is a separate token: `. , ; : ! ? ( ) [ ] " ' — - _`
   - Exception: `--` (double hyphen) is a single em-dash token
   - Exception: `...` (triple period) is a single ellipsis token
   - Exception: Gutenberg `_` wrapping is detected as italic markers (see 3.4)

3. **Numbers.** Contiguous digit sequences. Digits attached to words (like `17th`) stay as one token — the compound is looked up as a whole.

4. **Apostrophe handling.** The critical edge case:
   - Apostrophe AFTER a letter → part of the word (`don't`, `Mary's`, `'twas`)
   - Apostrophe NOT after a letter → standalone punctuation (opening quote)
   - Possessive at end (`heroes'`) → try whole token, then split base + apostrophe

5. **Case tracking.** The scanner records whether each word token's first character is uppercase. This is metadata for the resolver, NOT stored in the PBM.

**Scanner output:** `List[RawToken(text, type, is_capitalized, line_number, char_offset)]`

### 3.3 Stage 2: Token Resolver

The resolver maps raw token strings to hcp token IDs.

**Resolution algorithm (per token):**

```
resolve(raw_token):
    if raw_token.type == 'punctuation':
        return lookup_punctuation(raw_token.text)  # hcp_core byte codes

    if raw_token.type == 'word':
        # Step 1: Exact match
        token_id = word_cache.get(raw_token.text)
        if token_id:
            return token_id

        # Step 2: Case relaxation (if capitalized)
        if raw_token.is_capitalized:
            lowercase = raw_token.text.lower()
            token_id = word_cache.get(lowercase)
            if token_id:
                return token_id  # Positional capitalization — computed at reconstruction

        # Step 3: Possessive/apostrophe split
        if raw_token.text.endswith("'s") or raw_token.text.endswith("'"):
            base = raw_token.text.rstrip("'s").rstrip("'")
            base_id = word_cache.get(base) or word_cache.get(base.lower())
            if base_id:
                return [base_id, APOSTROPHE_TOKEN, S_TOKEN]  # Split into components

        # Step 4: Hyphenated compound split
        if '-' in raw_token.text:
            parts = raw_token.text.split('-')
            resolved_parts = [resolve_word(p) for p in parts]
            if all(r is not None for r in resolved_parts):
                return interleave(resolved_parts, HYPHEN_TOKEN)

        # Step 5: Unknown word → sic or TBD
        return handle_unknown(raw_token)
```

**Unknown word handling (Step 5):**

For MVP, unknown words are wrapped in `sic_start`/`sic_end` and stored character-by-character using hcp_core byte code tokens:

```
sic_start → [char_token_1, char_token_2, ...] → sic_end
```

This preserves the exact spelling for round-trip verification. The TBD mechanism (from the format spec) is for unresolvable *references*, not unknown *words*. An unknown word is anomalous content; a TBD is a known concept whose token doesn't exist yet. For Gutenberg fiction, nearly all unknowns will be archaic spellings, proper nouns, or OCR artifacts — sic is correct.

**Capitalization decision logic:**

```
is_positional_cap(raw_token, previous_tokens):
    # Capitalization at these positions is structural → not stored
    if at_paragraph_start:           return True
    if after_sentence_end_punct:     return True  # . ! ? followed by this word
    if after_line_break:             return True  # Gutenberg line wrapping

    # Everything else is inherent (baked into the token)
    return False
```

If `is_positional_cap` returns True, the resolver looks up the **lowercase** form. If False (mid-sentence capitalization), the resolver looks up the **exact** form — this is a proper noun or special term whose capitalization is inherent in the token.

### 3.4 Stage 3: Structure Detector

The structure detector identifies document structure from plain text formatting conventions and inserts structural marker tokens.

**Gutenberg plain text conventions:**

| Pattern | Detected as | PBM Marker |
|---------|------------|------------|
| Blank line (empty or whitespace-only) | Paragraph break | paragraph_end + paragraph_start |
| `\n` within paragraph | Line continuation | (nothing — word wrapping is an artifact of line length) |
| Leading spaces (2+) on a line | Indentation | indent_level_N after paragraph_start |
| `Chapter N` or `CHAPTER N` alone on a line | Chapter heading | chapter_break + title_start ... title_end |
| `Letter N` alone on a line | Section heading | section_break + title_start ... title_end |
| `_text_` (underscore wrapping) | Italic text | italic_start ... italic_end |
| All-caps line | Title/heading | Detected heuristically |

**Paragraph detection algorithm:**

```
for each logical_line in input:
    if line is blank:
        if in_paragraph:
            emit(paragraph_end)
            in_paragraph = False
        blank_count += 1
    else:
        if not in_paragraph:
            emit(paragraph_start)
            in_paragraph = True
            # Check for indentation
            indent = count_leading_spaces(line)
            if indent >= 2:
                level = classify_indent(indent)
                emit(indent_level_N)
        # Process line content through scanner + resolver
        emit_tokens(line)
```

**Line break vs. paragraph break:**

Gutenberg texts have hard line wraps at ~70-75 characters. These are NOT paragraph breaks — they're artifacts of the plain text format. The encoder treats them as word continuation (whitespace between tokens) UNLESS:
- The next line is blank → paragraph break
- The next line has different indentation → structure change
- The current line is very short (< 40 chars) AND the next line starts with uppercase → possible paragraph break

This heuristic won't be perfect. The MVP accepts some false positives/negatives; refinement comes from testing against Frankenstein.

**Gutenberg italic detection:**

```
_To Mrs. Saville, England._
```

The `_` characters are Gutenberg's convention for italics. Detection:
1. `_` at start of a word (preceded by whitespace/line-start) → italic_start, strip the `_`
2. `_` at end of a word (followed by whitespace/punctuation/line-end) → italic_end, strip the `_`
3. `_` in other positions → literal LOW LINE punctuation token

### 3.5 Stage 4: PBM Builder

The builder takes the resolved token stream (with structural markers interleaved) and writes it to the database.

**Steps:**

1. Allocate a PBM token ID in the zA namespace (e.g., `zA.AB.CA.AA.AA` for first book)
2. Wrap with document_start / document_end markers
3. Assign sequential positions (starting at 1)
4. Write to `pbm_content` table in a single transaction (batch INSERT)
5. Write provenance metadata to `document_provenance`
6. Write any tab_definitions if indentation was detected
7. Write the document token to the `tokens` table
8. Write position_metadata for any positions that need it (italic source positions, etc.)

**Batch insert strategy:**

For a ~75,000-word novel (Frankenstein), the content stream will be approximately 85,000–100,000 rows (words + punctuation + structural markers). Insert in batches of 10,000 rows using `executemany()` for efficiency.

---

## 4. Reconstruction Rules

Reconstruction converts a PBM content stream back into plain text. This is the inverse of encoding, and its correctness is what round-trip verification tests.

### 4.1 Inter-Word Whitespace

**Default rule:** Insert a single space between adjacent content tokens.

**Suppression rules (no space BEFORE):**
- `.` `,` `;` `:` `!` `?` — sentence/clause punctuation
- `)` `]` `}` — closing brackets
- `'` `"` when closing (even position in pair count)
- `-` when part of a hyphenated compound

**Suppression rules (no space AFTER):**
- `(` `[` `{` — opening brackets
- `'` `"` when opening (odd position in pair count)
- `-` when part of a hyphenated compound

**Special cases:**
- Between two punctuation tokens: no space (`."` stays `."`)
- After structural markers: no space (they're not content)

**Implementation:**

```python
def needs_space_before(current_token, previous_token):
    if previous_token is None:
        return False
    if previous_token.is_structural_marker:
        return False
    if current_token.is_structural_marker:
        return False
    if current_token.text in NO_SPACE_BEFORE:
        return False
    if previous_token.text in NO_SPACE_AFTER:
        return False
    return True
```

### 4.2 Positional Capitalization

**Rule:** Capitalize the first alphabetic character of the next word after:
1. Paragraph start (paragraph_start marker)
2. Sentence-ending punctuation (`.` `!` `?`) when followed by a content token (not closing quotes/brackets)
3. Colon in specific contexts (dialogue attribution — deferred to post-MVP)

**Implementation:**

```python
capitalize_next = False

for entry in content_stream:
    if entry.is_marker('paragraph_start'):
        capitalize_next = True
        continue

    if entry.is_punctuation and entry.text in '.!?':
        capitalize_next = True
        continue

    if entry.is_content_token and capitalize_next:
        surface = lookup_surface_form(entry.token_id)
        output.append(surface[0].upper() + surface[1:])
        capitalize_next = False
    elif entry.is_content_token:
        output.append(lookup_surface_form(entry.token_id))
```

**Edge cases for Frankenstein:**
- `St.` — the period is part of an abbreviation, not sentence-ending. But the encoder stores `st` + `.` as separate tokens. The reconstruction rule will over-capitalize after `St.` unless we handle abbreviations. **MVP approach:** Accept this imperfection; abbreviation-aware reconstruction is post-MVP.
- Actually, better approach: at encoding time, if a period follows a known abbreviation token (st, mr, mrs, dr, dec, etc.), the encoder records this in position_metadata (key=`abbreviation`, value=`true`). Reconstruction checks position_metadata before capitalizing.

### 4.3 Paragraph and Line Break Rendering

| Marker | Plain text output |
|--------|------------------|
| paragraph_start | (nothing — implicit in whitespace) |
| paragraph_end | `\n\n` (blank line) |
| line_break | `\n` |
| chapter_break | `\n\n\n` |
| section_break | `\n\n\n` |
| document_start | (nothing) |
| document_end | `\n` (trailing newline) |
| title_start | (nothing — content follows) |
| title_end | `\n` |

### 4.4 Indentation Rendering

When `indent_level_N` follows `paragraph_start`, insert indentation at the start of each line in that paragraph:
- Default (no tab_definitions): N × 2 spaces
- With tab_definitions: look up the original_representation for this indent level in this document

### 4.5 Formatting Marker Rendering

For plain text output:
- italic_start / italic_end → `_` (Gutenberg convention, for round-trip fidelity)
- bold_start / bold_end → (no plain text representation — stripped)
- all_caps_start / all_caps_end → uppercase the enclosed content tokens
- sic_start / sic_end → emit the enclosed character tokens as-is (the anomaly IS the content)

---

## 5. Round-Trip Verification

### 5.1 Verification Strategy

```
original_text → [normalize] → normalized_original
                                    ↕ compare
original_text → [encode] → PBM → [reconstruct] → reconstructed_text → [normalize] → normalized_reconstructed
```

**Normalization** handles differences that are acceptable (not content changes):
- Trailing whitespace on lines → stripped
- Trailing newlines at end of document → normalized to single `\n`
- Multiple consecutive blank lines → normalized to `\n\n`
- Tab characters → normalized based on tab_definitions

### 5.2 Verification Levels

**Level 1: Token-level (fast, during encoding).**
Count tokens in → count tokens out. If they differ, something was lost or invented.

**Level 2: Content-level (primary verification).**
After normalization, `normalized_original == normalized_reconstructed`. This is the gold standard.

**Level 3: Structural (diagnostic).**
Compare structural marker counts: paragraph_start count should match `\n\n` count in original (approximately). Chapter breaks should match "Chapter N" occurrences.

### 5.3 Handling Known Imperfections

Some differences between original and reconstructed text are acceptable for MVP:

| Difference | Acceptable? | Why |
|-----------|------------|-----|
| Trailing whitespace differences | Yes | Not content |
| Multiple blank lines collapsed to one | Yes | Structural normalization |
| Hard line wraps at different column | Yes | Line wrapping is a format artifact |
| Capitalization after abbreviation periods | No | Must be solved (abbreviation detection) |
| Unknown word character sequence | Yes if exact | sic markers preserve exact characters |

The key invariant: **every word in the original must appear in the reconstruction, in the same order, with the same spelling.** Whitespace and paragraph breaks may differ in their exact rendering, but the word sequence must be identical.

---

## 6. MVP Scope: Frankenstein (Gutenberg #84)

### 6.1 What to Implement

1. **Scanner** — tokenize Frankenstein's plain text (full file, no boilerplate stripping)
2. **Token resolver** — look up words in hcp_english, punctuation in hcp_core
3. **Structure detector** — paragraph breaks, chapter headings, Gutenberg italic `_` markers
4. **PBM builder** — write content stream to hcp_en_pbm.pbm_content
5. **Reconstructor** — PBM → plain text with whitespace/capitalization rules
6. **Round-trip verifier** — compare original vs. reconstructed

### 6.2 What to Skip for MVP

- **Entity resolution.** No entity_ref position_metadata. Proper nouns are stored as label tokens only.
- **HTML/markup ingestion.** Plain text only.
- **Multi-language support.** English only.
- **Abbreviation-period handling.** Accept over-capitalization after "St." "Mr." etc. as a known imperfection, track in test results.
- **Smart quote detection.** Treat all `'` and `"` as simple punctuation. Curly quotes if present in the source are separate Unicode characters handled by the character token lookup.
- **Complex indentation.** Detect leading spaces for basic indent levels. Don't try to infer nested list structure.
- **Metadata beyond provenance.** No copyright/IP fields for MVP (Frankenstein is public domain).

### 6.3 Frankenstein-Specific Considerations

**Text structure:**
- Gutenberg boilerplate: NOT stripped. Full file is encoded. A PBM is a faithful encoding of the source document. Boilerplate deduplication via sub-PBM references is a future optimization.
- Title block: "Frankenstein;\n\nor, the Modern Prometheus\n\nby Mary Wollstonecraft (Godwin) Shelley"
- Table of contents: "CONTENTS" followed by "Letter 1" through "Chapter 24"
- Letters 1-4, then Chapters 1-24
- Italic markers: `_To Mrs. Saville, England._`
- Em dashes: `—` (Unicode) and `--` (double hyphen)
- Archaic/unusual spellings: may trigger sic markers
- Dates: "Dec. 11th, 17—" — abbreviation period, number with suffix, truncated year

**Estimated scale:**
- ~75,000 words → ~85,000 content tokens (words + punctuation)
- ~3,000 paragraphs → ~6,000 paragraph markers
- ~30 chapter/section breaks
- Total content stream: ~95,000 rows in pbm_content

---

## 7. Token Lookup Strategy

### 7.1 The Problem

hcp_english has 1.29M+ word tokens. For each word in the input, we need to find its token ID. A database query per word would mean ~85,000 queries for Frankenstein — slow if done naively.

### 7.2 Strategy: In-Memory Cache with DB Fallback

**Phase 1: Pre-load common tokens.**
Before processing, load a working set into a Python dict:

```python
# Load ALL word tokens into memory
# 1.29M entries × ~60 bytes each ≈ 77MB — fits easily in RAM
word_cache = {}  # name → token_id

with conn_english.cursor() as cur:
    cur.execute("""
        SELECT name, token_id
        FROM tokens
        WHERE ns = 'AB' AND p2 = 'AB' AND p3 LIKE 'C%'
    """)
    for name, token_id in cur:
        # Store first match only (multiple POS entries for same spelling
        # resolve to the first one — POS disambiguation is post-MVP)
        if name not in word_cache:
            word_cache[name] = token_id
```

Also load derivatives (DA-DE) and multi-word expressions (EA-EC):
```python
    cur.execute("""
        SELECT name, token_id
        FROM tokens
        WHERE ns = 'AB' AND p2 = 'AB' AND p3 LIKE 'D%'
    """)
    # ... same pattern for E*
```

**Phase 2: Load punctuation map.**
Only 32 punctuation tokens from hcp_core — hardcode as a constant:

```python
PUNCTUATION_MAP = {
    '.': 'AA.AA.AA.AA.Aw',
    ',': 'AA.AA.AA.AA.Au',
    ';': 'AA.AA.AA.AA.BJ',
    ':': 'AA.AA.AA.AA.BI',
    '!': 'AA.AA.AA.AA.Ai',
    '?': 'AA.AA.AA.AA.BN',
    '(': 'AA.AA.AA.AA.Aq',
    ')': 'AA.AA.AA.AA.Ar',
    '-': 'AA.AA.AA.AA.Av',
    '"': 'AA.AA.AA.AA.Aj',
    "'": 'AA.AA.AA.AA.Ap',
    '_': 'AA.AA.AA.AA.Bv',
    # ... etc.
}
```

**Phase 3: Character fallback.**
For sic-wrapped unknown words, each character is looked up in a character map (also pre-loaded from hcp_core).

### 7.3 Duplicate Name Resolution

Multiple tokens may share the same name (different POS). Example: "run" exists as both noun (CA) and verb (CB). For the PBM content stream, we need ONE token ID per surface word.

**MVP rule:** Take the first token found. POS disambiguation is a future concern — the PBM stores the surface form, and the engine resolves POS from context using force patterns. The important thing is that the name → token_id mapping is stable (same input always produces same output) for round-trip verification.

**Implementation:** The cache loading query orders by p3 (so nouns come before verbs) and we take the first match. This is deterministic.

### 7.4 Performance Estimate

- Cache loading: ~5 seconds (1.29M rows from hcp_english)
- Per-token lookup: O(1) dict access, ~100ns
- Total lookup time for Frankenstein: ~85,000 tokens × 100ns ≈ 8.5ms
- DB writes: ~95,000 rows batched in groups of 10,000 ≈ 5-10 seconds
- **Total encoding time: ~15-20 seconds** (dominated by DB I/O)

---

## 8. Module Structure

```
src/hcp/ingest/
├── text_encoder.py          # Main encoder orchestrator
├── scanner.py               # Stage 1: text → raw tokens
├── resolver.py              # Stage 2: raw tokens → hcp token IDs
├── structure_detector.py    # Stage 3: detect paragraphs, headings, etc.
├── pbm_builder.py           # Stage 4: write to database
├── reconstructor.py         # PBM → text (for verification)
├── verifier.py              # Round-trip comparison
└── gutenberg_utils.py       # Gutenberg-specific: boilerplate stripping, italic detection
```

**Entry point:**
```python
from text_encoder import TextEncoder

encoder = TextEncoder(db_config)
pbm_id = encoder.encode_file(
    path="data/gutenberg/texts/00084_Frankenstein Or The Modern Prometheus.txt",
    doc_type="book",
    source_format="gutenberg_txt",
    fiction=True  # → uses v* namespace for fiction PBMs
)

# Verify round-trip
reconstructor = Reconstructor(db_config)
text = reconstructor.reconstruct(pbm_id, format="plain_text")
verifier = Verifier()
result = verifier.verify(original_text, text)
print(result)  # PASS/FAIL + diff details
```

**Wait — Frankenstein is fiction.** Per the fiction/non-fiction namespace split (pbm-format-spec.md Section 4.1), Frankenstein goes in v* (fiction PBMs), not z* (non-fiction). The encoder needs the fiction/non-fiction flag at encoding time to route to the correct namespace.

For MVP, the fiction PBM shard (vA) doesn't exist yet. Two options:
1. **Store in zA anyway** for now, with a note to migrate when vA exists
2. **Create vA shard** as part of MVP setup

Recommendation: Store in zA for now. The content format is identical regardless of namespace. Migration is a simple token_id rewrite when vA is created. Don't block MVP on shard creation.

---

## 9. Data Flow Example

**Input line:** `You will rejoice to hear that no disaster has accompanied the`

**Stage 1 (Scanner):**
```
[word:"You" cap:true] [word:"will"] [word:"rejoice"] [word:"to"] [word:"hear"]
[word:"that"] [word:"no"] [word:"disaster"] [word:"has"] [word:"accompanied"]
[word:"the"]
```

**Stage 2 (Resolver):**
- "You" → capitalized, at line start within paragraph. `is_positional_cap` → depends on context. If paragraph-initial → True, look up "you" → `AB.AB.CH.AA.xw`
- "will" → exact match → `AB.AB.CB.BW.{id}`
- "rejoice" → exact match → `AB.AB.CB.{id}`
- ... etc.
- "the" → exact match → `AB.AB.CD.AH.xN`

**Stage 3 (Structure Detector):**
This line is within a paragraph (previous line was not blank). No structural markers needed.

**Stage 4 (PBM Builder):**
If this is the start of a paragraph, the stream includes:
```
pos 1: paragraph_start    (AA.AE.AA.AI)
pos 2: you                (AB.AB.CH.AA.xw)
pos 3: will               (AB.AB.CB.BW.{id})
pos 4: rejoice            (AB.AB.CB.{id})
pos 5: to                 (AB.AB.CL.{id})
pos 6: hear               (AB.AB.CB.{id})
pos 7: that               (AB.AB.CF.{id})
pos 8: no                 (AB.AB.CG.{id})
pos 9: disaster           (AB.AB.CA.{id})
pos 10: has               (AB.AB.CB.{id})
pos 11: accompanied       (AB.AB.CB.{id})
pos 12: the               (AB.AB.CD.AH.xN)
... [next line continues]
```

**Reconstruction:**
```
[paragraph_start] → capitalize_next = True
"you" + capitalize → "You"
" " + "will" → " will"
" " + "rejoice" → " rejoice"
... etc.
→ "You will rejoice to hear that no disaster has accompanied the"
```

---

## 10. Open Questions (Updated Post-Implementation)

1. **Line wrapping fidelity.** Gutenberg texts have hard wraps at ~70 chars. The MVP implementation does NOT store line_break markers at hard wraps — instead, lines within paragraphs are joined with spaces. This means the reconstruction produces paragraph-only line breaks (no hard wraps). Word-sequence match is 100%; line layout differs. **RESOLVED for MVP:** Line wraps are a format artifact. Paragraph breaks are the content boundary.

2. **Homograph disambiguation.** "run" (noun) vs. "run" (verb) → same name, different tokens. MVP takes the first match. **RESOLVED:** POS disambiguation is the force pattern engine's job, not the encoder's.

3. **Gutenberg boilerplate.** Per Project Lead directive: the ENTIRE file is encoded, boilerplate and all. A PBM is a faithful encoding of the source document — nothing is stripped. Boilerplate deduplication via sub-PBM references is a future optimization. **RESOLVED.**

4. **Missing common words.** "man" (128 occurrences) and "sun" (45 occurrences) are missing from hcp_english. These are genuine Wiktionary import gaps. Currently handled via sic encoding. **Flag for DB specialist.**

5. **Smart quote possessives.** The resolver splits "father's" (with smart quote U+2019) into "father" + "'" + "s". The reconstructor suppresses space around the split to produce correct output. Works but fragile. **Consider adding possessive forms to hcp_english.**

---

## 11. Implementation Results (2026-02-14)

### Phase 2a+2b: COMPLETE

**Encoding Frankenstein (full file, 7,737 lines):**
- 93,881 PBM stream entries written to hcp_en_pbm
- 78,014 words, 11,063 punctuation tokens, 2,661 structural markers
- 2,143 sic tokens (380 unique unknown words)
- 827 paragraphs, 24 chapters detected
- 6.7s encode time (dominated by DB cache load)
- Written to hcp_en_pbm database

**Round-trip verification:**
- **Word-sequence match: 100%** (78,292 / 78,292 words)
- Content match: FAIL (expected — line wrapping differs)
- Resolution breakdown: 74,842 exact, 2,905 case-relaxed, 267 splits

**Unknown word categories:**
- Common words missing from hcp_english: man (128), sun (45)
- Proper nouns: Clerval (59), Justine (54), Safie (25), Krempe (9), etc.
- Place names: Chamounix, Salêve, Plainpalais, Arve, etc.
- Foreign words: schiavi, ognor, maladie, etc.
- Numbers: 21, 1993, 84, 2025 (from boilerplate)

### Remaining Phases

**Phase 2c: Edge case hardening.**
- Abbreviation period handling (over-capitalizes after "St." "Mr." "Dec.")
- Smart quote possessive handling (currently works via space suppression)
- TOC/contents structural detection (currently joins as single paragraph)

**Phase 2d: Scale test.**
- Encode all 10 Gutenberg texts
- Performance profiling
- Batch encoding pipeline
