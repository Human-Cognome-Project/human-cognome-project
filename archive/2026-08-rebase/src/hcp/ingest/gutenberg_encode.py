"""
Gutenberg text encoding pipeline: text → tokens → PBMs

Handles word-level tokenization with whitespace removal and extrapolation rules.
"""

import re
from pathlib import Path
from typing import List, Dict, Tuple, Optional
from dataclasses import dataclass
from collections import Counter, defaultdict
import json


@dataclass
class Token:
    """A single token (word or punctuation)."""
    string: str
    type: str  # 'word', 'punctuation', 'number', 'control'
    position: int  # Character offset in original text


@dataclass
class ForwardPairBond:
    """A forward pair bond between two tokens."""
    token_0: str
    token_1: str
    fbr: int  # Forward Bond Recurrence (count)


@dataclass
class PBM:
    """Pair-Bond Map for a scope."""
    scope_id: str
    scope_type: str  # 'sentence', 'paragraph', 'document'
    tokens: List[Token]
    bonds: List[ForwardPairBond]
    metadata: Dict


class Tokenizer:
    """Tokenize text into words and punctuation, removing whitespace."""

    # Punctuation patterns
    PUNCTUATION = set('.,;:!?()[]{}"\'-—…')

    def __init__(self, language: str = 'en'):
        self.language = language

    def tokenize(self, text: str) -> List[Token]:
        """
        Tokenize text into words and punctuation.

        Whitespace is removed - it will be extrapolated during reconstruction.

        Args:
            text: Input text

        Returns:
            List of Token objects
        """
        tokens = []
        position = 0

        # Split on whitespace and punctuation while preserving both
        # Use regex to split while keeping delimiters
        pattern = r"(\s+|[.,;:!?()\[\]{}\"'\-—…])"
        parts = re.split(pattern, text)

        for part in parts:
            if not part:  # Skip empty
                continue

            if part.isspace():  # Skip whitespace (removed, will be extrapolated)
                position += len(part)
                continue

            # Classify token
            if part in self.PUNCTUATION:
                token_type = 'punctuation'
            elif part.isdigit():
                token_type = 'number'
            else:
                token_type = 'word'  # Includes proper nouns - semantic analysis happens later

            tokens.append(Token(
                string=part,
                type=token_type,
                position=position
            ))

            position += len(part)

        return tokens


class PBMEncoder:
    """Encode tokens into Pair-Bond Maps."""

    def __init__(self):
        pass

    def encode_scope(
        self,
        tokens: List[Token],
        scope_id: str,
        scope_type: str = 'document',
        metadata: Optional[Dict] = None
    ) -> PBM:
        """
        Create PBM from token sequence.

        Args:
            tokens: List of Token objects
            scope_id: Unique identifier for this scope
            scope_type: Type of scope (sentence, paragraph, document)
            metadata: Optional metadata dict

        Returns:
            PBM object
        """
        # Count forward pair bonds
        bond_counts = Counter()

        for i in range(len(tokens) - 1):
            token_0 = tokens[i].string
            token_1 = tokens[i + 1].string
            bond_counts[(token_0, token_1)] += 1

        # Create FPB list
        bonds = [
            ForwardPairBond(token_0=t0, token_1=t1, fbr=count)
            for (t0, t1), count in bond_counts.items()
        ]

        return PBM(
            scope_id=scope_id,
            scope_type=scope_type,
            tokens=tokens,
            bonds=bonds,
            metadata=metadata or {}
        )

    def encode_hierarchical(
        self,
        text: str,
        doc_id: str,
        language: str = 'en'
    ) -> Dict[str, List[PBM]]:
        """
        Encode text at multiple scope levels.

        Args:
            text: Input text
            doc_id: Document identifier
            language: Language code

        Returns:
            Dict with 'document', 'paragraph', 'sentence' level PBMs
        """
        tokenizer = Tokenizer(language=language)

        result = {
            'document': [],
            'paragraph': [],
            'sentence': []
        }

        # Document-level
        doc_tokens = tokenizer.tokenize(text)
        result['document'].append(
            self.encode_scope(
                doc_tokens,
                scope_id=f"{doc_id}_doc",
                scope_type='document',
                metadata={'language': language}
            )
        )

        # Paragraph-level
        paragraphs = text.split('\n\n')
        for p_idx, para in enumerate(paragraphs):
            if para.strip():
                para_tokens = tokenizer.tokenize(para)
                result['paragraph'].append(
                    self.encode_scope(
                        para_tokens,
                        scope_id=f"{doc_id}_p{p_idx}",
                        scope_type='paragraph',
                        metadata={'language': language, 'para_num': p_idx}
                    )
                )

        # Sentence-level (basic - split on . ! ?)
        sentences = re.split(r'([.!?])\s+', text)
        sent_idx = 0
        for i in range(0, len(sentences) - 1, 2):
            sent_text = sentences[i] + sentences[i + 1]
            if sent_text.strip():
                sent_tokens = tokenizer.tokenize(sent_text)
                result['sentence'].append(
                    self.encode_scope(
                        sent_tokens,
                        scope_id=f"{doc_id}_s{sent_idx}",
                        scope_type='sentence',
                        metadata={'language': language, 'sent_num': sent_idx}
                    )
                )
                sent_idx += 1

        return result


