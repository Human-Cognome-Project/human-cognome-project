#!/usr/bin/env python3
"""
Resolve partial phrase components by minting missing entries.

Categories handled:
1. Possessives (devil's, man's) → mint as entries with morphology [root, POSS]
2. Possessive plurals (boys', witches') → mint with morphology [root, POSS_PL]
3. Hyphenated words (yellow-billed) → mint as entries, morphology = component words
4. Punctuation artifacts (it,  duck,) → strip punctuation, re-resolve
5. Capitalized (Newcastle, England) → mint as AD.AA Labels
6. Quoted ("c") → strip quotes, re-resolve
7. Other (foreign words, numbers) → mint what we can

After minting, re-runs phrase component resolution to upgrade partial → full.
"""

import re
import psycopg2

B50 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwx"


def encode_pair(value):
    if value < 0: value = 0
    if value >= 2500: value = 2499
    return B50[value // 50] + B50[value % 50]


def decode_pair(s):
    c0 = (ord(s[0]) - ord('A')) if s[0].isupper() else (26 + ord(s[0]) - ord('a'))
    c1 = (ord(s[1]) - ord('A')) if s[1].isupper() else (26 + ord(s[1]) - ord('a'))
    return c0 * 50 + c1


def letter_index(ch):
    c = ch.lower()
    if 'a' <= c <= 'z':
        return ord(c) - ord('a')
    if c == "'": return 26
    if c == '-': return 27
    if c.isdigit(): return 28
    return 29


def make_p3(word):
    first = word[0] if word else 'a'
    char_len = len(word)
    idx = letter_index(first)
    return encode_pair(idx * 50 + char_len)


def encode_counter(counter):
    p4 = counter // 2500
    p5 = counter % 2500
    return encode_pair(p4) + "." + encode_pair(p5)


def get_max_counters(cur, ns_p2):
    """Get current max counters per p3 bucket for a namespace."""
    cur.execute(f"""
        SELECT substring(token_id from 7 for 2) as p3,
               max(substring(token_id from 10 for 5)) as max_p4p5
        FROM kk_entries
        WHERE token_id LIKE '{ns_p2}.%'
        GROUP BY 1
    """)
    counters = {}
    for p3, max_p4p5 in cur.fetchall():
        if max_p4p5 and len(max_p4p5) == 5:
            p4_val = decode_pair(max_p4p5[:2])
            p5_val = decode_pair(max_p4p5[3:])
            counters[p3] = p4_val * 2500 + p5_val + 1
    return counters


def mint_entry(word, pos, ns, p2, counters, char_to_seq, morphology=None):
    """Create a new kk_entries row and return (token_id, entry_data)."""
    p3 = make_p3(word)
    counter = counters.get(p3, 0)
    counters[p3] = counter + 1
    p4p5 = encode_counter(counter)
    token_id = f"{ns}.{p2}.{p3}.{p4p5}"

    spelling = [char_to_seq[ch] for ch in word if ch in char_to_seq]

    return token_id, spelling, morphology


def main():
    conn = psycopg2.connect(host='localhost', user='hcp', password='hcp_dev', dbname='hcp_english')
    cur = conn.cursor()

    # --- Load lookups ---
    print("Loading lookups...", flush=True)

    # Word → token_id (all namespaces)
    cur.execute("SELECT word, token_id FROM kk_entries WHERE token_id IS NOT NULL")
    word_to_tid = {}
    for word, tid in cur.fetchall():
        if word not in word_to_tid:
            word_to_tid[word] = tid
        lower = word.lower()
        if lower not in word_to_tid:
            word_to_tid[lower] = tid

    # Character sequences
    cur.execute("SELECT seq, character FROM english_characters")
    char_to_seq = {row[1]: row[0] for row in cur.fetchall()}

    # Morpheme tokens (for POSS)
    cur.execute("SELECT word, token_id FROM kk_entries WHERE token_id LIKE 'AC.AA.%'")
    morpheme_tids = {}
    for word, tid in cur.fetchall():
        if word not in morpheme_tids:
            morpheme_tids[word] = tid

    # Get POSS morpheme token_id (apostrophe-s)
    poss_tid = morpheme_tids.get("'s") or morpheme_tids.get("-'s")
    print(f"  Word lookup: {len(word_to_tid)}", flush=True)
    print(f"  Morpheme lookup: {len(morpheme_tids)}", flush=True)
    print(f"  POSS morpheme tid: {poss_tid}", flush=True)

    # --- Get all unique unresolved components ---
    cur.execute("""
        SELECT DISTINCT unnest(component_tokens) as component
        FROM kk_phrase_components
        WHERE resolution_status = 'partial'
    """)
    unresolved = [row[0] for row in cur.fetchall() if row[0].startswith('[')]
    print(f"\nUnique unresolved components: {len(unresolved)}", flush=True)

    # --- Load counters ---
    aa_counters = get_max_counters(cur, 'AB.AA')
    ad_counters = get_max_counters(cur, 'AD.AA')

    # --- Categorize and resolve ---
    minted = 0
    resolved_by_strip = 0
    skipped = 0

    # Track what we mint: raw_word → token_id
    newly_minted = {}

    entries_to_insert = []  # (word, pos, token_id, spelling, morphology)

    for comp in unresolved:
        raw = comp[1:-1]  # strip [ ]

        # Already resolved somehow?
        if raw in word_to_tid:
            newly_minted[raw] = word_to_tid[raw]
            resolved_by_strip += 1
            continue

        # --- Possessive: word's ---
        if raw.endswith("'s") and len(raw) > 2:
            root = raw[:-2]
            root_tid = word_to_tid.get(root) or word_to_tid.get(root.lower())
            if root_tid:
                is_cap = root[0].isupper()
                ns = "AD" if is_cap else "AB"
                counters = ad_counters if is_cap else aa_counters
                pos = "noun"
                morph = [root_tid]
                if poss_tid:
                    morph.append(poss_tid)

                tid, spelling, _ = mint_entry(raw, pos, ns, "AA", counters, char_to_seq, morph)
                entries_to_insert.append((raw, pos, tid, spelling, morph))
                newly_minted[raw] = tid
                word_to_tid[raw] = tid
                minted += 1
                continue

        # --- Possessive plural: boys', witches' ---
        if raw.endswith("s'") or raw.endswith("'"):
            root_candidates = []
            if raw.endswith("s'"):
                root_candidates = [raw[:-1], raw[:-2]]  # boys' → boys or boy
            else:
                root_candidates = [raw[:-1]]  # witches' → witches

            root_tid = None
            for rc in root_candidates:
                root_tid = word_to_tid.get(rc) or word_to_tid.get(rc.lower())
                if root_tid:
                    break

            if root_tid:
                is_cap = raw[0].isupper()
                ns = "AD" if is_cap else "AB"
                counters = ad_counters if is_cap else aa_counters
                pos = "noun"
                morph = [root_tid]

                tid, spelling, _ = mint_entry(raw, pos, ns, "AA", counters, char_to_seq, morph)
                entries_to_insert.append((raw, pos, tid, spelling, morph))
                newly_minted[raw] = tid
                word_to_tid[raw] = tid
                minted += 1
                continue

        # --- Punctuation: strip trailing comma/semicolon/colon ---
        if raw[-1] in ',;:' and len(raw) > 1:
            stripped = raw[:-1]
            tid = word_to_tid.get(stripped) or word_to_tid.get(stripped.lower())
            if tid:
                newly_minted[raw] = tid  # map to the stripped version's token
                resolved_by_strip += 1
                continue

        # --- Quoted: strip quotes ---
        if raw.startswith('"') and raw.endswith('"'):
            stripped = raw[1:-1]
            tid = word_to_tid.get(stripped) or word_to_tid.get(stripped.lower())
            if tid:
                newly_minted[raw] = tid
                resolved_by_strip += 1
                continue

        # --- Hyphenated: mint as entry, morphology = component words ---
        if '-' in raw and len(raw) > 2:
            parts = raw.split('-')
            part_tids = []
            all_found = True
            for p in parts:
                ptid = word_to_tid.get(p) or word_to_tid.get(p.lower())
                if ptid:
                    part_tids.append(ptid)
                else:
                    all_found = False
                    break

            if all_found and part_tids:
                is_cap = raw[0].isupper()
                ns = "AD" if is_cap else "AB"
                counters = ad_counters if is_cap else aa_counters
                pos = "adj"  # most hyphenated in phrases are adjectives (yellow-billed, white-tailed)
                morph = part_tids

                tid, spelling, _ = mint_entry(raw, pos, ns, "AA", counters, char_to_seq, morph)
                entries_to_insert.append((raw, pos, tid, spelling, morph))
                newly_minted[raw] = tid
                word_to_tid[raw] = tid
                minted += 1
                continue

        # --- Capitalized: mint as Label ---
        if raw[0].isupper():
            pos = "name"
            morph = None
            tid, spelling, _ = mint_entry(raw, pos, "AD", "AA", ad_counters, char_to_seq, morph)
            entries_to_insert.append((raw, pos, tid, spelling, morph))
            newly_minted[raw] = tid
            word_to_tid[raw] = tid
            minted += 1
            continue

        # --- Numbers ---
        if re.match(r'^[\d.]+$', raw):
            # Numeric — mint in AB.AA
            pos = "num"
            morph = None
            tid, spelling, _ = mint_entry(raw, pos, "AB", "AA", aa_counters, char_to_seq, morph)
            entries_to_insert.append((raw, pos, tid, spelling, morph))
            newly_minted[raw] = tid
            word_to_tid[raw] = tid
            minted += 1
            continue

        # --- Foreign/other lowercase: mint in AB.AA ---
        if raw.isalpha() and raw == raw.lower():
            pos = "noun"  # default for foreign words
            morph = None
            tid, spelling, _ = mint_entry(raw, pos, "AB", "AA", aa_counters, char_to_seq, morph)
            entries_to_insert.append((raw, pos, tid, spelling, morph))
            newly_minted[raw] = tid
            word_to_tid[raw] = tid
            minted += 1
            continue

        # --- Anything else with letters: mint ---
        if any(c.isalpha() for c in raw):
            is_cap = raw[0].isupper()
            ns = "AD" if is_cap else "AB"
            counters = ad_counters if is_cap else aa_counters
            pos = "phrase"
            morph = None
            tid, spelling, _ = mint_entry(raw, pos, ns, "AA", counters, char_to_seq, morph)
            entries_to_insert.append((raw, pos, tid, spelling, morph))
            newly_minted[raw] = tid
            word_to_tid[raw] = tid
            minted += 1
            continue

        skipped += 1

    print(f"\n  Minted new entries: {minted}", flush=True)
    print(f"  Resolved by stripping: {resolved_by_strip}", flush=True)
    print(f"  Skipped: {skipped}", flush=True)

    # --- Insert new entries into kk_entries ---
    print(f"\nInserting {len(entries_to_insert)} new entries...", flush=True)
    batch = []
    for word, pos, tid, spelling, morph in entries_to_insert:
        batch.append((word, pos, tid, spelling if spelling else None, morph if morph else None))
        if len(batch) >= 1000:
            cur.executemany("""
                INSERT INTO kk_entries (word, pos, token_id, spelling, morphology)
                VALUES (%s, %s, %s, %s, %s)
            """, batch)
            conn.commit()
            batch = []

    if batch:
        cur.executemany("""
            INSERT INTO kk_entries (word, pos, token_id, spelling, morphology)
            VALUES (%s, %s, %s, %s, %s)
        """, batch)
        conn.commit()

    # --- Re-resolve partial phrases ---
    print("\nRe-resolving partial phrases...", flush=True)
    cur.execute("""
        SELECT phrase_entry_id, phrase_token_id, word_count, component_tokens
        FROM kk_phrase_components
        WHERE resolution_status = 'partial'
    """)
    partials = cur.fetchall()

    upgraded = 0
    still_partial = 0
    batch = []

    for entry_id, phrase_tid, wc, comp_tokens in partials:
        new_tokens = []
        all_resolved = True

        for tok in comp_tokens:
            if tok.startswith('['):
                raw = tok[1:-1]
                tid = newly_minted.get(raw)
                if tid:
                    new_tokens.append(tid)
                else:
                    new_tokens.append(tok)
                    all_resolved = False
            else:
                new_tokens.append(tok)

        new_status = 'full' if all_resolved else 'partial'
        if all_resolved:
            upgraded += 1
        else:
            still_partial += 1

        batch.append((new_tokens, new_status, entry_id, phrase_tid))

        if len(batch) >= 1000:
            cur.executemany("""
                UPDATE kk_phrase_components
                SET component_tokens = %s, resolution_status = %s
                WHERE phrase_entry_id = %s AND phrase_token_id = %s
            """, batch)
            conn.commit()
            batch = []

    if batch:
        cur.executemany("""
            UPDATE kk_phrase_components
            SET component_tokens = %s, resolution_status = %s
            WHERE phrase_entry_id = %s AND phrase_token_id = %s
        """, batch)
        conn.commit()

    print(f"  Upgraded to full: {upgraded}", flush=True)
    print(f"  Still partial: {still_partial}", flush=True)

    # --- Summary ---
    cur.execute("SELECT resolution_status, count(*) FROM kk_phrase_components GROUP BY 1")
    print(f"\n{'='*60}", flush=True)
    print("PARTIAL PHRASE RESOLUTION COMPLETE", flush=True)
    print(f"{'='*60}", flush=True)
    for status, count in cur.fetchall():
        print(f"  {status}: {count}", flush=True)

    cur.execute("SELECT substring(token_id from 1 for 5) as ns_p2, count(*) FROM kk_entries WHERE token_id IS NOT NULL GROUP BY 1 ORDER BY 1")
    print("\nEntry counts by namespace:")
    for ns_p2, count in cur.fetchall():
        print(f"  {ns_p2}: {count}", flush=True)

    cur.close()
    conn.close()


if __name__ == '__main__':
    main()
