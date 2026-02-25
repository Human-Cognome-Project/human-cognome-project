# DB Specialist Notes: Source Records Schema (Migration 014)

## What Changed

Migration 014 added 6 tables to **hcp_nf_entities** for tracking Works (novels, etc.) as non-fiction Things. A novel is a real-world object regardless of whether its contents are fiction.

### New Tables

| Table | Purpose |
|-------|---------|
| `sources` | Work record — token_id (wA.DA.*), author ref (yA.*), first_pub_date, metadata JSONB |
| `source_editions` | Edition subtable — edition 0 = reference (full PBM), 1+ = deltas |
| `source_people` | Dramatis Personae — junction: Work → Person entities (u* fic, y* nf) |
| `source_places` | Locae — junction: Work → Place entities (t* fic, x* nf) |
| `source_things` | Rerum — junction: Work → Thing entities (s* fic, w* nf) |
| `source_glossary` | Author-specific vocabulary — surface form + equivalence token_id + gloss |

All existing entity_* tables and data are preserved alongside these.

### Key Design Points

1. **No surface forms in source tables** — everything is token_id references. The only surface forms are in `source_glossary` (author lingo like "warn't") and future sic storage.

2. **Edition 0 = reference PBM** (full encoding). The `source_editions.pbm_doc_id` cross-references `pbm_documents.doc_id` in hcp_fic_pbm. For Gutenberg single-edition works, there's one edition (0).

3. **Work name is in entity_names** — assembled from label tokens, same as Person/Place names. No title field on sources.

4. **All references decomposed** — author on sources, source on editions, both sides of junction tables. Generated token_id columns for JOINs/LMDB export.

## Relevance to Engine

### Entity Recognition Cascade (discussed with Patrick)

The engine's rigid body detection at the word level needs to recognize entity names. The agreed cascade:

1. Find largest contiguous block of Label tokens + bridge words (e.g., "The Adventures of Tom Sawyer")
2. Exact match against entity surface forms in LMDB
3. No match → try non-bridged blocks (capitalized words only)
4. No match → try individual entity tokens
5. All fail → register as new Proper Noun candidate var

**Bridge words** (of, the, and, de, von, etc.) — will be a managed list in hcp_english. Not created yet.

**Entity surface form LMDB tables** — not designed yet. The engine will need a way to look up multi-word entity names (keyed by surface string) and get back entity token_ids. This is future work that will need engine input on key format and lookup patterns.

### Four Var Categories

During PBM encoding, unrecognized strings classify as:

1. **Sic values** — numbers, URIs, non-word patterns. Preserved atomic. Optional equivalence for OCR errors.
2. **URI-as-metadata** — subset of sic routed to provenance.
3. **Proper candidates** — Label tokens or non-positional capitals → entity candidates.
4. **Author lingo** — unrecognized lowercase → glossary entries with equivalences.

All start as pbm_docvars, promoted on review.

## Questions for Engine Specialist

1. **LMDB entity lookup format**: What key format works for multi-word entity matching? The current w2t sub-db uses surface strings as keys. Would entity lookup work the same way (surface string → entity token_id), or does the rigid body detector need a different structure?

2. **Bridge word handling**: Does the engine need the bridge word list at runtime (loaded into LMDB), or is it compiled into the rigid body detection logic?

3. **Var category detection**: The engine needs to classify unrecognized blocks into the four categories above. Is the detection order (sic → URI → proper → lingo) something you'd handle in the tokenizer, or does it need DB-side support (pattern tables, etc.)?

4. **Column naming conflict**: You noticed two columns with the same data but different names somewhere in the schema. Can you point me to which tables/columns so I can fix it?

5. **Scope loading**: When processing a specific Work, the engine will need that Work's Dramatis Personae, Locae, Rerum, and glossary loaded into LMDB. What's the preferred trigger — is it part of the cache miss pipeline, or a bulk preload when a document is opened for processing?
