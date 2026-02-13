-- HCP Migration 006a: Force Infrastructure Tokens (hcp_core)
--
-- Adds ~25 universal tokens to AA.AC.AB namespace:
--   AA.AC.AB.AA.* — Force types (7)
--   AA.AC.AB.AB.* — Relationship types (7)
--   AA.AC.AB.AC.* — LoD levels (8)
--   AA.AC.AB.AD.* — Universal structural principles (3+)
--
-- These are the vocabulary that all force pattern tables reference.
-- Language-specific constants go in hcp_english (Migration 006b+).
--
-- Namespace allocation rationale (Decision from DB specialist, per
-- Project Lead directive that core = cross-domain bridging config):
--   AA.AC.AA = conceptual mesh (NSM primitives + structural tokens)
--   AA.AC.AB = force infrastructure (engine configuration)
--
-- Prerequisite: hcp_core with decomposed token schema (Decisions 001, 005)

BEGIN;

-- ================================================================
-- FORCE TYPES (7 tokens) — AA.AC.AB.AA.*
-- ================================================================
-- Universal categories of force that operate in all expression modes.
-- Language-specific force CONSTANTS derive from these types.

INSERT INTO tokens (ns, p2, p3, p4, p5, name, category, subcategory, metadata) VALUES
('AA','AC','AB','AA','AA', 'attraction',        'force_type', 'universal', '{"description": "Selectional requirements between heads and dependents. Determines which elements can combine."}'),
('AA','AC','AB','AA','AB', 'binding_energy',     'force_type', 'universal', '{"description": "Strength of structural bonds. Determines how tightly elements are held together in a construction."}'),
('AA','AC','AB','AA','AC', 'ordering',           'force_type', 'universal', '{"description": "Linear precedence constraints. Determines surface word order from structural relationships."}'),
('AA','AC','AB','AA','AD', 'compatibility',      'force_type', 'universal', '{"description": "Category-level constraints on combination. Determines which syntactic categories can occupy which positions."}'),
('AA','AC','AB','AA','AE', 'valency',            'force_type', 'universal', '{"description": "Argument structure requirements. Determines how many and what type of complements a head requires."}'),
('AA','AC','AB','AA','AF', 'movement',           'force_type', 'universal', '{"description": "Displacement forces. Determines when elements appear in positions different from where they are interpreted."}'),
('AA','AC','AB','AA','AG', 'structural_repair',  'force_type', 'universal', '{"description": "Error correction and recovery forces. Handles ill-formed input, ellipsis, and structural ambiguity resolution."}');

-- ================================================================
-- RELATIONSHIP TYPES (7 tokens) — AA.AC.AB.AB.*
-- ================================================================
-- Universal structural relationships between elements.
-- Force profiles on these are documentation-only for now (deferred per Project Lead).

INSERT INTO tokens (ns, p2, p3, p4, p5, name, category, subcategory, metadata) VALUES
('AA','AC','AB','AB','AA', 'head_complement',    'relationship_type', 'universal', '{"description": "Head selects and licenses complement. Tightest structural bond (high binding energy)."}'),
('AA','AC','AB','AB','AB', 'head_adjunct',        'relationship_type', 'universal', '{"description": "Adjunct modifies head without being selected. Weaker binding than complement."}'),
('AA','AC','AB','AB','AC', 'subject_predicate',   'relationship_type', 'universal', '{"description": "External argument (subject) relates to predicate. Defines primary directionality plane."}'),
('AA','AC','AB','AB','AD', 'determiner_nominal',  'relationship_type', 'universal', '{"description": "Determiner specifies nominal. Unique structural relationship with specific binding properties."}'),
('AA','AC','AB','AB','AE', 'coordination',        'relationship_type', 'universal', '{"description": "Parallel elements joined by coordinator. Symmetric binding between conjuncts."}'),
('AA','AC','AB','AB','AF', 'movement_trace',      'relationship_type', 'universal', '{"description": "Displaced element linked to interpretation site. Movement force creates this relationship."}'),
('AA','AC','AB','AB','AG', 'coreference',         'relationship_type', 'universal', '{"description": "Identity relationship between expressions referring to the same entity. Binding theory constraints apply."}');

-- ================================================================
-- LOD LEVELS (8 tokens) — AA.AC.AB.AC.*
-- ================================================================
-- Level-of-detail hierarchy. Forces are LoD-relative and aggregate upward.
-- The lod_transitions table (below) encodes the aggregation chain.

INSERT INTO tokens (ns, p2, p3, p4, p5, name, category, subcategory, metadata) VALUES
('AA','AC','AB','AC','AA', 'byte',       'lod_level', 'universal', '{"level": 0, "description": "Raw byte encoding. Lowest LoD — character encoding, Unicode points."}'),
('AA','AC','AB','AC','AB', 'character',   'lod_level', 'universal', '{"level": 1, "description": "Individual characters/graphemes. Spelling, orthographic patterns."}'),
('AA','AC','AB','AC','AC', 'morpheme',    'lod_level', 'universal', '{"level": 2, "description": "Minimal meaningful units. Prefixes, roots, suffixes, inflections."}'),
('AA','AC','AB','AC','AD', 'word',        'lod_level', 'universal', '{"level": 3, "description": "Lexical items. Primary level for sub-categorization and selectional forces."}'),
('AA','AC','AB','AC','AE', 'phrase',      'lod_level', 'universal', '{"level": 4, "description": "Headed phrases (NP, VP, etc.). Forces aggregate from word-level constituents."}'),
('AA','AC','AB','AC','AF', 'clause',      'lod_level', 'universal', '{"level": 5, "description": "Clausal units (S, CP, IP). Subject-predicate relationship defines this level."}'),
('AA','AC','AB','AC','AG', 'sentence',    'lod_level', 'universal', '{"level": 6, "description": "Complete sentences. Top-level structural unit for single utterances."}'),
('AA','AC','AB','AC','AH', 'discourse',   'lod_level', 'universal', '{"level": 7, "description": "Multi-sentence text. Coreference, topic continuity, pragmatic forces operate here."}');

