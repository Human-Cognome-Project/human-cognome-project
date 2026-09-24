-- HCP Migration 007a: Namespace allocations for PBM and entity infrastructure
-- Target: hcp_core
-- Depends on: 006 (force patterns)
--
-- Adds:
--   - AA.AE (PBM Structural Markers) category + subcategories
--   - AA.AF (Entity Classification) category + subcategories
--   - Updates y* from retired "Proper Nouns" to "NF People"
--   - Fiction entity namespaces: v*, u*, t*, s*
--   - Non-fiction entity namespaces: w*, x*
--   - Updates z* description for fiction/non-fiction context

BEGIN;

-- ============================================================================
-- 1. AA.AE — PBM Structural Markers
-- ============================================================================

INSERT INTO namespace_allocations (pattern, name, description, alloc_type, parent) VALUES
('AA.AE',    'PBM Structural Tokens',    'Structural marker tokens for PBM document encoding',                'category',    'AA'),
('AA.AE.AA', 'Block-Level Markers',       'Document structure: paragraphs, sections, lists, tables',           'subcategory', 'AA.AE'),
('AA.AE.AB', 'Inline Format Markers',     'Text formatting: bold, italic, underline, etc.',                    'subcategory', 'AA.AE'),
('AA.AE.AC', 'Annotation Markers',        'Anomalies, footnotes, citations, editorial marks',                  'subcategory', 'AA.AE'),
('AA.AE.AD', 'Alignment/Layout Markers',  'Alignment, indentation, and layout intent',                         'subcategory', 'AA.AE'),
('AA.AE.AE', 'Non-Text Content Markers',  'References to images, figures, embedded objects, math',             'subcategory', 'AA.AE');

-- ============================================================================
-- 2. AA.AF — Entity Classification Tokens
-- ============================================================================

INSERT INTO namespace_allocations (pattern, name, description, alloc_type, parent) VALUES
('AA.AF',    'Entity Classification',   'Entity sub-type and relationship type tokens',                       'category',    'AA'),
('AA.AF.AA', 'Person Sub-Types',        'Entity classification: individual, collective, deity, named creature', 'subcategory', 'AA.AF'),
('AA.AF.AB', 'Place Sub-Types',         'Entity classification: settlement, geographic, building, region, etc.', 'subcategory', 'AA.AF'),
('AA.AF.AC', 'Thing Sub-Types',         'Entity classification: object, organization, species, concept, etc.', 'subcategory', 'AA.AF'),
('AA.AF.AD', 'Entity Relationship Types', 'Semantic/narrative relationship types between entities',            'subcategory', 'AA.AF');

-- ============================================================================
-- 3. Fiction/Non-Fiction namespace split
-- ============================================================================

-- Update existing y* (was "Proper Nouns & Abbreviations", now re-allocated)
UPDATE namespace_allocations
SET name = 'NF People',
    description = 'Non-fiction person entities — historical and contemporary'
WHERE pattern = 'y*';

-- Update z* description for clarity in fiction/non-fiction context
UPDATE namespace_allocations
SET description = 'Non-fiction PBMs — factual documents, reference works'
WHERE pattern = 'z*';

-- Non-fiction entity namespaces (x*, w* are new)
INSERT INTO namespace_allocations (pattern, name, description, alloc_type, parent) VALUES
('x*', 'NF Places',  'Non-fiction place entities — geographic and political',    'mode', NULL),
('w*', 'NF Things',  'Non-fiction thing entities — objects, organizations, concepts', 'mode', NULL);

-- Fiction namespaces (all new)
INSERT INTO namespace_allocations (pattern, name, description, alloc_type, parent) VALUES
('v*', 'Fiction PBMs',    'Fiction PBMs — literary works, fictional content',        'mode', NULL),
('u*', 'Fiction People',  'Fiction person entities — characters, mythological figures', 'mode', NULL),
('t*', 'Fiction Places',  'Fiction place entities — invented geographies',            'mode', NULL),
('s*', 'Fiction Things',  'Fiction thing entities — fictional objects, concepts',     'mode', NULL);

-- ============================================================================
-- 4. Shard-level allocations for entity namespaces
-- ============================================================================

-- Fiction shards
INSERT INTO namespace_allocations (pattern, name, description, alloc_type, parent) VALUES
('vA', 'Fiction PBMs (En)',    'Fiction PBMs (English-primary shard)',        'shard', 'v*'),
('uA', 'Fiction People (En)',  'Fiction characters (English-primary shard)',  'shard', 'u*'),
('tA', 'Fiction Places (En)',  'Fiction locations (English-primary shard)',   'shard', 't*'),
('sA', 'Fiction Things (En)',  'Fiction things (English-primary shard)',      'shard', 's*');

-- Non-fiction shards
INSERT INTO namespace_allocations (pattern, name, description, alloc_type, parent) VALUES
('yA', 'NF People (En)',  'Real people (English-primary shard)',  'shard', 'y*'),
('xA', 'NF Places (En)',  'Real places (English-primary shard)',  'shard', 'x*'),
('wA', 'NF Things (En)',  'Real things (English-primary shard)',  'shard', 'w*');

-- ============================================================================
-- Verify
-- ============================================================================

SELECT pattern, name, alloc_type, parent
FROM namespace_allocations
WHERE pattern IN ('AA.AE', 'AA.AF', 'y*', 'x*', 'w*', 'v*', 'u*', 't*', 's*',
                  'yA', 'xA', 'wA', 'vA', 'uA', 'tA', 'sA',
                  'AA.AE.AA', 'AA.AE.AB', 'AA.AE.AC', 'AA.AE.AD', 'AA.AE.AE',
                  'AA.AF.AA', 'AA.AF.AB', 'AA.AF.AC', 'AA.AF.AD')
ORDER BY pattern;

COMMIT;
