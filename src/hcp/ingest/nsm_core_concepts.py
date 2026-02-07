"""Ingest NSM concepts into core shard at AA.AC.AA.{tier}.{count}.

This script ingests the three tiers of NSM/LTWF concepts into the core shard:
- AA.AC.AA.AA.{n} = 65 NSM primes (universal primitives)
- AA.AC.AA.AB.{n} = ~300 Tier 1 molecules (universal/near-universal)
- AA.AC.AA.AC.{n} = ~2000 noncircular entries (common defining vocabulary)

All three tiers go into core shard as universal human-centric conceptual
foundations. Language shards contain surface expressions that decompose
through these concepts.

Data files from: https://learnthesewordsfirst.com/tools/
"""

import re
import sys
import json
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent.parent))

from hcp.db.postgres import connect as connect_core
from hcp.db.english import connect as connect_english
from hcp.core.token_id import encode_pair, decode_pair


def parse_lesson_data(filepath: Path) -> list[dict]:
    """Parse LessonData to extract all entries (primes + molecules).

    Returns list with lesson_number, words, definition, order.
    """
    entries = []
    order = 0

    with open(filepath, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue

            match = re.match(r'^(\d+-\d+)\.\s+(.+)$', line)
            if not match:
                continue

            lesson_num = match.group(1)
            content = match.group(2)

            # Extract primary words from {curly braces}, skip inflections
            words = []
            for word_match in re.finditer(r'\{([^}]+)\}', content):
                word = word_match.group(1).replace('_', ' ').strip()
                if word and not word.startswith('-'):
                    words.append(word)

            # Extract definitions
            definitions = re.findall(r'\[\[([^\]]+)\]\]', content)
            definition = ' | '.join(definitions) if definitions else ''

            if words:
                entries.append({
                    'lesson_number': lesson_num,
                    'words': words,
                    'definition': definition,
                    'order': order
                })
                order += 1

    return entries


def parse_index_data(filepath: Path) -> list[dict]:
    """Parse IndexData to extract 2000-word dictionary.

    Returns list with word, definition.
    """
    entries = []

    with open(filepath, 'r', encoding='utf-8') as f:
        for line_num, line in enumerate(f, 1):
            original_line = line
            line = line.strip()

            # Match: # INDEX: word(s) = definition
            # Word can have spaces, commas, parentheses: "am, are", "armour, (armor)"
            if line.startswith('# INDEX:'):
                match = re.match(r'^#\s*INDEX:\s*([^=]+?)\s*=\s*(.+)$', line)
                if match:
                    word = match.group(1).strip()
                    definition = match.group(2).strip()
                    # For multi-word entries, use first word as primary
                    primary_word = word.split(',')[0].strip().replace('(', '').replace(')', '')
                    entries.append({
                        'word': primary_word,
                        'definition': definition,
                        'variants': word  # Keep full variant list
                    })
                else:
                    print(f"WARNING: Line {line_num} doesn't match INDEX pattern: {line[:80]}")

    return entries


def parse_paraphrase_file(filepath: Path) -> dict[str, str]:
    """Parse ParaphraseWithUniversals to identify universal molecules.

    Returns dict: word -> paraphrase (or '[universal molecule]' marker).
    """
    paraphrases = {}

    with open(filepath, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#') or line.startswith('---'):
                continue

            # Lines with = are paraphrases for non-universal molecules
            match = re.match(r'^\{([^}]+)\}\s*=\s*(.+)$', line)
            if match:
                word = match.group(1).replace('_', ' ').strip()
                definition = match.group(2).strip()
                paraphrases[word] = definition
            # Standalone {word} in universal section = universal molecule
            elif line.startswith('{') and '=' not in line:
                words = re.findall(r'\{([^}]+)\}', line)
                for word in words:
                    word = word.replace('_', ' ').strip()
                    if word and not word.startswith('-') and word not in paraphrases:
                        paraphrases[word] = '[universal molecule]'

    return paraphrases


def find_english_exponents(conn, words: list[str]) -> list[str]:
    """Find token IDs for English exponents of a concept.

    Args:
        conn: connection to hcp_english
        words: list of word forms for this concept

    Returns:
        list of token IDs found in english shard
    """
    token_ids = []

    with conn.cursor() as cur:
        for word in words:
            cur.execute("""
                SELECT token_id
                FROM tokens
                WHERE LOWER(name) = LOWER(%s)
                  AND layer = 'word'
                LIMIT 5
            """, (word,))

            for (tid,) in cur.fetchall():
                token_ids.append(tid)

    return token_ids


def ingest_tier(core_conn, eng_conn, tier_code: str, entries: list[dict],
                paraphrases: dict[str, str], start_index: int = 0):
    """Ingest one tier of concepts into core shard.

    Args:
        core_conn: connection to hcp_core
        eng_conn: connection to hcp_english
        tier_code: 'AA' for primes, 'AB' for tier1, 'AC' for tier2
        entries: list of concept entries
        paraphrases: dict of universal molecule markers
        start_index: starting index for token IDs (default 0)
    """
    print(f"\nIngesting tier {tier_code}: {len(entries)} concepts")

    inserted = 0
    not_found = []

    for i, entry in enumerate(entries):
        # Generate token ID: AA.AC.AA.{tier}.{count}
        count = start_index + i
        count_pair = encode_pair(count)
        token_id = f"AA.AC.AA.{tier_code}.{count_pair}"

        # Get concept name (first word as canonical)
        if isinstance(entry, dict) and 'words' in entry:
            # Lesson data format
            words = entry['words']
            name = words[0] if words else f"concept_{count}"
            definition = entry.get('definition', '')
            variants = None
        else:
            # Index data format
            name = entry['word']
            definition = entry['definition']
            variants = entry.get('variants')  # May have variant forms
            words = [name]

        # Check if universal
        is_universal = any(paraphrases.get(w) == '[universal molecule]' for w in words)

        # Find English exponents
        eng_refs = find_english_exponents(eng_conn, words)

        # Build metadata
        metadata = {
            'eng_refs': eng_refs,
            'is_universal': is_universal
        }
        if definition:
            metadata['definition'] = definition
        if variants:
            metadata['variants'] = variants

        # Insert into core
        with core_conn.cursor() as cur:
            cur.execute("""
                INSERT INTO tokens (token_id, name, category, subcategory, metadata)
                VALUES (%s, %s, %s, %s, %s)
                ON CONFLICT (token_id) DO UPDATE SET
                    name = EXCLUDED.name,
                    category = EXCLUDED.category,
                    subcategory = EXCLUDED.subcategory,
                    metadata = EXCLUDED.metadata
            """, (
                token_id,
                name,
                f'nsm_{tier_code.lower()}',  # category: nsm_aa, nsm_ab, nsm_ac
                'universal' if is_universal else 'standard',
                json.dumps(metadata)
            ))

        inserted += 1

        # Progress output
        if eng_refs:
            status = '[universal]' if is_universal else ''
            print(f"  {token_id} {name:30s} → {len(eng_refs)} eng refs {status}")
        else:
            not_found.append(name)
            if len(not_found) <= 10:
                print(f"  {token_id} {name:30s} [no eng refs]")

    core_conn.commit()

    print(f"\nInserted {inserted} concepts into tier {tier_code}")
    if not_found:
        print(f"No English refs found for {len(not_found)} concepts")
        if len(not_found) <= 20:
            print(f"  Examples: {', '.join(not_found[:20])}")


def verify_ingestion(conn):
    """Print stats on ingested concepts."""
    with conn.cursor() as cur:
        cur.execute("""
            SELECT
                category,
                COUNT(*) as count,
                COUNT(*) FILTER (WHERE subcategory = 'universal') as universal_count
            FROM tokens
            WHERE token_id LIKE 'AA.AC.AA.%'
            GROUP BY category
            ORDER BY category
        """)

        print(f"\n{'='*60}")
        print(f"Core NSM Concepts Ingestion Summary:")
        print(f"{'='*60}")

        total = 0
        for category, count, universal in cur.fetchall():
            print(f"{category:<20s} {count:>6d} concepts ({universal:>4d} universal)")
            total += count

        print(f"{'-'*60}")
        print(f"{'TOTAL':<20s} {total:>6d} concepts")


def main():
    import argparse

    parser = argparse.ArgumentParser(description='Ingest NSM concepts into core shard')
    parser.add_argument('data_dir', help='Directory containing LTWF data files')
    args = parser.parse_args()

    data_dir = Path(args.data_dir)
    lesson_file = data_dir / 'NonCircularEnglish-LessonData.txt'
    index_file = data_dir / 'NonCircularEnglish-IndexData-uncompressed.txt'
    paraphrase_file = data_dir / 'ParaphraseWithUniversals.txt'

    # Check files exist
    for f in [lesson_file, paraphrase_file]:
        if not f.exists():
            print(f"Error: {f} not found")
            return 1

    # Index file might be compressed
    if not index_file.exists():
        alt_index = data_dir / 'NonCircularEnglish-IndexData.txt'
        if alt_index.exists():
            print(f"Decompressing {alt_index.name}...")
            import gzip
            with gzip.open(alt_index, 'rt', encoding='utf-8') as f_in:
                with open(index_file, 'w', encoding='utf-8') as f_out:
                    f_out.write(f_in.read())
        else:
            print(f"Error: {index_file} not found")
            return 1

    print("Parsing data files...")

    # Parse all three sources
    print("\n1. Parsing LessonData (primes + tier1 molecules)...")
    lesson_entries = parse_lesson_data(lesson_file)
    primes = lesson_entries[:65]  # First 65 are primes
    tier1_molecules = lesson_entries[65:]  # Rest are tier 1 molecules
    print(f"   Found {len(primes)} primes, {len(tier1_molecules)} tier 1 molecules")

    print("\n2. Parsing IndexData (tier2 noncircular entries)...")
    tier2_entries = parse_index_data(index_file)
    print(f"   Found {len(tier2_entries)} noncircular entries")

    print("\n3. Parsing ParaphraseWithUniversals...")
    paraphrases = parse_paraphrase_file(paraphrase_file)
    universal_count = sum(1 for v in paraphrases.values() if v == '[universal molecule]')
    print(f"   Found {universal_count} universal molecule markers")

    # Connect to databases
    print("\nConnecting to databases...")
    core_conn = connect_core()
    eng_conn = connect_english()

    try:
        # Ingest three tiers
        ingest_tier(core_conn, eng_conn, 'AA', primes, paraphrases, start_index=0)
        ingest_tier(core_conn, eng_conn, 'AB', tier1_molecules, paraphrases, start_index=0)
        ingest_tier(core_conn, eng_conn, 'AC', tier2_entries, paraphrases, start_index=0)

        verify_ingestion(core_conn)

    finally:
        core_conn.close()
        eng_conn.close()

    print("\nDone! NSM concepts ingested into core shard at AA.AC.AA.*")
    print("\nNext step: Run molecule walk on english shard to propagate decompositions.")

    return 0


if __name__ == '__main__':
    sys.exit(main())
