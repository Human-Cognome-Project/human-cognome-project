#!/usr/bin/env python3
"""
Mint token IDs for multi-word entries (p2=AB) and single-word abbreviations
that were missed in the first minting pass.

Multi-word entries: p2=AB, p3 = first_letter + word_count
Single-word abbreviations (pos='phrase'/'proverb'): p2=AA, p3 = first_letter + char_length

Same base-50 encoding as existing minter.
"""

import psycopg2
import sys

B50 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwx"

def encode_pair(value):
    """Encode integer 0-2499 as two base-50 characters."""
    if value < 0: value = 0
    if value >= 2500: value = 2499
    return B50[value // 50] + B50[value % 50]

def letter_index(ch):
    """Map first character to index for p3 encoding."""
    c = ch.lower()
    if 'a' <= c <= 'z':
        return ord(c) - ord('a')
    # Non-alpha: use index 26+ based on category
    if c == "'": return 26
    if c == '-': return 27
    if c.isdigit(): return 28
    return 29  # other symbols

def make_p3_multiword(word):
    """p3 for multi-word: first_letter_index + word_count."""
    first = word[0] if word else 'a'
    word_count = len(word.split())
    idx = letter_index(first)
    return encode_pair(idx * 50 + word_count)

def make_p3_singleword(word):
    """p3 for single-word: first_letter_index + char_length (same as existing minter)."""
    first = word[0] if word else 'a'
    char_len = len(word)
    idx = letter_index(first)
    return encode_pair(idx * 50 + char_len)

def encode_counter(counter):
    """Encode sequential counter as p4.p5 (two pairs)."""
    p4 = counter // 2500
    p5 = counter % 2500
    return encode_pair(p4) + "." + encode_pair(p5)


def main():
    conn = psycopg2.connect(host='localhost', user='hcp', password='hcp_dev', dbname='hcp_english')
    cur = conn.cursor()

    # --- Load existing counter state per namespace+p3 ---
    # We need to know the max counter already used in each bucket
    print("Loading existing counter state...", flush=True)

    # For AB.AA (single-word) buckets - get max p4.p5 per p3
    cur.execute("""
        SELECT substring(token_id from 7 for 2) as p3,
               max(substring(token_id from 10 for 5)) as max_p4p5
        FROM kk_entries
        WHERE token_id LIKE 'AB.AA.%'
        GROUP BY 1
    """)
    aa_counters = {}
    for p3, max_p4p5 in cur.fetchall():
        # Decode max_p4p5 back to integer
        if max_p4p5 and len(max_p4p5) == 5:
            p4_str = max_p4p5[:2]
            p5_str = max_p4p5[3:]  # skip the dot
            p4_val = (ord(p4_str[0]) - ord('A') if p4_str[0].isupper() else 26 + ord(p4_str[0]) - ord('a')) * 50 + \
                     (ord(p4_str[1]) - ord('A') if p4_str[1].isupper() else 26 + ord(p4_str[1]) - ord('a'))
            p5_val = (ord(p5_str[0]) - ord('A') if p5_str[0].isupper() else 26 + ord(p5_str[0]) - ord('a')) * 50 + \
                     (ord(p5_str[1]) - ord('A') if p5_str[1].isupper() else 26 + ord(p5_str[1]) - ord('a'))
            aa_counters[p3] = p4_val * 2500 + p5_val + 1  # next available
        else:
            aa_counters[p3] = 0

    # AD.AA (Labels) - same
    cur.execute("""
        SELECT substring(token_id from 7 for 2) as p3,
               max(substring(token_id from 10 for 5)) as max_p4p5
        FROM kk_entries
        WHERE token_id LIKE 'AD.AA.%'
        GROUP BY 1
    """)
    ad_counters = {}
    for p3, max_p4p5 in cur.fetchall():
        if max_p4p5 and len(max_p4p5) == 5:
            p4_str = max_p4p5[:2]
            p5_str = max_p4p5[3:]
            p4_val = (ord(p4_str[0]) - ord('A') if p4_str[0].isupper() else 26 + ord(p4_str[0]) - ord('a')) * 50 + \
                     (ord(p4_str[1]) - ord('A') if p4_str[1].isupper() else 26 + ord(p4_str[1]) - ord('a'))
            p5_val = (ord(p5_str[0]) - ord('A') if p5_str[0].isupper() else 26 + ord(p5_str[0]) - ord('a')) * 50 + \
                     (ord(p5_str[1]) - ord('A') if p5_str[1].isupper() else 26 + ord(p5_str[1]) - ord('a'))
            ad_counters[p3] = p4_val * 2500 + p5_val + 1
        else:
            ad_counters[p3] = 0

    # AB.AB counters start fresh (no existing multi-word IDs)
    ab_counters = {}

    print(f"  AA buckets with existing IDs: {len(aa_counters)}", flush=True)
    print(f"  AD buckets with existing IDs: {len(ad_counters)}", flush=True)

    # --- Fetch entries to mint ---
    cur.execute("""
        SELECT id, word, pos,
               array_length(string_to_array(word, ' '), 1) as word_count
        FROM kk_entries
        WHERE token_id IS NULL
        ORDER BY word
    """)
    entries = cur.fetchall()
    print(f"\nEntries to mint: {len(entries)}", flush=True)

    # --- Mint ---
    single_word_minted = 0
    multi_word_minted = 0
    capitalized_skipped = 0
    batch = []

    for entry_id, word, pos, word_count in entries:
        is_cap = word[0].isupper() if word else False
        is_multi = word_count and word_count >= 2

        if is_cap and is_multi:
            # Capitalized multi-word → entity DB, skip
            capitalized_skipped += 1
            continue

        if is_multi:
            # Lowercase multi-word → AB.AB
            ns = "AB"
            p2 = "AB"
            p3 = make_p3_multiword(word)
            counter = ab_counters.get(p3, 0)
            ab_counters[p3] = counter + 1
            multi_word_minted += 1
        elif is_cap:
            # Capitalized single-word (acronyms like NIMBY) → AD.AA
            ns = "AD"
            p2 = "AA"
            p3 = make_p3_singleword(word)
            counter = ad_counters.get(p3, 0)
            ad_counters[p3] = counter + 1
            single_word_minted += 1
        else:
            # Lowercase single-word (abbreviations like brb, wtf) → AB.AA
            ns = "AB"
            p2 = "AA"
            p3 = make_p3_singleword(word)
            counter = aa_counters.get(p3, 0)
            aa_counters[p3] = counter + 1
            single_word_minted += 1

        p4p5 = encode_counter(counter)
        token_id = f"{ns}.{p2}.{p3}.{p4p5}"
        batch.append((token_id, entry_id))

        if len(batch) >= 5000:
            cur.executemany("UPDATE kk_entries SET token_id = %s WHERE id = %s", batch)
            conn.commit()
            total = single_word_minted + multi_word_minted
            if total % 50000 == 0:
                print(f"  Minted: {total} ({single_word_minted} single, {multi_word_minted} multi, {capitalized_skipped} cap skipped)", flush=True)
            batch = []

    if batch:
        cur.executemany("UPDATE kk_entries SET token_id = %s WHERE id = %s", batch)
        conn.commit()

    # --- Populate spelling for newly minted entries ---
    print("\nPopulating spelling for newly minted entries...", flush=True)
    cur.execute("SELECT seq, character FROM english_characters")
    char_to_seq = {row[1]: row[0] for row in cur.fetchall()}

    cur.execute("""
        SELECT id, word FROM kk_entries
        WHERE token_id IS NOT NULL AND spelling IS NULL
    """)
    to_spell = cur.fetchall()
    print(f"  Entries needing spelling: {len(to_spell)}", flush=True)

    batch = []
    spelled = 0
    for entry_id, word in to_spell:
        seq_arr = []
        for ch in word:
            s = char_to_seq.get(ch)
            if s is not None:
                seq_arr.append(s)
            # Characters not in english_characters are skipped (shouldn't happen)

        if seq_arr:
            batch.append((seq_arr, entry_id))

        if len(batch) >= 5000:
            cur.executemany("UPDATE kk_entries SET spelling = %s WHERE id = %s", batch)
            conn.commit()
            spelled += len(batch)
            batch = []

    if batch:
        cur.executemany("UPDATE kk_entries SET spelling = %s WHERE id = %s", batch)
        conn.commit()
        spelled += len(batch)

    print(f"  Spelling populated: {spelled}", flush=True)

    # --- Summary ---
    print(f"\n{'='*60}", flush=True)
    print(f"MINTING COMPLETE", flush=True)
    print(f"{'='*60}", flush=True)
    print(f"Single-word (abbreviations): {single_word_minted}", flush=True)
    print(f"Multi-word (phrases): {multi_word_minted}", flush=True)
    print(f"Capitalized multi-word (skipped → entities): {capitalized_skipped}", flush=True)
    print(f"Spelling populated: {spelled}", flush=True)

    # Verify
    cur.execute("SELECT count(*) FROM kk_entries WHERE token_id IS NULL")
    remaining = cur.fetchone()[0]
    print(f"Entries still without token_id: {remaining}", flush=True)

    cur.execute("SELECT substring(token_id from 1 for 5) as ns_p2, count(*) FROM kk_entries WHERE token_id IS NOT NULL GROUP BY 1 ORDER BY 1")
    for ns_p2, count in cur.fetchall():
        print(f"  {ns_p2}: {count}", flush=True)

    cur.close()
    conn.close()


if __name__ == '__main__':
    main()
