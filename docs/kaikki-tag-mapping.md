# Kaikki → HCP Tag Mapping

**Author:** DB Specialist — 2026-03-10
**Input:** `docs/kaikki-analysis.md`, `docs/language-shard-restructure-spec.md`

This document is the definitive mapping for the Kaikki → hcp_english population script. It covers:
1. Sense/entry tags → characteristic bitmask bits
2. Kaikki PoS strings → HCP pos_tag enum values
3. forms[] tags → morpheme + characteristic classification
4. Etymology patterns → source_language (ISO 639-1/3)

---

## Characteristic Bit Constants

```python
# Register dimension (bits 0–7)
FORMAL       = 1 << 0   # 0x00000001
CASUAL       = 1 << 1   # 0x00000002
SLANG        = 1 << 2   # 0x00000004
VULGAR       = 1 << 3   # 0x00000008
DEROGATORY   = 1 << 4   # 0x00000010
LITERARY     = 1 << 5   # 0x00000020
TECHNICAL    = 1 << 6   # 0x00000040
# bit 7 reserved (NEUTRAL = absence of register bits)

# Temporal dimension (bits 8–11)
ARCHAIC      = 1 << 8   # 0x00000100
DATED        = 1 << 9   # 0x00000200
NEOLOGISM    = 1 << 10  # 0x00000400
# bit 11 reserved

# Geographic dimension (bits 12–15)
DIALECT      = 1 << 12  # 0x00001000
BRITISH      = 1 << 13  # 0x00002000
AMERICAN     = 1 << 14  # 0x00004000
AUSTRALIAN   = 1 << 15  # 0x00008000
# bits 16–19 reserved for future geographic variants

# Derivation dimension (bits 20–27)
STANDARD_RULE    = 1 << 20  # 0x00100000
IRREGULAR        = 1 << 21  # 0x00200000
SPELLING_VARIANT = 1 << 22  # 0x00400000
EYE_DIALECT      = 1 << 23  # 0x00800000
BORROWING        = 1 << 24  # 0x01000000
COMPOUND         = 1 << 25  # 0x02000000
ABBREVIATION     = 1 << 26  # 0x04000000
# bit 27 reserved
```

---

## 1. Sense/Entry Tags → Characteristic Bitmask

### Register

| Kaikki tag | Characteristic bits | Count in data | Notes |
|------------|---------------------|---------------|-------|
| `formal` | `FORMAL` | 799 | |
| `literary` | `LITERARY` | 967 | |
| `poetic` | `LITERARY` | 1,542 | Treat same as literary |
| `informal` | `CASUAL` | 18,397 | |
| `colloquial` | `CASUAL` | 6,940 | |
| `humorous` | `CASUAL` | 4,413 | Light casual |
| `childish` | `CASUAL` | 638 | Child speech register |
| `slang` | `SLANG` | 27,371 | |
| `Internet` | `SLANG \| NEOLOGISM` | 6,094 | Internet slang is typically neologism too |
| `vulgar` | `VULGAR` | 4,644 | |
| `derogatory` | `DEROGATORY` | 8,561 | |
| `offensive` | `DEROGATORY` | 1,887 | Same bit — note field captures distinction |
| `slur` | `DEROGATORY` | 834 | Strongest signal — add `note: "slur"` |
| `ethnic` | `DEROGATORY` | 767 | Only when co-occurring with slur/offensive |
| `technical` | `TECHNICAL` | (various) | |
| `euphemistic` | `CASUAL` | 1,929 | No separate bit — euphemism is register |
| `nonstandard` | `CASUAL \| DIALECT` | 3,676 | Both bits; context determines emphasis |

### Temporal

