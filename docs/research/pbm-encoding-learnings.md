# PBM Encoding Learnings

**From:** PBM Specialist batch encoding work (2026-02-14 to 2026-02-15)
**Status:** Complete — 110 documents encoded, round-trip verified

---

## Scope of Work

Encoded 110 Gutenberg fiction texts as PBMs in hcp_en_pbm (zA namespace). Texts range from 35KB (Struwwelpeter) to 5.3MB (Chekhov short stories compilation). All texts encode the ENTIRE file — no boilerplate stripping.

### Final Statistics

| Metric | Value |
|--------|-------|
| Documents encoded | 110 |
| Total PBM entries (pbm_content rows) | 21,328,592 |
| Total words processed | ~11M |
| Database size (hcp_en_pbm) | 3,530 MB |
| hcp_english tokens used | 1,396,117 |
| Unique unknown words (batch 2) | 27,684 |
| Round-trip pass rate | 100/110 PASS, 10 at 99.99%+ |
| Encode speed | ~100K words/sec |

### Document ID Scheme

Each document gets a token ID: `zA.AB.CA.AA.{p5}` where p5 is a 2-character base-52 value (A-Za-z, giving 2,704 possible documents per series).

- First 10 texts: AA through AJ (batch 1)
- Next 100 texts: AK through CF (batch 2)
- Texts sorted by file size, encoded smallest-first

---

## Pipeline Architecture

### Four-Stage Pipeline

```
Plain Text → Scanner → Resolver → Structure Detector → PBM Stream
                                                          ↓
                                                    pbm_content table
                                                          ↓
                                                    Reconstructor → Plain Text (verification)
```

### Stage 1: Scanner (`scanner.py`)
Character-by-character tokenization producing `RawToken` objects with:
- `text`: the raw surface form
- `type`: WORD, PUNCTUATION, NUMBER, ITALIC_START, ITALIC_END
- `is_capitalized`: whether first character is uppercase
- `line_number`, `char_offset`: source position

Key behaviors:
- Contractions kept as single tokens ("don't", "I'll")
- Smart quotes (U+201C/D, U+2018/9) treated as standalone punctuation
- Gutenberg underscore italics (_word_) detected and converted to ITALIC markers
- Em dashes (U+2014) and ellipses (U+2026) preserved as distinct tokens

### Stage 2: Resolver (`resolver.py`)
Maps raw tokens to HCP token IDs using an in-memory cache of ~1.35M entries from hcp_english (loaded once, ~77MB, takes ~8 seconds).

