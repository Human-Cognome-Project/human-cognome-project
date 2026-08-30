"""PostgreSQL schema and loader for Kaikki dictionary data.

Kaikki provides Wiktionary extracts as JSONL files. This module loads them
into PostgreSQL with indexed fields for common queries while preserving
the full JSON for flexibility.
"""

import json
from pathlib import Path

from .postgres import DB_CONFIG
import psycopg


def get_kaikki_connection():
    """Get a connection to the database for kaikki operations."""
    return psycopg.connect(**DB_CONFIG)


def init_kaikki_schema(conn):
    """Create the kaikki tables if they don't exist."""
    with conn.cursor() as cur:
        cur.execute("""
            -- Main entries table: one row per word+pos+etymology combination
            CREATE TABLE IF NOT EXISTS kaikki_entries (
                id              SERIAL PRIMARY KEY,
                word            TEXT NOT NULL,
                pos             TEXT NOT NULL,
                lang_code       TEXT NOT NULL DEFAULT 'en',
                etymology_num   INTEGER,
                etymology_text  TEXT,
                data            JSONB NOT NULL
            );

            -- Indexes for common lookups
            CREATE INDEX IF NOT EXISTS idx_kaikki_word
                ON kaikki_entries(word);
            CREATE INDEX IF NOT EXISTS idx_kaikki_word_lower
                ON kaikki_entries(LOWER(word));
            CREATE INDEX IF NOT EXISTS idx_kaikki_pos
                ON kaikki_entries(pos);
            CREATE INDEX IF NOT EXISTS idx_kaikki_lang
                ON kaikki_entries(lang_code);
            CREATE INDEX IF NOT EXISTS idx_kaikki_word_pos
                ON kaikki_entries(word, pos);

            -- GIN index for JSONB queries
            CREATE INDEX IF NOT EXISTS idx_kaikki_data
                ON kaikki_entries USING GIN (data);

            -- Senses table for quick definition lookups
            CREATE TABLE IF NOT EXISTS kaikki_senses (
                id              SERIAL PRIMARY KEY,
                entry_id        INTEGER NOT NULL REFERENCES kaikki_entries(id),
                gloss           TEXT,
                tags            TEXT[],
                data            JSONB
            );

            CREATE INDEX IF NOT EXISTS idx_kaikki_senses_entry
                ON kaikki_senses(entry_id);
            CREATE INDEX IF NOT EXISTS idx_kaikki_senses_gloss
                ON kaikki_senses USING GIN (to_tsvector('english', COALESCE(gloss, '')));

            -- Forms table for inflection lookups
            CREATE TABLE IF NOT EXISTS kaikki_forms (
                id              SERIAL PRIMARY KEY,
                entry_id        INTEGER NOT NULL REFERENCES kaikki_entries(id),
                form            TEXT NOT NULL,
                tags            TEXT[]
            );

            CREATE INDEX IF NOT EXISTS idx_kaikki_forms_entry
                ON kaikki_forms(entry_id);
            CREATE INDEX IF NOT EXISTS idx_kaikki_forms_form
                ON kaikki_forms(form);
            CREATE INDEX IF NOT EXISTS idx_kaikki_forms_form_lower
                ON kaikki_forms(LOWER(form));

            -- Relations table for semantic links (synonyms, antonyms, etc.)
            CREATE TABLE IF NOT EXISTS kaikki_relations (
                id              SERIAL PRIMARY KEY,
                entry_id        INTEGER NOT NULL REFERENCES kaikki_entries(id),
                relation_type   TEXT NOT NULL,
                target_word     TEXT NOT NULL,
                tags            TEXT[],
                data            JSONB
            );

            CREATE INDEX IF NOT EXISTS idx_kaikki_relations_entry
                ON kaikki_relations(entry_id);
            CREATE INDEX IF NOT EXISTS idx_kaikki_relations_type
                ON kaikki_relations(relation_type);
            CREATE INDEX IF NOT EXISTS idx_kaikki_relations_target
                ON kaikki_relations(target_word);
        """)
    conn.commit()


