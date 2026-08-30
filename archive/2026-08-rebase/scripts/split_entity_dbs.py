#!/usr/bin/env python3
"""
Split entity data from 2 DBs (nf_entities, fic_entities) into 6 DBs:
  hcp_nf_people, hcp_nf_places, hcp_nf_things
  hcp_fic_people, hcp_fic_places, hcp_fic_things

Each new DB gets the same schema. Data migrates by namespace:
  yA → nf_people, xA → nf_places, wA → nf_things
  uA → fic_people, tA → fic_places, sA → fic_things
"""

import json
import psycopg2
import psycopg2.extras
import subprocess


# Tables that contain entity data with ns-prefixed columns
ENTITY_TABLES = [
    ('tokens', 'ns'),
    ('entity_names', 'entity_ns'),
    ('entity_descriptions', 'entity_ns'),
    ('entity_properties', 'entity_ns'),
    ('entity_relationships', 'source_ns'),  # both source and target
    ('entity_appearances', 'entity_ns'),
    ('entity_rights', 'entity_ns'),
]

# nf_entities also has source_* tables (no ns column, keep in nf_things or all)
NF_SOURCE_TABLES = [
    'sources', 'source_editions', 'source_glossary',
    'source_people', 'source_places', 'source_things',
]

SPLITS = {
    'hcp_nf_people': ('hcp_nf_entities', 'yA'),
    'hcp_nf_places': ('hcp_nf_entities', 'xA'),
    'hcp_nf_things': ('hcp_nf_entities', 'wA'),
    'hcp_fic_people': ('hcp_fic_entities', 'uA'),
    'hcp_fic_places': ('hcp_fic_entities', 'tA'),
    'hcp_fic_things': ('hcp_fic_entities', 'sA'),
}


def create_db(dbname):
    """Create database if not exists."""
    conn = psycopg2.connect(host='localhost', user='hcp', password='hcp_dev', dbname='hcp_english')
    conn.autocommit = True
    cur = conn.cursor()
    cur.execute("SELECT 1 FROM pg_database WHERE datname = %s", (dbname,))
    if not cur.fetchone():
        cur.execute(f'CREATE DATABASE "{dbname}" OWNER hcp')
        print(f"  Created DB: {dbname}", flush=True)
    else:
        print(f"  DB exists: {dbname}", flush=True)
    cur.close()
    conn.close()


def apply_schema(dbname, schema_file):
    """Apply schema from dump file."""
    result = subprocess.run(
        ['psql', '-h', 'localhost', '-U', 'hcp', dbname, '-f', schema_file],
        env={'PGPASSWORD': 'hcp_dev', 'PATH': '/usr/bin:/bin'},
        capture_output=True, text=True
    )
    if result.returncode != 0:
        # Ignore "already exists" errors
        errors = [l for l in result.stderr.split('\n') if 'ERROR' in l and 'already exists' not in l]
        if errors:
            print(f"  Schema errors for {dbname}: {errors[:3]}", flush=True)


def migrate_table(src_conn, dst_conn, table, ns_col, ns_value):
    """Copy rows matching namespace from source to destination."""
    src = src_conn.cursor()
    dst = dst_conn.cursor()

    # Get column names (excluding generated columns)
    src.execute(f"""
        SELECT column_name FROM information_schema.columns
        WHERE table_name = '{table}' AND is_generated = 'NEVER'
        ORDER BY ordinal_position
    """)
    cols = [r[0] for r in src.fetchall()]

    if not cols:
        return 0

    col_list = ', '.join(cols)
    placeholders = ', '.join(['%s'] * len(cols))

    src.execute(f"SELECT {col_list} FROM {table} WHERE {ns_col} = %s", (ns_value,))
    rows = src.fetchall()

    if not rows:
        return 0

    # Convert any dict values (JSONB) to Json wrapper
    def fix_row(row):
        return tuple(
            psycopg2.extras.Json(v) if isinstance(v, dict) else v
            for v in row
        )

    batch = []
    for row in rows:
        batch.append(fix_row(row))
        if len(batch) >= 2000:
            dst.executemany(
                f"INSERT INTO {table} ({col_list}) VALUES ({placeholders}) ON CONFLICT DO NOTHING",
                batch
            )
            batch = []

    if batch:
        dst.executemany(
            f"INSERT INTO {table} ({col_list}) VALUES ({placeholders}) ON CONFLICT DO NOTHING",
            batch
        )

    dst_conn.commit()
    src.close()
    dst.close()
    return len(rows)


