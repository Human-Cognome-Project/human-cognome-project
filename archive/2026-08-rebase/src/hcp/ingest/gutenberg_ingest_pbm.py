"""
Gutenberg PBM Ingestion Pipeline

Reads Gutenberg texts and ingests into hcp_en_pbm PostgreSQL database.
- Tokenizes text and metadata
- Looks up/inserts tokens in hcp_english and hcp_names
- Creates FPB bonds
- Stores everything as token IDs
"""

import re
import json
import hashlib
from pathlib import Path
from typing import List, Dict, Tuple, Optional
from dataclasses import dataclass
from collections import Counter
import psycopg


@dataclass
class Token:
    """A tokenized word."""
    string: str
    is_capitalized: bool
    position: int


class TokenManager:
    """Manages token lookup and insertion in hcp_english and hcp_names."""

    def __init__(self, conn_english, conn_names):
        self.conn_english = conn_english
        self.conn_names = conn_names
        self.name_token_counter = self._get_next_name_counter()
        self.new_names = []  # Track for post-processing

    def _get_next_name_counter(self) -> int:
        """Get next available counter for y* namespace."""
        with self.conn_names.cursor() as cur:
            # Find the maximum numeric suffix in yA.NNNNN format
            cur.execute("""
                SELECT MAX(CAST(SUBSTRING(token_id FROM 'yA\.([0-9]+)') AS INTEGER))
                FROM tokens
                WHERE token_id ~ '^yA\.[0-9]+$'
            """)
            row = cur.fetchone()
            if row and row[0] is not None:
                return row[0] + 1
            return 1

    def get_or_insert_token(self, word: str, is_capitalized: bool) -> str:
        """
        Get token ID for word, inserting if new.

        Args:
            word: The word string
            is_capitalized: Whether word is capitalized

        Returns:
            Token ID (from hcp_english or hcp_names)
        """
        if is_capitalized:
            return self._get_or_insert_name(word)
        else:
            return self._get_or_insert_word(word)

    def _get_or_insert_name(self, word: str) -> str:
        """
        Get or insert capitalized word.

        First checks hcp_names for capitalized form (actual proper noun).
        Then checks hcp_english for lowercase version (common word at sentence start).
        Only adds to hcp_names if neither found (monitor these!).
        """
        with self.conn_names.cursor() as cur:
            # Check if exists in hcp_names (capitalized)
            cur.execute("SELECT token_id FROM tokens WHERE name = %s", (word,))
            row = cur.fetchone()
            if row:
                return row[0]

        # Check if lowercase version exists in hcp_english
        with self.conn_english.cursor() as cur:
            lowercase_word = word.lower()
            cur.execute("SELECT token_id FROM tokens WHERE name = %s", (lowercase_word,))
            row = cur.fetchone()
            if row:
                # Found lowercase version - use it (capitalization is formatting)
                return row[0]

        # Not found in either - add to hcp_names (monitor these!)
        with self.conn_names.cursor() as cur:
            token_id = f"yA.{self.name_token_counter:05d}"  # Simplified
            cur.execute(
                "INSERT INTO tokens (token_id, name, atomization, metadata) VALUES (%s, %s, %s, %s)",
                (token_id, word, '[]', '{}')
            )
            self.conn_names.commit()
            self.name_token_counter += 1
            self.new_names.append(word)
            return token_id

    def _get_or_insert_word(self, word: str) -> str:
        """Get or insert uncapitalized word in hcp_english (or tbd_tokens)."""
        with self.conn_english.cursor() as cur:
            # Try exact match first
            cur.execute("SELECT token_id FROM tokens WHERE name = %s", (word,))
            row = cur.fetchone()
            if row:
                return row[0]

            # If word ends with apostrophe or 's, try fuzzy search
            if word.endswith("'") or word.endswith("'s"):
                # Try without the apostrophe/possessive
                base_word = word.rstrip("'s").rstrip("'")
                if base_word:
                    cur.execute("SELECT token_id FROM tokens WHERE name = %s", (base_word,))
                    row = cur.fetchone()
                    if row:
                        return row[0]

            # TODO: Insert into tbd_tokens table (not implemented yet)
            # For now, return a placeholder
            return f"TBD_{word}"


