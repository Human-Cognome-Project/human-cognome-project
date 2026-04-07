#!/usr/bin/env python3
"""
Broad sweep: find all single-word AD.AA Labels with entity-specific glosses.
Exclude generic glosses (given name, surname, place in X, village in X, etc.)
Create entity records for everything that survives the filter.

Exclusion-based approach: if ALL of an entry's glosses are generic, skip it.
If ANY gloss is entity-specific, it gets an entity record.
"""

import psycopg2

B50 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwx"

# Tokens that start generic glosses — "A surname", "A village in", etc.
# token_id for key words that indicate generic name/place glosses
GENERIC_STARTS = {
    'AB.AA.SH.AD.ZQ',  # surname
    'AB.AA.VH.AA.QX',  # village
    'AB.AA.TE.AA.UP',  # town
    'AB.AA.PF.AA.YC',  # place
    'AB.AA.BI.AA.Ua',  # barangay
    'AB.AA.LI.AB.AI',  # locality
    'AB.AA.UO.AA.ZR',  # unincorporated
    'AB.AA.PJ.AB.tA',  # placename
    'AB.AA.NG.AA.eC',  # number
}

# Second position tokens that confirm generic pattern
GENERIC_SECONDS = {
    'AD.AA.GF.AA.Gt',  # Given (as in "given name")
    'AD.AA.PG.AA.ZW',  # Proper (as in "proper noun")
}

# Full 3-token patterns that are generic
GENERIC_PATTERNS_3 = {
    # "A surname."
    ('AB.AA.AB.AA.AU', 'AB.AA.SH.AD.ZQ', '[.]'),
    # "alternative form of"
    ('AB.AA.AL.AB.Bb', 'AB.AA.FE.AA.Qm', 'AD.AA.aC.AA.AK'),
}


def is_generic_gloss(tokenized_gloss):
    """Check if a tokenized gloss is a generic name/place description."""
    if not tokenized_gloss or len(tokenized_gloss) < 2:
        return True  # too short to be meaningful

    tg = tokenized_gloss

    # "A surname/village/town/place/barangay/locality/unincorporated/placename/number..."
    if tg[0] == 'AB.AA.AB.AA.AU' and len(tg) > 1 and tg[1] in GENERIC_STARTS:
        return True

    # "A male/female given name..."
    if tg[0] == 'AB.AA.AB.AA.AU' and len(tg) > 2:
        if tg[1] in ('AB.AA.ME.AA.Cd', 'AB.AA.FG.AA.Lb') and tg[2] == 'AD.AA.GF.AA.Gt':
            return True  # "A male/female given name"

    # "Proper noun component..."
    if len(tg) >= 2 and tg[0] == 'AD.AA.PG.AA.ZW' and tg[1] == 'AB.AA.NE.AA.BY':
        return True

    # "A City in..." or "City in..."
    if tg[0] == 'AD.AA.CE.AA.GC' and len(tg) > 1 and tg[1] == 'AD.AA.IC.AA.AR':
        return True
    if tg[0] == 'AB.AA.AB.AA.AU' and len(tg) > 1 and tg[1] == 'AD.AA.CE.AA.GC':
        return True

    # "alternative form/spelling of..."
    if tg[0] == 'AB.AA.AL.AB.Bb':
        return True

    # Starts with a place-type word directly
    if tg[0] in GENERIC_STARTS:
        return True

    return False


