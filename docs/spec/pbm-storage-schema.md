# PBM Storage Schema Design

**Status:** Design specification (Phase 2)
**Dependencies:** Document storage decision, sliceability requirements

---

## Overview

Storage schema for Pair-Bond Maps (PBMs) with metadata for slicing, querying, and reconstruction.

**Key requirements:**
- Store word-level PBMs (tokens are words/punctuation, whitespace implicit)
- Enable slicing by: document, paragraph, sentence, author, language, topic, era
- Support efficient reconstruction
- Link to source documents (provenance)
- Multi-language support

---

## Database Architecture

### Core Tables

#### **1. Documents**
Source texts with metadata.

```sql
CREATE TABLE documents (
    -- Identity
    doc_id SERIAL PRIMARY KEY,
    external_id VARCHAR(100),  -- Gutenberg ID, ISBN, etc.
    source VARCHAR(50),        -- 'gutenberg', 'custom', etc.

    -- Content
    title TEXT NOT NULL,
    content_hash CHAR(64),     -- SHA-256 of original text

    -- Metadata
    language VARCHAR(10) NOT NULL,  -- ISO 639-1 code (en, es, etc.)
    author TEXT[],
    author_birth_years INT[],
    publication_year INT,

    -- Classification
    subjects TEXT[],           -- Topics/themes
    genres TEXT[],            -- Literary genres
    bookshelves TEXT[],       -- Categories

    -- Storage
    file_path TEXT,           -- Path to original text file
    stored_at TIMESTAMP DEFAULT NOW(),

    -- Stats
    token_count INT,
    char_count INT,
    download_count INT        -- Original popularity (for reference)
);

CREATE INDEX idx_documents_language ON documents(language);
CREATE INDEX idx_documents_source ON documents(source);
CREATE INDEX idx_documents_subjects ON documents USING GIN(subjects);
```

#### **2. Scopes**
Hierarchical text segments (document → paragraph → sentence).

```sql
CREATE TABLE scopes (
    -- Identity
    scope_id SERIAL PRIMARY KEY,
    doc_id INT REFERENCES documents(doc_id),

    -- Hierarchy
    scope_type VARCHAR(20) NOT NULL,  -- 'document', 'paragraph', 'sentence'
    parent_scope_id INT REFERENCES scopes(scope_id),

    -- Position
    start_offset INT NOT NULL,  -- Character offset in document
    end_offset INT NOT NULL,
    sequence_num INT,          -- Order within parent scope

    -- Content
    content_hash CHAR(64),     -- Hash of this scope's content
    token_count INT,

    CONSTRAINT scope_type_check CHECK (scope_type IN ('document', 'paragraph', 'sentence', 'phrase'))
);

CREATE INDEX idx_scopes_doc ON scopes(doc_id);
CREATE INDEX idx_scopes_type ON scopes(scope_type);
CREATE INDEX idx_scopes_parent ON scopes(parent_scope_id);
```

#### **3. Tokens**
Word and punctuation tokens (from all documents).

```sql
CREATE TABLE tokens (
    -- Identity
    token_id BIGSERIAL PRIMARY KEY,
    token_string TEXT NOT NULL,
    token_type VARCHAR(20),    -- 'word', 'punctuation', 'number', etc.

    -- HCP addressing
    hcp_token_id VARCHAR(50),  -- Base-50 token address (when assigned)
    namespace VARCHAR(10),     -- Token namespace (AA, BA, etc.)

    -- Linguistic
    language VARCHAR(10),
    pos_tag VARCHAR(20),       -- Part of speech (optional)

    -- NSM decomposition (future)
    nsm_primitives TEXT[],     -- NSM primitive IDs
    abstraction_depth INT,     -- Layers from NSM primitives

    -- Stats
    doc_frequency INT DEFAULT 1,  -- Number of documents containing this token
    total_frequency BIGINT DEFAULT 1  -- Total occurrences across corpus
);

CREATE UNIQUE INDEX idx_tokens_string_lang ON tokens(token_string, language);
CREATE INDEX idx_tokens_type ON tokens(token_type);
CREATE INDEX idx_tokens_hcp_id ON tokens(hcp_token_id);
```

