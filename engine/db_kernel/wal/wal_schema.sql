-- wal_manager — WAL manager's own bookkeeping schema
--
-- Applied against an already created `wal_manager` database; this file
-- issues no CREATE DATABASE. This is the WAL manager's OWN Postgres
-- instance — separate from `hcp3_core` and from any language/personality
-- DB it observes. See WAL-PLAN.md and WAL-IMPL-PLAN.md (task W-1).
--
-- The WAL manager is a bookkeeper/observer, never a writer of the primary
-- change: it never touches `hcp3_core`, never reads the command string,
-- and never designs or drives the cache manager (WAL-IMPL-PLAN.md §0).
-- What it DOES write is its own bookkeeping, defined here.
--
-- No logic lives here beyond plain tables and keys — same discipline as
-- `schema/schema.sql`. All derivation (recognition, matching, monitoring)
-- belongs to the C++ layer (wal_recognize / wal_book / wal_ingest /
-- wal_monitor), not to triggers or functions in this schema.
--
-- REVERSE-SEARCH INDEXES ARE NOT ALLOWED. Every access is a bounded PK
-- follow (only-follow, no reverse index, no predicate scan on a non-key
-- column — WAL-PLAN.md §4, WAL-IMPL-PLAN.md Spec 3/§0). The only indexes
-- in this schema are the PK indexes needed to resolve a direct identity.
--
-- ============================================================================
-- obligation — the live open-obligation relation.
--
-- "Open = membership": a row exists IFF the obligation is open. There is
-- NO status/done/seq column — closing an obligation is a PK DELETE of its
-- row, never an UPDATE of a flag (WAL-IMPL-PLAN.md Spec 2, W-1).
--
-- Identity is a FIXED two-slot PK `(kind, addr_a, addr_b)`, mirroring
-- `schema/schema.sql`'s address-column pin (`text[] COLLATE "C"`) so PK
-- btree order equals address order:
--   structure  -> (parent, child)         addr_a = parent,  addr_b = child
--   membership -> (member, group)         addr_a = member,  addr_b = group
--   mass       -> (token_id, sentinel)    addr_a = token_id, addr_b = '{}'
--
-- Because Postgres PK columns are NOT NULL, a mass obligation's absent
-- second axis uses the SENTINEL `'{}'` — an empty, non-null `text[]` — not
-- NULL. This is own-schema latitude (WAL-PLAN.md "Standing"): it keeps
-- mass in this SAME relation, booked with the change's other followup
-- obligations, instead of a separate mass table (Spec 4 / WAL-PLAN.md §5).
-- The mass VALUE itself is untouched by this schema — it stays on the
-- FIXED `hcp3_core` `token.mass` column; this row is only the debt.
-- ============================================================================
CREATE TABLE obligation (
    -- Discriminator for the obligation's axis/kind. Read from which value
    -- is stored here, never inferred — same "kind is not a stored
    -- category, it is the axis a row sits on" discipline as
    -- schema/schema.sql, except here the axis itself IS what must be
    -- named (there is no separate table per axis in this DB), so it is a
    -- plain discriminator column, not a category column layered on top of
    -- an axis-specific table.
    kind    text NOT NULL,

    -- The first identity slot: parent (structure), member (membership),
    -- or token_id (mass). COLLATE "C" -- see schema/schema.sql's address
    -- column header; this keeps PK order equal to address order.
    addr_a  text[] COLLATE "C" NOT NULL,

    -- The second identity slot: child (structure), group (membership), or
    -- the sentinel empty array '{}' (mass — no second axis to identify).
    -- COLLATE "C" -- see above. NEVER NULL: the mass sentinel is the
    -- empty text[] '{}', not NULL, precisely so mass stays a normal row
    -- of this same PK shape instead of needing a nullable second slot.
    addr_b  text[] COLLATE "C" NOT NULL,

    -- Which local instance's report opened this obligation (core / a
    -- language shard / the personality DB). Identifies provenance only;
    -- it is never a filter key for list_open (WAL-IMPL-PLAN.md W-4: a
    -- source-filtered list_open would be a forbidden non-PK scan).
    source  text NOT NULL,

    -- The opening report's LSN on `source`'s own timeline. Per-source
    -- ordering only -- no cross-source global order is stored or implied
    -- (WAL-PLAN.md §4, Spec 6).
    lsn     bigint NOT NULL,

    PRIMARY KEY (kind, addr_a, addr_b),

    CONSTRAINT obligation_kind_valid
        CHECK (kind IN ('structure', 'membership', 'mass'))
);

COMMENT ON TABLE obligation IS
    'Live open-obligation relation. A row exists IFF the obligation is open; '
    'closing is a PK DELETE. No status/done/seq column -- open = membership, '
    'by design (WAL-IMPL-PLAN.md Spec 2).';
COMMENT ON COLUMN obligation.kind IS
    'Axis discriminator: structure | membership | mass. Never inferred -- read directly.';
COMMENT ON COLUMN obligation.addr_a IS
    'First identity slot: parent (structure), member (membership), or token_id (mass).';
