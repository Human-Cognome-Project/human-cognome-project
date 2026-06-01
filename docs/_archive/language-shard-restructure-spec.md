# Language Shard Restructure — Specification
*Linguist specialist — 2026-03-10*

This document defines the DATA REQUIREMENTS for restructuring hcp_english. It is a spec, not a schema — the DB specialist designs the physical structure from this. Nothing in this document should be taken as prescribing specific column names, table layouts, or storage strategies unless explicitly noted as a hard constraint.

---

## Why This Restructure

The current hcp_english evolved organically. The vocab is largely working (Phase 2 resolution functions), but the structure is not principled:
- Variants, inflected forms, and root words are mixed in the same token space
- The single `category` field cannot express multi-dimensional variant characteristics
- PoS is scattered across multiple fields with inconsistent semantics
- Gloss attachment has no defined place
- Loanwords have no source language tracking
- Standard-rule-derivable forms waste space as stored entries

The goal: a clean, principled structure where root tokens are the unit of identity, variants are properly characterised, and the schema is designed for the grammar identifier and concept space work that follows.

---

## Architectural Constraints (Non-Negotiable)

These are architectural decisions above the schema level — do not change them:

1. **Token ID format**: 14-byte 5-pair format `NS.P2.P3.P4.P5`. Generated column. Must be preserved exactly.
2. **AB namespace**: English tokens live in the AB namespace. Preserved.
3. **AA namespace**: hcp_core structural tokens, NSM primes. Not touched by this restructure.
4. **Lowercase canonical**: All root forms are lowercase. Labels/proper nouns handled via metadata, not by storing uppercase forms.
5. **LMDB vbed structure** (vocab beds `vbed_02`..`vbed_16`): The compiler reads from this schema. Schema changes must keep the compiler path viable — the DB specialist and engine specialist must coordinate on any changes to what the LMDB compiler reads.

---

## Core Data Model

### 1. Root Tokens

A **root token** is the canonical, uninflected base form of a word. It is the unit of identity in the system.

**What makes something a root**:
- It is a real English word in its uninflected base form
- It has at least one PoS role
- It is the target that all variant forms resolve to
- It has `freq_rank` data if it appears in frequency corpora (freq_rank = canonical signal)
- It is NOT itself a variant of another word (no `canonical_id` pointing elsewhere)

**What a root token needs to store**:
- Its surface form (lowercase)
- One or more PoS roles (see PoS section)
- One gloss per PoS role (see Gloss section)
- Frequency rank (if available)
- Whether it is a Label/proper noun candidate
- Its particle_key (for LMDB bed assignment — first letter + length, or apostrophe/hyphen bucket)

**What a root token does NOT store**:
- Its inflected forms (those are derivable by rule or stored as variants)
- Its archaic/dialect/casual surface variants (those are variant records)

### 2. Variant Forms

A **variant** is any surface form that resolves to a root token but is not itself the root. Variants are characterised by a multi-dimensional tag set (see Characteristic Taxonomy below).

**Types of variants**:

1. **Standard-rule variants**: Derivable from root by a known morphological rule (progressive, plural, past tense regular, etc.). These do NOT need to be stored as individual records — the rule + root is sufficient. They exist implicitly. The engine derives them at runtime.

2. **Irregular variants**: Cannot be derived by rule. Must be stored explicitly.
   - Examples: `went` (root: `go`), `ran` (root: `run`), `was` (root: `be`), `mice` (root: `mouse`)
   - Characteristic: IRREGULAR

3. **Spelling variants**: Same word, alternate orthography.
   - Examples: `colour`/`color`, `honour`/`honor`, `theatre`/`theater`
   - Characteristic: SPELLING_VARIANT + geographic tag (BRITISH/AMERICAN)

4. **Archaic variants**: Historical surface forms no longer in standard use.
   - Examples: `goeth` (root: `go`), `hath` (root: `have`), `thou` (root: `you`)
   - Characteristic: ARCHAIC (± FORMAL, ± CASUAL — see named intersections)

5. **Dialect variants**: Regional or social dialect forms.
   - Examples: `runnin'` (root: `run`), `ain't` (root: `be`+NEG or `have`+NEG)
   - Characteristic: DIALECT (± CASUAL, ± ARCHAIC)

6. **Eye dialect / phonetic spelling**: Written representation of spoken reduction.
   - Examples: `gonna` (root: `go`+WILL), `wanna` (root: `want`), `gotta` (root: `have`+obligation)
   - Characteristic: EYE_DIALECT + CASUAL

7. **Casual contractions / truncations**: Informal elisions.
   - Examples: `'em` (root: `them`), `'bout` (root: `about`), `'cause` (root: `because`)
   - Characteristic: CASUAL (± ARCHAIC for forms like `'twas`, `'tis`)

