"""
Scanner: Plain text → raw token stream.

Stage 1 of the PBM text encoder pipeline. Reads input text character-by-character
and produces a stream of RawToken objects for the resolver.

Handles: words, contractions, possessives, hyphens, numbers, punctuation,
Gutenberg underscore italics, smart quotes, em dashes, ellipses.
"""

from dataclasses import dataclass
from enum import Enum


class TokenType(Enum):
    WORD = 'word'
    PUNCTUATION = 'punctuation'
    NUMBER = 'number'
    ITALIC_START = 'italic_start'
    ITALIC_END = 'italic_end'


@dataclass
class RawToken:
    text: str
    type: TokenType
    is_capitalized: bool
    line_number: int
    char_offset: int


class Scanner:
    """Tokenize plain text into raw tokens."""

    # Characters that are always standalone punctuation
    SIMPLE_PUNCT = set('.,;:!?()[]{}')

    # Smart quotes and other Unicode punctuation treated as standalone
    UNICODE_PUNCT = {
        '\u201c',  # " LEFT DOUBLE QUOTATION MARK
        '\u201d',  # " RIGHT DOUBLE QUOTATION MARK
        '\u2018',  # ' LEFT SINGLE QUOTATION MARK
        '\u2019',  # ' RIGHT SINGLE QUOTATION MARK (also apostrophe)
        '\u2014',  # — EM DASH
        '\u2013',  # – EN DASH
        '\u2026',  # … ELLIPSIS
    }

    def scan(self, text: str) -> list[RawToken]:
        """Tokenize text into raw tokens.

        Args:
            text: Input text (should already have boilerplate stripped).

        Returns:
            List of RawToken objects.
        """
        tokens = []
        i = 0
        line_number = 1
        line_start = 0  # char offset of current line start

        while i < len(text):
            char = text[i]

            # Track line numbers
            if char == '\n':
                line_number += 1
                line_start = i + 1
                i += 1
                continue

            # Skip whitespace (space, tab, BOM, etc.)
            if char in ' \t\r\ufeff':
                i += 1
                continue

            offset = i - line_start

            # Underscore — check for Gutenberg italic markers
            if char == '_':
                token = self._handle_underscore(text, i, line_number, offset)
                tokens.append(token)
                i += 1
                continue

            # Smart quotes — standalone punctuation
            # BUT: RIGHT SINGLE QUOTATION MARK (') doubles as apostrophe
            if char == '\u2019':
                # Check if this is an apostrophe (within a word)
                if self._is_apostrophe_context(text, i):
                    # It's an apostrophe — include in word
                    word, end = self._scan_word(text, i, include_leading_apostrophe=False)
                    # Actually, the apostrophe is mid-word; scan_word from before it
                    # The previous token should have captured the first part...
                    # Simpler: if preceded by a letter and followed by a letter,
                    # let the word scanner handle it by backing up
                    pass  # Fall through to word handling below
                else:
                    tokens.append(RawToken(char, TokenType.PUNCTUATION, False,
                                           line_number, offset))
                    i += 1
                    continue

            if char in self.UNICODE_PUNCT and char != '\u2019':
                tokens.append(RawToken(char, TokenType.PUNCTUATION, False,
                                       line_number, offset))
                i += 1
                continue

            # Simple punctuation
            if char in self.SIMPLE_PUNCT:
                # Check for ellipsis: ...
                if char == '.' and i + 2 < len(text) and text[i+1] == '.' and text[i+2] == '.':
                    tokens.append(RawToken('...', TokenType.PUNCTUATION, False,
                                           line_number, offset))
                    i += 3
                    continue
                tokens.append(RawToken(char, TokenType.PUNCTUATION, False,
                                       line_number, offset))
                i += 1
                continue

            # Double hyphen → em dash equivalent
            if char == '-' and i + 1 < len(text) and text[i+1] == '-':
                tokens.append(RawToken('--', TokenType.PUNCTUATION, False,
                                       line_number, offset))
                i += 2
                continue

            # Standalone hyphen (not between letters)
            if char == '-' and not self._is_intraword_hyphen(text, i):
                tokens.append(RawToken('-', TokenType.PUNCTUATION, False,
                                       line_number, offset))
                i += 1
                continue

            # ASCII quote marks
            if char == '"':
                tokens.append(RawToken('"', TokenType.PUNCTUATION, False,
                                       line_number, offset))
                i += 1
                continue

            # ASCII apostrophe / single quote
            if char == "'":
                if self._is_apostrophe_context(text, i):
                    # Part of a word — fall through to word scanning
                    pass
                else:
                    tokens.append(RawToken("'", TokenType.PUNCTUATION, False,
                                           line_number, offset))
                    i += 1
                    continue

            # Word or number starting with a letter or digit
            if char.isalpha() or char == '\u2019' or char == "'":
                word, end = self._scan_word(text, i)
                is_cap = word[0].isupper() if word[0].isalpha() else False
                tokens.append(RawToken(word, TokenType.WORD, is_cap,
                                       line_number, offset))
                i = end
                continue

            if char.isdigit():
                num_word, end = self._scan_number_or_word(text, i)
                # If it's purely numeric, it's a NUMBER; if mixed, it's a WORD
                if num_word.isdigit():
                    tokens.append(RawToken(num_word, TokenType.NUMBER, False,
                                           line_number, offset))
                else:
                    tokens.append(RawToken(num_word, TokenType.WORD, False,
                                           line_number, offset))
                i = end
                continue

            # Intra-word hyphen (between letters) — handled by word scanner
            if char == '-':
                word, end = self._scan_word(text, i)
                is_cap = word[0].isupper() if word[0].isalpha() else False
                tokens.append(RawToken(word, TokenType.WORD, is_cap,
                                       line_number, offset))
                i = end
                continue

            # Any other character — emit as punctuation
            if not char.isspace():
                tokens.append(RawToken(char, TokenType.PUNCTUATION, False,
                                       line_number, offset))
            i += 1

        return tokens

    def _scan_word(self, text: str, start: int,
                   include_leading_apostrophe: bool = True) -> tuple[str, int]:
        """Scan a word token starting at position start.

        Words include:
        - Alphabetic characters
        - Internal apostrophes followed by letters (contractions: don't, it's)
        - Internal hyphens between letters (compounds: well-known)

        Returns (word_string, end_position).
        """
        i = start
        length = len(text)

        # Handle leading apostrophe for words like 'twas, 'tis
        if i < length and text[i] in ("'", '\u2019') and include_leading_apostrophe:
            if i + 1 < length and text[i + 1].isalpha():
                i += 1  # Skip the apostrophe, include it in the word
            else:
                return (text[i], i + 1)

        while i < length:
            char = text[i]

            if char.isalpha():
                i += 1
                continue

            # Apostrophe (ASCII or smart) followed by letter → contraction
            if char in ("'", '\u2019'):
                if i + 1 < length and text[i + 1].isalpha():
                    i += 1  # Include apostrophe
                    continue
                else:
                    # Trailing apostrophe (possessive like heroes')
                    # Include it — resolver will handle
                    break

            # Hyphen between letters → compound word
            if char == '-':
                if (i > start and text[i - 1].isalpha() and
                        i + 1 < length and text[i + 1].isalpha()):
                    i += 1  # Include hyphen
                    continue
                else:
                    break

            # Anything else ends the word
            break

        word = text[start:i]

        # Handle leading apostrophe inclusion
        if start > 0 and text[start] in ("'", '\u2019'):
            pass  # Already included

        return (word, i)

    def _scan_number_or_word(self, text: str, start: int) -> tuple[str, int]:
        """Scan a number that might have a suffix (11th, 1st, 17—).

        Returns (token_string, end_position).
        """
        i = start
        length = len(text)

        # Scan digits
        while i < length and text[i].isdigit():
            i += 1

        # Check for alphabetic suffix (11th, 1st, 2nd, 3rd)
        suffix_start = i
        while i < length and text[i].isalpha():
            i += 1

        # If suffix is short (1-4 chars), include it as part of the number-word
        if i - suffix_start <= 4 and i - suffix_start > 0:
            return (text[start:i], i)

        # Otherwise, just return the digits
        return (text[start:suffix_start], suffix_start)

    def _handle_underscore(self, text: str, pos: int,
                           line_number: int, offset: int) -> RawToken:
        """Handle underscore as either italic marker or literal punctuation.

        Gutenberg convention: _text_ means italic.
        - _ preceded by whitespace/line-start AND followed by non-whitespace → italic_start
        - _ preceded by non-whitespace AND followed by whitespace/line-end/punct → italic_end
        """
        before_is_boundary = (pos == 0 or text[pos - 1] in ' \t\n\r')
        after_is_boundary = (pos + 1 >= len(text) or
                             text[pos + 1] in ' \t\n\r' or
                             text[pos + 1] in self.SIMPLE_PUNCT or
                             text[pos + 1] in self.UNICODE_PUNCT)

        if before_is_boundary and not after_is_boundary:
            return RawToken('_', TokenType.ITALIC_START, False,
                            line_number, offset)
        elif not before_is_boundary and after_is_boundary:
            return RawToken('_', TokenType.ITALIC_END, False,
                            line_number, offset)
        else:
            # Ambiguous or standalone — treat as literal
            return RawToken('_', TokenType.PUNCTUATION, False,
                            line_number, offset)

    def _is_apostrophe_context(self, text: str, pos: int) -> bool:
        """Check if apostrophe at pos is within a word (contraction/possessive).

        True if preceded by a letter AND followed by a letter,
        OR if preceded by a letter (trailing possessive like heroes').
        """
        if pos > 0 and text[pos - 1].isalpha():
            if pos + 1 < len(text) and text[pos + 1].isalpha():
                return True  # Mid-word: don't, it's
        # Also handle leading apostrophe: 'twas, 'tis
        if pos + 1 < len(text) and text[pos + 1].isalpha():
            if pos == 0 or text[pos - 1] in ' \t\n\r':
                return True
        return False

    def _is_intraword_hyphen(self, text: str, pos: int) -> bool:
        """Check if hyphen at pos is between letters (compound word)."""
        return (pos > 0 and text[pos - 1].isalpha() and
                pos + 1 < len(text) and text[pos + 1].isalpha())
