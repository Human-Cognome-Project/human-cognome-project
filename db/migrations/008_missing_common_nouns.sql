-- HCP Migration 008: Add missing common nouns (man, sun)
-- Target: hcp_english
--
-- These words appeared thousands of times in Gutenberg texts as unknowns:
--   "man" — 2,164 occurrences
--   "sun" — 196 occurrences
-- Allocated at CA.HY (new p4 block, HX was full at 940 tokens).

BEGIN;

INSERT INTO tokens (ns, p2, p3, p4, p5, name, layer, subcategory, atomization)
VALUES
    ('AB', 'AB', 'CA', 'HY', 'AA', 'man', 'word', 'noun',
     '["AA.AB.AA.AJ.AM", "AA.AB.AA.AJ.AA", "AA.AB.AA.AJ.AN"]'::jsonb),
    ('AB', 'AB', 'CA', 'HY', 'AB', 'sun', 'word', 'noun',
     '["AA.AB.AA.AJ.AT", "AA.AB.AA.AJ.AV", "AA.AB.AA.AJ.AN"]'::jsonb);

COMMIT;
