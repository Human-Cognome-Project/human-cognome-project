# hcp_english New Schema Design

**Author:** DB Specialist — 2026-03-10
**Input:** `docs/language-shard-restructure-spec.md`, `docs/kaikki-analysis.md`
**Status:** Complete — addresses all 6 open questions from the spec

---

## Answers to Spec Open Questions

**Q1. Token table restructure vs new variant table?**
Separate `token_variants` table. The `tokens` table holds root tokens only — no `canonical_id` column. Variants live in `token_variants` with `canonical_id` pointing to the root. This keeps the tokens table clean for fast freq_rank queries and separates the different query patterns (root vocab load vs variant lookup).

**Q2. PoS storage?**
`token_pos` junction table: (token_id, pos, is_primary, gloss_id). PoS role stored as a typed enum. `is_primary` flags the most common PoS for single-lookup path. `gloss_id` FK into `token_glosses`. The current `layer`/`subcategory`/`aux_type` columns are retired.

**Q3. Gloss table?**
Separate `token_glosses` table: (id, token_id, pos, gloss_text, nsm_prime_refs, nuance_note, status). Status tracks population progress (DRAFT/REVIEWED/CONFIRMED). NSM prime refs are a JSONB array for now — populated incrementally. Not linked via FK to token_pos.gloss_id to avoid blocking population (gloss_id is nullable).

**Q4. LMDB entry extension?**
Confirmed viable. Proposed format: `[word (wordLength bytes) | token_id (14 bytes) | pos_primary (1 byte) | characteristics (4 bytes)]`. The existing vbed reader in the engine reads `[word | token_id]` at fixed offsets — adding 5 trailing bytes requires updating both the LMDB compiler and the vbed reader. This is a **Phase E/F coordination item** with the engine specialist. The schema design exposes what the compiler needs; the engine specialist decides the vbed format change.

**Q5. Migration path?**
Rebuild from fresh Kaikki, do NOT migrate the 1.4M existing token rows in-place. Rationale: the existing tokens have structural problems (mixed roots/inflected forms, inconsistent PoS encoding) that make bulk migration unreliable. Instead:
- Keep existing hcp_english tables during transition (no drop)
- Create new tables alongside existing ones
- Populate from Kaikki
- Merge freq_rank data by name lookup
- Retain structural tokens (AA namespace) — they're not in hcp_english, not touched
- After validation, drop old columns and rename

Existing `canonical_id` variant relationships (~63K) are superseded by the new `token_variants` table populated from Kaikki.

**Q6. hcp_core tokens (AA namespace)?**
Confirmed: AA namespace tokens are in `hcp_core`, not `hcp_english`. This restructure touches `hcp_english` only. The AA tokens (NSM primes, structural) are untouched.

---

## PoS Types

```sql
CREATE TYPE pos_tag AS ENUM (
    'N_COMMON',    -- common noun
    'N_PROPER',    -- proper noun / Label (always-capitalize)
    'N_PRONOUN',   -- pronoun
    'V_MAIN',      -- main verb
    'V_AUX',       -- auxiliary verb (be, have, do, will, shall, may, might, can, could...)
    'V_COPULA',    -- copula (be as linking verb — separate from aux for grammar identifier)
    'ADJ',         -- adjective
    'ADV',         -- adverb
    'PREP',        -- preposition
    'CONJ_COORD',  -- coordinating conjunction (and, but, or, nor, for, yet, so)
    'CONJ_SUB',    -- subordinating conjunction (because, although, while, since...)
    'DET',         -- determiner (the, a, an, some, any, this, that, my, your...)
    'INTJ',        -- interjection (oh, ah, well, hey...)
    'PART',        -- particle (up in "give up", out in "find out")
    'NUM'          -- numeral
);
```

---

## Characteristic Bitmask

**Single `INTEGER` (32-bit) sufficient; store as `INTEGER` in Postgres and as 4-byte value in LMDB.**

Bit layout follows the spec exactly.

### Register dimension (bits 0–7)