class Tokenizer:
    """Tokenize text into words and punctuation."""

    PUNCTUATION = set('.,;:!?()[]{}"-—…_')

    def tokenize(self, text: str) -> List[Token]:
        """
        Tokenize text, preserving capitalization info.

        Simple rule: If apostrophe follows a letter, include it and any following
        letters as part of the word token. Fuzzy lookup handles possessives.
        """
        tokens = []
        position = 0
        i = 0

        while i < len(text):
            char = text[i]

            # Skip whitespace
            if char.isspace():
                position += 1
                i += 1
                continue

            # Handle punctuation (except apostrophe after letters)
            if char in self.PUNCTUATION:
                tokens.append(Token(char, False, position))
                position += 1
                i += 1
                continue

            # Handle standalone apostrophe (not after a letter)
            if char == "'" and (i == 0 or not text[i-1].isalpha()):
                tokens.append(Token(char, False, position))
                position += 1
                i += 1
                continue

            # Handle word characters (letters/numbers)
            # Include apostrophes if they follow letters
            if char.isalnum() or (char == "'" and i > 0 and text[i-1].isalpha()):
                word_start = i
                while i < len(text):
                    if text[i].isalnum():
                        i += 1
                    elif text[i] == "'" and i > 0 and text[i-1].isalpha():
                        # Apostrophe after letter - include it and any following letters
                        i += 1
                        while i < len(text) and (text[i].isalnum() or text[i] == "'"):
                            i += 1
                        break
                    else:
                        break

                word = text[word_start:i]
                is_cap = word[0].isupper() if word and word[0].isalpha() else False
                tokens.append(Token(word, is_cap, position))
                position += len(word)
                continue

            # Unknown character - skip
            i += 1
            position += 1

        return tokens


