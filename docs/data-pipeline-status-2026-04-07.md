# HCP Data Pipeline Status — 2026-04-07

## Overview

Full Kaikki English dictionary ingested, tokenized, phrase-resolved, and entity-linked.
The English shard (hcp_english) and entity databases (hcp_nf_entities, hcp_fic_entities)
are populated and cross-referenced. Zero unminted entries remain.

## English Shard (hcp_english)

### Token Counts

| Namespace | Count | Description |
|-----------|-------|-------------|
| AB.AA | 980,039 | Single words |
| AB.AB | 279,873 | Multi-word phrases |
| AC.AA | 4,123 | Morphemes |
| AD.AA | 171,203 | Single-word Labels (proper nouns) |
| AD.AB | 58,726 | Multi-word Labels (compound proper nouns) |
| **Total** | **1,493,964** | |

### Data State

- **1,453,122 entries** in kk_entries, all with token_ids
- **Zero raw text remaining** — all content self-tokenized to token_id arrays
- **9.2M sense categories** for broadphase filtering (29,549 distinct names)
- **1.84M relations** (synonym, derived, related, hyponym, etc.), 91% resolved
- **1,249 english_characters** linked to 19,388 core Unicode tokens
- **Phrase components**: 279,873 phrases with ordered component token arrays

### Active Tables

| Table | Purpose |
|-------|---------|
| entries | Main token table (ns, p2-p5, token_id, word, spelling, morphology) |
| senses | Sense definitions (tokenized_gloss) |
| sense_categories | Category tags for broadphase filtering |
| sense_examples | Usage examples (tokenized_text) |
| relations | Word relationships (synonym, derived, etc.) |
| forms | Inflected forms |
| sounds | Pronunciation data |
| translations | Translation references (target_lang preserved for future shards) |
| phrase_components | Phrase → component word token mapping |
| english_characters | Character → core Unicode token links |
| inflection_rules | Engine morphological stripping rules |
| token_morph_rules | Engine morphology rules |
| ~~tokens / token_pos / token_variants~~ | **DROPPED** — old schema replaced by `entries` table with ns/p2-p5 decomposition |

### Stale Tables (Dropped)

delta_audit_progress, new_tokens, new_token_pos, new_token_variants,
staging_morphynet_deriv, staging_morphynet_infl, morpheme_inventory, token_glosses

## Entity Databases

### Entity Databases (6 DBs, split by literary/non-literary × person/place/thing)

| Database | Tokens | Contents |
|----------|--------|----------|
| hcp_nf_things | 142,513 | Languages, science, technology, organizations, holidays, ethnonyms, other |
| hcp_nf_people | 3,917 | Historical persons, religious figures, mythology (all traditions) |
| hcp_nf_places | 1,452 | Countries, continents, constellations, planets, settlements |
| hcp_fic_people | 962 | Fictional characters, literary individuals (librarian + Kaikki) |
| hcp_fic_places | 10 | Fictional settings (librarian data) |
| hcp_fic_things | 1 | Fictional objects (librarian data) |

### Entity Structure (same across all 6 DBs)

- **tokens**: ns/p2/p3/p4/p5 decomposed, generated token_id, name, category, subcategory, metadata
- **entity_names**: Links entity tokens to shard Label tokens (position-ordered, multi-language ready)
- **entity_descriptions**: Glosses moved from senses table (tokenized, comma-separated)
- **entity_properties**: Category tags, tradition, subcategory
- **entity_relationships**: Source/target entity links with relationship types
- **entity_appearances**: Entity → document links with role/prominence
- **entity_rights**: Rights/permissions metadata
- Token_ids use sequential encoding (Dewey-derived classification deferred)

### Other Databases

| Database | Purpose |
|----------|---------|
| hcp_core | Unicode characters, temporal namespace |
| hcp_envelope | Envelope definitions and queries |
| hcp_fic_pbm | Fiction document PBM bonds (1 doc, scaffolding) |
| hcp_var | Working set assembly (envelope hot cache staging) |
| *(hcp_nf_pbm)* | *(Future: nonfiction document PBM bonds)* |

## Engine Bridge (Pending C++ Work)

The old `tokens` table has been dropped. The engine C++ code references it in:

