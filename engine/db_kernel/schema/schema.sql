-- hcp3_core — cold swarm cache schema
--
-- Passive backing store for the token-graph. Applied against an already
-- created `hcp3_core` database; this file issues no CREATE DATABASE.
--
-- Everything is stored as token_ids, across two orthogonal relationship
-- axes, each a reciprocal pair (see NOTES.md "Relationship model & type —
-- firmed 2026-09-17", which governs on conflict):
--   STRUCTURE (literal composition, ordered):
--     1. parent data       -- ordered constituents, downward -> token_parent
--     2. child expressions -- reverse adjacency, navigation  -> token_child
--   MEMBERSHIP (grouping):
--     3. members           -- what a group contains, downward -> members
--     4. member_of         -- the groups a token joins, upward -> member_of
--
-- Type ('literal' vs 'label') is NOT stored: it is emergent and
-- LoD-relative, read from which axis/table a token has rows in, never a
-- column. There is no equivalence/kind table either — the relationship
-- "kind" is the axis a row sits on, not a stored category.
--
-- No logic lives here beyond plain tables and keys. All derivation,
-- validation and construction logic belongs to the C++ layer, not to
-- triggers or functions in this schema.
--
-- REVERSE-SEARCH INDEXES ARE NOT ALLOWED. The CPU only follows stored
-- lists; it never searches. Groups 2, 3 and 4 (token_child, members,
-- member_of) are therefore materialized, stored tables, not views over
-- token_parent/each other, and their rows are maintained by a later
-- write/mint controller: token_child is wired from token_parent on write
-- ("fold AND wire"); members/member_of are written together, both
-- directions, by a single call (`add_membership`). The redundancy this
-- creates is deliberate; a single sole-owner write controller keeps each
-- reciprocal pair consistent, so it is not normalized away here. The only
-- indexes in this schema are the PK indexes needed to resolve a direct
-- address.
--
-- ============================================================================
-- ADDRESS / TOKEN_ID ENCODING (config, not enforced by CHECK constraints)
-- ============================================================================
-- A token's address is an ordered array of "couplets". Each couplet is two
-- characters drawn from a base-50 alphabet: A-Z, a-z (52 chars) minus the
-- two that are visually ambiguous with zero, `o` and `O` -> 50 chars.
--
-- This alphabet, and the couplet width itself, are PROVISIONAL / config,
-- not schema law: they are documented here and in README.md, not baked
-- into CHECK constraints, so the loader/C++ layer can change them without
-- a migration. token_id is `text[]`, one couplet per array element.
--
-- token_id representation choice: the address array IS the canonical
-- identity (used as PRIMARY KEY / FK target everywhere below). A rendered
-- dot-joined text form is also stored as a plain column for human/debug
-- legibility and is populated by the loading layer, not derived in SQL
-- (see README.md "token_id choice").
--
-- OPEN TEST SLOT: addresses are conceptually "recorded only as the delta,
-- the shared prefix assumed by position" (prefix/delta compression against
-- an implied parent path). This schema stores the FULL address array,
-- lossless and testable. Delta-compressed storage is a later optimization
-- and is intentionally not implemented here.
-- ============================================================================


