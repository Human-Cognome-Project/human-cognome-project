"""Ingest canonical NSM molecules from Learn These Words First data.

This script parses the LTWF data files to extract:
1. The 367 canonical words (65 primes + ~300 molecules)
2. Their order in the lesson structure (defines tier/abstraction depth)
3. Tags corresponding words in the english shard

Once seeded, these canonical molecules act as building blocks for the rest
of the dictionary. The molecule walk can then propagate through the 1.25M
words in the shard using the canonical molecules as the foundation.

Data files from: https://learnthesewordsfirst.com/tools/
- NonCircularEnglish-LessonData.txt: 367 entries, primes + molecules
- ParaphraseWithUniversals.txt: molecule decompositions to universals
- NonCircularEnglish-IndexData.txt: 2000 word dictionary (optional)
"""

import re
import sys
from pathlib import Path

# Adjust path to find hcp module
sys.path.insert(0, str(Path(__file__).parent.parent.parent))

from hcp.db.english import connect as connect_english


def parse_lesson_data(filepath: Path) -> list[dict]:
    """Parse the LessonData file to extract canonical words in order.

    Returns list of entries with:
    - lesson_number: str (e.g., "1-01", "2-15")
    - words: list[str] (extracted from {curly braces})
    - definition: str (text in [[double brackets]])
    - order: int (sequential order in file)
    """
    entries = []
    order = 0

    with open(filepath, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()

            # Skip comments and empty lines
            if not line or line.startswith('#'):
                continue

            # Match lines like: 1-01. {word}, {words}, ... [[definition]]
            match = re.match(r'^(\d+-\d+)\.\s+(.+)$', line)
            if not match:
                continue

            lesson_num = match.group(1)
            content = match.group(2)

            # Extract words from {curly braces}
            words = []
            for word_match in re.finditer(r'\{([^}]+)\}', content):
                word = word_match.group(1)
                # Clean up: remove underscores, lowercase
                word = word.replace('_', ' ').strip()
                if word and not word.startswith('-'):  # Skip inflection markers like {-seeing}
                    words.append(word)

            # Extract definition from [[double brackets]]
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


def parse_paraphrase_file(filepath: Path) -> dict[str, str]:
    """Parse ParaphraseWithUniversals to get molecule decompositions.

    Returns dict mapping word -> paraphrase definition.
    """
    paraphrases = {}

    with open(filepath, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()

            # Skip comments, empty lines, section headers
            if not line or line.startswith('#') or line.startswith('---'):
                continue

            # Match lines like: {word} = definition
            match = re.match(r'^\{([^}]+)\}\s*=\s*(.+)$', line)
            if match:
                word = match.group(1).replace('_', ' ').strip()
                definition = match.group(2).strip()
                paraphrases[word] = definition
            # Also capture standalone molecule names (universal molecules)
            elif line.startswith('{') and '=' not in line:
                words = re.findall(r'\{([^}]+)\}', line)
                for word in words:
                    word = word.replace('_', ' ').strip()
                    if word and not word.startswith('-') and word not in paraphrases:
                        paraphrases[word] = '[universal molecule]'

    return paraphrases


def tag_words_in_shard(conn, canonical_words: list[dict], paraphrases: dict[str, str]):
    """Tag words in the english shard with canonical molecule metadata.

    Args:
        conn: database connection to hcp_english
        canonical_words: list of canonical word entries from LessonData
        paraphrases: dict of molecule paraphrases
    """
    # First, add columns if they don't exist
    with conn.cursor() as cur:
        # Check if columns exist
        cur.execute("""
            SELECT column_name
            FROM information_schema.columns
            WHERE table_name = 'tokens'
              AND column_name IN ('nsm_canonical_order', 'nsm_lesson_number', 'nsm_is_universal')
        """)
        existing_cols = {row[0] for row in cur.fetchall()}

        if 'nsm_canonical_order' not in existing_cols:
            print("Adding nsm_canonical_order column to tokens...")
            cur.execute("ALTER TABLE tokens ADD COLUMN nsm_canonical_order INTEGER")

        if 'nsm_lesson_number' not in existing_cols:
            print("Adding nsm_lesson_number column to tokens...")
            cur.execute("ALTER TABLE tokens ADD COLUMN nsm_lesson_number VARCHAR(10)")

        if 'nsm_is_universal' not in existing_cols:
            print("Adding nsm_is_universal column to tokens...")
            cur.execute("ALTER TABLE tokens ADD COLUMN nsm_is_universal BOOLEAN DEFAULT FALSE")

        conn.commit()

    print(f"\nTagging {len(canonical_words)} canonical entries in english shard...")

    tagged_count = 0
    not_found = []

    for entry in canonical_words:
        lesson_num = entry['lesson_number']
        order = entry['order']
        words_list = entry['words']

        for word in words_list:
            # Determine if universal (appears in paraphrases as [universal molecule])
            is_universal = paraphrases.get(word) == '[universal molecule]'

            # Try to find matching token(s) in english shard
            with conn.cursor() as cur:
                # Try exact match first (case-insensitive)
                cur.execute("""
                    SELECT token_id, name
                    FROM tokens
                    WHERE LOWER(name) = LOWER(%s)
                      AND layer = 'word'
                    LIMIT 5
                """, (word,))

                matches = cur.fetchall()

                if matches:
                    for token_id, name in matches:
                        cur.execute("""
                            UPDATE tokens
                            SET nsm_canonical_order = %s,
                                nsm_lesson_number = %s,
                                nsm_is_universal = %s
                            WHERE token_id = %s
                        """, (order, lesson_num, is_universal, token_id))
                        tagged_count += 1
                        print(f"  {lesson_num} [{order:3d}] {token_id} {name:20s} {'[universal]' if is_universal else ''}")
                else:
                    not_found.append((lesson_num, word))

    conn.commit()

    print(f"\nTagged {tagged_count} tokens with canonical molecule metadata")

    if not_found:
        print(f"\nNot found in shard ({len(not_found)} words):")
        for lesson_num, word in not_found[:20]:
            print(f"  {lesson_num}: {word}")
        if len(not_found) > 20:
            print(f"  ... and {len(not_found) - 20} more")


def verify_tagging(conn):
    """Print stats on the tagged molecules."""
    with conn.cursor() as cur:
        cur.execute("""
            SELECT
                COUNT(*) FILTER (WHERE nsm_canonical_order IS NOT NULL) as tagged_count,
                COUNT(*) FILTER (WHERE nsm_is_universal = TRUE) as universal_count,
                MIN(nsm_canonical_order) as min_order,
                MAX(nsm_canonical_order) as max_order
            FROM tokens
        """)
        tagged_count, universal_count, min_order, max_order = cur.fetchone()

        print(f"\n{'='*60}")
        print(f"Canonical Molecule Tagging Results:")
        print(f"{'='*60}")
        print(f"Total tokens tagged: {tagged_count}")
        print(f"Universal molecules: {universal_count}")
        print(f"Order range: {min_order} to {max_order}")

        # Sample distribution
        cur.execute("""
            SELECT
                nsm_lesson_number,
                COUNT(*) as count,
                COUNT(*) FILTER (WHERE nsm_is_universal = TRUE) as universal
            FROM tokens
            WHERE nsm_canonical_order IS NOT NULL
            GROUP BY nsm_lesson_number
            ORDER BY nsm_lesson_number
            LIMIT 15
        """)

        print(f"\nSample distribution by lesson:")
        print(f"{'Lesson':<10} {'Count':<8} {'Universal':<10}")
        print(f"{'-'*30}")
        for lesson, count, universal in cur.fetchall():
            print(f"{lesson:<10} {count:<8} {universal:<10}")


def main():
    import argparse

    parser = argparse.ArgumentParser(description='Ingest NSM canonical molecules')
    parser.add_argument('data_dir', help='Directory containing LTWF data files')
    args = parser.parse_args()

    data_dir = Path(args.data_dir)
    lesson_file = data_dir / 'NonCircularEnglish-LessonData.txt'
    paraphrase_file = data_dir / 'ParaphraseWithUniversals.txt'

    if not lesson_file.exists():
        print(f"Error: {lesson_file} not found")
        return 1

    if not paraphrase_file.exists():
        print(f"Error: {paraphrase_file} not found")
        return 1

    print("Parsing LessonData file...")
    canonical_words = parse_lesson_data(lesson_file)
    print(f"Found {len(canonical_words)} canonical entries")

    # Filter to only molecules (skip first ~65 primes, they're already in core shard)
    # Primes are order 0-64, molecules start around 65+
    molecules = [entry for entry in canonical_words if entry['order'] >= 65]
    print(f"Filtered to {len(molecules)} canonical molecules (skipping {len(canonical_words) - len(molecules)} primes)")

    print("\nParsing ParaphraseWithUniversals file...")
    paraphrases = parse_paraphrase_file(paraphrase_file)
    print(f"Found {len(paraphrases)} molecule paraphrases")

    # Count universal vs non-universal
    universal_count = sum(1 for v in paraphrases.values() if v == '[universal molecule]')
    print(f"  - Universal molecules: {universal_count}")
    print(f"  - Non-universal molecules: {len(paraphrases) - universal_count}")

    print("\nConnecting to hcp_english database...")
    conn = connect_english()

    try:
        tag_words_in_shard(conn, molecules, paraphrases)
        verify_tagging(conn)
    finally:
        conn.close()

    print("\nDone! Canonical molecules tagged.")
    print("\nNext step: Run nsm_molecules.py to propagate through the definition graph.")

    return 0


if __name__ == '__main__':
    sys.exit(main())