| Kaikki tag | Characteristic bits | Count in data | Notes |
|------------|---------------------|---------------|-------|
| `obsolete` | `ARCHAIC` | 39,651 | Strongest archaic signal |
| `archaic` | `ARCHAIC` | 24,452 | Direct mapping |
| `historical` | `ARCHAIC` | 16,361 | Archaic in practice |
| `dated` | `DATED` | 11,172 | Falling out of use |
| `neologism` | `NEOLOGISM` | 1,262 | |
| `uncommon` | (none — use freq_rank) | 3,328 | Weak signal; freq_rank handles rarity better |
| `rare` | (none — use freq_rank) | (various) | Same as uncommon |

### Geographic

| Kaikki tag | Characteristic bits | Count in data | Notes |
|------------|---------------------|---------------|-------|
| `US` | `AMERICAN` | 17,136 | |
| `UK` | `BRITISH` | 12,332 | |
| `British` | `BRITISH` | 4,167 | |
| `Australia` | `AUSTRALIAN` | 4,259 | |
| `dialectal` | `DIALECT` | 5,060 | |
| `Scotland` | `BRITISH \| DIALECT` | 3,483 | Scottish is British regional |
| `Ireland` | `BRITISH \| DIALECT` | 2,743 | Irish English is British regional |
| `Northern-England` | `BRITISH \| DIALECT` | 1,123 | |
| `Southern-US` | `AMERICAN \| DIALECT` | 367 | |
| `Commonwealth` | `BRITISH` | 944 | Broad British Commonwealth |
| `Canada` | (no bit yet — use note) | 3,587 | Bits 16–19 reserved; record in note for now |
| `India` | (no bit yet — use note) | 2,196 | Future |
| `New-Zealand` | (no bit yet — use note) | 1,763 | Future |
| `South-Africa` | (no bit yet — use note) | 1,024 | Future |
| `pronunciation-spelling` | `EYE_DIALECT \| CASUAL` | 2,574 | Phonetic spelling of speech |

### Derivation / structural (sense-level — usually triggers variant creation)

| Kaikki tag | Action | Notes |
|------------|--------|-------|
| `form-of` | Skip sense; process as variant | 525,941 — standard inflected form |
| `alt-of` | Create variant record | 163,377 — spelling/archaic/dialect variant |
| `misspelling` | Create variant with `DEROGATORY`? No — create with note only | 6,697 — common misspelling |
| `abbreviation` | Set `ABBREVIATION` on token | |
| `initialism` | Set `ABBREVIATION` on token | |
| `acronym` | Set `ABBREVIATION` on token | |
| `clipping` | Set `ABBREVIATION \| CASUAL` | |
| `contraction` | Handle via variant rules (morpheme = 'CONTRACTION') | |
| `idiomatic` | Keep root token, note idiomatic sense in gloss | Not a characteristic bit |
| `figuratively` | Keep root; no characteristic bit | Sense nuance |
| `mildly` | Qualifier — ignore as standalone tag | |

---

## 2. Kaikki PoS → HCP pos_tag

| Kaikki PoS | HCP pos_tag | Notes |
|-----------|-------------|-------|
| `noun` | `N_COMMON` | |
| `verb` | `V_MAIN` | Auxiliaries identified by word list post-hoc |
| `name` | `N_PROPER` | Labels — set proper_common='proper' |
| `adj` | `ADJ` | |
| `adv` | `ADV` | |
| `pron` | `N_PRONOUN` | |
| `prep` | `PREP` | |
| `conj` | `CONJ_COORD` or `CONJ_SUB` | Check gloss: "and/but/or" → COORD; "because/although" → SUB |
| `det` | `DET` | |
| `intj` | `INTJ` | |
| `particle` | `PART` | |
| `num` | `NUM` | |
| `phrase` | (skip) | Multi-token — future work |
| `prep_phrase` | (skip) | Multi-token |
| `proverb` | (skip) | Multi-token |
| `prefix` | (skip) | Sub-word morpheme |
| `suffix` | (skip) | Sub-word morpheme |
| `contraction` | (skip entry; process as variant) | Handle via variant rules |
| `symbol` | (skip) | |
| `character` | (skip) | |

