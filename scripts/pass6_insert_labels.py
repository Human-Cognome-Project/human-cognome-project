#!/usr/bin/env python3
"""
Pass 6: Insert Label tokens (N_PROPER) and Initialisms from Kaikki 'name' PoS.

Namespace allocation:
  AD = Labels     — single-word proper names with no lowercase entry in AB
  AE = Initialisms — abbreviations/acronyms/initialisms (all-cap words)

Words already in AB (common words) are skipped entirely — no N_PROPER
token_pos needed. Any lowercase word can contextually serve as a label;
the separate namespace exists only so the engine can do a fast cap-check
against words that are EXCLUSIVELY proper names.

Multi-word name entries (28K+) are skipped — they belong in entity DBs
or Pass 7 (AF namespace phrases).

Usage:
    python3 pass6_insert_labels.py [--dry-run] [--batch-size N]
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

NS_LABELS      = 'AD'
NS_INITIALISMS = 'AE'

# ---------------------------------------------------------------------------
# Base-50 encoding (shared with pass1)
# ---------------------------------------------------------------------------

B50 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwx"

def encode_pair(n: int) -> str:
    n = max(0, min(2499, int(n)))
    return B50[n // 50] + B50[n % 50]

def decode_pair(s: str) -> int:
    def b(c):
        if 'A' <= c <= 'Z': return ord(c) - ord('A')
        if 'a' <= c <= 'x': return 26 + ord(c) - ord('a')
        return 0
    return b(s[0]) * 50 + b(s[1])

def word_to_p3_p4(word: str) -> tuple[str, str]:
    first = word[0].lower() if word else 'a'
    p3_val = (ord(first) - ord('a')) if ('a' <= first <= 'z') else 0
    p4_val = min(len(word) - 1, 49)
    return encode_pair(p3_val), encode_pair(p4_val)

def load_bucket_state(conn, ns: str) -> dict[tuple, int]:
    state: dict[tuple, int] = {}
    with conn.cursor() as cur:
        cur.execute("""
            SELECT p3, p4, p2, MAX(p5) AS max_p5
            FROM tokens
            WHERE ns = %s
            GROUP BY p3, p4, p2
        """, (ns,))
        for p3, p4, p2, max_p5 in cur.fetchall():
            state[(p3, p4, p2)] = decode_pair(max_p5) + 1
    return state

def alloc_p5(state: dict, p3: str, p4: str) -> tuple[str, str]:
    block = 0
    while True:
        p2 = encode_pair(block)
        key = (p3, p4, p2)
        seq = state.get(key, 0)
        if seq < 2500:
            p5 = encode_pair(seq)
            state[key] = seq + 1
            if block > 0:
                logging.warning(f"OVERFLOW bucket ({p3},{p4}): using p2={p2} seq={seq}")
            return p5, p2
        block += 1

# ---------------------------------------------------------------------------
# Kaikki helpers
# ---------------------------------------------------------------------------

ABBR_TAGS = frozenset({'abbreviation', 'initialism', 'acronym'})

def is_root_sense(sense: dict) -> bool:
    tags = set(sense.get('tags', []))
    return 'form-of' not in tags and 'alt-of' not in tags

def get_first_root_gloss(entry: dict) -> str:
    for sense in entry.get('senses', []):
        if is_root_sense(sense):
            glosses = sense.get('glosses', [])
            if glosses:
                text = glosses[0].strip()
                if len(text) > 500:
                    text = text[:497] + '...'
                return text
    return ''

def classify_name_entry(entry: dict) -> str | None:
    """Return 'label', 'initialism', or None (skip)."""
    word = entry.get('word', '').strip()
    if not word or ' ' in word or word.startswith('-'):
        return None

    sense_tags: set[str] = set()
    for s in entry.get('senses', []):
        sense_tags.update(s.get('tags', []))

    if sense_tags & ABBR_TAGS:
        return 'initialism'
    return 'label'

# ---------------------------------------------------------------------------
# Kaikki streaming
# ---------------------------------------------------------------------------

def load_name_entries(
        ab_names: set[str],
) -> tuple[list[dict], list[dict]]:
    """
    Single pass through Kaikki for pos='name'.
    Returns (labels, initialisms).
    Each item: {word_lower, gloss, original_word}
    """
    labels:      list[dict] = []
    initialisms: list[dict] = []
    seen:        set[str]   = set()   # (word, category) dedup
    total = 0

    logging.info("Streaming Kaikki for pos='name' …")

    with open(KAIKKI_FILE, encoding='utf-8') as f:
        for i, line in enumerate(f):
            total += 1
            if i % 300_000 == 0 and i > 0:
                logging.info(
                    f"  scanned {i:,} lines  "
                    f"labels={len(labels):,}  initialisms={len(initialisms):,}"
                )
            try:
                entry = json.loads(line)
            except json.JSONDecodeError:
                continue

            if entry.get('pos') != 'name':
                continue

            cat = classify_name_entry(entry)
            if cat is None:
                continue

            word_orig  = entry.get('word', '').strip()
            word_lower = word_orig.lower()

            # Skip if already in AB (common word)
            if word_lower in ab_names:
                continue

            key = (word_lower, cat)
            if key in seen:
                continue
            seen.add(key)

            gloss = get_first_root_gloss(entry)
            if not gloss:
                gloss = 'proper name'

            item = {'word': word_lower, 'original': word_orig, 'gloss': gloss}
            if cat == 'label':
                labels.append(item)
            else:
                initialisms.append(item)

    logging.info(
        f"Done. Scanned {total:,} lines. "
        f"Labels: {len(labels):,}  Initialisms: {len(initialisms):,}"
    )
    return labels, initialisms

# ---------------------------------------------------------------------------
# DB helpers
# ---------------------------------------------------------------------------

def load_ab_names(conn) -> set[str]:
    logging.info("Loading AB token names …")
    with conn.cursor() as cur:
        cur.execute("SELECT name FROM tokens WHERE ns = 'AB'")
        result = {row[0] for row in cur.fetchall()}
    conn.commit()
    logging.info(f"  Loaded {len(result):,} AB names.")
    return result


def insert_name_batch(
        conn,
        entries: list[dict],
        ns: str,
        cap_property: str,
        bucket_state: dict,
        batch_size: int,
        dry_run: bool,
        label: str,
) -> tuple[int, int]:
    """
    Insert tokens + token_pos + token_glosses for one category.
    Returns (inserted, skipped).
    """
    total = len(entries)
    inserted = skipped = 0
    logging.info(f"  {label}: {total:,} entries  ns={ns}")

    for start in range(0, total, batch_size):
        batch = entries[start:start + batch_size]
        words = [e['word'] for e in batch]

        # Check which words already have a token in this ns
        with conn.cursor() as cur:
            cur.execute(
                "SELECT name FROM tokens WHERE name = ANY(%s) AND ns = %s",
                (words, ns)
            )
            existing = {row[0] for row in cur.fetchall()}
        conn.commit()

        new_entries = [e for e in batch if e['word'] not in existing]
        skipped += len(batch) - len(new_entries)

        if not new_entries or dry_run:
            inserted += len(new_entries)
            continue

        # Allocate token_id coordinates
        rows = []
        for e in new_entries:
            p3, p4 = word_to_p3_p4(e['word'])
            p5, p2 = alloc_p5(bucket_state, p3, p4)
            rows.append((p2, p3, p4, p5, e['word'], e['gloss']))

        try:
            with conn.transaction():
                with conn.cursor() as cur:
                    # INSERT tokens
                    cur.executemany("""
                        INSERT INTO tokens (ns, p2, p3, p4, p5, name, characteristics)
                        VALUES (%s, %s, %s, %s, %s, %s, 0)
                    """, [(ns, p2, p3, p4, p5, word) for p2, p3, p4, p5, word, _ in rows])

                    # Fetch back the token_ids
                    cur.execute(
                        "SELECT name, token_id FROM tokens WHERE name = ANY(%s) AND ns = %s",
                        ([r[4] for r in rows], ns)
                    )
                    token_id_map = {row[0]: row[1] for row in cur.fetchall()}

                    # INSERT token_pos (N_PROPER)
                    pos_rows = [
                        (token_id_map[word], 'N_PROPER', True, 0, cap_property)
                        for _, _, _, _, word, _ in rows
                        if word in token_id_map
                    ]
                    cur.executemany("""
                        INSERT INTO token_pos
                            (token_id, pos, is_primary, morpheme_accept, cap_property)
                        VALUES (%s, %s::pos_tag, %s, %s, %s)
                        ON CONFLICT (token_id, pos) DO NOTHING
                    """, pos_rows)

                    # INSERT token_glosses (DRAFT) — sense_number=1 (single primary sense)
                    gloss_map = {r[4]: r[5] for r in rows}
                    cur.executemany("""
                        INSERT INTO token_glosses
                            (token_id, pos, sense_number, gloss_text, tags, status)
                        VALUES (%s, 'N_PROPER'::pos_tag, 1, %s, '{}', 'DRAFT')
                        ON CONFLICT (token_id, pos, sense_number) DO NOTHING
                    """, [(token_id_map[word], gloss_map[word])
                          for word in token_id_map])

            inserted += len(new_entries)
        except Exception as e:
            logging.error(f"  {label} batch [{start}:{start+batch_size}] FAILED: {e}")
            raise

        if start % 2000 == 0 and start > 0:
            logging.info(f"    {start:,}/{total:,} done")

    logging.info(f"  {label} done: inserted={inserted:,} skipped={skipped:,}")
    return inserted, skipped


def verify(conn) -> None:
    logging.info("--- Verification ---")
    with conn.cursor() as cur:
        for ns, label in [(NS_LABELS, 'AD Labels'), (NS_INITIALISMS, 'AE Initialisms')]:
            cur.execute("SELECT count(*) FROM tokens WHERE ns = %s", (ns,))
            logging.info(f"  {label} tokens: {cur.fetchone()[0]:,}")
        cur.execute("""
            SELECT count(*) FROM token_pos WHERE pos = 'N_PROPER'
        """)
        logging.info(f"  N_PROPER token_pos records: {cur.fetchone()[0]:,}")
    conn.commit()

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument('--dry-run',    action='store_true')
    parser.add_argument('--batch-size', type=int, default=500)
    parser.add_argument('--log', default=os.path.join(
        os.path.dirname(__file__), 'pass6_progress.log'))
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
    logging.info(f"Pass 6 start  dry_run={args.dry_run}  batch_size={args.batch_size}")

    conn = psycopg.connect(DB_DSN, autocommit=False)

    ab_names = load_ab_names(conn)
    labels, initialisms = load_name_entries(ab_names)

    # Load bucket states for both namespaces
    bucket_ad = load_bucket_state(conn, NS_LABELS)
    bucket_ae = load_bucket_state(conn, NS_INITIALISMS)
    conn.commit()
    logging.info(f"Bucket state: AD={len(bucket_ad)} buckets  AE={len(bucket_ae)} buckets")

    total_ins = total_skp = 0

    ins, skp = insert_name_batch(
        conn, labels, NS_LABELS, 'start_cap',
        bucket_ad, args.batch_size, args.dry_run, 'Labels (AD)',
    )
    total_ins += ins; total_skp += skp

    ins, skp = insert_name_batch(
        conn, initialisms, NS_INITIALISMS, 'all_cap',
        bucket_ae, args.batch_size, args.dry_run, 'Initialisms (AE)',
    )
    total_ins += ins; total_skp += skp

    logging.info(f"Pass 6 complete: inserted={total_ins:,} skipped={total_skp:,}")

    if not args.dry_run:
        verify(conn)

    conn.close()
    logging.info("Done.")


if __name__ == '__main__':
    main()
