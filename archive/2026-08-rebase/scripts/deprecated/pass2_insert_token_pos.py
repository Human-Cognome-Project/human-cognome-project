#!/usr/bin/env python3
"""
Pass 2: Insert token_pos and token_morph_rules records from Kaikki.

For each Kaikki root entry (same filter as Pass 1):
  1. Look up token_id by name (should exist after Pass 1)
  2. Determine pos_tag (kaikki-tag-mapping.md section 2)
  3. Compute morpheme_accept bitmask
  4. Compute characteristics bitmask from sense tags
  5. INSERT token_pos (skip if already exists)
  6. INSERT token_morph_rules for each accepted morpheme

Special cases:
  - be: V_MAIN + V_AUX + V_COPULA (three records, is_primary on V_COPULA)
  - Auxiliaries: V_MAIN + V_AUX (from AUXILIARY_VERBS word list)
  - Uncountable nouns (Kaikki 'uncountable' tag): morpheme_accept=0
  - Initialisms (abbreviation/initialism tag): cap_property='all_cap' on N_PROPER
  - Conjunctions: CONJ_COORD vs CONJ_SUB from word list

Usage:
    python3 pass2_insert_token_pos.py [--pos verb|noun|adj|...] [--dry-run] [--batch-size N]
    Default: all PoS in canonical order.

References:
    docs/kaikki-population-plan.md  Pass 2
    docs/kaikki-tag-mapping.md      full tag → bitmask mapping
    db/migrations/029_new_english_schema.sql   schema
    db/migrations/031_token_morph_rules.sql    token_morph_rules
"""

import argparse
import json
import logging
import os
import re
import sys
from collections import defaultdict

import psycopg

KAIKKI_FILE = '/opt/project/sources/data/kaikki/english.jsonl'
DB_DSN      = 'dbname=hcp_english'

# ---------------------------------------------------------------------------
# Characteristic bitmask constants (kaikki-tag-mapping.md section 1)
# ---------------------------------------------------------------------------

FORMAL           = 1 << 0
CASUAL           = 1 << 1
SLANG            = 1 << 2
VULGAR           = 1 << 3
DEROGATORY       = 1 << 4
LITERARY         = 1 << 5
TECHNICAL        = 1 << 6
ARCHAIC          = 1 << 8
DATED            = 1 << 9
NEOLOGISM        = 1 << 10
DIALECT          = 1 << 12
BRITISH          = 1 << 13
AMERICAN         = 1 << 14
AUSTRALIAN       = 1 << 15
STANDARD_RULE    = 1 << 20
IRREGULAR        = 1 << 21
SPELLING_VARIANT = 1 << 22
EYE_DIALECT      = 1 << 23
BORROWING        = 1 << 24
COMPOUND         = 1 << 25
ABBREVIATION     = 1 << 26

# Sense tag → characteristics bits
SENSE_TAG_CHARS = {
    'formal':               FORMAL,
    'literary':             LITERARY,
    'poetic':               LITERARY,
    'informal':             CASUAL,
    'colloquial':           CASUAL,
    'humorous':             CASUAL,
    'childish':             CASUAL,
    'euphemistic':          CASUAL,
    'nonstandard':          CASUAL | DIALECT,
    'slang':                SLANG,
    'Internet':             SLANG | NEOLOGISM,
    'vulgar':               VULGAR,
    'derogatory':           DEROGATORY,
    'offensive':            DEROGATORY,
    'slur':                 DEROGATORY,
    'technical':            TECHNICAL,
    'obsolete':             ARCHAIC,
    'archaic':              ARCHAIC,
    'historical':           ARCHAIC,
    'dated':                DATED,
    'neologism':            NEOLOGISM,
    'dialectal':            DIALECT,
    'US':                   AMERICAN,
    'UK':                   BRITISH,
    'British':              BRITISH,
    'Australia':            AUSTRALIAN,
    'Scotland':             BRITISH | DIALECT,
    'Ireland':              BRITISH | DIALECT,
    'Northern-England':     BRITISH | DIALECT,
    'Southern-US':          AMERICAN | DIALECT,
    'Commonwealth':         BRITISH,
    'abbreviation':         ABBREVIATION,
    'initialism':           ABBREVIATION,
    'acronym':              ABBREVIATION,
    'clipping':             ABBREVIATION | CASUAL,
    'pronunciation-spelling': EYE_DIALECT | CASUAL,
}

