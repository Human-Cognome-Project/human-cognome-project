# Kaikki English Data Analysis
*Linguist specialist — 2026-03-10*

Source: `/opt/project/sources/data/kaikki/english.jsonl`
Extracted: 2026-03-07 from enwiktionary dump dated 2026-03-03.

---

## Summary Statistics

| Metric | Value |
|--------|-------|
| Total entries | 1,454,988 |
| Unique words (lowercase) | 1,323,204 |
| Words with multiple PoS | 84,051 (6.4%) |
| Total senses | 1,739,532 |
| Senses with gloss text | 1,738,464 (99.9%) |
| Entries with etymology_text | 521,930 (35.9%) |
| Entries with forms[] | 564,516 (38.8%) |

---

## PoS Distribution

| Kaikki PoS | Count | HCP mapping |
|-----------|-------|-------------|
| noun | 810,627 | N_COMMON |
| verb | 217,909 | V_MAIN |
| name | 193,471 | N_PROPER (Label) |
| adj | 182,437 | ADJ |
| adv | 27,187 | ADV |
| phrase | 5,061 | (multi-token, skip) |
| intj | 4,757 | INTJ |
| prep_phrase | 2,973 | (multi-token, skip) |
| prefix | 2,443 | (morpheme, skip) |
| suffix | 1,589 | (morpheme, skip) |
| proverb | 1,546 | (multi-token, skip) |
| pron | 991 | N_PRONOUN |
| contraction | 861 | (handle via variant rules) |
| prep | 856 | PREP |
| num | 630 | NUM |
| symbol | 469 | (skip) |
| conj | 374 | CONJ_COORD / CONJ_SUB |
| det | 327 | DET |
| character | 190 | (skip) |
| particle | 105 | PART |

**Notes**:
- `phrase`, `prep_phrase`, `proverb` = multi-token. Skip for now — HCP handles multi-token structures separately.
- `prefix`, `suffix` = sub-word morphemes. Not vocab tokens.
- `contraction` entries in Kaikki = mostly the n't/'re/'ll etc. contractions. Handle via existing variant/morph rules, not as root tokens.
- `name` (193K) = N_PROPER / Labels. Very large set — includes all personal names, place names, etc.

---

## Sense-Level Tags — Complete Relevant Set

These are the tags we care about for characteristic bitmask assignment. Grouped by HCP characteristic.

### → ARCHAIC
| Tag | Count | Notes |
|-----|-------|-------|
| obsolete | 39,651 | Strong archaic signal |
| archaic | 24,452 | Direct mapping |
| historical | 16,361 | Archaic in practice |
| dated | 11,172 | → DATED |
| uncommon | 3,328 | Weak signal — use with freq_rank |

### → CASUAL / SLANG / REGISTER
| Tag | Count | Notes |
|-----|-------|-------|
| slang | 27,371 | → SLANG |
| informal | 18,397 | → CASUAL |
| colloquial | 6,940 | → CASUAL |
| humorous | 4,413 | → CASUAL (light) |
| euphemistic | 1,929 | Register note — not a separate bit |
| childish | 638 | → CASUAL |
| Internet | 6,094 | → SLANG + NEOLOGISM |
| neologism | 1,262 | → NEOLOGISM |
| clipping | 1,769 | → ABBREVIATION |
| contraction | 1,215 | → ABBREVIATION |
| acronym | 2,268 | → ABBREVIATION |
| initialism | 25,469 | → ABBREVIATION |
| abbreviation | 42,619 | → ABBREVIATION |
| pronunciation-spelling | 2,574 | → EYE_DIALECT |
| nonstandard | 3,676 | → CASUAL or DIALECT depending on context |

### → VULGAR / DEROGATORY
| Tag | Count | Notes |
|-----|-------|-------|
| derogatory | 8,561 | → DEROGATORY |
| offensive | 1,887 | → DEROGATORY |
| slur | 834 | → DEROGATORY (strongest signal — racial/ethnic slurs) |
| ethnic | 767 | Often co-occurs with slur/offensive → DEROGATORY |
| vulgar | 4,644 | → VULGAR |
| mildly | 332 | Qualifier, not standalone — ignore as characteristic |

### → GEOGRAPHIC
| Tag | Count | Notes |
|-----|-------|-------|
| US | 17,136 | → AMERICAN |
| UK | 12,332 | → BRITISH |
| British | 4,167 | → BRITISH |
| Australia | 4,259 | → AUSTRALIAN |
| Canada | 3,587 | (add CANADIAN bit when needed) |
| Scotland | 3,483 | → BRITISH + DIALECT |
| Ireland | 2,743 | → BRITISH + DIALECT |
| dialectal | 5,060 | → DIALECT |
| Northern-England | 1,123 | → BRITISH + DIALECT |
| Southern-US | 367 | → AMERICAN + DIALECT |
| India | 2,196 | (future — Indian English) |
| New-Zealand | 1,763 | (future) |
| South-Africa | 1,024 | (future) |
| Commonwealth | 944 | → BRITISH (broad) |

