#!/usr/bin/env python3
"""
Phrase resolution: scan all tokenized arrays and replace word-token sequences
that match known phrases with the single phrase token_id.

Uses a trie for efficient prefix matching. Longest non-overlapping matches win
(greedy left-to-right, longest-first at each position).

Processes: tokenized_gloss, tokenized_text, tokenized_etymology, tokenized_context
"""

import psycopg2

TABLES = [
    ('kk_senses', 'id', 'tokenized_gloss'),
    ('kk_sense_examples', 'id', 'tokenized_text'),
    ('kk_entries', 'id', 'tokenized_etymology'),
    ('kk_relations', 'id', 'tokenized_context'),
]

BATCH_SIZE = 5000
COMMIT_EVERY = 50000


def build_trie(cur):
    """
    Build a trie from fully-resolved phrase components.
    Trie keys are token_id tuples, values are phrase_token_ids.
    Structure: dict of dicts, with '_val' key for terminal nodes.
    """
    print("Building phrase trie...", flush=True)

    cur.execute("""
        SELECT phrase_token_id, word_count, component_tokens
        FROM kk_phrase_components
        WHERE resolution_status = 'full'
        ORDER BY word_count DESC
    """)

    trie = {}
    count = 0

    for phrase_tid, word_count, component_tokens in cur.fetchall():
        node = trie
        for tok in component_tokens:
            if tok not in node:
                node[tok] = {}
            node = node[tok]
        # Store the phrase token_id at the terminal node
        # First write wins (we sorted by word_count DESC, but components are unique sequences)
        if '_val' not in node:
            node['_val'] = (phrase_tid, word_count)
            count += 1

    print(f"  Trie built: {count} patterns", flush=True)
    return trie


def resolve_array(tokens, trie):
    """
    Scan a token array for phrase matches using the trie.
    Greedy longest-match-first at each position, left-to-right.
    Returns new array with replacements, or None if no matches.
    """
    if not tokens or len(tokens) < 2:
        return None

    n = len(tokens)
    new_tokens = []
    i = 0
    changed = False

    while i < n:
        # Try to match the longest phrase starting at position i
        node = trie
        best_match = None  # (end_exclusive, phrase_tid)
        j = i

        while j < n and tokens[j] in node:
            node = node[tokens[j]]
            j += 1
            if '_val' in node:
                best_match = (j, node['_val'][0])
            # Keep going to find longer matches

        if best_match:
            end, phrase_tid = best_match
            new_tokens.append(phrase_tid)
            i = end
            changed = True
        else:
            new_tokens.append(tokens[i])
            i += 1

    return new_tokens if changed else None


def process_table(conn, trie, table, pk_col, array_col):
    """Process one table: scan all arrays, replace phrase matches."""
    cur = conn.cursor('server_cursor', withhold=True)
    update_cur = conn.cursor()

    cur.execute(f"""
        SELECT {pk_col}, {array_col}
        FROM {table}
        WHERE {array_col} IS NOT NULL
          AND array_length({array_col}, 1) >= 2
        ORDER BY {pk_col}
    """)

    scanned = 0
    replaced = 0
    total_tokens_saved = 0
    batch = []

    while True:
        rows = cur.fetchmany(BATCH_SIZE)
        if not rows:
            break

        for pk, tokens in rows:
            scanned += 1
            new_tokens = resolve_array(tokens, trie)
            if new_tokens is not None:
                saved = len(tokens) - len(new_tokens)
                total_tokens_saved += saved
                replaced += 1
                batch.append((new_tokens, pk))

        if len(batch) >= BATCH_SIZE:
            update_cur.executemany(
                f"UPDATE {table} SET {array_col} = %s WHERE {pk_col} = %s",
                batch
            )
            conn.commit()
            batch = []

        if scanned % COMMIT_EVERY == 0:
            if batch:
                update_cur.executemany(
                    f"UPDATE {table} SET {array_col} = %s WHERE {pk_col} = %s",
                    batch
                )
                batch = []
            conn.commit()
            print(f"    {table}: scanned {scanned}, replaced {replaced}, tokens saved {total_tokens_saved}", flush=True)

    if batch:
        update_cur.executemany(
            f"UPDATE {table} SET {array_col} = %s WHERE {pk_col} = %s",
            batch
        )
    conn.commit()

    cur.close()
    update_cur.close()

    return scanned, replaced, total_tokens_saved


def main():
    conn = psycopg2.connect(host='localhost', user='hcp', password='hcp_dev', dbname='hcp_english')
    cur = conn.cursor()

    # Build trie
    trie = build_trie(cur)
    cur.close()

    # Process each table
    print(f"\n{'='*60}", flush=True)
    print("PHRASE RESOLUTION", flush=True)
    print(f"{'='*60}", flush=True)

    grand_scanned = 0
    grand_replaced = 0
    grand_saved = 0

    for table, pk_col, array_col in TABLES:
        print(f"\n  Processing {table}.{array_col}...", flush=True)
        scanned, replaced, saved = process_table(conn, trie, table, pk_col, array_col)
        print(f"    Done: scanned {scanned}, replaced {replaced}, tokens saved {saved}", flush=True)
        grand_scanned += scanned
        grand_replaced += replaced
        grand_saved += saved

    print(f"\n{'='*60}", flush=True)
    print("PHRASE RESOLUTION COMPLETE", flush=True)
    print(f"{'='*60}", flush=True)
    print(f"  Total arrays scanned: {grand_scanned}", flush=True)
    print(f"  Arrays with replacements: {grand_replaced}", flush=True)
    print(f"  Total tokens saved: {grand_saved}", flush=True)

    conn.close()


if __name__ == '__main__':
    main()
