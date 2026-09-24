-- Migration 050: NSM prime explication table
--
-- Stores the ~65 NSM conceptual atoms as ordered sequences of phase-helper
-- calls (matching HCPPrimePhases.h API). The engine evaluates each prime by
-- folding the ops over a starting phase=0.
--
-- "ops" is a TEXT[] of PascalCase helper names (SetSubstantive, SetEntity,
-- SetIPhase, etc.). Implicit AND = sequential application; ordering is by
-- convention (class → entity → semantic axes), commutative at the bit level.
--
-- "molecule_props" is a JSONB placeholder for molecule-layer content (specific
-- predicate identity, direction/magnitude/position values, attachment sockets).
-- HCPMoleculeProps.h is planned but not defined; JSONB lets the schema
-- accommodate iterative design without re-migration each time the shape
-- shifts. Will normalize to columns once the molecule layer stabilizes.
--
-- "excluded_langs" is the universality-exclusion list — empty = applies
-- universally (NSM working hypothesis). When a language doesn't validate a
-- prime, the language code is added here.
--
-- "status" carries the "highly debated" caveat per-prime without removing
-- entries from the table.
--
-- Apply against: hcp_core.

\set ON_ERROR_STOP on

BEGIN;

CREATE TABLE IF NOT EXISTS nsm_primes (
    name           TEXT        PRIMARY KEY,
    ops            TEXT[]      NOT NULL,
    molecule_props JSONB       NOT NULL DEFAULT '{}'::JSONB,
    excluded_langs TEXT[]      NOT NULL DEFAULT '{}',
    status         TEXT        NOT NULL DEFAULT 'active',
    notes          TEXT,
    CONSTRAINT nsm_primes_status_ck
        CHECK (status IN ('active', 'debated', 'deprecated'))
);

-- Status filter (active vs debated)
CREATE INDEX IF NOT EXISTS nsm_primes_status_idx
    ON nsm_primes(status);

-- Which primes don't apply to language X
CREATE INDEX IF NOT EXISTS nsm_primes_excluded_langs_idx
    ON nsm_primes USING GIN(excluded_langs);

COMMENT ON TABLE nsm_primes IS
    'NSM conceptual atoms (~65 hypothesized universal primes). Each row is a prime '
    'whose explication is the ordered TEXT[] of phase-helper calls in ops, folded '
    'over phase=0 by the engine. molecule_props is the JSONB placeholder for '
    'molecule-layer content (HCPMoleculeProps.h planned). excluded_langs lists '
    'languages for which the prime is not validated (empty = universal-by-default).';

COMMENT ON COLUMN nsm_primes.ops IS
    'Ordered TEXT[] of PascalCase phase-helper names from HCPPrimePhases.h '
    '(SetSubstantive, SetEntity, SetIPhase, etc.). Implicit AND = sequential '
    'application starting from phase=0.';

COMMENT ON COLUMN nsm_primes.molecule_props IS
    'Molecule-layer content (specific predicate identity, axis values, direction, '
    'position, magnitude, attachment-socket specs). JSONB during iteration; '
    'normalize to columns once HCPMoleculeProps.h shape stabilizes.';

COMMENT ON COLUMN nsm_primes.excluded_langs IS
    'Language codes where this prime is NOT validated. Empty = universal-by-default '
    '(NSM working hypothesis). Updated when adding non-English languages.';

COMMIT;
