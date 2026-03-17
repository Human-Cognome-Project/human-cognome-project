#!/usr/bin/env python3
"""
Pass 3: Insert token_glosses records from Kaikki.

For each (token, PoS) pair with a token_pos record:
  1. Find the Kaikki entry for that word and PoS
  2. Collect all root senses (not form-of or alt-of)
  3. INSERT one token_glosses row per sense with sense_number (1-indexed) and tags
     Unique constraint: (token_id, pos, sense_number)

Multiple senses per (token, PoS) are stored in sense order from Kaikki.
Tags (domain, register, temporal) from each sense are stored in the tags TEXT[] column.

Usage:
    python3 pass3_insert_glosses.py [--pos verb|noun|...] [--dry-run] [--batch-size N]
"""

import argparse
import json
import logging
import os
import sys
from collections import defaultdict

import psycopg

KAIKKI_FILE = '/opt/project/sources/data/kaikki/english.jsonl'
DB_DSN      = 'dbname=hcp_english'

SKIP_POS = frozenset({
    'phrase', 'prep_phrase', 'proverb', 'prefix', 'suffix',
    'symbol', 'character', 'contraction', 'name',
})

POS_ORDER = ['verb', 'noun', 'adj', 'adv', 'pron', 'prep', 'conj',
             'det', 'intj', 'particle', 'num']

KAIKKI_TO_POS_TAG = {
    'noun':     'N_COMMON',
    'verb':     'V_MAIN',
    'adj':      'ADJ',
    'adv':      'ADV',
    'pron':     'N_PRONOUN',
    'prep':     'PREP',
    'conj':     None,           # handled specially: CONJ_COORD or CONJ_SUB
    'det':      'DET',
    'intj':     'INTJ',
    'particle': 'PART',
    'num':      'NUM',
}

COORD_CONJ = frozenset({'and', 'but', 'or', 'nor', 'for', 'yet', 'so',
                         'either', 'neither', 'not only', 'both'})

AUXILIARY_VERBS = frozenset({
    'be', 'have', 'do', 'will', 'shall', 'may', 'might',
    'can', 'could', 'would', 'should', 'must', 'need', 'dare', 'ought',
})

COPULA_VERBS = frozenset({'be'})

# Tags that are structural markers, not domain/register metadata
_STRUCTURAL_TAGS = frozenset({'form-of', 'alt-of'})


def is_root_sense(sense: dict) -> bool:
    tags = set(sense.get('tags', []))
    return 'form-of' not in tags and 'alt-of' not in tags


def collect_root_senses(entry: dict) -> list[dict]:
    """
    Return all root senses from a Kaikki entry as a list of dicts, 1-indexed:
      [{'sense_number': 1, 'gloss': str, 'tags': list[str]}, ...]
    Skips senses with no gloss text. Tags exclude structural markers.
    """
    senses = []
    for sense in entry.get('senses', []):
        if not is_root_sense(sense):
            continue
        glosses = sense.get('glosses', [])
        if not glosses:
            continue
        text = glosses[0].strip()
        if len(text) > 500:
            text = text[:497] + '...'
        tags = [t for t in sense.get('tags', []) if t not in _STRUCTURAL_TAGS]
        senses.append({'gloss': text, 'tags': tags})
    for i, s in enumerate(senses, 1):
        s['sense_number'] = i
    return senses


def pos_tags_for_entry(word: str, kaikki_pos: str) -> list[str]:
    """Return all pos_tag values this entry generates."""
    if kaikki_pos == 'conj':
        return ['CONJ_COORD' if word in COORD_CONJ else 'CONJ_SUB']
    pos_tag = KAIKKI_TO_POS_TAG.get(kaikki_pos)
    if pos_tag is None:
        return []
    tags = [pos_tag]
    if kaikki_pos == 'verb':
        if word in COPULA_VERBS:
            tags = ['V_MAIN', 'V_AUX', 'V_COPULA']
        elif word in AUXILIARY_VERBS:
            tags = ['V_MAIN', 'V_AUX']
    return tags


def load_kaikki_for_pos(target_pos: set[str]) -> dict[str, dict[str, list[dict]]]:
    """
    Single pass through Kaikki.
    Returns {kaikki_pos: {word_lower: [sense_dict, ...]}}.
    Only the first Kaikki entry per (word, pos) is used; all its root senses are kept.
    """
    result: dict[str, dict[str, list[dict]]] = defaultdict(dict)
    logging.info(f"Streaming Kaikki for PoS: {sorted(target_pos)}")
    total = kept_entries = kept_senses = 0

    with open(KAIKKI_FILE, encoding='utf-8') as f:
        for i, line in enumerate(f):
            total += 1
            if i % 200_000 == 0 and i > 0:
                logging.info(f"  scanned {i:,} lines, kept {kept_entries:,} entries / {kept_senses:,} senses")
            try:
                entry = json.loads(line)
            except json.JSONDecodeError:
                continue

            pos = entry.get('pos', '')
            if pos not in target_pos:
                continue

            word = entry.get('word', '').lower().strip()
            if not word or ' ' in word or word.startswith('-'):
                continue

            if word in result[pos]:
                continue  # already have senses for this (word, pos) from an earlier entry

            senses = collect_root_senses(entry)
            if senses:
                result[pos][word] = senses
                kept_entries += 1
                kept_senses  += len(senses)

    logging.info(f"Done. Scanned {total:,} lines, "
                 f"kept {kept_entries:,} entries / {kept_senses:,} senses.")
    return result