-- ----------------------------------------------------------------------------
-- token — the token store itself: identity + temporary notation.
-- ----------------------------------------------------------------------------
CREATE TABLE token (
    -- Canonical identity: the token's address, one base-50 couplet per
    -- array element, outermost-to-innermost. This IS the token_id.
    token_id        text[]  NOT NULL PRIMARY KEY,

    -- Human/debug rendering of token_id (e.g. 'AA.AA.AA.AA.AA'), dot-joined.
    -- Populated by the loading layer alongside token_id; not a generated
    -- column, so no derivation logic lives in this schema.
    token_text      text,

    -- TEMPORARY. Best-effort human-readable surface form of what this
    -- token represents (e.g. "0x", "particle-3"). Reference check only,
    -- for verifying composition during construction. WILL BE DROPPED once
    -- construction is verified — do not build anything downstream on it.
    notation        text,

    -- The token's OWN mass. A literal's mass is the sum of its parents'
    -- masses; a label's mass is the centroid of its members' masses. This
    -- is a plainly stored value — this column holds whatever is written and
    -- computes nothing; the aggregation/maintenance logic is a later,
    -- separate step (per "no logic in this schema"). Nullable, with NO
    -- default: base atoms have a mass immediately (e.g. 0x's is 0), and a
    -- stored 0 must stay distinguishable from "not yet computed", which is
    -- NULL — so composites carry NULL until the later logic fills them.
    mass            integer,

    CONSTRAINT token_id_nonempty CHECK (cardinality(token_id) > 0)
);

COMMENT ON TABLE token IS
    'The token store: one row per token_id. Identity only + temporary notation. '
    'Composition, membership and reverse-adjacency live in the other tables.';
COMMENT ON COLUMN token.token_id IS
    'Canonical identity: address as an array of base-50 couplets. See schema header for the alphabet (config, not enforced here).';
COMMENT ON COLUMN token.token_text IS
    'Dot-joined rendering of token_id for human/debug use. Loader-populated, not derived in SQL.';
COMMENT ON COLUMN token.notation IS
    'TEMPORARY: best-effort human-readable surface form, for construction-time verification only. Will be dropped.';
COMMENT ON COLUMN token.mass IS
    'The token''s own mass: a literal''s sum-of-parents, a label''s centroid-of-members. Stored as given, never computed here; nullable with no default so a real 0 (e.g. 0x) stays distinct from NULL "not yet computed".';


-- ----------------------------------------------------------------------------
-- token_parent — parent data: ordered composition, downward.
-- One row per (whole token, position) -> the constituent token_id at that
-- position, plus that element's own mass. ONE level of extension only —
-- not the transitive expansion.
-- ----------------------------------------------------------------------------
CREATE TABLE token_parent (
    -- The composite/whole token this row is a constituent of.
    token_id        text[]  NOT NULL REFERENCES token (token_id),

    -- Position of this constituent in token_id's direct sequence.
    -- OPEN TEST SLOT: ordinal base is 0-indexed here as a documented
    -- convention (matches array-index style used for addresses). Confirm
    -- against the C++ producer's iteration order before data load; flip
    -- to 1-based if that is what the producer emits.
    ordinal         integer NOT NULL,

    -- The constituent token_id at this position.
    parent_token_id text[]  NOT NULL REFERENCES token (token_id),

    -- Mass carried by THIS element/parent at this position (per-element,
    -- not whole-token). Blank at declare time (DECLARE_RECORD carries no
    -- mass); filled later by the manager's deferred aggregation
    -- workstream. Nullable with NO default, same discipline as token.mass:
    -- NULL ("not yet computed") must stay distinguishable from a real 0.
    -- The seed floor (the 16 hex atoms, mass 1; 0x, mass 0) is the only
    -- declared exception. This table stores whatever mass is given; it
    -- never computes it.
    mass            integer,

    PRIMARY KEY (token_id, ordinal)
);

COMMENT ON TABLE token_parent IS
    'Parent data: for each token, its ordered direct constituents (one level of extension only) and each constituent''s own mass.';
COMMENT ON COLUMN token_parent.token_id IS 'The composite/whole token.';
COMMENT ON COLUMN token_parent.ordinal IS 'Position within token_id''s direct constituent sequence (0-based; see OPEN TEST SLOT above).';
COMMENT ON COLUMN token_parent.parent_token_id IS 'The direct constituent token_id occupying this position.';
COMMENT ON COLUMN token_parent.mass IS 'Per-element mass of this parent at this position. Nullable: blank at declare, filled later by the deferred aggregation workstream; NULL != a real 0. Stored as given, never computed here.';

-- No reverse-lookup index here on purpose: reverse "which tokens list this
-- token as a parent" is answered by the stored token_child table below,
-- not by searching token_parent. A secondary index here would be exactly
-- the reverse-search structure this design forbids.


-- ----------------------------------------------------------------------------
-- token_child — child expressions: reverse adjacency, navigation only.
--
-- Materialized, stored table — NOT a view over token_parent. This table's
-- purpose is step-back capability OVER searching: a read follows this
-- stored list directly; it never derives the answer by searching
-- token_parent. This table is the deliberately redundant reverse of
-- token_parent, kept consistent by a single sole-owner write/mint
-- controller (a later piece) that wires a new composite into each of its
-- parents' child-expression lists on write ("fold AND wire"). No position
-- is stored here — an ordinal, if ever needed, is recovered by following
-- into the child's own token_parent row, not by reading this table.
-- ----------------------------------------------------------------------------
CREATE TABLE token_child (
    -- The token being pointed back to (a parent, from token_parent's
    -- point of view).
    token_id       text[] NOT NULL REFERENCES token (token_id),

    -- A token that directly lists token_id as one of its parents.
    child_token_id text[] NOT NULL REFERENCES token (token_id),

    PRIMARY KEY (token_id, child_token_id)
);

COMMENT ON TABLE token_child IS
    'Child expressions: unordered, materialized reverse adjacency. No position or mass stored. Maintained by the write controller ("fold AND wire"), deliberately redundant with token_parent.';
COMMENT ON COLUMN token_child.token_id IS 'The token being pointed back to.';
COMMENT ON COLUMN token_child.child_token_id IS 'A token that directly lists token_id as one of its parents.';


-- ----------------------------------------------------------------------------
-- members — membership, downward: what a group contains.
--
-- Second relationship axis (MEMBERSHIP), orthogonal to structure
-- (token_parent/token_child) above. Reciprocal of member_of below: the
-- pair is written together, both directions, by a single sole-owner write
-- (`add_membership`), the same discipline "fold AND wire" applies to
-- token_child. One-level arrayed follow keyed on the leading column
-- (token_id); PK-only index, no reverse-search index. No mass column, no
-- kind/category column — the relationship "kind" (structure vs
-- membership) is read from which table/axis a row sits on, never stored
-- (see schema header; NOTES.md "Relationship model & type — firmed
-- 2026-09-17", no-equivalence-table rule).
-- ----------------------------------------------------------------------------
CREATE TABLE members (
    -- The group token: a grouping's naming-literal token_id (a grouping
    -- has no address of its own — see NOTES.md label-has-no-own-address).
    token_id        text[]  NOT NULL REFERENCES token (token_id),

    -- A token this group directly contains.
    member_token_id text[]  NOT NULL REFERENCES token (token_id),

    PRIMARY KEY (token_id, member_token_id)
);

COMMENT ON TABLE members IS
    'Membership, downward: what a group (token_id) directly contains. Reciprocal of member_of, written together by add_membership. No mass/kind column — see schema header.';
COMMENT ON COLUMN members.token_id IS 'The group token.';
COMMENT ON COLUMN members.member_token_id IS 'A token this group directly contains.';


-- ----------------------------------------------------------------------------
-- member_of — membership, upward: the groups a token joins.
--
-- Reciprocal of members above. Same reciprocal-pair discipline as
-- token_parent/token_child: both directions are stored so the CPU follows
-- either one directly, never searching either table.
-- ----------------------------------------------------------------------------
CREATE TABLE member_of (
    -- The member token.
    token_id       text[] NOT NULL REFERENCES token (token_id),

    -- A group token_id this token directly belongs to.
    group_token_id text[] NOT NULL REFERENCES token (token_id),

    PRIMARY KEY (token_id, group_token_id)
);

COMMENT ON TABLE member_of IS
    'Membership, upward: the groups (group_token_id) a token (token_id) directly belongs to. Reciprocal of members, written together by add_membership.';
COMMENT ON COLUMN member_of.token_id IS 'The member token.';
COMMENT ON COLUMN member_of.group_token_id IS 'A group token_id this token directly belongs to.';
