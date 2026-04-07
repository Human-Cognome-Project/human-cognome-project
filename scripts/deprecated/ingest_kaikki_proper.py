#!/usr/bin/env python3
"""
Proper Kaikki ingestion — extract full lexical structure.

Token IDs assigned to:
  - Root words (base/lemma forms)
  - All inflected forms that are distinct words (walked, walking, walks, etc.)
  - Derived forms (walker, walkable, etc.)

Token IDs NOT assigned to (share parent's token_id):
  - Alternate spellings (colour→color, lite→light)
  - Misspellings (teh→the)
  - Dialect surface forms (walkin'→walking)
  - Accented alternates (café→cafe)

Structure preserved:
  - forms[] → inflected forms with grammatical tags
  - senses[] → glosses
  - derived[] → derivational children
  - synonyms/antonyms/hyponyms/hypernyms/related → semantic relations
  - etymology → source language data
  - form_of → links back to root
  - alt_of → alternate spelling links

Destructive consumption: entries deleted from staging as processed.
"""

import json
import os
import sys
import subprocess
from collections import defaultdict

DB_ENV = {**os.environ, 'PGPASSWORD': 'hcp_dev'}
KAIKKI_PATH = '/opt/project/sources/data/kaikki/english.jsonl'

def psql(sql):
    r = subprocess.run(['psql','-h','localhost','-U','hcp','-d','hcp_english','-t','-A','-c',sql],
                       capture_output=True, text=True, env=DB_ENV)
    if r.returncode != 0 and r.stderr.strip():
        print(f"SQL ERROR: {r.stderr.strip()}", file=sys.stderr)
    return r.stdout.strip()

def psql_exec(sql):
    r = subprocess.run(['psql','-h','localhost','-U','hcp','-d','hcp_english','-c',sql],
                       capture_output=True, text=True, env=DB_ENV)
    if r.returncode != 0 and r.stderr.strip():
        print(f"SQL ERROR: {r.stderr.strip()}", file=sys.stderr)
    return r.returncode == 0


# ---------------------------------------------------------------------------
# Token ID minting
# ---------------------------------------------------------------------------

B50 = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwx'

