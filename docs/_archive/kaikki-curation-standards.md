# Kaikki → hcp_english Curation Standards

**Author:** Linguist Specialist — 2026-03-16
**Status:** Living document — update only with Patrick's approval

This document defines the standard for every entry curated from Kaikki into hcp_english. It MUST be followed consistently across all contexts and sessions. No ad-hoc exceptions.

---

## Core Principle

Every word reduces to a **root concept**. Everything else — PoS, morphemes, glosses, variants — describes the **shape** around that concept.

- **Root tokens** are concept anchors (particles in the physics engine)
- **Morphemes** are force indicators (NSM structural markers → physics forces)
- **Glosses** decompose into NSM primitives (superposition states)
- **Transformation rules** are stored, not derived forms
- **Irregular deltas** are stored only where rules don't hold

---

## Token ID Encoding

5-pair format: `ns.p2.p3.p4.p5` (14 bytes, base-50 alphabet per character)

| Pair | Role | Encoding |
|------|------|----------|
| ns | Namespace | `AB` = English shard |
| p2 | Word group | `AA` = common, `AB`/`AC` = overflow groups (existing convention) |
| p3 | **Physics slice key** | char 1 = first letter (A=a, B=b, ... Z=z); char 2 = word length (A=1, B=2, ... Z=26, a=27, ...) |
| p4.p5 | Sequential counter | 4-char counter within slice (50^4 = 6.25M slots per slice) |

**p3 IS the particle key.** The engine loads all tokens with a matching p3 to fill one PBD bed. No joins, no lookups — the token_id prefix is the slice address.

The `particle_key` generated column on `tokens` is redundant with p3 and will be removed.

---

## The Tree Model

Each root token has this structure:

```
root (concept anchor, token_id, surface form = lowercase root)
├── PoS branch 1 (e.g., verb)
│   ├── gloss 1 (superposition state, NSM prime refs TBD)
│   ├── gloss 2
│   ├── rule acceptance (which standard transformations apply)
│   ├── irregular deltas (where rules break)
│   └── cross-references (if surface form collides with derivation of another root)
├── PoS branch 2 (e.g., noun)
│   ├── gloss 1
│   ├── ...
```

### Decision tree for each Kaikki entry

1. **Is this a root concept?**
   - Not a form-of or alt-of → YES, create/update root token
   - Form-of with only standard rule tags → NO, skip (rule-derivable)
   - Form-of with delta tags (irregular/archaic/dialect) → store as variant under the root it derives from
   - Alt-of → store as variant (spelling/dialect/archaic alternate)

2. **Which PoS branches does it occupy?**
   - Each Kaikki (word, pos) pair = one PoS branch
   - A word can have multiple: run(verb), run(noun)
   - Mark the most common as `is_primary`

3. **Under each PoS, what glosses?**
   - Each distinct sense = one gloss (superposition state)
   - First Kaikki sense = primary gloss
   - Gloss text from `senses[].glosses[0]`
   - `nsm_prime_refs` left NULL for now — populated later

4. **Under each PoS, what rules apply?**
   - Set `morpheme_accept` bits for standard transformations this root accepts in this PoS role
   - Defaults by PoS (overridable per entry):
     - N_COMMON: PLURAL, POSSESSIVE
     - V_MAIN: PAST, PROGRESSIVE, 3RD_SING, GERUND
     - ADJ: COMPARATIVE, SUPERLATIVE, ADVERB_LY
     - N_PROPER: POSSESSIVE only

5. **Under each PoS, any irregular deltas?**
   - If Kaikki forms[] has entries that DON'T follow standard rules → store as variant
   - e.g., run → ran (irregular past), mouse → mice (irregular plural)

6. **Cross-reference needed?**
   - ONLY when a surface form is both an independent root AND a valid rule derivation of a different root
   - e.g., "mister" = root (title) AND mist + -er (device)
   - Root takes matching priority; cross-ref records the derivation path

---

## What IS a root?

A root is the minimal concept-bearing form. Guidelines:

- **Verbs**: infinitive form (build, run, compute)
- **Nouns**: singular form (house, cat, idea)
- **Adjectives**: base form (fast, beautiful, red)
- **Adverbs**: if derived from adjective by -ly rule, NOT a separate root. If standalone (very, here, now), IS a root.

### Derived forms that are NOT separate roots

These are rule-derivable from their root and are NOT stored as tokens:
- Regular inflections: walks, walked, walking, cats, bigger, biggest
- Regular derivations where the meaning is compositional: quickly (quick + -ly), builder (build + -er), unhappy (un- + happy), rebuild (re- + build)

### Derived forms that ARE separate roots

When the derived form has acquired its own independent meaning that is no longer compositional:
- "Building" as "a structure" — not just "the act of building." This is its own root concept... BUT it also exists as a derivation of "build." The independent noun meaning is a PoS branch with its own gloss. The -ing derivation path is a cross-reference.
- "Computer" as "an electronic device" — historically compute + -er, but the meaning has drifted far enough that it's its own concept.

**Decision heuristic**: If the primary modern meaning is NOT just "root + affix meaning," it's probably its own root. But if the derivation path is still valid/recognizable, record the cross-reference.