def main():
    schema_file = '/tmp/entity_schema.sql'

    # Also dump nf schema (has source_* tables)
    print("Dumping schemas...", flush=True)
    subprocess.run(
        ['pg_dump', '-h', 'localhost', '-U', 'hcp', 'hcp_fic_entities',
         '--schema-only', '--no-owner', '--no-privileges', '--no-comments',
         '-f', '/tmp/fic_entity_schema.sql'],
        env={'PGPASSWORD': 'hcp_dev', 'PATH': '/usr/bin:/bin'},
        capture_output=True
    )
    subprocess.run(
        ['pg_dump', '-h', 'localhost', '-U', 'hcp', 'hcp_nf_entities',
         '--schema-only', '--no-owner', '--no-privileges', '--no-comments',
         '-f', '/tmp/nf_entity_schema.sql'],
        env={'PGPASSWORD': 'hcp_dev', 'PATH': '/usr/bin:/bin'},
        capture_output=True
    )

    # Create 6 new DBs
    print("\nCreating databases...", flush=True)
    for dbname in SPLITS:
        create_db(dbname)

    # Apply schemas
    print("\nApplying schemas...", flush=True)
    for dbname, (src_db, ns) in SPLITS.items():
        if 'nf' in dbname:
            apply_schema(dbname, '/tmp/nf_entity_schema.sql')
        else:
            apply_schema(dbname, '/tmp/fic_entity_schema.sql')
        print(f"  Schema applied: {dbname}", flush=True)

    # Migrate data
    print("\nMigrating data...", flush=True)
    for dbname, (src_db, ns) in SPLITS.items():
        print(f"\n  {src_db} ({ns}) → {dbname}", flush=True)

        src_conn = psycopg2.connect(host='localhost', user='hcp', password='hcp_dev', dbname=src_db)
        dst_conn = psycopg2.connect(host='localhost', user='hcp', password='hcp_dev', dbname=dbname)

        for table, ns_col in ENTITY_TABLES:
            # Check if table exists in source
            src_cur = src_conn.cursor()
            src_cur.execute("""
                SELECT EXISTS(SELECT 1 FROM pg_tables WHERE tablename = %s)
            """, (table,))
            if not src_cur.fetchone()[0]:
                continue
            src_cur.close()

            count = migrate_table(src_conn, dst_conn, table, ns_col, ns)
            if count > 0:
                print(f"    {table}: {count} rows", flush=True)

        # For entity_relationships, also copy rows where target_ns matches
        # (relationships can cross namespaces)
        src_cur = src_conn.cursor()
        src_cur.execute("""
            SELECT EXISTS(SELECT 1 FROM pg_tables WHERE tablename = 'entity_relationships')
        """)
        if src_cur.fetchone()[0]:
            src_cur.execute("""
                SELECT column_name FROM information_schema.columns
                WHERE table_name = 'entity_relationships' AND is_generated = 'NEVER'
                ORDER BY ordinal_position
            """)
            cols = [r[0] for r in src_cur.fetchall()]
            col_list = ', '.join(cols)
            placeholders = ', '.join(['%s'] * len(cols))

            src_cur.execute(f"""
                SELECT {col_list} FROM entity_relationships
                WHERE target_ns = %s AND source_ns != %s
            """, (ns, ns))
            cross_rows = src_cur.fetchall()
            if cross_rows:
                dst_cur = dst_conn.cursor()
                dst_cur.executemany(
                    f"INSERT INTO entity_relationships ({col_list}) VALUES ({placeholders}) ON CONFLICT DO NOTHING",
                    cross_rows
                )
                dst_conn.commit()
                print(f"    entity_relationships (cross-ns targets): {len(cross_rows)} rows", flush=True)
                dst_cur.close()
        src_cur.close()

        src_conn.close()
        dst_conn.close()

    # Summary
    print(f"\n{'='*60}", flush=True)
    print("ENTITY DB SPLIT COMPLETE", flush=True)
    print(f"{'='*60}", flush=True)

    for dbname in sorted(SPLITS.keys()):
        conn = psycopg2.connect(host='localhost', user='hcp', password='hcp_dev', dbname=dbname)
        cur = conn.cursor()
        cur.execute("SELECT count(*) FROM tokens")
        count = cur.fetchone()[0]
        print(f"  {dbname}: {count} tokens", flush=True)
        cur.close()
        conn.close()


if __name__ == '__main__':
    main()