8. **Loanwords in active use**: Words borrowed from other languages, used as English in English texts.
   - Examples: `café`, `naïve`, `schadenfreude`, `rendezvous`
   - Characteristic: BORROWING + source_language (ISO 639-1)
   - Note: these become English root tokens if fully assimilated. If still foreign-feeling, BORROWING tag applies.

**What a variant record needs to store**:
- The surface form
- A reference to its root token
- A characteristic bitmask (see taxonomy)
- Source language (if BORROWING)
- A free-text note (optional — for derogatory context, rule exceptions, disambiguation)
- The morphological rule it follows (if STANDARD_RULE — for documentation, not runtime use)

### 3. PoS (Part of Speech)

PoS is stored at the **(root token, PoS role)** level. A root token may have multiple PoS roles.

**PoS roles** (see grammar-identifier-spec.md for full definitions):

| Code | Class |
|------|-------|
| N_COMMON | Common noun |
| N_PROPER | Proper noun / Label |
| N_PRONOUN | Pronoun |
| V_MAIN | Main verb |
| V_AUX | Auxiliary verb |
| V_COPULA | Copula |
| ADJ | Adjective |
| ADV | Adverb |
| DET | Determiner |
| PREP | Preposition |
| CONJ_COORD | Coordinating conjunction |
| CONJ_SUB | Subordinating conjunction |
| PART | Particle |
| NUM | Numeral |
| INTJ | Interjection |

**Requirement**: The schema must support one root token having multiple PoS roles. `run` is both V_MAIN and N_COMMON. `fast` is both ADJ and ADV.

**Critical exception — Label PoS (N_PROPER)**:

All storage is lowercase. `grace`, `mark`, `cooper` — always lowercase in the DB regardless of Label status. Capitalization is a PROPERTY on the PoS record, not part of the surface form.

- N_PROPER PoS carries `cap_property: start_cap` (or `all_cap` for initialisms like `fbi`, `usa`)
- N_COMMON on the same surface form carries no cap property
- Same lowercase surface form; different PoS; different cap property

N_PROPER tokens are always independent root tokens. A Label exists because of what it IS — a name, a place, a title — not because of its relationship to any common word. `patrick`, `london`, `thames` are Labels with no common word equivalent. `grace`, `mark`, `cooper` happen to also be common words — that is incidental.

**Label gloss**: Whether an N_PROPER entry gets a distinct gloss or shares/derives from the common gloss is determined during meaning assembly, not pre-specified in the schema. If the Label's conceptual content turns out to be completely divorced from the common word, a distinct gloss is noted at that point. The schema simply supports it — each (surface_form, PoS) pair can have its own gloss record. No upfront decision required.

Characteristic tags (SLANG, VULGAR, DEROGATORY) on N_COMMON forms never apply to N_PROPER entries sharing the same spelling.

**What a PoS record needs**:
- Reference to root token
- PoS role code
- Reference to the gloss for this (root, PoS) pair
- Whether this is the PRIMARY PoS (most common role for this word)

### 4. Glosses

A **gloss** is the conceptual anchor for a (root token, PoS role) pair. It is the connection point to concept space and NSM.

**Scope**: One gloss per (root, PoS) pair. Inflected forms and variants do NOT have their own glosses — they resolve to the root and inherit its gloss for the appropriate PoS.

**What a gloss record needs**:
- Reference to the (root, PoS) pair it belongs to
- A short human-readable definition (the "gloss text")
- NSM prime reference(s): which NSM prime(s) this concept maps to (may be multiple, may be approximate at this stage)
- A nuance note (free text — for future nuance linking, edge cases, disambiguation between near-synonyms)
- Status: DRAFT | REVIEWED | CONFIRMED (glosses will be populated gradually — need to track completeness)

**What glosses do NOT cover**:
- Standard inflectional meanings (past tense, plural, etc.) — these are physics-side derivations from morph bits
- Variant register differences (archaic vs casual form of same concept) — these are characteristic tags on the variant

**Example**:
- Root: `run`, PoS: V_MAIN → gloss: "to move rapidly on foot; to operate or manage" → NSM: MOVE / DO
- Root: `run`, PoS: N_COMMON → gloss: "an act of running; a sequence or period" → NSM: HAPPENING / TIME
- Root: `ran` — NO gloss. Resolves to `run`, inherits V_MAIN gloss.

---

## Characteristic Taxonomy

### Bitmask Design

All characteristics are stored as a bitmask (minimum 32-bit integer to accommodate all dimensions). Multiple bits may be set simultaneously.

A word can carry characteristics from multiple dimensions simultaneously. The combination is the complete characterisation — no single dimension dominates.

### Register Dimension (bits 0-7)

