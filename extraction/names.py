"""PostgreSQL connection for Name Components shard (hcp_names).

The Names shard stores:
- Name component tokens (yA.*) — single words that appear in proper nouns
- Associated Kaikki entry/sense/form/relation data for single-word proper nouns

This is a cross-linguistic shard — name components belong to all language shards.
"""

import json
import psycopg

DB_CONFIG = {
    "dbname": "hcp_names",
    "user": "hcp",
    "password": "hcp_dev",
    "host": "localhost",
    "port": 5432,
}


def connect():
    """Get a connection to the Names shard database."""
    return psycopg.connect(**DB_CONFIG)


def init_schema(conn):
    """Create the Names shard tables if they don't exist."""
    with conn.cursor() as cur:
        cur.execute("""
            -- Name component tokens
            CREATE TABLE IF NOT EXISTS tokens (
                token_id    TEXT PRIMARY KEY,
                name        TEXT NOT NULL,
                atomization JSONB DEFAULT '[]'::jsonb,
                metadata    JSONB DEFAULT '{}'::jsonb
            );

            CREATE INDEX IF NOT EXISTS idx_tokens_name
                ON tokens(name);

            -- Kaikki entry data (for single-word proper nouns)
            CREATE TABLE IF NOT EXISTS entries (
                id              SERIAL PRIMARY KEY,
                word_token      TEXT REFERENCES tokens(token_id),
                pos_token       TEXT,
                etymology_num   INTEGER,
                etymology_tokens TEXT[],
                kaikki_id       INTEGER,
                word            TEXT
            );

            CREATE INDEX IF NOT EXISTS idx_entries_word_token
                ON entries(word_token);
            CREATE INDEX IF NOT EXISTS idx_entries_word
                ON entries(word);

            -- Senses
            CREATE TABLE IF NOT EXISTS senses (
                id              SERIAL PRIMARY KEY,
                entry_id        INTEGER NOT NULL REFERENCES entries(id),
                gloss_tokens    TEXT[],
                tag_tokens      TEXT[]
            );

            CREATE INDEX IF NOT EXISTS idx_senses_entry
                ON senses(entry_id);

            -- Forms
            CREATE TABLE IF NOT EXISTS forms (
                id              SERIAL PRIMARY KEY,
                entry_id        INTEGER NOT NULL REFERENCES entries(id),
                form_token      TEXT,
                tag_tokens      TEXT[],
                form_text       TEXT,
                form_tokens     TEXT[]
            );

            CREATE INDEX IF NOT EXISTS idx_forms_entry
                ON forms(entry_id);

            -- Relations
            CREATE TABLE IF NOT EXISTS relations (
                id              SERIAL PRIMARY KEY,
                entry_id        INTEGER NOT NULL REFERENCES entries(id),
                relation_token  TEXT,
                target_token    TEXT,
                target_word     TEXT,
                tag_tokens      TEXT[]
            );

            CREATE INDEX IF NOT EXISTS idx_relations_entry
                ON relations(entry_id);
        """)
    conn.commit()


def insert_token(cur, token_id: str, name: str, atomization: list = None,
                 metadata: dict = None):
    """Insert a name component token, updating on conflict."""
    cur.execute("""
        INSERT INTO tokens (token_id, name, atomization, metadata)
        VALUES (%s, %s, %s::jsonb, %s::jsonb)
        ON CONFLICT (token_id) DO UPDATE SET
            name = EXCLUDED.name,
            atomization = EXCLUDED.atomization,
            metadata = tokens.metadata || EXCLUDED.metadata
    """, (token_id, name,
          json.dumps(atomization or []),
          json.dumps(metadata or {})))


def dump_sql(conn, output_path: str):
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
