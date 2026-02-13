# Entity Database Design

**Author:** hcp_lib (Librarian Specialist)
**Date:** 2026-02-13
**Status:** Draft specification — ready for review
**Depends on:** pbm-design-notes.md, pbm-format-spec.md, token-addressing.md

---

## 1. Overview

Entity databases store **named entities** — the proper nouns that appear in texts being PBM-encoded. Before a text enters the PBM pipeline, the librarian identifies all named entities, ensures they have label tokens in the language shard, and creates or links entity records in the appropriate entity database.

Fiction and non-fiction entities are **completely separate**. A character named "Paris" in the Iliad and the city of Paris in a history book are different entities in different databases. No shared entity space, no collision, no ambiguity.

### What an Entity Is

An entity is a **specific named thing** — not a concept, not a category, but a particular instance. "Frodo Baggins" is an entity. "hobbit" is a species concept (also an entity, but of type Species). "The Shire" is a place entity. "The One Ring" is a thing entity.

Every entity:
- Has a token ID in one of the entity namespaces
- Has a primary name assembled from label tokens in the language shard
- May have aliases, titles, epithets
- Has typed relationships to other entities
- Appears in one or more PBM-encoded documents
- Has a copyright/IP status

### What an Entity Is Not

- Not a word token. "frodo" as a lowercase dictionary entry lives in AB.AB. The entity "Frodo Baggins" lives in u*.
- Not a concept mesh token. Conceptual meanings live in AA.AC. Entities are concrete instances.
- Not a PBM marker. Document structure lives in AA.AE. Entities are referenced content.

---

## 2. Namespace Architecture

### 2.1 Allocation

Eight namespace letters, four per side:

| Side | PBMs | People | Places | Things |
|------|------|--------|--------|--------|
| **Non-fiction** | z* | y* | x* | w* |
| **Fiction** | v* | u* | t* | s* |

This allocation may shift to sequential numbering in the future. Current scheme is provisional but operational.

### 2.2 Database Hosting Strategy

At bootstrap scale, entity databases will be small. The pragmatic approach:

| Database | Namespaces | Contents |
|----------|------------|----------|
| hcp_fic_entities | u*, t*, s* | All fiction entities (combined) |
| hcp_nf_entities | y*, x*, w* | All non-fiction entities (combined) |

When any entity type within a database exceeds ~500MB, split it into its own database. The shard_registry in hcp_core handles routing regardless of physical database boundaries.

**Shard registry entries needed:**

| Namespace | Shard DB | Description |
|-----------|----------|-------------|
| uA | hcp_fic_entities | Fiction people entities |
| tA | hcp_fic_entities | Fiction place entities |
| sA | hcp_fic_entities | Fiction thing entities |
| yA | hcp_nf_entities | Non-fiction people entities |
| xA | hcp_nf_entities | Non-fiction place entities |
| wA | hcp_nf_entities | Non-fiction thing entities |

### 2.3 Internal Addressing

Entity token IDs follow the standard 5-pair structure:

```
uA.BA.AA.AA.AA
│  │  ││ │  └── Specific entity within sub-type + group
│  │  ││ └───── Entity group/collection
│  │  │└─────── Variant (A = primary)
│  │  └──────── Entity sub-type (see §2.4)
│  └─────────── Shard letter (A = first shard)
└────────────── Mode: fiction people
```

The 3rd pair uses **double-duty encoding** (matching the zA convention): 1st character = entity sub-type, 2nd character = variant.

### 2.4 Entity Sub-Type Addressing

**People entities (u* fiction / y* non-fiction):**

| 1st char | Sub-type | Examples |
|----------|----------|----------|
| A | Individual | Named characters / real people |
| B | Collective | "The Fellowship," "The Inklings" |
| C | Deity/divine | Gods, angels, divine figures |
| D | Named creature | Named animals, beasts, monsters |

**Place entities (t* fiction / x* non-fiction):**

| 1st char | Sub-type | Examples |
|----------|----------|----------|
| A | Settlement | City, town, village, outpost |
| B | Geographic feature | Mountain, river, forest, ocean |
| C | Building/structure | Castle, tower, inn, temple |
| D | Region/territory | Kingdom, empire, province, county |
| E | World/plane | Entire world, dimension, plane of existence |
| F | Celestial body | Moon, star, planet |

**Thing entities (s* fiction / w* non-fiction):**

| 1st char | Sub-type | Examples |
|----------|----------|----------|
| A | Object/artifact | Sword, ring, book, vehicle |
| B | Organization | Guild, army, school, company |
| C | Species/race | Elves, hobbits, kandra |
| D | Concept/system | Magic system, religion, philosophy |
| E | Event | Battle, war, catastrophe, festival |
| F | Language | Fictional/historical language |
| G | Material/substance | Mithril, atium, vibranium |

Sub-types E-Z and a-z are reserved for future allocation in all entity types.

---

## 3. Fiction Entity Schema

All fiction entity tables live in `hcp_fic_entities`. The same schema pattern applies to non-fiction entities in `hcp_nf_entities` (differences noted in §7).

### 3.1 Table: tokens (Entity Registry)

Standard HCP token table. One row per entity. This IS the entity registry.

```sql
CREATE TABLE tokens (
    ns          TEXT NOT NULL,
    p2          TEXT,
    p3          TEXT,
    p4          TEXT,
    p5          TEXT,
    token_id    TEXT NOT NULL GENERATED ALWAYS AS (
                    ns || COALESCE('.' || p2, '')
                       || COALESCE('.' || p3, '')
                       || COALESCE('.' || p4, '')
                       || COALESCE('.' || p5, '')
                ) STORED,
    name        TEXT NOT NULL,           -- Developer label (e.g., 'frodo_baggins')
    category    TEXT,                     -- Entity type: 'person', 'place', 'thing'
    subcategory TEXT,                     -- Sub-type: 'individual', 'settlement', 'artifact', etc.

    CONSTRAINT tokens_pkey PRIMARY KEY (token_id)
);

CREATE INDEX idx_tokens_prefix ON tokens (ns, p2, p3, p4, p5);
CREATE INDEX idx_tokens_ns ON tokens (ns);
CREATE INDEX idx_tokens_ns_p2 ON tokens (ns, p2);
CREATE INDEX idx_tokens_name ON tokens (name);
CREATE INDEX idx_tokens_category ON tokens (category);
CREATE INDEX idx_tokens_subcategory ON tokens (subcategory);
```

