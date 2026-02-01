"""
Tokenizer: promote byte sequences to higher-level tokens.

Handles the progression from bytes -> glyphs -> words -> phrases.
"""
from __future__ import annotations

from dataclasses import dataclass, field
from typing import Iterator, Sequence

from ..core.token_id import TokenID
from ..core.pair_bond import PairBondMap
from ..storage.token_store import TokenStore
from .byte_atomizer import ByteAtomizer, ByteSpan, ByteSpanClassifier
from .covalent_tables import (
    BondStrength,
    CovalentTable,
    GlyphBoundaryDetector,
    DEFAULT_TABLE,
)


@dataclass
class TokenSpan:
    """A span of tokens with metadata."""
    tokens: list[TokenID]
    start: int  # Byte position
    end: int    # Byte position
    level: int  # Abstraction level (0=byte, 1=glyph, 2=word, etc.)
    source: bytes  # Original bytes

    @property
    def length(self) -> int:
        return len(self.tokens)

    def as_string(self) -> str | None:
        """Try to decode source as UTF-8."""
        try:
            return self.source.decode("utf-8")
        except UnicodeDecodeError:
            return None


@dataclass
class TokenizerConfig:
    """Configuration for tokenization behavior."""
    promote_glyphs: bool = True      # Combine UTF-8 bytes into glyphs
    promote_words: bool = True       # Combine letter sequences into words
    min_word_length: int = 2         # Minimum length for word promotion
    register_tokens: bool = False    # Auto-register in token store
    covalent_table: CovalentTable = field(default_factory=lambda: DEFAULT_TABLE)