# ---------------------------------------------------------------------------
# morpheme_accept bitmask constants (schema: token_pos.morpheme_accept)
# ---------------------------------------------------------------------------

MORPH_PLURAL      = 1 << 0   # 1
MORPH_PAST        = 1 << 1   # 2
MORPH_PROGRESSIVE = 1 << 2   # 4
MORPH_3RD_SING    = 1 << 3   # 8
MORPH_COMPARATIVE = 1 << 4   # 16
MORPH_SUPERLATIVE = 1 << 5   # 32
MORPH_ADVERB_LY   = 1 << 6   # 64
MORPH_POSSESSIVE  = 1 << 7   # 128
MORPH_GERUND      = 1 << 8   # 256

# Morpheme name → morpheme_accept bit
MORPHEME_BITS = {
    'PLURAL':      MORPH_PLURAL,
    'PAST':        MORPH_PAST,
    'PROGRESSIVE': MORPH_PROGRESSIVE,
    '3RD_SING':    MORPH_3RD_SING,
    'COMPARATIVE': MORPH_COMPARATIVE,
    'SUPERLATIVE': MORPH_SUPERLATIVE,
    'ADVERB_LY':   MORPH_ADVERB_LY,
    'POSSESSIVE':  MORPH_POSSESSIVE,
}

# ---------------------------------------------------------------------------
# PoS configuration
# ---------------------------------------------------------------------------

SKIP_POS = frozenset({
    'phrase', 'prep_phrase', 'proverb', 'prefix', 'suffix',
    'symbol', 'character', 'contraction', 'name',
})

POS_ORDER = ['verb', 'noun', 'adj', 'adv', 'pron', 'prep', 'conj',
             'det', 'intj', 'particle', 'num']

# Kaikki PoS → pos_tag enum
KAIKKI_TO_POS_TAG = {
    'noun':     'N_COMMON',
    'verb':     'V_MAIN',
    'adj':      'ADJ',
    'adv':      'ADV',
    'pron':     'N_PRONOUN',
    'prep':     'PREP',
    'det':      'DET',
    'intj':     'INTJ',
    'particle': 'PART',
    'num':      'NUM',
    # conj handled specially (COORD vs SUB)
}

# pos_tag → default morpheme_accept
POS_MORPH_ACCEPT = {
    'N_COMMON':  MORPH_PLURAL,
    'V_MAIN':    MORPH_PAST | MORPH_PROGRESSIVE | MORPH_3RD_SING,
    'ADJ':       MORPH_COMPARATIVE | MORPH_SUPERLATIVE,
    'ADV':       0,
    'N_PRONOUN': 0,
    'PREP':      0,
    'DET':       0,
    'INTJ':      0,
    'PART':      0,
    'NUM':       0,
    'CONJ_COORD': 0,
    'CONJ_SUB':  0,
    'V_AUX':     0,
    'V_COPULA':  0,
    'N_PROPER':  0,
}

# Auxiliary verbs: get both V_MAIN and V_AUX records
AUXILIARY_VERBS = frozenset({
    'be', 'have', 'do', 'will', 'shall', 'may', 'might',
    'can', 'could', 'would', 'should', 'must', 'need', 'dare', 'ought',
})

# Copula: be only (also gets V_COPULA record)
COPULA_VERBS = frozenset({'be'})