#### **4. PBMs (Pair-Bond Maps)**
Forward pair bonds with recurrence counts.

```sql
CREATE TABLE pbms (
    -- Identity
    pbm_id BIGSERIAL PRIMARY KEY,
    scope_id INT REFERENCES scopes(scope_id),

    -- Bond
    token_id_0 BIGINT REFERENCES tokens(token_id),
    token_id_1 BIGINT REFERENCES tokens(token_id),

    -- Recurrence
    fbr INT NOT NULL,  -- Forward Bond Recurrence (count in this scope)

    -- Position (for reconstruction)
    first_occurrence_offset INT,  -- Position in scope where first occurs

    -- Metadata
    created_at TIMESTAMP DEFAULT NOW(),

    CONSTRAINT pbm_unique_bond_in_scope UNIQUE (scope_id, token_id_0, token_id_1)
);

CREATE INDEX idx_pbms_scope ON pbms(scope_id);
CREATE INDEX idx_pbms_token0 ON pbms(token_id_0);
CREATE INDEX idx_pbms_token1 ON pbms(token_id_1);
CREATE INDEX idx_pbms_bond ON pbms(token_id_0, token_id_1);
```

#### **5. Reconstruction Seeds**
Seed pairs for starting reconstruction (optimization).

```sql
CREATE TABLE reconstruction_seeds (
    seed_id SERIAL PRIMARY KEY,
    scope_id INT REFERENCES scopes(scope_id),

    -- Seed tokens (distinct pairs that can start reconstruction)
    token_id_0 BIGINT REFERENCES tokens(token_id),
    token_id_1 BIGINT REFERENCES tokens(token_id),

    -- Metadata
    seed_strength FLOAT,  -- How reliable this seed is

    UNIQUE (scope_id, token_id_0, token_id_1)
);

CREATE INDEX idx_seeds_scope ON reconstruction_seeds(scope_id);
```

---

## Metadata for Slicing

### Query Examples

**Slice by language:**
```sql
-- Get all English PBMs
SELECT pbms.*
FROM pbms
JOIN scopes ON pbms.scope_id = scopes.scope_id
JOIN documents ON scopes.doc_id = documents.doc_id
WHERE documents.language = 'en';
```

**Slice by author birth era:**
```sql
-- Get PBMs from 19th century authors
SELECT pbms.*
FROM pbms
JOIN scopes ON pbms.scope_id = scopes.scope_id
JOIN documents ON scopes.doc_id = documents.doc_id
WHERE documents.author_birth_years && ARRAY[1800, 1801, ..., 1899];
```

**Slice by topic:**
```sql
-- Get PBMs from fiction
SELECT pbms.*
FROM pbms
JOIN scopes ON pbms.scope_id = scopes.scope_id
JOIN documents ON scopes.doc_id = documents.doc_id
WHERE 'fiction' = ANY(documents.subjects);
```

**Slice by scope type:**
```sql
-- Get sentence-level PBMs only
SELECT pbms.*
FROM pbms
JOIN scopes ON pbms.scope_id = scopes.scope_id
WHERE scopes.scope_type = 'sentence';
```

---

## Whitespace Reconstruction Rules

**Storage options:**