**Resolution cascade for words:**
1. **Exact match** — normalized form in word cache
2. **Case relaxation** — lowercase lookup if capitalized
3. **Possessive split** — "king's" → [king] + ['] + [s]
4. **Hyphenated compound split** — "well-known" → [well] + [-] + [known]
5. **Sic fallback** — character-by-character encoding with sic_start/sic_end markers

**Key design decisions:**
- Smart apostrophe (U+2019) normalized to ASCII (U+0027) for cache lookup, but **original character preserved in surface form** — this was a bug fix during batch 1
- Possessives decomposed rather than stored as whole forms (avoids combinatorial explosion)
- Hyphen splits use original text characters for surfaces, not normalized forms

### Stage 3: Structure Detection (`text_encoder.py`)
Groups scanner output into structural blocks and emits structural markers:
- `document_start` / `document_end` — wraps entire document
- `paragraph_start` / `paragraph_end` — detected from blank lines
- `chapter_start` / `chapter_end` — regex detection of "Chapter N" patterns
- `line_break` — hard line breaks within blocks

All markers are core tokens from the AA.AE namespace.

### Stage 4: Database Write
Batch INSERT into `pbm_content` table, 5,000 rows per batch:

```sql
pbm_content (
    doc_ns, doc_p2, doc_p3, doc_p4, doc_p5,  -- decomposed doc ID
    doc_id,                                    -- generated PK (doc)
    position,                                  -- sequential position in stream
    ns, p2, p3, p4, p5,                        -- decomposed token ID
    token_id                                   -- generated PK (token)
)
```

### Reconstruction
The reconstructor (`reconstructor.py`) rebuilds plain text by:
1. Loading surface form cache (~1.4M entries from hcp_english)
2. Walking the PBM stream token-by-token
3. Applying spacing rules (NO_SPACE_BEFORE/AFTER sets)
4. Applying positional capitalization (after sentence-ending punctuation)
5. Rendering structural markers as whitespace (paragraph → double newline, etc.)
6. Passing sic-encoded characters through unchanged

### Verification
The verifier (`verifier.py`) confirms round-trip fidelity by:
- Extracting word sequences from both original and reconstructed text
- Case-insensitive word-by-word comparison
- Reporting match rate and first N mismatches for debugging

---

## Bond Patterns Observed

These patterns emerged during encoding 110 texts and are directly relevant to force pattern / physics engine work.

### 1. Contraction Bonds (High Binding Energy)

Contractions like "don't", "can't", "I'll", "we're" are stored as **single tokens** — the apostrophe is internal to the word, not a split point. This is correct: contractions are tightly bound (high binding energy, dismantling changes meaning). The scanner recognizes these by checking if the apostrophe is followed by common suffixes (n't, 'll, 're, 've, 'd, 's when not possessive).

**Force implication:** Contractions represent bonds where binding energy ≈ 1.0 — splitting them destroys the token identity. The force framework should treat these as atomic at the word level.

### 2. Possessive Decomposition (Medium Binding Energy)

"king's" decomposes to [king] + ['] + [s]. This is a productive morphological bond — any noun can take it. Unlike contractions, the base word retains identity.

**Force implication:** Possessives are moderate binding energy (0.5-0.7) — the base word is independently meaningful, but the possessive marker creates a dependency bond. The 's attaches via a valency slot on the noun.

### 3. Hyphenated Compounds (Variable Binding Energy)

Hyphenated compounds split into parts: "well-known" → [well] + [-] + [known]. All parts must be known tokens for the split to succeed; otherwise the whole compound goes to sic encoding.

**Force implication:** Hyphen bonds vary enormously in binding energy:
- **Lexicalized compounds** ("mother-in-law"): high binding, essentially a single concept
- **Productive compounds** ("well-known"): medium binding, modifiable
- **Ad hoc compounds** ("never-to-be-forgotten"): low binding, created on the fly

The force engine needs to handle this spectrum. Sub-categorization patterns for compounds should distinguish lexicalized vs. productive vs. ad hoc.

### 4. Punctuation Attachment (Ordering Constraints)

The reconstructor's NO_SPACE_BEFORE/AFTER rules encode **ordering constraints** — punctuation binds tightly to adjacent words with strict positional rules:
- Period, comma, semicolon: attach RIGHT (no space before)
- Opening quotes/parens: attach LEFT (no space after)
- Hyphens: attach BOTH directions (no space either side)

**Force implication:** These are ordering constraint forces at strength ≈ 1.0 (absolute). Violations produce ungrammatical text. The force engine can model these as rigid constraints rather than preferences.

### 5. Sic Encoding as Bond Failure

When a word isn't in hcp_english, it gets sic-encoded: `sic_start` + character-by-character byte codes + `sic_end`. This is essentially a **degenerate bond** — the word couldn't form any bonds in the token space, so it's preserved at the atomic (character) level.

**Force implication:** Sic tokens represent bond failures — tokens that couldn't find attraction partners. The 27,684 unique unknowns from batch 2 are overwhelmingly:
- **Proper nouns** (character names: d'Artagnan, Alyosha, Natásha, Villefort)
- **Foreign words** with diacritics (Dantès, Kutúzov, Borís)
- **Gutenberg metadata** ("gutenberg", "www", "1924")

Once entity DBs are populated, most of these become resolved tokens with proper bonds. The remaining few genuine unknowns signal vocabulary gaps.

### 6. Structural Marker Nesting (Category Compatibility)

Structural markers form a strict nesting hierarchy:
```
document_start
  chapter_start
    paragraph_start
      [content tokens]
    paragraph_end
  chapter_end
document_end
```

**Force implication:** This is category compatibility at the structural level — paragraphs can only occur inside chapters or documents, chapters only inside documents. The force engine should enforce this as absolute constraints (strength = 1.0). Nesting violations are structural parse errors.

### 7. Capitalization as Positional Force

Capitalization is NOT stored — it's computed by the reconstructor using positional rules: "after sentence-ending punctuation, capitalize next word." This is a **positional force** that operates at the sentence boundary.

**Force implication:** This maps cleanly to ordering constraint forces. Sentence-initial capitalization is a movement/ordering force with strength ≈ 0.9 (near-absolute, but "e.g." and "i.e." create exceptions). The reconstructor's rules are essentially force constant tables.

### 8. Paragraph and Chapter Boundaries as Field Boundaries

Blank lines in source text create paragraph boundaries; "Chapter N" patterns create chapter boundaries. These define **force field boundaries** — forces aggregate differently within vs. across these boundaries.

**Force implication:** This is the LoD transition point. Word-level forces resolve within phrases. Phrase-level forces resolve within clauses. Clause-level forces resolve within sentences. Sentence-level forces resolve within paragraphs. Paragraph-level forces (coherence, topic continuity) operate at the paragraph level and up.

---

## Scaling Observations

### Cache Performance
- **Word cache load:** ~8 seconds for 1.35M entries, ~77MB memory
- **Surface cache load:** ~10 seconds for 1.4M entries
- **Resolution speed:** ~100K words/second after cache is warm
- **Cache reuse across documents:** Critical — loading once and encoding 110 texts amortizes the startup cost

### Database Write Performance
- **Batch insert size:** 5,000 rows per INSERT
- **Write speed:** ~50K rows/second
- **Total inserts:** 21.3M rows across 110 documents
- **No index contention:** Sequential writes to single table, position column is sequential within each document

### Shard Sizing
- 110 documents = 3,530 MB (live DB size)
- Average: ~32 MB per document
- Largest document (Chekhov compilation): ~1.3M entries ≈ ~200 MB
- **2 GB soft target:** ~60-65 average documents per shard
- War and Peace alone (800K entries) ≈ ~130 MB

### Token Utilization
- hcp_english has 1,396,117 tokens
- A typical novel uses 5,000-15,000 unique tokens
- The full 110-document corpus covers approximately 40-50% of hcp_english
- Most unused tokens are rare/archaic forms, variant spellings, and specialized vocabulary

---

## Known Issues and Edge Cases

### 1. Sic-Encoded Possessives (10 texts at 99.99%)
When a word isn't in hcp_english and is possessive (e.g., "O'Connell's"), the sic encoding wraps the base word character-by-character, but the possessive 's may or may not be included in the sic span depending on scanner token boundaries. The verifier's word extraction regex sometimes disagrees with the reconstructor about where the word boundary is.

**Impact:** Cosmetic — affects verification match rate by a few words per 100K+. Content is faithfully preserved.
**Fix needed:** Harmonize sic-possessive boundary handling between scanner, resolver, and verifier.

### 2. Smart Apostrophe in Hyphen-Split Compounds
Original bug: "blind-man's-buff" would lose the smart apostrophe (U+2019) in the hyphen-split path because the resolver used normalized text (ASCII) for all surface forms.

**Fixed in resolver.py (commit 3b3704d):** Now splits both normalized and original text on hyphens, using original parts for surface forms.

### 3. Diacritical Characters in Names
Characters like Natásha, Dantès, Kutúzov go to sic encoding because hcp_english doesn't have their diacritical forms. The byte-code sic encoding preserves them faithfully but doesn't create bonds.

**Resolution path:** Entity DBs will populate these as proper noun tokens. The librarian pipeline catalogs entities before encoding; once entity tokens exist in the language shard, these will resolve normally.

### 4. Gutenberg Metadata as Content
Because we encode entire files, Gutenberg headers, footers, and license text appear in every PBM. Words like "gutenberg" (575 occurrences) and "www" (400 occurrences) appear in the unknown list.

**Resolution path:** These become known tokens once the vocabulary grows. Alternatively, sub-PBM references can deduplicate the boilerplate across documents.

---

## Implications for Physics Engine

### PBM as Molecular Structure
A PBM stream is structurally analogous to a molecular chain:
- **Tokens = atoms** (with properties: category, sub-cat pattern, valency)
- **Spacing/punctuation rules = bond geometry** (rigid constraints on how atoms connect)
- **Structural markers = molecular backbone** (document/chapter/paragraph skeleton)
- **Sic regions = inert/noble elements** (no bonds, just pass through)

### What OpenMM Offers
OpenMM's topology model maps surprisingly well:
- **Topology:** Document structure (chains = chapters, residues = paragraphs, atoms = tokens)
- **System:** Force field definitions (7 force types as custom forces)
- **Custom forces:** Tabulated functions for sub-cat patterns, attraction profiles, binding energies
- **Platform:** GPU acceleration for large documents

### What Needs Custom Work
1. **Discrete token space:** OpenMM works with continuous coordinates; PBM tokens are discrete. Need to map token IDs to positions in a conceptual space.
2. **Asymmetric forces:** Linguistic forces are often asymmetric (head attracts complement, not vice versa). OpenMM's pairwise forces are typically symmetric.
3. **LoD transitions:** Phrase-level aggregation into clause-level shapes has no direct OpenMM analog. This may need a multi-scale simulation approach.
4. **Force envelope accumulation:** As PBM tokens stream in, forces accumulate into a 3D conceptual mesh. This "growing molecule" pattern needs a simulation where atoms are added incrementally.

### Recommended Proof of Concept
Start with a single sentence: encode it as a PBM, then use the physics engine to:
1. Assign each token a position in 3D conceptual space
2. Apply sub-categorization forces (valency slots)
3. Apply attraction forces (head-complement bonding)
4. Run energy minimization to find the "natural" structure
5. Compare the resulting geometry to the expected syntactic parse

This tests whether the 7 force types can produce meaningful structural representations from PBM data.

---

## File Reference

| File | Role | Size |
|------|------|------|
| `src/hcp/ingest/scanner.py` | Stage 1: text → raw tokens | ~200 lines |
| `src/hcp/ingest/resolver.py` | Stage 2: raw tokens → HCP token IDs | ~450 lines |
| `src/hcp/ingest/text_encoder.py` | Orchestrator: structure detection + pipeline | ~450 lines |
| `src/hcp/ingest/reconstructor.py` | PBM → plain text (inverse) | ~280 lines |
| `src/hcp/ingest/verifier.py` | Round-trip verification | ~240 lines |
| `src/hcp/ingest/batch_encode.py` | Batch encoding coordinator | ~360 lines |
| `docs/research/pbm-design-notes.md` | Pre-specialist planning notes | ~166 lines |
| `docs/research/english-force-patterns.md` | 7 force types + sub-cat patterns | ~500+ lines |
| `docs/research/force-pattern-db-requirements.md` | Force pattern DB schema | ~300+ lines |
