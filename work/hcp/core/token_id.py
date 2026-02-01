"""
Token ID system using hierarchical dotted notation with Base-20 encoding.

Address scheme (LoD-dependent depth):
    00.00.00.00.{value}    Byte codes (256 values)
    00.00.00.01.{value}    NSM primitives (65 values)

Higher LoD layers will define their own addressing depth as needed.
The address structure itself encodes what level of detail you're at.

Base-20 uses characters: 0-9, A-J (case insensitive)
"""
from __future__ import annotations

from dataclasses import dataclass
from typing import ClassVar

# Base-20 alphabet: 0-9 followed by A-J
BASE20_CHARS = "0123456789ABCDEFGHIJ"
BASE20_DECODE = {c: i for i, c in enumerate(BASE20_CHARS)}
BASE20_DECODE.update({c.lower(): i for i, c in enumerate(BASE20_CHARS) if c.isalpha()})


def encode_base20(value: int, min_length: int = 1) -> str:
    """Encode an integer as a base-20 string."""
    if value < 0:
        raise ValueError("Cannot encode negative values")
    if value == 0:
        return BASE20_CHARS[0] * min_length

    chars = []
    while value:
        chars.append(BASE20_CHARS[value % 20])
        value //= 20

    result = "".join(reversed(chars))
    if len(result) < min_length:
        result = BASE20_CHARS[0] * (min_length - len(result)) + result
    return result


def decode_base20(encoded: str) -> int:
    """Decode a base-20 string to an integer."""
    result = 0
    for char in encoded:
        if char not in BASE20_DECODE:
            raise ValueError(f"Invalid base-20 character: {char}")
        result = result * 20 + BASE20_DECODE[char]
    return result


@dataclass(frozen=True, slots=True)
class TokenID:
    """
    Immutable Token ID with hierarchical dotted address.

    Each TokenID is a tuple of base-20 encoded segments.
    The prefix (all but last) identifies the category/LoD layer.
    The final segment is the value within that category.

    Base addresses:
        00.00.00.00.{v}    Byte codes
        00.00.00.01.{v}    NSM primitives
    """
    segments: tuple[int, ...]

    # Well-known prefixes
    PREFIX_BYTE: ClassVar[tuple[int, ...]] = (0, 0, 0, 0)
    PREFIX_NSM: ClassVar[tuple[int, ...]] = (0, 0, 0, 1)
    PREFIX_GLYPH: ClassVar[tuple[int, ...]] = (0, 0, 0, 2)  # Placeholder
    PREFIX_WORD: ClassVar[tuple[int, ...]] = (0, 0, 0, 3)   # Placeholder

    def __post_init__(self) -> None:
        if not self.segments:
            raise ValueError("TokenID must have at least one segment")
        for s in self.segments:
            if s < 0:
                raise ValueError(f"Segment values must be non-negative, got {s}")

    @property
    def prefix(self) -> tuple[int, ...]:
        """The prefix (all segments except the last)."""
        return self.segments[:-1]

    @property
    def value(self) -> int:
        """The final segment (the value)."""
        return self.segments[-1]

    @property
    def depth(self) -> int:
        """Address depth (number of segments)."""
        return len(self.segments)

    def is_byte(self) -> bool:
        """Check if this is a byte code token (00.00.00.00.*)."""
        return self.prefix == self.PREFIX_BYTE

    def is_nsm(self) -> bool:
        """Check if this is an NSM primitive token (00.00.00.01.*)."""
        return self.prefix == self.PREFIX_NSM

    def is_glyph(self) -> bool:
        """Check if this is a glyph token (00.00.00.02.*)."""
        return self.prefix == self.PREFIX_GLYPH

    def is_word(self) -> bool:
        """Check if this is a word token (00.00.00.03.*)."""
        return self.prefix == self.PREFIX_WORD

    def has_prefix(self, prefix: tuple[int, ...]) -> bool:
        """Check if this token starts with the given prefix."""
        return self.segments[:len(prefix)] == prefix

    @classmethod
    def from_string(cls, token_str: str) -> TokenID:
        """Parse a dotted token ID string like '00.00.00.00.0C'."""
        parts = token_str.split(".")
        if not parts:
            raise ValueError("Empty token ID string")
        segments = tuple(decode_base20(p) for p in parts)
        return cls(segments=segments)

    def to_string(self, min_segment_length: int = 2) -> str:
        """Convert to dotted string format like '00.00.00.00.0C'."""
        return ".".join(
            encode_base20(s, min_length=min_segment_length)
            for s in self.segments
        )

    def __str__(self) -> str:
        return self.to_string()

    def __repr__(self) -> str:
        return f"TokenID({self.to_string()})"

    @classmethod
    def byte(cls, byte_value: int) -> TokenID:
        """Create a byte code token: 00.00.00.00.{value}"""
        if byte_value < 0 or byte_value > 255:
            raise ValueError(f"Byte value must be 0-255, got {byte_value}")
        return cls(segments=cls.PREFIX_BYTE + (byte_value,))

    @classmethod
    def nsm(cls, primitive_index: int) -> TokenID:
        """Create an NSM primitive token: 00.00.00.01.{value}"""
        return cls(segments=cls.PREFIX_NSM + (primitive_index,))

    @classmethod
    def glyph(cls, codepoint: int) -> TokenID:
        """Create a glyph token: 00.00.00.02.{codepoint}"""
        return cls(segments=cls.PREFIX_GLYPH + (codepoint,))

    @classmethod
    def word(cls, index: int) -> TokenID:
        """Create a word token: 00.00.00.03.{index}"""
        return cls(segments=cls.PREFIX_WORD + (index,))

    @classmethod
    def from_prefix(cls, prefix: tuple[int, ...], value: int) -> TokenID:
        """Create a token with arbitrary prefix and value."""
        return cls(segments=prefix + (value,))

    # Backward compatibility helpers for code that used .mode
    @property
    def mode(self) -> int:
        """Legacy: return prefix category (0=byte, 1=nsm)."""
        if self.is_byte():
            return 0
        if self.is_nsm():
            return 1
        return -1


# Convenience functions
def byte_token(b: int) -> TokenID:
    """Create a token for a raw byte value (0-255)."""
    return TokenID.byte(b)


def nsm_token(index: int) -> TokenID:
    """Create a token for an NSM primitive."""
    return TokenID.nsm(index)


def char_token(char: str) -> TokenID:
    """Create a byte token for a single ASCII character."""
    if len(char) != 1:
        raise ValueError(f"Expected single character, got {len(char)}")
    b = ord(char)
    if b > 255:
        raise ValueError(f"Character {char!r} is not a single byte")
    return TokenID.byte(b)
