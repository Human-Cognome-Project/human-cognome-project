#!/usr/bin/env python3
"""
Merge frequency lists and update hcp_english.tokens.freq_rank.

Sources:
  - Wikipedia word frequency 2023 (MIT) — primary, written English
  - OpenSubtitles full (CC BY-SA-4.0) — supplementary, dialogue English

Strategy:
  Wikipedia rank is primary ordering. For words in both lists, the geometric
  mean of both ranks provides a blended score. Words only in OpenSubtitles
  are appended after all Wikipedia-ranked words.

  This gives written-English ordering as the backbone, with dialogue-common
  words (you, gonna, wanna, etc.) getting a rank boost rather than being lost.

Usage:
  python scripts/merge_frequency_ranks.py [--dry-run]
"""

import sys
import math
import argparse
import psycopg

WIKI_FILE = "/tmp/wiki_freq.txt"
OPENSUB_FILE = "/tmp/opensub_full.txt"

DB_PARAMS = {
    "dbname": "hcp_english",
    "user": "hcp",
    "password": "hcp_dev",
    "host": "localhost",
}


def load_freq_file(path, max_rank=200_000):
    """Load word\tcount file, return {word: rank} (1-based)."""
    words = {}
    rank = 0
    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) < 2:
                continue
            word = parts[0].lower()
            rank += 1
            if rank > max_rank:
                break
            if word not in words:  # first occurrence = highest rank
                words[word] = rank
    return words


def merge_ranks(wiki, opensub):
    """
    Merge two rank dicts into a single ordering.

    - Both present: geometric mean of ranks (blended score)
    - Wiki only: wiki rank (slight penalty to keep it behind dual-sourced words of similar rank)
    - OpenSub only: appended after all wiki-ranked words

    Returns sorted list of (word, final_rank).
    """
    all_words = set(wiki.keys()) | set(opensub.keys())

    scored = []
    sub_only = []

    for word in all_words:
        w = wiki.get(word)
        s = opensub.get(word)

        if w and s:
            # Geometric mean — balances both sources
            score = math.sqrt(w * s)
            scored.append((word, score))
        elif w:
            # Wiki only — use wiki rank directly (these are written-English words
            # that don't appear in dialogue, which is fine for our corpus)
            scored.append((word, float(w)))
        else:
            # OpenSub only — dialogue words not in Wikipedia top 200K
            # Append after all wiki-ranked words
            sub_only.append((word, s))

    # Sort scored words by their blended score
    scored.sort(key=lambda x: x[1])

    # Append sub-only words, ordered by their subtitle rank
    sub_only.sort(key=lambda x: x[1])

    merged = []
    rank = 1
    for word, _ in scored:
        merged.append((word, rank))
        rank += 1
    for word, _ in sub_only:
        merged.append((word, rank))
        rank += 1

    return merged


def update_database(merged, dry_run=False):
    """Update freq_rank in hcp_english.tokens."""
    conn = psycopg.connect(**DB_PARAMS)
    cur = conn.cursor()

    # Ensure column exists
    cur.execute("""
        ALTER TABLE tokens ADD COLUMN IF NOT EXISTS freq_rank INTEGER;
    """)

    # Clear existing ranks
    cur.execute("UPDATE tokens SET freq_rank = NULL WHERE freq_rank IS NOT NULL;")
    cleared = cur.rowcount
    print(f"Cleared {cleared} existing freq_rank values")

    # Build lookup: word -> rank
    rank_lookup = {word: rank for word, rank in merged}

    # Get all token names in AB namespace
    cur.execute("SELECT DISTINCT name FROM tokens WHERE ns LIKE 'AB%';")
    token_names = [row[0] for row in cur.fetchall()]
    print(f"Found {len(token_names)} distinct token names in AB namespace")

    # Match and update
    matched = 0
    batch = []
    for name in token_names:
        rank = rank_lookup.get(name.lower())
        if rank:
            batch.append((rank, name))
            matched += 1

    print(f"Matched {matched} / {len(token_names)} tokens ({100*matched/len(token_names):.1f}%)")

    if dry_run:
        print("\n[DRY RUN] Top 30 ranked tokens would be:")
        batch.sort(key=lambda x: x[0])
        for rank, name in batch[:30]:
            print(f"  {rank:6d}  {name}")
        print(f"\n[DRY RUN] No database changes made.")
        conn.rollback()
    else:
        # Batch update using temp table for speed
        cur.execute("""
            CREATE TEMP TABLE freq_update (
                name TEXT NOT NULL,
                freq_rank INTEGER NOT NULL
            );
        """)
        cur.executemany("INSERT INTO freq_update (name, freq_rank) VALUES (%s, %s)",
                        [(name, rank) for rank, name in batch])
        cur.execute("""
            UPDATE tokens t
            SET freq_rank = f.freq_rank
            FROM freq_update f
            WHERE t.name = f.name AND t.ns LIKE 'AB%';
        """)
        updated = cur.rowcount
        cur.execute("DROP TABLE freq_update;")
        conn.commit()

        # Stats
        cur.execute("""
            SELECT count(*) FILTER (WHERE freq_rank IS NOT NULL) as ranked,
                   count(*) FILTER (WHERE freq_rank <= 1000) as top_1k,
                   count(*) FILTER (WHERE freq_rank <= 10000) as top_10k,
                   count(*) FILTER (WHERE freq_rank <= 50000) as top_50k,
                   count(*) as total
            FROM tokens WHERE ns LIKE 'AB%';
        """)
        row = cur.fetchone()
        print(f"\nResults:")
        print(f"  Updated:  {updated} rows")
        print(f"  Ranked:   {row[0]} / {row[4]} ({100*row[0]/row[4]:.1f}%)")
        print(f"  Top 1K:   {row[1]}")
        print(f"  Top 10K:  {row[2]}")
        print(f"  Top 50K:  {row[3]}")

        # Label vs non-label breakdown
        cur.execute("""
            SELECT subcategory = 'label' as is_label,
                   count(*) FILTER (WHERE freq_rank IS NOT NULL) as ranked,
                   count(*) as total
            FROM tokens WHERE ns LIKE 'AB%'
            GROUP BY subcategory = 'label';
        """)
        for row in cur.fetchall():
            label = "Labels" if row[0] else "Words"
            print(f"  {label}: {row[1]} / {row[2]} ranked")

    cur.close()
    conn.close()


def main():
    parser = argparse.ArgumentParser(description="Merge frequency lists → hcp_english.freq_rank")
    parser.add_argument("--dry-run", action="store_true", help="Show what would happen without writing")
    args = parser.parse_args()

    print("Loading Wikipedia frequency list...")
    wiki = load_freq_file(WIKI_FILE, max_rank=200_000)
    print(f"  {len(wiki)} unique words (top 200K)")

    print("Loading OpenSubtitles frequency list...")
    opensub = load_freq_file(OPENSUB_FILE, max_rank=200_000)
    print(f"  {len(opensub)} unique words (top 200K)")

    print("Merging...")
    merged = merge_ranks(wiki, opensub)
    print(f"  {len(merged)} total ranked words")

    both = len(set(wiki.keys()) & set(opensub.keys()))
    wiki_only = len(set(wiki.keys()) - set(opensub.keys()))
    sub_only = len(set(opensub.keys()) - set(wiki.keys()))
    print(f"  Both: {both}, Wiki-only: {wiki_only}, Sub-only: {sub_only}")

    print("\nUpdating database...")
    update_database(merged, dry_run=args.dry_run)


if __name__ == "__main__":
    main()
