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

## Migration 015: Docvar Staging (also applied)

Added to **hcp_fic_pbm** alongside existing PBM tables.

### New table: `docvar_groups`

Alias groupings per document. All docvars in the same group are aliases for the same entity/concept.

| Column | Purpose |
|--------|---------|
| `id` | Serial PK |
| `doc_id` | References pbm_documents |
| `suggested_ns/p2/p3/p4/p5` | Decomposed suggested entity match (NULL if new) |
| `suggested_id` | Generated entity token_id |
| `entity_type` | 'person', 'place', 'thing', 'lingo', 'sic', 'uri_metadata' |
| `status` | 'pending' → 'confirmed' → 'promoted' (or 'rejected') |
| `suggested_by` | 'engine', 'reviewer', 'backprop' |

### Changes to `pbm_docvars`

| New Column | Purpose |
|------------|---------|
| `var_category` | Engine-assigned: 'proper', 'lingo', 'sic', 'uri_metadata' |
| `group_id` | FK to docvar_groups (NULL = ungrouped) |

### Engine workflow

1. Create docvars during encoding (existing)
2. Set `var_category` on each var
3. Group aliases → create `docvar_groups` entry, set `group_id` on member vars
4. Optionally set `suggested_ns/p2/...` if entity match found
5. Back-propagation: when a stem resolves later, update earlier var's `equivalence` and `group_id`

### Reviewer workflow

1. Query `docvar_groups WHERE status = 'pending'` for review queue
2. Confirm/reject groupings and matches
3. On confirmation → status = 'promoted', bond data patched, entity records created

## Questions for Engine Specialist

1. **Column naming conflict**: You noticed two columns with the same data but different names somewhere in the schema. Can you point me to which tables/columns so I can fix it?

2. **Anything else needed?** The general approach is: build it, see it in the GUI, refine. Let me know what's missing once you start wiring.