class GutenbergPipeline:
    """Full pipeline: Gutenberg text → PBMs with metadata."""

    def __init__(self, output_dir: Path):
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        self.encoder = PBMEncoder()

    def process_file(
        self,
        text_file: Path,
        gutenberg_id: int,
        metadata: Dict
    ) -> Dict[str, List[PBM]]:
        """
        Process a Gutenberg text file into PBMs.

        Args:
            text_file: Path to text file
            gutenberg_id: Gutenberg book ID
            metadata: Book metadata from Gutendex

        Returns:
            Dict of PBMs by scope level
        """
        # Read text
        text = text_file.read_text(encoding='utf-8')

        # Strip Gutenberg header/footer
        text = self._strip_gutenberg_boilerplate(text)

        # Determine language
        language = metadata.get('languages', ['en'])[0]

        # Encode at all scope levels
        doc_id = f"pg{gutenberg_id}"
        pbms = self.encoder.encode_hierarchical(text, doc_id, language)

        # Save PBMs
        self._save_pbms(pbms, gutenberg_id, metadata)

        return pbms

    def _strip_gutenberg_boilerplate(self, text: str) -> str:
        """
        Remove Gutenberg header and footer.

        Gutenberg texts have standard headers/footers like:
        *** START OF THE PROJECT GUTENBERG EBOOK ... ***
        *** END OF THE PROJECT GUTENBERG EBOOK ... ***
        """
        # Find start marker
        start_pattern = r'\*\*\* START OF (?:THE|THIS) PROJECT GUTENBERG EBOOK.*?\*\*\*'
        match = re.search(start_pattern, text, re.IGNORECASE)
        if match:
            text = text[match.end():]

        # Find end marker
        end_pattern = r'\*\*\* END OF (?:THE|THIS) PROJECT GUTENBERG EBOOK.*?\*\*\*'
        match = re.search(end_pattern, text, re.IGNORECASE)
        if match:
            text = text[:match.start()]

        return text.strip()

    def _save_pbms(self, pbms: Dict[str, List[PBM]], gutenberg_id: int, metadata: Dict):
        """Save PBMs to JSON files."""
        output_file = self.output_dir / f"pg{gutenberg_id}_pbms.json"

        # Convert to serializable format
        data = {
            'gutenberg_id': gutenberg_id,
            'metadata': metadata,
            'pbms': {}
        }

        for scope_type, pbm_list in pbms.items():
            data['pbms'][scope_type] = [
                {
                    'scope_id': pbm.scope_id,
                    'scope_type': pbm.scope_type,
                    'token_count': len(pbm.tokens),
                    'bond_count': len(pbm.bonds),
                    'tokens': [
                        {'string': t.string, 'type': t.type, 'pos': t.position}
                        for t in pbm.tokens
                    ],
                    'bonds': [
                        {'token_0': b.token_0, 'token_1': b.token_1, 'fbr': b.fbr}
                        for b in pbm.bonds
                    ],
                    'metadata': pbm.metadata
                }
                for pbm in pbm_list
            ]

        output_file.write_text(json.dumps(data, indent=2))
        print(f"Saved PBMs to {output_file}")


if __name__ == "__main__":
    # Example usage
    from gutenberg_fetch import GutendexFetcher, fetch_english_fiction_sample

    # Fetch a few books
    print("Fetching sample books from Gutenberg...")
    books = fetch_english_fiction_sample(max_books=3)

    # Download texts
    fetcher = GutendexFetcher()
    text_dir = Path("../../data/gutenberg/texts")
    metadata_file = Path("../../data/gutenberg/metadata.json")

    downloaded = fetcher.download_collection(books, text_dir, metadata_file)

    # Encode to PBMs
    print("\nEncoding to PBMs...")
    pipeline = GutenbergPipeline(output_dir=Path("../../data/gutenberg/pbms"))

    for i, (book, text_file) in enumerate(zip(books, downloaded)):
        print(f"\nProcessing {book.title}...")
        pbms = pipeline.process_file(
            text_file=text_file,
            gutenberg_id=book.id,
            metadata={
                'title': book.title,
                'authors': book.author_names,
                'languages': book.languages,
                'subjects': book.subjects
            }
        )

        # Show stats
        for scope_type, pbm_list in pbms.items():
            total_bonds = sum(len(pbm.bonds) for pbm in pbm_list)
            print(f"  {scope_type}: {len(pbm_list)} scopes, {total_bonds} total bonds")

    print("\nDone!")
