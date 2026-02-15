-- HCP Migration 007b: Insert 91 PBM structural marker tokens
-- Target: hcp_core
-- Depends on: 007a (namespace allocations)
--
-- 91 tokens in the AA.AE namespace:
--   AA.AE.AA — 32 block-level markers
--   AA.AE.AB — 22 inline formatting markers
--   AA.AE.AC — 14 annotation markers
--   AA.AE.AD — 13 alignment/layout markers
--   AA.AE.AE — 10 non-text content markers
--
-- All tokens use 4-pair addressing (p5 = NULL).

BEGIN;

-- ============================================================================
-- Block-level markers (AA.AE.AA) — 32 tokens
-- ============================================================================

INSERT INTO tokens (ns, p2, p3, p4, name, category, subcategory) VALUES
('AA', 'AE', 'AA', 'AA', 'document_start',          'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AB', 'document_end',            'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AC', 'part_break',              'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AD', 'chapter_break',           'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AE', 'section_break',           'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AF', 'subsection_break',        'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AG', 'subsubsection_break',     'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AH', 'minor_break',             'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AI', 'paragraph_start',         'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AJ', 'paragraph_end',           'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AK', 'line_break',              'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AL', 'page_break',              'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AM', 'horizontal_rule',         'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AN', 'block_quote_start',       'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AP', 'block_quote_end',         'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AQ', 'list_ordered_start',      'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AR', 'list_ordered_end',        'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AS', 'list_unordered_start',    'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AT', 'list_unordered_end',      'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AU', 'list_item_start',         'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AV', 'list_item_end',           'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AW', 'table_start',             'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AX', 'table_end',               'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AY', 'table_row_start',         'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'AZ', 'table_row_end',           'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'Aa', 'table_cell_start',        'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'Ab', 'table_cell_end',          'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'Ac', 'table_header_cell_start', 'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'Ad', 'table_header_cell_end',   'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'Ae', 'code_block_start',        'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'Af', 'code_block_end',          'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'Ag', 'title_start',             'pbm_marker', 'block'),
('AA', 'AE', 'AA', 'Ah', 'title_end',               'pbm_marker', 'block');

-- ============================================================================
-- Inline formatting markers (AA.AE.AB) — 22 tokens
-- ============================================================================

INSERT INTO tokens (ns, p2, p3, p4, name, category, subcategory) VALUES
('AA', 'AE', 'AB', 'AA', 'bold_start',          'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AB', 'bold_end',            'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AC', 'italic_start',        'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AD', 'italic_end',          'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AE', 'underline_start',     'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AF', 'underline_end',       'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AG', 'strikethrough_start', 'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AH', 'strikethrough_end',   'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AI', 'superscript_start',   'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AJ', 'superscript_end',     'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AK', 'subscript_start',     'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AL', 'subscript_end',       'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AM', 'all_caps_start',      'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AN', 'all_caps_end',        'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AP', 'small_caps_start',    'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AQ', 'small_caps_end',      'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AR', 'code_inline_start',   'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AS', 'code_inline_end',     'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AT', 'link_start',          'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AU', 'link_end',            'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AV', 'highlight_start',     'pbm_marker', 'inline'),
('AA', 'AE', 'AB', 'AW', 'highlight_end',       'pbm_marker', 'inline');

-- ============================================================================
-- Annotation markers (AA.AE.AC) — 14 tokens
-- ============================================================================

