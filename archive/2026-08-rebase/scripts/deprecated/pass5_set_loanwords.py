#!/usr/bin/env python3
"""
Pass 5: Set source_language and BORROWING bit for loanword tokens.

UPDATE-only pass — no new rows inserted.

For each Kaikki root entry whose first etymology_template with name in
('bor', 'bor+', 'lbor', 'slbor') gives a non-native source language:
  UPDATE tokens
     SET source_language = <3-char ISO code>,
         characteristics = characteristics | (1<<24)   -- BORROWING
   WHERE name = <word>
     AND source_language IS NULL;   -- idempotent

Native / ancestral languages (Old English, Middle English, Proto-Germanic,
Proto-Indo-European, etc.) are skipped — they are NOT borrowings.

Usage:
    python3 pass5_set_loanwords.py [--dry-run] [--batch-size N]
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

BORROWING = 1 << 24   # characteristics bit

# ---------------------------------------------------------------------------
# Language code handling
# ---------------------------------------------------------------------------

# Native / ancestral languages: treat as non-borrowings.
# These represent the genetic ancestry of English, not foreign borrowings.
NATIVE_LANGS = frozenset({
    'ang',       # Old English
    'enm',       # Middle English
    'sco',       # Scots (dialect of English)
    'gem-pro',   # Proto-Germanic
    'gmw-pro',   # Proto-West-Germanic
    'ine-pro',   # Proto-Indo-European
    'mul',       # Multiple/unclassified — skip
    'und',       # Undetermined — skip
    'unk',       # Unknown — skip
    'onf',       # Old Northern French (debatable, keep for now)
})

# Explicit mapping for codes > 3 chars or needing normalization
LANG_MAP = {
    # Latin varieties → 'la'
    'la-new':   'la',
    'la-med':   'la',
    'la-lat':   'la',
    'la-mlat':  'la',
    'la-cl':    'la',
    'la-vul':   'la',
    # Greek varieties → 'grc'
    'grc-koi':  'grc',
    'grc-byz':  'grc',
    # Old French → 'fr'  (Middle French 'frm' kept as-is, fits in 3)
    'fro':      'fr',
    # Chinese varieties → 'cmn'
    'cmn-pinyin': 'cmn',
    'cmn-wadegiles': 'cmn',
    'cmn-hans': 'cmn',
    'cmn-hant': 'cmn',
    'yue':      'cmn',
    'wuu':      'cmn',
    'nan':      'cmn',
    # Dutch/Flemish
    'dum':      'nl',   # Middle Dutch
    'vls':      'nl',   # West Flemish
    # German varieties
    'goh':      'de',   # Old High German
    'gmh':      'de',   # Middle High German
    # Scandinavian
    'gmq-pro':  'non',  # Proto-North-Germanic
    # Fallbacks for long codes: strip after '-'
}

# Skip any language code that indicates a proto-language or native ancestor
def is_native(lang: str) -> bool:
    if lang in NATIVE_LANGS:
        return True
    if lang.endswith('-pro'):
        return True
    if lang.startswith('ine-') or lang.startswith('gem-') or lang.startswith('gmw-'):
        return True
    return False


def normalize_lang(lang: str) -> str | None:
    """Return ≤3-char language code, or None to skip."""
    if not lang:
        return None
    if is_native(lang):
        return None
    if lang in LANG_MAP:
        return LANG_MAP[lang]
    if len(lang) <= 3:
        return lang
    # Longer code: take part before first '-' if it's ≤3 chars
    base = lang.split('-')[0]
    if len(base) <= 3:
        return base
    return None   # can't fit — skip

# ---------------------------------------------------------------------------
# Kaikki streaming
# ---------------------------------------------------------------------------

BOR_TEMPLATES = frozenset({'bor', 'bor+', 'lbor', 'slbor'})

def extract_source_lang(entry: dict) -> str | None:
    """Extract first borrow-template source language from etymology_templates."""
    for tmpl in entry.get('etymology_templates', []):
        if tmpl.get('name') in BOR_TEMPLATES:
            args = tmpl.get('args', {})
            # Kaikki bor template: args['1'] = target lang (usually 'en'), args['2'] = source lang
            lang = args.get('2', '') or args.get('1', '')
            if not lang:
                continue
            code = normalize_lang(lang)
            if code:
                return code
    return None


def load_loanwords(target_pos: set) -> dict[str, str]:
    """
    Single pass through Kaikki.
    Returns {word_lower: source_lang_code} for root entries with a borrow etymology.
    First entry wins (deduplicates by word).
    """
    result: dict[str, str] = {}
    total = kept = 0
    logging.info(f"Streaming Kaikki for loanwords …")

    with open(KAIKKI_FILE, encoding='utf-8') as f:
        for i, line in enumerate(f):
            total += 1
            if i % 300_000 == 0 and i > 0:
                logging.info(f"  scanned {i:,} lines, found {kept:,} loanwords")
            try:
                entry = json.loads(line)
            except json.JSONDecodeError:
                continue

            if entry.get('pos', '') not in target_pos:
                continue

            word = entry.get('word', '').lower().strip()
            if not word or ' ' in word or word.startswith('-'):
                continue

            # Only root entries
            has_root = any(
                'form-of' not in set(s.get('tags', [])) and
                'alt-of'  not in set(s.get('tags', []))
                for s in entry.get('senses', [])
            )
            if not has_root:
                continue

            # Skip if already found
            if word in result:
                continue

            lang = extract_source_lang(entry)
            if lang:
                result[word] = lang
                kept += 1

    logging.info(f"Done. Scanned {total:,} lines, found {kept:,} loanwords.")
    return result

# ---------------------------------------------------------------------------
# DB update
# ---------------------------------------------------------------------------

def update_loanwords(conn, loanwords: dict[str, str], batch_size: int,
                     dry_run: bool) -> tuple[int, int]:
    """
    Batch UPDATE tokens for loanword source_language + BORROWING bit.
    Returns (updated, skipped).
    """
    words = sorted(loanwords.keys())
    total = len(words)
    updated = skipped = 0

    logging.info(f"  Updating {total:,} loanword tokens …")

    for start in range(0, total, batch_size):
        batch_words = words[start:start + batch_size]

        if dry_run:
            updated += len(batch_words)
            continue

        try:
            with conn.transaction():
                with conn.cursor() as cur:
                    for word in batch_words:
                        lang = loanwords[word]
                        cur.execute("""
                            UPDATE tokens
                               SET source_language = %s,
                                   characteristics = characteristics | %s
                             WHERE name = %s
                               AND source_language IS NULL
                        """, (lang, BORROWING, word))
                        if cur.rowcount > 0:
                            updated += cur.rowcount
                        else:
                            skipped += 1
        except Exception as e:
            logging.error(f"  Batch [{start}:{start+batch_size}] FAILED: {e}")
            raise

        if start % 20_000 == 0 and start > 0:
            logging.info(f"    {start:,}/{total:,} done")

    logging.info(f"  Update done: updated={updated:,} skipped={skipped:,}")
    return updated, skipped


def verify(conn) -> None:
    logging.info("--- Verification ---")
    with conn.cursor() as cur:
        cur.execute("""
            SELECT source_language, count(*)
            FROM tokens WHERE source_language IS NOT NULL
            GROUP BY source_language ORDER BY count DESC LIMIT 20
        """)
        rows = cur.fetchall()
        total_loan = sum(r[1] for r in rows)
        logging.info(f"Tokens with source_language set: {total_loan:,}")
        for lang, cnt in rows:
            logging.info(f"  {lang.strip():6}  {cnt:,}")

        cur.execute("""
            SELECT count(*) FROM tokens
            WHERE (characteristics & %s) != 0
        """, (BORROWING,))
        logging.info(f"Tokens with BORROWING bit: {cur.fetchone()[0]:,}")
    conn.commit()

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

POS_ORDER = ['verb', 'noun', 'adj', 'adv', 'pron', 'prep', 'conj',
             'det', 'intj', 'particle', 'num']

def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument('--dry-run',    action='store_true')
    parser.add_argument('--batch-size', type=int, default=1000)
    parser.add_argument('--log', default=os.path.join(
        os.path.dirname(__file__), 'pass5_progress.log'))
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
    logging.info(f"Pass 5 start  dry_run={args.dry_run}  batch_size={args.batch_size}")

    loanwords = load_loanwords(set(POS_ORDER))

    # Top languages preview
    from collections import Counter
    lang_counts = Counter(loanwords.values())
    logging.info("Top source languages:")
    for lang, cnt in lang_counts.most_common(15):
        logging.info(f"  {lang:6}  {cnt:,}")

    conn = psycopg.connect(DB_DSN, autocommit=False)

    updated, skipped = update_loanwords(conn, loanwords, args.batch_size, args.dry_run)
    logging.info(f"Pass 5 complete: updated={updated:,} skipped={skipped:,}")

    if not args.dry_run:
        verify(conn)

    conn.close()
    logging.info("Done.")


if __name__ == '__main__':
    main()
