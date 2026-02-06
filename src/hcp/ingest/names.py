"""Ingest name components from proper noun entries into the Names shard.

This script:
1. Extracts all distinct name components from entries with NULL word_token
2. Creates y* tokens for each component (single words only)
3. Moves single-word entry data to hcp_names
4. Multi-word entries stay in hcp_english (will move to x* later)
"""

import re
from collections import defaultdict

from ..core.token_id import encode_name_token_id
from ..db.postgres import connect as connect_core
from ..db.english import connect as connect_english
from ..db.names import connect as connect_names, init_schema, insert_token


def build_char_lookup(core_conn) -> dict[str, str]:
    """Build lookup table: character -> Token ID.

    Reads from core database (character tokens at AA.AB.AA.*).
    """
    char_to_token = {}
    with core_conn.cursor() as cur:
        cur.execute("""
            SELECT token_id, metadata->>'char' as char
            FROM tokens
            WHERE token_id LIKE 'AA.AB.AA.%'
            AND metadata ? 'char'
        """)
        for token_id, char in cur.fetchall():
            if char:
                char_to_token[char] = token_id
    return char_to_token


def atomize_to_chars(word: str, char_lookup: dict[str, str]) -> list[str]:
    """Convert word to list of character Token IDs."""
    result = []
    for char in word:
        token_id = char_lookup.get(char)
        if token_id:
            result.append(token_id)
    return result


def split_into_components(word: str) -> list[str]:
    """Split a name into its component words.

    Handles:
    - Space-separated: "South China Sea" -> ["South", "China", "Sea"]
    - Hyphenated: "al-Amin" -> ["al-Amin"] (kept as single component)
    - Single word: "Paris" -> ["Paris"]
    """
    # Split on spaces only — hyphenated names are single components
    return word.split()