def process_pos(conn, pos: str, word_senses: dict[str, list[dict]],
                dry_run: bool, batch_size: int) -> tuple[int, int]:
    """
    Insert token_glosses rows for one PoS. Multiple senses per (token_id, pos).
    Returns (inserted, skipped).
    """
    words   = sorted(word_senses.keys())
    total   = len(words)
    inserted = skipped = 0

    total_senses = sum(len(v) for v in word_senses.values())
    logging.info(f"  {pos}: {total:,} words / {total_senses:,} senses to insert")

    for start in range(0, total, batch_size):
        batch_words = words[start:start + batch_size]

        # Look up token_ids for this batch
        with conn.cursor() as cur:
            cur.execute("SELECT name, token_id FROM tokens WHERE name = ANY(%s)", (batch_words,))
            token_map = {row[0]: row[1] for row in cur.fetchall()}
        conn.commit()

        if dry_run:
            for word in batch_words:
                if word in token_map:
                    inserted += len(word_senses[word])
            continue

        try:
            with conn.transaction():
                for word in batch_words:
                    if word not in token_map:
                        continue
                    token_id = token_map[word]
                    senses   = word_senses[word]

                    pos_tags = pos_tags_for_entry(word, pos)
                    if not pos_tags:
                        continue

                    for pos_tag in pos_tags:
                        # Check token_pos exists for this (token_id, pos_tag)
                        with conn.cursor() as cur:
                            cur.execute("""
                                SELECT id FROM token_pos
                                WHERE token_id = %s AND pos = %s::pos_tag
                            """, (token_id, pos_tag))
                            tp_row = cur.fetchone()

                        if tp_row is None:
                            continue  # no token_pos for this PoS — skip

                        for sense in senses:
                            with conn.cursor() as cur:
                                cur.execute("""
                                    INSERT INTO token_glosses
                                        (token_id, pos, sense_number, gloss_text, tags, status)
                                    VALUES (%s, %s::pos_tag, %s, %s, %s, 'DRAFT')
                                    ON CONFLICT (token_id, pos, sense_number) DO NOTHING
                                """, (token_id, pos_tag,
                                      sense['sense_number'],
                                      sense['gloss'],
                                      sense['tags']))
                            inserted += 1

        except Exception as e:
            logging.error(f"  {pos} batch [{start}:{start+batch_size}] FAILED: {e}")
            raise

        if start % 10_000 == 0 and start > 0:
            logging.info(f"    {start:,}/{total:,} words done")

    logging.info(f"  {pos} done: inserted={inserted:,}, skipped={skipped:,}")
    return inserted, skipped


def verify(conn) -> None:
    logging.info("--- Verification ---")
    with conn.cursor() as cur:
        cur.execute("SELECT count(*) FROM token_glosses")
        logging.info(f"token_glosses rows: {cur.fetchone()[0]:,}")
        cur.execute("SELECT status, count(*) FROM token_glosses GROUP BY status ORDER BY status")
        for row in cur.fetchall():
            logging.info(f"  status={row[0]}: {row[1]:,}")
        cur.execute("""
            SELECT pos, count(*) FROM token_glosses GROUP BY pos ORDER BY count DESC
        """)
        logging.info("PoS distribution:")
        for row in cur.fetchall():
            logging.info(f"  {row[0]}: {row[1]:,}")
        cur.execute("""
            SELECT sense_number, count(*) FROM token_glosses
            GROUP BY sense_number ORDER BY sense_number
        """)
        logging.info("Sense number distribution:")
        for row in cur.fetchall():
            logging.info(f"  sense {row[0]}: {row[1]:,}")
    conn.commit()


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--pos',        help='Process only this PoS')
    parser.add_argument('--dry-run',    action='store_true')
    parser.add_argument('--batch-size', type=int, default=500)
    parser.add_argument('--log',        default=os.path.join(os.path.dirname(__file__), 'pass3_progress.log'))
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
    logging.info(f"Pass 3 start  dry_run={args.dry_run}  batch_size={args.batch_size}")

    if args.pos:
        if args.pos not in POS_ORDER:
            parser.error(f"Unknown PoS '{args.pos}'. Valid: {POS_ORDER}")
        target_pos_order = [args.pos]
    else:
        target_pos_order = POS_ORDER

    entries_by_pos = load_kaikki_for_pos(set(target_pos_order))

    conn = psycopg.connect(DB_DSN, autocommit=False)

    total_inserted = total_skipped = 0
    for pos in target_pos_order:
        if pos not in entries_by_pos:
            logging.info(f"  {pos}: no glosses")
            continue
        ins, skp = process_pos(conn, pos, entries_by_pos[pos], args.dry_run, args.batch_size)
        total_inserted += ins
        total_skipped  += skp

    logging.info(f"Pass 3 complete: inserted={total_inserted:,}, skipped={total_skipped:,}")

    if not args.dry_run:
        verify(conn)

    conn.close()
    logging.info("Done.")


if __name__ == '__main__':
    main()