# Coordinating conjunctions (and, but, or, nor, for, yet, so)
COORD_CONJ = frozenset({'and', 'but', 'or', 'nor', 'for', 'yet', 'so',
                         'either', 'neither', 'not only', 'both'})

# ---------------------------------------------------------------------------
# Inflection rule loading and matching
# ---------------------------------------------------------------------------

def load_inflection_rules(conn) -> dict[str, list[dict]]:
    """Load all inflection_rules from DB, grouped by morpheme, sorted by priority."""
    rules: dict[str, list[dict]] = defaultdict(list)
    with conn.cursor() as cur:
        cur.execute("""
            SELECT id, morpheme, priority, condition, strip_suffix, add_suffix
            FROM inflection_rules
            ORDER BY morpheme, priority ASC
        """)
        for row in cur.fetchall():
            rid, morpheme, priority, condition, strip_suffix, add_suffix = row
            rules[morpheme].append({
                'id': rid,
                'condition': condition,
                'strip_suffix': strip_suffix,
                'add_suffix': add_suffix,
            })
    conn.commit()
    return rules

def apply_doubling(root: str, suffix: str) -> str:
    """Python port of apply_doubling_rule() from migration 030."""
    doubable = set('bdfgmnprt')
    vowels   = set('aeiou')
    if len(root) < 2:
        return root + suffix
    c_last  = root[-1]
    c_vowel = root[-2]
    c_prev  = root[-3] if len(root) >= 3 else ''
    if c_last not in doubable:
        return root + suffix
    if c_vowel not in vowels:
        return root + suffix
    if c_prev and c_prev in vowels:
        return root + suffix  # digraph (rain, beat) → no doubling
    if len(root) >= 4 and root[-2:] in ('en', 'on', 'an', 'er', 'or'):
        return root + suffix  # unstressed final syllable
    return root + c_last + suffix  # CVC confirmed → double

def find_rule(root: str, morpheme: str, rules: dict) -> tuple[int, str, str] | None:
    """
    Find first matching inflection rule for (root, morpheme).
    Returns (rule_id, strip_suffix, add_suffix) or None.
    For __DOUBLING__ rules: pre-computes add_suffix with doubled consonant absorbed.
    """
    for rule in rules.get(morpheme, []):
        if re.search(rule['condition'], root):
            if rule['strip_suffix'] == '__DOUBLING__':
                inflected   = apply_doubling(root, rule['add_suffix'])
                add_computed = inflected[len(root):]  # doubled_char + original_suffix
                return rule['id'], '', add_computed
            else:
                return rule['id'], rule['strip_suffix'], rule['add_suffix']
    return None

# ---------------------------------------------------------------------------
# Kaikki entry processing
# ---------------------------------------------------------------------------

def is_root_entry(entry: dict) -> bool:
    for sense in entry.get('senses', []):
        tags = set(sense.get('tags', []))
        if 'form-of' not in tags and 'alt-of' not in tags:
            return True
    return False

def compute_characteristics(entry: dict) -> int:
    """Aggregate characteristic bits from all root senses."""
    chars = 0
    for sense in entry.get('senses', []):
        tags = set(sense.get('tags', []))
        if 'form-of' in tags or 'alt-of' in tags:
            continue
        for tag, bits in SENSE_TAG_CHARS.items():
            if tag in tags:
                chars |= bits
    return chars

def is_uncountable(entry: dict) -> bool:
    """
    True only if explicitly uncountable and NEVER countable across all root senses.
    Kaikki often tags senses ['countable', 'uncountable'] for flexible nouns — these
    should be treated as countable (morpheme_accept includes PLURAL).
    Only purely uncountable entries (water, information, rice) return True here.
    """
    has_uncountable = False
    has_countable   = False
    for sense in entry.get('senses', []):
        tags = set(sense.get('tags', []))
        if 'form-of' in tags or 'alt-of' in tags:
            continue
        if 'uncountable' in tags or 'plural-only' in tags:
            has_uncountable = True
        if 'countable' in tags:
            has_countable = True
    return has_uncountable and not has_countable

