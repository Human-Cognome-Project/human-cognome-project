#!/usr/bin/env python3
"""
Load Kaikki JSONL into structured Postgres tables using psycopg2.
Every entry gets a row. Everything relevant extracted. No raw JSON kept.
Structural wiki categories excluded.
"""

import json
import sys
import psycopg2
from psycopg2.extras import execute_values

KAIKKI_PATH = '/opt/project/sources/data/kaikki/english.jsonl'

STRUCTURAL_PATTERNS = [
    'pages with', 'pages using', 'english links with',
    'english entries with incorrect', 'entries with',
    'rhymes:', 'requests for'
]

def is_structural(name):
    n = name.lower()
    return any(p in n for p in STRUCTURAL_PATTERNS)


def connect():
    return psycopg2.connect(host='localhost', user='hcp', password='hcp_dev', dbname='hcp_english')


def main():
    print("="*60)
    print("LOADING KAIKKI INTO STRUCTURED TABLES")
    print("="*60)

    conn = connect()
    conn.autocommit = False
    cur = conn.cursor()

    loaded = 0
    skipped = 0
    errors = 0

    with open(KAIKKI_PATH) as f:
        for line_no, line in enumerate(f):
            if line_no % 1000 == 0 and line_no > 0:
                conn.commit()
                if line_no % 5000 == 0:
                    print(f"  Line {line_no}: loaded {loaded}, skipped {skipped}, errors {errors}", flush=True)

            try:
                entry = json.loads(line)
            except:
                errors += 1
                continue

            word = entry.get('word', '').strip()
            pos = entry.get('pos', '')
            if not word or not pos:
                skipped += 1
                continue

            etym_num = entry.get('etymology_number', 0) or 0
            etym_text = (entry.get('etymology_text') or '')[:1000]

            # Check deprecated/obsolete
            all_tags = set()
            for sense in entry.get('senses', []):
                for tag in sense.get('tags', []):
                    all_tags.add(tag.lower())
            is_deprecated = 'deprecated' in all_tags

            # form-of / alt-of
            form_of_word = None
            form_of_tags = None
            alt_of_word = None
            alt_of_tags = None

            for sense in entry.get('senses', []):
                tags = sense.get('tags', [])
                if 'form-of' in tags and not form_of_word:
                    fo = sense.get('form_of', [])
                    if fo and isinstance(fo, list) and fo[0].get('word'):
                        form_of_word = fo[0]['word'].strip()
                        form_of_tags = [t for t in tags if t != 'form-of']
                if 'alt-of' in tags and not alt_of_word:
                    ao = sense.get('alt_of', sense.get('form_of', []))
                    if ao and isinstance(ao, list) and len(ao) > 0:
                        w = ao[0].get('word', '').strip() if isinstance(ao[0], dict) else ''
                        if w:
                            alt_of_word = w
                            alt_of_tags = [t for t in tags if t != 'alt-of']

            try:
                # Insert entry
                cur.execute("""
                    INSERT INTO kk_entries (word, pos, etymology_number, etymology_text, is_deprecated,
                                           form_of_word, form_of_tags, alt_of_word, alt_of_tags)
                    VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s)
                    ON CONFLICT (word, pos, etymology_number) DO NOTHING
                    RETURNING id
                """, (word, pos, etym_num, etym_text, is_deprecated,
                      form_of_word, form_of_tags, alt_of_word, alt_of_tags))

                row = cur.fetchone()
                if not row:
                    skipped += 1
                    continue
                entry_id = row[0]

                # Forms
                form_rows = []
                for fd in entry.get('forms', []):
                    fw = fd.get('form', '').strip()
                    if not fw or fw in ('no-table-tags', 'glossary'):
                        continue
                    ftags = fd.get('tags', [])
                    if 'table-tags' in ftags or 'inflection-template' in ftags:
                        continue
                    source = fd.get('source', 'head')
                    form_rows.append((entry_id, fw, ftags, source))

                if form_rows:
                    execute_values(cur, """
                        INSERT INTO kk_forms (entry_id, form, tags, source) VALUES %s
                    """, form_rows, template="(%s, %s, %s::text[], %s)")

                # Senses
                for i, sense in enumerate(entry.get('senses', []), 1):
                    glosses = sense.get('glosses', [])
                    if not glosses:
                        continue

                    gloss = glosses[0][:1000] if isinstance(glosses[0], str) else str(glosses[0])[:1000]
                    raw_glosses = sense.get('raw_glosses', [])
                    raw_gloss = raw_glosses[0][:1000] if raw_glosses and isinstance(raw_glosses[0], str) else None
                    tags = sense.get('tags', [])
                    topics = sense.get('topics', [])
                    wikidata = sense.get('wikidata', [None])
                    wd = wikidata[0] if wikidata and isinstance(wikidata, list) else None
                    senseid = sense.get('senseid', sense.get('id'))
                    if senseid and not isinstance(senseid, str):
                        senseid = str(senseid)

                    cur.execute("""
                        INSERT INTO kk_senses (entry_id, sense_num, gloss, raw_gloss, tags, topics, wikidata_id, sense_id)
                        VALUES (%s, %s, %s, %s, %s, %s, %s, %s)
                        ON CONFLICT DO NOTHING
                        RETURNING id
                    """, (entry_id, i, gloss, raw_gloss, tags, topics, wd, senseid))

                    srow = cur.fetchone()
                    if not srow:
                        continue
                    sense_db_id = srow[0]

                    # Sense categories
                    cat_rows = []
                    for cat in sense.get('categories', []):
                        if isinstance(cat, dict):
                            name = cat.get('name', '')
                            if name and not is_structural(name):
                                cat_rows.append((sense_db_id, name, cat.get('kind', ''), cat.get('source', '')))

                    if cat_rows:
                        execute_values(cur, """
                            INSERT INTO kk_sense_categories (sense_id, name, kind, source) VALUES %s
                        """, cat_rows)

                    # Sense examples
                    ex_rows = []
                    for ex in sense.get('examples', []):
                        text = ex.get('text', '').strip()
                        if text:
                            ex_rows.append((sense_db_id, text[:2000], (ex.get('ref') or '')[:500], ex.get('type', 'example')))

                    if ex_rows:
                        execute_values(cur, """
                            INSERT INTO kk_sense_examples (sense_id, text, ref, type) VALUES %s
                        """, ex_rows)

                    # Sense-level relations
                    for rel_type in ['synonyms', 'antonyms', 'hyponyms', 'hypernyms', 'related', 'derived', 'meronyms', 'holonyms', 'coordinate_terms']:
                        for rel in sense.get(rel_type, []):
                            rword = rel.get('word', '').strip()
                            if not rword:
                                continue
                            rtype = rel_type.rstrip('s')
                            if rtype == 'coordinate_term':
                                rtype = 'coordinate'
                            rtags = rel.get('tags', [])
                            sense_ctx = rel.get('sense', '')
                            cur.execute("""
                                INSERT INTO kk_relations (entry_id, sense_id, relation_type, target_word, tags, sense_context)
                                VALUES (%s, %s, %s, %s, %s, %s)
                            """, (entry_id, sense_db_id, rtype, rword, rtags, sense_ctx))

                # Entry-level relations
                rel_rows = []
                for rel_type in ['synonyms', 'antonyms', 'hyponyms', 'hypernyms', 'related', 'derived', 'meronyms', 'holonyms', 'coordinate_terms']:
                    for rel in entry.get(rel_type, []):
                        rword = rel.get('word', '').strip()
                        if not rword:
                            continue
                        rtype = rel_type.rstrip('s')
                        if rtype == 'coordinate_term':
                            rtype = 'coordinate'
                        rtags = rel.get('tags', [])
                        sense_ctx = rel.get('sense', '')
                        rel_rows.append((entry_id, None, rtype, rword, None, rtags, sense_ctx))

                # Descendants
                for desc in entry.get('descendants', []):
                    dword = desc.get('word', '').strip()
                    if dword:
                        dlang = desc.get('lang', '')
                        dtags = desc.get('raw_tags', desc.get('tags', []))
                        if not isinstance(dtags, list):
                            dtags = [str(dtags)]
                        rel_rows.append((entry_id, None, 'descendant', dword, dlang, dtags, ''))

                if rel_rows:
                    execute_values(cur, """
                        INSERT INTO kk_relations (entry_id, sense_id, relation_type, target_word, target_lang, tags, sense_context)
                        VALUES %s
                    """, rel_rows, template="(%s, %s, %s, %s, %s, %s::text[], %s)")

                # Translations
                trans_rows = []
                for trans in entry.get('translations', []):
                    tword = trans.get('word', '').strip()
                    if not tword:
                        continue
                    tlang = trans.get('lang', '')
                    tcode = trans.get('lang_code', trans.get('code', ''))
                    tsense = trans.get('sense', '')
                    ttags = trans.get('tags', [])
                    troman = trans.get('roman')
                    trans_rows.append((entry_id, tlang, tcode, tword, tsense, ttags, troman))

                if trans_rows:
                    execute_values(cur, """
                        INSERT INTO kk_translations (entry_id, lang, lang_code, word, sense, tags, roman)
                        VALUES %s
                    """, trans_rows, template="(%s, %s, %s, %s, %s, %s::text[], %s)")

                # Sounds
                sound_rows = []
                for sound in entry.get('sounds', []):
                    ipa = sound.get('ipa')
                    enpr = sound.get('enpr')
                    stags = sound.get('tags', [])
                    if ipa or enpr:
                        sound_rows.append((entry_id, ipa, enpr, stags))

                if sound_rows:
                    execute_values(cur, """
                        INSERT INTO kk_sounds (entry_id, ipa, enpr, tags) VALUES %s
                    """, sound_rows, template="(%s, %s, %s, %s::text[])")

                loaded += 1

            except Exception as e:
                conn.rollback()
                errors += 1
                if errors <= 5:
                    print(f"  Error on line {line_no} ({word}/{pos}): {e}", file=sys.stderr)
                continue

    conn.commit()
    cur.close()

    print(f"\n{'='*60}")
    print("LOAD COMPLETE")
    print(f"{'='*60}")
    print(f"Loaded: {loaded}")
    print(f"Skipped: {skipped}")
    print(f"Errors: {errors}")

    cur = conn.cursor()
    for table in ['kk_entries', 'kk_forms', 'kk_senses', 'kk_sense_categories',
                   'kk_sense_examples', 'kk_relations', 'kk_translations', 'kk_sounds']:
        cur.execute(f"SELECT count(*) FROM {table}")
        print(f"  {table}: {cur.fetchone()[0]}")
    cur.close()
    conn.close()


if __name__ == '__main__':
    main()