| File | Usage |
|------|-------|
| HCPEnvelopeManager.cpp | Envelope assembly queries reference `tokens` |
| HCPVocabulary.cpp | Vocab loading from `tokens` |
| HCPBondCompiler.cpp | Bond table compilation from `tokens` |
| HCPPbmReader.cpp | PBM reading, references `token_morph_rules` |
| HCPCacheMissResolver.cpp | Cache miss resolution from `tokens` |
| HCPStorage.cpp | Entity cross-ref queries |

**Migration needed**: Update these to query `entries` table. The `entries` table has
decomposed ns/p2/p3/p4/p5 columns (matching entity DB structure), plus word, pos,
spelling, morphology. 1.49M entries vs the old 553K. Envelope queries will need
updating to use the new table and column names.

### Engine Operations That Need C++ Equivalents

| Operation | Current | Future |
|-----------|---------|--------|
| Phrase resolution | Python trie (batch) | Engine PBD via LMDB phrase beds |
| Entity matching | Python sweep (batch) | Engine manifest post-processing |
| Relation resolution | Python lookup (batch) | Envelope-assembled join at resolve time |
| Possessive minting | Not implemented | See-once-record: mint on first encounter |
| Broadphase phrase matching | N/A | 2-stage: p3 regex (letter+length per word), then content comparison |

## Scripts

### Active (scripts/)

| Script | Purpose |
|--------|---------|
| setup_kaikki_schema.py | Creates kk_ tables |
| load_kaikki_fast.py | Bulk Kaikki JSONL loader |
| self_tokenize.py | Consumes text fields into token_id arrays |
| mint_multiword_ids.py | Mints phrase tokens (AB.AB) |
| mint_category_phrases.py | Mints Wiktionary category names as phrases |
| build_phrase_components.py | Builds phrase → component token mapping |
| phrase_resolve_tokenized.py | Trie-based phrase replacement in tokenized arrays |
| resolve_partial_phrases.py | Resolves partial phrase components (possessives, hyphenated) |
| create_nf_people_entities.py | Creates nf person entities |
| create_nf_place_entities.py | Creates nf country/continent entities |
| create_nf_other_entities.py | Creates nf language/astronomical/holiday entities |
| create_multiword_entities.py | Processes 59K multi-word proper nouns → entity DBs |
| sweep_entity_glosses.py | Exclusion-based sweep: all Labels with non-generic glosses → entities |
| resolve_relations.py | Resolves multi-token relation targets |
| resolve_relations_pass2.py | Case-insensitive second pass |
| merge_frequency_ranks.py | Wikipedia/OpenSubtitles frequency merge |
| ingest_texts.py | Text ingestion driver |
| hcp_client.py | Engine socket client |
| run_benchmark.py | Engine benchmark runner |

### Deprecated (scripts/deprecated/)

build_from_morphynet.py, compose_from_morphynet.py, resolve_unresolved_morphemes.py,
phase2_rule_resolution.py, ingest_kaikki_proper.py, load_kaikki.py, plus earlier
pass1-6 scripts and cleanup utilities.

## Key Design Principles

1. **Everything gets a token** — no exclusion, categories filter instead
2. **Labels are meaning-independent** — surface forms in the shard, meaning in entity DBs
3. **Entity tokens are language-independent** — shards link TO entities, not the other way
4. **Categories are query optimization** — the token graph encodes knowledge, categories are broadphase
5. **See-it-once-and-record** — possessives, alternate forms minted on first encounter
6. **Broadphase before content** — p3 pattern matching (letter + length) before token comparison
7. **Postgres assembles, LMDB caches, engine resolves** — three-layer data pipeline
8. **Mythology is nonfiction** — no judgment on religious validity, all traditions equal
9. **Deltas everywhere** — sub-places reference parent places, glosses are the delta from components

## Remaining Work

- Old tokens → kk_entries migration in C++ engine
- LMDB recompilation from clean kk_entries data
- Entity DB encoding scheme (Dewey-derived, deferred)
- Fiction doc encoding (author_id + work_number)
- 169K unresolved relations (40K char-level, 11K foreign, 117K missing entries)
- Possessive entry minting pipeline
- Cross-shard entity linking (future language shards)
- Middle/Old English shard for archaic text resolution
- Engine testing against clean data with phrase/entity resolution