**When in doubt, ask Patrick.**

---

## Characteristics Bitmask — LVM Layered Model

32-bit integer. Bits grouped by dimension. **Stored using LVM (Logical Volume Manager) strategy**: each level of the tree stores ONLY the delta bits not already present at the level above.

### Layering (LoD inheritance)

- **Token level** (`tokens.characteristics`): bits that apply to ALL senses, ALL PoS branches (e.g., BORROWING, ABBREVIATION)
- **PoS level** (`token_pos.characteristics`): bits specific to this grammatical role, not inherited from token (e.g., word is FORMAL only as noun)
- **Gloss level** (`token_glosses.characteristics`): bits specific to THIS sense only (e.g., one sense of "ass" is VULGAR, others are not)

**To get the full characteristic profile for a specific sense**: OR the three levels: `token.chars | pos.chars | gloss.chars`. No duplication — each level only adds what's new.

### Register (bits 0-7)
| Bit | Constant | Meaning |
|-----|----------|---------|
| 0 | FORMAL | Elevated, official, ceremonial |
| 1 | CASUAL | Informal, everyday |
| 2 | SLANG | Colloquial, ephemeral |
| 3 | VULGAR | Profane, obscene |
| 4 | DEROGATORY | Slur, discriminatory |
| 5 | LITERARY | Poetic, elevated literary |
| 6 | TECHNICAL | Domain-specific, specialist |
| 7 | (reserved — NEUTRAL = absence of register bits) |

### Temporal (bits 8-11)
| Bit | Constant | Meaning |
|-----|----------|---------|
| 8 | ARCHAIC | Obsolete, no longer in active use |
| 9 | DATED | Falling out of use |
| 10 | NEOLOGISM | Recently coined |
| 11 | (reserved) |

### Geographic (bits 12-15)
| Bit | Constant | Meaning |
|-----|----------|---------|
| 12 | DIALECT | Regional or social dialect |
| 13 | BRITISH | British English |
| 14 | AMERICAN | American English |
| 15 | AUSTRALIAN | Australian English |
| 16-19 | (reserved for future geographic) |

### Derivation (bits 20-27)
| Bit | Constant | Meaning |
|-----|----------|---------|
| 20 | STANDARD_RULE | Derivable by standard morphological rule |
| 21 | IRREGULAR | Must be stored explicitly |
| 22 | SPELLING_VARIANT | Alternate orthography |
| 23 | EYE_DIALECT | Phonetic spelling of spoken reduction |
| 24 | BORROWING | Loanword (see source_language) |
| 25 | COMPOUND | Compound word or portmanteau |
| 26 | ABBREVIATION | Abbreviated or contracted form |
| 27 | (reserved) |

---

## PoS Tags

```
N_COMMON, N_PROPER, N_PRONOUN,
V_MAIN, V_AUX, V_COPULA,
ADJ, ADV, PREP,
CONJ_COORD, CONJ_SUB,
DET, INTJ, PART, NUM
```

### Kaikki PoS → HCP mapping
| Kaikki | HCP |
|--------|-----|
| noun | N_COMMON |
| name | N_PROPER |
| pron | N_PRONOUN |
| verb | V_MAIN (post-process aux list for V_AUX/V_COPULA) |
| adj | ADJ |
| adv | ADV |
| prep | PREP |
| conj | CONJ_COORD or CONJ_SUB (check gloss) |
| det | DET |
| intj | INTJ |
| particle | PART |
| num | NUM |
| phrase, prep_phrase, proverb | SKIP (multi-token) |
| prefix, suffix | SKIP (morphemes, not tokens) |
| contraction | SKIP entry; process as variant |
| symbol, character | SKIP |

---

## Morpheme Acceptance Bits

Per-PoS bitmask on `token_pos.morpheme_accept`. Encodes which standard transformation rules a root accepts in this PoS role.

| Bit | Constant | Rule |
|-----|----------|------|
| 0 | MORPH_PLURAL | +s, +es, +ies |
| 1 | MORPH_PAST | +ed, doubled consonant+ed |
| 2 | MORPH_PROGRESSIVE | +ing |
| 3 | MORPH_3RD_SING | +s, +es |
| 4 | MORPH_COMPARATIVE | +er |
| 5 | MORPH_SUPERLATIVE | +est |
| 6 | MORPH_ADVERB_LY | +ly |
| 7 | MORPH_POSSESSIVE | +'s |
| 8 | MORPH_GERUND | verb → noun via -ing |

---

## Standard Transformation Rules

Source of truth: `hcp_english.inflection_rules` table (39 rules). These produce derived forms WITHOUT storing the form. The engine strips outside-in during ingestion, applies inside-out during reconstruction.

