#!/usr/bin/env python3
"""
Create nonfiction entities for languages, astronomical bodies, and holidays.

Languages → wA (things) namespace
Astronomical → xA (places) namespace
Holidays → wA (things) namespace
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


def get_counter(nf_cur, ns):
    """Get next sequential counter for a namespace by counting all existing entries."""
    nf_cur.execute("SELECT count(*) FROM tokens WHERE ns = %s", (ns,))
    count = nf_cur.fetchone()[0]
    if count == 0:
        return 0
    # Find actual highest sequential to avoid collisions
    nf_cur.execute("""
        SELECT p4, p5 FROM tokens WHERE ns = %s
        ORDER BY p4 DESC, p5 DESC LIMIT 1
    """, (ns,))
    row = nf_cur.fetchone()
    if row:
        val = decode_pair(row[0]) * 2500 + decode_pair(row[1]) + 1
        return max(val, count)
    return count


def create_entity(eng_cur, nf_cur, entry_id, word, label_tid, ns, p4, p5, category, subcategory):
    """Create one entity with names, glosses, and properties."""
    name_lower = word.lower().replace(' ', '_')

    # Insert token
    nf_cur.execute("""
        INSERT INTO tokens (ns, p2, p3, p4, p5, name, category, subcategory, metadata)
        VALUES (%s, 'AA', 'AA', %s, %s, %s, %s, %s, '{}')
    """, (ns, p4, p5, name_lower, category, subcategory))

    # Insert entity_names
    parts = label_tid.split('.')
    if len(parts) == 5:
        nf_cur.execute("""
            INSERT INTO entity_names (entity_ns, entity_p2, entity_p3, entity_p4, entity_p5,
                                      name_group, name_type, position,
                                      ns, p2, p3, p4, p5)
            VALUES (%s, 'AA', 'AA', %s, %s,
                    0, 'primary', 1,
                    %s, %s, %s, %s, %s)
        """, (ns, p4, p5, parts[0], parts[1], parts[2], parts[3], parts[4]))

    # Move glosses
    eng_cur.execute("""
        SELECT s.tokenized_gloss
        FROM kk_senses s
        WHERE s.entry_id = %s AND s.tokenized_gloss IS NOT NULL
    """, (entry_id,))
    glosses = 0
    for (tg,) in eng_cur.fetchall():
        if tg:
            gloss_str = ','.join(tg)
            nf_cur.execute("""
                INSERT INTO entity_descriptions (entity_ns, entity_p2, entity_p3, entity_p4, entity_p5,
                                                 description_type, description)
                VALUES (%s, 'AA', 'AA', %s, %s, 'gloss', %s)
            """, (ns, p4, p5, gloss_str))
            glosses += 1

    return glosses


def main():
    eng = psycopg2.connect(host='localhost', user='hcp', password='hcp_dev', dbname='hcp_english')
    nf = psycopg2.connect(host='localhost', user='hcp', password='hcp_dev', dbname='hcp_nf_entities')
    eng_cur = eng.cursor()
    nf_cur = nf.cursor()

    # Get existing names to avoid duplicates
    nf_cur.execute("SELECT ns, name FROM tokens")
    existing = {(row[0], row[1]) for row in nf_cur.fetchall()}

    # === LANGUAGES (wA things) ===
    print("=== LANGUAGES ===", flush=True)
    wA_counter = get_counter(nf_cur, 'wA')
    print(f"  wA counter starts at: {wA_counter}", flush=True)

    eng_cur.execute("""
        SELECT DISTINCT e.id, e.word, e.token_id
        FROM kk_entries e
        JOIN kk_senses s ON s.entry_id = e.id
        JOIN kk_sense_categories sc ON sc.sense_id = s.id
        WHERE e.token_id LIKE 'AD.AA.%%'
          AND e.pos = 'name'
          AND array_length(string_to_array(e.word, ' '), 1) = 1
          AND sc.name LIKE '%%Languages%%'
        ORDER BY e.word
    """)
    lang_entries = eng_cur.fetchall()

    lang_created = 0
    lang_glosses = 0
    for entry_id, word, label_tid in lang_entries:
        name_lower = word.lower().replace(' ', '_')
        if ('wA', name_lower) in existing:
            continue

        p4 = encode_pair(wA_counter // 2500)
        p5 = encode_pair(wA_counter % 2500)
        wA_counter += 1

        g = create_entity(eng_cur, nf_cur, entry_id, word, label_tid, 'wA', p4, p5, 'thing', 'language')
        lang_glosses += g
        existing.add(('wA', name_lower))
        lang_created += 1

        if lang_created % 500 == 0:
            nf.commit()
            print(f"  Languages: {lang_created}", flush=True)

    nf.commit()
    print(f"  Languages created: {lang_created}, glosses: {lang_glosses}", flush=True)

    # === ASTRONOMICAL (xA places) ===
    print("\n=== ASTRONOMICAL ===", flush=True)
    xA_counter = get_counter(nf_cur, 'xA')
    print(f"  xA counter starts at: {xA_counter}", flush=True)

    eng_cur.execute("""
        SELECT DISTINCT e.id, e.word, e.token_id
        FROM kk_entries e
        JOIN kk_senses s ON s.entry_id = e.id
        JOIN kk_sense_categories sc ON sc.sense_id = s.id
        WHERE e.token_id LIKE 'AD.AA.%%'
          AND e.pos = 'name'
          AND array_length(string_to_array(e.word, ' '), 1) = 1
          AND (sc.name LIKE '%%Planets%%' OR sc.name LIKE '%%Constellations%%')
        ORDER BY e.word
    """)
    astro_entries = eng_cur.fetchall()

    astro_created = 0
    astro_glosses = 0
    for entry_id, word, label_tid in astro_entries:
        name_lower = word.lower().replace(' ', '_')
        if ('xA', name_lower) in existing:
            continue

        p4 = encode_pair(xA_counter // 2500)
        p5 = encode_pair(xA_counter % 2500)
        xA_counter += 1

        # Determine planet vs constellation
        eng_cur.execute("""
            SELECT DISTINCT sc.name FROM kk_senses s
            JOIN kk_sense_categories sc ON sc.sense_id = s.id
            WHERE s.entry_id = %s AND (sc.name LIKE '%%Planets%%' OR sc.name LIKE '%%Constellations%%')
        """, (entry_id,))
        cats = [r[0] for r in eng_cur.fetchall()]
        subcat = 'constellation' if any('Constellation' in c for c in cats) else 'planet'

        g = create_entity(eng_cur, nf_cur, entry_id, word, label_tid, 'xA', p4, p5, 'place', subcat)
        astro_glosses += g
        existing.add(('xA', name_lower))
        astro_created += 1

    nf.commit()
    print(f"  Astronomical created: {astro_created}, glosses: {astro_glosses}", flush=True)

    # === HOLIDAYS (wA things) ===
    print("\n=== HOLIDAYS ===", flush=True)
    wA_counter = get_counter(nf_cur, 'wA')  # refresh after languages

    eng_cur.execute("""
        SELECT DISTINCT e.id, e.word, e.token_id
        FROM kk_entries e
        JOIN kk_senses s ON s.entry_id = e.id
        JOIN kk_sense_categories sc ON sc.sense_id = s.id
        WHERE e.token_id LIKE 'AD.AA.%%'
          AND e.pos = 'name'
          AND array_length(string_to_array(e.word, ' '), 1) = 1
          AND sc.name LIKE '%%Holidays%%'
        ORDER BY e.word
    """)
    holiday_entries = eng_cur.fetchall()

    hol_created = 0
    hol_glosses = 0
    for entry_id, word, label_tid in holiday_entries:
        name_lower = word.lower().replace(' ', '_')
        if ('wA', name_lower) in existing:
            continue

        p4 = encode_pair(wA_counter // 2500)
        p5 = encode_pair(wA_counter % 2500)
        wA_counter += 1

        g = create_entity(eng_cur, nf_cur, entry_id, word, label_tid, 'wA', p4, p5, 'thing', 'holiday')
        hol_glosses += g
        existing.add(('wA', name_lower))
        hol_created += 1

    nf.commit()

    # === SUMMARY ===
    print(f"\n{'='*60}", flush=True)
    print(f"NF OTHER ENTITIES CREATED", flush=True)
    print(f"{'='*60}", flush=True)
    print(f"  Languages: {lang_created} (glosses: {lang_glosses})", flush=True)
    print(f"  Astronomical: {astro_created} (glosses: {astro_glosses})", flush=True)
    print(f"  Holidays: {hol_created} (glosses: {hol_glosses})", flush=True)

    nf_cur.execute("SELECT ns, category, subcategory, count(*) FROM tokens GROUP BY 1,2,3 ORDER BY 1,4 DESC")
    print("\nFull nf_entities breakdown:")
    for ns, cat, subcat, count in nf_cur.fetchall():
        print(f"  {ns} {cat}/{subcat}: {count}", flush=True)

    eng_cur.close()
    nf_cur.close()
    eng.close()
    nf.close()


if __name__ == '__main__':
    main()
