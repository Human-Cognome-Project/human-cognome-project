-- HCP Migration 007c: Insert entity classification tokens
-- Target: hcp_core
-- Depends on: 007a (namespace allocations)
--
-- Entity sub-type tokens at AA.AF.AA-AC (17 tokens)
-- Entity relationship type tokens at AA.AF.AD (~40 tokens)
-- All tokens use 4-pair addressing (p5 = NULL).

BEGIN;

-- ============================================================================
-- Person sub-types (AA.AF.AA) — 4 tokens
-- ============================================================================

INSERT INTO tokens (ns, p2, p3, p4, name, category, subcategory) VALUES
('AA', 'AF', 'AA', 'AA', 'individual',      'entity_type', 'person'),
('AA', 'AF', 'AA', 'AB', 'collective',      'entity_type', 'person'),
('AA', 'AF', 'AA', 'AC', 'deity',           'entity_type', 'person'),
('AA', 'AF', 'AA', 'AD', 'named_creature',  'entity_type', 'person');

-- ============================================================================
-- Place sub-types (AA.AF.AB) — 6 tokens
-- ============================================================================

INSERT INTO tokens (ns, p2, p3, p4, name, category, subcategory) VALUES
('AA', 'AF', 'AB', 'AA', 'settlement',        'entity_type', 'place'),
('AA', 'AF', 'AB', 'AB', 'geographic_feature', 'entity_type', 'place'),
('AA', 'AF', 'AB', 'AC', 'building',           'entity_type', 'place'),
('AA', 'AF', 'AB', 'AD', 'region',             'entity_type', 'place'),
('AA', 'AF', 'AB', 'AE', 'world',              'entity_type', 'place'),
('AA', 'AF', 'AB', 'AF', 'celestial_body',     'entity_type', 'place');

-- ============================================================================
-- Thing sub-types (AA.AF.AC) — 7 tokens
-- ============================================================================

INSERT INTO tokens (ns, p2, p3, p4, name, category, subcategory) VALUES
('AA', 'AF', 'AC', 'AA', 'object',       'entity_type', 'thing'),
('AA', 'AF', 'AC', 'AB', 'organization', 'entity_type', 'thing'),
('AA', 'AF', 'AC', 'AC', 'species',      'entity_type', 'thing'),
('AA', 'AF', 'AC', 'AD', 'concept',      'entity_type', 'thing'),
('AA', 'AF', 'AC', 'AE', 'event',        'entity_type', 'thing'),
('AA', 'AF', 'AC', 'AF', 'language',     'entity_type', 'thing'),
('AA', 'AF', 'AC', 'AG', 'material',     'entity_type', 'thing');

-- ============================================================================
-- Entity relationship types (AA.AF.AD) — 40 tokens
-- Per entity-db-design.md §3.5.1
-- ============================================================================

-- Person <-> Person relationships
INSERT INTO tokens (ns, p2, p3, p4, name, category, subcategory) VALUES
('AA', 'AF', 'AD', 'AA', 'parent_of',    'entity_rel', 'person_person'),
('AA', 'AF', 'AD', 'AB', 'child_of',     'entity_rel', 'person_person'),
('AA', 'AF', 'AD', 'AC', 'sibling_of',   'entity_rel', 'person_person'),
('AA', 'AF', 'AD', 'AD', 'spouse_of',    'entity_rel', 'person_person'),
('AA', 'AF', 'AD', 'AE', 'betrothed_to', 'entity_rel', 'person_person'),
('AA', 'AF', 'AD', 'AF', 'mentor_of',    'entity_rel', 'person_person'),
('AA', 'AF', 'AD', 'AG', 'student_of',   'entity_rel', 'person_person'),
('AA', 'AF', 'AD', 'AH', 'ally_of',      'entity_rel', 'person_person'),
('AA', 'AF', 'AD', 'AI', 'enemy_of',     'entity_rel', 'person_person'),
('AA', 'AF', 'AD', 'AJ', 'rival_of',     'entity_rel', 'person_person'),
('AA', 'AF', 'AD', 'AK', 'serves',       'entity_rel', 'person_person'),
('AA', 'AF', 'AD', 'AL', 'served_by',    'entity_rel', 'person_person'),
('AA', 'AF', 'AD', 'AM', 'created_by',   'entity_rel', 'person_person'),
('AA', 'AF', 'AD', 'AN', 'killed_by',    'entity_rel', 'person_person'),
('AA', 'AF', 'AD', 'AP', 'killed',       'entity_rel', 'person_person');

