#!/usr/bin/env python3
"""
Mint multi-word Wiktionary category names as phrase tokens.

- Strips 'en:' and other language prefixes
- Lowercases categories that aren't genuinely proper (all-lowercase-component test)
- Checks existing entries before minting
- Creates entries with spelling + component token links (morphology field)
- Proper noun categories go to AD namespace, common phrases to AB.AB
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
    if len(ch) != 1:
        return 29  # multi-byte unicode → other bucket
    c = ch.lower()
    if len(c) != 1:
        return 29  # lower() expanded (e.g. ß → ss)
    if 'a' <= c <= 'z':
        return ord(c) - ord('a')
    if c == "'": return 26
    if c == '-': return 27
    if c.isdigit(): return 28
    return 29


def make_p3_multiword(word):
    first = word[0] if word else 'a'
    word_count = len(word.split())
    idx = letter_index(first)
    return encode_pair(idx * 50 + word_count)


def make_p3_singleword(word):
    first = word[0] if word else 'a'
    char_len = len(word)
    idx = letter_index(first)
    return encode_pair(idx * 50 + char_len)


def encode_counter(counter):
    p4 = counter // 2500
    p5 = counter % 2500
    return encode_pair(p4) + "." + encode_pair(p5)


def get_max_counters(cur, ns_p2):
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


def clean_category_name(name):
    """Clean a category name for use as an entry word."""
    # Strip language prefixes (en:, pt:, zh:, etc.)
    name = re.sub(r'^[a-z]{2,3}:', '', name)
    # Strip underscore artifacts
    name = name.replace('_', ' ')
    # Strip parenthetical qualifiers like (plant), (fish) — keep content
    # Actually keep them, they disambiguate
    return name.strip()


def has_proper_noun_element(word, label_set):
    """Check if any component word is a proper noun (capitalized and in label set or not in common words)."""
    parts = word.split()
    for p in parts:
        if p[0:1].isupper() and p.lower() not in ('a', 'an', 'the', 'of', 'in', 'on', 'at', 'to', 'for', 'and', 'or', 'with', 'by', 'from', 'not', 'no', 'but', 'as', 'is', 'it', 'its', 'vs', 'vs.'):
            return True
    return False


def main():
    conn = psycopg2.connect(host='localhost', user='hcp', password='hcp_dev', dbname='hcp_english')
    cur = conn.cursor()

    # --- Load lookups ---
    print("Loading lookups...", flush=True)

    # Word → token_id (all namespaces, including unminted for existence check)
    cur.execute("SELECT word, token_id FROM kk_entries WHERE token_id IS NOT NULL")
    word_to_tid = {}
    for word, tid in cur.fetchall():
        if word not in word_to_tid:
            word_to_tid[word] = tid
        lower = word.lower()
        if lower not in word_to_tid:
            word_to_tid[lower] = tid

    # Also track all words that exist (even without token_id) to avoid insert collisions
    cur.execute("SELECT DISTINCT word FROM kk_entries")
    existing_words = {row[0] for row in cur.fetchall()}
    existing_words_lower = {w.lower() for w in existing_words}

    # Character sequences
    cur.execute("SELECT seq, character FROM english_characters")
    char_to_seq = {row[1]: row[0] for row in cur.fetchall()}

    print(f"  Word lookup: {len(word_to_tid)}", flush=True)

    # --- Get all distinct multi-word category names ---
    cur.execute("""
        SELECT DISTINCT
            CASE WHEN name LIKE 'en:%%' THEN substring(name from 4) ELSE name END as clean_name
        FROM kk_sense_categories
        WHERE array_length(string_to_array(
            CASE WHEN name LIKE 'en:%%' THEN substring(name from 4) ELSE name END, ' '), 1) >= 2
    """)
    raw_cats = [row[0] for row in cur.fetchall()]
    print(f"  Distinct multi-word categories: {len(raw_cats)}", flush=True)

    # --- Clean and deduplicate ---
    cleaned = set()
    cat_mapping = {}  # cleaned_name → original names (for updating references later)

    for raw in raw_cats:
        clean = clean_category_name(raw)
        if not clean or len(clean) < 2:
            continue
        if clean not in cleaned:
            cleaned.add(clean)
            cat_mapping[clean] = []
        cat_mapping[clean].append(raw)

    print(f"  After cleaning/dedup: {len(cleaned)}", flush=True)

    # --- Check which already exist ---
    already_exist = set()
    need_minting = []

    for name in sorted(cleaned):
        # Check exact
        if name in word_to_tid:
            already_exist.add(name)
            continue
        # Check lowered
        lower = name.lower()
        if lower in word_to_tid:
            already_exist.add(name)
            continue
        # Check with proper noun logic - if it has caps and lowercase version exists
        if has_proper_noun_element(name, set()):
            if lower in word_to_tid:
                already_exist.add(name)
                continue
        need_minting.append(name)

    print(f"  Already exist: {len(already_exist)}", flush=True)
    print(f"  Need minting: {len(need_minting)}", flush=True)

    # --- Load counters ---
    ab_ab_counters = get_max_counters(cur, 'AB.AB')
    ab_aa_counters = get_max_counters(cur, 'AB.AA')
    ad_aa_counters = get_max_counters(cur, 'AD.AA')

    # --- Mint ---
    entries_to_insert = []
    minted_phrases = 0
    minted_labels = 0
    minted_single = 0

    for name in need_minting:
        # Determine if this is a proper noun entity or a common phrase
        is_proper = has_proper_noun_element(name, set())
        word_count = len(name.split())

        # For minting: lowercase if not proper
        mint_word = name if is_proper else name.lower()

        # Double check the lowered version doesn't already exist
        if mint_word in word_to_tid:
            already_exist.add(name)
            continue

        # Resolve component words
        parts = mint_word.split()
        component_tids = []
        for p in parts:
            tid = word_to_tid.get(p) or word_to_tid.get(p.lower())
            if tid:
                component_tids.append(tid)

        # Build spelling
        spelling = [char_to_seq[ch] for ch in mint_word if ch in char_to_seq]

        if word_count == 1:
            # Single word after cleaning (rare)
            if is_proper:
                ns, p2 = "AD", "AA"
                p3 = make_p3_singleword(mint_word)
                counters = ad_aa_counters
                minted_labels += 1
            else:
                ns, p2 = "AB", "AA"
                p3 = make_p3_singleword(mint_word)
                counters = ab_aa_counters
                minted_single += 1
        else:
            # Multi-word
            if is_proper:
                # Proper noun phrase — still needs a token
                # These go to AB.AB for now (entity DB cross-ref later)
                ns, p2 = "AB", "AB"
                p3 = make_p3_multiword(mint_word)
                counters = ab_ab_counters
                minted_phrases += 1
            else:
                ns, p2 = "AB", "AB"
                p3 = make_p3_multiword(mint_word)
                counters = ab_ab_counters
                minted_phrases += 1

        counter = counters.get(p3, 0)
        counters[p3] = counter + 1
        p4p5 = encode_counter(counter)
        token_id = f"{ns}.{p2}.{p3}.{p4p5}"

        pos = "name" if is_proper else "noun"
        morph = component_tids if component_tids else None

        entries_to_insert.append((mint_word, pos, token_id, spelling or None, morph))
        word_to_tid[mint_word] = token_id

    print(f"\n  Minting: {len(entries_to_insert)} entries", flush=True)
    print(f"    Phrases: {minted_phrases}", flush=True)
    print(f"    Labels: {minted_labels}", flush=True)
    print(f"    Single words: {minted_single}", flush=True)

    # --- Insert or update ---
    insert_batch = []
    update_batch = []
    for word, pos, tid, spelling, morph in entries_to_insert:
        if word in existing_words or word in existing_words_lower:
            # Entry exists but may lack token_id — update it
            update_batch.append((tid, spelling, morph, word))
        else:
            insert_batch.append((word, pos, tid, spelling, morph))

    print(f"  New inserts: {len(insert_batch)}", flush=True)
    print(f"  Updates (existing entries): {len(update_batch)}", flush=True)

    batch = []
    for entry in insert_batch:
        batch.append(entry)
        if len(batch) >= 2000:
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

    # Update existing entries that lack token_ids
    batch = []
    for entry in update_batch:
        batch.append(entry)
        if len(batch) >= 2000:
            cur.executemany("""
                UPDATE kk_entries SET token_id = %s, spelling = %s, morphology = %s
                WHERE word = %s AND token_id IS NULL
            """, batch)
            conn.commit()
            batch = []

    if batch:
        cur.executemany("""
            UPDATE kk_entries SET token_id = %s, spelling = %s, morphology = %s
            WHERE word = %s AND token_id IS NULL
        """, batch)
        conn.commit()

    # --- Also add to phrase components for the multi-word ones ---
    print("\nBuilding phrase components for new entries...", flush=True)
    cur.execute("""
        SELECT id, word, token_id
        FROM kk_entries
        WHERE token_id LIKE 'AB.AB.%%'
          AND id NOT IN (SELECT phrase_entry_id FROM kk_phrase_components)
    """)
    new_phrases = cur.fetchall()
    print(f"  New phrases needing components: {len(new_phrases)}", flush=True)

    batch = []
    full = 0
    partial = 0
    for entry_id, word, phrase_tid in new_phrases:
        parts = word.split()
        comp_tids = []
        all_resolved = True
        for p in parts:
            tid = word_to_tid.get(p) or word_to_tid.get(p.lower())
            if tid:
                comp_tids.append(tid)
            else:
                comp_tids.append(f"[{p}]")
                all_resolved = False

        status = 'full' if all_resolved else 'partial'
        if all_resolved:
            full += 1
        else:
            partial += 1

        batch.append((entry_id, phrase_tid, len(parts), comp_tids, status))

        if len(batch) >= 2000:
            cur.executemany("""
                INSERT INTO kk_phrase_components
                (phrase_entry_id, phrase_token_id, word_count, component_tokens, resolution_status)
                VALUES (%s, %s, %s, %s, %s)
            """, batch)
            conn.commit()
            batch = []

    if batch:
        cur.executemany("""
            INSERT INTO kk_phrase_components
            (phrase_entry_id, phrase_token_id, word_count, component_tokens, resolution_status)
            VALUES (%s, %s, %s, %s, %s)
        """, batch)
        conn.commit()

    print(f"  Components: {full} full, {partial} partial", flush=True)

    # --- Summary ---
    print(f"\n{'='*60}", flush=True)
    print("CATEGORY PHRASE MINTING COMPLETE", flush=True)
    print(f"{'='*60}", flush=True)

    cur.execute("SELECT substring(token_id from 1 for 5) as ns_p2, count(*) FROM kk_entries WHERE token_id IS NOT NULL GROUP BY 1 ORDER BY 1")
    for ns_p2, count in cur.fetchall():
        print(f"  {ns_p2}: {count}", flush=True)

    cur.execute("SELECT resolution_status, count(*) FROM kk_phrase_components GROUP BY 1")
    print("\nPhrase components:")
    for status, count in cur.fetchall():
        print(f"  {status}: {count}", flush=True)

    cur.close()
    conn.close()


if __name__ == '__main__':
    main()
