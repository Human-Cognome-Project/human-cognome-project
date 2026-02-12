# Decision 002: Names Shard Elimination

**Date:** 2026-02-12
**Status:** Decided, migration pending
**Shards affected:** hcp_names (retiring), hcp_english (absorbing)

## Context

The `hcp_names` database (yA namespace, ~150K tokens) stored "name components" — words that appear in proper nouns (personal names, place names, organizations, temporal labels). The design intent was to keep these cross-linguistic atoms separate from language-specific word tokens.

## Problem

During processing of even a few documents, capitalized versions appeared for essentially any word based on sentence position. This revealed a fundamental issue: having names in a separate database risked pointless duplication of a huge amount of data. Most "name components" are just capitalized forms of existing English words.

## Key Insight

**A "name" is NOT a token property — it's a Proper Noun CONSTRUCT (a bond pattern).**

"Alice" is not inherently a name. It's the word "alice" with a capitalized spelling. It becomes a name when it appears in a Proper Noun bond pattern (e.g. "Alice Liddell" → a Person entity assembled from word tokens via pair bonds).

## New Approach

### 1. Capitalized forms as spelling variants

Every word in `hcp_english` gets its capitalized form as an alternate spelling (form variant). This is a natural property of the word — English capitalizes sentence-initial words, proper usage, titles, etc. The capitalized form belongs with the word, not in a separate namespace.

### 2. Name-only elements

Words that are ONLY names (no other dictionary definition) — like "Bartholomew" — go into `hcp_english` as regular word tokens with:
- Only a capitalized spelling (no lowercase form in common use)
- PoS = `label` if there is no other dictionary definition

The `label` PoS distinguishes these from words that have independent semantic content. A `label` token's meaning comes entirely from the Proper Noun construct it participates in.

### 3. Proper Nouns as bond patterns

The capitalized version of a word is only a "name" when combined into a Proper Noun construct. This is a pair-bond pattern, not a token-level attribute. The PBM captures:
- Which words combine to form the proper noun
- The bonding pattern that identifies it as a Person/Place/Thing entity

### 4. Ingestion behavior

During ingestion, unrecognized capitalized tokens should be cross-referenced as possible transplants — especially if used as a name in context. If a word exists in `hcp_english` but is capitalized in source text, the capitalized form is added as a form variant. If it doesn't exist at all, it's created as a new token with appropriate PoS.

## Important Distinction: AA vs AB tokens

AA namespace tokens (concepts in hcp_core) that share labels with English words (AB namespace in hcp_english) are **intentionally separate entities**. The AA token is the concept — a universal computational element. The AB token is the text form — how English expresses that concept. Words in the core database are labels for understanding; they are not the thing itself.

Example: The NSM primitive "WANT" (AA namespace) is a universal cognitive concept. The English word "want" (AB.AB namespace) is the text token used to express it. They share a label but are distinct entities with different roles.

## Proper Noun Namespace Shift (Planned)

The current proper noun entity namespaces:
- `v*` = People entities
- `w*` = Places
- `x*` = Things

These will eventually shift to sequential allocation rather than scattered single-letter prefixes. **This is a planned future change — no implementation yet.** The current allocation is documented in the addressing spec and will be updated when the shift occurs.

## Impact

### hcp_names database
- Status: **deprecated, pending migration**
- Data preserved until useful parts migrated to hcp_english
- yA namespace allocation retired
- shard_registry entry marked deprecated

### hcp_english database
- Will absorb relevant name-only tokens as regular word entries
- Form variants (capitalized spellings) to be added for all words
- New PoS subcategory `label` for name-only tokens

### Token addressing
- yA namespace retired
- No new namespace needed — names are AB.AB tokens with form variants
- Proper Noun entities (v*/w*/x*) remain as future entity constructs

## Migration Plan (Future Task)

1. Identify name-only tokens in hcp_names that don't already exist in hcp_english
2. Create hcp_english entries for them with PoS = `label`
3. Add capitalized form variants for all hcp_english words
4. Verify no data loss
5. Drop hcp_names database and remove yA from shard_registry
