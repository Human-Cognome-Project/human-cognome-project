-- Migration 020: Positional modifiers on token positions
-- Target: hcp_fic_pbm
-- Depends on: 013 (positions on starters)
--
-- Store deltas, not copies. Morph bits and cap flags are positional
-- modifiers in the token position map. Bare tokens (lowercase, uninflected)
-- carry zero overhead — only tokens with modifications store data.
--
-- Encoding: sparse pairs in base-50. Each non-zero modifier is stored as:
--   [position_b50(4 chars) + modifier_b50(4 chars)]
-- Concatenated into a single TEXT column. NULL = no modifiers (bare document).
--
-- modifier value = (morph_bits << 2) | cap_flags
--   cap_flags: bit 0 = first_cap, bit 1 = all_caps (2 bits)
--   morph_bits: 16-bit field:
--     PLURAL(0), POSS(1), POSS_PL(2), PAST(3), PROG(4), 3RD(5),
--     NEG(6), COND(7), WILL(8), HAVE(9), BE(10), AM(11), bits 12-15 reserved
--
-- Max modifier value with 16 morph bits: (0xFFFF << 2) | 0x3 = 262,143
-- Base-50 4-char max: 6,249,999 — headroom to ~20 morph bits before overflow.
--
-- Bonds are always between base forms (uninflected). Modifiers live
-- exclusively in this positional map. All inflections of "walk" collapse
-- to one bond entry with accumulated count; the surface form is
-- reconstructed from position + modifier at read time.

\connect hcp_fic_pbm

ALTER TABLE pbm_starters
    ADD COLUMN IF NOT EXISTS modifiers TEXT;

COMMENT ON COLUMN pbm_starters.modifiers IS
    'Sparse base-50 encoded positional modifiers: [(pos_b50)(mod_b50)]... NULL = bare document';