**V_AUX and V_COPULA**: Kaikki doesn't distinguish these — they are all tagged `verb`. Post-process using a known auxiliary word list:
- `V_AUX`: be, have, do, will, shall, may, might, can, could, would, should, must, need, dare, ought, used
- `V_COPULA`: be (specifically as linking verb — same token, both V_AUX and V_COPULA records)

---

## 3. forms[] Tags → Morpheme Classification

### Standard rule tags (SKIP — do not create variant records)

These forms are derivable by rule at runtime. Recording them wastes space.

```python
STANDARD_RULE_TAGS = {
    'plural', 'singular',
    'past', 'present',
    'participle', 'past participle', 'present participle',
    'gerund', 'gerund-participle',
    'superlative', 'comparative',
    'third-person', 'second-person', 'first-person',
    'singular',
    'indicative', 'subjunctive', 'imperative', 'infinitive',
    'third-person singular',
}
```

### Delta tags (CREATE variant record)

| forms[] tag | morpheme field | characteristics bits | Notes |
|-------------|----------------|---------------------|-------|
| `archaic` | `'ARCHAIC'` | `ARCHAIC \| IRREGULAR` | |
| `obsolete` | `'ARCHAIC'` | `ARCHAIC \| IRREGULAR` | Same treatment |
| `historical` | `'ARCHAIC'` | `ARCHAIC` | |
| `dialectal` | `'DIALECT'` | `DIALECT \| IRREGULAR` | |
| `US` | `'SPELLING_US'` | `AMERICAN \| SPELLING_VARIANT` | |
| `UK` | `'SPELLING_UK'` | `BRITISH \| SPELLING_VARIANT` | |
| `Australia` | `'SPELLING_AU'` | `AUSTRALIAN \| SPELLING_VARIANT` | |
| `rare` | (skip or create with freq_rank note) | (none) | Low value; skip unless it has other tags |
| `nonstandard` | `'DIALECT'` | `DIALECT \| CASUAL` | |
| `pronunciation-spelling` | `'EYE_DIALECT'` | `EYE_DIALECT \| CASUAL` | |
| `dated` | `'ARCHAIC'` | `DATED` | Weaker than ARCHAIC |
| `alternative` | `'SPELLING_VARIANT'` | `SPELLING_VARIANT` | Check for geographic co-tags |

### Decision logic for a form entry:

```python
def classify_form(form_tags: list[str]) -> tuple[str | None, int]:
    """Returns (morpheme, characteristics) or (None, 0) to skip."""

    tags = set(form_tags)

    # If only standard rule tags → skip
    if tags.issubset(STANDARD_RULE_TAGS):
        return None, 0

    # Calculate delta tags present
    delta_tags = tags - STANDARD_RULE_TAGS

    chars = 0
    morpheme_parts = []

    if 'archaic' in delta_tags or 'obsolete' in delta_tags:
        chars |= ARCHAIC | IRREGULAR
        morpheme_parts.append('ARCHAIC')
    if 'dialectal' in delta_tags or 'nonstandard' in delta_tags:
        chars |= DIALECT | IRREGULAR
        morpheme_parts.append('DIALECT')
    if 'pronunciation-spelling' in delta_tags:
        chars |= EYE_DIALECT | CASUAL
        morpheme_parts.append('EYE_DIALECT')
    if 'historical' in delta_tags or 'dated' in delta_tags:
        chars |= DATED
        if not morpheme_parts:
            morpheme_parts.append('ARCHAIC')
    if 'US' in delta_tags:
        chars |= AMERICAN | SPELLING_VARIANT
        morpheme_parts.append('SPELLING_US')
    if 'UK' in delta_tags:
        chars |= BRITISH | SPELLING_VARIANT
        morpheme_parts.append('SPELLING_UK')
    if 'Australia' in delta_tags:
        chars |= AUSTRALIAN | SPELLING_VARIANT
        morpheme_parts.append('SPELLING_AU')
    if 'alternative' in delta_tags and not morpheme_parts:
        # Alternative with no geographic context = generic spelling variant
        chars |= SPELLING_VARIANT
        morpheme_parts.append('SPELLING_VARIANT')

    if not morpheme_parts:
        return None, 0  # delta tags but nothing we handle — skip

    # Combine morpheme context with the standard rule it's a variant of
    # e.g. if form tags include 'past' AND 'archaic': morpheme = 'ARCHAIC_PAST'
    base_morph = None
    for t in ['past', 'plural', 'present', 'participle', 'comparative', 'superlative']:
        if t in tags:
            base_morph = t.upper()
            break

    morpheme = morpheme_parts[0]
    if base_morph and morpheme in ('ARCHAIC', 'DIALECT'):
        morpheme = morpheme + '_' + base_morph

    return morpheme, chars
```

