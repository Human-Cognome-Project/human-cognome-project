# hcp_english New Schema Design

**Author:** DB Specialist — 2026-03-10
**Status:** Draft for review
**Input:** `docs/kaikki-analysis.md` (linguist), Patrick's session brief

---

## Design Principles

1. **Roots only in the tokens table.** Regular inflections (walks, walked, walking) are not stored — the engine strips them. The DB stores roots and irregular/special deltas only.
2. **One token_id per root word form.** `"run"` gets one token_id regardless of how many PoS it covers. PoS disambiguation is downstream from the physics layer.
3. **PoS as structured metadata, not embedded in namespace.** A separate `token_pos` table holds (token, PoS) pairs with gloss, morpheme acceptance, and per-PoS characteristics.
4. **Morpheme acceptance as flags.** Each PoS record says WHAT inflections the root accepts. Postgres uses these flags to assemble inflected forms on demand for LMDB — no need to pre-store them.
5. **Variants for deltas only.** Irregular forms (`ran`, `went`, `was`), archaic/dialectal forms (`walketh`, `runnin'`), and spelling variants (`colour`/`color`) go in a separate `token_variants` table with morpheme type noted.
6. **Characteristic bitmask.** A `BIGINT` bitmask on tokens and token_pos encodes register, geography, and usage properties. No separate category text column.
7. **Labels lowercase in DB.** `proper_common = 'proper'` is the always-capitalize marker. The `name` field is always lowercase.

---

## PoS Types

```
N_COMMON    — common noun
N_PROPER    — proper noun (Label / always-capitalize)
N_PRONOUN   — pronoun (I, he, she, they, it, we, you)
V_MAIN      — main verb
V_AUX       — auxiliary verb (be, have, do, will, shall, may, might, can, could...)
ADJ         — adjective
ADV         — adverb
PREP        — preposition
CONJ_COORD  — coordinating conjunction (and, but, or, nor, for, yet, so)
CONJ_SUB    — subordinating conjunction (because, although, while, since...)
DET         — determiner (the, a, an, some, any, this, that, my, your...)
INTJ        — interjection (oh, ah, well, hey...)
PART        — particle (up in "give up", out in "find out")
NUM         — numeral
```

---

## Characteristic Bitmask

Single `BIGINT` used on both `tokens` and `token_pos`. Bits are:

### Token-level characteristics (stable across all PoS uses)
| Bit | Constant | Meaning |
|-----|----------|---------|
| 0 | `CHAR_PROPER` | Always-capitalize (Label); redundant with `proper_common` but queryable via bitmask |
| 1 | `CHAR_AMERICAN` | US English spelling or usage |
| 2 | `CHAR_BRITISH` | British English spelling or usage |
| 3 | `CHAR_AUSTRALIAN` | Australian English |
| 4 | `CHAR_CANADIAN` | Canadian English |
| 5 | `CHAR_BORROWING` | Loanword (non-native origin, retains foreign character) |
| 6 | `CHAR_ABBREVIATION` | Acronym, initialism, or abbreviation (e.g., NATO, TV, approx.) |

### Register / usage characteristics (may differ per PoS — stored on both token and token_pos)
| Bit | Constant | Meaning | Kaikki source tags |
|-----|----------|---------|-------------------|
| 8 | `CHAR_ARCHAIC` | Obsolete or archaic in current usage | obsolete, archaic, historical |
| 9 | `CHAR_DATED` | Old-fashioned but still understood | dated |
| 10 | `CHAR_SLANG` | Slang | slang, Internet |
| 11 | `CHAR_CASUAL` | Informal / colloquial | informal, colloquial, humorous, childish |
| 12 | `CHAR_VULGAR` | Vulgar | vulgar |
| 13 | `CHAR_DEROGATORY` | Derogatory, offensive, or slur | derogatory, offensive, slur, ethnic |
| 14 | `CHAR_LITERARY` | Literary or poetic | literary, poetic |
| 15 | `CHAR_FORMAL` | Formal register | formal |
| 16 | `CHAR_DIALECT` | Dialectal | dialectal, dialect |
| 17 | `CHAR_NEOLOGISM` | Recent / neologism | neologism, Internet (combined with SLANG) |
| 18 | `CHAR_EYE_DIALECT` | Pronunciation spelling (eye dialect) | pronunciation-spelling |
| 19 | `CHAR_MISSPELLING` | Known misspelling (variants only) | misspelling |

