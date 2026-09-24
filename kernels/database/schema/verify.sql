-- hcp3_core schema verification
--
-- Read-only. Every query inspects information_schema / pg_catalog only —
-- nothing here mutates data or structure. Run against hcp3_core after
-- schema.sql has been applied. Each query is preceded by a comment stating
-- the expected result; see tests.md for the checklist form of the same.


-- ============================================================================
-- 1. Tables exist, with the right kind.
-- ============================================================================
-- Expect exactly these 5 rows, ALL as BASE TABLE:
--   member_of   | BASE TABLE
--   members     | BASE TABLE
--   token       | BASE TABLE
--   token_child | BASE TABLE
--   token_parent| BASE TABLE
-- (token_sibling_group and token.type are gone — see schema.sql header.)
SELECT table_name, table_type
FROM information_schema.tables
WHERE table_schema = 'public'
  AND table_name IN (
      'token', 'token_parent', 'token_child', 'members', 'member_of'
  )
ORDER BY table_name;

-- Expect 0 rows: token_sibling_group must not exist any more.
SELECT table_name
FROM information_schema.tables
WHERE table_schema = 'public' AND table_name = 'token_sibling_group';


-- ============================================================================
-- 2. token — columns and types.
-- ============================================================================
-- Expect 4 rows (no `type` column any more):
--   token_id   | ARRAY (element type text) | NOT NULL
--   token_text | text                       | nullable
--   notation   | text                       | nullable
--   mass       | integer                    | nullable
SELECT column_name, data_type, is_nullable
FROM information_schema.columns
WHERE table_schema = 'public' AND table_name = 'token'
ORDER BY ordinal_position;

-- Expect 0 rows: the `type` column is dropped.
SELECT column_name
FROM information_schema.columns
WHERE table_schema = 'public' AND table_name = 'token' AND column_name = 'type';

-- Expect: token_id is PRIMARY KEY.
SELECT tc.constraint_type, kcu.column_name
FROM information_schema.table_constraints tc
JOIN information_schema.key_column_usage kcu
  ON tc.constraint_name = kcu.constraint_name AND tc.table_schema = kcu.table_schema
WHERE tc.table_schema = 'public' AND tc.table_name = 'token' AND tc.constraint_type = 'PRIMARY KEY';

-- Expect: one CHECK constraint (token_id_nonempty) referencing token.
SELECT constraint_name
FROM information_schema.table_constraints
WHERE table_schema = 'public' AND table_name = 'token' AND constraint_type = 'CHECK'
  AND constraint_name = 'token_id_nonempty';


-- ============================================================================
-- 3. token_parent — columns, PK, FKs.
-- ============================================================================
-- Expect 4 columns: token_id (ARRAY, NOT NULL), ordinal (integer, NOT NULL),
-- parent_token_id (ARRAY, NOT NULL), mass (integer, NULLABLE — changed from
-- NOT NULL: blank at declare, filled later; NULL != a real 0).
SELECT column_name, data_type, is_nullable
FROM information_schema.columns
WHERE table_schema = 'public' AND table_name = 'token_parent'
ORDER BY ordinal_position;

-- Expect: PRIMARY KEY on (token_id, ordinal) — 2 rows.
SELECT kcu.column_name
FROM information_schema.table_constraints tc
JOIN information_schema.key_column_usage kcu
  ON tc.constraint_name = kcu.constraint_name AND tc.table_schema = kcu.table_schema
WHERE tc.table_schema = 'public' AND tc.table_name = 'token_parent' AND tc.constraint_type = 'PRIMARY KEY'
ORDER BY kcu.ordinal_position;

-- Expect: 2 FOREIGN KEY constraints, both referencing token(token_id)
-- (token_id -> token, parent_token_id -> token).
SELECT
    kcu.column_name AS fk_column,
    ccu.table_name  AS ref_table,
    ccu.column_name AS ref_column
FROM information_schema.table_constraints tc
JOIN information_schema.key_column_usage kcu
  ON tc.constraint_name = kcu.constraint_name AND tc.table_schema = kcu.table_schema
JOIN information_schema.constraint_column_usage ccu
  ON tc.constraint_name = ccu.constraint_name AND tc.table_schema = ccu.table_schema
WHERE tc.table_schema = 'public' AND tc.table_name = 'token_parent' AND tc.constraint_type = 'FOREIGN KEY'
ORDER BY kcu.column_name;