def get_conj_pos_tag(word: str) -> str:
    """Return CONJ_COORD or CONJ_SUB for a conjunction word."""
    return 'CONJ_COORD' if word in COORD_CONJ else 'CONJ_SUB'

def get_pos_records(word: str, kaikki_pos: str, entry: dict) -> list[dict]:
    """
    Return list of {pos_tag, is_primary, cap_property, morpheme_accept, characteristics}
    for a given Kaikki entry.
    """
    chars = compute_characteristics(entry)
    records = []

    if kaikki_pos == 'conj':
        pos_tag = get_conj_pos_tag(word)
        records.append({
            'pos': pos_tag,
            'is_primary': True,
            'cap_property': None,
            'morpheme_accept': 0,
            'characteristics': chars,
        })
        return records

    if kaikki_pos not in KAIKKI_TO_POS_TAG and kaikki_pos != 'verb':
        return records

    if kaikki_pos == 'verb':
        morph_accept = POS_MORPH_ACCEPT['V_MAIN']
        is_primary_vmain = True

        records.append({
            'pos': 'V_MAIN',
            'is_primary': True,
            'cap_property': None,
            'morpheme_accept': morph_accept,
            'characteristics': chars,
        })

        if word in AUXILIARY_VERBS:
            records[0]['is_primary'] = False  # V_MAIN not primary for auxiliaries
            records.append({
                'pos': 'V_AUX',
                'is_primary': True,
                'cap_property': None,
                'morpheme_accept': 0,
                'characteristics': chars,
            })

        if word in COPULA_VERBS:
            # be gets V_MAIN (not primary) + V_AUX (not primary) + V_COPULA (primary)
            for r in records:
                r['is_primary'] = False
            records.append({
                'pos': 'V_COPULA',
                'is_primary': True,
                'cap_property': None,
                'morpheme_accept': 0,
                'characteristics': chars,
            })

        return records

    pos_tag = KAIKKI_TO_POS_TAG[kaikki_pos]
    morph_accept = POS_MORPH_ACCEPT.get(pos_tag, 0)

    # Uncountable / plural-only nouns don't inflect
    if pos_tag == 'N_COMMON' and is_uncountable(entry):
        morph_accept = 0

    # Abbreviation / initialism flag
    cap_property = None
    if chars & ABBREVIATION:
        cap_property = 'all_cap'

    records.append({
        'pos': pos_tag,
        'is_primary': True,
        'cap_property': cap_property,
        'morpheme_accept': morph_accept,
        'characteristics': chars,
    })
    return records

# ---------------------------------------------------------------------------
# DB insertion
# ---------------------------------------------------------------------------

def load_kaikki_for_pos(target_pos: set[str]) -> dict[str, dict[str, dict]]:
    """
    Single pass through Kaikki. Returns {pos: {word_lower: entry_summary}}.
    entry_summary: {'characteristics': int, 'uncountable': bool, 'senses': [...]}
    Keeps only root entries. De-dupes by (pos, word) — first occurrence wins.
    """
    result: dict[str, dict[str, dict]] = defaultdict(dict)
    logging.info(f"Streaming Kaikki for PoS: {sorted(target_pos)}")
    total = kept = 0

    with open(KAIKKI_FILE, encoding='utf-8') as f:
        for i, line in enumerate(f):
            total += 1
            if i % 200_000 == 0 and i > 0:
                logging.info(f"  scanned {i:,} lines, kept {kept:,}")
            try:
                entry = json.loads(line)
            except json.JSONDecodeError:
                continue

            pos = entry.get('pos', '')
            if pos not in target_pos:
                continue
            if not is_root_entry(entry):
                continue

            word = entry.get('word', '').lower().strip()
            if not word or ' ' in word or word.startswith('-'):
                continue

            if word not in result[pos]:
                result[pos][word] = entry
                kept += 1

    logging.info(f"Done. Scanned {total:,} lines, kept {kept:,} root entries.")
    return result