**Rule for token-level `characteristics`**: OR of all sense-level and PoS-level characteristic bits across all senses. This gives a fast filter without joining `token_pos`. For example, an `ARCHAIC` bit on `tokens` means at least one PoS usage is archaic; the precise PoS is in `token_pos`.

---

## Morpheme Acceptance Bitmask

Stored on `token_pos.morpheme_accept`. Tells Postgres what regular inflections to generate for this (token, PoS) entry. Used by envelope assembly queries.

| Bit | Constant | Meaning |
|-----|----------|---------|
| 0 | `MORPH_PLURAL` | Accepts regular plural (-s, -es, -ies) |
| 1 | `MORPH_PAST` | Accepts regular past (-ed, doubled-consonant+ed) |
| 2 | `MORPH_PROGRESSIVE` | Accepts progressive (-ing) |
| 3 | `MORPH_3RD_SING` | Accepts 3rd-person singular present (-s, -es) |
| 4 | `MORPH_COMPARATIVE` | Accepts comparative (-er) |
| 5 | `MORPH_SUPERLATIVE` | Accepts superlative (-est) |
| 6 | `MORPH_ADVERB_LY` | Accepts adverb formation (-ly) |
| 7 | `MORPH_POSSESSIVE` | Accepts possessive (-'s) |
| 8 | `MORPH_GERUND` | Accepts gerund (verb→noun via -ing) |

These bits are not exhaustive — they cover the rules the engine already applies. Additional bits can be added as the engine gains new rules.

---

## Schema

### `tokens` — Root words

```sql
CREATE TABLE tokens (
    -- Namespace coordinates (token_id generator)
    ns          CHAR(2)     NOT NULL DEFAULT 'AB',
    p2          CHAR(2)     NOT NULL,
    p3          CHAR(2)     NOT NULL,
    p4          CHAR(2)     NOT NULL,
    p5          CHAR(2)     NOT NULL,

    -- Generated identity
    token_id    TEXT        GENERATED ALWAYS AS (
                    ns || '.' || p2 || '.' || p3 || '.' || p4 || '.' || p5
                ) STORED NOT NULL,

    -- Surface form — always lowercase
    name        TEXT        NOT NULL,

    -- Label marker (always-capitalize proper noun)
    proper_common TEXT,          -- 'proper' | NULL

    -- Frequency rank (from merged Wikipedia/OpenSubtitles data)
    freq_rank   INTEGER,         -- NULL = unranked

    -- Loanword source language
    source_language CHAR(3),     -- ISO 639-1/3 (NULL = native English)

    -- Characteristic bitmask (token-level summary: bits 0–6, OR'd summary of bits 8–19)
    characteristics BIGINT      NOT NULL DEFAULT 0,

    -- Derived bucketing key for PBD bed assignment
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

**No `canonical_id` on tokens.** Root tokens are roots; they have no canonical. Variants live in `token_variants`.

### `token_pos` — PoS records

```sql
CREATE TYPE pos_tag AS ENUM (
    'N_COMMON', 'N_PROPER', 'N_PRONOUN',
    'V_MAIN', 'V_AUX',
    'ADJ', 'ADV', 'PREP',
    'CONJ_COORD', 'CONJ_SUB', 'DET', 'INTJ', 'PART', 'NUM'
);

CREATE TABLE token_pos (
    id              SERIAL      PRIMARY KEY,
    token_id        TEXT        NOT NULL REFERENCES tokens (token_id) ON DELETE CASCADE,
    pos             pos_tag     NOT NULL,

    -- Primary gloss (first sense from Kaikki, or manually entered)
    gloss           TEXT,

    -- What regular inflections this (token, PoS) accepts
    morpheme_accept BIGINT      NOT NULL DEFAULT 0,

    -- Per-PoS characteristics (register, usage — bits 8–19)
    characteristics BIGINT      NOT NULL DEFAULT 0,

    UNIQUE (token_id, pos)
);

CREATE INDEX idx_token_pos_token  ON token_pos (token_id);
CREATE INDEX idx_token_pos_pos    ON token_pos (pos);
CREATE INDEX idx_token_pos_morph  ON token_pos (morpheme_accept) WHERE morpheme_accept != 0;
```

### `token_variants` — Irregular and special forms

```sql
CREATE TABLE token_variants (
    id              SERIAL      PRIMARY KEY,

    -- Points to the root token
    canonical_id    TEXT        NOT NULL REFERENCES tokens (token_id) ON DELETE CASCADE,

    -- Surface form of this variant (always lowercase)
    name            TEXT        NOT NULL,

    -- Which PoS this variant applies to (NULL = applies to all PoS of the canonical)
    pos             pos_tag,

    -- What morpheme this represents
    -- Examples: 'PAST', 'PLURAL', 'PROGRESSIVE', 'ARCHAIC', 'DIALECT',
    --           'SPELLING_US', 'SPELLING_UK', 'CONTRACTION', 'MISSPELLING'
    morpheme        TEXT,

    -- Characteristic bitmask (bits 8–19: register/usage; bits 1–4: geographic)
    characteristics BIGINT      NOT NULL DEFAULT 0,

    -- Optional note (e.g., "archaic past tense", "misspelling of 'receive'")
    note            TEXT,

    UNIQUE (canonical_id, name, COALESCE(morpheme, ''))
);

CREATE INDEX idx_variants_canonical ON token_variants (canonical_id);
CREATE INDEX idx_variants_name      ON token_variants (name);
CREATE INDEX idx_variants_morpheme  ON token_variants (morpheme) WHERE morpheme IS NOT NULL;
```

### `inflection_rules` — Parameterized regular inflection rules

```sql
CREATE TABLE inflection_rules (
    id          SERIAL      PRIMARY KEY,
    morpheme    TEXT        NOT NULL,   -- 'PAST', 'PLURAL', 'PROGRESSIVE', etc.
    priority    INTEGER     NOT NULL,   -- Lower number = checked first
    condition   TEXT        NOT NULL,   -- POSIX regex on root name (applied in Postgres)
    transform   TEXT        NOT NULL,   -- SQL expression: e.g. 'name || ''ed'''
    -- or strip_suffix + add_suffix for clarity:
    strip_suffix TEXT,                  -- suffix to remove from name before adding
    add_suffix  TEXT,                   -- suffix to add
    description TEXT,

    UNIQUE (morpheme, priority)
);
```

Example rows:
```
morpheme='PAST', priority=1, condition='^.+[^aeiou]e$',          strip_suffix='e',   add_suffix='ed',  description='silent-e drop: like→liked'
morpheme='PAST', priority=2, condition='^.+[aeiou][bcdfgklmnprstvz]$', strip_suffix='',  add_suffix='ed',  description='doubled consonant: tap→tapped' (NOTE: requires doubling logic — see below)
morpheme='PAST', priority=99, condition='.*',                     strip_suffix='',    add_suffix='ed',  description='default: walk→walked'
morpheme='PLURAL', priority=1, condition='^.+(s|x|z|ch|sh)$',    strip_suffix='',    add_suffix='es',  description='sibilant +es: kiss→kisses'
morpheme='PLURAL', priority=2, condition='^.+[^aeiou]y$',        strip_suffix='y',   add_suffix='ies', description='consonant+y→ies: city→cities'
morpheme='PLURAL', priority=99, condition='.*',                   strip_suffix='',    add_suffix='s',   description='default +s: cat→cats'
```

Note: doubled-consonant rules (tapping: `tap→tapped`) require phonological logic (CVC pattern check) that a simple regex+suffix can't express. These are better handled as a stored function rather than table rows.

---

## How Envelope Queries Work Against the New Schema

### Priority 1 — Labels (proper nouns, tier 0 broadphase)

```sql
SELECT name, token_id
FROM tokens
WHERE proper_common = 'proper'
  AND length(name) BETWEEN 2 AND 16
ORDER BY freq_rank ASC NULLS LAST
```

No `canonical_id IS NULL` guard needed — the new schema has no canonical_id on the tokens table; root tokens are all canonical by definition.

### Priorities 2–16 — Freq-ranked common vocab (short words first)

```sql
-- Priority 2: length 2 (function words — they, of, to, is, was, ...)
SELECT name, token_id
FROM tokens
WHERE length(name) = 2
  AND freq_rank IS NOT NULL
  AND (characteristics & (1 << 8)) = 0  -- exclude ARCHAIC
ORDER BY freq_rank ASC LIMIT 1500
```

Same pattern for lengths 3–16 at priorities 3–16.

### Priority 17 — Irregular variant forms

```sql
SELECT tv.name, tv.canonical_id AS token_id
FROM token_variants tv
JOIN tokens t ON t.token_id = tv.canonical_id
WHERE t.freq_rank IS NOT NULL
  AND (tv.characteristics & (1 << 8)) = 0  -- exclude ARCHAIC
  AND tv.morpheme IN ('PAST', 'PLURAL', 'PROGRESSIVE')  -- core irregular inflections
ORDER BY t.freq_rank ASC NULLS LAST
```

This loads `ran → run.token_id`, `went → go.token_id`, `mice → mouse.token_id` etc. into `env_vocab`. The engine looks up the variant form, gets the canonical token_id directly.

### Priority 18 — Postgres-assembled regular inflections (tense-preloading)

For contexts where early morpheme detection signals likely tense context (past tense heavy text), Postgres can pre-generate regular inflected forms and serve them into `env_vocab` without storing them:

```sql
-- Generate regular past tense for top-ranked verbs (no stored variant = regular form applies)
SELECT
    CASE
        WHEN t.name ~ '[^aeiou]e$'   THEN t.name || 'd'
        WHEN t.name ~ '[aeiou][bcdfgklmnprstvz]$' AND length(t.name) BETWEEN 3 AND 6
             THEN t.name || right(t.name, 1) || 'ed'  -- simplified doubling
        ELSE t.name || 'ed'
    END AS inflected_name,
    t.token_id
FROM tokens t
JOIN token_pos tp ON tp.token_id = t.token_id AND tp.pos = 'V_MAIN'
WHERE (tp.morpheme_accept & (1 << 1)) != 0  -- ACCEPTS_PAST
  AND t.freq_rank IS NOT NULL
  AND NOT EXISTS (
    SELECT 1 FROM token_variants tv
    WHERE tv.canonical_id = t.token_id AND tv.morpheme = 'PAST'
  )
ORDER BY t.freq_rank ASC LIMIT 5000
```

This is an assembly query: Postgres generates the surface form on the fly, LMDB caches it. No separate stored rows for `walked`, `talked`, `called`.

**Note:** The doubling rule (tap→tapped) needs careful implementation — a single regex can't reliably identify CVC patterns for doubling. Recommend a stored function `apply_regular_past(root TEXT) RETURNS TEXT` rather than inline CASE.

### `fiction_victorian` additional queries

```sql
-- Priority 19: Archaic/dialectal variants for period fiction
SELECT tv.name, tv.canonical_id AS token_id
FROM token_variants tv
JOIN tokens t ON t.token_id = tv.canonical_id
WHERE (tv.characteristics & ((1 << 8) | (1 << 16))) != 0  -- ARCHAIC or DIALECT
ORDER BY t.freq_rank ASC NULLS LAST

-- Priority 20: LITERARY-tagged tokens
SELECT t.name, t.token_id
FROM tokens t
WHERE (t.characteristics & (1 << 14)) != 0  -- LITERARY
  AND t.freq_rank IS NOT NULL
ORDER BY t.freq_rank ASC NULLS LAST
```

---

## Kaikki Population Plan

### Pass 1 — Root tokens

For each Kaikki entry where no sense has `form-of` tag AND PoS is in our target set:
- Create token row: `name = lower(word)`, freq_rank later from frequency data
- Set `proper_common = 'proper'` if PoS is `name`
- Set token-level `characteristics` from etymology (BORROWING, AMERICAN, BRITISH)

For each PoS (some words have multiple PoS entries in Kaikki):
- Create `token_pos` row: pos from PoS mapping, gloss from first sense, morpheme_accept from PoS defaults (nouns get MORPH_PLURAL, verbs get MORPH_PAST | MORPH_PROGRESSIVE | MORPH_3RD_SING, adjectives get MORPH_COMPARATIVE | MORPH_SUPERLATIVE | MORPH_ADVERB_LY)
- Set `token_pos.characteristics` from sense tags (slang, archaic, etc.)

### Pass 2 — Delta variants (from forms[])

For each entry's `forms[]` array:
- Skip forms with only standard rule tags (plural, past, present, participle, superlative, comparative, singular, third-person, gerund, indicative, second-person, first-person, subjunctive, imperative, infinitive)
- Keep forms with any delta tag: {archaic, obsolete, dialectal, US, UK, Australia, rare, nonstandard, pronunciation-spelling, historical, dated, alternative}
- Create `token_variants` row: name = form, canonical_id = token, morpheme from form tag context, characteristics from form tags

### Pass 3 — Alt-of entries (spelling/archaic variants)

For each Kaikki entry with `alt-of` sense:
- These are canonical variants of another word
- Create `token_variants` row pointing to the alt-of target
- Set morpheme = 'SPELLING_UK'/'SPELLING_US'/'ARCHAIC' based on tags

### Pass 4 — Loanwords

For each entry with etymology_text containing BORROWING signals:
- Regex on common patterns: "from French", "from Latin", "borrowed from", "from Japanese", etc.
- Set `CHAR_BORROWING` bit + `source_language` ISO code on the token row

### Pass 5 — Derogatory / Vulgar

For entries with sense tags `derogatory`, `offensive`, `slur`, `vulgar`:
- Set `CHAR_DEROGATORY` or `CHAR_VULGAR` on `token_pos.characteristics`
- OR into `tokens.characteristics` summary bit

### Pass 6 — Misspellings

For Kaikki entries with `misspelling` tag:
- Create `token_variants` row with `CHAR_MISSPELLING` bit set + `note = 'misspelling of X'`
- These resolve to the correct form's token_id so text using the misspelling still resolves

---

## Migration from Current hcp_english

Current schema has ~1.4M tokens with mixed roots + inflected forms + variants in a flat `tokens` table. The new schema is a rebuild from source (Kaikki), not a migration in place.

Planned approach:
1. Create new tables in hcp_english (or a new `hcp_english_v2` DB for parallel running)
2. Run Kaikki population passes
3. Merge frequency ranks from existing `freq_rank` data (by name lookup)
4. Repoint engine to new schema for testing
5. Retire old tables once validated

The current `canonical_id` + `proper_common` work from migrations 025–028 remains valid — the Label tier design (proper_common='proper', lowercase names) carries forward directly.

---

## Open Questions for Patrick

**Q1. Token_id allocation for new entries**: The current token_id is generated from `ns/p2/p3/p4/p5` namespace coordinates. For the fresh Kaikki import, how should p2/p3/p4/p5 be allocated? Options:
- a) Sequential auto-increment within the AB namespace (simple, opaque)
- b) Meaningful namespace coordinates (p2 = PoS block, p3/p4/p5 = sequential)
- c) Hash of (name, PoS) for deterministic regeneration

**Q2. Multi-PoS token_id**: Under "one token_id per root form", `"run"` (noun + verb) has one token_id. The PoS disambiguation is in `token_pos`. Is this correct, or should noun-"run" and verb-"run" have separate token_ids (different force profiles)? The physics layer currently sees one surface form — one match. Downstream disambiguation can use token_pos. But if the forces are fundamentally different (noun "run" = a result, verb "run" = an action), separate token_ids might be needed from the start.

**Q3. Postgres-assembled inflected forms**: Is the tense-preloading envelope query pattern (Priority 18 above) the right direction? It generates regular inflected forms in SQL and loads them into LMDB — no stored rows. Alternatively, the engine's rule stripper handles all regular forms, and the envelope only loads roots + irregular variants. Which model?

**Q4. `inflection_rules` table**: Should the regular inflection rules be stored in Postgres (the `inflection_rules` table design above) for use by assembly queries, or hardcoded in the engine's rule stripper only? Storing in Postgres enables Postgres-side assembly but requires keeping two copies of the rules in sync.

**Q5. Misspellings**: Store as variants in `token_variants` with `CHAR_MISSPELLING` bit + note? They appear in real text and should resolve, but shouldn't be reinforced. Recommend yes — store but mark clearly. Any objection?

---

## Summary of Changes from Current Schema

| Aspect | Current | New |
|--------|---------|-----|
| PoS storage | `layer`/`subcategory` text columns on tokens | Separate `token_pos` table, typed enum |
| Inflected forms | Stored as tokens with same table | Not stored; generated by engine rules |
| Irregular variants | `canonical_id` on tokens table | Separate `token_variants` table with morpheme field |
| Characteristic data | Free-form `category` column | Typed bitmask on tokens + token_pos |
| Morpheme acceptance | Not stored | `morpheme_accept` bitmask on token_pos |
| Gloss | Not stored | `gloss` on token_pos (from Kaikki first sense) |
| Label markers | `proper_common = 'proper'` (from migrations 026–028) | Same, carries forward |
| Envelope assembly | Queries return stored forms only | Queries can generate inflected forms on demand |