class Tokenizer:
    """
    Multi-level tokenizer that promotes bytes to higher abstractions.

    Process:
    1. Atomize to bytes (level 0)
    2. Combine covalent UTF-8 sequences into glyphs (level 1)
    3. Combine letter sequences into words (level 2)
    4. (Future) Combine words into phrases (level 3+)
    """

    def __init__(
        self,
        config: TokenizerConfig | None = None,
        token_store: TokenStore | None = None,
    ) -> None:
        self.config = config or TokenizerConfig()
        self.token_store = token_store
        self._atomizer = ByteAtomizer()
        self._classifier = ByteSpanClassifier()
        self._glyph_detector = GlyphBoundaryDetector()
        # Cache for word tokens
        self._word_tokens: dict[bytes, TokenID] = {}
        self._next_word_id: int = 0

    def tokenize(self, data: bytes) -> list[TokenID]:
        """Tokenize bytes into a sequence of tokens."""
        # Level 0: Byte tokens
        byte_tokens = self._atomizer.to_tokens(data)

        if not self.config.promote_glyphs:
            return byte_tokens

        # Level 1: Promote to glyphs
        glyph_tokens = self._promote_to_glyphs(data)

        if not self.config.promote_words:
            return glyph_tokens

        # Level 2: Promote to words
        return self._promote_to_words(data, glyph_tokens)

    def tokenize_text(self, text: str) -> list[TokenID]:
        """Tokenize text (UTF-8 encoded)."""
        return self.tokenize(text.encode("utf-8"))

    def _promote_to_glyphs(self, data: bytes) -> list[TokenID]:
        """Combine UTF-8 multi-byte sequences into glyph tokens."""
        glyphs = self._glyph_detector.split_glyphs(data)
        tokens = []

        for glyph in glyphs:
            if len(glyph) == 1:
                # Single byte -> byte token
                tokens.append(TokenID.byte(glyph[0]))
            else:
                # Multi-byte -> glyph token (decode to codepoint)
                try:
                    char = glyph.decode("utf-8")
                    tokens.append(TokenID.glyph(ord(char)))
                except UnicodeDecodeError:
                    # Invalid UTF-8: keep as byte tokens
                    tokens.extend(TokenID.byte(b) for b in glyph)

        return tokens

    def _promote_to_words(
        self,
        data: bytes,
        glyph_tokens: list[TokenID],
    ) -> list[TokenID]:
        """Combine letter sequences into word tokens."""
        spans = self._classifier.span_bytes(data)
        result = []
        token_idx = 0

        for span in spans:
            span_len = self._count_tokens_in_span(span, data)

            if span.span_type == "word" and len(span.atoms) >= self.config.min_word_length:
                # Promote to word token
                word_bytes = span.to_bytes()
                word_token = self._get_or_create_word_token(word_bytes)
                result.append(word_token)
            else:
                # Keep individual tokens
                result.extend(glyph_tokens[token_idx:token_idx + span_len])

            token_idx += span_len

        return result

    def _count_tokens_in_span(self, span: ByteSpan, data: bytes) -> int:
        """Count how many glyph tokens correspond to a byte span."""
        # For ASCII, it's 1:1 with bytes
        # For UTF-8, we need to count actual glyphs
        span_data = data[span.start:span.end]
        glyphs = self._glyph_detector.split_glyphs(span_data)
        return len(glyphs)

    def _get_or_create_word_token(self, word: bytes) -> TokenID:
        """Get existing word token or create new one."""
        if word not in self._word_tokens:
            token = TokenID.word(self._next_word_id)
            self._word_tokens[word] = token
            self._next_word_id += 1

            if self.token_store and self.config.register_tokens:
                self.token_store.register(token, content=word)

        return self._word_tokens[word]

    def get_word(self, token: TokenID) -> bytes | None:
        """Get the word bytes for a word token."""
        for word, t in self._word_tokens.items():
            if t == token:
                return word
        return None

    def get_word_str(self, token: TokenID) -> str | None:
        """Get the word string for a word token."""
        word = self.get_word(token)
        if word:
            try:
                return word.decode("utf-8")
            except UnicodeDecodeError:
                return None
        return None

    def tokenize_to_spans(self, data: bytes) -> list[TokenSpan]:
        """Tokenize and return spans with full metadata."""
        spans = self._classifier.span_bytes(data)
        result = []

        for span in spans:
            span_data = data[span.start:span.end]

            if span.span_type == "word" and len(span.atoms) >= self.config.min_word_length:
                word_token = self._get_or_create_word_token(span_data)
                result.append(TokenSpan(
                    tokens=[word_token],
                    start=span.start,
                    end=span.end,
                    level=2,  # Word level
                    source=span_data,
                ))
            else:
                # Glyph-level tokenization
                tokens = self._promote_to_glyphs(span_data)
                for i, token in enumerate(tokens):
                    # Calculate byte positions per glyph
                    glyphs = self._glyph_detector.split_glyphs(span_data)
                    glyph_start = span.start + sum(len(g) for g in glyphs[:i])
                    glyph_end = glyph_start + len(glyphs[i]) if i < len(glyphs) else span.end

                    level = 1 if token.is_glyph() else 0
                    result.append(TokenSpan(
                        tokens=[token],
                        start=glyph_start,
                        end=glyph_end,
                        level=level,
                        source=span_data[glyph_start - span.start:glyph_end - span.start],
                    ))

        return result

    def to_pbm(self, data: bytes) -> PairBondMap:
        """Tokenize and create a PairBondMap."""
        tokens = self.tokenize(data)
        pbm = PairBondMap()
        pbm.add_sequence(tokens)
        return pbm


def tokenize(text: str, promote_words: bool = True) -> list[TokenID]:
    """Convenience function for quick tokenization."""
    config = TokenizerConfig(promote_words=promote_words)
    tokenizer = Tokenizer(config)
    return tokenizer.tokenize_text(text)


def text_to_pbm(text: str, promote_words: bool = True) -> PairBondMap:
    """Convenience function to create PBM from text."""
    config = TokenizerConfig(promote_words=promote_words)
    tokenizer = Tokenizer(config)
    return tokenizer.to_pbm(text.encode("utf-8"))