| Bit | Constant | Meaning |
|-----|----------|---------|
| 0 | FORMAL | Elevated, official, or ceremonial register |
| 1 | CASUAL | Informal, relaxed, everyday speech |
| 2 | SLANG | Colloquial, often ephemeral informal usage |
| 3 | VULGAR | Profane, obscene, crude |
| 4 | DEROGATORY | Slur, discriminatory, harmful term |
| 5 | LITERARY | Poetic, elevated literary register |
| 6 | TECHNICAL | Domain-specific, specialist vocabulary |
| 7 | (reserved — NEUTRAL is the absence of register bits) | |

### Temporal dimension (bits 8–11)

| Bit | Constant | Meaning |
|-----|----------|---------|
| 8 | ARCHAIC | Obsolete or very dated — no longer in active use |
| 9 | DATED | Outdated but still recognisable — falling out of use |
| 10 | NEOLOGISM | Recently coined — not yet in standard dictionaries |
| 11 | (reserved) | |

### Geographic dimension (bits 12–15)

| Bit | Constant | Meaning |
|-----|----------|---------|
| 12 | DIALECT | Non-specific regional or social dialect |
| 13 | BRITISH | British English |
| 14 | AMERICAN | American English |
| 15 | AUSTRALIAN | Australian English |
| 16–19 | (reserved for additional geographic variants) | |

### Derivation dimension (bits 20–27)

| Bit | Constant | Meaning |
|-----|----------|---------|
| 20 | STANDARD_RULE | Derivable by standard morphological rule |
| 21 | IRREGULAR | Not rule-derivable — must be stored explicitly |
| 22 | SPELLING_VARIANT | Alternate orthography, same pronunciation/meaning |
| 23 | EYE_DIALECT | Phonetic spelling of spoken reduction |
| 24 | BORROWING | Loanword — see source_language field |
| 25 | COMPOUND | Compound word or portmanteau |
| 26 | ABBREVIATION | Abbreviated or contracted form |
| 27 | (reserved) | |

### Bits 28–31: Reserved.

Named intersections (important multi-bit patterns):
- `CASUAL | ARCHAIC` = historical informal speech (Victorian working class)
- `ARCHAIC | FORMAL` = old elevated register (legal, liturgical)
- `CASUAL | DIALECT` = regional informal
- `VULGAR | SLANG` = crude colloquial

---

## Morpheme Acceptance Bitmask

Stored on `token_pos.morpheme_accept` (separate `INTEGER` from characteristics). Encodes what regular inflections a root in this PoS role accepts. Used by Postgres assembly queries to generate inflected forms for LMDB without storing them.

