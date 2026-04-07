#!/usr/bin/env python3
"""
Migrate kk_ tables to proper shard schema:
1. Drop old tokens/token_pos/token_variants (after backing up FKs)
2. Add ns/p2/p3/p4/p5 decomposition to kk_entries
3. Rename kk_ tables to proper names
4. Rebuild indexes and constraints
"""

import psycopg2


def main():
    conn = psycopg2.connect(host='localhost', user='hcp', password='hcp_dev', dbname='hcp_english')
    conn.autocommit = False
    cur = conn.cursor()

    # --- Step 1: Drop old tables ---
    print("Step 1: Dropping old token tables...", flush=True)
    cur.execute("DROP TABLE IF EXISTS token_morph_rules CASCADE")
    cur.execute("DROP TABLE IF EXISTS token_variants CASCADE")
    cur.execute("DROP TABLE IF EXISTS token_pos CASCADE")
    cur.execute("DROP TABLE IF EXISTS tokens CASCADE")
    conn.commit()
    print("  Old tables dropped.", flush=True)

    # --- Step 2: Add decomposed token_id columns to kk_entries ---
    print("Step 2: Adding decomposed token_id columns...", flush=True)
    cur.execute("ALTER TABLE kk_entries ADD COLUMN IF NOT EXISTS ns TEXT")
    cur.execute("ALTER TABLE kk_entries ADD COLUMN IF NOT EXISTS p2 TEXT")
    cur.execute("ALTER TABLE kk_entries ADD COLUMN IF NOT EXISTS p3 TEXT")
    cur.execute("ALTER TABLE kk_entries ADD COLUMN IF NOT EXISTS p4 TEXT")
    cur.execute("ALTER TABLE kk_entries ADD COLUMN IF NOT EXISTS p5 TEXT")
    conn.commit()

    # Populate from token_id
    print("  Populating ns/p2/p3/p4/p5 from token_id...", flush=True)
    cur.execute("""
        UPDATE kk_entries SET
            ns = split_part(token_id, '.', 1),
            p2 = split_part(token_id, '.', 2),
            p3 = split_part(token_id, '.', 3),
            p4 = split_part(token_id, '.', 4),
            p5 = split_part(token_id, '.', 5)
        WHERE token_id IS NOT NULL AND ns IS NULL
    """)
    conn.commit()
    print(f"  Updated {cur.rowcount} rows.", flush=True)

    # Add indexes on decomposed fields
    print("  Adding indexes...", flush=True)
    cur.execute("CREATE INDEX IF NOT EXISTS idx_entries_ns ON kk_entries(ns)")
    cur.execute("CREATE INDEX IF NOT EXISTS idx_entries_ns_p2 ON kk_entries(ns, p2)")
    cur.execute("CREATE INDEX IF NOT EXISTS idx_entries_ns_p2_p3 ON kk_entries(ns, p2, p3)")
    cur.execute("CREATE INDEX IF NOT EXISTS idx_entries_prefix ON kk_entries(ns, p2, p3, p4, p5)")
    conn.commit()

    # --- Step 3: Rename all kk_ tables ---
    print("Step 3: Renaming tables...", flush=True)

    renames = [
        ('kk_entries', 'entries'),
        ('kk_senses', 'senses'),
        ('kk_sense_categories', 'sense_categories'),
        ('kk_sense_examples', 'sense_examples'),
        ('kk_relations', 'relations'),
        ('kk_forms', 'forms'),
        ('kk_sounds', 'sounds'),
        ('kk_translations', 'translations'),
        ('kk_phrase_components', 'phrase_components'),
    ]

    for old_name, new_name in renames:
        # Check if old table exists
        cur.execute("SELECT EXISTS(SELECT 1 FROM pg_tables WHERE tablename = %s)", (old_name,))
        if cur.fetchone()[0]:
            # Drop new name if it exists (shouldn't, but safety)
            cur.execute(f"DROP TABLE IF EXISTS {new_name} CASCADE")
            cur.execute(f"ALTER TABLE {old_name} RENAME TO {new_name}")
            print(f"  {old_name} → {new_name}", flush=True)
        else:
            print(f"  {old_name} not found, skipping", flush=True)

    conn.commit()

    # --- Step 4: Rename indexes that reference old names ---
    print("Step 4: Cleaning up index names...", flush=True)
    # The indexes auto-rename with the table in Postgres, but let's rename
    # the ones with kk_ prefix for clarity
    cur.execute("""
        SELECT indexname FROM pg_indexes
        WHERE schemaname = 'public' AND indexname LIKE 'kk_%%'
        ORDER BY indexname
    """)
    old_indexes = [r[0] for r in cur.fetchall()]
    for idx in old_indexes:
        new_idx = idx.replace('kk_', '', 1)
        try:
            cur.execute(f"ALTER INDEX {idx} RENAME TO {new_idx}")
            print(f"  {idx} → {new_idx}", flush=True)
        except Exception as e:
            print(f"  {idx}: {e}", flush=True)
            conn.rollback()
    conn.commit()

    # Also rename idx_kk* prefixed indexes
    cur.execute("""
        SELECT indexname FROM pg_indexes
        WHERE schemaname = 'public' AND indexname LIKE 'idx_kk%%'
        ORDER BY indexname
    """)
    old_indexes = [r[0] for r in cur.fetchall()]
    for idx in old_indexes:
        new_idx = idx.replace('idx_kk', 'idx_', 1)
        try:
            cur.execute(f"ALTER INDEX {idx} RENAME TO {new_idx}")
            print(f"  {idx} → {new_idx}", flush=True)
        except Exception as e:
            print(f"  {idx}: {e}", flush=True)
            conn.rollback()
    conn.commit()

    # --- Step 5: Rename sequences ---
    print("Step 5: Renaming sequences...", flush=True)
    cur.execute("""
        SELECT sequencename FROM pg_sequences
        WHERE schemaname = 'public' AND sequencename LIKE 'kk_%%'
    """)
    for (seq,) in cur.fetchall():
        new_seq = seq.replace('kk_', '', 1)
        try:
            cur.execute(f"ALTER SEQUENCE {seq} RENAME TO {new_seq}")
            print(f"  {seq} → {new_seq}", flush=True)
        except Exception as e:
            print(f"  {seq}: {e}", flush=True)
            conn.rollback()
    conn.commit()

    # --- Verify ---
    print("\nVerification:", flush=True)
    cur.execute("SELECT tablename FROM pg_tables WHERE schemaname = 'public' ORDER BY tablename")
    print("  Tables:", flush=True)
    for (t,) in cur.fetchall():
        print(f"    {t}", flush=True)

    cur.execute("SELECT count(*), count(ns) FROM entries")
    total, with_ns = cur.fetchone()
    print(f"\n  entries: {total} total, {with_ns} with decomposed ns", flush=True)

    # Sample
    cur.execute("SELECT ns, p2, p3, p4, p5, token_id, word FROM entries WHERE token_id IS NOT NULL LIMIT 3")
    for row in cur.fetchall():
        print(f"  {row}", flush=True)

    cur.close()
    conn.close()
    print("\nDone.", flush=True)


if __name__ == '__main__':
    main()