def int_to_b50_pair(n):
    return B50[n // 50] + B50[n % 50]

class TokenIdMinter:
    """
    Mint token_ids in AB namespace.
    p3 = (starting_letter, word_length) as 2-char pair.
    p4+p5 = sequential within bucket.
    Non-alpha and accented chars bucket to position 27 ('b').
    """
    def __init__(self):
        self.buckets = {}

    def mint(self, name):
        if not name:
            return None

        first = name[0].lower()
        word_len = min(len(name), 49)

        # p3 char 1: starting letter
        if first >= 'a' and first <= 'z':
            idx = ord(first) - ord('a')
            if idx == 14:  # o → lowercase 'a'
                p3_c1 = 'a'
            else:
                p3_c1 = B50[idx]

        else:
            p3_c1 = B50[27]  # non-alpha bucket

        # p3 char 2: word length
        p3_c2 = B50[word_len]
        p3 = p3_c1 + p3_c2

        # Sequential within bucket
        if p3 not in self.buckets:
            self.buckets[p3] = 0
        self.buckets[p3] += 1
        seq = self.buckets[p3]

        p4_val = seq // 2500
        p5_val = seq % 2500
        p4 = B50[p4_val // 50] + B50[p4_val % 50]
        p5 = B50[p5_val // 50] + B50[p5_val % 50]

        return f"AB.AA.{p3}.{p4}.{p5}"


# ---------------------------------------------------------------------------
# PoS mapping
# ---------------------------------------------------------------------------

POS_MAP = {
    'verb': 'V_MAIN',
    'noun': 'N_COMMON',
    'adj': 'ADJ',
    'adv': 'ADV',
    'prep': 'PREP',
    'conj': 'CONJ_COORD',
    'det': 'DET',
    'intj': 'INTJ',
    'num': 'NUM',
    'pron': 'N_PRONOUN',
    'particle': 'PART',
    'name': 'N_PROPER',
    'phrase': None,  # skip for now
    'prefix': None,
    'suffix': None,
    'infix': None,
    'character': None,
    'symbol': None,
    'proverb': None,
    'contraction': None,  # handle separately
    'article': 'DET',
    'abbrev': None,
}


# ---------------------------------------------------------------------------
# Classify: is this entry a root, a form, or an alternate?
# ---------------------------------------------------------------------------

def classify_entry(entry):
    """
    Returns: 'root', 'form_of', 'alt_of', or 'skip'
    """
    senses = entry.get('senses', [])

    has_form_of = False
    has_alt_of = False
    has_real_sense = False
    is_misspelling = False

    for sense in senses:
        tags = set(sense.get('tags', []))
        if 'form-of' in tags:
            has_form_of = True
        elif 'alt-of' in tags:
            has_alt_of = True
            if 'misspelling' in tags:
                is_misspelling = True
        else:
            has_real_sense = True

    # If ALL senses are form-of → this is an inflected form
    if has_form_of and not has_real_sense:
        return 'form_of'

    # If ALL senses are alt-of → this is an alternate spelling
    if has_alt_of and not has_real_sense:
        return 'alt_of'

    # If it has at least one real sense → it's a root (even if also form-of for another sense)
    if has_real_sense:
        return 'root'

    # Edge case: only form-of AND alt-of, no real senses
    if has_form_of:
        return 'form_of'
    if has_alt_of:
        return 'alt_of'

    return 'skip'


def safe_sql(text):
    """Escape single quotes for SQL."""
    if text is None:
        return ''
    return text.replace("'", "''")


# ---------------------------------------------------------------------------
# Schema setup
# ---------------------------------------------------------------------------

def setup_schema():
    """Create clean tables for proper Kaikki ingestion."""
    psql_exec("""
        -- Drop existing new_ tables if any
        DROP TABLE IF EXISTS kaikki_token_relations CASCADE;
        DROP TABLE IF EXISTS kaikki_token_derived CASCADE;
        DROP TABLE IF EXISTS kaikki_token_senses CASCADE;
        DROP TABLE IF EXISTS kaikki_token_forms CASCADE;
        DROP TABLE IF EXISTS kaikki_tokens CASCADE;

        -- Root words and distinct inflected forms
        CREATE TABLE kaikki_tokens (
            token_id    TEXT PRIMARY KEY,
            name        TEXT NOT NULL,
            pos         TEXT,
            is_root     BOOLEAN NOT NULL DEFAULT true,
            root_token_id TEXT,  -- if not root, points to root
            etymology_text TEXT,
            UNIQUE (name, pos)
        );
        CREATE INDEX idx_kt_name ON kaikki_tokens (name);
        CREATE INDEX idx_kt_root ON kaikki_tokens (root_token_id);

        -- All surface forms (inflected, alternate, dialect, misspelling)
        -- Forms that are distinct words have their own token_id in kaikki_tokens
        -- Forms that are alternates share parent's token_id
        CREATE TABLE kaikki_token_forms (
            id          SERIAL PRIMARY KEY,
            token_id    TEXT NOT NULL,  -- the token_id this form resolves to
            form        TEXT NOT NULL,
            tags        TEXT[],  -- grammatical/variant tags from Kaikki
            source      TEXT,    -- 'conjugation', 'head', 'alt-of', 'misspelling', 'dialect'
            UNIQUE (token_id, form, tags)
        );
        CREATE INDEX idx_ktf_form ON kaikki_token_forms (form);
        CREATE INDEX idx_ktf_token ON kaikki_token_forms (token_id);

        -- Glosses / senses
        CREATE TABLE kaikki_token_senses (
            id          SERIAL PRIMARY KEY,
            token_id    TEXT NOT NULL,
            sense_num   SMALLINT NOT NULL DEFAULT 1,
            gloss_text  TEXT NOT NULL,
            tags        TEXT[],
            UNIQUE (token_id, sense_num)
        );
        CREATE INDEX idx_kts_token ON kaikki_token_senses (token_id);

        -- Derived terms (derivational children)
        CREATE TABLE kaikki_token_derived (
            id          SERIAL PRIMARY KEY,
            token_id    TEXT NOT NULL,  -- parent
            derived_word TEXT NOT NULL,
            derived_token_id TEXT  -- filled in after all roots ingested
        );
        CREATE INDEX idx_ktd_token ON kaikki_token_derived (token_id);
        CREATE INDEX idx_ktd_word ON kaikki_token_derived (derived_word);

        -- Semantic relations (synonyms, antonyms, hyponyms, hypernyms, related)
        CREATE TABLE kaikki_token_relations (
            id          SERIAL PRIMARY KEY,
            token_id    TEXT NOT NULL,
            relation    TEXT NOT NULL,  -- synonym, antonym, hyponym, hypernym, related
            target_word TEXT NOT NULL,
            target_token_id TEXT,  -- filled in after all roots ingested
            tags        TEXT[]
        );
        CREATE INDEX idx_ktr_token ON kaikki_token_relations (token_id);
        CREATE INDEX idx_ktr_target ON kaikki_token_relations (target_word);
    """)
    print("Schema created.")


# ---------------------------------------------------------------------------
# Main ingestion
# ---------------------------------------------------------------------------

def main():
    print("="*60)
    print("PROPER KAIKKI INGESTION")
    print("="*60)

    setup_schema()
    minter = TokenIdMinter()

    # Pass 1: Ingest root entries (words with real senses)
    print("\nPass 1: Root entries...")
    root_count = 0
    form_of_entries = []
    alt_of_entries = []
    skipped = 0

    with open(KAIKKI_PATH) as f:
        for line_no, line in enumerate(f):
            if line_no % 100000 == 0 and line_no > 0:
                print(f"  Line {line_no}, roots: {root_count}")

            try:
                entry = json.loads(line)
            except:
                continue

            word = entry.get('word', '').strip()
            if not word:
                continue

            # Skip multi-word, initialisms, and funky entries
            if ' ' in word:
                skipped += 1
                continue
            if word.isupper() and len(word) > 1:  # initialisms like FBI, NATO
                skipped += 1
                continue
            if any(c in word for c in '()[]{}0123456789'):  # numbered/bracketed entries
                skipped += 1
                continue

            pos = entry.get('pos', '')
            hcp_pos = POS_MAP.get(pos)
            if hcp_pos is None:
                skipped += 1
                continue

            classification = classify_entry(entry)

            if classification == 'skip':
                skipped += 1
                continue

            if classification == 'form_of':
                form_of_entries.append((line_no, entry))
                continue

            if classification == 'alt_of':
                alt_of_entries.append((line_no, entry))
                continue

            # It's a root entry — mint token_id
            name_lower = word.lower()
            token_id = minter.mint(name_lower)
            if not token_id:
                continue

            safe_name = safe_sql(name_lower)
            safe_pos = safe_sql(hcp_pos)
            safe_etym = safe_sql(entry.get('etymology_text', '')[:500])

            # Insert token
            psql_exec(f"""
                INSERT INTO kaikki_tokens (token_id, name, pos, is_root, etymology_text)
                VALUES ('{token_id}', '{safe_name}', '{safe_pos}', true, '{safe_etym}')
                ON CONFLICT (name, pos) DO NOTHING;
            """)

            # Insert senses
            for i, sense in enumerate(entry.get('senses', []), 1):
                tags = sense.get('tags', [])
                if 'form-of' in tags or 'alt-of' in tags:
                    continue
                glosses = sense.get('glosses', [])
                if not glosses:
                    continue
                safe_gloss = safe_sql(glosses[0][:500])
                safe_tags = '{' + ','.join(f'"{safe_sql(t)}"' for t in tags) + '}'
                psql_exec(f"""
                    INSERT INTO kaikki_token_senses (token_id, sense_num, gloss_text, tags)
                    VALUES ('{token_id}', {i}, '{safe_gloss}', '{safe_tags}')
                    ON CONFLICT DO NOTHING;
                """)

            # Insert forms (from the entry's forms list)
            for form_data in entry.get('forms', []):
                form_word = form_data.get('form', '').strip().lower()
                if not form_word or form_word == name_lower:
                    continue
                form_tags = form_data.get('tags', [])
                # Skip metadata entries
                if 'table-tags' in form_tags or 'inflection-template' in form_tags:
                    continue
                if form_word == 'no-table-tags' or form_word == 'glossary':
                    continue

                safe_form = safe_sql(form_word)
                safe_ftags = '{' + ','.join(f'"{safe_sql(t)}"' for t in form_tags) + '}'
                source = form_data.get('source', 'head')
                psql_exec(f"""
                    INSERT INTO kaikki_token_forms (token_id, form, tags, source)
                    VALUES ('{token_id}', '{safe_form}', '{safe_ftags}', '{safe_sql(source)}')
                    ON CONFLICT DO NOTHING;
                """)

            # Insert derived terms
            for derived in entry.get('derived', []):
                dword = derived.get('word', '').strip().lower()
                if dword:
                    safe_dword = safe_sql(dword)
                    psql_exec(f"""
                        INSERT INTO kaikki_token_derived (token_id, derived_word)
                        VALUES ('{token_id}', '{safe_dword}');
                    """)

            # Insert relations
            for rel_type in ['synonyms', 'antonyms', 'hyponyms', 'hypernyms', 'related']:
                for rel in entry.get(rel_type, []):
                    rword = rel.get('word', '').strip().lower()
                    if rword:
                        safe_rword = safe_sql(rword)
                        rtags = rel.get('tags', [])
                        safe_rtags = '{' + ','.join(f'"{safe_sql(t)}"' for t in rtags) + '}'
                        rel_name = rel_type.rstrip('s')  # synonyms→synonym
                        psql_exec(f"""
                            INSERT INTO kaikki_token_relations (token_id, relation, target_word, tags)
                            VALUES ('{token_id}', '{rel_name}', '{safe_rword}', '{safe_rtags}');
                        """)

            root_count += 1

    print(f"\n  Roots ingested: {root_count}")
    print(f"  Form-of entries saved for pass 2: {len(form_of_entries)}")
    print(f"  Alt-of entries saved for pass 3: {len(alt_of_entries)}")
    print(f"  Skipped (no mapped PoS): {skipped}")

    # Pass 2: Form-of entries get their own token_id (they're distinct words)
    print(f"\nPass 2: Form-of entries (distinct words)...")
    form_count = 0

    for line_no, entry in form_of_entries:
        word = entry.get('word', '').strip()
        name_lower = word.lower()
        pos = entry.get('pos', '')
        hcp_pos = POS_MAP.get(pos, 'N_COMMON')

        # Find what it's a form of
        root_word = None
        form_tags = []
        for sense in entry.get('senses', []):
            if 'form-of' in sense.get('tags', []):
                fo = sense.get('form_of', [])
                if fo:
                    root_word = fo[0].get('word', '').strip().lower()
                    form_tags = [t for t in sense.get('tags', []) if t != 'form-of']
                break

        if not root_word:
            continue

        # Look up root's token_id
        root_tid = psql(f"SELECT token_id FROM kaikki_tokens WHERE name = '{safe_sql(root_word)}' LIMIT 1")

        # Mint own token_id
        token_id = minter.mint(name_lower)
        if not token_id:
            continue

        safe_name = safe_sql(name_lower)
        safe_pos = safe_sql(hcp_pos)

        psql_exec(f"""
            INSERT INTO kaikki_tokens (token_id, name, pos, is_root, root_token_id)
            VALUES ('{token_id}', '{safe_name}', '{safe_pos}', false, {f"'{root_tid}'" if root_tid else 'NULL'})
            ON CONFLICT (name, pos) DO NOTHING;
        """)

        form_count += 1
        if form_count % 10000 == 0:
            print(f"  Forms: {form_count}")

    print(f"  Form-of entries ingested: {form_count}")

    # Pass 3: Alt-of entries share parent's token_id (not distinct words)
    print(f"\nPass 3: Alt-of entries (surface variations)...")
    alt_count = 0

    for line_no, entry in alt_of_entries:
        word = entry.get('word', '').strip()
        name_lower = word.lower()

        # Find what it's an alt of
        target_word = None
        alt_tags = []
        for sense in entry.get('senses', []):
            if 'alt-of' in sense.get('tags', []):
                fo = sense.get('form_of', [])
                if fo:
                    target_word = fo[0].get('word', '').strip().lower()
                alt_tags = [t for t in sense.get('tags', []) if t != 'alt-of']
                break

        if not target_word:
            continue

        # Look up target's token_id
        target_tid = psql(f"SELECT token_id FROM kaikki_tokens WHERE name = '{safe_sql(target_word)}' LIMIT 1")
        if not target_tid:
            continue

        # Determine source type
        if 'misspelling' in alt_tags:
            source = 'misspelling'
        elif any(t in alt_tags for t in ['dialectal', 'pronunciation-spelling']):
            source = 'dialect'
        else:
            source = 'alt-of'

        safe_form = safe_sql(name_lower)
        safe_atags = '{' + ','.join(f'"{safe_sql(t)}"' for t in alt_tags) + '}'

        psql_exec(f"""
            INSERT INTO kaikki_token_forms (token_id, form, tags, source)
            VALUES ('{target_tid}', '{safe_form}', '{safe_atags}', '{source}')
            ON CONFLICT DO NOTHING;
        """)

        alt_count += 1
        if alt_count % 5000 == 0:
            print(f"  Alts: {alt_count}")

    print(f"  Alt-of entries ingested: {alt_count}")

    # Final counts
    print(f"\n{'='*60}")
    print("INGESTION COMPLETE")
    print(f"{'='*60}")
    tokens = psql("SELECT count(*) FROM kaikki_tokens")
    roots = psql("SELECT count(*) FROM kaikki_tokens WHERE is_root = true")
    forms = psql("SELECT count(*) FROM kaikki_token_forms")
    senses = psql("SELECT count(*) FROM kaikki_token_senses")
    derived = psql("SELECT count(*) FROM kaikki_token_derived")
    relations = psql("SELECT count(*) FROM kaikki_token_relations")

    print(f"Tokens (all): {tokens}")
    print(f"  Roots: {roots}")
    print(f"  Forms (distinct words): {int(tokens) - int(roots)}")
    print(f"Surface forms (alts/variants): {forms}")
    print(f"Senses: {senses}")
    print(f"Derived terms: {derived}")
    print(f"Relations: {relations}")


if __name__ == '__main__':
    main()
