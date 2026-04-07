#!/usr/bin/env python3
"""
Create nonfiction people entities from single-word AD.AA Labels in hcp_english
that have person/mythology/religious categories.

For each entity:
1. Create token in nf_entities.tokens (yA namespace, sequential)
2. Create entity_names row linking entity → AD.AA Label token
3. Move tokenized glosses to entity_descriptions
4. Store relevant categories as entity_properties
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


def encode_counter(counter):
    p4 = counter // 2500
    p5 = counter % 2500
    return encode_pair(p4), encode_pair(p5)


def main():
    eng = psycopg2.connect(host='localhost', user='hcp', password='hcp_dev', dbname='hcp_english')
    nf = psycopg2.connect(host='localhost', user='hcp', password='hcp_dev', dbname='hcp_nf_entities')
    eng_cur = eng.cursor()
    nf_cur = nf.cursor()

    # --- Find current max sequential in yA ---
    nf_cur.execute("SELECT max(p5) FROM tokens WHERE ns = 'yA' AND p2 = 'AA' AND p3 = 'AA' AND p4 = 'AA'")
    row = nf_cur.fetchone()
    if row and row[0]:
        counter = decode_pair(row[0]) + 1
    else:
        counter = 0

    # Check if there are entries with p4 beyond AA
    nf_cur.execute("SELECT max(p4), max(p5) FROM tokens WHERE ns = 'yA'")
    row = nf_cur.fetchone()
    max_p4 = row[0] if row else 'AA'
    max_p5 = row[1] if row else 'AA'
    if max_p4 and max_p4 != 'AA':
        counter = decode_pair(max_p4) * 2500 + decode_pair(max_p5) + 1
    elif max_p5:
        counter = decode_pair(max_p5) + 1

    print(f"Starting sequential counter at: {counter}", flush=True)

    # --- Get all nf people from english shard ---
    eng_cur.execute("""
        SELECT DISTINCT e.id, e.word, e.token_id
        FROM kk_entries e
        WHERE e.token_id LIKE 'AD.AA.%%'
          AND e.pos = 'name'
          AND array_length(string_to_array(e.word, ' '), 1) = 1
          AND EXISTS (
            SELECT 1 FROM kk_senses s JOIN kk_sense_categories sc ON sc.sense_id=s.id
            WHERE s.entry_id=e.id
            AND (sc.name IN ('People','Individuals','en:Individuals')
                 OR sc.name LIKE '%%mythology%%'
                 OR sc.name LIKE '%%biblical%%')
          )
        ORDER BY e.word
    """)
    entries = eng_cur.fetchall()
    print(f"Entries to process: {len(entries)}", flush=True)

    # --- Check which already exist in nf_entities (by name match) ---
    nf_cur.execute("SELECT name FROM tokens WHERE ns = 'yA'")
    existing_names = {row[0] for row in nf_cur.fetchall()}
    print(f"Already in nf_entities: {len(existing_names)}", flush=True)

    created = 0
    skipped = 0
    glosses_moved = 0
    properties_added = 0

    for entry_id, word, label_tid in entries:
        name_lower = word.lower().replace(' ', '_')

        # Skip if already exists
        if name_lower in existing_names:
            skipped += 1
            continue

        # --- Mint entity token ---
        p4, p5 = encode_counter(counter)
        entity_tid = f"yA.AA.AA.{p4}.{p5}"
        counter += 1

        # Determine subcategory from Kaikki categories
        eng_cur.execute("""
            SELECT DISTINCT sc.name
            FROM kk_senses s
            JOIN kk_sense_categories sc ON sc.sense_id = s.id
            WHERE s.entry_id = %s
            AND (sc.name IN ('People','Individuals','en:Individuals','Fictional characters',
                             'Nicknames for individuals','en:Nicknames for individuals')
                 OR sc.name LIKE '%%mythology%%'
                 OR sc.name LIKE '%%biblical%%')
        """, (entry_id,))
        cats = [row[0] for row in eng_cur.fetchall()]

        # Primary category
        subcategory = None
        for c in cats:
            if 'mythology' in c.lower():
                subcategory = c.replace('en:', '')
                break
            if 'biblical' in c.lower():
                subcategory = 'Biblical'
                break
        if not subcategory:
            for c in cats:
                if c in ('Individuals', 'en:Individuals', 'People'):
                    subcategory = 'Historical person'
                    break
        if not subcategory:
            subcategory = 'person'

        # --- Insert token ---
        nf_cur.execute("""
            INSERT INTO tokens (ns, p2, p3, p4, p5, name, category, subcategory, metadata)
            VALUES ('yA', 'AA', 'AA', %s, %s, %s, 'person', %s, '{}')
        """, (p4, p5, name_lower, subcategory))

        # --- Insert entity_names (link entity → Label token) ---
        # Parse the AD.AA label token_id into components
        parts = label_tid.split('.')
        if len(parts) == 5:
            nf_cur.execute("""
                INSERT INTO entity_names (entity_ns, entity_p2, entity_p3, entity_p4, entity_p5,
                                          name_group, name_type, position,
                                          ns, p2, p3, p4, p5)
                VALUES ('yA', 'AA', 'AA', %s, %s,
                        0, 'primary', 1,
                        %s, %s, %s, %s, %s)
            """, (p4, p5, parts[0], parts[1], parts[2], parts[3], parts[4]))

        # --- Move glosses to entity_descriptions ---
        eng_cur.execute("""
            SELECT s.tokenized_gloss
            FROM kk_senses s
            WHERE s.entry_id = %s AND s.tokenized_gloss IS NOT NULL
        """, (entry_id,))

        for (tg,) in eng_cur.fetchall():
            if tg:
                # Store as tokenized description
                gloss_str = ','.join(tg)
                nf_cur.execute("""
                    INSERT INTO entity_descriptions (entity_ns, entity_p2, entity_p3, entity_p4, entity_p5,
                                                     description_type, description)
                    VALUES ('yA', 'AA', 'AA', %s, %s, 'gloss', %s)
                """, (p4, p5, gloss_str))
                glosses_moved += 1

        # --- Store categories as properties ---
        for cat in cats:
            nf_cur.execute("""
                INSERT INTO entity_properties (entity_ns, entity_p2, entity_p3, entity_p4, entity_p5,
                                               key, value)
                VALUES ('yA', 'AA', 'AA', %s, %s, 'category', %s)
                ON CONFLICT DO NOTHING
            """, (p4, p5, cat))
            properties_added += 1

        # Also store any tradition-specific categories
        eng_cur.execute("""
            SELECT DISTINCT sc.name
            FROM kk_senses s
            JOIN kk_sense_categories sc ON sc.sense_id = s.id
            WHERE s.entry_id = %s
            AND sc.name NOT LIKE 'English%%'
            AND sc.name NOT LIKE 'Terms%%'
            AND sc.name NOT LIKE 'Pages%%'
            AND (sc.name LIKE '%%mythology%%' OR sc.name LIKE '%%religion%%'
                 OR sc.name LIKE '%%biblical%%' OR sc.name LIKE '%%Individuals%%'
                 OR sc.name LIKE 'People')
        """, (entry_id,))
        for (extra_cat,) in eng_cur.fetchall():
            nf_cur.execute("""
                INSERT INTO entity_properties (entity_ns, entity_p2, entity_p3, entity_p4, entity_p5,
                                               key, value)
                VALUES ('yA', 'AA', 'AA', %s, %s, 'tradition', %s)
                ON CONFLICT DO NOTHING
            """, (p4, p5, extra_cat))

        existing_names.add(name_lower)
        created += 1

        if created % 200 == 0:
            nf.commit()
            print(f"  Created: {created}, glosses: {glosses_moved}, properties: {properties_added}", flush=True)

    nf.commit()

    print(f"\n{'='*60}", flush=True)
    print(f"NF PEOPLE ENTITIES CREATED", flush=True)
    print(f"{'='*60}", flush=True)
    print(f"  Created: {created}", flush=True)
    print(f"  Skipped (existing): {skipped}", flush=True)
    print(f"  Glosses moved: {glosses_moved}", flush=True)
    print(f"  Properties added: {properties_added}", flush=True)

    nf_cur.execute("SELECT category, subcategory, count(*) FROM tokens WHERE ns = 'yA' GROUP BY 1, 2 ORDER BY 3 DESC LIMIT 20")
    print("\nToken breakdown:")
    for cat, subcat, count in nf_cur.fetchall():
        print(f"  {cat}/{subcat}: {count}", flush=True)

    eng_cur.close()
    nf_cur.close()
    eng.close()
    nf.close()


if __name__ == '__main__':
    main()