### **Option A: In Language Config Table**
```sql
CREATE TABLE spacing_rules (
    rule_id SERIAL PRIMARY KEY,
    language VARCHAR(10) NOT NULL,

    -- Pattern
    left_token_type VARCHAR(20),   -- 'word', 'punctuation', etc.
    right_token_type VARCHAR(20),
    left_token_value TEXT,         -- Specific token (optional)
    right_token_value TEXT,        -- Specific token (optional)

    -- Spacing
    spacing VARCHAR(10),           -- ' ', '', '\n', etc.

    -- Priority (for conflict resolution)
    priority INT DEFAULT 1,

    UNIQUE (language, left_token_type, right_token_type,
            left_token_value, right_token_value)
);

-- Example data for English
INSERT INTO spacing_rules (language, left_token_type, right_token_type, spacing, priority) VALUES
  ('en', 'word', 'word', ' ', 1),
  ('en', 'word', 'punctuation', '', 2),
  ('en', 'punctuation', 'word', ' ', 1),
  ('en', 'punctuation', 'punctuation', '', 1);

-- Exception: opening quote
INSERT INTO spacing_rules (language, left_token_type, right_token_type, right_token_value, spacing, priority) VALUES
  ('en', 'word', 'punctuation', '"', ' ', 3);  -- Higher priority = exception
```

### **Option B: JSON Config Files**
```
textures/
└── spacing/
    ├── en.json
    ├── es.json
    └── zh.json
```

**`textures/spacing/en.json`:**
```json
{
  "language": "en",
  "default": " ",
  "rules": [
    {
      "left": {"type": "word"},
      "right": {"type": "word"},
      "spacing": " ",
      "priority": 1
    },
    {
      "left": {"type": "word"},
      "right": {"type": "punctuation"},
      "spacing": "",
      "priority": 2,
      "exceptions": [
        {"right_value": "\"", "spacing": " "}
      ]
    },
    {
      "left": {"type": "punctuation"},
      "right": {"type": "word"},
      "spacing": " ",
      "priority": 1
    }
  ]
}
```

---

## Reconstruction Algorithm

```python
def reconstruct_from_pbm(scope_id: int) -> str:
    """
    Reconstruct text from PBM.

    Args:
        scope_id: Scope to reconstruct

    Returns:
        Reconstructed text with whitespace
    """
    # 1. Get seed pair (or first token in sequence)
    seed = get_reconstruction_seed(scope_id)

    # 2. Build token sequence using physics engine
    #    (follows PBM bonds, energy minimization)
    tokens = physics_engine.reconstruct(scope_id, seed)

    # 3. Get language for this scope
    language = get_scope_language(scope_id)

    # 4. Apply spacing rules
    text = ""
    for i, token in enumerate(tokens):
        text += token.string

        if i < len(tokens) - 1:
            next_token = tokens[i + 1]
            spacing = get_spacing_rule(
                language=language,
                left_type=token.type,
                right_type=next_token.type,
                left_value=token.string,
                right_value=next_token.string
            )
            text += spacing

    return text
```

---

## Storage Location Recommendations

**Directory structure:**
```
/home/patrick/gits/human-cognome-project/
├── data/
│   ├── gutenberg/
│   │   ├── texts/          # Original downloaded texts
│   │   └── metadata.json   # Gutendex metadata
│   └── processed/
│       └── pbms/           # Processed PBM data (if not in DB)
├── db/
│   ├── pbm_storage.sql     # Schema creation script
│   └── spacing_rules.sql   # Default spacing rules
└── textures/
    └── spacing/            # JSON config files (if using Option B)
        ├── en.json
        ├── es.json
        └── zh.json
```

**Database:**
- Development: SQLite at `data/hcp_pbms.db`
- Production: PostgreSQL (same as existing shards)

---

## Next Steps

1. **Decide spacing rule storage:** Database table vs JSON config files
2. **Create schema SQL script** in `db/pbm_storage.sql`
3. **Implement encoding pipeline** that populates these tables
4. **Test reconstruction** with Gutenberg samples

---

## Open Questions

1. Should reconstruction seeds be auto-generated or manually defined?
2. How to handle multi-language documents (code-switching)?
3. Should we store character-level PBMs in addition to word-level?
4. Compression strategy for PBM storage at scale?