class GutenbergPBMIngester:
    """Ingest Gutenberg texts into hcp_en_pbm database."""

    def __init__(self):
        # Connect to databases
        self.conn_pbm = psycopg.connect(
            host="localhost",
            dbname="hcp_en_pbm",
            user="hcp",
            password="hcp_dev"
        )
        self.conn_english = psycopg.connect(
            host="localhost",
            dbname="hcp_english",
            user="hcp",
            password="hcp_dev"
        )
        self.conn_names = psycopg.connect(
            host="localhost",
            dbname="hcp_names",
            user="hcp",
            password="hcp_dev"
        )

        self.token_manager = TokenManager(self.conn_english, self.conn_names)
        self.tokenizer = Tokenizer()
        self.doc_counter = 1

    def process_book(self, text_file: Path, metadata: Dict) -> str:
        """
        Process a Gutenberg book into PBM database.

        Args:
            text_file: Path to text file
            metadata: Book metadata from Gutendex

        Returns:
            Document token ID (z* namespace)
        """
        print(f"\nProcessing: {metadata['title']}")

        # Read and clean text
        text = text_file.read_text(encoding='utf-8')
        text = self._strip_gutenberg_boilerplate(text)

        # Tokenize
        tokens = self.tokenizer.tokenize(text)
        print(f"  Tokenized: {len(tokens)} tokens")

        # Get token IDs, handling apostrophe splitting
        token_ids = []
        for token in tokens:
            if token.string in self.tokenizer.PUNCTUATION:
                # Punctuation - lookup in core or use literal
                token_id = token.string  # Simplified for now
                token_ids.append(token_id)
            else:
                # Try lookup of whole token first
                token_id = self.token_manager.get_or_insert_token(
                    token.string,
                    token.is_capitalized
                )

                # If not found (TBD) and ends with just apostrophe (not 's),
                # check if it's a closing quote by testing base word
                if (token_id.startswith("TBD_") and
                    token.string.endswith("'") and
                    not token.string.endswith("'s") and
                    len(token.string) > 1):

                    base_word = token.string[:-1]
                    base_token_id = self.token_manager.get_or_insert_token(
                        base_word,
                        token.is_capitalized
                    )

                    # If base word found, it's a closing quote - split it
                    if not base_token_id.startswith("TBD_"):
                        token_ids.append(base_token_id)
                        token_ids.append("'")  # Apostrophe as separate closing quote
                    else:
                        # Base word also not found - keep together
                        token_ids.append(token_id)
                else:
                    # Token found, or doesn't match split pattern
                    token_ids.append(token_id)

        # Create FPB bonds
        bond_counts = Counter()
        for i in range(len(token_ids) - 1):
            bond_counts[(token_ids[i], token_ids[i + 1])] += 1

        print(f"  FPB bonds: {len(bond_counts)} unique pairs")

        # Assign document token ID (z* namespace)
        doc_token_id = f"zA.{self.doc_counter:05d}"  # Simplified
        self.doc_counter += 1

        # Insert into database
        self._insert_document(doc_token_id, text_file, text, metadata, token_ids, bond_counts)

        return doc_token_id

    def _insert_document(self, doc_token_id: str, text_file: Path, text: str,
                        metadata: Dict, token_ids: List[str],
                        bond_counts: Counter):
        """Insert document and all related data into hcp_en_pbm."""

        with self.conn_pbm.cursor() as cur:
            # TODO: Process authors, title, genres, etc. into token IDs
            # For now, simplified insertion

            # Calculate hash
            content_hash = hashlib.sha256(text.encode()).hexdigest()

            # First FPB for reconstruction seed
            first_fpb = [token_ids[0], token_ids[1]] if len(token_ids) >= 2 else None

            # Insert document
            cur.execute("""
                INSERT INTO documents (
                    token_id, external_id, source_token, title_tokens,
                    document_type_token, publication_year, language_token,
                    file_path, content_hash, char_count, token_count,
                    download_count, first_fpb
                ) VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
            """, (
                doc_token_id,
                str(metadata.get('id')),
                'gutenberg',  # TODO: Tokenize
                ['TBD'],  # TODO: Tokenize title
                'fiction',  # TODO: Tokenize
                metadata.get('publication_year'),
                'en',  # TODO: Tokenize
                str(text_file),
                content_hash,
                len(text),
                len(token_ids),
                metadata.get('download_count'),
                first_fpb
            ))

            # Insert FPB bonds
            bond_data = [
                (doc_token_id, token0, token1, count)
                for (token0, token1), count in bond_counts.items()
            ]

            cur.executemany(
                "INSERT INTO fpb_bonds (doc_token_id, token0_id, token1_id, fbr) VALUES (%s, %s, %s, %s)",
                bond_data
            )

            self.conn_pbm.commit()
            print(f"  ✓ Inserted: {doc_token_id}")

    def _strip_gutenberg_boilerplate(self, text: str) -> str:
        """Remove Gutenberg header/footer."""
        start_pattern = r'\*\*\* START OF (?:THE|THIS) PROJECT GUTENBERG EBOOK.*?\*\*\*'
        match = re.search(start_pattern, text, re.IGNORECASE)
        if match:
            text = text[match.end():]

        end_pattern = r'\*\*\* END OF (?:THE|THIS) PROJECT GUTENBERG EBOOK.*?\*\*\*'
        match = re.search(end_pattern, text, re.IGNORECASE)
        if match:
            text = text[:match.start()]

        return text.strip()

    def close(self):
        """Close database connections."""
        self.conn_pbm.close()
        self.conn_english.close()
        self.conn_names.close()


def main():
    """Run ingestion on downloaded Gutenberg books."""

    # Load metadata
    metadata_file = Path("data/gutenberg/metadata.json")
    with open(metadata_file) as f:
        books_metadata = json.load(f)

    # Get text files
    text_dir = Path("data/gutenberg/texts")
    text_files = sorted(text_dir.glob("*.txt"))

    print(f"Found {len(text_files)} text files")
    print(f"Found {len(books_metadata)} metadata entries")

    # Create ingester
    ingester = GutenbergPBMIngester()

    try:
        # Process each book
        for text_file in text_files:
            # Find matching metadata
            # Extract Gutenberg ID from filename (e.g., "00084_Frankenstein...")
            gutenberg_id = int(text_file.name.split('_')[0])

            metadata = next(
                (m for m in books_metadata if m['id'] == gutenberg_id),
                None
            )

            if metadata:
                doc_token_id = ingester.process_book(text_file, metadata)
            else:
                print(f"  Warning: No metadata for {text_file.name}")

        print("\n" + "="*60)
        print("Ingestion complete!")
        print("="*60)

        # TODO: Post-processing for capitalized words
        if ingester.token_manager.new_names:
            print(f"\nNew name components added: {len(ingester.token_manager.new_names)}")
            print("  (Check for sentence-initial false positives)")

    finally:
        ingester.close()


if __name__ == "__main__":
    main()