The `name` field is a developer label, not the entity's display name. Display names are assembled from label tokens via entity_names (§3.2).

### 3.2 Table: entity_names (Name Assembly)

An entity's name is assembled from **label tokens** in the language shard. "Frodo Baggins" = label token "frodo" (position 1) + label token "baggins" (position 2). The label tokens carry capitalized form variants.

Multiple name forms are supported via `name_group`:
- Group 0 = primary name
- Group 1+ = aliases, titles, epithets, birth names

```sql
CREATE TABLE entity_names (
    -- Which entity (decomposed reference)
    entity_ns   TEXT NOT NULL,
    entity_p2   TEXT,
    entity_p3   TEXT,
    entity_p4   TEXT,
    entity_p5   TEXT,
    entity_id   TEXT NOT NULL GENERATED ALWAYS AS (
                    entity_ns || COALESCE('.' || entity_p2, '')
                               || COALESCE('.' || entity_p3, '')
                               || COALESCE('.' || entity_p4, '')
                               || COALESCE('.' || entity_p5, '')
                ) STORED,

    name_group  SMALLINT NOT NULL DEFAULT 0,  -- 0 = primary, 1+ = aliases
    name_type   TEXT NOT NULL DEFAULT 'primary',
                -- 'primary', 'alias', 'title', 'epithet', 'birth_name',
                -- 'nickname', 'pen_name', 'regnal_name', 'married_name'
    position    SMALLINT NOT NULL,             -- Order within name group

    -- Label token from language shard (decomposed reference)
    ns          TEXT NOT NULL,
    p2          TEXT,
    p3          TEXT,
    p4          TEXT,
    p5          TEXT,
    token_id    TEXT NOT NULL GENERATED ALWAYS AS (
                    ns || COALESCE('.' || p2, '')
                       || COALESCE('.' || p3, '')
                       || COALESCE('.' || p4, '')
                       || COALESCE('.' || p5, '')
                ) STORED,

    CONSTRAINT entity_names_pkey PRIMARY KEY (
        entity_ns, entity_p2, entity_p3, entity_p4, entity_p5,
        name_group, position
    )
);

CREATE INDEX idx_entity_names_entity ON entity_names (
    entity_ns, entity_p2, entity_p3, entity_p4, entity_p5
);
-- Reverse lookup: which entity has this label token in their name?
CREATE INDEX idx_entity_names_token ON entity_names (ns, p2, p3, p4, p5);
CREATE INDEX idx_entity_names_type ON entity_names (name_type);
```

**Example: Frodo Baggins with aliases**

| entity_id | name_group | name_type | position | token_id |
|-----------|------------|-----------|----------|----------|
| uA.AA.AA.AA.AA | 0 | primary | 1 | AB.AB.CA.Xx.yy (frodo) |
| uA.AA.AA.AA.AA | 0 | primary | 2 | AB.AB.CA.Xx.zz (baggins) |
| uA.AA.AA.AA.AA | 1 | alias | 1 | AB.AB.CA.Xx.mm (mr.) |
| uA.AA.AA.AA.AA | 1 | alias | 2 | AB.AB.CA.Xx.nn (underhill) |
| uA.AA.AA.AA.AA | 2 | epithet | 1 | AB.AB.CA.Xx.pp (ring) |
| uA.AA.AA.AA.AA | 2 | epithet | 2 | AB.AB.CA.Xx.qq (bearer) |

(Token IDs are illustrative, not real addresses.)

### 3.3 Table: entity_descriptions

Free-text descriptions typed by role. Separate rows for different description aspects avoids a single massive text blob and allows typed retrieval.

```sql
CREATE TABLE entity_descriptions (
    id              SERIAL PRIMARY KEY,

    -- Which entity (decomposed reference)
    entity_ns       TEXT NOT NULL,
    entity_p2       TEXT,
    entity_p3       TEXT,
    entity_p4       TEXT,
    entity_p5       TEXT,
    entity_id       TEXT NOT NULL GENERATED ALWAYS AS (
                        entity_ns || COALESCE('.' || entity_p2, '')
                                   || COALESCE('.' || entity_p3, '')
                                   || COALESCE('.' || entity_p4, '')
                                   || COALESCE('.' || entity_p5, '')
                    ) STORED,

    description_type TEXT NOT NULL,
                    -- People: 'appearance', 'personality', 'history', 'abilities', 'motivation'
                    -- Places: 'geography', 'culture', 'history', 'economy', 'climate'
                    -- Things: 'function', 'history', 'construction', 'significance', 'properties'
    description     TEXT NOT NULL,
    source_note     TEXT              -- Where this description was sourced from
);

CREATE INDEX idx_entity_desc_entity ON entity_descriptions (
    entity_ns, entity_p2, entity_p3, entity_p4, entity_p5
);
CREATE INDEX idx_entity_desc_type ON entity_descriptions (description_type);
```

### 3.4 Table: entity_properties (EAV)

Extensible key-value properties for type-specific attributes. Avoids needing separate detail tables per entity type during bootstrap.

```sql
CREATE TABLE entity_properties (
    -- Which entity (decomposed reference)
    entity_ns   TEXT NOT NULL,
    entity_p2   TEXT,
    entity_p3   TEXT,
    entity_p4   TEXT,
    entity_p5   TEXT,
    entity_id   TEXT NOT NULL GENERATED ALWAYS AS (
                    entity_ns || COALESCE('.' || entity_p2, '')
                               || COALESCE('.' || entity_p3, '')
                               || COALESCE('.' || entity_p4, '')
                               || COALESCE('.' || entity_p5, '')
                ) STORED,

    key         TEXT NOT NULL,
    value       TEXT NOT NULL,

    CONSTRAINT entity_properties_pkey PRIMARY KEY (
        entity_ns, entity_p2, entity_p3, entity_p4, entity_p5, key
    )
);

CREATE INDEX idx_entity_props_entity ON entity_properties (
    entity_ns, entity_p2, entity_p3, entity_p4, entity_p5
);
CREATE INDEX idx_entity_props_key ON entity_properties (key);
CREATE INDEX idx_entity_props_key_value ON entity_properties (key, value);
```

**Standard property keys by entity type:**