---

## 4. Etymology → source_language

Extract source language from `etymology_text` field. Regex-based heuristic — sufficient for first pass.

### Patterns

```python
ETYMOLOGY_PATTERNS = [
    # Most common: "from X" at start or after comma
    (r'\bfrom (?:Middle |Old |Early |Late )?French\b',  'fr'),
    (r'\bfrom (?:Middle |Old |Early |Late )?Latin\b',   'la'),
    (r'\bfrom (?:Old |Middle )?English\b',              None),   # Native — no BORROWING
    (r'\bfrom (?:Middle |Old )?High German\b',          'de'),
    (r'\bfrom (?:Old |Middle )?German\b',               'de'),
    (r'\bfrom (?:Old |Middle )?Dutch\b',                'nl'),
    (r'\bfrom Italian\b',                               'it'),
    (r'\bfrom Spanish\b',                               'es'),
    (r'\bfrom Portuguese\b',                            'pt'),
    (r'\bfrom Greek\b',                                 'el'),
    (r'\bfrom Ancient Greek\b',                         'grc'),
    (r'\bfrom Old Norse\b',                             'non'),
    (r'\bfrom Norse\b',                                 'non'),
    (r'\bfrom Japanese\b',                              'ja'),
    (r'\bfrom Arabic\b',                                'ar'),
    (r'\bfrom Persian\b',                               'fa'),
    (r'\bfrom Russian\b',                               'ru'),
    (r'\bfrom Chinese\b',                               'zh'),
    (r'\bfrom Hebrew\b',                                'he'),
    (r'\bfrom Sanskrit\b',                              'sa'),
    (r'\bfrom Turkish\b',                               'tr'),
    (r'\bfrom Hindi\b',                                 'hi'),

    # "borrowed from"
    (r'\bborrowed from (?:the )?(\w+)',                 'match_group_1'),  # generic fallback

    # "of X origin"
    (r'\bof French origin\b',                          'fr'),
    (r'\bof Latin origin\b',                           'la'),
    (r'\bof Germanic origin\b',                        'de'),
]
```

### Rules:

1. Match patterns in order (most specific first within each language). Take the first match.
2. If match returns `None` (Old English, Middle English): **do NOT set BORROWING bit**. Old English = native English; not a loanword.
3. If match returns a language code: set `BORROWING` bit + `source_language` on the token.
4. If no pattern matches: leave source_language NULL, no BORROWING bit.
5. Old French (`fro`) → `fr` (French). The distinction matters for historical linguistics but not for our loading purposes.

### Special cases:

| Pattern | Treatment |
|---------|-----------|
| Old English / Anglo-Saxon etymology | Native — no BORROWING, no source_language |
| Old French / Norman French | BORROWING + `fr` |
| Latin (scholarly coinage) | BORROWING + `la` (e.g., "versus", "et cetera", "veto") |
| Latin (fully assimilated) | BORROWING + `la` (tag all; engine treats as normal tokens) |
| "ultimately from X" | Ignore — too distant. Direct borrowing language takes precedence. |

---

## 5. alt-of / form-of Variant Creation

### alt-of entries

