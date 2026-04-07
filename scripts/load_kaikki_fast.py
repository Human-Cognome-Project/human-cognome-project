#!/usr/bin/env python3
"""
Fast Kaikki loader. Drops FK constraints, batch inserts everything,
then restores constraints. Uses sequential IDs we control instead of
RETURNING from the database.
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


def main():
    print("="*60, flush=True)
    print("FAST KAIKKI LOAD", flush=True)
    print("="*60, flush=True)

    conn = psycopg2.connect(host='localhost', user='hcp', password='hcp_dev', dbname='hcp_english')
    cur = conn.cursor()

    # Drop FK constraints for speed
    print("Dropping FK constraints...", flush=True)
    cur.execute("""
        ALTER TABLE kk_forms DROP CONSTRAINT IF EXISTS kk_forms_entry_id_fkey;
        ALTER TABLE kk_senses DROP CONSTRAINT IF EXISTS kk_senses_entry_id_fkey;
        ALTER TABLE kk_sense_categories DROP CONSTRAINT IF EXISTS kk_sense_categories_sense_id_fkey;
        ALTER TABLE kk_sense_examples DROP CONSTRAINT IF EXISTS kk_sense_examples_sense_id_fkey;
        ALTER TABLE kk_relations DROP CONSTRAINT IF EXISTS kk_relations_entry_id_fkey;
        ALTER TABLE kk_relations DROP CONSTRAINT IF EXISTS kk_relations_sense_id_fkey;
        ALTER TABLE kk_translations DROP CONSTRAINT IF EXISTS kk_translations_entry_id_fkey;
        ALTER TABLE kk_sounds DROP CONSTRAINT IF EXISTS kk_sounds_entry_id_fkey;
    """)
    conn.commit()

    # Also drop indexes temporarily
    print("Dropping indexes...", flush=True)
    cur.execute("""
        DROP INDEX IF EXISTS idx_kke_word;
        DROP INDEX IF EXISTS idx_kke_pos;
        DROP INDEX IF EXISTS idx_kke_token;
        DROP INDEX IF EXISTS idx_kke_form_of;
        DROP INDEX IF EXISTS idx_kke_alt_of;
        DROP INDEX IF EXISTS idx_kkf_entry;
        DROP INDEX IF EXISTS idx_kkf_form;
        DROP INDEX IF EXISTS idx_kks_entry;
        DROP INDEX IF EXISTS idx_kks_topics;
        DROP INDEX IF EXISTS idx_kksc_sense;
        DROP INDEX IF EXISTS idx_kksc_name;
        DROP INDEX IF EXISTS idx_kksc_kind;
        DROP INDEX IF EXISTS idx_kkse_sense;
        DROP INDEX IF EXISTS idx_kkr_entry;
        DROP INDEX IF EXISTS idx_kkr_sense;
        DROP INDEX IF EXISTS idx_kkr_type;
        DROP INDEX IF EXISTS idx_kkr_target;
        DROP INDEX IF EXISTS idx_kkt_entry;
        DROP INDEX IF EXISTS idx_kkt_lang;
        DROP INDEX IF EXISTS idx_kkt_word;
        DROP INDEX IF EXISTS idx_kkso_entry;
    """)
    conn.commit()

    # Reset sequences
    cur.execute("ALTER SEQUENCE kk_entries_id_seq RESTART WITH 1")
    cur.execute("ALTER SEQUENCE kk_senses_id_seq RESTART WITH 1")
    conn.commit()

    # We'll track IDs ourselves
    entry_id = 0
    sense_id = 0

    # Batch buffers
    BATCH = 2000
    entry_buf = []
    form_buf = []
    sense_buf = []
    cat_buf = []
    example_buf = []
    rel_buf = []
    trans_buf = []
    sound_buf = []

    loaded = 0
    skipped = 0
    errors = 0

    def flush_all():
        nonlocal entry_buf, form_buf, sense_buf, cat_buf, example_buf, rel_buf, trans_buf, sound_buf

        if entry_buf:
            execute_values(cur,
                "INSERT INTO kk_entries (id, word, pos, etymology_number, etymology_text, is_deprecated, form_of_word, form_of_tags, alt_of_word, alt_of_tags) VALUES %s ON CONFLICT (word, pos, etymology_number) DO NOTHING",
                entry_buf, template="(%s, %s, %s, %s, %s, %s, %s, %s::text[], %s, %s::text[])")
            entry_buf = []

        if form_buf:
            execute_values(cur,
                "INSERT INTO kk_forms (entry_id, form, tags, source) VALUES %s",
                form_buf, template="(%s, %s, %s::text[], %s)")
            form_buf = []

        if sense_buf:
            execute_values(cur,
                "INSERT INTO kk_senses (id, entry_id, sense_num, gloss, raw_gloss, tags, topics, wikidata_id, sense_id) VALUES %s",
                sense_buf, template="(%s, %s, %s, %s, %s, %s::text[], %s::text[], %s, %s)")
            sense_buf = []

        if cat_buf:
            execute_values(cur,
                "INSERT INTO kk_sense_categories (sense_id, name, kind, source) VALUES %s",
                cat_buf)
            cat_buf = []

        if example_buf:
            execute_values(cur,
                "INSERT INTO kk_sense_examples (sense_id, text, ref, type) VALUES %s",
                example_buf)
            example_buf = []

        if rel_buf:
            execute_values(cur,
                "INSERT INTO kk_relations (entry_id, sense_id, relation_type, target_word, target_lang, tags, sense_context) VALUES %s",
                rel_buf, template="(%s, %s, %s, %s, %s, %s::text[], %s)")
            rel_buf = []

        if trans_buf:
            execute_values(cur,
                "INSERT INTO kk_translations (entry_id, lang, lang_code, word, sense, tags, roman) VALUES %s",
                trans_buf, template="(%s, %s, %s, %s, %s, %s::text[], %s)")
            trans_buf = []

        if sound_buf:
            execute_values(cur,
                "INSERT INTO kk_sounds (entry_id, ipa, enpr, tags) VALUES %s",
                sound_buf, template="(%s, %s, %s, %s::text[])")
            sound_buf = []

        conn.commit()

    print("Loading entries...", flush=True)

    with open(KAIKKI_PATH) as f:
        for line_no, line in enumerate(f):
            if line_no % 5000 == 0 and line_no > 0:
                flush_all()
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

            # Check deprecated
            is_deprecated = False
            for sense in entry.get('senses', []):
                if 'deprecated' in sense.get('tags', []):
                    is_deprecated = True
                    break

            # form-of / alt-of
            form_of_word = None
            form_of_tags = None
            alt_of_word = None
            alt_of_tags = None

            for sense in entry.get('senses', []):
                tags = sense.get('tags', [])
                if 'form-of' in tags and not form_of_word:
                    fo = sense.get('form_of', [])
                    if fo and isinstance(fo, list) and len(fo) > 0 and isinstance(fo[0], dict):
                        form_of_word = fo[0].get('word', '').strip() or None
                        form_of_tags = [t for t in tags if t != 'form-of'] or None
                if 'alt-of' in tags and not alt_of_word:
                    ao = sense.get('alt_of', sense.get('form_of', []))
                    if ao and isinstance(ao, list) and len(ao) > 0 and isinstance(ao[0], dict):
                        alt_of_word = ao[0].get('word', '').strip() or None
                        alt_of_tags = [t for t in tags if t != 'alt-of'] or None

            entry_id += 1
            entry_buf.append((entry_id, word, pos, etym_num, etym_text, is_deprecated,
                              form_of_word, form_of_tags, alt_of_word, alt_of_tags))

            # Forms
            for fd in entry.get('forms', []):
                fw = fd.get('form', '').strip()
                if not fw or fw in ('no-table-tags', 'glossary'):
                    continue
                ftags = fd.get('tags', [])
                if 'table-tags' in ftags or 'inflection-template' in ftags:
                    continue
                form_buf.append((entry_id, fw, ftags, fd.get('source', 'head')))

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
                wd_list = sense.get('wikidata', [])
                wd = wd_list[0] if wd_list and isinstance(wd_list, list) else None
                sid = sense.get('senseid', sense.get('id'))
                if sid and not isinstance(sid, str):
                    sid = str(sid)

                sense_id += 1
                sense_buf.append((sense_id, entry_id, i, gloss, raw_gloss, tags, topics, wd, sid))

                # Sense categories
                for cat in sense.get('categories', []):
                    if isinstance(cat, dict):
                        name = cat.get('name', '')
                        if name and not is_structural(name):
                            cat_buf.append((sense_id, name, cat.get('kind', ''), cat.get('source', '')))

                # Sense examples
                for ex in sense.get('examples', []):
                    text = ex.get('text', '').strip()
                    if text:
                        cat_buf_ref = (ex.get('ref') or '')[:500]
                        example_buf.append((sense_id, text[:2000], cat_buf_ref, ex.get('type', 'example')))

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
                        rel_buf.append((entry_id, sense_id, rtype, rword, None, rtags, sense_ctx))

            # Entry-level relations
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
                    rel_buf.append((entry_id, None, rtype, rword, None, rtags, sense_ctx))

            # Descendants
            for desc in entry.get('descendants', []):
                dword = desc.get('word', '').strip()
                if dword:
                    dlang = desc.get('lang', '')
                    dtags = desc.get('raw_tags', desc.get('tags', []))
                    if not isinstance(dtags, list):
                        dtags = [str(dtags)]
                    rel_buf.append((entry_id, None, 'descendant', dword, dlang, dtags, ''))

            # Translations
            for trans in entry.get('translations', []):
                tword = trans.get('word', '').strip()
                if not tword:
                    continue
                trans_buf.append((entry_id, trans.get('lang', ''), trans.get('lang_code', trans.get('code', '')),
                                  tword, trans.get('sense', ''), trans.get('tags', []), trans.get('roman')))

            # Sounds
            for sound in entry.get('sounds', []):
                ipa = sound.get('ipa')
                enpr = sound.get('enpr')
                if ipa or enpr:
                    sound_buf.append((entry_id, ipa, enpr, sound.get('tags', [])))

            loaded += 1

    # Final flush
    flush_all()

    # Update sequences to match our manual IDs
    print("Updating sequences...", flush=True)
    cur.execute(f"ALTER SEQUENCE kk_entries_id_seq RESTART WITH {entry_id + 1}")
    cur.execute(f"ALTER SEQUENCE kk_senses_id_seq RESTART WITH {sense_id + 1}")
    conn.commit()

    # Restore indexes
    print("Rebuilding indexes...", flush=True)
    cur.execute("""
        CREATE INDEX idx_kke_word ON kk_entries (word);
        CREATE INDEX idx_kke_pos ON kk_entries (pos);
        CREATE INDEX idx_kke_token ON kk_entries (token_id) WHERE token_id IS NOT NULL;
        CREATE INDEX idx_kke_form_of ON kk_entries (form_of_word) WHERE form_of_word IS NOT NULL;
        CREATE INDEX idx_kke_alt_of ON kk_entries (alt_of_word) WHERE alt_of_word IS NOT NULL;
        CREATE INDEX idx_kkf_entry ON kk_forms (entry_id);
        CREATE INDEX idx_kkf_form ON kk_forms (form);
        CREATE INDEX idx_kks_entry ON kk_senses (entry_id);
        CREATE INDEX idx_kks_topics ON kk_senses USING GIN (topics) WHERE topics IS NOT NULL;
        CREATE INDEX idx_kksc_sense ON kk_sense_categories (sense_id);
        CREATE INDEX idx_kksc_name ON kk_sense_categories (name);
        CREATE INDEX idx_kksc_kind ON kk_sense_categories (kind);
        CREATE INDEX idx_kkse_sense ON kk_sense_examples (sense_id);
        CREATE INDEX idx_kkr_entry ON kk_relations (entry_id);
        CREATE INDEX idx_kkr_sense ON kk_relations (sense_id) WHERE sense_id IS NOT NULL;
        CREATE INDEX idx_kkr_type ON kk_relations (relation_type);
        CREATE INDEX idx_kkr_target ON kk_relations (target_word);
        CREATE INDEX idx_kkt_entry ON kk_translations (entry_id);
        CREATE INDEX idx_kkt_lang ON kk_translations (lang_code);
        CREATE INDEX idx_kkt_word ON kk_translations (word);
        CREATE INDEX idx_kkso_entry ON kk_sounds (entry_id);
    """)
    conn.commit()

    # Restore FK constraints
    print("Restoring FK constraints...", flush=True)
    cur.execute("""
        ALTER TABLE kk_forms ADD CONSTRAINT kk_forms_entry_id_fkey FOREIGN KEY (entry_id) REFERENCES kk_entries(id) ON DELETE CASCADE;
        ALTER TABLE kk_senses ADD CONSTRAINT kk_senses_entry_id_fkey FOREIGN KEY (entry_id) REFERENCES kk_entries(id) ON DELETE CASCADE;
        ALTER TABLE kk_sense_categories ADD CONSTRAINT kk_sense_categories_sense_id_fkey FOREIGN KEY (sense_id) REFERENCES kk_senses(id) ON DELETE CASCADE;
        ALTER TABLE kk_sense_examples ADD CONSTRAINT kk_sense_examples_sense_id_fkey FOREIGN KEY (sense_id) REFERENCES kk_senses(id) ON DELETE CASCADE;
        ALTER TABLE kk_relations ADD CONSTRAINT kk_relations_entry_id_fkey FOREIGN KEY (entry_id) REFERENCES kk_entries(id) ON DELETE CASCADE;
        ALTER TABLE kk_translations ADD CONSTRAINT kk_translations_entry_id_fkey FOREIGN KEY (entry_id) REFERENCES kk_entries(id) ON DELETE CASCADE;
        ALTER TABLE kk_sounds ADD CONSTRAINT kk_sounds_entry_id_fkey FOREIGN KEY (entry_id) REFERENCES kk_entries(id) ON DELETE CASCADE;
    """)
    conn.commit()

    print(f"\n{'='*60}", flush=True)
    print("LOAD COMPLETE", flush=True)
    print(f"{'='*60}", flush=True)
    print(f"Loaded: {loaded}", flush=True)
    print(f"Skipped: {skipped}", flush=True)
    print(f"Errors: {errors}", flush=True)
    print(f"Entry IDs assigned: {entry_id}", flush=True)
    print(f"Sense IDs assigned: {sense_id}", flush=True)

    for table in ['kk_entries', 'kk_forms', 'kk_senses', 'kk_sense_categories',
                   'kk_sense_examples', 'kk_relations', 'kk_translations', 'kk_sounds']:
        cur.execute(f"SELECT count(*) FROM {table}")
        print(f"  {table}: {cur.fetchone()[0]}", flush=True)

    cur.close()
    conn.close()


if __name__ == '__main__':
    main()
