#!/usr/bin/env python3
"""
Pass 3: Insert token_glosses records from Kaikki.

For each (token, PoS) pair with a token_pos record:
  1. Find the Kaikki entry for that word and PoS
  2. Take the first gloss from the first non-form-of, non-alt-of sense
  3. INSERT into token_glosses (status='DRAFT')
  4. UPDATE token_pos.gloss_id to point to the new gloss record

One gloss per (token_id, pos) pair — the primary meaning.
Multi-sense disambiguation can be added later via nuance_note.

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


def is_root_sense(sense: dict) -> bool:
    tags = set(sense.get('tags', []))
    return 'form-of' not in tags and 'alt-of' not in tags


def get_first_root_gloss(entry: dict) -> str:
    """Return first gloss from first non-form-of, non-alt-of sense."""
    for sense in entry.get('senses', []):
        if is_root_sense(sense):
            glosses = sense.get('glosses', [])
            if glosses:
                # Clean up gloss text: strip trailing period, truncate very long ones
                text = glosses[0].strip()
                if len(text) > 500:
                    text = text[:497] + '...'
                return text
    return ''


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


def load_kaikki_for_pos(target_pos: set[str]) -> dict[str, dict[str, str]]:
    """
    Single pass through Kaikki.
    Returns {pos: {word_lower: first_root_gloss}}.
    """
    result: dict[str, dict[str, str]] = defaultdict(dict)
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

            # Check has at least one root sense
            if not any(is_root_sense(s) for s in entry.get('senses', [])):
                continue

            word = entry.get('word', '').lower().strip()
            if not word or ' ' in word or word.startswith('-'):
                continue

            if word not in result[pos]:
                gloss = get_first_root_gloss(entry)
                if gloss:
                    result[pos][word] = gloss
                    kept += 1

    logging.info(f"Done. Scanned {total:,} lines, kept {kept:,} glosses.")
    return result


def process_pos(conn, pos: str, word_glosses: dict[str, str],
                dry_run: bool, batch_size: int) -> tuple[int, int]:
    """
    Insert token_glosses and update token_pos.gloss_id for one PoS.
    Returns (inserted, skipped).
    """
    words   = sorted(word_glosses.keys())
    total   = len(words)
    inserted = skipped = 0

    logging.info(f"  {pos}: {total:,} glosses to insert")

    for start in range(0, total, batch_size):
        batch_words = words[start:start + batch_size]

        # Look up token_ids for this batch
        with conn.cursor() as cur:
            cur.execute("SELECT name, token_id FROM tokens WHERE name = ANY(%s)", (batch_words,))
            token_map = {row[0]: row[1] for row in cur.fetchall()}
        conn.commit()

        if dry_run:
            inserted += len([w for w in batch_words if w in token_map])
            continue

        try:
            with conn.transaction():
                for word in batch_words:
                    if word not in token_map:
                        continue
                    token_id = token_map[word]
                    gloss_text = word_glosses[word]

                    # Determine pos_tags for this (word, kaikki_pos) pair
                    pos_tags = pos_tags_for_entry(word, pos)
                    if not pos_tags:
                        continue

                    for pos_tag in pos_tags:
                        # Check token_pos exists for this (token_id, pos_tag)
                        # and doesn't already have a gloss_id set
                        with conn.cursor() as cur:
                            cur.execute("""
                                SELECT id, gloss_id FROM token_pos
                                WHERE token_id = %s AND pos = %s::pos_tag
                            """, (token_id, pos_tag))
                            tp_row = cur.fetchone()

                        if tp_row is None:
                            continue  # no token_pos for this PoS — skip

                        tp_id, existing_gloss_id = tp_row
                        if existing_gloss_id is not None:
                            skipped += 1
                            continue  # already has gloss

                        # INSERT token_glosses
                        with conn.cursor() as cur:
                            cur.execute("""
                                INSERT INTO token_glosses
                                    (token_id, pos, gloss_text, status)
                                VALUES (%s, %s::pos_tag, %s, 'DRAFT')
                                ON CONFLICT (token_id, pos) DO NOTHING
                                RETURNING id
                            """, (token_id, pos_tag, gloss_text))
                            row = cur.fetchone()

                        if row is not None:
                            gloss_id = row[0]
                            # UPDATE token_pos.gloss_id
                            with conn.cursor() as cur:
                                cur.execute("""
                                    UPDATE token_pos SET gloss_id = %s
                                    WHERE id = %s
                                """, (gloss_id, tp_id))
                            inserted += 1
                        else:
                            skipped += 1

        except Exception as e:
            logging.error(f"  {pos} batch [{start}:{start+batch_size}] FAILED: {e}")
            raise

        if start % 10_000 == 0 and start > 0:
            logging.info(f"    {start:,}/{total:,} done")

    logging.info(f"  {pos} done: inserted={inserted:,}, skipped={skipped:,}")
    return inserted, skipped


def verify(conn) -> None:
    logging.info("--- Verification ---")
    with conn.cursor() as cur:
        cur.execute("SELECT count(*) FROM token_glosses")
        logging.info(f"token_glosses rows: {cur.fetchone()[0]:,}")
        cur.execute("SELECT status, count(*) FROM token_glosses GROUP BY status")
        for row in cur.fetchall():
            logging.info(f"  status={row[0]}: {row[1]:,}")
        cur.execute("""
            SELECT count(*) FROM token_pos WHERE gloss_id IS NOT NULL
        """)
        logging.info(f"token_pos with gloss_id set: {cur.fetchone()[0]:,}")
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
