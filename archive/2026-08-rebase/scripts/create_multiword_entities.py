#!/usr/bin/env python3
"""
Process all 59K capitalized multi-word entries:
1. Mint token_ids in AD.AB namespace (multi-word Labels)
2. Create entity records in appropriate entity DB (nf/fic)
3. Link entity_names to component word tokens
4. Move glosses to entity_descriptions
5. Build phrase components for resolution
"""

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
        return 29
    c = ch.lower()
    if len(c) != 1:
        return 29
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


def counter_to_p4p5(counter):
    return encode_pair(counter // 2500), encode_pair(counter % 2500)


def get_entity_counter(cur, ns):
    """Get next safe counter for entity DB namespace."""
    cur.execute("SELECT token_id FROM tokens WHERE ns = %s", (ns,))
    existing = {r[0] for r in cur.fetchall()}
    cur.execute("SELECT count(*) FROM tokens WHERE ns = %s", (ns,))
    count = cur.fetchone()[0]
    return count, existing


def entity_counter_to_parts(counter):
    v5 = counter % 2500
    rest = counter // 2500
    v4 = rest % 2500
    rest = rest // 2500
    v3 = rest % 2500
    v2 = rest // 2500
    return encode_pair(v2), encode_pair(v3), encode_pair(v4), encode_pair(v5)


def determine_entity_type(cats):
    """Classify entity from its categories."""
    cat_str = ' '.join(cats).lower() if cats else ''

    if 'fictional' in cat_str:
        return 'fic', 'person', 'fictional'
    if any(x in cat_str for x in ['mythology', 'mythological']):
        return 'nf', 'person', 'mythology'
    if any(x in cat_str for x in ['buddha', 'bodhisattva', 'biblical', 'bible', 'saint',
                                    'god', 'gods', 'deity', 'religion']):
        return 'nf', 'person', 'religious'
    if any(x in cat_str for x in ['planet', 'constellation', 'asteroid', 'astronomy']):
        return 'nf', 'place', 'astronomical'
    if any(x in cat_str for x in ['language']):
        return 'nf', 'thing', 'language'
    if any(x in cat_str for x in ['country', 'countries', 'continent']):
        return 'nf', 'place', 'country'
    if any(x in cat_str for x in ['individual', 'people', 'historical']):
        return 'nf', 'person', 'historical'
    if any(x in cat_str for x in ['organization', 'company']):
        return 'nf', 'thing', 'organization'
    if any(x in cat_str for x in ['computing', 'software', 'programming']):
        return 'nf', 'thing', 'technology'
    if any(x in cat_str for x in ['geology', 'chemistry', 'biology', 'physics',
                                    'medicine', 'botany', 'zoology', 'mathematics']):
        return 'nf', 'thing', 'science'
    if any(x in cat_str for x in ['holiday', 'festival']):
        return 'nf', 'thing', 'holiday'
    if any(x in cat_str for x in ['eponym']):
        return 'nf', 'thing', 'eponym'

    # Default: nf thing
    return 'nf', 'thing', 'other'


NS_MAP = {
    ('nf', 'person'): 'yA',
    ('nf', 'place'): 'xA',
    ('nf', 'thing'): 'wA',
    ('fic', 'person'): 'uA',
    ('fic', 'place'): 'tA',
    ('fic', 'thing'): 'sA',
}


def main():
    eng = psycopg2.connect(host='localhost', user='hcp', password='hcp_dev', dbname='hcp_english')
    nf = psycopg2.connect(host='localhost', user='hcp', password='hcp_dev', dbname='hcp_nf_entities')
    fic = psycopg2.connect(host='localhost', user='hcp', password='hcp_dev', dbname='hcp_fic_entities')
    ec = eng.cursor()
    nfc = nf.cursor()
    fc = fic.cursor()

    # --- Load word→token lookup ---
    print("Loading word→token lookup...", flush=True)
    ec.execute("SELECT word, token_id FROM kk_entries WHERE token_id IS NOT NULL")
    word_to_tid = {}
    for word, tid in ec.fetchall():
        if word not in word_to_tid:
            word_to_tid[word] = tid
        lower = word.lower()
        if lower not in word_to_tid:
            word_to_tid[lower] = tid
    print(f"  Words: {len(word_to_tid)}", flush=True)

    # Character sequences for spelling
    ec.execute("SELECT seq, character FROM english_characters")
    char_to_seq = {row[1]: row[0] for row in ec.fetchall()}

    # --- Load AD.AB counters (for minting Label token_ids) ---
    ec.execute("""
        SELECT substring(token_id from 7 for 2) as p3,
               max(substring(token_id from 10 for 5)) as max_p4p5
        FROM kk_entries WHERE token_id LIKE 'AD.AB.%%'
        GROUP BY 1
    """)
    label_counters = {}
    for p3, max_p4p5 in ec.fetchall():
        if max_p4p5 and len(max_p4p5) == 5:
            p4_val = decode_pair(max_p4p5[:2])
            p5_val = decode_pair(max_p4p5[3:])
            label_counters[p3] = p4_val * 2500 + p5_val + 1

    # --- Load entity DB counters ---
    print("Loading entity counters...", flush=True)
    entity_counters = {}
    entity_existing = {}
    for ns in ['yA', 'xA', 'wA']:
        c, e = get_entity_counter(nfc, ns)
        entity_counters[ns] = c
        entity_existing[ns] = e
    for ns in ['uA', 'tA', 'sA']:
        c, e = get_entity_counter(fc, ns)
        entity_counters[ns] = c
        entity_existing[ns] = e

    # Existing entity names to avoid dupes
    nfc.execute("SELECT name FROM tokens")
    nf_names = {r[0] for r in nfc.fetchall()}
    fc.execute("SELECT name FROM tokens")
    fic_names = {r[0] for r in fc.fetchall()}

    # --- Fetch all entries to process ---
    ec.execute("""
        SELECT e.id, e.word, e.pos
        FROM kk_entries e
        WHERE e.token_id IS NULL
          AND e.word ~ '^[A-Z]'
          AND array_length(string_to_array(e.word, ' '), 1) >= 2
        ORDER BY e.word
    """)
    entries = ec.fetchall()
    print(f"Entries to process: {len(entries)}", flush=True)

    minted = 0
    entities_created = 0
    glosses_moved = 0
    skipped_existing = 0

    eng_batch = []  # (token_id, spelling, entry_id)

    for entry_id, word, pos in entries:
        name_lower = word.lower().replace(' ', '_')

        # --- Step 1: Mint Label token_id in AD.AB ---
        p3 = make_p3_multiword(word)
        counter = label_counters.get(p3, 0)
        p4, p5 = counter_to_p4p5(counter)
        label_counters[p3] = counter + 1
        label_tid = f"AD.AB.{p3}.{p4}.{p5}"

        spelling = [char_to_seq[ch] for ch in word if ch in char_to_seq] or None

        eng_batch.append((label_tid, spelling, entry_id))

        if len(eng_batch) >= 2000:
            ec2 = eng.cursor()
            ec2.executemany("UPDATE kk_entries SET token_id = %s, spelling = %s WHERE id = %s", eng_batch)
            eng.commit()
            ec2.close()
            eng_batch = []

        # --- Step 2: Resolve component words ---
        parts = word.split()
        comp_tids = []
        for p in parts:
            tid = word_to_tid.get(p) or word_to_tid.get(p.lower())
            if tid:
                comp_tids.append(tid)

        # --- Step 3: Get categories for classification ---
        ec.execute("""
            SELECT DISTINCT sc.name
            FROM kk_senses s
            JOIN kk_sense_categories sc ON sc.sense_id = s.id
            WHERE s.entry_id = %s
            AND sc.name NOT LIKE 'English%%'
            AND sc.name NOT LIKE 'Terms%%'
            AND sc.name NOT LIKE 'en:%%'
        """, (entry_id,))
        cats = [r[0] for r in ec.fetchall()]

        db_type, category, subcategory = determine_entity_type(cats)
        ns = NS_MAP[(db_type, category)]
        cur = nfc if db_type == 'nf' else fc
        conn = nf if db_type == 'nf' else fic
        names_set = nf_names if db_type == 'nf' else fic_names
        existing_tids = entity_existing[ns]

        # Skip if entity already exists
        if name_lower in names_set:
            skipped_existing += 1
            minted += 1
            continue

        # --- Step 4: Create entity token ---
        ent_counter = entity_counters[ns]
        while True:
            ep2, ep3, ep4, ep5 = entity_counter_to_parts(ent_counter)
            ent_tid = f"{ns}.{ep2}.{ep3}.{ep4}.{ep5}"
            if ent_tid not in existing_tids:
                break
            ent_counter += 1
        entity_counters[ns] = ent_counter + 1
        existing_tids.add(ent_tid)

        cur.execute("""
            INSERT INTO tokens (ns, p2, p3, p4, p5, name, category, subcategory, metadata)
            VALUES (%s, %s, %s, %s, %s, %s, %s, %s, '{}')
        """, (ns, ep2, ep3, ep4, ep5, name_lower, category, subcategory))

        # --- Step 5: entity_names — link to component Label tokens ---
        for pos_idx, comp_tid in enumerate(comp_tids):
            cparts = comp_tid.split('.')
            if len(cparts) == 5:
                cur.execute("""
                    INSERT INTO entity_names (entity_ns, entity_p2, entity_p3, entity_p4, entity_p5,
                                              name_group, name_type, position,
                                              ns, p2, p3, p4, p5)
                    VALUES (%s, %s, %s, %s, %s, 0, 'primary', %s, %s, %s, %s, %s, %s)
                """, (ns, ep2, ep3, ep4, ep5, pos_idx + 1,
                      cparts[0], cparts[1], cparts[2], cparts[3], cparts[4]))

        # --- Step 6: Move glosses ---
        ec.execute("""
            SELECT s.tokenized_gloss
            FROM kk_senses s
            WHERE s.entry_id = %s AND s.tokenized_gloss IS NOT NULL
        """, (entry_id,))
        for (tg,) in ec.fetchall():
            if tg:
                cur.execute("""
                    INSERT INTO entity_descriptions (entity_ns, entity_p2, entity_p3, entity_p4, entity_p5,
                                                     description_type, description)
                    VALUES (%s, %s, %s, %s, %s, 'gloss', %s)
                """, (ns, ep2, ep3, ep4, ep5, ','.join(tg)))
                glosses_moved += 1

        names_set.add(name_lower)
        entities_created += 1
        minted += 1

        if minted % 2000 == 0:
            nf.commit()
            fic.commit()
            print(f"  Minted: {minted}, entities: {entities_created}, glosses: {glosses_moved}, skipped: {skipped_existing}", flush=True)

    # Flush remaining label mints
    if eng_batch:
        ec2 = eng.cursor()
        ec2.executemany("UPDATE kk_entries SET token_id = %s, spelling = %s WHERE id = %s", eng_batch)
        eng.commit()
        ec2.close()

    nf.commit()
    fic.commit()

    # --- Build phrase components for newly minted AD.AB entries ---
    print("\nBuilding phrase components...", flush=True)
    ec.execute("""
        SELECT e.id, e.word, e.token_id
        FROM kk_entries e
        LEFT JOIN kk_phrase_components pc ON pc.phrase_entry_id = e.id
        WHERE e.token_id LIKE 'AD.AB.%%'
          AND pc.phrase_entry_id IS NULL
    """)
    new_phrases = ec.fetchall()
    print(f"  New phrases needing components: {len(new_phrases)}", flush=True)

    pc_batch = []
    full = 0
    partial = 0
    for eid, word, ptid in new_phrases:
        parts = word.split()
        comp = []
        all_ok = True
        for p in parts:
            tid = word_to_tid.get(p) or word_to_tid.get(p.lower())
            if tid:
                comp.append(tid)
            else:
                comp.append(f"[{p}]")
                all_ok = False
        status = 'full' if all_ok else 'partial'
        if all_ok:
            full += 1
        else:
            partial += 1
        pc_batch.append((eid, ptid, len(parts), comp, status))
        if len(pc_batch) >= 2000:
            ec2 = eng.cursor()
            ec2.executemany("""
                INSERT INTO kk_phrase_components (phrase_entry_id, phrase_token_id, word_count, component_tokens, resolution_status)
                VALUES (%s, %s, %s, %s, %s)
            """, pc_batch)
            eng.commit()
            ec2.close()
            pc_batch = []

    if pc_batch:
        ec2 = eng.cursor()
        ec2.executemany("""
            INSERT INTO kk_phrase_components (phrase_entry_id, phrase_token_id, word_count, component_tokens, resolution_status)
            VALUES (%s, %s, %s, %s, %s)
        """, pc_batch)
        eng.commit()
        ec2.close()

    print(f"  Components: {full} full, {partial} partial", flush=True)

    # --- Summary ---
    print(f"\n{'='*60}", flush=True)
    print(f"MULTI-WORD ENTITY PROCESSING COMPLETE", flush=True)
    print(f"{'='*60}", flush=True)
    print(f"  Labels minted (AD.AB): {minted}", flush=True)
    print(f"  Entities created: {entities_created}", flush=True)
    print(f"  Glosses moved: {glosses_moved}", flush=True)
    print(f"  Skipped (existing): {skipped_existing}", flush=True)

    # Final counts
    ec.execute("SELECT substring(token_id from 1 for 5) as ns, count(*) FROM kk_entries WHERE token_id IS NOT NULL GROUP BY 1 ORDER BY 1")
    print("\nEnglish shard:")
    for ns, count in ec.fetchall():
        print(f"  {ns}: {count}", flush=True)

    nfc.execute("SELECT ns, count(*) FROM tokens GROUP BY 1 ORDER BY 1")
    print("\nnf_entities:")
    for ns, count in nfc.fetchall():
        print(f"  {ns}: {count}", flush=True)

    fc.execute("SELECT ns, count(*) FROM tokens GROUP BY 1 ORDER BY 1")
    print("\nfic_entities:")
    for ns, count in fc.fetchall():
        print(f"  {ns}: {count}", flush=True)

    ec.close(); nfc.close(); fc.close()
    eng.close(); nf.close(); fic.close()


if __name__ == '__main__':
    main()
