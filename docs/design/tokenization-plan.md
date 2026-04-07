# Tokenization & Migration Plan

**Date:** 2026-04-02
**Status:** Planning

---

## Current State

- 1,453,122 Kaikki entries in kk_ tables (fully structured, categories supplemented from raw data)
- 975,134 entries with token IDs (AB namespace: 971K words, AC namespace: 4K morphemes)
- ~164K Labels with token IDs (AD namespace)
- ~314K entries without IDs (multi-word, phrases, symbols, numerals — deferred)
- 9.2M sense categories loaded

---

## Step 1: Character Namespace

All English-functional single characters go into one sequential namespace under a single p3 value.

**Contents:** a-z, A-Z, 0-9, common punctuation (. , ; : ! ? ' " - / ( ) [ ] { }), accented variants (é, ñ, ü, etc.), common symbols ($, &, @, #, %, +, =, etc.)

**Estimated count:** <500 characters. Fits easily in one 2500-slot p5 range.

**Addressing:** All characters share the same p3+p4 prefix. Each character is identified by its p5 value alone within that prefix. Spelling of any word = shared prefix + sequence of p5 values.

**Convention:** Use p3 = "AA" (represents letter_index=0 + length=0, which is impossible for real words, so no collision).

**Format:** AB.AA.AA.AA.{p5} for full qualified form. Within context, just {p5} per character.

### Question for Patrick:
- p3 = "AA" as the character namespace — does this work, or different convention?

---

## Step 2: Spelling Field

Each word entry gets a spelling field: the sequence of character p5 values that spell it.

**Storage:** Array of small integers or a compact text representation on kk_entries.

**Example:** "walk" → [p5_of_w, p5_of_a, p5_of_l, p5_of_k]

This is the char→word bond. Every word is explicitly tied to its constituent characters in the English shard character namespace.

### Question for Patrick:
- Store as INTEGER[] (array of p5 indices) or as TEXT (concatenated p5 pairs)? Array is queryable, text is more compact.

---

## Step 3: Morphology Field

For every non-lemma entry (forms, derived terms), store the morpheme construction chain.

**What it contains:**
- The lemma token_id this form derives from
- The ordered morpheme chain (using AC namespace morpheme token_ids or labels)

**Example:**
- "walked" → lemma: token_id_of_walk, chain: [PAST]
- "unkindness" → lemma: token_id_of_kind, chain: [NEG_UN, NOUN_NESS]
- "went" → lemma: token_id_of_go, chain: [PAST, IRREGULAR]

**Source data:**
- kk_entries.form_of_word + form_of_tags gives us the lemma link and grammatical tags
- The morpheme itself needs to be identified from the tags or derived from spelling comparison

### Question for Patrick:
- Should the morphology reference the AC namespace morpheme token_ids, or just store morpheme labels as text? Token_id references are more structured, text labels are simpler.

---

## Step 4: Mint Remaining Entries

**Symbol words:** Entries with meaningful English definitions that happen to be symbols. Mint in AB namespace.

**Numeral words:** Entries like "one", "two", "thirteen" etc. (if not already minted — need to check). Also digit entries "0"-"9" with their English PoS roles.

**Missing component words (~5,122 lowercase):** Latin/scientific terms that appear in multi-word entries but don't have their own kk_entries. These need new kk_entries rows created, then token IDs minted.

**Missing name elements (~5,686 capitalized):** Name components from multi-word proper nouns. Need new kk_entries rows in appropriate namespace (AD for Labels).

**Hyphenated compounds:** Verify component elements exist. The compound itself goes one LoD up (AB.AB or equivalent). Components stay in AB.AA.

### Question for Patrick:
- For the ~5,122 missing lowercase words — should these be created as new kk_entries (with minimal data — just word and pos), or handled differently?

---

## Step 5: Unicode Ties

Link English shard single characters to core shard (hcp_core) Unicode codepoints.

This is a cross-reference field on the character entries: "this English character corresponds to this core Unicode token."

The core shard has ~5,470 tokens including byte codes, Unicode characters, structural markers.

**Format:** A field on the character entry pointing to the hcp_core token_id for the same codepoint.

---

## Step 6: Multi-word Entries (LoD Up)

Multi-word entries (phrases, compound proper nouns, idioms) go under p2 = AB (or next available after AA).

Within that LoD level, same 3-pair addressing: {first_letter+word_count}.{counter}.{counter}

Component words reference down to the single-word LoD via full token IDs.

Deferred until all single-word entries and characters are solid.

---

## Step 7: Tokenization Migration

Once all IDs are assigned and fields populated:

1. The engine processes every text field in the kk_ tables (glosses, examples, etymology text)
2. Each word in those texts resolves against the token database
3. The resolved output writes to the working DBs (hcp_english tokens table, LMDB)
4. Cross-references in kk_relations get target_token_ids filled in
5. The database becomes self-referential

---

## Execution Order

1. Character namespace setup
2. Spelling field population
3. Morphology field population
4. Mint remaining entries
5. Unicode ties
6. Multi-word entries
7. Tokenization migration