| Entity Type | Key | Example Values |
|-------------|-----|----------------|
| Person | gender | 'male', 'female', 'non-binary', 'unknown' |
| Person | status | 'alive', 'dead', 'undead', 'unknown' |
| Person | species | Entity ID of species in s* (e.g., sA.CA.AA.AA.xx for 'hobbit') |
| Person | birth_era | 'Third Age', 'Victorian era' |
| Person | occupation | 'ring-bearer', 'wizard', 'king' |
| Place | population_est | '100', '50000' |
| Place | founding_era | 'Second Age' |
| Place | government_type | 'monarchy', 'republic', 'council' |
| Thing (org) | org_type | 'military', 'religious', 'commercial', 'political' |
| Thing (org) | member_count_est | '9', '10000' |
| Thing (object) | creator | Entity ID of creator |
| Thing (object) | material | Entity ID of material in s* |
| Thing (species) | sapient | 'true', 'false' |
| Thing (species) | lifespan_est | '100', 'immortal' |
| All | fictional_universe | Entity ID of the universe in s* |
| All | first_appearance_work | Entity ID of work in PBM namespace |
| All | canon_status | 'canon', 'semi-canon', 'non-canon', 'disputed' |

The `fictional_universe` property is **critical** — it groups all entities belonging to the same fictional world. A fictional universe is itself a Thing entity (sub-type D: Concept/system) in the s* namespace.

### 3.5 Table: entity_relationships

Typed, directional relationships between any two entities. Can cross entity types (character → place, character → organization, place → region).

```sql
CREATE TABLE entity_relationships (
    id              SERIAL PRIMARY KEY,

    -- Source entity (decomposed reference)
    source_ns       TEXT NOT NULL,
    source_p2       TEXT,
    source_p3       TEXT,
    source_p4       TEXT,
    source_p5       TEXT,
    source_id       TEXT NOT NULL GENERATED ALWAYS AS (
                        source_ns || COALESCE('.' || source_p2, '')
                                   || COALESCE('.' || source_p3, '')
                                   || COALESCE('.' || source_p4, '')
                                   || COALESCE('.' || source_p5, '')
                    ) STORED,

    -- Target entity (decomposed reference)
    target_ns       TEXT NOT NULL,
    target_p2       TEXT,
    target_p3       TEXT,
    target_p4       TEXT,
    target_p5       TEXT,
    target_id       TEXT NOT NULL GENERATED ALWAYS AS (
                        target_ns || COALESCE('.' || target_p2, '')
                                   || COALESCE('.' || target_p3, '')
                                   || COALESCE('.' || target_p4, '')
                                   || COALESCE('.' || target_p5, '')
                    ) STORED,

    relationship_type TEXT NOT NULL,
    -- See §3.5.1 for controlled vocabulary
    qualifier       TEXT,             -- Additional context (e.g., 'estranged', 'adoptive')
    temporal_note   TEXT,             -- When the relationship applies (e.g., 'Third Age')
    source_note     TEXT              -- Where this relationship was sourced
);

CREATE INDEX idx_entity_rel_source ON entity_relationships (
    source_ns, source_p2, source_p3, source_p4, source_p5
);
CREATE INDEX idx_entity_rel_target ON entity_relationships (
    target_ns, target_p2, target_p3, target_p4, target_p5
);
CREATE INDEX idx_entity_rel_type ON entity_relationships (relationship_type);
```

#### 3.5.1 Relationship Type Vocabulary

Relationships are **directional**: source → target. Many have natural inverses (parent_of ↔ child_of). The engine can compute inverses; only one direction needs to be stored.

**Person ↔ Person:**
- `parent_of`, `child_of`, `sibling_of`
- `spouse_of`, `betrothed_to`
- `mentor_of`, `student_of`
- `ally_of`, `enemy_of`, `rival_of`
- `serves`, `served_by`
- `created_by` (for beings created by others)
- `killed_by`, `killed`

**Person ↔ Place:**
- `born_in`, `died_in`
- `lives_in`, `rules`, `founded`
- `visited`, `imprisoned_in`

**Person ↔ Thing:**
- `member_of` (organization)
- `leader_of` (organization)
- `possesses` (object)
- `created` (object/artifact)
- `belongs_to_species` (species)
- `speaks` (language)
- `participated_in` (event)

**Place ↔ Place:**
- `contains`, `contained_by`
- `borders`, `near`
- `capital_of`

**Place ↔ Thing:**
- `located_in` (organization HQ)
- `occurred_at` (event)
- `source_of` (material)

**Thing ↔ Thing:**
- `part_of` (sub-organization, component)
- `allied_with`, `opposed_to` (organizations)
- `derived_from` (material, concept)
- `used_in` (event ↔ object)
- `related_to` (generic)

### 3.6 Table: entity_appearances

Links entities to the PBM documents where they appear. This is the bridge between the entity databases and the PBM stores.

```sql
CREATE TABLE entity_appearances (
    -- Which entity (decomposed reference)
    entity_ns   TEXT NOT NULL,
    entity_p2   TEXT,
    entity_p3   TEXT,
    entity_p4   TEXT,
    entity_p5   TEXT,
    entity_id   TEXT NOT NULL GENERATED ALWAYS AS (
                    entity_ns || COALESCE('.' || entity_p2, '')
                               || COALESCE('.' || entity_p3, '')
                               || COALESCE('.' || entity_p4, '')
                               || COALESCE('.' || entity_p5, '')
                ) STORED,

    -- Which PBM document (decomposed reference into z*/v* namespace)
    doc_ns      TEXT NOT NULL,
    doc_p2      TEXT,
    doc_p3      TEXT,
    doc_p4      TEXT,
    doc_p5      TEXT,
    doc_id      TEXT NOT NULL GENERATED ALWAYS AS (
                    doc_ns || COALESCE('.' || doc_p2, '')
                            || COALESCE('.' || doc_p3, '')
                            || COALESCE('.' || doc_p4, '')
                            || COALESCE('.' || doc_p5, '')
                ) STORED,

    role        TEXT,                 -- 'protagonist', 'antagonist', 'mentioned',
                                     -- 'setting', 'mcguffin', 'subject', etc.
    prominence  TEXT,                 -- 'major', 'minor', 'background', 'mentioned'
    first_mention_position INTEGER,  -- Position in PBM content stream where entity first appears

    CONSTRAINT entity_appearances_pkey PRIMARY KEY (
        entity_ns, entity_p2, entity_p3, entity_p4, entity_p5,
        doc_ns, doc_p2, doc_p3, doc_p4, doc_p5
    )
);

CREATE INDEX idx_entity_app_entity ON entity_appearances (
    entity_ns, entity_p2, entity_p3, entity_p4, entity_p5
);
CREATE INDEX idx_entity_app_doc ON entity_appearances (
    doc_ns, doc_p2, doc_p3, doc_p4, doc_p5
);
CREATE INDEX idx_entity_app_role ON entity_appearances (role);
```

