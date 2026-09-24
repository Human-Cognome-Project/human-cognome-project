-- wal_manager schema verification
--
-- Read-only. Every query inspects information_schema / pg_catalog only --
-- nothing here mutates data or structure. Run against `wal_manager` after
-- wal_schema.sql has been applied. Each query is preceded by a comment
-- stating the expected result; see wal_schema_tests.md for the checklist
-- form of the same.


-- ============================================================================
-- 1. Tables exist, with the right kind.
-- ============================================================================
-- Expect exactly these 2 rows, both as BASE TABLE:
--   history    | BASE TABLE
--   obligation | BASE TABLE
SELECT table_name, table_type
FROM information_schema.tables
WHERE table_schema = 'public'
  AND table_name IN ('obligation', 'history')
ORDER BY table_name;


-- ============================================================================
-- 2. obligation — columns, PK, kind check.
-- ============================================================================
-- Expect 5 columns, all NOT NULL:
--   kind   | text  | NOT NULL
--   addr_a | ARRAY | NOT NULL
--   addr_b | ARRAY | NOT NULL
--   source | text  | NOT NULL
--   lsn    | bigint| NOT NULL
SELECT column_name, data_type, is_nullable
FROM information_schema.columns
WHERE table_schema = 'public' AND table_name = 'obligation'
ORDER BY ordinal_position;

-- Expect: PRIMARY KEY is exactly (kind, addr_a, addr_b), in that order.
SELECT kcu.column_name
FROM information_schema.table_constraints tc
JOIN information_schema.key_column_usage kcu
  ON tc.constraint_name = kcu.constraint_name AND tc.table_schema = kcu.table_schema
WHERE tc.table_schema = 'public' AND tc.table_name = 'obligation' AND tc.constraint_type = 'PRIMARY KEY'
ORDER BY kcu.ordinal_position;

-- Expect: one CHECK constraint (obligation_kind_valid) exists.
SELECT constraint_name
FROM information_schema.table_constraints
WHERE table_schema = 'public' AND table_name = 'obligation' AND constraint_type = 'CHECK'
  AND constraint_name = 'obligation_kind_valid';

-- Expect 0 rows: no `status`/`done`/`seq` column on obligation (Spec 2 --
-- open = membership; there is no in-row completion flag).
SELECT column_name
FROM information_schema.columns
WHERE table_schema = 'public' AND table_name = 'obligation'
  AND column_name IN ('status', 'done', 'seq');


-- ============================================================================
-- 3. history — columns, PK, scope/settled checks.
-- ============================================================================
-- Expect 7 columns:
--   source         | text  | NOT NULL
--   lsn            | bigint| NOT NULL
--   footprint      | ARRAY | NOT NULL
--   scope          | text  | NOT NULL
--   settled_kind   | text  | nullable
--   settled_addr_a | ARRAY | nullable
--   settled_addr_b | ARRAY | nullable
SELECT column_name, data_type, is_nullable
FROM information_schema.columns
WHERE table_schema = 'public' AND table_name = 'history'
ORDER BY ordinal_position;

-- Expect: PRIMARY KEY is exactly (source, lsn), in that order.
SELECT kcu.column_name
FROM information_schema.table_constraints tc
JOIN information_schema.key_column_usage kcu
  ON tc.constraint_name = kcu.constraint_name AND tc.table_schema = kcu.table_schema
WHERE tc.table_schema = 'public' AND tc.table_name = 'history' AND tc.constraint_type = 'PRIMARY KEY'
ORDER BY kcu.ordinal_position;

-- Expect: 3 CHECK constraints on history (scope, settled_kind, and the
-- all-or-nothing settled-identity shape). Excludes Postgres's own
-- auto-generated NOT NULL pseudo-CHECK constraints (named
-- "<oid>_<n>_not_null" from PG 12+), which are a catalog artifact of
-- each column's NOT NULL, not a constraint this schema declared.
SELECT constraint_name
FROM information_schema.table_constraints
WHERE table_schema = 'public' AND table_name = 'history' AND constraint_type = 'CHECK'
  AND constraint_name NOT LIKE '%\_not\_null' ESCAPE '\'
ORDER BY constraint_name;


-- ============================================================================
-- 4. Address columns carry COLLATE "C".
-- ============================================================================
-- Expect exactly these 4 rows, all collname 'C':
--   history    | settled_addr_a | C
--   history    | settled_addr_b | C
--   obligation | addr_a         | C
--   obligation | addr_b         | C
-- (history.footprint is deliberately NOT in this list -- it is dot-joined
-- debug text, never compared/ranged/keyed on, same discipline as
-- schema/schema.sql's token_text; it stays on the database default
-- collation.)
SELECT c.relname AS table_name, a.attname AS column_name, coll.collname
FROM pg_attribute a
JOIN pg_class c ON c.oid = a.attrelid
JOIN pg_namespace n ON n.oid = c.relnamespace
JOIN pg_collation coll ON coll.oid = a.attcollation
WHERE n.nspname = 'public'
  AND c.relname IN ('obligation', 'history')
  AND a.attnum > 0 AND NOT a.attisdropped
  AND a.attname IN ('addr_a', 'addr_b', 'settled_addr_a', 'settled_addr_b')
ORDER BY c.relname, a.attname;

-- Expect 0 rows: history.footprint must stay on the database default
-- collation, never pinned to "C" (it is not an address column).
SELECT c.relname, a.attname, coll.collname
FROM pg_attribute a
JOIN pg_class c ON c.oid = a.attrelid
JOIN pg_namespace n ON n.oid = c.relnamespace
JOIN pg_collation coll ON coll.oid = a.attcollation
WHERE n.nspname = 'public' AND c.relname = 'history' AND a.attname = 'footprint'
  AND coll.collname <> 'default';


-- ============================================================================
-- 5. Mass sentinel shape -- addr_b is non-null-shaped (never NULL).
-- ============================================================================
-- Expect 0 rows: addr_b (and addr_a) must be declared NOT NULL, so the
-- mass sentinel '{}' -- an empty, non-null text[] -- is the only way to
-- represent "no second axis", never a NULL. (is_nullable already checked
-- in section 2; this re-asserts it narrowly for addr_b, the sentinel-
-- bearing column.)
SELECT column_name
FROM information_schema.columns
WHERE table_schema = 'public' AND table_name = 'obligation'
  AND column_name = 'addr_b' AND is_nullable = 'YES';


-- ============================================================================
-- 6. No triggers/functions.
-- ============================================================================
-- Expect 0 rows: no logic lives in this schema (all derivation is the
-- C++ layer -- wal_recognize / wal_book / wal_ingest / wal_monitor).
SELECT trigger_name, event_object_table
FROM information_schema.triggers
WHERE trigger_schema = 'public'
  AND event_object_table IN ('obligation', 'history');


-- ============================================================================
-- 7. PK indexes only -- no reverse-search indexes anywhere in the schema.
-- ============================================================================
-- Expect every row's indexname to end in "_pkey" -- i.e. every index found
-- is a primary-key index, none are secondary/reverse-search indexes. In
-- particular this must show no source-keyed index on obligation and no
-- secondary index of any kind on history.
SELECT tablename, indexname
FROM pg_indexes
WHERE schemaname = 'public'
  AND tablename IN ('obligation', 'history')
ORDER BY tablename, indexname;