def load_kaikki_jsonl(conn, filepath: Path, lang_code: str = 'en',
                       batch_size: int = 1000, limit: int = None):
    """Load a Kaikki JSONL file into the database.

    Args:
        conn: Database connection
        filepath: Path to the .jsonl file
        lang_code: Language code (e.g., 'en', 'es')
        batch_size: Number of entries to insert per transaction
        limit: Optional limit on number of entries to load (for testing)

    Returns:
        Dict with counts of inserted records
    """
    counts = {'entries': 0, 'senses': 0, 'forms': 0, 'relations': 0}

    # Relation types to extract
    relation_types = [
        'synonyms', 'antonyms', 'hypernyms', 'hyponyms',
        'meronyms', 'coordinate_terms', 'related', 'derived'
    ]

    entries_batch = []

    with open(filepath, 'r', encoding='utf-8') as f:
        for line_num, line in enumerate(f, 1):
            if limit and counts['entries'] >= limit:
                break

            try:
                entry = json.loads(line)
            except json.JSONDecodeError as e:
                print(f"  Skipping line {line_num}: {e}")
                continue

            word = entry.get('word', '')
            pos = entry.get('pos', '')

            if not word or not pos:
                continue

            entries_batch.append({
                'word': word,
                'pos': pos,
                'lang_code': lang_code,
                'etymology_num': entry.get('etymology_number'),
                'etymology_text': entry.get('etymology_text'),
                'data': entry,
                'senses': entry.get('senses', []),
                'forms': entry.get('forms', []),
                'relations': {rt: entry.get(rt, []) for rt in relation_types}
            })

            if len(entries_batch) >= batch_size:
                _insert_batch(conn, entries_batch, counts)
                entries_batch = []

                if counts['entries'] % 10000 == 0:
                    print(f"  Loaded {counts['entries']:,} entries...")

    # Insert remaining
    if entries_batch:
        _insert_batch(conn, entries_batch, counts)

    return counts


def _insert_batch(conn, batch: list, counts: dict):
    """Insert a batch of entries with their related data."""
    with conn.cursor() as cur:
        for item in batch:
            # Insert main entry
            cur.execute("""
                INSERT INTO kaikki_entries
                    (word, pos, lang_code, etymology_num, etymology_text, data)
                VALUES (%s, %s, %s, %s, %s, %s::jsonb)
                RETURNING id
            """, (
                item['word'], item['pos'], item['lang_code'],
                item['etymology_num'], item['etymology_text'],
                json.dumps(item['data'])
            ))
            entry_id = cur.fetchone()[0]
            counts['entries'] += 1

            # Insert senses
            for sense in item['senses']:
                glosses = sense.get('glosses', [])
                gloss = glosses[0] if glosses else None
                tags = sense.get('tags', [])

                cur.execute("""
                    INSERT INTO kaikki_senses (entry_id, gloss, tags, data)
                    VALUES (%s, %s, %s, %s::jsonb)
                """, (entry_id, gloss, tags, json.dumps(sense)))
                counts['senses'] += 1

            # Insert forms
            for form in item['forms']:
                form_text = form.get('form', '')
                if form_text:
                    tags = form.get('tags', [])
                    cur.execute("""
                        INSERT INTO kaikki_forms (entry_id, form, tags)
                        VALUES (%s, %s, %s)
                    """, (entry_id, form_text, tags))
                    counts['forms'] += 1

            # Insert relations
            for rel_type, relations in item['relations'].items():
                for rel in relations:
                    target = rel.get('word', '')
                    if target:
                        tags = rel.get('tags', [])
                        cur.execute("""
                            INSERT INTO kaikki_relations
                                (entry_id, relation_type, target_word, tags, data)
                            VALUES (%s, %s, %s, %s, %s::jsonb)
                        """, (entry_id, rel_type, target, tags, json.dumps(rel)))
                        counts['relations'] += 1

    conn.commit()


def run(language: str = 'english', limit: int = None):
    """Load a Kaikki language file into the database."""
    source_dir = Path(__file__).parent.parent.parent.parent / "sources" / "data" / "kaikki"
    filepath = source_dir / f"{language}.jsonl"

    if not filepath.exists():
        print(f"File not found: {filepath}")
        return

    # Extract lang code from filename or use mapping
    lang_codes = {
        'english': 'en', 'german': 'de', 'spanish': 'es', 'french': 'fr',
        'dutch': 'nl', 'portuguese': 'pt', 'polish': 'pl', 'swedish': 'sv',
        'chinese': 'zh', 'turkish': 'tr', 'czech': 'cs', 'greek': 'el',
        'armenian': 'hy', 'bulgarian': 'bg', 'cyrillic': 'cu'
    }
    lang_code = lang_codes.get(language, language[:2])

    print(f"Loading {filepath.name} ({filepath.stat().st_size / 1e9:.1f} GB)...")

    conn = get_kaikki_connection()

    print("Initializing schema...")
    init_kaikki_schema(conn)

    print(f"Loading entries (lang={lang_code})...")
    counts = load_kaikki_jsonl(conn, filepath, lang_code=lang_code, limit=limit)

    print(f"\nDone!")
    print(f"  Entries:   {counts['entries']:,}")
    print(f"  Senses:    {counts['senses']:,}")
    print(f"  Forms:     {counts['forms']:,}")
    print(f"  Relations: {counts['relations']:,}")

    conn.close()


if __name__ == "__main__":
    import sys
    language = sys.argv[1] if len(sys.argv) > 1 else 'english'
    limit = int(sys.argv[2]) if len(sys.argv) > 2 else None
    run(language, limit)