| Bit | Constant | Meaning |
|-----|----------|---------|
| 0 | MORPH_PLURAL | Accepts regular plural (-s, -es, -ies) |
| 1 | MORPH_PAST | Accepts regular past (-ed, doubled-consonant+ed) |
| 2 | MORPH_PROGRESSIVE | Accepts progressive (-ing) |
| 3 | MORPH_3RD_SING | Accepts 3rd-person singular present (-s, -es) |
| 4 | MORPH_COMPARATIVE | Accepts comparative (-er) |
| 5 | MORPH_SUPERLATIVE | Accepts superlative (-est) |
| 6 | MORPH_ADVERB_LY | Accepts adverb formation (-ly from adjective) |
| 7 | MORPH_POSSESSIVE | Accepts possessive (-'s) |
| 8 | MORPH_GERUND | Accepts gerund (verb → noun via -ing) |

PoS defaults (set at population time, overridable):
- N_COMMON: `MORPH_PLURAL | MORPH_POSSESSIVE`
- V_MAIN: `MORPH_PAST | MORPH_PROGRESSIVE | MORPH_3RD_SING | MORPH_GERUND`
- ADJ: `MORPH_COMPARATIVE | MORPH_SUPERLATIVE | MORPH_ADVERB_LY`
- ADV: (none — most adverbs don't inflect)
- N_PROPER: `MORPH_POSSESSIVE` (only — proper nouns don't pluralise by default)

---

## Schema

### `tokens` — Root words

```sql
CREATE TABLE tokens (
    -- Namespace coordinates (token_id generator — format unchanged)
    ns          CHAR(2)     NOT NULL DEFAULT 'AB',
    p2          CHAR(2)     NOT NULL,
    p3          CHAR(2)     NOT NULL,
    p4          CHAR(2)     NOT NULL,
    p5          CHAR(2)     NOT NULL,

    -- Generated identity (14-byte, 5-pair, format preserved)
    token_id    TEXT        GENERATED ALWAYS AS (
                    ns || '.' || p2 || '.' || p3 || '.' || p4 || '.' || p5
                ) STORED NOT NULL,

    -- Surface form — always lowercase root form
    name        TEXT        NOT NULL,

    -- Label marker — 'proper' if this is an always-capitalize proper noun
    proper_common TEXT,          -- 'proper' | NULL

    -- Frequency rank from merged corpus data (NULL = unranked)
    freq_rank   INTEGER,

    -- Source language for loanword roots (ISO 639-1/3; NULL = native English)
    -- Only set when BORROWING bit (24) is set in characteristics
    source_language CHAR(3),

    -- Characteristic bitmask (INTEGER = 32 bits; summary of token_pos characteristics)
    -- Token-level: geographic, borrowing, abbreviation, proper
    -- Summary: OR of all token_pos.characteristics bits for this token
    characteristics INTEGER     NOT NULL DEFAULT 0,

    -- PBD bed assignment key (derived)
    particle_key TEXT GENERATED ALWAYS AS (
        CASE
            WHEN name ~ '''' THEN '''' || length(name)
            WHEN name ~ '-'  THEN '-' || length(name)
            ELSE left(name, 1) || length(name)
        END
    ) STORED,

    PRIMARY KEY (ns, p2, p3, p4, p5),
    UNIQUE (token_id)
);

CREATE INDEX idx_tokens_name         ON tokens (name);
CREATE INDEX idx_tokens_freq_rank    ON tokens (freq_rank) WHERE freq_rank IS NOT NULL;
CREATE INDEX idx_tokens_proper       ON tokens (proper_common) WHERE proper_common IS NOT NULL;
CREATE INDEX idx_tokens_particle_key ON tokens (particle_key);
CREATE INDEX idx_tokens_chars        ON tokens (characteristics) WHERE characteristics != 0;
```

### `token_pos` — PoS records

```sql
CREATE TABLE token_pos (
    id              SERIAL      PRIMARY KEY,
    token_id        TEXT        NOT NULL REFERENCES tokens (token_id) ON DELETE CASCADE,
    pos             pos_tag     NOT NULL,

    -- Most common PoS for this token (fast single-lookup path)
    is_primary      BOOLEAN     NOT NULL DEFAULT false,

    -- Capitalization property (for N_PROPER tokens)
    -- 'start_cap' = first letter capitalised (normal Labels: London, Patrick)
    -- 'all_cap'   = all caps (initialisms used as words: FBI→fbi, NATO→nato)
    -- NULL        = no cap property (all other PoS)
    cap_property    TEXT,       -- 'start_cap' | 'all_cap' | NULL

    -- FK to gloss record (nullable — gloss population is iterative)
    gloss_id        INTEGER,    -- FK → token_glosses.id (added when available)

    -- What regular inflections this (token, PoS) accepts
    morpheme_accept INTEGER     NOT NULL DEFAULT 0,

    -- Per-PoS register/temporal/geographic characteristics (bits 0–19)
    -- Derivation bits (20–27) are set on token_variants, not here
    characteristics INTEGER     NOT NULL DEFAULT 0,

    UNIQUE (token_id, pos)
);

CREATE INDEX idx_token_pos_token   ON token_pos (token_id);
CREATE INDEX idx_token_pos_pos     ON token_pos (pos);
CREATE INDEX idx_token_pos_primary ON token_pos (token_id, is_primary) WHERE is_primary = true;
```

### `token_glosses` — Gloss records

```sql
CREATE TABLE token_glosses (
    id              SERIAL      PRIMARY KEY,
    token_id        TEXT        NOT NULL REFERENCES tokens (token_id) ON DELETE CASCADE,
    pos             pos_tag     NOT NULL,

    -- Primary gloss text (from Kaikki first sense, or manually written)
    gloss_text      TEXT        NOT NULL,

    -- NSM prime references: JSONB array of NSM prime token_ids
    -- e.g. ["AA.AB.AC.AA.AB", "AA.AB.AC.AA.AC"] for "run" → MOVE + DO
    -- NULL = not yet mapped
    nsm_prime_refs  JSONB,

    -- Free-text disambiguation note (edge cases, near-synonym distinctions)
    nuance_note     TEXT,

    -- Population status
    status          TEXT        NOT NULL DEFAULT 'DRAFT'
                    CHECK (status IN ('DRAFT', 'REVIEWED', 'CONFIRMED')),

    created_at      TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at      TIMESTAMPTZ NOT NULL DEFAULT now(),

    UNIQUE (token_id, pos)
);

CREATE INDEX idx_glosses_token  ON token_glosses (token_id);
CREATE INDEX idx_glosses_status ON token_glosses (status);
```

After population, `token_pos.gloss_id` can be set:
```sql
UPDATE token_pos tp
SET gloss_id = tg.id
FROM token_glosses tg
WHERE tg.token_id = tp.token_id AND tg.pos = tp.pos;
```

### `token_variants` — Irregular and special surface forms

```sql
CREATE TABLE token_variants (
    id              SERIAL      PRIMARY KEY,

    -- Root token this variant resolves to
    canonical_id    TEXT        NOT NULL REFERENCES tokens (token_id) ON DELETE CASCADE,

    -- Surface form — always lowercase
    name            TEXT        NOT NULL,

    -- PoS scope (NULL = variant applies to all PoS roles of canonical)
    pos             pos_tag,

    -- Morpheme this variant represents:
    -- 'PAST', 'PLURAL', 'PROGRESSIVE', 'COMPARATIVE', 'SUPERLATIVE'
    -- 'ARCHAIC_PAST', 'ARCHAIC_PLURAL'   (archaic irregular)
    -- 'DIALECT', 'EYE_DIALECT'           (dialect/phonetic)
    -- 'SPELLING_US', 'SPELLING_UK', 'SPELLING_AU'   (orthographic)
    -- 'CONTRACTION'                      (casual elision: 'em, 'bout)
    -- 'MISSPELLING'                      (common misspelling — resolves but is marked)
    morpheme        TEXT,

    -- Characteristic bitmask — primarily derivation dimension (bits 20–27)
    -- plus applicable register/temporal/geographic bits
    characteristics INTEGER     NOT NULL DEFAULT 0,

    -- Source language (ISO 639-1/3) — set when BORROWING bit (24) is set
    source_language CHAR(3),

    -- Optional note (e.g. "misspelling of 'receive'", "Scottish dialect form")
    note            TEXT,

    UNIQUE (canonical_id, name, COALESCE(morpheme, ''))
);

CREATE INDEX idx_variants_canonical  ON token_variants (canonical_id);
CREATE INDEX idx_variants_name       ON token_variants (name);
CREATE INDEX idx_variants_morpheme   ON token_variants (morpheme) WHERE morpheme IS NOT NULL;
CREATE INDEX idx_variants_chars      ON token_variants (characteristics) WHERE characteristics != 0;
```

### `inflection_rules` — Parameterized regular inflection rules

Used by Postgres assembly queries to generate inflected surface forms on demand (env_vocab loading without storing every inflected form).

```sql
CREATE TABLE inflection_rules (
    id              SERIAL      PRIMARY KEY,
    morpheme        TEXT        NOT NULL,   -- 'PAST', 'PLURAL', 'PROGRESSIVE', etc.
    priority        INTEGER     NOT NULL,   -- Lower = checked first
    condition       TEXT        NOT NULL,   -- POSIX regex on root name
    strip_suffix    TEXT        NOT NULL DEFAULT '',  -- remove from root end
    add_suffix      TEXT        NOT NULL DEFAULT '',  -- append after strip
    description     TEXT,

    UNIQUE (morpheme, priority)
);
```

Example rows:

| morpheme | priority | condition | strip_suffix | add_suffix | description |
|----------|----------|-----------|--------------|------------|-------------|
| PAST | 1 | `[^aeiou]e$` | `e` | `ed` | silent-e drop: like→liked |
| PAST | 2 | `[^aeiou][^aeiou]$` | `` | `ed` | default +ed: walk→walked |
| PLURAL | 1 | `(s\|x\|z\|ch\|sh)$` | `` | `es` | sibilant: kiss→kisses |
| PLURAL | 2 | `[^aeiou]y$` | `y` | `ies` | consonant+y: city→cities |
| PLURAL | 99 | `.*` | `` | `s` | default +s: cat→cats |
| PROGRESSIVE | 1 | `[^aeiou]e$` | `e` | `ing` | silent-e: make→making |
| PROGRESSIVE | 99 | `.*` | `` | `ing` | default: walk→walking |
| ADVERB_LY | 1 | `[^aeiou]y$` | `y` | `ily` | happy→happily |
| ADVERB_LY | 99 | `.*` | `` | `ly` | default: quick→quickly |

Note: doubled-consonant patterns (tap→tapped, run→running) require CVC phonological analysis beyond simple regex. These are handled by a stored function `apply_doubling_rule(root TEXT, suffix TEXT) RETURNS TEXT` rather than this table.

---

## Envelope Queries Against New Schema

### Priority 1 — Labels (tier 0 broadphase)

```sql
SELECT t.name, t.token_id
FROM tokens t
JOIN token_pos tp ON tp.token_id = t.token_id AND tp.pos = 'N_PROPER'
WHERE t.proper_common = 'proper'
  AND length(t.name) BETWEEN 2 AND 16
ORDER BY t.freq_rank ASC NULLS LAST
```

### Priority 2–16 — Freq-ranked roots by length

```sql
-- Priority 2 (length 2):
SELECT name, token_id
FROM tokens
WHERE length(name) = 2
  AND freq_rank IS NOT NULL
  AND (characteristics & (1 << 8)) = 0   -- exclude ARCHAIC
ORDER BY freq_rank ASC LIMIT 1500
```

No `token_pos` join needed for basic vocab loading — characteristics summary on `tokens` is sufficient.

### Priority 17 — Irregular variants (core morphemes)

```sql
SELECT tv.name, tv.canonical_id AS token_id
FROM token_variants tv
JOIN tokens t ON t.token_id = tv.canonical_id
WHERE t.freq_rank IS NOT NULL
  AND (tv.characteristics & (1 << 21)) != 0   -- IRREGULAR bit
  AND (tv.characteristics & (1 << 8)) = 0      -- exclude ARCHAIC
  AND tv.morpheme IN ('PAST', 'PLURAL', 'PROGRESSIVE')
ORDER BY t.freq_rank ASC NULLS LAST
```

### Priority 18 — Postgres-assembled regular inflections

```sql
-- Generate regular past tense for top verbs (no stored irregular → regular rule applies)
SELECT
    CASE
        WHEN t.name ~ '[^aeiou]e$'  THEN left(t.name, length(t.name)-1) || 'ed'
        ELSE t.name || 'ed'
    END AS inflected_name,
    t.token_id
FROM tokens t
JOIN token_pos tp ON tp.token_id = t.token_id
WHERE tp.pos = 'V_MAIN'
  AND (tp.morpheme_accept & (1 << 1)) != 0    -- MORPH_PAST
  AND t.freq_rank IS NOT NULL
  AND NOT EXISTS (
    SELECT 1 FROM token_variants tv
    WHERE tv.canonical_id = t.token_id AND tv.morpheme = 'PAST'
  )
ORDER BY t.freq_rank ASC LIMIT 5000
```

The doubled-consonant case calls `apply_doubling_rule()` — left as a TODO for the implementation phase.

### `fiction_victorian` extras

```sql
-- Priority 19: Archaic + dialect variants
SELECT tv.name, tv.canonical_id AS token_id
FROM token_variants tv
JOIN tokens t ON t.token_id = tv.canonical_id
WHERE (tv.characteristics & ((1 << 8) | (1 << 12))) != 0  -- ARCHAIC or DIALECT
ORDER BY t.freq_rank ASC NULLS LAST

-- Priority 20: Literary-tagged roots
SELECT name, token_id
FROM tokens
WHERE (characteristics & (1 << 5)) != 0  -- LITERARY
  AND freq_rank IS NOT NULL
ORDER BY freq_rank ASC NULLS LAST
```

---

## Migration Plan

### Phase C — Apply new schema

1. Create new types and tables alongside existing ones (no DROP of existing yet)
2. The existing `tokens` table retains its structure during transition — new tables added in same DB
3. New tables: `token_pos`, `token_glosses`, `token_variants`, `inflection_rules`
4. Existing `tokens` columns to eventually retire: `layer`, `subcategory`, `aux_type`, `canonical_id`, `category` — kept until Kaikki population validates replacements

### Phase D — Kaikki population

See `docs/kaikki-tag-mapping.md` for tag→characteristic mapping.

1. **Pass 1 — Roots**: Create token rows from Kaikki non-form-of entries. Use sequential token_id allocation within AB namespace (p2/p3/p4/p5 auto-incremented — simple and correct since token_id identity comes from coordinates, not semantics).
2. **Pass 2 — token_pos**: Create PoS records. Set morpheme_accept defaults by PoS. Mark is_primary from Kaikki PoS ordering.
3. **Pass 3 — Glosses**: Create token_glosses rows from Kaikki `senses[0].glosses[0]`. Status = DRAFT.
4. **Pass 4 — Delta variants**: Forms with delta tags → token_variants with characteristics bitmask.
5. **Pass 5 — Alt-of variants**: Kaikki alt-of entries → token_variants.
6. **Pass 6 — Loanwords**: Etymology extraction → BORROWING bit + source_language.
7. **Pass 7 — Freq merge**: Merge existing freq_rank data by name lookup.
8. **Pass 8 — characteristics summary**: UPDATE tokens SET characteristics = (SELECT bit_or(characteristics) FROM token_pos WHERE ...) for each token.

### Phase E — LMDB compiler update

The existing `compile_vocab_lmdb.py` reads from `hcp_english.tokens`. Updated to read from new schema and write extended vbed entries: `[word | token_id (14 bytes) | pos_primary (1 byte) | characteristics (4 bytes)]`.

Coordination needed with engine specialist on vbed entry format change before this runs.

---

## What Changes vs Current Schema

| Aspect | Current | New |
|--------|---------|-----|
| PoS | `layer`/`subcategory`/`aux_type` columns | `token_pos` table, typed enum, `is_primary` flag |
| Glosses | Not stored | `token_glosses` table with status tracking |
| Variants | `canonical_id` on `tokens` | Separate `token_variants` table |
| Characteristics | `category` text column | `INTEGER` bitmask, 32-bit |
| Morpheme acceptance | Not stored | `morpheme_accept` bitmask on `token_pos` |
| Loanword origin | Not stored | `source_language CHAR(3)` on `tokens` + `token_variants` |
| N_PROPER cap type | `proper_common = 'proper'` only | + `cap_property` on `token_pos` |
| Inflected forms | Stored as tokens | Not stored; Postgres assembles from `inflection_rules` |
| LMDB vbed entries | `[word | token_id]` | `[word | token_id | pos_primary | characteristics]` (Phase E/F) |
