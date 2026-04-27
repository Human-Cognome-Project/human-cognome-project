-- source_english: structured/dedup'd source representation of English Wiktextract data.
-- Drained from source_wiktionary.wiktextract_raw (English subset) via the delta rule:
-- "if the JSON duplicates it across entries, it becomes a row everyone points to."
--
-- IDs in this DB are throwaway internal bigserials. Mapping to proper hcp_english
-- token_IDs happens at transfer time, not here.

-- ---------- root entries ----------------------------------------------------
-- One row per (word, pos, etym_number) — same identity as a Wiktextract entry.
CREATE TABLE entries (
    entry_id      bigserial PRIMARY KEY,
    source_id     bigint NOT NULL,                       -- wiktextract_raw.id (cross-DB, not FK)
    word          text NOT NULL,
    pos           text NOT NULL,
    etym_number   smallint,                              -- nullable; many entries omit it
    etymology_id  bigint,                                -- FK -> etymology_blobs (set later)
    notes         text,                                  -- working triage column
    drained_at    timestamptz NOT NULL DEFAULT now()
);
CREATE UNIQUE INDEX uniq_entries_natural ON entries (word, pos, COALESCE(etym_number, -1));
CREATE INDEX idx_entries_source ON entries (source_id);
CREATE INDEX idx_entries_word ON entries (word);

-- ---------- etymology blobs (dedup'd; mostly unique prose, but rule applies) -
CREATE TABLE etymology_blobs (
    etymology_id  bigserial PRIMARY KEY,
    text_hash     bytea NOT NULL,                        -- sha256 for dedup (text may be huge)
    text          text NOT NULL
);
CREATE UNIQUE INDEX uniq_etym_hash ON etymology_blobs (text_hash);

ALTER TABLE entries
    ADD CONSTRAINT fk_entries_etym
    FOREIGN KEY (etymology_id) REFERENCES etymology_blobs(etymology_id);

-- ---------- gloss tree (the dedup'd hierarchy pivot) ------------------------
-- Each unique (parent_id, gloss_text) is a row. Shared prefixes share rows.
-- Senses point to their leaf gloss node.
CREATE TABLE gloss_nodes (
    gloss_id    bigserial PRIMARY KEY,
    parent_id   bigint REFERENCES gloss_nodes(gloss_id),  -- NULL = root
    text        text NOT NULL,
    depth       smallint NOT NULL                          -- 0 = root, derived
);
CREATE UNIQUE INDEX uniq_gloss_parent_text
    ON gloss_nodes (COALESCE(parent_id, 0), text);
CREATE INDEX idx_gloss_parent ON gloss_nodes (parent_id);

-- ---------- senses -----------------------------------------------------------
CREATE TABLE senses (
    sense_id       bigserial PRIMARY KEY,
    entry_id       bigint NOT NULL REFERENCES entries(entry_id) ON DELETE CASCADE,
    sense_n        smallint NOT NULL,                       -- 1-based ordinal in entry
    gloss_leaf_id  bigint REFERENCES gloss_nodes(gloss_id), -- deepest path node
    raw_gloss      text,                                    -- "(colloquial) ..." prefixed string
    UNIQUE (entry_id, sense_n)
);
CREATE INDEX idx_senses_gloss_leaf ON senses (gloss_leaf_id);

-- ---------- tag vocab (flat dedup) ------------------------------------------
CREATE TABLE tags (
    tag_id         bigserial PRIMARY KEY,
    text           text NOT NULL UNIQUE,
    canonical_id   bigint REFERENCES tags(tag_id)            -- e.g., British -> UK
);

CREATE TABLE sense_tags (
    sense_id  bigint NOT NULL REFERENCES senses(sense_id) ON DELETE CASCADE,
    tag_id    bigint NOT NULL REFERENCES tags(tag_id),
    PRIMARY KEY (sense_id, tag_id)
);

-- ---------- category vocab (flat dedup) -------------------------------------
CREATE TABLE categories (
    category_id  bigserial PRIMARY KEY,
    name         text NOT NULL UNIQUE,
    notes        text                                          -- triage column
);

CREATE TABLE sense_categories (
    sense_id     bigint NOT NULL REFERENCES senses(sense_id) ON DELETE CASCADE,
    category_id  bigint NOT NULL REFERENCES categories(category_id),
    PRIMARY KEY (sense_id, category_id)
);

-- ---------- forms (per-entry inflections, alt spellings, regional forms) ----
CREATE TABLE forms (
    form_id    bigserial PRIMARY KEY,
    entry_id   bigint NOT NULL REFERENCES entries(entry_id) ON DELETE CASCADE,
    form_text  text NOT NULL,
    UNIQUE (entry_id, form_text)
);
CREATE INDEX idx_forms_text ON forms (form_text);

CREATE TABLE form_tags (
    form_id  bigint NOT NULL REFERENCES forms(form_id) ON DELETE CASCADE,
    tag_id   bigint NOT NULL REFERENCES tags(tag_id),
    PRIMARY KEY (form_id, tag_id)
);

-- ---------- example sentences (per-sense) -----------------------------------
CREATE TABLE examples (
    example_id  bigserial PRIMARY KEY,
    sense_id    bigint NOT NULL REFERENCES senses(sense_id) ON DELETE CASCADE,
    text        text NOT NULL,
    citation    text,                                          -- source/title if present
    type        text                                           -- "example" | "quote"
);
CREATE INDEX idx_examples_sense ON examples (sense_id);

-- ---------- lexical relations (synonyms, antonyms, hyponyms, etc.) ----------
-- All structurally identical — sense (or entry) -> other word, typed.
CREATE TABLE lex_relations (
    relation_id     bigserial PRIMARY KEY,
    sense_id        bigint REFERENCES senses(sense_id) ON DELETE CASCADE,
    entry_id        bigint REFERENCES entries(entry_id) ON DELETE CASCADE,
    relation_type   text NOT NULL,             -- synonym | antonym | hyponym | hypernym | ...
    target_word     text NOT NULL,
    target_sense    smallint,                  -- if specified
    target_pos      text,                      -- if specified
    notes           text                       -- triage
);
CREATE INDEX idx_relations_sense ON lex_relations (sense_id) WHERE sense_id IS NOT NULL;
CREATE INDEX idx_relations_entry ON lex_relations (entry_id) WHERE entry_id IS NOT NULL;
CREATE INDEX idx_relations_target ON lex_relations (target_word);

-- ---------- pronunciations / sounds -----------------------------------------
CREATE TABLE sounds (
    sound_id   bigserial PRIMARY KEY,
    entry_id   bigint NOT NULL REFERENCES entries(entry_id) ON DELETE CASCADE,
    ipa        text,
    enpr       text,                                          -- enPR transcription
    audio_url  text,
    homophone  text                                            -- if listed
);
CREATE TABLE sound_tags (
    sound_id  bigint NOT NULL REFERENCES sounds(sound_id) ON DELETE CASCADE,
    tag_id    bigint NOT NULL REFERENCES tags(tag_id),
    PRIMARY KEY (sound_id, tag_id)
);

-- ---------- drainage progress meta ------------------------------------------
CREATE TABLE drain_progress (
    pk             smallint PRIMARY KEY DEFAULT 1 CHECK (pk = 1),
    started_at     timestamptz,
    completed_at   timestamptz,
    rows_drained   bigint DEFAULT 0,
    last_source_id bigint,
    notes          text
);
INSERT INTO drain_progress (pk) VALUES (1);