### → LITERARY / FORMAL / TECHNICAL
| Tag | Count | Notes |
|-----|-------|-------|
| literary | 967 | → LITERARY |
| poetic | 1,542 | → LITERARY |
| formal | 799 | → FORMAL |
| idiomatic | 12,187 | Phrase-level — skip for single tokens |
| figuratively | 8,295 | Sense note, not characteristic |

### → DERIVATION (forms[] tags)
| Tag | Count | Notes |
|-----|-------|-------|
| plural | 401,134 | Standard rule — skip unless irregular |
| past | 125,224 | Standard rule — skip unless irregular |
| present | 121,515 | Standard rule — skip |
| participle | 120,344 | Standard rule — skip |
| superlative | 81,815 | Standard rule — skip |
| comparative | 81,760 | Standard rule — skip |
| third-person | 59,693 | Standard rule — skip |
| alternative | 141,019 | → SPELLING_VARIANT (check if geographic tag present) |
| archaic | 3,984 | In forms[] → ARCHAIC variant |
| obsolete | 5,931 | In forms[] → ARCHAIC variant |
| US | 2,513 | In forms[] → SPELLING_VARIANT + AMERICAN |
| UK | 2,446 | In forms[] → SPELLING_VARIANT + BRITISH |
| dialectal | 649 | In forms[] → DIALECT variant |
| pronunciation-spelling | 364 | In forms[] → EYE_DIALECT |

### → FORM-OF / ALT-OF (structural sense tags)
| Tag | Count | Notes |
|-----|-------|-------|
| form-of | 525,941 | This sense = an inflected form of something else |
| alt-of | 163,377 | This sense = alternative form of something else |
| misspelling | 6,697 | Known misspelling — may want to record, may want to skip |

**Critical note on form-of**: 525,941 senses (30% of all senses) are flagged `form-of` — meaning that entry is an inflected form, not a root. These are candidates for the delta filter: if the form follows a standard rule, discard it. If it doesn't (irregular), keep it as a stored variant.

---

## Delta Forms — What to Keep vs Discard

Patrick's rule: **only record deltas, not standard rule forms.**

### Standard rules (DO NOT record as variants):
- Regular plural: +s, +es, +ies→y
- Regular past: +ed, doubled consonant +ed
- Regular progressive: +ing, doubled consonant +ing
- Regular 3rd person: +s, +es
- Regular comparative: +er
- Regular superlative: +est
- Regular adverb: +ly

### Record as variants (IRREGULAR deltas):
- Irregular past: `ran`, `went`, `was`, `swam`, `broke`, `took`...
- Irregular plural: `mice`, `feet`, `teeth`, `criteria`, `data`, `sheep`, `fish`...
- Suppletive forms: `go`→`went`, `be`→`was/were/is/are`...
- Archaic forms in forms[]: `walketh`, `hath`, `goeth`...
- Dialectal forms in forms[]: `runnin'`, `ain't`...
- Spelling variants (US/UK): `colour`/`color`, `honour`/`honor`...

### Kaikki filter logic for forms[]:
```
for form in entry.forms:
    if has_standard_rule_tags_only(form.tags):
        skip  # derivable, don't store
    elif has_any_delta_tag(form.tags):
        store as variant  # irregular, archaic, dialectal, spelling variant
    elif 'alternative' in form.tags:
        check_if_geographic_variant → store as SPELLING_VARIANT
```

Where `delta_tags` = {archaic, obsolete, dialectal, US, UK, Australia, rare, nonstandard, pronunciation-spelling, historical, dated}
And `standard_rule_tags` = {plural, past, present, participle, superlative, comparative, singular, third-person, gerund, indicative, second-person, first-person, subjunctive, imperative, infinitive}

---

## Etymology Language Patterns

35.9% of entries have etymology text. Top source languages for loanword detection:

| Language | Mentions | ISO 639-1 |
|----------|----------|-----------|
| Latin | 42,061 | `la` |
| French | 29,291 | `fr` |
| German | 21,579 | `de` |
| Greek | 17,370 | `el` / `grc` (ancient) |
| Old English | 16,146 | `ang` |
| Old French | 10,059 | `fro` |
| Dutch | 9,407 | `nl` |
| Italian | 6,861 | `it` |
| Spanish | 6,762 | `es` |
| Norse/Old Norse | 3,551 | `non` |
| Japanese | 3,477 | `ja` |
| Russian | 3,428 | `ru` |
| Arabic | 3,320 | `ar` |
| Portuguese | 1,782 | `pt` |
| Persian | 1,763 | `fa` |

