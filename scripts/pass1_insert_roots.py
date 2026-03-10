#!/usr/bin/env python3
"""
Pass 1: Insert root tokens from Kaikki into hcp_english.

Processes PoS categories in canonical order (verb, noun, adj, adv, pron,
prep, conj, det, intj, particle, num). Pass 6 (name/N_PROPER) is separate.

For each PoS:
  1. Single stream through Kaikki JSONL collecting root entries (not form-of/alt-of)
  2. Sort alphabetically
  3. Check existing tokens (never insert duplicates)
  4. Mint new token_ids using AB.AA.p3.p4.p5 scheme
  5. INSERT in batches of 1000 with explicit transactions

Usage:
    python3 pass1_insert_roots.py [options]

Options:
    --pos NAME        Only process this PoS (e.g. verb, noun). Default: all in order.
    --dry-run         Show what would be inserted without committing.
    --batch-size N    Rows per transaction (default 1000).
    --log FILE        Log file path (default: pass1_progress.log in script dir).

Token ID scheme (ns='AB', p2='AA' standard / p2='AB' overflow):
    B50 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwx"
    p3  = encode_pair(ord(first_char) - ord('a'))  -- a=AA, z=AZ
    p4  = encode_pair(word_length - 1)             -- len1=AA, len50=Ax
    p5  = sequential 0-indexed within (p3,p4) bucket, 2500 per bucket
    Overflow (p5 >= 2500): use p2='AB' with p5 restarting from 0.

References:
    docs/kaikki-population-plan.md  -- full plan
    docs/kaikki-tag-mapping.md      -- tag mappings
"""

import argparse
import json
import logging
import os
import sys
from collections import defaultdict

import psycopg

# ---------------------------------------------------------------------------
# Config
# ---------------------------------------------------------------------------

KAIKKI_FILE = '/opt/project/sources/data/kaikki/english.jsonl'
DB_DSN      = 'dbname=hcp_english'

# PoS to skip (multi-token, sub-word morphemes, handled separately)
SKIP_POS = frozenset({
    'phrase', 'prep_phrase', 'proverb', 'prefix', 'suffix',
    'symbol', 'character', 'contraction',
    'name',  # Pass 6
})

# Processing order for Pass 1
POS_ORDER = ['verb', 'noun', 'adj', 'adv', 'pron', 'prep', 'conj',
             'det', 'intj', 'particle', 'num']

# ---------------------------------------------------------------------------
# Base-50 encoding (matches HCPDbUtils.h B50[])
# ---------------------------------------------------------------------------

B50 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwx"