```python
# Kaikki entry: word='colour', pos='noun', senses=[{alt_of: [{word: 'color'}], tags: ['British']}]
# → create token_variants row:
#   canonical_id = lookup_token_id('color')
#   name = 'colour'
#   pos = 'N_COMMON'
#   morpheme = 'SPELLING_UK'
#   characteristics = BRITISH | SPELLING_VARIANT
```

### form-of entries (inflected — check for delta tags)

```python
# Kaikki entry: word='ran', pos='verb', senses=[{form_of: [{word: 'run'}], tags: ['past']}]
# → check: 'past' is in STANDARD_RULE_TAGS → skip (engine derives past tense)

# Kaikki entry: word='spake', pos='verb', senses=[{form_of: [{word: 'speak'}], tags: ['past', 'archaic']}]
# → check: 'archaic' is a delta tag → create variant:
#   canonical_id = lookup_token_id('speak')
#   name = 'spake'
#   morpheme = 'ARCHAIC_PAST'
#   characteristics = ARCHAIC | IRREGULAR
```

---

## 6. Contraction Handling

Kaikki `contraction` PoS entries and contraction-tagged sense entries are handled as variant records, not root tokens.

| Contraction | Root | morpheme | characteristics |
|-------------|------|----------|-----------------|
| `don't` | `do` | `CONTRACTION_NEG` | `CASUAL` |
| `can't` | `can` | `CONTRACTION_NEG` | `CASUAL` |
| `won't` | `will` | `CONTRACTION_NEG` | `CASUAL` |
| `'em` | `them` | `CONTRACTION` | `CASUAL` |
| `'bout` | `about` | `CONTRACTION` | `CASUAL` |
| `'cause` | `because` | `CONTRACTION` | `CASUAL` |
| `gonna` | `go` | `EYE_DIALECT` | `EYE_DIALECT \| CASUAL` |
| `wanna` | `want` | `EYE_DIALECT` | `EYE_DIALECT \| CASUAL` |
| `gotta` | `have` | `EYE_DIALECT` | `EYE_DIALECT \| CASUAL` |
| `'twas` | `it` | `CONTRACTION` | `CASUAL \| ARCHAIC` |
| `ain't` | `be` or `have` | `CONTRACTION_NEG` | `CASUAL \| DIALECT` |

Apostrophe contractions (`n't`, `'re`, `'ve`, `'ll`, `'s`, `'m`, `'d`) already handled by the engine's morph bit system — DO NOT create variant records for these. They are positional modifiers, not stored forms.

---

## 7. Misspelling Handling

Create variant records for Kaikki-tagged misspellings. They appear in real text and should resolve, but are clearly marked.

```python
# characteristics = 0 (no register/temporal/geographic bits)
# morpheme = 'MISSPELLING'
# note = f"misspelling of '{correct_form}'"
```

No characteristic bit assigned for MISSPELLING (per spec: "store as variant with NO characteristic bit but with note"). This allows them to resolve correctly while the note is available for quality filtering.

---

## 8. Scale Estimates After Filtering

Based on Kaikki analysis + these rules:

| Category | Estimated rows | Table |
|----------|----------------|-------|
| Root tokens (all PoS types, non-form-of) | ~900K | `tokens` |
| PoS records (1.1 avg per root) | ~1M | `token_pos` |
| Gloss records (same as PoS records) | ~1M | `token_glosses` |
| Irregular variants (IRREGULAR bit) | ~50–100K | `token_variants` |
| Spelling variants (SPELLING_VARIANT) | ~20–30K | `token_variants` |
| Archaic variants (ARCHAIC forms) | ~40K | `token_variants` |
| Eye dialect / casual contractions | ~5K | `token_variants` |
| Misspellings | ~6K | `token_variants` |

Total token_variants: ~120–180K rows.

Compare to current hcp_english: 1.4M tokens (roots + inflected forms mixed). The new schema is ~35% of current size but structured and query-optimised.