def run():
    """Run name component ingestion."""
    print("Connecting to databases...")
    core_conn = connect_core()
    eng_conn = connect_english()
    names_conn = connect_names()

    print("Initializing names shard schema...")
    init_schema(names_conn)

    print("Building character lookup from core...")
    char_lookup = build_char_lookup(core_conn)
    print(f"  {len(char_lookup)} characters in lookup")

    # Phase 1: Collect all distinct name components
    print("\nPhase 1: Collecting name components...")
    components = set()

    with eng_conn.cursor() as cur:
        cur.execute("""
            SELECT DISTINCT word FROM entries
            WHERE word_token IS NULL OR word_token = ''
        """)
        for (word,) in cur.fetchall():
            if word:
                for component in split_into_components(word):
                    components.add(component)

    print(f"  {len(components)} distinct name components")

    # Phase 2: Create y* tokens for each component
    print("\nPhase 2: Creating y* tokens...")
    component_to_token = {}
    count = 0

    with names_conn.cursor() as cur:
        for component in sorted(components):
            token_id = encode_name_token_id(count)
            atomization = atomize_to_chars(component, char_lookup)

            insert_token(cur, token_id, component, atomization=atomization)
            component_to_token[component] = token_id
            count += 1

            if count % 10000 == 0:
                print(f"    {count} tokens created...")

    names_conn.commit()
    print(f"  {count} tokens created")

    # Phase 3: Move single-word entry data
    print("\nPhase 3: Moving single-word entries to names shard...")
    moved_entries = 0
    moved_senses = 0
    moved_forms = 0
    moved_relations = 0

    # Get single-word entries (no spaces)
    with eng_conn.cursor() as eng_cur:
        eng_cur.execute("""
            SELECT id, word, pos_token, etymology_num, etymology_tokens, kaikki_id
            FROM entries
            WHERE (word_token IS NULL OR word_token = '')
            AND word NOT LIKE '%% %%'
        """)
        single_word_entries = eng_cur.fetchall()

    print(f"  {len(single_word_entries)} single-word entries to move")

    # Map old entry IDs to new entry IDs
    entry_id_map = {}

    with names_conn.cursor() as names_cur, eng_conn.cursor() as eng_cur:
        for old_id, word, pos_token, etym_num, etym_tokens, kaikki_id in single_word_entries:
            word_token = component_to_token.get(word)
            if not word_token:
                continue

            # Insert entry into names shard
            names_cur.execute("""
                INSERT INTO entries (word_token, pos_token, etymology_num,
                                     etymology_tokens, kaikki_id, word)
                VALUES (%s, %s, %s, %s, %s, %s)
                RETURNING id
            """, (word_token, pos_token, etym_num, etym_tokens, kaikki_id, word))
            new_id = names_cur.fetchone()[0]
            entry_id_map[old_id] = new_id
            moved_entries += 1

            if moved_entries % 10000 == 0:
                print(f"    {moved_entries} entries moved...")

        names_conn.commit()

        # Move senses
        print(f"  Moving senses...")
        for old_id, new_id in entry_id_map.items():
            eng_cur.execute("""
                SELECT gloss_tokens, tag_tokens FROM senses WHERE entry_id = %s
            """, (old_id,))
            for gloss_tokens, tag_tokens in eng_cur.fetchall():
                names_cur.execute("""
                    INSERT INTO senses (entry_id, gloss_tokens, tag_tokens)
                    VALUES (%s, %s, %s)
                """, (new_id, gloss_tokens, tag_tokens))
                moved_senses += 1

        names_conn.commit()

        # Move forms
        print(f"  Moving forms...")
        for old_id, new_id in entry_id_map.items():
            eng_cur.execute("""
                SELECT form_token, tag_tokens, form_text, form_tokens
                FROM forms WHERE entry_id = %s
            """, (old_id,))
            for form_token, tag_tokens, form_text, form_tokens in eng_cur.fetchall():
                names_cur.execute("""
                    INSERT INTO forms (entry_id, form_token, tag_tokens, form_text, form_tokens)
                    VALUES (%s, %s, %s, %s, %s)
                """, (new_id, form_token, tag_tokens, form_text, form_tokens))
                moved_forms += 1

        names_conn.commit()

        # Move relations
        print(f"  Moving relations...")
        for old_id, new_id in entry_id_map.items():
            eng_cur.execute("""
                SELECT relation_token, target_token, target_word, tag_tokens
                FROM relations WHERE entry_id = %s
            """, (old_id,))
            for rel_token, tgt_token, tgt_word, tag_tokens in eng_cur.fetchall():
                names_cur.execute("""
                    INSERT INTO relations (entry_id, relation_token, target_token,
                                           target_word, tag_tokens)
                    VALUES (%s, %s, %s, %s, %s)
                """, (new_id, rel_token, tgt_token, tgt_word, tag_tokens))
                moved_relations += 1

        names_conn.commit()

    # Phase 4: Delete moved entries from english shard
    print("\nPhase 4: Cleaning up english shard...")
    old_ids = list(entry_id_map.keys())

    with eng_conn.cursor() as cur:
        # Delete in correct order (children first)
        cur.execute("DELETE FROM relations WHERE entry_id = ANY(%s)", (old_ids,))
        deleted_rel = cur.rowcount

        cur.execute("DELETE FROM forms WHERE entry_id = ANY(%s)", (old_ids,))
        deleted_forms = cur.rowcount

        cur.execute("DELETE FROM senses WHERE entry_id = ANY(%s)", (old_ids,))
        deleted_senses = cur.rowcount

        cur.execute("DELETE FROM entries WHERE id = ANY(%s)", (old_ids,))
        deleted_entries = cur.rowcount

    eng_conn.commit()

    print("\n=== Summary ===")
    print(f"Name components (y* tokens): {count:>10,}")
    print(f"Entries moved:               {moved_entries:>10,}")
    print(f"Senses moved:                {moved_senses:>10,}")
    print(f"Forms moved:                 {moved_forms:>10,}")
    print(f"Relations moved:             {moved_relations:>10,}")
    print(f"\nDeleted from english shard:")
    print(f"  Entries:   {deleted_entries:,}")
    print(f"  Senses:    {deleted_senses:,}")
    print(f"  Forms:     {deleted_forms:,}")
    print(f"  Relations: {deleted_rel:,}")

    # Check remaining multi-word entries
    with eng_conn.cursor() as cur:
        cur.execute("""
            SELECT COUNT(*) FROM entries
            WHERE (word_token IS NULL OR word_token = '')
        """)
        remaining = cur.fetchone()[0]
    print(f"\nMulti-word entries remaining in english shard: {remaining:,}")

    core_conn.close()
    eng_conn.close()
    names_conn.close()


if __name__ == "__main__":
    run()