def encode_pair(n: int) -> str:
    """Encode integer 0-2499 as 2-char base-50 string."""
    n = max(0, min(2499, int(n)))
    return B50[n // 50] + B50[n % 50]

def decode_pair(s: str) -> int:
    """Decode 2-char base-50 string to integer."""
    def b(c):
        if 'A' <= c <= 'Z': return ord(c) - ord('A')
        if 'a' <= c <= 'x': return 26 + ord(c) - ord('a')
        return 0
    return b(s[0]) * 50 + b(s[1])

def word_to_p3_p4(word: str) -> tuple[str, str]:
    """
    Return (p3, p4) coordinate strings for a word.
    p3: starting character (a=AA ... z=AZ; non-alpha → AA)
    p4: word length - 1, capped at 49 (lengths 1-50 map to AA-Ax)
    """
    first = word[0].lower() if word else 'a'
    p3_val = (ord(first) - ord('a')) if ('a' <= first <= 'z') else 0
    p4_val = min(len(word) - 1, 49)
    return encode_pair(p3_val), encode_pair(p4_val)

# ---------------------------------------------------------------------------
# Kaikki filtering
# ---------------------------------------------------------------------------

def is_root_entry(entry: dict) -> bool:
    """Return True if entry has at least one sense that is NOT form-of or alt-of."""
    for sense in entry.get('senses', []):
        tags = set(sense.get('tags', []))
        if 'form-of' not in tags and 'alt-of' not in tags:
            return True
    return False

def get_first_root_gloss(entry: dict) -> str:
    """Return first gloss text from a non-form-of, non-alt-of sense."""
    for sense in entry.get('senses', []):
        tags = set(sense.get('tags', []))
        if 'form-of' not in tags and 'alt-of' not in tags:
            glosses = sense.get('glosses', [])
            if glosses:
                return glosses[0]
    return ''

# ---------------------------------------------------------------------------
# Kaikki streaming
# ---------------------------------------------------------------------------

def load_kaikki_roots(target_pos: set[str]) -> dict[str, list[tuple[str, str]]]:
    """
    Single-pass stream through Kaikki JSONL.
    Returns {pos: [(word_lower, first_gloss), ...]} sorted alphabetically per PoS.
    Only root entries (not purely form-of/alt-of). Deduplicates by (pos, word).
    """
    entries: dict[str, dict[str, str]] = defaultdict(dict)  # pos → {word → gloss}

    logging.info(f"Streaming {KAIKKI_FILE} for PoS: {sorted(target_pos)}")
    total_lines = 0
    total_kept  = 0

    with open(KAIKKI_FILE, encoding='utf-8') as f:
        for i, line in enumerate(f):
            total_lines += 1
            if i % 200_000 == 0 and i > 0:
                logging.info(f"  scanned {i:,} lines, kept {total_kept:,} roots")
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
            if not word:
                continue
            # Pass 1 is single-word tokens only.
            # Multi-word phrases (idioms) go to Pass 7.
            # Hyphen-prefix fragments (-goon) are not standard vocab — skip.
            if ' ' in word or word.startswith('-'):
                continue

            # First gloss wins for each (pos, word) pair
            if word not in entries[pos]:
                entries[pos][word] = get_first_root_gloss(entry)
                total_kept += 1

    logging.info(f"Done. Scanned {total_lines:,} lines. Kept {total_kept:,} root entries.")

    # Sort alphabetically within each PoS
    return {pos: sorted(words.items()) for pos, words in entries.items()}

# ---------------------------------------------------------------------------
# Bucket state management
# ---------------------------------------------------------------------------

def load_bucket_state(conn) -> dict[tuple, int]:
    """
    Query current max p5 per (p3, p4, p2) bucket for ns='AB'.
    Returns {(p3, p4, p2): next_seq_int}.
    Only ns='AB' with p2 in ('AA','AB') — the Kaikki allocation space.
    """
    state: dict[tuple, int] = {}
    with conn.cursor() as cur:
        cur.execute("""
            SELECT p3, p4, p2, MAX(p5) AS max_p5
            FROM tokens
            WHERE ns = 'AB' AND p2 IN ('AA', 'AB')
            GROUP BY p3, p4, p2
        """)
        for p3, p4, p2, max_p5 in cur.fetchall():
            state[(p3, p4, p2)] = decode_pair(max_p5) + 1
    return state

def alloc_p5(state: dict, p3: str, p4: str) -> tuple[str, str]:
    """
    Allocate next p5 for (p3, p4) bucket.
    Returns (p5_str, p2_str). Advances counter.
    Uses p2='AA' (standard). At 2500+, promotes to p2='AB' (overflow).
    """
    key_aa = (p3, p4, 'AA')
    key_ab = (p3, p4, 'AB')

    seq = state.get(key_aa, 0)
    if seq < 2500:
        p5 = encode_pair(seq)
        state[key_aa] = seq + 1
        if seq >= 2400:
            logging.warning(f"Bucket ({p3},{p4}) p2=AA nearing limit: {seq+1}/2500")
        return p5, 'AA'

    # Overflow
    seq2 = state.get(key_ab, 0)
    p5 = encode_pair(seq2)
    state[key_ab] = seq2 + 1
    logging.warning(f"OVERFLOW bucket ({p3},{p4}): using p2=AB seq={seq2}")
    return p5, 'AB'

# ---------------------------------------------------------------------------
# DB insertion
# ---------------------------------------------------------------------------

def process_pos(conn, pos: str, word_gloss_list: list[tuple[str, str]],
                bucket_state: dict, dry_run: bool, batch_size: int) -> tuple[int, int]:
    """
    Insert root tokens for one PoS. Returns (inserted, skipped).
    Splits into batches of batch_size, each in its own transaction.
    """
    inserted = 0
    skipped  = 0
    total    = len(word_gloss_list)

    logging.info(f"  Processing {pos}: {total:,} root entries")

    for start in range(0, total, batch_size):
        batch = word_gloss_list[start:start + batch_size]
        words = [w for w, _ in batch]

        # Determine which words already exist in tokens
        with conn.cursor() as cur:
            cur.execute(
                "SELECT name FROM tokens WHERE name = ANY(%s)",
                (words,)
            )
            existing = {row[0] for row in cur.fetchall()}
        # Commit the read transaction so the next conn.transaction() starts a fresh BEGIN
        conn.commit()

        new_pairs = [(w, g) for w, g in batch if w not in existing]
        skipped  += len(batch) - len(new_pairs)

        if not new_pairs:
            continue

        if dry_run:
            inserted += len(new_pairs)
            if start == 0:
                logging.info(f"    DRY-RUN sample: {new_pairs[0][0]!r} .. {new_pairs[-1][0]!r}")
            continue

        # Allocate token_id coordinates for each new word
        rows = []
        for word, _ in new_pairs:
            p3, p4 = word_to_p3_p4(word)
            p5, p2 = alloc_p5(bucket_state, p3, p4)
            rows.append((p2, p3, p4, p5, word))

        try:
            with conn.transaction():
                with conn.cursor() as cur:
                    cur.executemany(
                        """
                        INSERT INTO tokens (ns, p2, p3, p4, p5, name, characteristics)
                        VALUES ('AB', %s, %s, %s, %s, %s, 0)
                        """,
                        rows
                    )
            inserted += len(rows)
        except Exception as e:
            logging.error(f"    Batch [{start}:{start+batch_size}] FAILED: {e}")
            logging.error(f"    First word: {new_pairs[0][0]!r}")
            raise

        if start % 10_000 == 0 and start > 0:
            logging.info(f"    {start:,}/{total:,} done, inserted so far: {inserted:,}")

    logging.info(f"  {pos} done: inserted={inserted:,}, skipped(existing)={skipped:,}")
    return inserted, skipped

# ---------------------------------------------------------------------------
# Verification
# ---------------------------------------------------------------------------

def verify_pos_counts(conn, pos_list: list[str]) -> None:
    """Log token counts for spot-check after insertion."""
    logging.info("--- Verification ---")
    with conn.cursor() as cur:
        cur.execute("SELECT count(*) FROM tokens WHERE ns='AB' AND p2='AA'")
        total_new = cur.fetchone()[0]
        logging.info(f"Total new Kaikki tokens (ns=AB, p2=AA): {total_new:,}")

        # Bucket distribution — top 10 densest
        cur.execute("""
            SELECT p3, p4, count(*) AS n
            FROM tokens
            WHERE ns='AB' AND p2='AA'
            GROUP BY p3, p4
            ORDER BY n DESC
            LIMIT 10
        """)
        rows = cur.fetchall()
        if rows:
            logging.info("Top 10 densest (p3, p4) buckets:")
            for p3, p4, n in rows:
                p3_char = chr(ord('a') + decode_pair(p3)) if decode_pair(p3) < 26 else '?'
                p4_len  = decode_pair(p4) + 1
                logging.info(f"  {p3_char}-len{p4_len}: {n:,} tokens")

        # Overflow detection is handled in alloc_p5() which logs a WARNING when p2='AB' is used.
        # Old tokens with p2='AB' use p3 values outside a-z range — not our overflow.

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--pos',        help='Only process this PoS (e.g. verb, noun)')
    parser.add_argument('--dry-run',    action='store_true', help='Show counts, no DB writes')
    parser.add_argument('--batch-size', type=int, default=1000, help='Rows per transaction')
    parser.add_argument('--log',        default=os.path.join(os.path.dirname(__file__), 'pass1_progress.log'))
    args = parser.parse_args()

    # Logging
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s %(levelname)s %(message)s',
        handlers=[
            logging.FileHandler(args.log),
            logging.StreamHandler(sys.stdout),
        ]
    )
    logging.info("=" * 60)
    logging.info(f"Pass 1 start  dry_run={args.dry_run}  batch_size={args.batch_size}")

    # Determine which PoS to process
    if args.pos:
        if args.pos not in POS_ORDER:
            parser.error(f"Unknown PoS '{args.pos}'. Valid: {POS_ORDER}")
        target_pos_order = [args.pos]
    else:
        target_pos_order = POS_ORDER

    target_pos_set = set(target_pos_order)

    # Stream Kaikki
    roots_by_pos = load_kaikki_roots(target_pos_set)

    for pos in target_pos_order:
        if pos not in roots_by_pos:
            logging.info(f"  {pos}: no root entries found")
        else:
            logging.info(f"  {pos}: {len(roots_by_pos[pos]):,} root entries to process")

    # Connect to DB
    logging.info(f"Connecting to {DB_DSN}")
    conn = psycopg.connect(DB_DSN, autocommit=False)

    # Load current bucket state (read-only; commit to close the implicit transaction
    # so subsequent conn.transaction() calls start fresh BEGIN/COMMIT blocks)
    bucket_state = load_bucket_state(conn)
    conn.commit()
    logging.info(f"Loaded bucket state: {len(bucket_state)} existing buckets")

    # Process each PoS
    total_inserted = total_skipped = 0
    for pos in target_pos_order:
        if pos not in roots_by_pos:
            continue
        ins, skp = process_pos(
            conn, pos, roots_by_pos[pos],
            bucket_state, args.dry_run, args.batch_size
        )
        total_inserted += ins
        total_skipped  += skp

    logging.info(f"Pass 1 complete: inserted={total_inserted:,}, skipped={total_skipped:,}")

    if not args.dry_run:
        verify_pos_counts(conn, target_pos_order)

    conn.close()
    logging.info("Done.")

if __name__ == '__main__':
    main()
