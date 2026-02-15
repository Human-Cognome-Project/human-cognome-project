"""
Simple text-to-PBM ingest for testing cognition module.

Tokenizes text at word level and generates pair bonds from adjacencies.
This is a POC - P's real ingest uses byte-level atomization.

Usage:
    python -m work.hcp.cognition.ingest --text data/gutenberg/texts/00011_*.txt
    python -m work.hcp.cognition.ingest --text FILE --output pbm.db
"""
from __future__ import annotations

import re
import sqlite3
from pathlib import Path
from dataclasses import dataclass
from typing import Iterator
from collections import defaultdict

from ..core.token_id import TokenID, encode_base20
from ..core.pair_bond import PairBondMap, PairBond


# Word token namespace: 0.0.0.3.{word_id} (using PREFIX_WORD from TokenID)
WORD_PREFIX = (0, 0, 0, 3)


@dataclass
class IngestStats:
    """Statistics from ingestion."""
    source_file: str
    total_words: int
    unique_words: int
    total_bonds: int
    unique_bonds: int


class WordTokenizer:
    """
    Simple word-level tokenizer.

    Maps words to TokenIDs in the word namespace (0.0.0.3.*).
    """

    def __init__(self) -> None:
        self._word_to_id: dict[str, TokenID] = {}
        self._id_to_word: dict[TokenID, str] = {}
        self._next_id: int = 0

    def tokenize(self, word: str) -> TokenID:
        """Get or create TokenID for a word."""
        # Normalize: lowercase, strip
        word = word.lower().strip()

        if word not in self._word_to_id:
            # Create new TokenID: 0.0.0.3.{word_id}
            token_id = TokenID(WORD_PREFIX + (self._next_id,))
            self._word_to_id[word] = token_id
            self._id_to_word[token_id] = word
            self._next_id += 1

        return self._word_to_id[word]

    def lookup(self, token: TokenID) -> str | None:
        """Look up word for a token."""
        return self._id_to_word.get(token)

    @property
    def vocab_size(self) -> int:
        return len(self._word_to_id)


def extract_words(text: str) -> Iterator[str]:
    """Extract words from text, preserving order."""
    # Match word characters, allowing internal apostrophes/hyphens
    pattern = r"[a-zA-Z]+(?:['-][a-zA-Z]+)*"
    for match in re.finditer(pattern, text):
        yield match.group()


def text_to_pbm(text: str, tokenizer: WordTokenizer | None = None) -> tuple[PairBondMap, WordTokenizer, IngestStats]:
    """
    Convert text to PairBondMap.

    Returns (pbm, tokenizer, stats).
    """
    if tokenizer is None:
        tokenizer = WordTokenizer()

    pbm = PairBondMap()
    words = list(extract_words(text))

    # Build pair bonds from adjacent words
    prev_token: TokenID | None = None
    for i, word in enumerate(words):
        token = tokenizer.tokenize(word)

        if prev_token is not None:
            pbm.add_bond(prev_token, token, position=i-1)

        prev_token = token

    stats = IngestStats(
        source_file="",
        total_words=len(words),
        unique_words=tokenizer.vocab_size,
        total_bonds=pbm.total_bonds,
        unique_bonds=pbm.unique_bonds,
    )

    return pbm, tokenizer, stats


def ingest_file(filepath: Path, tokenizer: WordTokenizer | None = None) -> tuple[PairBondMap, WordTokenizer, IngestStats]:
    """Ingest a text file to PairBondMap."""
    text = filepath.read_text(encoding='utf-8', errors='replace')
    pbm, tokenizer, stats = text_to_pbm(text, tokenizer)
    stats.source_file = str(filepath)
    return pbm, tokenizer, stats