| Bit | Name | Meaning |
|-----|------|---------|
| 0 | FORMAL | Elevated, official, or ceremonial register |
| 1 | CASUAL | Informal, relaxed, everyday speech |
| 2 | SLANG | Colloquial, often ephemeral informal usage |
| 3 | VULGAR | Profane, obscene, crude |
| 4 | DEROGATORY | Slur, discriminatory, harmful term |
| 5 | LITERARY | Poetic, elevated literary register |
| 6 | TECHNICAL | Domain-specific, specialist vocabulary |
| 7 | NEUTRAL | Explicitly unmarked (reserved — NEUTRAL is the default, absence of register bits) |

**Note on DEROGATORY**: This is a distinct dimension from VULGAR. A racial slur is not profane — it is harmful in a categorically different way. The `note` field on the variant record should capture WHY a term is derogatory (racial, ableist, sexist, etc.) for future NAPIER use. Sub-taxonomy of derogatory types is deferred — the free-text note is sufficient for now.

**Named intersections** (important patterns, not stored separately — just the bit combination):
- CASUAL + ARCHAIC: historical informal speech (Victorian working class, dialect literature). Different grammar patterns from either alone.
- ARCHAIC + FORMAL: old elevated register (legal, liturgical, literary).
- CASUAL + DIALECT: regional informal (Huck Finn register).
- VULGAR + SLANG: crude colloquial.

### Temporal Dimension (bits 8-11)

| Bit | Name | Meaning |
|-----|------|---------|
| 8 | ARCHAIC | Obsolete or very dated — no longer in active use |
| 9 | DATED | Outdated but still recognisable — falling out of use |
| 10 | NEOLOGISM | Recently coined — not yet in standard dictionaries |
| 11 | (reserved) | |

CURRENT is the default — absence of temporal bits means contemporary standard usage.

### Geographic Dimension (bits 12-15)

| Bit | Name | Meaning |
|-----|------|---------|
| 12 | DIALECT | Non-specific regional or social dialect |
| 13 | BRITISH | British English |
| 14 | AMERICAN | American English |
| 15 | AUSTRALIAN | Australian English |

Additional geographic variants (Canadian, South African, Indian English, etc.) to be added as data warrants. Bits 16-19 reserved for geographic expansion.

### Derivation Dimension (bits 20-27)

| Bit | Name | Meaning |
|-----|------|---------|
| 20 | STANDARD_RULE | Derivable by standard morphological rule — stored for completeness only |
| 21 | IRREGULAR | Not rule-derivable — must be stored explicitly |
| 22 | SPELLING_VARIANT | Alternate orthography, same pronunciation/meaning |
| 23 | EYE_DIALECT | Phonetic spelling of spoken reduction |
| 24 | BORROWING | Loanword — see source_language field |
| 25 | COMPOUND | Compound word or portmanteau |
| 26 | ABBREVIATION | Abbreviated or contracted form |
| 27 | (reserved) | |

### Bits 28-31: Reserved for future use.

---

## Loanword Model

When BORROWING bit is set, the variant record must also carry a `source_language` field.

**Format**: ISO 639-1 two-letter language code (e.g., `fr`, `la`, `de`, `ja`, `ar`, `it`, `es`, `grc` for Ancient Greek, `non` for Old Norse).

**Rationale**: When HCP eventually builds language shards for French, Latin, etc., loanwords in English can be cross-linked to their source shard entries. The source_language field is the foreign key stub for that future relationship.

**Assimilation question**: Some loanwords are so fully assimilated they are effectively English roots (`café`, `ballet`, `genre`). Others remain foreign (`schadenfreude`, `weltanschauung`). For now: tag all loanwords with BORROWING regardless of assimilation level. The engine can treat them as standard tokens; the tag is metadata for future nuance.

---

## PoS Storage Requirements (Runtime)

The grammar identifier kernel (see grammar-identifier-spec.md) reads PoS at token lookup time. Requirements:

1. **Fast single lookup**: Given a token_id, return its primary PoS in O(1). This is the hot path.
2. **Multi-PoS lookup**: Given a token_id, return all PoS roles (for ambiguous words). Slightly less hot.
3. **LMDB-friendly**: PoS should be encodable in the LMDB vocab entry alongside token_id. Ideally fits in the existing fixed-width entry format or a small extension.

Current LMDB entry format: `[word (wordLength bytes) | token_id (14 bytes)]`
Proposed extension: `[word (wordLength bytes) | token_id (14 bytes) | pos_primary (1 byte) | characteristics (4 bytes)]`

This adds 5 bytes per entry — acceptable. The DB specialist should confirm whether the LMDB compiler can accommodate this extension without breaking the existing vbed reader.

---

## Variant Lookup Requirements (Runtime)

The resolve loop (TryVariantNormalize, TryInflectionStrip) needs to find the root for a given surface form. Requirements:

