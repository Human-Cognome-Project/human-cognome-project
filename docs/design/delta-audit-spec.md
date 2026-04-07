# Delta Audit — Token Root Integrity

**Author:** Patrick + Claude Opus 4.6
**Date:** 2026-03-23
**Status:** Design

---

## Problem

The `hcp_english.tokens` table holds ~569K entries intended to be irreducible conceptual roots. Approximately 50-60K are **pre-computed deltas masquerading as roots**: inflected forms, derived forms, and compositional forms that should not exist as independent tokens.

This corruption exists because broad filtering rules were used during Kaikki population instead of individual word assessment. The Kaikki filter checked for `form-of`/`alt-of` Wiktionary tags, but many derived forms passed because Wiktionary gives them independent headword entries. "Things", "kindly", "running" all passed the filter despite being deltas from their base forms.

### Why This Matters

Every false root in the tokens table:
- Erases morpheme force signals (the engine matches the expanded form directly, never generating the delta)
- Fragments the cognitive map with duplicate concepts
- Corrupts the word-to-concept translation boundary
- Makes morpheme stripping rule validation impossible (can't identify rule errors without clean reference data)

Clean word space is a prerequisite for accurate ToM modeling. NAPIER's ability to perceive, model, and communicate Theory of Mind depends directly on the fidelity of this boundary.

---

## The Delta Principle

**Every entry's root status is determined by one question: is this a delta from another point?**

- If the word's meaning can be fully expressed as an existing base + morpheme chain, it is a delta.
- The full chain is always followed. "Unkindly" → un + kind + ly. No arbitrary stopping points.
- PoS is irrelevant to root status. "Kindly" having ADJ senses does not make it a root — those senses are derivative of kind + ADVERB_LY.
- Accumulated dictionary senses do not create root status. The meaning IS the composition.
- Multiple glosses on a token are deltas — different views of the same concept conditioned by PoS or usage context.

### Default Root Form

**Present tense, first person singular, or equivalent** — the most compact available root form. All deltas are from this point.

This is especially important for archaic and deprecated forms with unique surface forms. These are stored as full variants under the core token with appropriate tags:
- "won't" → variant of "will" (CONTRACTION + NEG)
- "thou" → variant of "you" (ARCHAIC)
- "went" → variant of "go" (PAST + IRREGULAR)
- "hath" → variant of "have" (ARCHAIC)
- "wrought" → variant of "work" (PAST_PARTICIPLE + ARCHAIC)

### False Positives

Genuine roots that coincidentally match a suffix/prefix pattern are rare. The only true false positives are etymologically accidental:
- "butter" — not "butt" + er (Germanic root)
- "hamster" — not "hamst" + er
- "master" — not "mast" + er (Latin magister)

### Overlap Cases

Rarely, a word has both a valid independent meaning AND a coincidental morphological decomposition. Example: "mister" is its own root (a form of address), but "mist" + AGENT_ER is a valid surface parse.

The test remains: is the primary meaning expressible as base + morpheme? "Mister" does not mean "one who mists" — it is a root. The alternate decomposition (mist + AGENT_ER) should be noted on the entry because it serves as a **concept space fallback**. If context pushes the primary gloss out of focus — e.g. a text about someone who mists plants — the alternate decomposition becomes the correct conceptual path. Without it documented, there is no route to that meaning. The word resolves fine in word space (direct match), but concept space translation needs the fallback.

---

## Solution: Sonnet Word-by-Word Assessment

A Claude Sonnet instance reads each non-Label token in `hcp_english.tokens` and makes an individual delta assessment. No batch filters. No pre-computed candidate bases. No suffix grouping. Each word gets individual attention from a model that understands English morphology.

### Scope

Sonnet's scope is strictly the `hcp_english` shard:
- Read tokens
- Assess each word individually
- Act on verdicts — update/remove entries in `hcp_english`
- Log genuine uncertainties to a file for human review

**Nothing else.** No LMDB. No engine. No envelope system. No downstream pipeline. Words only.

### Decision Process Per Word

For each token entry, Sonnet:

1. Looks at the word
2. Determines: is there a more compact root form that this derives from?
3. **If yes**: this is a delta. Record the base and the morpheme chain. Apply appropriate tags (ARCHAIC, IRREGULAR, DIALECT, CASUAL, etc.). Update `hcp_english` accordingly — ensure the variant relationship is recorded under the base token, remove the false root.
4. **If no**: this is a root. Move on.
5. **If genuinely uncertain**: log the word and the uncertainty to a review file. Do not force a decision.

### What Sonnet Knows

Sonnet has near-native English vocabulary. It knows:
- "Thou" is archaic second person of "you"
- "Wrought" is archaic past participle of "work"
- "Kindly" is kind + ly regardless of how many adjective senses Wiktionary lists
- "Butter" has no morphological relationship to "butt"

Edge cases should be genuinely rare — a few hundred words, not thousands.

### Morpheme Labels

Sonnet should use consistent morpheme labels for delta chains:

**Suffixes**: PLURAL, PAST, PROGRESSIVE, 3RD_SING, ADVERB_LY, AGENT_ER, COMPARATIVE_ER, SUPERLATIVE_EST, NOUN_NESS, NOUN_MENT, NOUN_TION, NOUN_ITY, ADJ_ABLE, ADJ_FUL, ADJ_LESS, ADJ_OUS, ADJ_IVE, ADJ_AL, ADJ_IC, VERB_IZE, VERB_ATE, VERB_EN, VERB_IFY, POSSESSIVE

**Prefixes**: NEG_UN, NEG_IN, NEG_IM, NEG_IL, NEG_IR, NEG_DIS, NEG_NON, ITER_RE, PRE, MIS, OVER, OUT, UNDER, ANTI, DE

**Other**: IRREGULAR (paired with the morpheme it represents), ARCHAIC, DIALECT, CASUAL, CONTRACTION, COMPOUND

New labels may be created if the existing set doesn't cover a specific morpheme relationship. Consistency matters more than fitting a fixed list.

---

## DB Operations

When Sonnet identifies a delta:

1. Ensure the base token exists in `tokens` (it should — if it doesn't, flag for review)
2. Create/update a `token_variants` entry linking the surface form to the base token with the morpheme chain and characteristic tags
3. Migrate any gloss information that is genuinely unique to the variant (rare — most glosses are just restatements of the base concept + morpheme)
4. Remove the false root from `tokens` (cascades to `token_pos`, `token_glosses`)

When Sonnet confirms a root: no action needed.

When Sonnet is uncertain: log to `delta_audit_uncertain.log` with the word, the candidate base (if any), and the reason for uncertainty.

---

## Secondary Output: Rule Validation

After the audit completes, the delta verdicts can be compared against the existing `inflection_rules` table to identify:
- **Missing rules**: Sonnet found deltas that no existing rule would catch
- **Overfiring rules**: existing rules that would strip genuine roots
- **Priority errors**: rules firing in the wrong order

This is a separate analysis step using the audit data. It is not part of Sonnet's task.

---

## Success Criteria

1. Every non-Label token in `hcp_english.tokens` has been individually assessed
2. Confirmed deltas are removed from `tokens` and properly recorded as variants under their base
3. Uncertainty log is small (hundreds, not thousands) and reviewed by Patrick
4. Morpheme stripping rules can be validated against clean reference data for the first time