COMMENT ON COLUMN obligation.addr_b IS
    'Second identity slot: child (structure), group (membership), or the sentinel '
    'empty text[] ''{}'' (mass -- never NULL; keeps mass in this same relation).';
COMMENT ON COLUMN obligation.source IS
    'Which local instance''s report opened this obligation. Provenance only -- '
    'never a list_open filter key (that would be a forbidden non-PK scan).';
COMMENT ON COLUMN obligation.lsn IS
    'The opening report''s LSN on source''s own timeline. Per-source order only.';

-- No secondary index here on purpose: list_open reads this relation
-- directly (unfiltered, or filtered on a PK-leading-column prefix --
-- WAL-IMPL-PLAN.md W-4); a source-keyed or any other non-PK index would
-- be exactly the reverse-search structure this design forbids.


-- ============================================================================
-- history — append-only record of WAL reports seen.
--
-- ONE row per report (source, lsn): the durable audit trail behind the
-- live `obligation` set. This is where per-source ordering/progress lives
-- -- NOT a second index on `obligation` (WAL-IMPL-PLAN.md W-1). Settlement
-- is at most one identity per report (WAL-PLAN.md §3's per-write report
-- model), so `record_seen` is the SOLE history append: it both records
-- the report and, if that report's write settled an open obligation,
-- names which one. `close()` (W-4) never appends its own row -- doing so
-- would collide on this table's PK.
-- ============================================================================
CREATE TABLE history (
    -- Which local instance this report came from.
    source          text NOT NULL,

    -- That source's own native LSN for this report. Per-source ordering
    -- only; no cross-source global `seq` is stored (WAL-PLAN.md §4).
    lsn             bigint NOT NULL,

    -- The report's decoded footprint, as the dot-joined rendering of each
    -- involved token_id (same rendering convention as `token.token_text`
    -- in schema/schema.sql). Token_ids in one footprint can differ in
    -- couplet depth, so they cannot be packed into a single rectangular
    -- multi-dimensional text[][]; each element here is one token_id's
    -- own dot-joined string, not a raw address array. Debug/audit
    -- legibility only -- never compared, ranged, or keyed on.
    footprint       text[] NOT NULL DEFAULT '{}',

    -- The report's address scope, read from its address range: 'local'
    -- (the personality DB's private range) or 'global' (shared range) --
    -- WAL-PLAN.md §1/§4.
    scope           text NOT NULL,

    -- The settled-identity reference: which open obligation (if any) this
    -- report's write settled, named by the SAME (kind, addr_a, addr_b)
    -- shape as `obligation`'s PK. NULL/empty together when the report
    -- opened work but settled nothing (WAL-IMPL-PLAN.md W-1). This is a
    -- reference by value, not a foreign key: the obligation row named
    -- here has typically already been PK-deleted by the time this report
    -- is recorded (settlement closes it), so it cannot be a live FK
    -- target.
    settled_kind    text,
    settled_addr_a  text[] COLLATE "C",
    settled_addr_b  text[] COLLATE "C",

    PRIMARY KEY (source, lsn),

    CONSTRAINT history_scope_valid
        CHECK (scope IN ('local', 'global')),

    CONSTRAINT history_settled_kind_valid
        CHECK (settled_kind IS NULL OR settled_kind IN ('structure', 'membership', 'mass')),

    -- All-or-nothing: either nothing settled (all three NULL) or a full
    -- settled identity is recorded (all three present). Prevents a
    -- half-recorded settlement.
    CONSTRAINT history_settled_identity_shape
        CHECK (
            (settled_kind IS NULL AND settled_addr_a IS NULL AND settled_addr_b IS NULL)
            OR
            (settled_kind IS NOT NULL AND settled_addr_a IS NOT NULL AND settled_addr_b IS NOT NULL)
        )
);

COMMENT ON TABLE history IS
    'Append-only record of WAL reports seen, one row per (source, lsn). The sole '
    'history append (record_seen); close() never appends here. Settlement is at '
    'most one identity per report (WAL-PLAN.md §3).';
COMMENT ON COLUMN history.source IS 'Which local instance this report came from.';
COMMENT ON COLUMN history.lsn IS 'That source''s own native LSN for this report. Per-source order only.';
COMMENT ON COLUMN history.footprint IS
    'Dot-joined rendering of each token_id in the report''s footprint (audit/debug only, never keyed on).';
COMMENT ON COLUMN history.scope IS 'Address scope read from the report''s address range: local or global.';
COMMENT ON COLUMN history.settled_kind IS
    'Axis of the obligation this report settled, or NULL if it settled nothing.';
COMMENT ON COLUMN history.settled_addr_a IS 'Settled identity''s first slot, or NULL if nothing settled.';
COMMENT ON COLUMN history.settled_addr_b IS 'Settled identity''s second slot, or NULL if nothing settled.';

-- No secondary index here either: history is read by its own PK
-- (source, lsn) for per-source replay/progress; no reverse-search index
-- is introduced for any other access pattern.
