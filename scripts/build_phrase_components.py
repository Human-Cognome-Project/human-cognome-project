#!/usr/bin/env python3
"""
Build phrase component lookup: for each AB.AB phrase, resolve its component
words to ordered AB.AA token_id sequences.

Creates kk_phrase_components table mapping phrase_token_id → component_tokens[].
"""

import psycopg2

def main():
    conn = psycopg2.connect(host='localhost', user='hcp', password='hcp_dev', dbname='hcp_english')
    cur = conn.cursor()

    # --- Create table ---
    cur.execute("""
        DROP TABLE IF EXISTS kk_phrase_components;
        CREATE TABLE kk_phrase_components (
            phrase_entry_id INTEGER NOT NULL,
            phrase_token_id TEXT NOT NULL,
            word_count SMALLINT NOT NULL,
            component_tokens TEXT[] NOT NULL,
            resolution_status TEXT NOT NULL DEFAULT 'full'
        );
    """)
    conn.commit()

    # --- Build word→token_id lookup (first match per word) ---
    print("Building word→token_id lookup...", flush=True)
    cur.execute("SELECT word, token_id FROM kk_entries WHERE token_id LIKE 'AB.AA.%'")
    word_to_tid = {}
    for word, tid in cur.fetchall():
        if word not in word_to_tid:
            word_to_tid[word] = tid
    print(f"  Word lookup: {len(word_to_tid)} entries", flush=True)

    # --- Fetch all phrases ---
    cur.execute("""
        SELECT id, token_id, word
        FROM kk_entries
        WHERE token_id LIKE 'AB.AB.%'
        ORDER BY id
    """)
    phrases = cur.fetchall()
    print(f"  Phrases to resolve: {len(phrases)}", flush=True)

    # --- Resolve components ---
    full = 0
    partial = 0
    batch = []

    for entry_id, phrase_tid, word in phrases:
        parts = word.split(' ')
        component_tids = []
        all_resolved = True

        for part in parts:
            tid = word_to_tid.get(part)
            if not tid:
                tid = word_to_tid.get(part.lower())
            if tid:
                component_tids.append(tid)
            else:
                component_tids.append(f"[{part}]")
                all_resolved = False

        status = 'full' if all_resolved else 'partial'
        if all_resolved:
            full += 1
        else:
            partial += 1

        batch.append((entry_id, phrase_tid, len(parts), component_tids, status))

        if len(batch) >= 5000:
            cur.executemany(
                "INSERT INTO kk_phrase_components (phrase_entry_id, phrase_token_id, word_count, component_tokens, resolution_status) VALUES (%s, %s, %s, %s, %s)",
                batch
            )
            conn.commit()
            batch = []
            if (full + partial) % 50000 == 0:
                print(f"  Progress: {full + partial} ({full} full, {partial} partial)", flush=True)

    if batch:
        cur.executemany(
            "INSERT INTO kk_phrase_components (phrase_entry_id, phrase_token_id, word_count, component_tokens, resolution_status) VALUES (%s, %s, %s, %s, %s)",
            batch
        )
        conn.commit()

    # --- Add indexes ---
    print("Adding indexes...", flush=True)
    cur.execute("CREATE INDEX idx_pc_phrase_tid ON kk_phrase_components(phrase_token_id);")
    cur.execute("CREATE INDEX idx_pc_status ON kk_phrase_components(resolution_status);")
    cur.execute("CREATE INDEX idx_pc_word_count ON kk_phrase_components(word_count);")
    conn.commit()

    print(f"\n{'='*60}", flush=True)
    print(f"PHRASE COMPONENT RESOLUTION COMPLETE", flush=True)
    print(f"{'='*60}", flush=True)
    print(f"  Full resolution: {full}", flush=True)
    print(f"  Partial resolution: {partial}", flush=True)

    # --- Distribution ---
    cur.execute("""
        SELECT word_count, resolution_status, count(*)
        FROM kk_phrase_components
        GROUP BY 1, 2
        ORDER BY 1, 2
    """)
    print("\n  Word count | Status  | Count", flush=True)
    for wc, status, count in cur.fetchall():
        print(f"  {wc:10d} | {status:7s} | {count}", flush=True)

    cur.close()
    conn.close()


if __name__ == '__main__':
    main()