-- Person <-> Place relationships
INSERT INTO tokens (ns, p2, p3, p4, name, category, subcategory) VALUES
('AA', 'AF', 'AD', 'AQ', 'born_in',        'entity_rel', 'person_place'),
('AA', 'AF', 'AD', 'AR', 'died_in',        'entity_rel', 'person_place'),
('AA', 'AF', 'AD', 'AS', 'lives_in',       'entity_rel', 'person_place'),
('AA', 'AF', 'AD', 'AT', 'rules',          'entity_rel', 'person_place'),
('AA', 'AF', 'AD', 'AU', 'founded',        'entity_rel', 'person_place'),
('AA', 'AF', 'AD', 'AV', 'visited',        'entity_rel', 'person_place'),
('AA', 'AF', 'AD', 'AW', 'imprisoned_in',  'entity_rel', 'person_place');

-- Person <-> Thing relationships
INSERT INTO tokens (ns, p2, p3, p4, name, category, subcategory) VALUES
('AA', 'AF', 'AD', 'AX', 'member_of',           'entity_rel', 'person_thing'),
('AA', 'AF', 'AD', 'AY', 'leader_of',           'entity_rel', 'person_thing'),
('AA', 'AF', 'AD', 'AZ', 'possesses',           'entity_rel', 'person_thing'),
('AA', 'AF', 'AD', 'Aa', 'created',             'entity_rel', 'person_thing'),
('AA', 'AF', 'AD', 'Ab', 'belongs_to_species',  'entity_rel', 'person_thing'),
('AA', 'AF', 'AD', 'Ac', 'speaks',              'entity_rel', 'person_thing'),
('AA', 'AF', 'AD', 'Ad', 'participated_in',     'entity_rel', 'person_thing');

-- Place <-> Place relationships
INSERT INTO tokens (ns, p2, p3, p4, name, category, subcategory) VALUES
('AA', 'AF', 'AD', 'Ae', 'contains',      'entity_rel', 'place_place'),
('AA', 'AF', 'AD', 'Af', 'contained_by',  'entity_rel', 'place_place'),
('AA', 'AF', 'AD', 'Ag', 'borders',       'entity_rel', 'place_place'),
('AA', 'AF', 'AD', 'Ah', 'near',          'entity_rel', 'place_place'),
('AA', 'AF', 'AD', 'Ai', 'capital_of',    'entity_rel', 'place_place');

-- Place <-> Thing relationships
INSERT INTO tokens (ns, p2, p3, p4, name, category, subcategory) VALUES
('AA', 'AF', 'AD', 'Aj', 'located_in',  'entity_rel', 'place_thing'),
('AA', 'AF', 'AD', 'Ak', 'occurred_at', 'entity_rel', 'place_thing'),
('AA', 'AF', 'AD', 'Al', 'source_of',   'entity_rel', 'place_thing');

-- Thing <-> Thing relationships
INSERT INTO tokens (ns, p2, p3, p4, name, category, subcategory) VALUES
('AA', 'AF', 'AD', 'Am', 'part_of',       'entity_rel', 'thing_thing'),
('AA', 'AF', 'AD', 'An', 'allied_with',   'entity_rel', 'thing_thing'),
('AA', 'AF', 'AD', 'Ap', 'opposed_to',    'entity_rel', 'thing_thing'),
('AA', 'AF', 'AD', 'Aq', 'derived_from',  'entity_rel', 'thing_thing'),
('AA', 'AF', 'AD', 'Ar', 'used_in',       'entity_rel', 'thing_thing'),
('AA', 'AF', 'AD', 'As', 'related_to',    'entity_rel', 'thing_thing');

-- ============================================================================
-- Verify counts
-- ============================================================================

SELECT subcategory, COUNT(*) as count
FROM tokens
WHERE ns = 'AA' AND p2 = 'AF'
GROUP BY subcategory
ORDER BY subcategory;

-- Expected: person=4, place=6, thing=7, person_person=15, person_place=7,
--           person_thing=7, place_place=5, place_thing=3, thing_thing=6
-- Total: 17 entity sub-types + 43 relationship types = 60 tokens

COMMIT;