-- ================================================================
-- UNIVERSAL STRUCTURAL PRINCIPLES — AA.AC.AB.AD.*
-- ================================================================
-- Cross-linguistic structural rules that apply regardless of language.

INSERT INTO tokens (ns, p2, p3, p4, p5, name, category, subcategory, metadata) VALUES
('AA','AC','AB','AD','AA', 'phrasal_category_from_head',  'structural_principle', 'universal', '{"description": "The category of a phrase is determined by the category of its head. NP has N head, VP has V head, etc. Universal across human languages."}'),
('AA','AC','AB','AD','AB', 'binary_branching',            'structural_principle', 'universal', '{"description": "Structural combinations are maximally binary. Complex structures built by successive binary merges."}'),
('AA','AC','AB','AD','AC', 'endocentricity',              'structural_principle', 'universal', '{"description": "Every phrase has a head that determines its distribution and semantic type. Phrases inherit properties from their heads."}');

-- ================================================================
-- LOD TRANSITIONS TABLE
-- ================================================================
-- Encodes the aggregation chain: forces at lower LoD aggregate into
-- forces at the next higher LoD. 7 transitions for 8 levels.

CREATE TABLE IF NOT EXISTS lod_transitions (
    -- Lower LoD level (decomposed reference to AA.AC.AB.AC token)
    from_ns TEXT NOT NULL, from_p2 TEXT, from_p3 TEXT, from_p4 TEXT, from_p5 TEXT,
    from_token TEXT GENERATED ALWAYS AS (
        from_ns ||
        COALESCE('.' || from_p2, '') ||
        COALESCE('.' || from_p3, '') ||
        COALESCE('.' || from_p4, '') ||
        COALESCE('.' || from_p5, '')
    ) STORED NOT NULL,

    -- Higher LoD level (decomposed reference to AA.AC.AB.AC token)
    to_ns TEXT NOT NULL, to_p2 TEXT, to_p3 TEXT, to_p4 TEXT, to_p5 TEXT,
    to_token TEXT GENERATED ALWAYS AS (
        to_ns ||
        COALESCE('.' || to_p2, '') ||
        COALESCE('.' || to_p3, '') ||
        COALESCE('.' || to_p4, '') ||
        COALESCE('.' || to_p5, '')
    ) STORED NOT NULL,

    PRIMARY KEY (from_ns, from_p2, from_p3, from_p4, from_p5)
);

CREATE INDEX idx_lod_transitions_from ON lod_transitions(from_ns, from_p2, from_p3, from_p4, from_p5);
CREATE INDEX idx_lod_transitions_to   ON lod_transitions(to_ns, to_p2, to_p3, to_p4, to_p5);

INSERT INTO lod_transitions (from_ns, from_p2, from_p3, from_p4, from_p5, to_ns, to_p2, to_p3, to_p4, to_p5) VALUES
('AA','AC','AB','AC','AA', 'AA','AC','AB','AC','AB'),  -- byte → character
('AA','AC','AB','AC','AB', 'AA','AC','AB','AC','AC'),  -- character → morpheme
('AA','AC','AB','AC','AC', 'AA','AC','AB','AC','AD'),  -- morpheme → word
('AA','AC','AB','AC','AD', 'AA','AC','AB','AC','AE'),  -- word → phrase
('AA','AC','AB','AC','AE', 'AA','AC','AB','AC','AF'),  -- phrase → clause
('AA','AC','AB','AC','AF', 'AA','AC','AB','AC','AG'),  -- clause → sentence
('AA','AC','AB','AC','AG', 'AA','AC','AB','AC','AH');  -- sentence → discourse

COMMIT;

-- ================================================================
-- VERIFICATION
-- ================================================================

\echo ''
\echo '=== Force infrastructure tokens ==='
SELECT category, COUNT(*) AS count
FROM tokens
WHERE ns = 'AA' AND p2 = 'AC' AND p3 = 'AB'
GROUP BY category ORDER BY category;

\echo ''
\echo '=== Token listing ==='
SELECT token_id, name, category
FROM tokens
WHERE ns = 'AA' AND p2 = 'AC' AND p3 = 'AB'
ORDER BY token_id;

\echo ''
\echo '=== LoD transitions ==='
SELECT from_token, '→' AS dir, to_token
FROM lod_transitions
ORDER BY from_token;

\echo ''
\echo '=== Total AA.AC token count ==='
SELECT COUNT(*) AS total FROM tokens WHERE ns = 'AA' AND p2 = 'AC';
