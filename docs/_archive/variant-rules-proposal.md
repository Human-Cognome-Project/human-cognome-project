# Variant Rules Proposal
*Linguist specialist — 2026-03-04*

This document covers Part 1 (variant transformation rules), Part 2 (ambiguous form and circular pair resolutions), and Part 3 (coverage analysis for Victorian-era texts).

---

## Part 1: Variant Transformation Rules

### Key Design Decision: Separate Variant Pass

Variant normalization is NOT the same as inflection stripping, and must not be merged into `TryInflectionStrip`. The reason:

- `TryInflectionStrip` turns an inflected surface form into a base form (e.g. "running"→"run"). The base is then matched against the existing vocab bed. First match wins.
- Variant normalization turns a non-standard surface form into a standard form (e.g. "runnin'"→"running"). That normalized form may itself need inflection stripping.
- These are different operations that compose in a specific order.

**Proposed pipeline order** (stages only run on what the previous stage didn't resolve):

```
1. Phase 2 PBD (vocab beds) — resolves ~94–98% of text
2. TryInflectionStrip → re-inject synthetic base into PBD
   - Silent-e fallback for failed -ed strips (already implemented)
3. TryVariantNormalize → re-inject normalized form into PBD
   - Normalized form MAY itself be inflected → TryInflectionStrip again
4. Morpheme decomposition (contractions n't/'re/'ve/'ll/'s/'m/'d, hyphens)
   - Decomposed bases re-run TryVariantNormalize if still unresolved
5. var bucket — genuinely unresolvable
```

The critical interaction: "darlin's" → Stage 4 contraction strips `'s` → base "darlin'" → this unresolved base now goes to Stage 3 (TryVariantNormalize) → "darling" → PBD resolves.

This means Stage 3 must run on decomposed bases that come out of Stage 4 unresolved, not just on the original unresolved runs. The simplest implementation: after Stage 4, collect all still-unresolved bases, run TryVariantNormalize on them, re-inject, and do one more PBD pass.

---

### Rule Table

Each rule specifies: **pattern**, **category**, **existence guard**, **morph bits**, **edge cases**.

The "existence guard" column answers: must the normalized base already be in the vocab bed to apply the rule? For variant normalization (unlike inflection stripping), we generally do want a guard — otherwise we create noise from over-eager pattern matching. However, since PBD IS the existence check (it resolves to nothing if the base isn't there), the guard is implicit: if the normalized form doesn't resolve, it falls through to var. No explicit pre-check needed.

---

#### Rule V-1: g-dropping (`-in'` → `-ing`)

**Category**: dialect
**Pattern**: word ends in `[consonant]in'` (apostrophe at end, representing dropped "g")
**Transformation**: remove trailing `'`, append `g` → `[stem]ing`
**Then**: the resulting `-ing` form goes through `TryInflectionStrip` (PROG strip) to get the base

**Examples**:
| Input | Normalized | After Inflection Strip | Final Token |
|-------|-----------|----------------------|-------------|
| darlin' | darling | — (darling is a standalone word) | darling |
| runnin' | running | run (PROG) | run + PROG |
| somethin' | something | — (in vocab) | something |
| nothin' | nothing | — (in vocab) | nothing |
| everythin' | everything | — (in vocab) | everything |
| goin' | going | go (PROG) | go + PROG |
| doin' | doing | do (PROG) | do + PROG |
| sayin' | saying | say (PROG) | say + PROG |
| comin' | coming | come (PROG) | come + PROG |
| lookin' | looking | look (PROG) | look + PROG |
| thinkin' | thinking | think (PROG) | think + PROG |
| walkin' | walking | walk (PROG) | walk + PROG |
| talkin' | talking | talk (PROG) | talk + PROG |

**Guard**: The apostrophe at the end is the primary guard — this pattern is highly specific. Very few standard English words end in `[consonant]in'`. False positive risk is low.

**Morph bits**: VARIANT_DIALECT (bit 14) + PROG (bit 4, from the subsequent -ing strip)

**Edge cases**:
- `somethin'`, `nothin'`, `everythin'`: the `-in'` maps not to `[stem]ing` by simple substitution — `somethin'` → `something` (add `g`, works). `nothin'` → `nothing` (add `g`, works). ✓
- `smilin'` → `smiling` → `smile` (PROG). ✓
- `lovin'` → `loving` → `love` (PROG). ✓ (silent-e case, already handled by existing `-ing` rule)
- `an'` → `and` (NOT this rule — this is a separate apostrophe truncation of "and")

---

#### Rule V-2: g-dropping without apostrophe (`-in` terminal → `-ing`)

**Category**: dialect
**Pattern**: word ends in `-in` (no apostrophe), treated as dropped `g` in dialect speech
**Transformation**: append `g` → `[stem]ing`

**HIGH RISK OF FALSE POSITIVES**. Most English words ending in `-in` are NOT dialect forms:
`cabin`, `basin`, `raisin`, `cousin`, `curtain`, `captain`, `margin`, `origin`, `martin`, `satin`, `latin`, `begin`, `within`, `domain`, `certain`, `mountain`, `fountain`, `villain`, etc.

**Recommendation**: Do NOT implement as a rule. Leave as individual DB entries for the handful of common dialect forms (`mornin`, `evenin`, `darlin`). The false-positive rate makes this rule harmful.

---

#### Rule V-3: Archaic 3rd-person singular present (`-eth` → base)

**Category**: archaic
**Pattern**: word ends in `-eth`
**Transformation**: strip `-eth`, try base, try base+e
**Morph bits**: THIRD (bit 5) + VARIANT_ARCHAIC (bit 13)

**Examples**:
| Input | Base candidate | Final token |
|-------|---------------|-------------|
| walketh | walk | walk + THIRD + VARIANT_ARCHAIC |
| loveth | lov → love (silent-e) | love + THIRD + VARIANT_ARCHAIC |
| cometh | com → come (silent-e) | come + THIRD + VARIANT_ARCHAIC |
| speaketh | speak | speak + THIRD + VARIANT_ARCHAIC |
| taketh | tak → take (silent-e) | take + THIRD + VARIANT_ARCHAIC |
| maketh | mak → make (silent-e) | make + THIRD + VARIANT_ARCHAIC |
| goeth | go (try "go" + "e"→"goe" fails, try "go" succeeds) | go + THIRD + VARIANT_ARCHAIC |
| rideth | rid → ride (silent-e) | ride + THIRD + VARIANT_ARCHAIC |
| liveth | liv → live (silent-e) | live + THIRD + VARIANT_ARCHAIC |
| knoweth | know | know + THIRD + VARIANT_ARCHAIC |
| giveth | giv → give (silent-e) | give + THIRD + VARIANT_ARCHAIC |

**Irregular forms — handle via individual DB entries, NOT this rule**:
| Form | Correct mapping | Notes |
|------|----------------|-------|
| hath | have | "ha" + "e" → "hae" fails; needs individual entry |
| doth | do | "do" + silent-e → "doe" fails; needs care |
| saith | say | "sai" → "saie"? No — irregular |
| wilt | will | entirely irregular |

**Guard**: minimum length 5 (avoids "Seth", "Beth" as names). The rule should only apply to words where the `-eth` form is not itself a standard vocabulary item. Since the existing vocab bed IS the guard, if `walketh` isn't in the bed, PBD fails → strip rule applies → `walk` goes to PBD. If somehow `eth` were a word (it's not standard English), the strip wouldn't fire because `walketh` would have resolved in PBD directly.

**Note on strip order conflict**: The existing `TryInflectionStrip` has no `-eth` rule, so no conflict. This rule belongs in the new `TryVariantNormalize` pass.

---

#### Rule V-4: Archaic 2nd-person singular present (`-est` archaic verb form)

**Category**: archaic
**Pattern**: word ends in `-est` used as archaic 2nd-person (not superlative)

**CONFLICT**: The existing `TryInflectionStrip` already handles `-est` (superlative adjective: `tallest`→`tall`). The archaic verb form `walkest`→`walk` would be correctly stripped by the existing rule mechanically, but with wrong morph bits (no bits set for superlative vs THIRD for 2nd person archaic).

**Recommendation**: The mechanical strip result is correct (base word identified). The morph bit distinction between "superlative adjective" and "archaic 2nd person" matters only for downstream force analysis, not for token resolution. For now, accept the existing `-est` strip; add VARIANT_ARCHAIC bit for verb forms when context makes it clear. Do not add a separate rule that conflicts with the existing one.

For irregular archaic 2nd-person forms, use individual DB entries:
| Form | Maps to | Notes |
|------|---------|-------|
| art | be | 2nd person singular present of "be" |
| hast | have | 2nd person singular of "have" |
| dost | do | 2nd person singular of "do" |
| wast | be | 2nd person singular past of "be" |
| wert | be | 2nd person singular past of "be" |
| canst | can | 2nd person singular of "can" |
| wilt | will | 2nd person singular of "will" (also archaic past) |

---

#### Rule V-5: Archaic modal `-st` ending (`couldst`, `wouldst`, `shouldst`)

**Category**: archaic
**Pattern**: word ends in `dst` (specifically `ouldst`, `houldst`)
**Transformation**: strip final `st` → canonical modal

**Examples**:
| Input | Strip | Result |
|-------|-------|--------|
| couldst | strip `st` | could |
| wouldst | strip `st` | would |
| shouldst | strip `st` | should |

**Guard**: This is extremely narrow. The pattern `[modal]st` only applies to: couldst, wouldst, shouldst. No other common English words end in `ouldst` or `houldst`. These three could equally be handled as individual DB entries — that may be simpler than a rule.

**Recommendation**: Individual DB entries. The rule is not general enough to justify code complexity.

---

#### Rule V-6: Archaic `-t` past tense variants

**Category**: archaic (British: dialect for some)
**Pattern**: `-t` instead of `-ed` past tense

**HIGH RISK**: Most English words ending in `-t` are NOT past tense variants (most, best, last, past, fast, just, rest, test, mast, cast, etc.). A general `-t` → base rule would cause massive false positives.

**Recommendation**: These forms are already in the Wiktionary data as individual entries. Let the DB handle them. Do NOT implement as a rule.

Key forms (likely already in DB via Wiktionary):
`dreamt`, `learnt`, `spelt`, `burnt`, `smelt`, `knelt`, `leapt`, `crept`, `wept`, `slept`, `kept`, `swept`, `dealt`, `felt`, `built`, `dwelt`, `meant`, `lent`, `bent`, `sent`, `spent`, `went` (→ go)

Note: `went` → `go` is highly irregular and already well-represented in Wiktionary.

---

#### Rule V-7: Archaic pronoun forms

**Category**: archaic
These are standalone word substitutions with no suffix pattern — cannot be rules.

| Archaic form | Modern canonical | Notes |
|-------------|-----------------|-------|
| thou | you | subject singular |
| thee | you | object singular |
| thy | your | possessive |
| thine | your | possessive (before vowel) / "yours" (predicate) |
| ye | you | subject plural, or formal singular |
| thyself | yourself | reflexive |
| whereof | of which | relative pronoun |
| whereto | to which | |
| wherein | in which | |
| whereby | by which | |
| wherefore | why / for what reason | |
| hereof | of this | |
| therein | in that/there | |
| thereof | of that | |
| thereto | to that | |
| hereby | by this | |
| hereafter | after this | |

All should be individual DB entries (already likely in Wiktionary data for the main forms). The `where-/here-/there-` compounds may need manual addition.

---

#### Rule V-8: Casual leading-apostrophe truncation

**Category**: casual/dialect
**Pattern**: leading apostrophe representing elided syllable

These CANNOT be suffix rules (they're prefix elisions). They need individual entries or a separate lookup table.

| Form | Canonical | Notes |
|------|-----------|-------|
| 'em | them | unstressed "them" |
| 'bout | about | casual "about" |
| 'cause | because | casual "because" |
| 'til | until | common truncation |
| 'tis | it is | archaic/poetic contraction |
| 'twas | it was | archaic/poetic |
| 'twere | it were | archaic/poetic |
| 'twill | it will | archaic/poetic |
| 'gainst | against | poetic |
| 'mongst | amongst | poetic |
| 'neath | beneath | poetic |
| 'midst | amidst | poetic |

Note: `'tis`, `'twas`, `'twere`, `'twill` are two-token decompositions (`it + is/was/were/will`). These need the multi-token decomposition design (deferred per the audit doc). For now: individual entries mapping to the most meaningful single token (`'tis`→`be`, `'twas`→`be`), or leave as var/lingo until decomposition is built.

---

### Morph Bit Assignments for Variant Flags

Bits 12-15 are reserved. Recommended assignments:

| Bit | Name | Value | Meaning |
|-----|------|-------|---------|
| 12 | VARIANT | 1 << 12 | Generic variant flag (set on ALL variant resolutions) |
| 13 | VARIANT_ARCHAIC | 1 << 13 | Archaic/obsolete/dated form |
| 14 | VARIANT_DIALECT | 1 << 14 | Dialectal form |
| 15 | VARIANT_CASUAL | 1 << 15 | Casual/informal/slang |

VARIANT is always set when any variant resolution fires. The specific subtype bit is set additionally. Literary forms (poetic): use VARIANT_ARCHAIC (closest semantic fit — poetic register is typically archaic in HCP texts).

---

### Strip Order — Complete Specification

```
STAGE 0: Phase 2 PBD (vocab beds — primary resolution)
  └─ Resolves ~94-98% of running text

STAGE 1: TryInflectionStrip (existing, runs on unresolved runs)
  Priority order (first match wins):
    1. -ies → -y           (PLURAL|THIRD)
    2. -ves → -f / -fe     (PLURAL)
    3. -ied → -y           (PAST)
    4. doubled-cons + -ing  (PROG)
    5. doubled-cons + -ed   (PAST)
    6. -ing (base, base+e)  (PROG)
    7. -ed  (base, base+e)  (PAST)
    8. -er  (comparative)   (no bits)
    9. -est (superlative)   (no bits)  ← also catches some archaic 2nd-person verb forms
   10. -es                  (PLURAL|THIRD)
   11. -s                   (PLURAL|THIRD)
   12. -ily → -y            (no bits)
   13. -ly                  (no bits)
   14. -ness                (no bits)
  └─ Synthetic bases injected into PBD queues; silent-e fallback for failed -ed

STAGE 2: TryVariantNormalize (NEW — runs on runs still unresolved after Stage 1)
  Priority order:
    1. -in'  (g-drop dialect): stem + "g" → normalized form
       → normalized form re-enters TryInflectionStrip
    2. -eth  (archaic 3rd person): strip -eth, try base, try base+e → THIRD|VARIANT_ARCHAIC
    [Rules V-2, V-5, V-6 NOT implemented as rules — use DB entries]
  └─ Normalized forms injected into PBD queues

STAGE 3: Morpheme decomposition (existing — contractions + hyphens)
  Contraction priority (longest suffix first):
    n't → NEG
    're  → BE
    've  → HAVE
    'll  → WILL
    's   → POSS
    'm   → AM
    'd   → COND
  └─ Decomposed bases → PBD; if still unresolved, → Stage 4

STAGE 4: TryVariantNormalize on decomposed bases (NEW — second variant pass)
  Same rule set as Stage 2. Handles cases like:
    darlin's → Stage 3 strips 's → base "darlin'" → Stage 4 applies V-1 → "darling"
  └─ Normalized bases → PBD

STAGE 5: var bucket — genuinely unresolvable
```

**Key invariant**: TryVariantNormalize runs AFTER TryInflectionStrip (not before), because:
- Inflection stripping is high-confidence and high-coverage.
- Variant normalization is lower-confidence and should only fire when the inflection chain fails.
- Applying variant rules first risks masking actual inflections.

**Composition example — "runnin's"** (hypothetical):
1. Stage 0: "runnin's" not in PBD → unresolved
2. Stage 1: TryInflectionStrip fails (apostrophe, no standard suffix)
3. Stage 3: contraction decomp finds `'s` → base "runnin'" unresolved
4. Stage 4: TryVariantNormalize("runnin'") → V-1 fires → "running"
5. "running" → PBD (not in vocab, it's a stripped form)
6. TryInflectionStrip("running") → doubled cons+ing → "run" (PROG)
7. "run" → PBD → resolves
8. Final: token "run" + PROG + POSS + VARIANT_DIALECT

---

## Part 2: Ambiguous Form Resolutions

### Notable ambiguous cases (from audit)

| Form | Candidates from Wiktionary | Recommended mapping | Rationale |
|------|---------------------------|---------------------|-----------|
| couldst | {can, could} | **could** | Archaic 2nd-person singular past tense modal; `canst` is the archaic form of "can", `couldst` is the archaic 2nd-person of "could" |
| et | {ate, eat} | **eat** | Dialectal past tense of "eat" (analogous to "sat/sit", "got/get"); maps to base form with PAST bit |
| abolisht | {abolish, abolished} | **abolish** | Archaic `-t` past tense variant of the verb; base form is the canonical |
| curst | {curse, cursed} | **curse** | Archaic `-t` past tense; base form canonical. (Note: also appears as archaic adjective "curst"="cursed" — same token, context resolves) |

### General strategy for ambiguous archaic verb forms
- When a form is an archaic past tense variant (e.g., `abolisht`, `curst`, `stopt`, `dropt`): map to the **base verb form**, not the past tense. The PAST morph bit on the position carries the inflection information. This produces cleaner PBM bonds (base-to-base, not base-to-inflection).
- When a form is an archaic present tense form (e.g., `couldst`): map to the **canonical present/root form** that it inflects from.

### Remaining 67 ambiguous forms
Without running the full SQL query, the general resolution rules are:
1. Prefer the BASE form (uninflected) over the inflected form as canonical target
2. Prefer the more common/familiar modern word when both candidates are valid
3. If both candidates are equally valid with no clear preference, mark as `category='archaic'` with NULL `canonical_id` — leave it to context (future force analysis)

---

## Part 3: Circular Pair Resolutions

### Final recommendations

| Pair | Action | Rationale |
|------|--------|-----------|
| `prejudical` → `prejudicial` | **Implement** (clear `prejudical`, set canonical_id → prejudicial token_id, category='archaic') | `prejudical` is an obsolete misspelling. One direction only. `prejudicial` is the correct modern form. |
| `haught` → `haughty` | **Implement** | `haught` is an archaic adjective meaning "haughty". Maps cleanly to `haughty`. Category: archaic. |
| `haut` → `haughty` | **Implement** | French-influenced archaic variant, same mapping as `haught`. Category: archaic. |
| `rencontre` → `encounter` | **Implement** | French loanword used in archaic/literary English for "encounter". Category: archaic/literary. |
| `rencounter` → `encounter` | **Implement** | Anglicized form of `rencontre`, same meaning. Category: archaic. |
| `betel` ↔ `beetle` | **Leave as-is** | Completely different words. Betel = palm plant (Piper betle). Beetle = insect or vehicle. Wiktionary error — do not create a variant relationship. Both should be independent vocab entries with no canonical_id. |
| `travel` ↔ `travail` | **Leave as-is** | Different modern English words with shared Old French etymology (`travailler`), but they have diverged completely in meaning. `travel` = to journey; `travail` = hard labor/suffering. Not variants of each other. Leave both as independent vocab entries. |
| `shave` ↔ `shove` | **Leave as-is** | Different modern words. Both derive from Proto-Germanic but have entirely separate meanings and forms in modern English. Not variants. |
| `prophecy` ↔ `prophesy` | **Leave as-is** | These are NOT variants — they are complementary parts of speech. `prophecy` (noun), `prophesy` (verb). The spelling distinction encodes grammatical category. Both should remain as independent vocabulary entries with no canonical_id. |

### SQL to implement the fixes

```sql
-- prejudical → prejudicial
UPDATE tokens t
SET canonical_id = (SELECT token_id FROM tokens WHERE name = 'prejudicial' LIMIT 1),
    category = 'archaic'
WHERE t.name = 'prejudical';

-- haught, haut → haughty
UPDATE tokens t
SET canonical_id = (SELECT token_id FROM tokens WHERE name = 'haughty' LIMIT 1),
    category = 'archaic'
WHERE t.name IN ('haught', 'haut');

-- rencontre, rencounter → encounter
UPDATE tokens t
SET canonical_id = (SELECT token_id FROM tokens WHERE name = 'encounter' LIMIT 1),
    category = 'archaic'
WHERE t.name IN ('rencontre', 'rencounter');
```

---

## Part 4: Questionable Wiktionary Mappings

### `bang` → `bhang`

**Reject**. These are entirely different words:
- `bang`: strike, loud noise, haircut fringe, to bang (verb)
- `bhang`: a preparation of cannabis leaves used in South Asian cuisine and beverages

Wiktionary may contain an erroneous note about `bang` as a dialectal variant of `bhang` or vice versa, but this mapping does not hold in any register of English that would appear in HCP texts. The semantic gap is complete.

**Action**: Clear `canonical_id` and `category` on `bang` (if set). Leave as independent vocabulary entry.

### `yew` → `you`

**Reject**. `yew` (the coniferous tree, *Taxus*) and `you` (2nd person pronoun) are homophone accidents in modern English, not variant forms. There is no register of English where `yew` is used as a variant spelling of `you` in meaningful text (the eye-dialect spelling is extremely limited joke usage). Any Victorian-era text using `yew` means the tree.

**Action**: Clear canonical_id on `yew` if set. Leave as independent vocabulary entry.

### `ew` → `yew` (cascaded to `you`)

**Reject**. `ew` is an interjection of disgust (informal: "ew, gross"). It has no semantic or etymological relationship to `yew` the tree or `you` the pronoun. This is a Wiktionary data error.

**Action**: Clear canonical_id on `ew`. Leave as independent vocabulary entry (or let it remain as casual interjection in the regular vocab).

### SQL

```sql
-- Clear the bad mappings
UPDATE tokens SET canonical_id = NULL, category = NULL
WHERE name IN ('bang', 'yew', 'ew')
  AND canonical_id IS NOT NULL;
```

Note: verify `bang` actually has a bad mapping before running. The suspicious pairs query (short words) should surface this.

---

## Part 5: Coverage Analysis — Victorian-Era Texts

### What the 5,601 Wiktionary entries cover

The existing Wiktionary-derived DB entries handle:
- The bulk of archaic English vocabulary (5,180 archaic entries)
- Core archaic pronouns: `thou`, `thee`, `thy`, `thine`, `ye` — likely present
- Core archaic verb irregulars: `art`, `hast`, `hath`, `dost`, `doth`, `wilt` — likely present
- Common archaic past tense forms: `dreamt`, `learnt`, `spelt`, `burnt`, `crept`, `wept`, `slept`, `dealt`, `felt`, `dwelt`, `meant` — high confidence these are in Wiktionary
- `-eth` forms of common verbs (Wiktionary has many, but not all)
- `-est` archaic verb forms (same)

**Confidence: ~85-90% of archaic vocabulary already covered by Wiktionary data.**

### What the new rules add

**Rule V-1 (`-in'` g-drop)** — HIGH VALUE for Victorian dialect speech:
- Covers all `-ing` forms in dialect dialogue: "I'm runnin'", "she's darlin'", "we're comin'"
- Victorian texts (Sherlock Holmes, Dracula, Huck Finn planned) have significant dialect speech
- Estimated coverage: 50-200 instances per text in texts with working-class or regional dialect characters
- Without this rule, ALL dialect speech tokens go to var — with it, they resolve cleanly
- **This rule covers a class of forms NOT in Wiktionary** (Wiktionary doesn't tag informal g-drops as "archaic" forms)

**Rule V-3 (`-eth` 3rd person)** — MEDIUM VALUE:
- Covers regular `-eth` forms not individually listed in Wiktionary
- Victorian texts don't use much `-eth` (it's more Jacobean/Early Modern English)
- Useful for: quoted scripture, archaic registers within Victorian texts, historical fiction
- Most high-frequency `-eth` forms (hath, doth, saith) need individual entries anyway
- **Estimated new coverage**: 20-50 forms per text in heavily archaic registers

### What remains genuinely foreign/var/lingo

These are CORRECT to keep in var:
- Latin phrases common in Victorian academic/legal text: *ad hoc*, *ab initio*, *inter alia*, *prima facie*, *sine qua non*
- French phrases: *nom de plume*, *tête-à-tête*, *carte blanche*, *bête noire*
- Proper nouns: character names, place names, titles
- Technical/scientific coinages
- Text-specific neologisms
- Acronyms and initialisms
- Numbers, dates, citations

### Rough estimate: rules vs individual entries

Of the 5,601 Wiktionary entries currently in the DB:
- ~200-400 entries follow the `-eth` rule pattern (could be generated by rule, but since they're already individually in the DB, no change needed)
- The `-in'` rule is NET NEW coverage — Wiktionary has none of these (they're informal spellings, not archaic vocabulary)
- ~100-200 entries that are irregular (hath, doth, wilt, thou, thee, etc.) must remain as individual entries regardless

**The key value of rules is not replacing existing entries, but covering dialect forms that Wiktionary doesn't track.** The `-in'` rule alone is worth implementing as it covers an entire productive class of informal English spellings that appear throughout literary and colloquial text.

---

## Summary of Actionable Items

### For DB specialist (migration / SQL)

1. Fix circular pairs (SQL above): `prejudical`→`prejudicial`, `haught`/`haut`→`haughty`, `rencontre`/`rencounter`→`encounter`
2. Clear bad mappings: `bang`, `yew`, `ew`
3. Fix ambiguous forms: `couldst`→`could`, `et`→`eat`, `abolisht`→`abolish`, `curst`→`curse`
4. Add individual entries for leading-apostrophe forms: `'em`, `'bout`, `'cause`, `'til` (and `'tis`, `'twas` as notes for future multi-token decomp)
5. Add individual entries for archaic irregular forms not in Wiktionary: `art`→`be`, `hast`→`have`, `wast`→`be`, `wert`→`be`, `canst`→`can`, `wilt`→`will`, `couldst`→`could`, `wouldst`→`would`, `shouldst`→`should` (some may already be present via Wiktionary)

### For engine specialist (code)

1. Add morph bits 12-15: `VARIANT`, `VARIANT_ARCHAIC`, `VARIANT_DIALECT`, `VARIANT_CASUAL` to `MorphBit` namespace in `HCPResolutionChamber.h`
2. Implement `TryVariantNormalize(word)` function in `HCPVocabBed.cpp` with rules V-1 and V-3
3. Wire `TryVariantNormalize` into the resolve loop after `TryInflectionStrip` (Stage 2) and after contraction decomposition (Stage 4) per the pipeline order above
4. Wire envelope-based variant lookup (env_archaic / env_dialect / env_casual) for the 5,601 DB entries — set VARIANT + subtype bits on resolution from these envelopes

### Rules to implement as code

| Rule | Priority | Estimated LOC |
|------|----------|---------------|
| V-1: `-in'` g-drop | High | ~15 |
| V-3: `-eth` 3rd person | Medium | ~10 |

All other rules (V-2, V-4, V-5, V-6, V-7, V-8) are better served by individual DB entries. The pattern specificity is too low, the Wiktionary data already covers many of them, or decomposition design is not yet built.