### Suffix rules (24 rules, 8 morphemes)
| Morpheme | Variants | Key examples |
|----------|----------|-------------|
| PAST | silent-e (-e+ed), CVC doubling, default (+ed) | like→liked, tap→tapped, walk→walked |
| PROGRESSIVE | silent-e (-e+ing), CVC doubling, default (+ing) | make→making, run→running, walk→walking |
| PLURAL | sibilant (+es), consonant+y (-y+ies), default (+s) | kiss→kisses, city→cities, cat→cats |
| 3RD_SING | sibilant/o (+es), consonant+y (-y+ies), default (+s) | kiss→kisses, fly→flies, walk→walks |
| COMPARATIVE | silent-e, CVC, consonant+y (-y+ier), default (+er) | nice→nicer, big→bigger, happy→happier |
| SUPERLATIVE | silent-e, CVC, consonant+y (-y+iest), default (+est) | nice→nicest, big→biggest, happy→happiest |
| ADVERB_LY | consonant+y (-y+ily), le-drop (-le+ly), default (+ly) | happy→happily, simple→simply, quick→quickly |
| POSSESSIVE | default (+'s) | cat→cat's |

### Prefix rules (15 rules)
| Morpheme | Prefix | Meaning | Example |
|----------|--------|---------|---------|
| PFX_NEG | un- | negation/reversal | unhappy→happy, undo→do |
| PFX_ITER | re- | repetition/again | redo→do, rewrite→write |
| PFX_PRE | pre- | before | prepay→pay, preview→view |
| PFX_MIS | mis- | wrongly | misuse→use, mislead→lead |
| PFX_NEG_DIS | dis- | negation | disagree→agree, dislike→like |
| PFX_REV | de- | reversal | defrost→frost, decode→code |
| PFX_NEG_NON | non- | negation | nonsense→sense, nonstop→stop |
| PFX_NEG_IN | in- | negation | incorrect→correct |
| PFX_NEG_IM | im- | negation | impossible→possible |
| PFX_NEG_IL | il- | negation | illegal→legal |
| PFX_NEG_IR | ir- | negation | irregular→regular |
| PFX_ANTI | anti- | against | antiwar→war |
| PFX_OVER | over- | excess | overcook→cook |
| PFX_OUT | out- | surpass | outrun→run |
| PFX_UNDER | under- | deficit | underpay→pay |

### Not yet rules (assess per-entry during curation)
Derivational suffixes that may become rules as patterns emerge:
- **-er** (agent noun): builder, computer — productive but cross-reference complications
- **-ness** (state noun): happiness, darkness — productive from adjectives
- **-ment** (result noun): agreement, movement — semi-productive
- **-able/-ible** (capacity adj): readable, visible — semi-productive
- **-tion/-sion** (process noun): creation, decision — mostly lexicalized borrowings

---

## Cross-Reference Rules

A cross-reference is needed ONLY when:
1. A surface form exists as its own independent root concept, AND
2. That same surface form is also a valid rule derivation of a DIFFERENT root

The independent root takes matching priority. The cross-reference records the alternate derivation path for superposition.

**Do NOT create cross-references for:**
- Words with multiple PoS (that's just multiple branches on one root)
- Words with multiple glosses (that's superposition under one PoS branch)
- Regular derivations that don't have independent root status

---

## Kaikki Tag → Characteristics Mapping

See `docs/kaikki-tag-mapping.md` for the complete mapping table. Summary:

- `formal` → FORMAL
- `informal/colloquial/humorous/childish` → CASUAL
- `slang` → SLANG
- `vulgar` → VULGAR
- `derogatory/offensive/slur` → DEROGATORY
- `literary/poetic` → LITERARY
- `obsolete/archaic/historical` → ARCHAIC
- `dated` → DATED
- `neologism` → NEOLOGISM
- `US` → AMERICAN
- `UK/British` → BRITISH
- `Australia` → AUSTRALIAN
- `dialectal` → DIALECT
- `pronunciation-spelling` → EYE_DIALECT | CASUAL

---

## Processing Workflow

### Per-entry checklist
1. Read Kaikki entry (word, pos, senses, forms, etymology)
2. Is this a root? (not form-of, not alt-of with only standard tags)
3. Does this root already exist in hcp_english? (may have been created from another PoS)
4. Create/update root token
5. Add PoS branch
6. Add glosses under that PoS (one per distinct sense)
7. Set morpheme_accept bits
8. Check forms[] for irregular deltas → store as variants
9. Check etymology → set BORROWING + source_language if applicable
10. Set characteristics bits from sense tags
11. Check for cross-reference collisions
12. Mark entry as processed in staging DB

### Batch-safe categories (can process in groups with review)
- Form-of entries with ONLY standard rule tags → skip (mark processed)
- Entries where PoS is phrase/prep_phrase/proverb/prefix/suffix/symbol/character → skip

### Entry-by-entry required
- Everything else. Especially:
  - Any word with multiple PoS entries
  - Any word with irregular forms
  - Any word where root identification is ambiguous
  - Any word with cross-reference potential

---

## Quality Gates

- After each chunk: verify sample entries can round-trip (root + rules → correct surface form)
- No chunk larger than ~200 entries committed without review
- Track processed/remaining count in staging DB
- When in doubt → ask Patrick

---

## What We Skip (for now)

- Multi-token phrases (phrase, prep_phrase, proverb)
- Sub-word morphemes as standalone entries (prefix, suffix)
- Symbols and characters
- Standard-rule inflected forms (the whole point of this design)
- NSM prime mapping (structure ready, populated later)