-- Expect 0 rows: no secondary index on parent_token_id. Reverse lookups
-- go through the stored token_child table (section 6), never through a
-- reverse-search index on token_parent — see section 9 for the blanket
-- "PK indexes only" assertion across all five tables.
SELECT indexname, indexdef
FROM pg_indexes
WHERE schemaname = 'public' AND tablename = 'token_parent' AND indexname = 'token_parent_parent_token_id_idx';


-- ============================================================================
-- 4. members — columns, PK, FKs (membership, downward: group -> members).
-- ============================================================================
-- Expect 2 columns: token_id, member_token_id (arrays, NOT NULL). No mass,
-- no kind/category column.
SELECT column_name, data_type, is_nullable
FROM information_schema.columns
WHERE table_schema = 'public' AND table_name = 'members'
ORDER BY ordinal_position;

-- Expect: PRIMARY KEY on (token_id, member_token_id) — 2 rows.
SELECT kcu.column_name
FROM information_schema.table_constraints tc
JOIN information_schema.key_column_usage kcu
  ON tc.constraint_name = kcu.constraint_name AND tc.table_schema = kcu.table_schema
WHERE tc.table_schema = 'public' AND tc.table_name = 'members' AND tc.constraint_type = 'PRIMARY KEY'
ORDER BY kcu.ordinal_position;

-- Expect: 2 FOREIGN KEY constraints, both referencing token(token_id).
SELECT
    kcu.column_name AS fk_column,
    ccu.table_name  AS ref_table,
    ccu.column_name AS ref_column
FROM information_schema.table_constraints tc
JOIN information_schema.key_column_usage kcu
  ON tc.constraint_name = kcu.constraint_name AND tc.table_schema = kcu.table_schema
JOIN information_schema.constraint_column_usage ccu
  ON tc.constraint_name = ccu.constraint_name AND tc.table_schema = ccu.table_schema
WHERE tc.table_schema = 'public' AND tc.table_name = 'members' AND tc.constraint_type = 'FOREIGN KEY'
ORDER BY kcu.column_name;


-- ============================================================================
-- 5. member_of — columns, PK, FKs (membership, upward: member -> groups).
-- ============================================================================
-- Expect 2 columns: token_id, group_token_id (arrays, NOT NULL). No mass,
-- no kind/category column.
SELECT column_name, data_type, is_nullable
FROM information_schema.columns
WHERE table_schema = 'public' AND table_name = 'member_of'
ORDER BY ordinal_position;

-- Expect: PRIMARY KEY on (token_id, group_token_id) — 2 rows.
SELECT kcu.column_name
FROM information_schema.table_constraints tc
JOIN information_schema.key_column_usage kcu
  ON tc.constraint_name = kcu.constraint_name AND tc.table_schema = kcu.table_schema
WHERE tc.table_schema = 'public' AND tc.table_name = 'member_of' AND tc.constraint_type = 'PRIMARY KEY'
ORDER BY kcu.ordinal_position;

-- Expect: 2 FOREIGN KEY constraints, both referencing token(token_id).
SELECT
    kcu.column_name AS fk_column,
    ccu.table_name  AS ref_table,
    ccu.column_name AS ref_column
FROM information_schema.table_constraints tc
JOIN information_schema.key_column_usage kcu
  ON tc.constraint_name = kcu.constraint_name AND tc.table_schema = kcu.table_schema
JOIN information_schema.constraint_column_usage ccu
  ON tc.constraint_name = ccu.constraint_name AND tc.table_schema = ccu.table_schema
WHERE tc.table_schema = 'public' AND tc.table_name = 'member_of' AND tc.constraint_type = 'FOREIGN KEY'
ORDER BY kcu.column_name;


-- ============================================================================
-- 6. token_child — materialized reverse adjacency (a real table, not a view).
-- ============================================================================
-- Expect 'r' (ordinary table / relkind 'r'), confirming token_child is a
-- BASE TABLE, not a view ('v').
SELECT c.relkind
FROM pg_catalog.pg_class c
JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace
WHERE n.nspname = 'public' AND c.relname = 'token_child';

-- Expect 2 columns: token_id (ARRAY, NOT NULL), child_token_id (ARRAY, NOT NULL).
SELECT column_name, data_type, is_nullable
FROM information_schema.columns
WHERE table_schema = 'public' AND table_name = 'token_child'
ORDER BY ordinal_position;

