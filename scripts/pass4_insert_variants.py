#!/usr/bin/env python3
"""
Pass 4: Insert token_variants from Kaikki (irregular inflections, archaic/obsolete forms).

For each Kaikki entry whose senses are all form_of/alt_of (non-root):
  1. For each form_of sense with a known morpheme tag (PAST, PLURAL, etc.):
       a. Look up the canonical word in our tokens table
       b. Compute the expected regular form using inflection_rules
       c. If variant_word != expected  →  insert as IRREGULAR in token_variants
  2. For each alt_of sense with archaic/obsolete tags:
       a. Look up the canonical word in our tokens table
       b. If variant not already in our tokens table  →  insert with ARCHAIC/DATED bits

Cleanup step (after inserts):
  Delete token_morph_rules rows for (canonical_id, morpheme) where an IRREGULAR
  token_variants row now exists for that pair — the irregular variant IS the inflection.

Usage:
    python3 pass4_insert_variants.py [--dry-run] [--batch-size N]
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
# Characteristics bits (must match pass2)
# ---------------------------------------------------------------------------

ARCHAIC          = 1 << 8   # 256
DATED            = 1 << 9   # 512
DIALECT          = 1 << 12  # 4096
IRREGULAR        = 1 << 21  # 2097152
SPELLING_VARIANT = 1 << 22  # 4194304

# ---------------------------------------------------------------------------
# Kaikki form_of tag → morpheme name
# ---------------------------------------------------------------------------

def extract_morpheme(tags: set) -> str | None:
    """Return morpheme name from Kaikki form_of sense tags, or None if not supported."""
    if 'plural' in tags:
        return 'PLURAL'
    if 'past' in tags:
        # ['past'] = simple past; ['past', 'participle'] = past participle.
        # Map both to PAST — our schema has one morpheme for the past form.
        # For verbs where simple past == past participle (said, made, told), this
        # is the only Kaikki tag; for strong verbs (went, ran), separate entries exist.
        return 'PAST'
    if 'gerund' in tags and 'present' in tags and 'participle' in tags:
        return 'PROGRESSIVE'
    if 'comparative' in tags:
        return 'COMPARATIVE'
    if 'superlative' in tags:
        return 'SUPERLATIVE'
    if 'third-person' in tags and 'singular' in tags and 'present' in tags:
        return '3RD_SING'
    return None


def extract_alt_characteristics(tags: set) -> int | None:
    """Return characteristics bits for an alt_of entry, or None to skip."""
    bits = 0
    if 'archaic' in tags or 'obsolete' in tags:
        bits |= ARCHAIC
    if 'dated' in tags:
        bits |= DATED
    if 'dialectal' in tags or 'nonstandard' in tags:
        bits |= DIALECT | ARCHAIC
    if not bits:
        return None
    return bits

# ---------------------------------------------------------------------------
# Inflection rule helpers (ported from pass2)
# ---------------------------------------------------------------------------

def apply_doubling(root: str, suffix: str) -> str:
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
        return root + suffix
    if len(root) >= 4 and root[-2:] in ('en', 'on', 'an', 'er', 'or'):
        return root + suffix
    return root + c_last + suffix


def compute_regular_form(root: str, morpheme: str, rules: dict) -> str | None:
    """Compute expected regular inflected form for (root, morpheme). Returns None if no rule."""
    for rule in rules.get(morpheme, []):
        if re.search(rule['condition'], root):
            add = rule['add_suffix']
            strip = rule['strip_suffix']
            if strip == '__DOUBLING__':
                return apply_doubling(root, add)
            base = root[:-len(strip)] if strip else root
            return base + add
    return None


def load_inflection_rules(conn) -> dict[str, list[dict]]:
    rules: dict[str, list[dict]] = defaultdict(list)
    with conn.cursor() as cur:
        cur.execute("""
            SELECT morpheme, priority, condition, strip_suffix, add_suffix
            FROM inflection_rules ORDER BY morpheme, priority ASC
        """)
        for morpheme, priority, cond, strip, add in cur.fetchall():
            rules[morpheme].append({
                'condition': cond,
                'strip_suffix': strip,
                'add_suffix': add,
            })
    conn.commit()
    return rules

# ---------------------------------------------------------------------------
# Kaikki streaming
# ---------------------------------------------------------------------------


def load_variants_from_kaikki(
        token_map: dict[str, str],
        inf_rules: dict,
) -> tuple[list[dict], list[dict]]:
    """
    Single pass through Kaikki.
    Returns (irregular_list, archaic_list).

    Each item in irregular_list:
        {variant, canonical_id, morpheme, characteristics}

    Each item in archaic_list:
        {variant, canonical_id, characteristics}
    """
    irregular: list[dict] = []
    archaic:   list[dict] = []
    total = 0

    logging.info(f"Streaming {KAIKKI_FILE} for variant entries …")

    with open(KAIKKI_FILE, encoding='utf-8') as f:
        for i, line in enumerate(f):
            total += 1
            if i % 300_000 == 0 and i > 0:
                logging.info(f"  scanned {i:,} lines  irr={len(irregular):,}  arch={len(archaic):,}")
            try:
                entry = json.loads(line)
            except json.JSONDecodeError:
                continue

            senses = entry.get('senses', [])
            if not senses:
                continue

            variant_word = entry.get('word', '').lower().strip()
            if not variant_word or ' ' in variant_word or variant_word.startswith('-'):
                continue

            # Process each sense (scan all entries, not just pure-variant ones,
            # so we catch form_of senses in entries that also have root senses,
            # e.g., "was" has one colloquial root sense but also form_of "be" PAST)
            seen_pairs: set = set()   # (canonical_id, morpheme) already added this entry

            for sense in senses:
                tags = set(sense.get('tags', []))

                # ── form_of → irregular inflections ──
                if 'form-of' in tags:
                    for fo_item in sense.get('form_of', []):
                        canonical_word = fo_item.get('word', '').lower().strip()
                        if not canonical_word or ' ' in canonical_word:
                            continue
                        canonical_id = token_map.get(canonical_word)
                        if canonical_id is None:
                            continue

                        morpheme = extract_morpheme(tags)
                        if morpheme is None:
                            continue

                        # Extra characteristics from sense tags
                        chars = IRREGULAR
                        if 'archaic' in tags or 'obsolete' in tags:
                            chars |= ARCHAIC
                        if 'dialectal' in tags:
                            chars |= DIALECT

                        # Compute expected regular form
                        expected = compute_regular_form(canonical_word, morpheme, inf_rules)
                        if expected is not None and expected.lower() == variant_word:
                            continue  # regular form — skip

                        pair_key = (canonical_id, morpheme)
                        if pair_key in seen_pairs:
                            continue
                        seen_pairs.add(pair_key)

                        irregular.append({
                            'variant':       variant_word,
                            'canonical_id':  canonical_id,
                            'morpheme':      morpheme,
                            'characteristics': chars,
                        })

                # ── alt_of → archaic/obsolete spelling variants ──
                elif 'alt-of' in tags:
                    chars = extract_alt_characteristics(tags)
                    if chars is None:
                        continue

                    for ao_item in sense.get('alt_of', []):
                        canonical_word = ao_item.get('word', '').lower().strip()
                        if not canonical_word or ' ' in canonical_word:
                            continue
                        canonical_id = token_map.get(canonical_word)
                        if canonical_id is None:
                            continue

                        pair_key = (canonical_id, None)
                        if pair_key in seen_pairs:
                            continue
                        seen_pairs.add(pair_key)

                        archaic.append({
                            'variant':        variant_word,
                            'canonical_id':   canonical_id,
                            'characteristics': chars,
                        })

    logging.info(f"Done. Scanned {total:,} lines. "
                 f"Irregular: {len(irregular):,}  Archaic/obsolete: {len(archaic):,}")
    return irregular, archaic

# ---------------------------------------------------------------------------
# DB insertion
# ---------------------------------------------------------------------------

def load_token_map(conn) -> dict[str, str]:
    """Load all tokens: {lowercase_name: token_id}."""
    logging.info("Loading token map from DB …")
    with conn.cursor() as cur:
        cur.execute("SELECT name, token_id FROM tokens")
        result = {row[0]: row[1] for row in cur.fetchall()}
    conn.commit()
    logging.info(f"  Loaded {len(result):,} tokens.")
    return result


def insert_variants(conn, rows: list[dict], batch_size: int, dry_run: bool,
                    label: str) -> tuple[int, int]:
    """
    Batch-insert into token_variants.
    Returns (inserted, skipped).
    """
    inserted = skipped = 0
    total = len(rows)
    logging.info(f"  {label}: {total:,} candidates")

    for start in range(0, total, batch_size):
        batch = rows[start:start + batch_size]

        if dry_run:
            inserted += len(batch)
            continue

        try:
            with conn.transaction():
                with conn.cursor() as cur:
                    for row in batch:
                        cur.execute("""
                            INSERT INTO token_variants
                                (canonical_id, name, morpheme, characteristics)
                            VALUES (%s, %s, %s, %s)
                            ON CONFLICT (canonical_id, name,
                                         COALESCE(morpheme, '')) DO NOTHING
                        """, (
                            row['canonical_id'],
                            row['variant'],
                            row.get('morpheme'),
                            row['characteristics'],
                        ))
                        inserted += cur.rowcount
                        skipped  += (1 - cur.rowcount)
        except Exception as e:
            logging.error(f"  {label} batch [{start}:{start+batch_size}] FAILED: {e}")
            raise

        if start % 10_000 == 0 and start > 0:
            logging.info(f"    {start:,}/{total:,} done")

    logging.info(f"  {label} done: inserted={inserted:,} skipped={skipped:,}")
    return inserted, skipped


def cleanup_superseded_morph_rules(conn, dry_run: bool) -> int:
    """
    Delete token_morph_rules rows where an IRREGULAR token_variants row
    exists for the same (canonical_id, morpheme).  Returns count deleted.
    """
    if dry_run:
        with conn.cursor() as cur:
            cur.execute("""
                SELECT count(*) FROM token_morph_rules tmr
                WHERE EXISTS (
                    SELECT 1 FROM token_variants tv
                    WHERE tv.canonical_id = tmr.token_id
                      AND tv.morpheme     = tmr.morpheme
                      AND (tv.characteristics & %s) != 0
                )
            """, (IRREGULAR,))
            n = cur.fetchone()[0]
        conn.commit()
        logging.info(f"  Cleanup (dry-run): would delete {n:,} token_morph_rules rows")
        return n

    with conn.transaction():
        with conn.cursor() as cur:
            cur.execute("""
                DELETE FROM token_morph_rules tmr
                WHERE EXISTS (
                    SELECT 1 FROM token_variants tv
                    WHERE tv.canonical_id = tmr.token_id
                      AND tv.morpheme     = tmr.morpheme
                      AND (tv.characteristics & %s) != 0
                )
            """, (IRREGULAR,))
            n = cur.rowcount
    logging.info(f"  Cleanup: deleted {n:,} superseded token_morph_rules rows")
    return n


def verify(conn) -> None:
    logging.info("--- Verification ---")
    with conn.cursor() as cur:
        cur.execute("SELECT count(*) FROM token_variants")
        logging.info(f"token_variants rows: {cur.fetchone()[0]:,}")

        cur.execute("""
            SELECT count(*) FROM token_variants
            WHERE (characteristics & %s) != 0
        """, (IRREGULAR,))
        logging.info(f"  IRREGULAR: {cur.fetchone()[0]:,}")

        cur.execute("""
            SELECT count(*) FROM token_variants
            WHERE (characteristics & %s) != 0
        """, (ARCHAIC,))
        logging.info(f"  ARCHAIC: {cur.fetchone()[0]:,}")

        # Per-morpheme breakdown
        cur.execute("""
            SELECT morpheme, count(*) FROM token_variants
            WHERE morpheme IS NOT NULL
            GROUP BY morpheme ORDER BY count DESC
        """)
        for row in cur.fetchall():
            logging.info(f"    {row[0]}: {row[1]:,}")

        cur.execute("SELECT count(*) FROM token_morph_rules")
        logging.info(f"token_morph_rules rows after cleanup: {cur.fetchone()[0]:,}")
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
        os.path.dirname(__file__), 'pass4_progress.log'))
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
    logging.info(f"Pass 4 start  dry_run={args.dry_run}  batch_size={args.batch_size}")

    conn = psycopg.connect(DB_DSN, autocommit=False)

    token_map  = load_token_map(conn)
    inf_rules  = load_inflection_rules(conn)

    irregular, archaic = load_variants_from_kaikki(token_map, inf_rules)

    total_ins = total_skp = 0
    ins, skp = insert_variants(conn, irregular, args.batch_size, args.dry_run, "IRREGULAR")
    total_ins += ins; total_skp += skp

    ins, skp = insert_variants(conn, archaic, args.batch_size, args.dry_run, "ARCHAIC/obsolete")
    total_ins += ins; total_skp += skp

    logging.info(f"Pass 4 inserts: inserted={total_ins:,} skipped={total_skp:,}")

    cleanup_superseded_morph_rules(conn, args.dry_run)

    if not args.dry_run:
        verify(conn)

    conn.close()
    logging.info("Done.")


if __name__ == '__main__':
    main()