### 3.7 Table: entity_rights (Copyright/IP Tracking)

Per-entity intellectual property tracking. "Frodo" = Tolkien Estate IP. "Odysseus" = public domain.

```sql
CREATE TABLE entity_rights (
    -- Which entity (decomposed reference)
    entity_ns       TEXT NOT NULL,
    entity_p2       TEXT,
    entity_p3       TEXT,
    entity_p4       TEXT,
    entity_p5       TEXT,
    entity_id       TEXT NOT NULL GENERATED ALWAYS AS (
                        entity_ns || COALESCE('.' || entity_p2, '')
                                   || COALESCE('.' || entity_p3, '')
                                   || COALESCE('.' || entity_p4, '')
                                   || COALESCE('.' || entity_p5, '')
                    ) STORED,

    rights_status   TEXT NOT NULL,
                    -- 'public_domain': free use (ancient mythology, expired copyright)
                    -- 'copyrighted': active copyright holder
                    -- 'trademarked': trademarked name/likeness
                    -- 'fair_use': analysis/reference permitted, reproduction restricted
                    -- 'cc_by': Creative Commons Attribution
                    -- 'cc_by_sa': Creative Commons Attribution-ShareAlike
                    -- 'unknown': rights status not yet determined
    rights_holder   TEXT,             -- Name of the rights holder (if copyrighted)
    rights_note     TEXT,             -- Additional context
    jurisdiction    TEXT,             -- 'US', 'UK', 'EU', 'international', etc.
    expiry_year     INTEGER,          -- Year copyright expires (if known)
    determination_date DATE,          -- When this rights status was determined
    determination_source TEXT,        -- How we determined this (e.g., 'gutenberg_catalog', 'manual')

    CONSTRAINT entity_rights_pkey PRIMARY KEY (
        entity_ns, entity_p2, entity_p3, entity_p4, entity_p5
    )
);

CREATE INDEX idx_entity_rights_status ON entity_rights (rights_status);
CREATE INDEX idx_entity_rights_holder ON entity_rights (rights_holder);
```

### 3.8 Schema Summary (Fiction)

| Table | Purpose | Expected Scale |
|-------|---------|----------------|
| tokens | Entity registry (one row per entity) | Thousands to tens of thousands |
| entity_names | Name components from language shard | ~3-5 rows per entity |
| entity_descriptions | Typed free-text descriptions | ~2-5 rows per entity |
| entity_properties | Key-value attributes (EAV) | ~5-15 rows per entity |
| entity_relationships | Typed directional links between entities | ~3-10 rows per entity |
| entity_appearances | Entity-to-PBM document links | ~1-20 rows per entity |
| entity_rights | IP/copyright tracking | 1 row per entity |

For a massive fictional universe like Tolkien's Middle-earth: ~3,000 named entities. Sanderson's Cosmere: ~5,000+. Each universe produces roughly 15,000-75,000 total rows across all tables. Well within 2GB even for hundreds of fictional universes in the same shard.

---

## 4. Gutenberg Metadata Analysis

### 4.1 Available Fields

Project Gutenberg provides metadata via RDF/XML (Dublin Core + custom terms) and the Gutendex JSON API. Our existing `gutenberg_fetch.py` uses Gutendex. Fields available:

| Field | Source | HCP Mapping |
|-------|--------|-------------|
| id | Gutenberg | document_provenance.external_id |
| title | Gutenberg | PBM tokens.name |
| authors[].name | Gutenberg (pgterms:agent) | Non-fiction person entity (y*) |
| authors[].birth_year | Gutenberg (pgterms:birthdate) | entity_properties on author entity |
| authors[].death_year | Gutenberg (pgterms:deathdate) | entity_properties on author entity |
| subjects[] | LCSH headings | Classification + entity extraction |
| bookshelves[] | Gutenberg categories | Fiction/non-fiction classification |
| languages[] | Gutenberg | document_provenance.content_language |
| copyright | Gutenberg | Boolean → entity_rights.rights_status |
| download_count | Gutenberg | PBM metadata (lightweight) |
| formats{} | Gutenberg | document_provenance.source_path |

**RDF-only fields** (not in Gutendex, available from the RDF catalog):

| Field | Source | HCP Mapping |
|-------|--------|-------------|
| marcrel:ill | MARC relator | Illustrator as person entity (y*) |
| marcrel:trl | MARC relator | Translator as person entity (y*) |
| pgterms:alias | Gutenberg | entity_names (alias name groups) |
| pgterms:webpage | Gutenberg | entity_properties (reference_url) |
| dcterms:description | Dublin Core | PBM metadata |
| Flesch reading ease | Gutenberg | PBM metadata |

### 4.2 Fiction vs. Non-Fiction Classification

Classification uses a multi-signal approach. No single field is definitive.

**Primary signal: bookshelves[]**
- Contains "Category: Novels" → fiction
- Contains "Category: Science-Fiction & Fantasy" → fiction
- Contains "Category: Romance" → fiction (usually)
- Contains "Historical Fiction" → fiction
- Contains "Crime Fiction" → fiction
- Contains "Children's Literature" → needs secondary check
- Contains "Category: History" → non-fiction
- Contains "Category: Science" → non-fiction
- Contains "Category: Philosophy" → non-fiction

**Secondary signal: subjects[] (LCSH)**
- Subject ending in "-- Fiction" → fiction
- "Fictitious character" in subject → fiction
- "Science fiction" → fiction
- "Short stories" → fiction
- "Biography" → non-fiction
- "History" without "Fiction" suffix → non-fiction

**Tertiary signal: LCC classification** (if available from RDF)
- PS, PR, PZ = Literature → likely fiction
- PQ = Romance literature → likely fiction
- D, E, F = History → non-fiction
- Q = Science → non-fiction
- H = Social sciences → non-fiction

**Decision tree:**
1. Check bookshelves for explicit fiction/non-fiction categories
2. Check subjects for "-- Fiction" pattern or fiction genre terms
3. Check LCC prefix if available
4. If ambiguous → flag for manual classification

