#!/usr/bin/env python3
"""
Create nonfiction place entities for countries and continents.

These are the only single-word place Labels that unambiguously identify
one Thing. Cities, towns, landmarks all need compound forms to disambiguate.

For each entity:
1. Create token in nf_entities.tokens (xA namespace, sequential)
2. Create entity_names row linking entity → AD.AA Label token
3. Move tokenized glosses to entity_descriptions
4. Store categories as entity_properties
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


def main():
    eng = psycopg2.connect(host='localhost', user='hcp', password='hcp_dev', dbname='hcp_english')
    nf = psycopg2.connect(host='localhost', user='hcp', password='hcp_dev', dbname='hcp_nf_entities')
    eng_cur = eng.cursor()
    nf_cur = nf.cursor()

    # --- Find current max sequential in xA ---
    nf_cur.execute("SELECT max(p4), max(p5) FROM tokens WHERE ns = 'xA'")
    row = nf_cur.fetchone()
    max_p4 = row[0] if row and row[0] else 'AA'
    max_p5 = row[1] if row and row[1] else 'AA'
    if max_p4 != 'AA':
        counter = decode_pair(max_p4) * 2500 + decode_pair(max_p5) + 1
    else:
        nf_cur.execute("SELECT count(*) FROM tokens WHERE ns = 'xA'")
        counter = nf_cur.fetchone()[0]

    print(f"Starting sequential counter at: {counter}", flush=True)

    # --- Get countries and continents ---
    eng_cur.execute("""
        SELECT DISTINCT e.id, e.word, e.token_id
        FROM kk_entries e
        JOIN kk_senses s ON s.entry_id = e.id
        JOIN kk_sense_categories sc ON sc.sense_id = s.id
        WHERE e.token_id LIKE 'AD.AA.%%'
          AND e.pos = 'name'
          AND array_length(string_to_array(e.word, ' '), 1) = 1
          AND (sc.name LIKE '%%Countries%%' OR sc.name LIKE '%%Continent%%')
        ORDER BY e.word
    """)
    entries = eng_cur.fetchall()
    print(f"Country/continent entries: {len(entries)}", flush=True)

    # --- Check existing ---
    nf_cur.execute("SELECT name FROM tokens WHERE ns = 'xA'")
    existing_names = {row[0] for row in nf_cur.fetchall()}

    created = 0
    skipped = 0
    glosses_moved = 0

    for entry_id, word, label_tid in entries:
        name_lower = word.lower().replace(' ', '_')

        if name_lower in existing_names:
            skipped += 1
            continue

        # Mint entity token
        p4 = encode_pair(counter // 2500)
        p5 = encode_pair(counter % 2500)
        entity_tid = f"xA.AA.AA.{p4}.{p5}"
        counter += 1

        # Determine if country or continent
        eng_cur.execute("""
            SELECT DISTINCT sc.name
            FROM kk_senses s
            JOIN kk_sense_categories sc ON sc.sense_id = s.id
            WHERE s.entry_id = %s
            AND (sc.name LIKE '%%Countries%%' OR sc.name LIKE '%%Continent%%')
        """, (entry_id,))
        cats = [row[0] for row in eng_cur.fetchall()]

        subcategory = 'country'
        for c in cats:
            if 'continent' in c.lower():
                subcategory = 'continent'
                break

        # Insert token
        nf_cur.execute("""
            INSERT INTO tokens (ns, p2, p3, p4, p5, name, category, subcategory, metadata)
            VALUES ('xA', 'AA', 'AA', %s, %s, %s, 'place', %s, '{}')
        """, (p4, p5, name_lower, subcategory))

        # Insert entity_names
        parts = label_tid.split('.')
        if len(parts) == 5:
            nf_cur.execute("""
                INSERT INTO entity_names (entity_ns, entity_p2, entity_p3, entity_p4, entity_p5,
                                          name_group, name_type, position,
                                          ns, p2, p3, p4, p5)
                VALUES ('xA', 'AA', 'AA', %s, %s,
                        0, 'primary', 1,
                        %s, %s, %s, %s, %s)
            """, (p4, p5, parts[0], parts[1], parts[2], parts[3], parts[4]))

        # Move glosses
        eng_cur.execute("""
            SELECT s.tokenized_gloss
            FROM kk_senses s
            WHERE s.entry_id = %s AND s.tokenized_gloss IS NOT NULL
        """, (entry_id,))
        for (tg,) in eng_cur.fetchall():
            if tg:
                gloss_str = ','.join(tg)
                nf_cur.execute("""
                    INSERT INTO entity_descriptions (entity_ns, entity_p2, entity_p3, entity_p4, entity_p5,
                                                     description_type, description)
                    VALUES ('xA', 'AA', 'AA', %s, %s, 'gloss', %s)
                """, (p4, p5, gloss_str))
                glosses_moved += 1

        # Store categories as properties
        for cat in cats:
            nf_cur.execute("""
                INSERT INTO entity_properties (entity_ns, entity_p2, entity_p3, entity_p4, entity_p5,
                                               key, value)
                VALUES ('xA', 'AA', 'AA', %s, %s, 'category', %s)
                ON CONFLICT DO NOTHING
            """, (p4, p5, cat))

        existing_names.add(name_lower)
        created += 1

    nf.commit()

    print(f"\n{'='*60}", flush=True)
    print(f"NF PLACE ENTITIES CREATED", flush=True)
    print(f"{'='*60}", flush=True)
    print(f"  Created: {created}", flush=True)
    print(f"  Skipped (existing): {skipped}", flush=True)
    print(f"  Glosses moved: {glosses_moved}", flush=True)

    nf_cur.execute("SELECT subcategory, count(*) FROM tokens WHERE ns = 'xA' GROUP BY 1 ORDER BY 2 DESC")
    print("\nBreakdown:")
    for subcat, count in nf_cur.fetchall():
        print(f"  {subcat}: {count}", flush=True)

    eng_cur.close()
    nf_cur.close()
    eng.close()
    nf.close()


if __name__ == '__main__':
    main()
