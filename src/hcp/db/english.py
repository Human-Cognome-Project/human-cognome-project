"""PostgreSQL connection for English language shard (hcp_english).

The English shard stores:
- Word tokens (AB.AB.*) with Token IDs and atomization
- Later: definitions, senses, relations encoded as Token ID sequences
"""

import json
import psycopg

DB_CONFIG = {
    "dbname": "hcp_english",
    "user": "hcp",
    "password": "hcp_dev",
    "host": "localhost",
    "port": 5432,
}


def connect():
    """Get a connection to the English shard database."""
    return psycopg.connect(**DB_CONFIG)


def init_schema(conn):
    """Create the English shard tables if they don't exist."""
    with conn.cursor() as cur:
        cur.execute("""
            CREATE TABLE IF NOT EXISTS tokens (
                token_id    TEXT PRIMARY KEY,
                name        TEXT NOT NULL,
                layer       TEXT,
                subcategory TEXT,
                atomization JSONB DEFAULT '[]'::jsonb
            );

            CREATE INDEX IF NOT EXISTS idx_tokens_name
                ON tokens(name);
            CREATE INDEX IF NOT EXISTS idx_tokens_layer
                ON tokens(layer);
        """)
    conn.commit()


def insert_token(cur, token_id: str, name: str, layer: str = None,
                 subcategory: str = None, atomization: list = None):
    """Insert a token, updating on conflict."""
    cur.execute("""
        INSERT INTO tokens (token_id, name, layer, subcategory, atomization)
        VALUES (%s, %s, %s, %s, %s::jsonb)
        ON CONFLICT (token_id) DO UPDATE SET
            name = EXCLUDED.name,
            layer = EXCLUDED.layer,
            subcategory = EXCLUDED.subcategory,
            atomization = EXCLUDED.atomization
    """, (token_id, name, layer, subcategory,
          json.dumps(atomization or [])))


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