INSERT INTO tokens (ns, p2, p3, p4, name, category, subcategory) VALUES
('AA', 'AE', 'AC', 'AA', 'sic_start',      'pbm_marker', 'annotation'),
('AA', 'AE', 'AC', 'AB', 'sic_end',        'pbm_marker', 'annotation'),
('AA', 'AE', 'AC', 'AC', 'footnote_ref',   'pbm_marker', 'annotation'),
('AA', 'AE', 'AC', 'AD', 'footnote_start', 'pbm_marker', 'annotation'),
('AA', 'AE', 'AC', 'AE', 'footnote_end',   'pbm_marker', 'annotation'),
('AA', 'AE', 'AC', 'AF', 'endnote_ref',    'pbm_marker', 'annotation'),
('AA', 'AE', 'AC', 'AG', 'endnote_start',  'pbm_marker', 'annotation'),
('AA', 'AE', 'AC', 'AH', 'endnote_end',    'pbm_marker', 'annotation'),
('AA', 'AE', 'AC', 'AI', 'aside_start',    'pbm_marker', 'annotation'),
('AA', 'AE', 'AC', 'AJ', 'aside_end',      'pbm_marker', 'annotation'),
('AA', 'AE', 'AC', 'AK', 'redacted',       'pbm_marker', 'annotation'),
('AA', 'AE', 'AC', 'AL', 'tbd',            'pbm_marker', 'annotation'),
('AA', 'AE', 'AC', 'AM', 'citation_start', 'pbm_marker', 'annotation'),
('AA', 'AE', 'AC', 'AN', 'citation_end',   'pbm_marker', 'annotation');

-- ============================================================================
-- Alignment/layout markers (AA.AE.AD) — 13 tokens
-- ============================================================================

INSERT INTO tokens (ns, p2, p3, p4, name, category, subcategory) VALUES
('AA', 'AE', 'AD', 'AA', 'align_left',      'pbm_marker', 'layout'),
('AA', 'AE', 'AD', 'AB', 'align_center',    'pbm_marker', 'layout'),
('AA', 'AE', 'AD', 'AC', 'align_right',     'pbm_marker', 'layout'),
('AA', 'AE', 'AD', 'AD', 'align_justify',   'pbm_marker', 'layout'),
('AA', 'AE', 'AD', 'AE', 'indent_level_1',  'pbm_marker', 'layout'),
('AA', 'AE', 'AD', 'AF', 'indent_level_2',  'pbm_marker', 'layout'),
('AA', 'AE', 'AD', 'AG', 'indent_level_3',  'pbm_marker', 'layout'),
('AA', 'AE', 'AD', 'AH', 'indent_level_4',  'pbm_marker', 'layout'),
('AA', 'AE', 'AD', 'AI', 'indent_level_5',  'pbm_marker', 'layout'),
('AA', 'AE', 'AD', 'AJ', 'indent_level_6',  'pbm_marker', 'layout'),
('AA', 'AE', 'AD', 'AK', 'indent_level_7',  'pbm_marker', 'layout'),
('AA', 'AE', 'AD', 'AL', 'indent_level_8',  'pbm_marker', 'layout'),
('AA', 'AE', 'AD', 'AM', 'hanging_indent',  'pbm_marker', 'layout');

-- ============================================================================
-- Non-text content markers (AA.AE.AE) — 10 tokens
-- ============================================================================

INSERT INTO tokens (ns, p2, p3, p4, name, category, subcategory) VALUES
('AA', 'AE', 'AE', 'AA', 'image_ref',           'pbm_marker', 'non_text'),
('AA', 'AE', 'AE', 'AB', 'figure_start',        'pbm_marker', 'non_text'),
('AA', 'AE', 'AE', 'AC', 'figure_end',          'pbm_marker', 'non_text'),
('AA', 'AE', 'AE', 'AD', 'caption_start',       'pbm_marker', 'non_text'),
('AA', 'AE', 'AE', 'AE', 'caption_end',         'pbm_marker', 'non_text'),
('AA', 'AE', 'AE', 'AF', 'embedded_object_ref', 'pbm_marker', 'non_text'),
('AA', 'AE', 'AE', 'AG', 'math_start',          'pbm_marker', 'non_text'),
('AA', 'AE', 'AE', 'AH', 'math_end',            'pbm_marker', 'non_text'),
('AA', 'AE', 'AE', 'AI', 'audio_ref',           'pbm_marker', 'non_text'),
('AA', 'AE', 'AE', 'AJ', 'video_ref',           'pbm_marker', 'non_text');

-- ============================================================================
-- Verify counts
-- ============================================================================

SELECT subcategory, COUNT(*) as count
FROM tokens
WHERE ns = 'AA' AND p2 = 'AE'
GROUP BY subcategory
ORDER BY subcategory;

-- Expected: block=32, inline=22, annotation=14, layout=13, non_text=10 = 91 total

COMMIT;