Roughly 85% of Gutenberg texts get clear bookshelf assignments. The remaining 15% need subject + LCC fallback.

### 4.3 Entity Extraction from Gutenberg Metadata

LCSH subject headings frequently name entities directly:

| Subject Pattern | Entity Type | Example |
|----------------|-------------|---------|
| `{Name} (Fictitious character) -- Fiction` | Fiction person (u*) | "Frankenstein's monster (Fictitious character) -- Fiction" |
| `{Name} (Fictitious character) -- Juvenile fiction` | Fiction person (u*) | "Alice (Fictitious character from Carroll) -- Juvenile fiction" |
| `{Place} ({Country}) -- Fiction` | Real place in fiction context (x*) | "London (England) -- Fiction", "Florence (Italy) -- Fiction" |
| `{Place} -- Fiction` | Real place in fiction context (x*) | "England -- Fiction", "New England -- Fiction" |
| `{Topic} -- Fiction` | Thematic tag, not entity | "Whaling -- Fiction" |
| `March family (Fictitious characters)` | Fiction collective (u*.B*) | "March family (Fictitious characters) -- Fiction" |

**Extraction rules:**
1. Subjects matching `(Fictitious character)` → create fiction person entity
2. Subjects matching `{Place} ({Country}) -- Fiction` → link real place entity to fiction PBM
3. Author entries → create non-fiction person entity (authors are always real people)
4. Named character patterns in subjects → create fiction person entity

### 4.4 Gutenberg → HCP Pipeline

For each Gutenberg text:

1. **Fetch metadata** (Gutendex API or RDF catalog)
2. **Classify** fiction/non-fiction using §4.2 decision tree
3. **Extract entities from metadata** per §4.3 rules
4. **Leverage training data** for well-known works (see §5)
5. **Create label tokens** in hcp_english if name components don't exist
6. **Create entity records** in hcp_fic_entities or hcp_nf_entities
7. **Link entities to PBM** via entity_appearances
8. **Record rights** — Gutenberg texts with `copyright: false` → `public_domain`
9. **Record provenance** in PBM metadata

---

## 5. Librarian Workflow

### 5.1 The Six-Step Pipeline

For any text entering the PBM system, the librarian executes these steps in order:

#### Step 1: Classify the Document

Determine fiction vs. non-fiction. This routes all subsequent entity work to the correct databases.

- **Gutenberg texts**: Use §4.2 multi-signal classification
- **Known works**: Training data makes this trivial (e.g., "Lord of the Rings" = fiction)
- **Unknown texts**: Manual classification or NLP-assisted genre detection
- **Edge cases**: Historical fiction, biographical novels, roman-a-clef → classify as fiction (the entities within may reference real people, handled by cross-referencing non-fiction entity DB)

#### Step 2: Leverage Training Data

For well-known works, the librarian (or the LLM driving it) already knows the entities. This is the fastest, most reliable step.

