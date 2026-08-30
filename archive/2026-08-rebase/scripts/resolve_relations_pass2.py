#!/usr/bin/env python3
"""
Second pass: resolve remaining relations with case-insensitive matching
and possessive/punctuation stripping.
"""

import psycopg2


def main():
    conn = psycopg2.connect(host='localhost', user='hcp', password='hcp_dev', dbname='hcp_english')
    cur = conn.cursor()

    # Build token_id → word lookup
    print('Building lookups...', flush=True)
    cur.execute('SELECT token_id, word FROM kk_entries WHERE token_id IS NOT NULL')
    tid_to_word = {}
    for tid, word in cur.fetchall():
        if tid not in tid_to_word:
            tid_to_word[tid] = word

    # Build word → token_id with case-insensitive matching
    cur.execute('SELECT word, token_id FROM kk_entries WHERE token_id IS NOT NULL')
    word_to_tid = {}
    for word, tid in cur.fetchall():
        if word not in word_to_tid:
            word_to_tid[word] = tid
        lower = word.lower()
        if lower not in word_to_tid:
            word_to_tid[lower] = tid

    # Fetch remaining unresolved (exclude foreign and character-level)
    print('Fetching unresolved...', flush=True)
    cur.execute("""
        SELECT id, tokenized_target
        FROM kk_relations
        WHERE target_token_id IS NULL
          AND (target_lang IS NULL OR target_lang = '')
          AND tokenized_target IS NOT NULL
          AND tokenized_target[1] NOT LIKE 'AB.AA.AA.AA.%%'
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
                words.append(None)

        if None in words:
            unresolved += 1
            continue

        phrase = ' '.join(words)

        # Try multiple matching strategies
        target_tid = None

        # 1. Exact match
        target_tid = word_to_tid.get(phrase)

        # 2. Lowercase
        if not target_tid:
            target_tid = word_to_tid.get(phrase.lower())

        # 3. Title case
        if not target_tid:
            target_tid = word_to_tid.get(phrase.title())

        # 4. Strip trailing punctuation
        if not target_tid and phrase.endswith(('.', ',', ';', ':')):
            stripped = phrase[:-1].strip()
            target_tid = word_to_tid.get(stripped) or word_to_tid.get(stripped.lower())

        # 5. Possessive: "X's Y" → try "X's Y" as-is then "X Y"
        if not target_tid and "'s " in phrase:
            deposs = phrase.replace("'s ", " ")
            target_tid = word_to_tid.get(deposs) or word_to_tid.get(deposs.lower())

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
            if resolved % 20000 == 0:
                print(f'  Resolved: {resolved}, unresolved: {unresolved}', flush=True)

    if batch:
        cur2 = conn.cursor()
        cur2.executemany('UPDATE kk_relations SET target_token_id = %s WHERE id = %s', batch)
        conn.commit()
        cur2.close()

    print(f'\n  Resolved: {resolved}', flush=True)
    print(f'  Unresolved: {unresolved}', flush=True)

    # Final state
    cur.execute("""
        SELECT count(*), count(target_token_id)
        FROM kk_relations
    """)
    total, res = cur.fetchone()
    print(f'\n  Total: {total}, resolved: {res}, remaining: {total - res}', flush=True)

    cur.close()
    conn.close()


if __name__ == '__main__':
    main()