def insert_pos_for_word(conn, token_id: str, word: str, kaikki_pos: str, entry: dict,
                        inflection_rules: dict, dry_run: bool) -> tuple[int, int]:
    """
    Insert token_pos and token_morph_rules for one (token, kaikki_pos) pair.
    Returns (pos_inserted, morph_inserted).
    """
    pos_records = get_pos_records(word, kaikki_pos, entry)
    if not pos_records:
        return 0, 0

    pos_inserted  = 0
    morph_inserted = 0

    for rec in pos_records:
        pos_tag      = rec['pos']
        is_primary   = rec['is_primary']
        cap_property = rec['cap_property']
        morph_accept = rec['morpheme_accept']
        chars        = rec['characteristics']

        if dry_run:
            pos_inserted += 1
            continue

        # INSERT token_pos — UPSERT so re-runs can fix morpheme_accept / characteristics
        with conn.cursor() as cur:
            cur.execute("""
                INSERT INTO token_pos
                    (token_id, pos, is_primary, cap_property, morpheme_accept, characteristics)
                VALUES (%s, %s::pos_tag, %s, %s, %s, %s)
                ON CONFLICT (token_id, pos) DO UPDATE SET
                    morpheme_accept = EXCLUDED.morpheme_accept,
                    characteristics = EXCLUDED.characteristics
            """, (token_id, pos_tag, is_primary, cap_property, morph_accept, chars))
            pos_inserted += cur.rowcount if cur.rowcount >= 0 else 1

        # INSERT token_morph_rules for each accepted morpheme
        if morph_accept > 0:
            morphemes_to_check = [
                ('PAST',        MORPH_PAST),
                ('PLURAL',      MORPH_PLURAL),
                ('PROGRESSIVE', MORPH_PROGRESSIVE),
                ('3RD_SING',    MORPH_3RD_SING),
                ('COMPARATIVE', MORPH_COMPARATIVE),
                ('SUPERLATIVE', MORPH_SUPERLATIVE),
                ('ADVERB_LY',   MORPH_ADVERB_LY),
            ]
            for morpheme_name, morph_bit in morphemes_to_check:
                if not (morph_accept & morph_bit):
                    continue
                rule_match = find_rule(word, morpheme_name, inflection_rules)
                if rule_match is None:
                    continue
                rule_id, strip_suffix, add_suffix = rule_match
                with conn.cursor() as cur:
                    cur.execute("""
                        INSERT INTO token_morph_rules
                            (token_id, morpheme, rule_id, strip_suffix, add_suffix)
                        VALUES (%s, %s, %s, %s, %s)
                        ON CONFLICT (token_id, morpheme) DO NOTHING
                    """, (token_id, morpheme_name, rule_id, strip_suffix, add_suffix))
                    morph_inserted += cur.rowcount if cur.rowcount >= 0 else 1

    return pos_inserted, morph_inserted


