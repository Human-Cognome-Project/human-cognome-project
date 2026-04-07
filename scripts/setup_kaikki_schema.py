#!/usr/bin/env python3
"""
Create the Kaikki ingestion schema.
Every entry gets a row. Everything relevant extracted. No raw JSON kept.
Structural wiki categories excluded. Deprecated entries flagged.
"""

import os, sys, subprocess

DB_ENV = {**os.environ, 'PGPASSWORD': 'hcp_dev'}

def psql_exec(sql):
    r = subprocess.run(['psql','-h','localhost','-U','hcp','-d','hcp_english','-c',sql],
                       capture_output=True, text=True, env=DB_ENV)
    if r.returncode != 0 and r.stderr.strip():
        print(f"ERR: {r.stderr.strip()}", file=sys.stderr)
    return r.returncode == 0

# Structural category patterns to EXCLUDE
# These are wiki plumbing, not semantic data
EXCLUDE_PATTERNS = """
'Pages with%',
'Pages using%',
'English links with%',
'English entries with incorrect%',
'Entries with%',
'Rhymes:%'
"""

schema = """
-- Drop any previous attempt
DROP TABLE IF EXISTS kk_translations CASCADE;
DROP TABLE IF EXISTS kk_descendants CASCADE;
DROP TABLE IF EXISTS kk_sounds CASCADE;
DROP TABLE IF EXISTS kk_sense_examples CASCADE;
DROP TABLE IF EXISTS kk_sense_categories CASCADE;
DROP TABLE IF EXISTS kk_relations CASCADE;
DROP TABLE IF EXISTS kk_senses CASCADE;
DROP TABLE IF EXISTS kk_forms CASCADE;
DROP TABLE IF EXISTS kk_entries CASCADE;

-- =================================================================
-- Core entry: one row per (word, pos, etymology_number) in Kaikki
-- =================================================================
CREATE TABLE kk_entries (
    id                  SERIAL PRIMARY KEY,
    word                TEXT NOT NULL,
    pos                 TEXT NOT NULL,
    etymology_number    SMALLINT DEFAULT 0,
    etymology_text      TEXT,
    is_deprecated       BOOLEAN NOT NULL DEFAULT false,
    -- Token ID assigned later during tokenization pass
    token_id            TEXT,
    -- form_of/alt_of targets (if this entry is a form or alternate)
    form_of_word        TEXT,     -- word it's a form of
    form_of_tags        TEXT[],   -- grammatical tags (plural, past, etc.)
    alt_of_word         TEXT,     -- word it's an alternate of
    alt_of_tags         TEXT[],   -- variant tags (misspelling, dialectal, etc.)
    UNIQUE (word, pos, etymology_number)
);
CREATE INDEX idx_kke_word ON kk_entries (word);
CREATE INDEX idx_kke_pos ON kk_entries (pos);
CREATE INDEX idx_kke_token ON kk_entries (token_id) WHERE token_id IS NOT NULL;
CREATE INDEX idx_kke_form_of ON kk_entries (form_of_word) WHERE form_of_word IS NOT NULL;
CREATE INDEX idx_kke_alt_of ON kk_entries (alt_of_word) WHERE alt_of_word IS NOT NULL;

-- =================================================================
-- Forms: inflected forms from the entry's forms[] list
-- =================================================================
CREATE TABLE kk_forms (
    id          SERIAL PRIMARY KEY,
    entry_id    INTEGER NOT NULL REFERENCES kk_entries(id) ON DELETE CASCADE,
    form        TEXT NOT NULL,
    tags        TEXT[],
    source      TEXT       -- 'conjugation', 'head', etc.
);
CREATE INDEX idx_kkf_entry ON kk_forms (entry_id);
CREATE INDEX idx_kkf_form ON kk_forms (form);

-- =================================================================
-- Senses: one per sense within an entry
-- =================================================================
CREATE TABLE kk_senses (
    id          SERIAL PRIMARY KEY,
    entry_id    INTEGER NOT NULL REFERENCES kk_entries(id) ON DELETE CASCADE,
    sense_num   SMALLINT NOT NULL,
    gloss       TEXT NOT NULL,
    raw_gloss   TEXT,
    tags        TEXT[],
    topics      TEXT[],
    wikidata_id TEXT,
    sense_id    TEXT
);
CREATE INDEX idx_kks_entry ON kk_senses (entry_id);
CREATE INDEX idx_kks_topics ON kk_senses USING GIN (topics) WHERE topics IS NOT NULL;

-- =================================================================
-- Sense categories: semantic/domain/morphological groupings per sense
-- Excludes structural wiki categories
-- =================================================================
CREATE TABLE kk_sense_categories (
    id          SERIAL PRIMARY KEY,
    sense_id    INTEGER NOT NULL REFERENCES kk_senses(id) ON DELETE CASCADE,
    name        TEXT NOT NULL,
    kind        TEXT,      -- 'other', 'place', 'lifeform', 'topical'
    source      TEXT
);
CREATE INDEX idx_kksc_sense ON kk_sense_categories (sense_id);
CREATE INDEX idx_kksc_name ON kk_sense_categories (name);
CREATE INDEX idx_kksc_kind ON kk_sense_categories (kind);

-- =================================================================
-- Sense examples: quotations and usage examples per sense
-- =================================================================
CREATE TABLE kk_sense_examples (
    id          SERIAL PRIMARY KEY,
    sense_id    INTEGER NOT NULL REFERENCES kk_senses(id) ON DELETE CASCADE,
    text        TEXT NOT NULL,
    ref         TEXT,          -- citation/reference
    type        TEXT           -- 'example' or 'quotation'
);
CREATE INDEX idx_kkse_sense ON kk_sense_examples (sense_id);

-- =================================================================
-- Relations: all semantic relationships
-- (synonyms, antonyms, hyponyms, hypernyms, related, derived,
--  meronyms, holonyms, coordinate_terms, descendants)
-- Both entry-level and sense-level
-- =================================================================
CREATE TABLE kk_relations (
    id              SERIAL PRIMARY KEY,
    entry_id        INTEGER NOT NULL REFERENCES kk_entries(id) ON DELETE CASCADE,
    sense_id        INTEGER REFERENCES kk_senses(id) ON DELETE CASCADE,  -- NULL = entry-level
    relation_type   TEXT NOT NULL,   -- synonym, antonym, hyponym, hypernym, derived, related, meronym, holonym, coordinate, descendant
    target_word     TEXT NOT NULL,
    target_lang     TEXT,            -- for descendants/translations, the target language
    tags            TEXT[],
    sense_context   TEXT             -- which sense this relation applies to (from 'sense' field)
);
CREATE INDEX idx_kkr_entry ON kk_relations (entry_id);
CREATE INDEX idx_kkr_sense ON kk_relations (sense_id) WHERE sense_id IS NOT NULL;
CREATE INDEX idx_kkr_type ON kk_relations (relation_type);
CREATE INDEX idx_kkr_target ON kk_relations (target_word);

-- =================================================================
-- Translations: cross-language mappings per entry
-- Quick-check broadphase + actual translation data
-- =================================================================
CREATE TABLE kk_translations (
    id          SERIAL PRIMARY KEY,
    entry_id    INTEGER NOT NULL REFERENCES kk_entries(id) ON DELETE CASCADE,
    lang        TEXT NOT NULL,
    lang_code   TEXT NOT NULL,
    word        TEXT NOT NULL,
    sense       TEXT,          -- which sense this translates
    tags        TEXT[],
    roman       TEXT           -- romanization for non-Latin scripts
);
CREATE INDEX idx_kkt_entry ON kk_translations (entry_id);
CREATE INDEX idx_kkt_lang ON kk_translations (lang_code);
CREATE INDEX idx_kkt_word ON kk_translations (word);

-- =================================================================
-- Sounds: pronunciation data
-- =================================================================
CREATE TABLE kk_sounds (
    id          SERIAL PRIMARY KEY,
    entry_id    INTEGER NOT NULL REFERENCES kk_entries(id) ON DELETE CASCADE,
    ipa         TEXT,
    enpr        TEXT,
    tags        TEXT[]
);
CREATE INDEX idx_kkso_entry ON kk_sounds (entry_id);
"""

print("Creating Kaikki ingestion schema...")
psql_exec(schema)
print("Done.")
