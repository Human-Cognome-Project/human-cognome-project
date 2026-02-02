"""PostgreSQL schema and operations for HCP core database.

The core db stores:
- Tokens: all defined tokens with their addresses and metadata
- PBM entries: pair-bond map records (token0.token1.fbr per scope)
- Scopes: registered scopes (encoding tables, documents, etc.)
"""

import json

import psycopg

DB_CONFIG = {
    "dbname": "hcp_core",
    "user": "hcp",
    "password": "hcp_dev",
    "host": "localhost",
    "port": 5432,
}


def connect():
    """Get a connection to the core database."""
    return psycopg.connect(**DB_CONFIG)


def init_schema(conn):
    """Create the core database tables if they don't exist."""
    with conn.cursor() as cur:
        cur.execute("""
            CREATE TABLE IF NOT EXISTS tokens (
                token_id    TEXT PRIMARY KEY,
                name        TEXT NOT NULL,
                category    TEXT,
                subcategory TEXT,
                metadata    JSONB DEFAULT '{}'::jsonb
            );

            CREATE TABLE IF NOT EXISTS scopes (
                scope_id    TEXT PRIMARY KEY,
                name        TEXT NOT NULL,
                scope_type  TEXT NOT NULL,
                parent_id   TEXT REFERENCES scopes(scope_id),
                metadata    JSONB DEFAULT '{}'::jsonb
            );

            CREATE TABLE IF NOT EXISTS pbm_entries (
                scope_id    TEXT NOT NULL REFERENCES scopes(scope_id),
                token0_id   TEXT NOT NULL,
                token1_id   TEXT NOT NULL,
                fbr         INTEGER NOT NULL DEFAULT 1,
                position    INTEGER,
                PRIMARY KEY (scope_id, token0_id, token1_id)
            );

            CREATE INDEX IF NOT EXISTS idx_pbm_scope
                ON pbm_entries(scope_id);
            CREATE INDEX IF NOT EXISTS idx_pbm_token0
                ON pbm_entries(token0_id);
            CREATE INDEX IF NOT EXISTS idx_pbm_token1
                ON pbm_entries(token1_id);
            CREATE INDEX IF NOT EXISTS idx_tokens_category
                ON tokens(category);
        """)
    conn.commit()


def insert_token(cur, token_id, name, category=None, subcategory=None, metadata=None):
    """Insert a token, updating on conflict."""
    cur.execute("""
        INSERT INTO tokens (token_id, name, category, subcategory, metadata)
        VALUES (%s, %s, %s, %s, %s::jsonb)
        ON CONFLICT (token_id) DO UPDATE SET
            name = EXCLUDED.name,
            category = EXCLUDED.category,
            subcategory = EXCLUDED.subcategory,
            metadata = tokens.metadata || EXCLUDED.metadata
    """, (token_id, name, category, subcategory,
          json.dumps(metadata or {})))


def insert_scope(cur, scope_id, name, scope_type, parent_id=None, metadata=None):
    """Insert a scope, updating on conflict."""
    cur.execute("""
        INSERT INTO scopes (scope_id, name, scope_type, parent_id, metadata)
        VALUES (%s, %s, %s, %s, %s::jsonb)
        ON CONFLICT (scope_id) DO UPDATE SET
            name = EXCLUDED.name,
            scope_type = EXCLUDED.scope_type,
            parent_id = EXCLUDED.parent_id,
            metadata = scopes.metadata || EXCLUDED.metadata
    """, (scope_id, name, scope_type, parent_id,
          json.dumps(metadata or {})))


def insert_pbm_entry(cur, scope_id, token0_id, token1_id, fbr=1, position=None):
    """Insert a PBM entry, incrementing FBR on conflict."""
    cur.execute("""
        INSERT INTO pbm_entries (scope_id, token0_id, token1_id, fbr, position)
        VALUES (%s, %s, %s, %s, %s)
        ON CONFLICT (scope_id, token0_id, token1_id) DO UPDATE SET
            fbr = pbm_entries.fbr + EXCLUDED.fbr
    """, (scope_id, token0_id, token1_id, fbr, position))


def get_pbm(conn, scope_id):
    """Retrieve all PBM entries for a scope."""
    with conn.cursor() as cur:
        cur.execute("""
            SELECT token0_id, token1_id, fbr, position
            FROM pbm_entries
            WHERE scope_id = %s
            ORDER BY position NULLS LAST, token0_id, token1_id
        """, (scope_id,))
        return cur.fetchall()


def get_token(conn, token_id):
    """Retrieve a token by its ID."""
    with conn.cursor() as cur:
        cur.execute("SELECT * FROM tokens WHERE token_id = %s", (token_id,))
        return cur.fetchone()


def dump_sql(conn, output_path):
    """Export the database as a SQL dump file."""
    import subprocess
    result = subprocess.run(
        ["pg_dump", "--clean", "--if-exists", "--no-owner",
         "-d", DB_CONFIG["dbname"],
         "-U", DB_CONFIG["user"],
         "-h", DB_CONFIG["host"],
         "-f", output_path],
        env={"PGPASSWORD": DB_CONFIG["password"]},
        capture_output=True, text=True
    )
    if result.returncode != 0:
        raise RuntimeError(f"pg_dump failed: {result.stderr}")