def process_pos(conn, pos: str, word_entries: dict[str, dict],
                inflection_rules: dict, dry_run: bool, batch_size: int) -> tuple[int, int, int]:
    """
    Process all (word, entry) pairs for one PoS.
    Returns (pos_inserted, morph_inserted, not_found).
    """
    words = sorted(word_entries.keys())
    total = len(words)
    pos_inserted_total = morph_inserted_total = not_found_total = 0

    logging.info(f"  {pos}: {total:,} entries to process")

    for start in range(0, total, batch_size):
        batch_words = words[start:start + batch_size]

        # Look up token_ids for this batch
        with conn.cursor() as cur:
            cur.execute("SELECT name, token_id FROM tokens WHERE name = ANY(%s)", (batch_words,))
            token_map = {row[0]: row[1] for row in cur.fetchall()}
        conn.commit()

        not_found = [w for w in batch_words if w not in token_map]
        if not_found:
            not_found_total += len(not_found)
            if len(not_found) <= 5:
                logging.warning(f"  {pos} batch [{start}]: {len(not_found)} names not in tokens: {not_found}")

        try:
            with conn.transaction():
                for word in batch_words:
                    if word not in token_map:
                        continue
                    token_id = token_map[word]
                    entry    = word_entries[word]
                    pi, mi = insert_pos_for_word(
                        conn, token_id, word, pos, entry, inflection_rules, dry_run
                    )
                    pos_inserted_total  += pi
                    morph_inserted_total += mi
        except Exception as e:
            logging.error(f"  {pos} batch [{start}:{start+batch_size}] FAILED: {e}")
            raise

        if start % 10_000 == 0 and start > 0:
            logging.info(f"    {start:,}/{total:,} done")

    logging.info(f"  {pos} done: pos_records={pos_inserted_total:,}, "
                 f"morph_rules={morph_inserted_total:,}, not_found={not_found_total:,}")
    return pos_inserted_total, morph_inserted_total, not_found_total

# ---------------------------------------------------------------------------
# Verification
# ---------------------------------------------------------------------------

def verify(conn) -> None:
    logging.info("--- Verification ---")
    with conn.cursor() as cur:
        cur.execute("SELECT count(*) FROM token_pos")
        logging.info(f"token_pos rows: {cur.fetchone()[0]:,}")
        cur.execute("SELECT count(*) FROM token_morph_rules")
        logging.info(f"token_morph_rules rows: {cur.fetchone()[0]:,}")
        cur.execute("SELECT pos, count(*) FROM token_pos GROUP BY pos ORDER BY count DESC")
        logging.info("PoS distribution:")
        for row in cur.fetchall():
            logging.info(f"  {row[0]}: {row[1]:,}")
    conn.commit()

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--pos',        help='Process only this PoS')
    parser.add_argument('--dry-run',    action='store_true')
    parser.add_argument('--batch-size', type=int, default=1000)
    parser.add_argument('--log',        default=os.path.join(os.path.dirname(__file__), 'pass2_progress.log'))
    args = parser.parse_args()

    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s %(levelname)s %(message)s',
        handlers=[
            logging.FileHandler(args.log),
            logging.StreamHandler(sys.stdout),
        ]
    )
    logging.info("=" * 60)
    logging.info(f"Pass 2 start  dry_run={args.dry_run}  batch_size={args.batch_size}")

    if args.pos:
        if args.pos not in POS_ORDER:
            parser.error(f"Unknown PoS '{args.pos}'. Valid: {POS_ORDER}")
        target_pos_order = [args.pos]
    else:
        target_pos_order = POS_ORDER

    target_pos_set = set(target_pos_order)

    # Stream Kaikki
    entries_by_pos = load_kaikki_for_pos(target_pos_set)

    # Connect and load inflection rules
    conn = psycopg.connect(DB_DSN, autocommit=False)
    inflection_rules = load_inflection_rules(conn)
    logging.info(f"Loaded inflection rules for morphemes: {sorted(inflection_rules.keys())}")

    total_pos = total_morph = total_missing = 0
    for pos in target_pos_order:
        if pos not in entries_by_pos:
            logging.info(f"  {pos}: no entries")
            continue
        pi, mi, nf = process_pos(
            conn, pos, entries_by_pos[pos], inflection_rules,
            args.dry_run, args.batch_size
        )
        total_pos   += pi
        total_morph += mi
        total_missing += nf

    logging.info(f"Pass 2 complete: pos_records={total_pos:,}, "
                 f"morph_rules={total_morph:,}, not_found={total_missing:,}")

    if not args.dry_run:
        verify(conn)

    conn.close()
    logging.info("Done.")

if __name__ == '__main__':
    main()
