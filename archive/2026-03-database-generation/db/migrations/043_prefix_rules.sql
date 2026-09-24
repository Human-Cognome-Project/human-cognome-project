-- Migration 043: Add prefix rule support to inflection_rules
--
-- inflection_rules previously only supported suffix transformations.
-- Prefix rules use the same table; rule_type distinguishes them.
-- Other languages add their own prefix rows — engine is data-driven.
--
-- strip_prefix / add_prefix mirror strip_suffix / add_suffix:
--   analysis  (surface → base): strip strip_prefix from front
--   generation (base → surface): prepend add_prefix
-- condition regex is applied against the BASE (after stripping prefix).

\connect hcp_english

BEGIN;

ALTER TABLE inflection_rules
    ADD COLUMN rule_type    TEXT NOT NULL DEFAULT 'SUFFIX',
    ADD COLUMN strip_prefix TEXT NOT NULL DEFAULT '',
    ADD COLUMN add_prefix   TEXT NOT NULL DEFAULT '';

-- English bound prefix rules
-- condition: applied against the BASE (word after stripping prefix).
-- '.{3,}' = base must be >= 3 chars (avoids nonsense strips on short words).
-- Longer minimum for longer prefixes (anti/non) to be conservative.
INSERT INTO inflection_rules
    (morpheme, priority, rule_type, condition,
     strip_prefix, add_prefix, strip_suffix, add_suffix, description)
VALUES
    ('PFX_NEG',     1, 'PREFIX', '.{3,}', 'un',   'un',   '', '', 'un-  negation:   unhappy→happy, undo→do'),
    ('PFX_ITER',    1, 'PREFIX', '.{3,}', 're',   're',   '', '', 're-  iteration:  redo→do, rewrite→write'),
    ('PFX_PRE',     1, 'PREFIX', '.{3,}', 'pre',  'pre',  '', '', 'pre- before:     prepay→pay, preview→view'),
    ('PFX_MIS',     1, 'PREFIX', '.{3,}', 'mis',  'mis',  '', '', 'mis- wrongly:    misuse→use, mislead→lead'),
    ('PFX_NEG_DIS', 1, 'PREFIX', '.{3,}', 'dis',  'dis',  '', '', 'dis- negation:   disagree→agree, dislike→like'),
    ('PFX_REV',     1, 'PREFIX', '.{3,}', 'de',   'de',   '', '', 'de-  reversal:   defrost→frost, decode→code'),
    ('PFX_NEG_NON', 1, 'PREFIX', '.{4,}', 'non',  'non',  '', '', 'non- negation:   nonsense→sense, nonstop→stop'),
    ('PFX_NEG_IN',  1, 'PREFIX', '.{3,}', 'in',   'in',   '', '', 'in-  negation:   incorrect→correct'),
    ('PFX_NEG_IM',  1, 'PREFIX', '.{3,}', 'im',   'im',   '', '', 'im-  negation:   impossible→possible'),
    ('PFX_NEG_IL',  1, 'PREFIX', '.{3,}', 'il',   'il',   '', '', 'il-  negation:   illegal→legal'),
    ('PFX_NEG_IR',  1, 'PREFIX', '.{3,}', 'ir',   'ir',   '', '', 'ir-  negation:   irregular→regular'),
    ('PFX_ANTI',    1, 'PREFIX', '.{3,}', 'anti', 'anti', '', '', 'anti- against:   antiwar→war, antilock→lock');

COMMIT;