**Example: Frankenstein (Gutenberg #84)**

Training data yields immediately:
- Characters: Victor Frankenstein, The Creature, Elizabeth Lavenza, Henry Clerval, Robert Walton, Alphonse Frankenstein, William Frankenstein, Justine Moritz, M. Krempe, M. Waldman, De Lacey family, Safie
- Places: Geneva, Ingolstadt, the Arctic, Mont Blanc, the Orkney Islands
- Things: The Creature (also a character), Frankenstein's laboratory

**Example: Pride and Prejudice (Gutenberg #1342)**

Training data yields:
- Characters: Elizabeth Bennet, Mr. Darcy, Jane Bennet, Mr. Bingley, Lydia Bennet, Mr. Wickham, Mr. Collins, Lady Catherine de Bourgh, Charlotte Lucas, Mrs. Bennet, Mr. Bennet, Mary Bennet, Kitty Bennet, Mr. Gardiner, Mrs. Gardiner, Georgiana Darcy, Colonel Fitzwilliam
- Places: Longbourn, Netherfield, Pemberley, Rosings Park, Meryton, Hunsford, Lambton, Brighton, London
- Things: none particularly notable

For copyrighted works not in Gutenberg (e.g., Lord of the Rings): training data is equally rich. Entity knowledge is not limited to public domain texts.

**What training data provides:**
- Entity names (primary + aliases)
- Entity types and sub-types
- Major relationships (family, allegiances, enmities)
- Entity prominence in the work
- Fictional universe membership
- Basic descriptions (appearance, role)

**What training data may miss:**
- Minor background characters mentioned once
- Precise relationship details from obscure passages
- Characters introduced in lesser-known editions or appendices

#### Step 3: Search for Unknowns

For works where training data is insufficient (rare, unpublished, little-known fiction):

1. Search for the work online — reviews, wiki pages, fan sites, study guides
2. Search for author's body of work — recurring characters, shared universes
3. Look for structured entity databases — fan wikis often have character lists

This step is rarely needed for Gutenberg texts (mostly well-known classics). It becomes important for contemporary or indie fiction.

#### Step 4: Scan the Text

NER (Named Entity Recognition) pass over the actual text to catch anything missed by steps 2-3.

- **Capitalized sequences** not at sentence start → candidate proper nouns
- **Dialogue attribution** ("said X") → character names
- **Location markers** ("in X", "at X", "to X") → place candidates
- **Title + name patterns** ("Mr. X", "King X", "the X of Y") → person candidates with titles

Cross-reference candidates against entities already identified in steps 2-3. New candidates are flagged for review.

#### Step 5: Check Language Shard

For each entity's name components, check if label tokens exist in hcp_english (AB.AB):

```sql
-- Check if 'frodo' exists as a label token
SELECT token_id FROM tokens
WHERE name = 'frodo' AND subcategory = 'label'
LIMIT 1;
```

Label tokens (PoS = `label`) are words that have no independent dictionary definition — they exist primarily as name components. Many common name components ("john", "elizabeth", "london") already exist in the language shard as regular words with label variants.

#### Step 6: Create Missing Records

1. **Create label tokens** in hcp_english for any name components not found in step 5
2. **Create entity records** in the appropriate entity database (hcp_fic_entities or hcp_nf_entities)
3. **Populate entity_names** with name component references
4. **Populate entity_relationships** for known relationships
5. **Populate entity_appearances** linking entities to the PBM
6. **Populate entity_rights** with IP status
7. **Populate entity_properties** with known attributes
8. **Populate entity_descriptions** with sourced descriptions

### 5.2 The Fictional Universe Pattern

When processing a work that belongs to a shared fictional universe:

1. **Check if the universe entity exists** in s* (thing, sub-type D: concept)
2. **Create it if missing** — e.g., sA.DA.AA.AA.AA for "Middle-earth"
3. **Set `fictional_universe` property** on all entities created for this work
4. **Check for entity reuse** — if another work in the same universe is already processed, reuse existing entities rather than creating duplicates
5. **Record new appearances** — existing entities appearing in a new work get additional entity_appearances rows

### 5.3 Author Entity Handling

Authors are always **non-fiction person entities** (y*), even when the PBM is fiction:

- The author "Mary Shelley" is a real person → y* entity
- The character "Victor Frankenstein" is fictional → u* entity
- Both are linked to the PBM for Frankenstein via entity_appearances, but with different roles ("author" vs. "protagonist")

The PBM's document_provenance records the author relationship. The entity_appearances table records which entities appear IN the content.

### 5.4 Real Entities in Fiction

Fiction often references real places and historical figures:

- "London" in Pride and Prejudice → the real London (x* entity), referenced from a fiction PBM (v*)
- "Napoleon" mentioned in War and Peace → the real Napoleon (y* entity), referenced from fiction PBM

These cross-side references are stored in entity_appearances. The entity lives in the non-fiction entity DB; the appearance record points to a fiction PBM. This is correct — the entity itself is real, it just happens to appear in a fictional work.

Fictional versions of real places (when the fiction alters them significantly) may warrant a separate fiction entity with a relationship to the real entity: `based_on` or `inspired_by`.

---

## 6. Copyright/IP Schema

### 6.1 Document-Level Rights

Already handled by PBM `document_provenance.rights_status` (§5.2 of pbm-format-spec.md). Values: `public_domain`, `fair_use`, `licensed`, `unknown`.

For Gutenberg texts: `copyright: false` → `rights_status = 'public_domain'` (in the US).

### 6.2 Entity-Level Rights

Tracked by the `entity_rights` table (§3.7). Key distinctions:

| Scenario | Entity Rights | Document Rights | Analysis OK? | Generation OK? |
|----------|--------------|-----------------|--------------|----------------|
| Odysseus in The Odyssey | public_domain | public_domain | Yes | Yes |
| Frodo in Lord of the Rings | copyrighted (Tolkien Estate) | copyrighted | Yes | No (without license) |
| Napoleon in a history text | public_domain | public_domain | Yes | Yes |
| Frodo in fan fiction | copyrighted (Tolkien Estate) | varies | Yes | Derivative work issues |
| Alice in Wonderland | public_domain (expired) | public_domain | Yes | Yes |
| Harry Potter | copyrighted (Rowling/WB) | copyrighted | Yes | No (without license) |

**Rule:** The system can KNOW about any entity for analysis purposes. Copyright status governs what can be GENERATED or REPRODUCED.

### 6.3 Rights Inheritance

When an entity's rights are not explicitly set:
1. Check the entity's `fictional_universe` — if the universe entity has rights, inherit them
2. Check the first appearance work's document rights
3. Default to `unknown`

This is computed by the engine, not stored redundantly. The entity_rights table only stores explicit determinations.

### 6.4 Work-Level Rights Table

An additional table linking PBM documents to rights information, complementing document_provenance:

```sql
CREATE TABLE document_rights (
    -- Which PBM (decomposed reference)
    doc_ns          TEXT NOT NULL,
    doc_p2          TEXT,
    doc_p3          TEXT,
    doc_p4          TEXT,
    doc_p5          TEXT,
    doc_id          TEXT NOT NULL GENERATED ALWAYS AS (
                        doc_ns || COALESCE('.' || doc_p2, '')
                                 || COALESCE('.' || doc_p3, '')
                                 || COALESCE('.' || doc_p4, '')
                                 || COALESCE('.' || doc_p5, '')
                    ) STORED,

    rights_status   TEXT NOT NULL,    -- Same vocabulary as entity_rights
    rights_holder   TEXT,
    license_type    TEXT,             -- 'public_domain_us', 'cc_by_4.0', 'all_rights_reserved', etc.
    source_catalog  TEXT,             -- 'gutenberg', 'archive_org', 'manual', etc.
    catalog_id      TEXT,             -- ID in the source catalog (e.g., Gutenberg book number)
    jurisdiction    TEXT,
    expiry_year     INTEGER,
    determination_date DATE,
    determination_source TEXT,

    CONSTRAINT document_rights_pkey PRIMARY KEY (
        doc_ns, doc_p2, doc_p3, doc_p4, doc_p5
    )
);

CREATE INDEX idx_doc_rights_status ON document_rights (rights_status);
CREATE INDEX idx_doc_rights_catalog ON document_rights (source_catalog, catalog_id);
```

This table lives in the PBM database (hcp_en_pbm for non-fiction, fiction PBM DB for fiction). It complements the simpler `rights_status` field in document_provenance with full rights details.

---

## 7. Non-Fiction Entity Differences

Non-fiction entities (y*, x*, w*) share the same schema as fiction entities but have additional considerations.

### 7.1 Schema: Identical Structure

The tables in hcp_nf_entities are structurally identical to hcp_fic_entities:
- tokens, entity_names, entity_descriptions, entity_properties, entity_relationships, entity_appearances, entity_rights

Same DDL, different data. The namespace prefix (y/x/w vs. u/t/s) distinguishes them.

### 7.2 Property Differences

Non-fiction entities have real-world attributes that fiction entities don't:

**People (y*) — additional property keys:**

| Key | Description | Example |
|-----|-------------|---------|
| birth_date | Actual date (YYYY-MM-DD or partial) | '1797-08-30' |
| death_date | Actual date | '1851-02-01' |
| nationality | Country/countries of citizenship | 'British' |
| wikidata_id | Wikidata identifier | 'Q692' (for Mary Shelley) |
| gutenberg_agent_id | Gutenberg person ID | '61' |
| viaf_id | VIAF identifier | '6293' |
| isni | ISNI identifier | '0000 0001 2117 8154' |

**Places (x*) — additional property keys:**

| Key | Description | Example |
|-----|-------------|---------|
| latitude | Decimal degrees | '48.8566' |
| longitude | Decimal degrees | '2.3522' |
| country | ISO 3166-1 alpha-2 | 'FR' |
| geonames_id | GeoNames identifier | '2988507' |
| wikidata_id | Wikidata identifier | 'Q90' (for Paris) |
| population | Current population | '2161000' |

**Things (w*) — additional property keys:**

| Key | Description | Example |
|-----|-------------|---------|
| founding_date | Actual date | '1776-07-04' |
| dissolution_date | Actual date | '' |
| wikidata_id | Wikidata identifier | 'Q30' (for USA) |
| website | Official website URL | '' |
| industry | For organizations | 'publishing' |

### 7.3 Relationship Differences

Non-fiction relationships need temporal precision:

| Relationship | Fiction | Non-Fiction |
|-------------|---------|-------------|
| `rules` | "Aragorn rules Gondor" (timeless in narrative) | "Elizabeth II rules UK (1952-2022)" — dates matter |
| `member_of` | "Frodo is in the Fellowship" | "Shelley was in the Romantic movement" |
| `lives_in` | "Bilbo lives in Bag End" | "Shelley lived in London, then Geneva, then Italy" — changes over time |

The `temporal_note` field on entity_relationships handles this. For non-fiction, it should carry ISO dates or date ranges when known. For fiction, it carries narrative time references ("Third Age", "before the War of the Ring").

### 7.4 Authority Control

Non-fiction entities should link to established authority files when possible:

- **Wikidata** (wikidata_id) — universal knowledge base
- **VIAF** (viaf_id) — Virtual International Authority File for persons
- **GeoNames** (geonames_id) — geographic entity database
- **ISNI** — International Standard Name Identifier
- **Gutenberg agent ID** — for authors in Project Gutenberg

These identifiers are stored as entity_properties and enable cross-referencing with external knowledge bases. Fiction entities generally lack authority file entries (except for extremely well-known characters with Wikidata entries).

### 7.5 Entity Reuse Across Documents

Non-fiction entities are heavily reused. "Napoleon" appears in thousands of texts. "London" appears in even more. Unlike fiction (where each universe has its own entity set), non-fiction entities are shared globally:

- One "London" entity in x* serves ALL non-fiction PBMs that mention London
- One "Mary Shelley" entity in y* serves all texts by or about her
- entity_appearances grows large for prominent entities

This is correct behavior. The entity is the same real-world thing regardless of which document discusses it.

---

## 8. Scale Considerations

### 8.1 Fiction Entity Scale Estimates

| Universe | Characters | Places | Things | Total Entities |
|----------|-----------|--------|--------|---------------|
| Tolkien's Middle-earth | ~1,500 | ~500 | ~1,000 | ~3,000 |
| Sanderson's Cosmere | ~2,000 | ~600 | ~1,500 | ~4,100 |
| Wheel of Time | ~2,500 | ~400 | ~800 | ~3,700 |
| Harry Potter | ~700 | ~200 | ~500 | ~1,400 |
| A Song of Ice and Fire | ~2,500 | ~500 | ~600 | ~3,600 |
| Shakespeare (all plays) | ~1,200 | ~300 | ~200 | ~1,700 |
| Greek mythology (combined) | ~2,000 | ~400 | ~800 | ~3,200 |
| **Single typical novel** | ~20-50 | ~5-15 | ~5-10 | ~30-75 |
| **Gutenberg fiction (~20K texts)** | ~200K | ~50K | ~50K | ~300K |

At ~20 rows per entity across all tables, 300K entities produce ~6M rows. With typical row sizes, this fits comfortably in a single 2GB shard.

### 8.2 Non-Fiction Entity Scale Estimates

Non-fiction entities grow with the breadth of content ingested:

| Category | Estimated Entities | Notes |
|----------|-------------------|-------|
| Historical figures (y*) | ~500K | Anyone mentioned by name in Gutenberg non-fiction |
| Geographic entities (x*) | ~100K | Cities, countries, rivers, mountains |
| Organizations/things (w*) | ~200K | Companies, institutions, treaties, inventions |
| **Total** | ~800K | After full Gutenberg non-fiction ingestion |

At ~15 rows per entity, 800K entities produce ~12M rows. May need shard splitting for non-fiction. The y* (people) namespace will grow fastest.

### 8.3 Shard Splitting Triggers

Split when a database approaches 1.5GB (well before the 2GB target):

| Trigger | Action |
|---------|--------|
| hcp_fic_entities > 1.5GB | Split largest entity type (probably u*) into own DB |
| hcp_nf_entities > 1.5GB | Split y* (people) into hcp_nf_people |
| Any single shard > 1.5GB | Further split by sub-type (e.g., y*.AA = individuals, y*.BA = collectives) |

Shard splitting is transparent to the engine: update shard_registry, move data, engine routes queries by namespace prefix.

### 8.4 Fictional Universe Partitioning

Within a fiction entity shard, entities are naturally partitioned by fictional universe (via the `fictional_universe` property). This enables:

- **Bulk operations**: Load/unload all entities for a universe
- **Consistency checks**: Verify all entity relationships within a universe are valid
- **Export**: Ship a complete fictional universe as a unit

If a single fictional universe becomes enormous (unlikely but possible for massive collaborative worldbuilding), it could theoretically warrant its own sub-namespace allocation. Unlikely to be needed in practice.

### 8.5 Index Strategy

Primary query patterns and their supporting indexes:

| Query Pattern | Index Used |
|--------------|------------|
| Look up entity by token ID | tokens_pkey |
| Find all entities in a namespace | idx_tokens_ns |
| Find entities by type | idx_tokens_category + idx_tokens_subcategory |
| Get entity's names | idx_entity_names_entity |
| Reverse lookup: entity by name token | idx_entity_names_token |
| Get entity's relationships | idx_entity_rel_source |
| Find who relates to an entity | idx_entity_rel_target |
| Find entity's appearances | idx_entity_app_entity |
| Find entities in a document | idx_entity_app_doc |
| Get entity's properties | idx_entity_props_entity |
| Find entities with specific property | idx_entity_props_key_value |

All indexes use decomposed token columns for B-tree prefix compression.

---

## 9. DB Specialist Action Items

### Phase 1: Foundation

1. **Create `hcp_fic_entities` database** (owner: hcp)
   - Install helper functions (000_helpers.sql)
   - Create all 7 tables per §3.1-3.7

2. **Create `hcp_nf_entities` database** (owner: hcp)
   - Install helper functions
   - Create identical tables (same DDL)

3. **Register in shard_registry** (hcp_core):
   - uA → hcp_fic_entities
   - tA → hcp_fic_entities
   - sA → hcp_fic_entities
   - yA → hcp_nf_entities
   - xA → hcp_nf_entities
   - wA → hcp_nf_entities

4. **Register namespaces** in hcp_core.namespace_allocations:

```sql
INSERT INTO namespace_allocations (pattern, name, description, alloc_type, parent) VALUES
-- Fiction entity namespaces
('u',      'Fiction People',     'Fictional character entities',                    'mode', NULL),
('uA',     'Fiction People (En)', 'Fiction characters (English-primary shard)',      'shard', 'u'),
('t',      'Fiction Places',     'Fictional location entities',                     'mode', NULL),
('tA',     'Fiction Places (En)', 'Fiction locations (English-primary shard)',       'shard', 't'),
('s',      'Fiction Things',     'Fictional object/org/concept entities',           'mode', NULL),
('sA',     'Fiction Things (En)', 'Fiction things (English-primary shard)',          'shard', 's'),
-- Non-fiction entity namespaces
('y',      'NF People',          'Real person entities',                            'mode', NULL),
('yA',     'NF People (En)',     'Real people (English-primary shard)',             'shard', 'y'),
('x',      'NF Places',          'Real location entities',                          'mode', NULL),
('xA',     'NF Places (En)',     'Real places (English-primary shard)',             'shard', 'x'),
('w',      'NF Things',          'Real object/org/concept entities',                'mode', NULL),
('wA',     'NF Things (En)',     'Real things (English-primary shard)',             'shard', 'w'),
-- Fiction PBM namespace
('v',      'Fiction PBMs',       'Fiction document PBM storage',                    'mode', NULL),
('vA',     'Fiction PBMs (En)',  'Fiction PBMs (English-primary shard)',            'shard', 'v');
```

5. **Create `document_rights` table** in hcp_en_pbm per §6.4

### Phase 2: Seed Data (after librarian workflow is operational)

6. **Seed author entities** from existing Gutenberg metadata (10 authors from data/gutenberg/metadata.json → 10 entities in y*)
7. **Seed place entities** extracted from Gutenberg subject headings
8. **Seed character entities** from Gutenberg subject headings with "(Fictitious character)" pattern
9. **Create label tokens** in hcp_english for any missing name components

### Phase 3: Validation

10. **Round-trip verification**: Entity created → name assembled from label tokens → matches expected surface form
11. **Relationship integrity**: All relationship target_ids resolve to existing entities
12. **Cross-shard references**: Entity appearances correctly reference PBM token IDs in z*/v*
13. **Rights consistency**: Entity rights status aligns with document rights

---

## Appendix A: Entity Sub-Type Token Allocation

Entity sub-types should be registered as tokens in hcp_core for type safety. Proposed allocation under AA.AF (new p2 category):

```
AA.AF                   Entity Classification Tokens
├── AA.AF.AA             Person sub-types
│   ├── AA.AF.AA.AA      individual
│   ├── AA.AF.AA.AB      collective
│   ├── AA.AF.AA.AC      deity
│   └── AA.AF.AA.AD      named_creature
├── AA.AF.AB             Place sub-types
│   ├── AA.AF.AB.AA      settlement
│   ├── AA.AF.AB.AB      geographic_feature
│   ├── AA.AF.AB.AC      building
│   ├── AA.AF.AB.AD      region
│   ├── AA.AF.AB.AE      world
│   └── AA.AF.AB.AF      celestial_body
├── AA.AF.AC             Thing sub-types
│   ├── AA.AF.AC.AA      object
│   ├── AA.AF.AC.AB      organization
│   ├── AA.AF.AC.AC      species
│   ├── AA.AF.AC.AD      concept
│   ├── AA.AF.AC.AE      event
│   ├── AA.AF.AC.AF      language
│   └── AA.AF.AC.AG      material
└── AA.AF.AD             Relationship types
    ├── AA.AF.AD.AA      parent_of
    ├── AA.AF.AD.AB      child_of
    ├── ... (full vocabulary per §3.5.1)
```

Registration SQL to be generated by DB specialist from the §3.5.1 vocabulary once approved.

## Appendix B: Worked Example — Frankenstein

Demonstrating the full librarian workflow for Gutenberg #84: Frankenstein; Or, The Modern Prometheus.

**Step 1: Classify** — Bookshelves include "Category: Novels", "Category: Science-Fiction & Fantasy". Subjects include "Gothic fiction", "Horror tales", "Science fiction". Classification: **fiction**.

**Step 2: Training data** yields:

| Entity | Type | Namespace | Sub-type |
|--------|------|-----------|----------|
| Victor Frankenstein | Person | u* | individual |
| The Creature | Person | u* | named_creature |
| Elizabeth Lavenza | Person | u* | individual |
| Henry Clerval | Person | u* | individual |
| Robert Walton | Person | u* | individual |
| Alphonse Frankenstein | Person | u* | individual |
| William Frankenstein | Person | u* | individual |
| Justine Moritz | Person | u* | individual |
| Geneva | Place | x* (real) | settlement |
| Ingolstadt | Place | x* (real) | settlement |
| The Arctic | Place | x* (real) | geographic_feature |
| University of Ingolstadt | Thing | w* (real) | organization |

Note: Geneva, Ingolstadt, the Arctic, and the University of Ingolstadt are **real places/things** used in a fiction context. They go in the non-fiction entity DBs (x*, w*) and are linked to the fiction PBM via entity_appearances.

**Step 3: Search** — Not needed. Frankenstein is extremely well-known.

**Step 4: Scan** — Would catch minor characters like M. Krempe, M. Waldman, the De Lacey family, Safie if missed in step 2.

**Step 5: Check language shard** — Verify label tokens exist in AB.AB for: "victor", "frankenstein", "elizabeth", "lavenza", "henry", "clerval", "robert", "walton", etc.

**Step 6: Create records** — Insert entities, names, relationships, appearances, rights.

Key relationships:
- Victor Frankenstein `created` The Creature
- Victor `spouse_of` Elizabeth Lavenza
- Victor `child_of` Alphonse Frankenstein
- Victor `sibling_of` William Frankenstein
- Victor `lives_in` Geneva (cross-DB: fiction entity → non-fiction place)
- Henry Clerval `ally_of` Victor Frankenstein
- Justine Moritz `killed_by` (wrongful execution — qualifier: "wrongfully accused")

Rights: All entities from Frankenstein are **public_domain** (published 1818, copyright long expired).
