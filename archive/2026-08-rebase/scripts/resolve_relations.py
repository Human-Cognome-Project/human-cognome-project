#!/usr/bin/env python3
"""
Resolve multi-token relation targets against existing entries.
Reconstruct target phrase from token arrays, look up in kk_entries.
"""

import psycopg2


def main():
    conn = psycopg2.connect(host='localhost', user='hcp', password='hcp_dev', dbname='hcp_english')
    cur = conn.cursor()

    # Build token_id → word lookup
    print('Building token→word lookup...', flush=True)
    cur.execute('SELECT token_id, word FROM kk_entries WHERE token_id IS NOT NULL')
    tid_to_word = {}
    for tid, word in cur.fetchall():
        if tid not in tid_to_word:
            tid_to_word[tid] = word
    print(f'  Tokens: {len(tid_to_word)}', flush=True)

    # Build word → token_id lookup
    cur.execute('SELECT word, token_id FROM kk_entries WHERE token_id IS NOT NULL')
    word_to_tid = {}
    for word, tid in cur.fetchall():
        if word not in word_to_tid:
            word_to_tid[word] = tid
        lower = word.lower()
        if lower not in word_to_tid:
            word_to_tid[lower] = tid

    # Process unresolved English relations
    print('Resolving relations...', flush=True)
    cur.execute("""
        SELECT id, tokenized_target
        FROM kk_relations
        WHERE target_token_id IS NULL
          AND (target_lang IS NULL OR target_lang = '')
          AND tokenized_target IS NOT NULL
    """)
    rows = cur.fetchall()
    print(f'  To resolve: {len(rows)}', flush=True)

    resolved = 0
    unresolved = 0
    batch = []

    for rel_id, tgt_tokens in rows:
        # Reconstruct target phrase
        words = []
        for tid in tgt_tokens:
            w = tid_to_word.get(tid)
            if w:
                words.append(w)
            elif tid.startswith('[') and tid.endswith(']'):
                words.append(tid[1:-1])
            else:
                words.append('?')

        phrase = ' '.join(words)

        # Look up the phrase
        target_tid = word_to_tid.get(phrase) or word_to_tid.get(phrase.lower())

        if target_tid:
            batch.append((target_tid, rel_id))
            resolved += 1
        else:
            unresolved += 1

        if len(batch) >= 5000:
            cur2 = conn.cursor()
            cur2.executemany('UPDATE kk_relations SET target_token_id = %s WHERE id = %s', batch)
            conn.commit()
            cur2.close()
            batch = []
            if resolved % 50000 == 0:
                print(f'  Resolved: {resolved}, unresolved: {unresolved}', flush=True)

    if batch:
        cur2 = conn.cursor()
        cur2.executemany('UPDATE kk_relations SET target_token_id = %s WHERE id = %s', batch)
        conn.commit()
        cur2.close()

    print(f'\n  Resolved: {resolved}', flush=True)
    print(f'  Unresolved: {unresolved}', flush=True)

    # Check final state
    cur.execute("""
        SELECT
            count(*) as total,
            count(target_token_id) as resolved,
            count(*) - count(target_token_id) as remaining
        FROM kk_relations
    """)
    total, res, rem = cur.fetchone()
    print(f'\n  Total relations: {total}', flush=True)
    print(f'  Resolved: {res}', flush=True)
    print(f'  Remaining: {rem}', flush=True)

    cur.close()
    conn.close()


if __name__ == '__main__':
    main()