1. **Surface form → root lookup**: Given a surface form (e.g., `went`), return root token_id + characteristic bitmask. This is the miss path (only fires when PBD fails).
2. **Characteristic-filtered lookup**: Given a root, return all variants matching a characteristic filter (e.g., all ARCHAIC variants). Used for envelope pre-loading.
3. **Envelope activation**: Load all variants of a given characteristic class into LMDB env_* sub-dbs. Batch operation, not hot path.

---

## What Kaikki Provides

The fresh Kaikki English extract provides per-entry:
- `word`: surface form
- `pos`: part of speech (Wiktionary tag — needs mapping to HCP PoS codes)
- `etymology_text`: etymology (source language for loanword detection)
- `senses[]`: definitions with gloss text and tags
- `forms[]`: inflected forms with tags (tags identify: plural, past, present-participle, etc.)
- `tags[]`: entry-level tags (offensive, vulgar, archaic, dialectal, etc.)
- Relation data: synonyms, antonyms, derived terms, related

**Mapping Kaikki → HCP**:
- `pos` → PoS role code (mapping table needed — Kaikki uses "verb", "noun", "adj" etc.)
- `etymology_text` → source_language extraction (regex/NLP to identify "from French X", "from Latin X")
- `senses[].tags` → characteristic bitmask (Kaikki tags like "offensive", "vulgar", "archaic", "dialectal", "informal" → our bits)
- `forms[]` → variant records (form + tags → surface form + characteristic bits)
- `senses[].glosses[0]` → gloss text for (root, PoS) pairs
- `tags[]` at entry level → inherited by all senses if sense-level tags absent

**Kaikki tag → characteristic bit mapping** (partial — full mapping to be worked out during processing):

| Kaikki tag | HCP characteristic |
|------------|-------------------|
| archaic | ARCHAIC |
| dated | DATED |
| informal | CASUAL |
| colloquial | CASUAL |
| slang | SLANG |
| vulgar | VULGAR |
| offensive | DEROGATORY |
| dialectal | DIALECT |
| British | BRITISH |
| American | AMERICAN |
| Australian | AUSTRALIAN |
| rare | (freq_rank signal, not characteristic) |
| literary | LITERARY |
| formal | FORMAL |
| technical | TECHNICAL |

---

## What This Restructure Is NOT

- Not a change to token_id format or namespace allocation
- Not a change to PBM storage or bond encoding
- Not a change to Phase 1 (codepoint) or Phase 2 (vocab bed) resolution algorithms
- Not adding NSM prime definitions (those are hcp_core, AA namespace — separate work)
- Not adding gloss content for all tokens immediately — glosses are populated iteratively, starting with high-frequency roots
- Glosses for standard inflectional variants are NOT needed — morph bits on the physics side cover this completely

---

## Open Questions for DB Specialist

1. **Token table restructure vs new variant table**: Current schema has `canonical_id` on the tokens table pointing to root. Better design may be a separate `token_variants` table (root_token_id, surface_form, characteristics, source_language, note). Which is cleaner for the query patterns above?

2. **PoS storage**: Currently scattered across `layer`, `subcategory`, `proper_common`, `aux_type`. Needs consolidation. A `token_pos` junction table (token_id, pos_code, is_primary, gloss_id) seems right — confirm.

3. **Gloss table**: Separate `token_glosses` table (token_id, pos_code, gloss_text, nsm_prime_refs, nuance_note, status). Confirm structure.

4. **LMDB entry extension**: Adding pos_primary (1 byte) + characteristics (4 bytes) to vbed entries. Confirm this doesn't break existing reader code — coordinate with engine specialist.

5. **Migration path**: 1.4M token entries currently, ~63K with canonical_id set. Do we migrate existing data into the new structure, or rebuild from fresh Kaikki + migrate only what Kaikki doesn't cover (entities, structural tokens, NSM primes)?

6. **hcp_core tokens**: AA namespace tokens are NOT touched by this restructure. Confirm boundary.

---

## Implementation Phases (High Level)

For the DB specialist and implementation planner:

**Phase A — Schema design**: DB specialist designs physical schema from this spec. No data changes yet.

**Phase B — Kaikki extraction**: Download fresh Kaikki English JSONL. Analyse structure. Write Kaikki → HCP mapping script.

**Phase C — Schema migration**: Apply new schema to hcp_english. Migrate existing data where applicable.

**Phase D — Kaikki population**: Run Kaikki → new schema population. Roots, PoS, variants, loanwords, glosses (where available).

**Phase E — LMDB compiler update**: Update compile_vocab_lmdb.py to read new schema and produce updated vbed entries with PoS + characteristics.

**Phase F — Engine update**: Update grammar identifier and resolve loop to use new PoS + characteristic data.

**Phase G — Verification**: Run existing texts (Dracula, Sign of Four, etc.) through updated pipeline. Verify var rates don't regress. Verify grammar identifier output.
