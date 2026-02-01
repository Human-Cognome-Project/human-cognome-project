"""
Covalent bonding tables: define bond strengths and affinities between byte classes.

These tables encode structural knowledge about how bytes/characters typically bond.
"""
from __future__ import annotations

from dataclasses import dataclass
from enum import IntEnum
from typing import Callable


class BondStrength(IntEnum):
    """Predefined bond strength levels."""
    NONE = 0       # No natural bond
    WEAK = 1       # Can bond but rarely
    MODERATE = 2   # Common bonding
    STRONG = 3     # Frequent bonding
    COVALENT = 4   # Essentially inseparable (e.g., UTF-8 continuations)


@dataclass(frozen=True)
class ByteClass:
    """Classification of a byte value."""
    name: str
    test: Callable[[int], bool]

    def matches(self, value: int) -> bool:
        return self.test(value)


# Define byte classes
BYTE_CLASSES = {
    "uppercase": ByteClass("uppercase", lambda b: 65 <= b <= 90),
    "lowercase": ByteClass("lowercase", lambda b: 97 <= b <= 122),
    "digit": ByteClass("digit", lambda b: 48 <= b <= 57),
    "space": ByteClass("space", lambda b: b == 32),
    "newline": ByteClass("newline", lambda b: b in (10, 13)),
    "tab": ByteClass("tab", lambda b: b == 9),
    "punctuation": ByteClass("punctuation", lambda b: b < 128 and not (
        65 <= b <= 90 or 97 <= b <= 122 or 48 <= b <= 57 or b in (9, 10, 13, 32)
    ) and b >= 32),
    "control": ByteClass("control", lambda b: b < 32 and b not in (9, 10, 13)),
    "utf8_lead2": ByteClass("utf8_lead2", lambda b: 192 <= b <= 223),  # 2-byte lead
    "utf8_lead3": ByteClass("utf8_lead3", lambda b: 224 <= b <= 239),  # 3-byte lead
    "utf8_lead4": ByteClass("utf8_lead4", lambda b: 240 <= b <= 247),  # 4-byte lead
    "utf8_cont": ByteClass("utf8_cont", lambda b: 128 <= b <= 191),    # Continuation
    "high_byte": ByteClass("high_byte", lambda b: b >= 248),           # Invalid UTF-8
}


def classify_byte(value: int) -> str:
    """Get the primary class for a byte value."""
    for name, cls in BYTE_CLASSES.items():
        if cls.matches(value):
            return name
    return "unknown"


class CovalentTable:
    """
    Bond affinity table between byte classes.

    Higher values indicate stronger natural bonds.
    """

    def __init__(self) -> None:
        # Default: moderate affinity within same class, weak between different
        self._affinities: dict[tuple[str, str], BondStrength] = {}
        self._setup_defaults()

    def _setup_defaults(self) -> None:
        """Setup default affinities based on language structure."""
        # Letters strongly bond with each other (word formation)
        for c1 in ("uppercase", "lowercase"):
            for c2 in ("uppercase", "lowercase"):
                self._affinities[(c1, c2)] = BondStrength.STRONG

        # Digits bond with each other
        self._affinities[("digit", "digit")] = BondStrength.STRONG

        # Letters and digits can bond (identifiers, alphanumeric)
        for c in ("uppercase", "lowercase"):
            self._affinities[(c, "digit")] = BondStrength.MODERATE
            self._affinities[("digit", c)] = BondStrength.MODERATE

        # Space weakly bonds (word separator)
        self._affinities[("space", "uppercase")] = BondStrength.WEAK
        self._affinities[("space", "lowercase")] = BondStrength.WEAK
        self._affinities[("uppercase", "space")] = BondStrength.WEAK
        self._affinities[("lowercase", "space")] = BondStrength.WEAK

        # UTF-8 multi-byte: covalent bonds (must stay together)
        for lead in ("utf8_lead2", "utf8_lead3", "utf8_lead4"):
            self._affinities[(lead, "utf8_cont")] = BondStrength.COVALENT
        self._affinities[("utf8_cont", "utf8_cont")] = BondStrength.COVALENT

        # Punctuation can follow words
        for c in ("uppercase", "lowercase"):
            self._affinities[(c, "punctuation")] = BondStrength.MODERATE
            self._affinities[("punctuation", c)] = BondStrength.WEAK

    def get_affinity(self, class1: str, class2: str) -> BondStrength:
        """Get bond affinity between two byte classes."""
        return self._affinities.get((class1, class2), BondStrength.NONE)

    def bond_strength(self, byte1: int, byte2: int) -> BondStrength:
        """Calculate bond strength between two specific bytes."""
        class1 = classify_byte(byte1)
        class2 = classify_byte(byte2)
        return self.get_affinity(class1, class2)

    def set_affinity(self, class1: str, class2: str, strength: BondStrength) -> None:
        """Set affinity between two byte classes."""
        self._affinities[(class1, class2)] = strength


# Global default table
DEFAULT_TABLE = CovalentTable()


class UTF8Validator:
    """Validate and analyze UTF-8 byte sequences."""

    @staticmethod
    def expected_continuation_count(lead_byte: int) -> int:
        """Return expected continuation bytes after a lead byte."""
        if lead_byte < 128:
            return 0  # ASCII
        if 192 <= lead_byte <= 223:
            return 1  # 2-byte sequence
        if 224 <= lead_byte <= 239:
            return 2  # 3-byte sequence
        if 240 <= lead_byte <= 247:
            return 3  # 4-byte sequence
        return -1  # Invalid

    @staticmethod
    def is_valid_continuation(byte: int) -> bool:
        """Check if byte is a valid UTF-8 continuation byte."""
        return 128 <= byte <= 191

    def validate_sequence(self, data: bytes) -> list[tuple[int, int, bool]]:
        """
        Validate UTF-8 sequences.

        Returns list of (start, end, is_valid) tuples for each character.
        """
        result = []
        i = 0
        while i < len(data):
            byte = data[i]
            expected = self.expected_continuation_count(byte)

            if expected == 0:
                # ASCII or single byte
                result.append((i, i + 1, byte < 128))
                i += 1
            elif expected > 0:
                # Multi-byte sequence
                end = i + 1 + expected
                if end > len(data):
                    # Not enough bytes
                    result.append((i, len(data), False))
                    break
                # Check continuations
                valid = all(
                    self.is_valid_continuation(data[j])
                    for j in range(i + 1, end)
                )
                result.append((i, end, valid))
                i = end
            else:
                # Invalid lead byte
                result.append((i, i + 1, False))
                i += 1

        return result


class GlyphBoundaryDetector:
    """Detect boundaries between Unicode glyphs."""

    def __init__(self) -> None:
        self._validator = UTF8Validator()

    def find_boundaries(self, data: bytes) -> list[int]:
        """Return byte positions where glyphs begin."""
        boundaries = [0]
        sequences = self._validator.validate_sequence(data)
        for start, end, _ in sequences:
            if end < len(data):
                boundaries.append(end)
        return boundaries

    def split_glyphs(self, data: bytes) -> list[bytes]:
        """Split byte sequence into individual glyphs."""
        sequences = self._validator.validate_sequence(data)
        return [data[start:end] for start, end, _ in sequences]


def bond_strength(byte1: int, byte2: int) -> BondStrength:
    """Convenience function for default table bond strength."""
    return DEFAULT_TABLE.bond_strength(byte1, byte2)


def is_utf8_covalent(byte1: int, byte2: int) -> bool:
    """Check if two bytes form a covalent UTF-8 bond."""
    return DEFAULT_TABLE.bond_strength(byte1, byte2) == BondStrength.COVALENT