def save_to_sqlite(
    pbm: PairBondMap,
    tokenizer: WordTokenizer,
    db_path: Path,
    source_name: str = "unknown"
) -> None:
    """Save PBM and vocabulary to SQLite database."""
    conn = sqlite3.connect(db_path)
    cur = conn.cursor()

    # Create tables
    cur.executescript("""
        CREATE TABLE IF NOT EXISTS vocabulary (
            token_id TEXT PRIMARY KEY,
            word TEXT NOT NULL
        );

        CREATE TABLE IF NOT EXISTS pbms (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            source TEXT NOT NULL,
            left_token TEXT NOT NULL,
            right_token TEXT NOT NULL,
            count INTEGER NOT NULL DEFAULT 1
        );

        CREATE INDEX IF NOT EXISTS idx_pbms_left ON pbms(left_token);
        CREATE INDEX IF NOT EXISTS idx_pbms_right ON pbms(right_token);
        CREATE INDEX IF NOT EXISTS idx_pbms_source ON pbms(source);
    """)

    # Insert vocabulary
    vocab_data = [(_token_to_str(token), word) for word, token in tokenizer._word_to_id.items()]
    cur.executemany(
        "INSERT OR REPLACE INTO vocabulary (token_id, word) VALUES (?, ?)",
        vocab_data
    )

    # Insert bonds
    bond_data = []
    for left_token in pbm._bonds:
        for right_token, recurrence in pbm._bonds[left_token].items():
            bond_data.append((
                source_name,
                _token_to_str(left_token),
                _token_to_str(right_token),
                recurrence.count
            ))

    cur.executemany(
        "INSERT INTO pbms (source, left_token, right_token, count) VALUES (?, ?, ?, ?)",
        bond_data
    )

    conn.commit()
    conn.close()


def _token_to_str(token: TokenID) -> str:
    """Convert TokenID to string representation."""
    return '.'.join(str(s) for s in token.segments)


def _str_to_token(token_str: str) -> TokenID:
    """Convert string back to TokenID."""
    parts = tuple(int(s) for s in token_str.split('.'))
    return TokenID(parts)


def load_from_sqlite(db_path: Path, source: str | None = None) -> tuple[PairBondMap, WordTokenizer]:
    """Load PBM and vocabulary from SQLite database."""
    conn = sqlite3.connect(db_path)
    cur = conn.cursor()

    # Load vocabulary
    tokenizer = WordTokenizer()
    cur.execute("SELECT token_id, word FROM vocabulary")
    for token_str, word in cur.fetchall():
        token = _str_to_token(token_str)
        tokenizer._word_to_id[word] = token
        tokenizer._id_to_word[token] = word
        # Track next_id based on max word id seen
        if token.prefix == WORD_PREFIX:
            tokenizer._next_id = max(tokenizer._next_id, token.value + 1)

    # Load bonds
    pbm = PairBondMap()
    query = "SELECT left_token, right_token, count FROM pbms"
    if source:
        query += " WHERE source = ?"
        cur.execute(query, (source,))
    else:
        cur.execute(query)

    for left_str, right_str, count in cur.fetchall():
        left = _str_to_token(left_str)
        right = _str_to_token(right_str)
        # Add bond with count
        for _ in range(count):
            pbm.add_bond(left, right)

    conn.close()
    return pbm, tokenizer


def main():
    """CLI for text ingestion."""
    import argparse

    parser = argparse.ArgumentParser(description="Ingest text to PBM")
    parser.add_argument("--text", type=Path, required=True, help="Text file to ingest")
    parser.add_argument("--output", type=Path, default=Path("hcp_pbms.db"), help="Output SQLite DB")
    parser.add_argument("--verbose", "-v", action="store_true", help="Verbose output")
    parser.add_argument("--accumulate", "-a", action="store_true", help="Accumulate vocabulary from existing DB")
    args = parser.parse_args()

    # Load existing tokenizer if accumulating
    tokenizer = None
    if args.accumulate and args.output.exists():
        try:
            _, tokenizer = load_from_sqlite(args.output)
            print(f"Loaded existing vocabulary: {tokenizer.vocab_size:,} words")
        except Exception:
            pass

    # Ingest
    print(f"Ingesting: {args.text}")
    pbm, tokenizer, stats = ingest_file(args.text, tokenizer)

    print(f"  Words: {stats.total_words:,}")
    print(f"  Unique words: {stats.unique_words:,}")
    print(f"  Bonds: {stats.total_bonds:,}")
    print(f"  Unique bonds: {stats.unique_bonds:,}")

    # Save
    source_name = args.text.stem
    save_to_sqlite(pbm, tokenizer, args.output, source_name)
    print(f"Saved to: {args.output}")

    # Verify
    if args.verbose:
        loaded_pbm, loaded_tokenizer = load_from_sqlite(args.output, source_name)
        print(f"Verified: {loaded_pbm.total_bonds:,} bonds, {loaded_tokenizer.vocab_size:,} vocab")


if __name__ == "__main__":
    main()