**Note on Old English and Old French**: These are not loanwords in the modern sense — they are ancestral forms. "Old English" etymology means the word is native English, not borrowed. "Old French" typically means borrowed into Middle English from Norman French. Treatment:
- Old English etymology → native root, no BORROWING tag
- Old French / French etymology → BORROWING + `fr` (the word entered English from French)
- Latin etymology → context-dependent: scholarly coinages (BORROWING + `la`), naturalized Latin roots (may not need BORROWING)

**Practical approach**: Flag BORROWING when the word still feels foreign or retains non-English spelling patterns. Etymology extraction is a heuristic — exact language code from etymology_text requires NLP parsing. A regex approach on common patterns ("from French", "from Latin", "borrowed from", "from Japanese") is sufficient for the first pass.

---

## Sense Structure

Each sense in `senses[]` contains:
- `glosses`: list of strings (99.9% coverage). First element is the primary gloss.
- `tags`: list of tags (characteristic signals)
- `raw_glosses`: sometimes present (more detailed)
- `form_of`: if this is a form-of entry, points to the base word
- `alt_of`: if this is an alt-of entry, points to the canonical form
- `topics`: subject domain tags (medicine, computing, etc.)
- `categories`: Wiktionary categories

**Gloss coverage is excellent** (99.9%). The first element of `glosses[]` is suitable as the HCP gloss text for (root, PoS) pairs.

For words with multiple senses under the same PoS, the **first sense** is typically the most common/primary meaning. This is Wiktionary convention. Use first sense gloss as primary gloss.

---

## Multi-PoS Words

84,051 words (6.4%) appear with multiple PoS entries. Examples:
- `run`: noun + verb
- `fast`: adj + adv + verb + noun
- `light`: noun + adj + verb + adv

Each (word, PoS) combination is a separate entry in Kaikki with its own senses and forms. Each needs its own PoS record and gloss in HCP.

---

## Scale Estimates for HCP Population

Filtering Kaikki → HCP roots + variants:

| Category | Estimated count | Notes |
|----------|-----------------|-------|
| Root tokens (non-name, single PoS) | ~700K | Noun + verb + adj + adv, deduplicated |
| Root tokens (multi-PoS, each PoS record) | ~1M+ total PoS records | 84K words × avg 2 PoS |
| Label tokens (N_PROPER) | ~193K | From `name` PoS entries |
| Irregular delta variants to store | ~50-100K | Estimated — forms with delta tags |
| Spelling variants (US/UK) | ~20-30K | `alternative` forms with geographic tags |
| Archaic variants | ~40K | `obsolete`+`archaic` sense+form tags |
| DEROGATORY-tagged entries | ~12K | derogatory + offensive + slur tags combined |
| VULGAR-tagged entries | ~5K | vulgar tag |

**Vs current hcp_english**: 1,395,502 tokens, 809K in LMDB. The restructured DB should be leaner (roots only) but richer (structured variants, PoS, glosses).

---

## Processing Plan (for DB population script)

Based on this analysis, the Kaikki→HCP processing should:

1. **Pass 1 — Roots**: For each entry where the sense is NOT `form-of` and NOT `alt-of`: create/update root token record + PoS record + gloss from first sense.

2. **Pass 2 — Delta variants**: For each entry's `forms[]`: apply delta filter. Keep only forms with delta tags. Create variant records with characteristic bitmask.

3. **Pass 3 — Alt-of variants**: For entries with `alt-of` sense tag: these are spelling/dialectal/archaic variants pointing to their canonical. Create variant records.

4. **Pass 4 — Loanwords**: For entries with BORROWING-signal etymology: set BORROWING flag + extract source_language from etymology_text.

5. **Pass 5 — Derogatory/Vulgar**: For entries with `derogatory`/`offensive`/`slur`/`vulgar` sense tags: set DEROGATORY or VULGAR characteristic bits. Populate `note` field with tag context.

6. **Labels (N_PROPER) last**: Process `name` PoS entries as standalone Label root tokens. No relationship to common-word entries with same spelling.

---

## Open Items for Discussion

1. **Misspellings** (6,697 entries): Kaikki tags known misspellings. Should these be stored as variants with a MISSPELLING characteristic bit, or discarded? They appear in real text and should probably resolve to their correct form — but we don't want to reinforce them. Recommend: store as variant with NO characteristic bit set but with a `note: "misspelling of X"`. Resolves correctly, note is available for quality filtering.

2. **Phrases and multi-token entries** (phrase, prep_phrase, proverb — ~8K entries): Skip for now. Multi-token decomposition is future work.

3. **`idiomatic` tag** (12,187): Idiomatic senses of words (e.g., "kick the bucket" sense of "kick"). Single-token entries with idiomatic senses are valid roots — the idiom is a higher-level structure. Keep root, mark idiomatic sense in gloss note.

4. **Etymology parsing precision**: Regex on etymology_text is a heuristic. Kaikki also has `etymology_templates[]` which may provide structured language codes. Worth checking for better source_language extraction — but regex is sufficient for first pass.