def encode_pair(value):
    if value < 0: value = 0
    if value >= 2500: value = 2499
    return B50[value // 50] + B50[value % 50]


def decode_pair(s):
    c0 = (ord(s[0]) - ord('A')) if s[0].isupper() else (26 + ord(s[0]) - ord('a'))
    c1 = (ord(s[1]) - ord('A')) if s[1].isupper() else (26 + ord(s[1]) - ord('a'))
    return c0 * 50 + c1


def get_existing_token_ids(cur, ns):
    """Get all existing token_ids for a namespace as a set."""
    cur.execute("SELECT token_id FROM tokens WHERE ns = %s", (ns,))
    return {r[0] for r in cur.fetchall()}


def get_counter(cur, ns):
    """Get next safe counter by finding max across all entries."""
    cur.execute("SELECT count(*) FROM tokens WHERE ns = %s", (ns,))
    count = cur.fetchone()[0]
    # Start well past any existing entries
    return max(count, 0)


def counter_to_parts(counter):
    """Convert flat counter back to p2, p3, p4, p5 pairs."""
    v5 = counter % 2500
    rest = counter // 2500
    v4 = rest % 2500
    rest = rest // 2500
    v3 = rest % 2500
    v2 = rest // 2500
    return encode_pair(v2), encode_pair(v3), encode_pair(v4), encode_pair(v5)


def determine_entity_db_and_category(cats):
    """Decide nf vs fic and category based on sense categories."""
    cat_str = ' '.join(cats).lower()

    # Fictional → fic
    if 'fictional' in cat_str:
        return 'fic', 'person', 'fictional_character'

    # Religious/spiritual
    if any(x in cat_str for x in ['buddha', 'bodhisattva', 'islam', 'hindu', 'christian',
                                    'jewish', 'bible', 'biblical', 'god', 'gods', 'deity',
                                    'religion', 'saint']):
        return 'nf', 'person', 'religious'

    # Mythology
    if 'mythology' in cat_str:
        tradition = 'mythology'
        for c in cats:
            if 'mythology' in c.lower():
                tradition = c.replace('en:', '')
                break
        return 'nf', 'person', tradition

    # Astronomy/astrology
    if any(x in cat_str for x in ['planet', 'constellation', 'asteroid', 'astronomy', 'astrology']):
        return 'nf', 'place', 'astronomical'

    # Languages
    if 'languages' in cat_str or 'language' in cat_str:
        return 'nf', 'thing', 'language'

    # Organizations
    if 'organization' in cat_str:
        return 'nf', 'thing', 'organization'

    # Computing/technology
    if any(x in cat_str for x in ['computing', 'software', 'programming']):
        return 'nf', 'thing', 'technology'

    # Science
    if any(x in cat_str for x in ['geology', 'chemistry', 'biology', 'physics', 'medicine',
                                    'botany', 'zoology', 'anatomy']):
        return 'nf', 'thing', 'science'

    # Calendar/time
    if any(x in cat_str for x in ['calendar', 'month', 'holiday']):
        return 'nf', 'thing', 'temporal'

    # People (individuals, historical)
    if any(x in cat_str for x in ['individual', 'people', 'person', 'historical']):
        return 'nf', 'person', 'historical'

    # Ethnonyms
    if 'ethnonym' in cat_str:
        return 'nf', 'thing', 'ethnonym'

    # Default: nf thing
    return 'nf', 'thing', 'other'


def main():
    eng = psycopg2.connect(host='localhost', user='hcp', password='hcp_dev', dbname='hcp_english')
    nf = psycopg2.connect(host='localhost', user='hcp', password='hcp_dev', dbname='hcp_nf_entities')
    fic = psycopg2.connect(host='localhost', user='hcp', password='hcp_dev', dbname='hcp_fic_entities')
    ec = eng.cursor()
    nfc = nf.cursor()
    fc = fic.cursor()

    # Load existing entity names and token_ids
    nfc.execute("SELECT ns, name FROM tokens")
    nf_existing = {(r[0], r[1]) for r in nfc.fetchall()}
    fc.execute("SELECT ns, name FROM tokens")
    fic_existing = {(r[0], r[1]) for r in fc.fetchall()}

    nfc.execute("SELECT token_id FROM tokens")
    nf_tids = {r[0] for r in nfc.fetchall()}
    fc.execute("SELECT token_id FROM tokens")
    fic_tids = {r[0] for r in fc.fetchall()}
    existing_token_ids = {'nf': nf_tids, 'fic': fic_tids}

    print(f"Existing: nf={len(nf_existing)}, fic={len(fic_existing)}", flush=True)

    # Get counters
    nf_counters = {
        'yA': get_counter(nfc, 'yA'),  # people
        'xA': get_counter(nfc, 'xA'),  # places
        'wA': get_counter(nfc, 'wA'),  # things
    }
    fic_counters = {
        'uA': get_counter(fc, 'uA'),   # people/creatures
        'tA': get_counter(fc, 'tA'),   # places
        'sA': get_counter(fc, 'sA'),   # things
    }

    NS_MAP = {
        ('nf', 'person'): 'yA',
        ('nf', 'place'): 'xA',
        ('nf', 'thing'): 'wA',
        ('fic', 'person'): 'uA',
        ('fic', 'place'): 'tA',
        ('fic', 'thing'): 'sA',
    }

    # Get ALL single-word AD.AA Labels with glosses
    ec.execute("""
        SELECT e.id, e.word, e.token_id
        FROM kk_entries e
        WHERE e.token_id LIKE 'AD.AA.%%'
          AND e.pos = 'name'
          AND array_length(string_to_array(e.word, ' '), 1) = 1
          AND EXISTS (SELECT 1 FROM kk_senses s WHERE s.entry_id = e.id AND s.tokenized_gloss IS NOT NULL)
        ORDER BY e.word
    """)
    all_entries = ec.fetchall()
    print(f"Total Labels with glosses: {len(all_entries)}", flush=True)

    created_nf = 0
    created_fic = 0
    skipped_generic = 0
    skipped_existing = 0
    glosses_moved = 0

    for entry_id, word, label_tid in all_entries:
        name_lower = word.lower().replace(' ', '_')

        # Get all glosses for this entry
        ec.execute("""
            SELECT s.id, s.tokenized_gloss
            FROM kk_senses s
            WHERE s.entry_id = %s AND s.tokenized_gloss IS NOT NULL
        """, (entry_id,))
        senses = ec.fetchall()

        # Check if ANY gloss is non-generic
        has_entity_gloss = False
        entity_glosses = []
        for sense_id, tg in senses:
            if not is_generic_gloss(tg):
                has_entity_gloss = True
                entity_glosses.append((sense_id, tg))

        if not has_entity_gloss:
            skipped_generic += 1
            continue

        # Get categories for classification
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

        db_type, category, subcategory = determine_entity_db_and_category(cats)
        ns = NS_MAP[(db_type, category)]

        # Check if exists
        existing_set = nf_existing if db_type == 'nf' else fic_existing
        if (ns, name_lower) in existing_set:
            skipped_existing += 1
            continue

        # Get counter and cursor
        counters = nf_counters if db_type == 'nf' else fic_counters
        cur = nfc if db_type == 'nf' else fc
        conn = nf if db_type == 'nf' else fic

        counter = counters[ns]
        existing_tids = existing_token_ids[db_type]

        # Find a non-colliding token_id
        while True:
            p2, p3, p4, p5 = counter_to_parts(counter)
            tid = f"{ns}.{p2}.{p3}.{p4}.{p5}"
            if tid not in existing_tids:
                break
            counter += 1

        counters[ns] = counter + 1
        existing_tids.add(tid)

        # Insert token
        cur.execute("""
            INSERT INTO tokens (ns, p2, p3, p4, p5, name, category, subcategory, metadata)
            VALUES (%s, %s, %s, %s, %s, %s, %s, %s, '{}')
        """, (ns, p2, p3, p4, p5, name_lower, category, subcategory))

        # Insert entity_names
        parts = label_tid.split('.')
        if len(parts) == 5:
            cur.execute("""
                INSERT INTO entity_names (entity_ns, entity_p2, entity_p3, entity_p4, entity_p5,
                                          name_group, name_type, position,
                                          ns, p2, p3, p4, p5)
                VALUES (%s, %s, %s, %s, %s, 0, 'primary', 1, %s, %s, %s, %s, %s)
            """, (ns, p2, p3, p4, p5, parts[0], parts[1], parts[2], parts[3], parts[4]))

        # Move entity-specific glosses only
        for sense_id, tg in entity_glosses:
            gloss_str = ','.join(tg)
            cur.execute("""
                INSERT INTO entity_descriptions (entity_ns, entity_p2, entity_p3, entity_p4, entity_p5,
                                                 description_type, description)
                VALUES (%s, %s, %s, %s, %s, 'gloss', %s)
            """, (ns, p2, p3, p4, p5, gloss_str))
            glosses_moved += 1

        existing_set.add((ns, name_lower))
        if db_type == 'nf':
            created_nf += 1
        else:
            created_fic += 1

        total = created_nf + created_fic
        if total % 1000 == 0:
            nf.commit()
            fic.commit()
            print(f"  Progress: nf={created_nf}, fic={created_fic}, skipped_generic={skipped_generic}, skipped_existing={skipped_existing}", flush=True)

    nf.commit()
    fic.commit()

    print(f"\n{'='*60}", flush=True)
    print(f"ENTITY SWEEP COMPLETE", flush=True)
    print(f"{'='*60}", flush=True)
    print(f"  Created NF: {created_nf}", flush=True)
    print(f"  Created FIC: {created_fic}", flush=True)
    print(f"  Skipped (generic): {skipped_generic}", flush=True)
    print(f"  Skipped (existing): {skipped_existing}", flush=True)
    print(f"  Glosses moved: {glosses_moved}", flush=True)

    print("\nNF breakdown:")
    nfc.execute("SELECT ns, category, subcategory, count(*) FROM tokens GROUP BY 1,2,3 ORDER BY 1,4 DESC")
    for ns, cat, subcat, count in nfc.fetchall():
        print(f"  {ns} {cat}/{subcat}: {count}", flush=True)

    print("\nFIC breakdown:")
    fc.execute("SELECT ns, category, subcategory, count(*) FROM tokens GROUP BY 1,2,3 ORDER BY 1,4 DESC")
    for ns, cat, subcat, count in fc.fetchall():
        print(f"  {ns} {cat}/{subcat}: {count}", flush=True)

    ec.close(); nfc.close(); fc.close()
    eng.close(); nf.close(); fic.close()


if __name__ == '__main__':
    main()