-- Expect: PRIMARY KEY on (token_id, child_token_id) — 2 rows.
SELECT kcu.column_name
FROM information_schema.table_constraints tc
JOIN information_schema.key_column_usage kcu
  ON tc.constraint_name = kcu.constraint_name AND tc.table_schema = kcu.table_schema
WHERE tc.table_schema = 'public' AND tc.table_name = 'token_child' AND tc.constraint_type = 'PRIMARY KEY'
ORDER BY kcu.ordinal_position;

-- Expect: 2 FOREIGN KEY constraints, both referencing token(token_id)
-- (token_id -> token, child_token_id -> token).
SELECT
    kcu.column_name AS fk_column,
    ccu.table_name  AS ref_table,
    ccu.column_name AS ref_column
FROM information_schema.table_constraints tc
JOIN information_schema.key_column_usage kcu
  ON tc.constraint_name = kcu.constraint_name AND tc.table_schema = kcu.table_schema
JOIN information_schema.constraint_column_usage ccu
  ON tc.constraint_name = ccu.constraint_name AND tc.table_schema = ccu.table_schema
WHERE tc.table_schema = 'public' AND tc.table_name = 'token_child' AND tc.constraint_type = 'FOREIGN KEY'
ORDER BY kcu.column_name;


-- ============================================================================
-- 7. No unexpected triggers or functions attached to any of these tables.
-- ============================================================================
-- Expect 0 rows: no logic lives in this schema (all logic is the C++ layer).
SELECT trigger_name, event_object_table
FROM information_schema.triggers
WHERE trigger_schema = 'public'
  AND event_object_table IN (
      'token', 'token_parent', 'token_child', 'members', 'member_of'
  );


-- ============================================================================
-- 8. Address columns carry COLLATE "C" (firmed 2026-09-18).
-- ============================================================================
-- Every text[] ADDRESS column (the PK and its four FK counterparts) must be
-- pinned COLLATE "C" so element comparison is byte order = codec::kAlphabet's
-- documented order (A-N,P-Z,a-n,p-z); PK btree order must equal address
-- order for a contiguous trunk/range to be a contiguous PK range (the
-- gather primitive's prerequisite). Expect exactly these 9 rows, all
-- collname 'C':
--   member_of    | token_id        | C
--   member_of    | group_token_id  | C
--   members      | token_id        | C
--   members      | member_token_id | C
--   token        | token_id        | C
--   token_child  | token_id        | C
--   token_child  | child_token_id  | C
--   token_parent | token_id        | C
--   token_parent | parent_token_id | C
SELECT c.relname AS table_name, a.attname AS column_name, coll.collname
FROM pg_attribute a
JOIN pg_class c ON c.oid = a.attrelid
JOIN pg_namespace n ON n.oid = c.relnamespace
JOIN pg_collation coll ON coll.oid = a.attcollation
WHERE n.nspname = 'public'
  AND c.relname IN ('token', 'token_parent', 'token_child', 'members', 'member_of')
  AND a.attnum > 0 AND NOT a.attisdropped
  AND a.attname IN (
      'token_id', 'parent_token_id', 'child_token_id',
      'member_token_id', 'group_token_id'
  )
ORDER BY c.relname, a.attname;

-- Expect 0 rows: token_text is plain debug text, never an address column
-- (not compared/ranged/keyed on) — deliberately left on the database
-- default collation, NOT pinned to "C". This query fails (returns a row)
-- if token_text is ever mistakenly given a non-default collation.
SELECT c.relname, a.attname, coll.collname
FROM pg_attribute a
JOIN pg_class c ON c.oid = a.attrelid
JOIN pg_namespace n ON n.oid = c.relnamespace
JOIN pg_collation coll ON coll.oid = a.attcollation
WHERE n.nspname = 'public' AND c.relname = 'token' AND a.attname = 'token_text'
  AND coll.collname <> 'default';


-- ============================================================================
-- 9. PK indexes only — no reverse-search indexes anywhere in the schema.
-- ============================================================================
-- The CPU only follows stored lists, it never searches; the only allowed
-- indexes are the PK indexes needed to resolve a direct address. Expect
-- every row's indexname to end in "_pkey" (i.e. every index found is a
-- primary-key index, none are secondary/reverse-search indexes). In
-- particular this must show NO token_parent_parent_token_id_idx and no
-- secondary index on members/member_of's second column.
SELECT tablename, indexname
FROM pg_indexes
WHERE schemaname = 'public'
  AND tablename IN (
      'token', 'token_parent', 'token_child', 'members', 'member_of'
  )
ORDER BY tablename, indexname;
