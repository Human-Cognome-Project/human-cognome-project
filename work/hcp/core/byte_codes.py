"""
Complete byte code table: all 256 values under 00.00.00.00.{value}

Each byte code is classified by:
- Category (control, printable, whitespace, utf8_lead, utf8_cont, high)
- Subcategory (letter_upper, letter_lower, digit, punctuation, etc.)
- Covalent bonding class (how it bonds with neighbors)
- Display representation
"""
from __future__ import annotations

from dataclasses import dataclass
from enum import Enum
from typing import ClassVar

from .token_id import TokenID, encode_base20


class ByteCategory(Enum):
    """Primary byte classification."""
    CONTROL = "control"           # 0x00-0x1F (except whitespace), 0x7F
    WHITESPACE = "whitespace"     # 0x09, 0x0A, 0x0D, 0x20
    LETTER_UPPER = "letter_upper" # 0x41-0x5A
    LETTER_LOWER = "letter_lower" # 0x61-0x7A
    DIGIT = "digit"               # 0x30-0x39
    PUNCTUATION = "punctuation"   # Printable non-alphanumeric ASCII
    UTF8_LEAD2 = "utf8_lead2"     # 0xC0-0xDF (2-byte sequence lead)
    UTF8_LEAD3 = "utf8_lead3"     # 0xE0-0xEF (3-byte sequence lead)
    UTF8_LEAD4 = "utf8_lead4"     # 0xF0-0xF7 (4-byte sequence lead)
    UTF8_CONT = "utf8_cont"       # 0x80-0xBF (continuation byte)
    INVALID = "invalid"           # 0xF8-0xFF (invalid UTF-8)


class BondClass(Enum):
    """Covalent bonding classification for pair bond formation."""
    ALPHA = "alpha"         # Letters - strong intra-word bonds
    NUMERIC = "numeric"     # Digits - strong intra-number bonds
    SEPARATOR = "separator" # Whitespace - weak word-boundary bonds
    DELIMITER = "delimiter" # Punctuation - moderate structural bonds
    COVALENT = "covalent"   # UTF-8 multi-byte - inseparable bonds
    INERT = "inert"         # Control chars - no bonding
    UNSTABLE = "unstable"   # Invalid bytes - unpredictable bonding


@dataclass(frozen=True)
class ByteCode:
    """Complete specification of a single byte code."""
    value: int             # 0-255
    hex: str               # "0x00" format
    category: ByteCategory
    bond_class: BondClass
    display: str           # Human-readable display
    name: str              # Descriptive name
    ascii_char: str | None # ASCII character if printable

    @property
    def token_id(self) -> TokenID:
        """Get the TokenID for this byte code."""
        return TokenID.byte(self.value)

    @property
    def address(self) -> str:
        """Get the full dotted address."""
        return self.token_id.to_string()

    def __str__(self) -> str:
        return f"{self.address} ({self.hex}) {self.display}"


def _classify_byte(value: int) -> tuple[ByteCategory, BondClass, str, str, str | None]:
    """Classify a byte value. Returns (category, bond_class, display, name, ascii_char)."""

    # Control characters (0x00-0x1F, 0x7F)
    control_names = {
        0x00: "NUL", 0x01: "SOH", 0x02: "STX", 0x03: "ETX",
        0x04: "EOT", 0x05: "ENQ", 0x06: "ACK", 0x07: "BEL",
        0x08: "BS",  0x09: "HT",  0x0A: "LF",  0x0B: "VT",
        0x0C: "FF",  0x0D: "CR",  0x0E: "SO",  0x0F: "SI",
        0x10: "DLE", 0x11: "DC1", 0x12: "DC2", 0x13: "DC3",
        0x14: "DC4", 0x15: "NAK", 0x16: "SYN", 0x17: "ETB",
        0x18: "CAN", 0x19: "EM",  0x1A: "SUB", 0x1B: "ESC",
        0x1C: "FS",  0x1D: "GS",  0x1E: "RS",  0x1F: "US",
        0x7F: "DEL",
    }

    # Whitespace subset of controls
    if value in (0x09, 0x0A, 0x0D, 0x20):
        ws_names = {0x09: "TAB", 0x0A: "LF", 0x0D: "CR", 0x20: "SPACE"}
        ws_display = {0x09: "\\t", 0x0A: "\\n", 0x0D: "\\r", 0x20: "' '"}
        return (ByteCategory.WHITESPACE, BondClass.SEPARATOR,
                ws_display[value], ws_names[value], chr(value) if value == 0x20 else None)

    # Other control characters
    if value in control_names:
        return (ByteCategory.CONTROL, BondClass.INERT,
                f"<{control_names[value]}>", control_names[value], None)

    # Printable ASCII (0x21-0x7E)
    if 0x21 <= value <= 0x7E:
        char = chr(value)

        # Uppercase letters
        if 0x41 <= value <= 0x5A:
            return (ByteCategory.LETTER_UPPER, BondClass.ALPHA,
                    f"'{char}'", f"LATIN CAPITAL LETTER {char}", char)

        # Lowercase letters
        if 0x61 <= value <= 0x7A:
            return (ByteCategory.LETTER_LOWER, BondClass.ALPHA,
                    f"'{char}'", f"LATIN SMALL LETTER {char}", char)

        # Digits
        if 0x30 <= value <= 0x39:
            return (ByteCategory.DIGIT, BondClass.NUMERIC,
                    f"'{char}'", f"DIGIT {char}", char)

        # Everything else is punctuation
        punct_names = {
            0x21: "EXCLAMATION MARK", 0x22: "QUOTATION MARK",
            0x23: "NUMBER SIGN", 0x24: "DOLLAR SIGN",
            0x25: "PERCENT SIGN", 0x26: "AMPERSAND",
            0x27: "APOSTROPHE", 0x28: "LEFT PARENTHESIS",
            0x29: "RIGHT PARENTHESIS", 0x2A: "ASTERISK",
            0x2B: "PLUS SIGN", 0x2C: "COMMA",
            0x2D: "HYPHEN-MINUS", 0x2E: "FULL STOP",
            0x2F: "SOLIDUS",
            0x3A: "COLON", 0x3B: "SEMICOLON",
            0x3C: "LESS-THAN SIGN", 0x3D: "EQUALS SIGN",
            0x3E: "GREATER-THAN SIGN", 0x3F: "QUESTION MARK",
            0x40: "COMMERCIAL AT",
            0x5B: "LEFT SQUARE BRACKET", 0x5C: "REVERSE SOLIDUS",
            0x5D: "RIGHT SQUARE BRACKET", 0x5E: "CIRCUMFLEX ACCENT",
            0x5F: "LOW LINE",
            0x60: "GRAVE ACCENT",
            0x7B: "LEFT CURLY BRACKET", 0x7C: "VERTICAL LINE",
            0x7D: "RIGHT CURLY BRACKET", 0x7E: "TILDE",
        }
        name = punct_names.get(value, f"PUNCT {char}")
        return (ByteCategory.PUNCTUATION, BondClass.DELIMITER,
                f"'{char}'", name, char)

    # UTF-8 continuation bytes (0x80-0xBF)
    if 0x80 <= value <= 0xBF:
        return (ByteCategory.UTF8_CONT, BondClass.COVALENT,
                f"<CONT {value - 0x80:02d}>",
                f"UTF8 CONTINUATION {value - 0x80}", None)

    # UTF-8 2-byte lead (0xC0-0xDF)
    if 0xC0 <= value <= 0xDF:
        return (ByteCategory.UTF8_LEAD2, BondClass.COVALENT,
                f"<LEAD2 {value - 0xC0:02d}>",
                f"UTF8 2-BYTE LEAD {value - 0xC0}", None)

    # UTF-8 3-byte lead (0xE0-0xEF)
    if 0xE0 <= value <= 0xEF:
        return (ByteCategory.UTF8_LEAD3, BondClass.COVALENT,
                f"<LEAD3 {value - 0xE0:02d}>",
                f"UTF8 3-BYTE LEAD {value - 0xE0}", None)

    # UTF-8 4-byte lead (0xF0-0xF7)
    if 0xF0 <= value <= 0xF7:
        return (ByteCategory.UTF8_LEAD4, BondClass.COVALENT,
                f"<LEAD4 {value - 0xF0:02d}>",
                f"UTF8 4-BYTE LEAD {value - 0xF0}", None)

    # Invalid UTF-8 (0xF8-0xFF)
    return (ByteCategory.INVALID, BondClass.UNSTABLE,
            f"<INVALID {value:02X}>",
            f"INVALID BYTE 0x{value:02X}", None)


def _build_table() -> list[ByteCode]:
    """Build the complete 256-entry byte code table."""
    table = []
    for value in range(256):
        category, bond_class, display, name, ascii_char = _classify_byte(value)
        table.append(ByteCode(
            value=value,
            hex=f"0x{value:02X}",
            category=category,
            bond_class=bond_class,
            display=display,
            name=name,
            ascii_char=ascii_char,
        ))
    return table


# The complete byte code table
BYTE_TABLE: list[ByteCode] = _build_table()

# Lookup by value
BYTE_BY_VALUE: dict[int, ByteCode] = {bc.value: bc for bc in BYTE_TABLE}

# Lookup by category
BYTE_BY_CATEGORY: dict[ByteCategory, list[ByteCode]] = {}
for _bc in BYTE_TABLE:
    BYTE_BY_CATEGORY.setdefault(_bc.category, []).append(_bc)

# Lookup by bond class
BYTE_BY_BOND_CLASS: dict[BondClass, list[ByteCode]] = {}
for _bc in BYTE_TABLE:
    BYTE_BY_BOND_CLASS.setdefault(_bc.bond_class, []).append(_bc)


def get_byte(value: int) -> ByteCode:
    """Get byte code entry by value."""
    return BYTE_BY_VALUE[value]


def get_category(value: int) -> ByteCategory:
    """Get category for a byte value."""
    return BYTE_BY_VALUE[value].category


def get_bond_class(value: int) -> BondClass:
    """Get bond class for a byte value."""
    return BYTE_BY_VALUE[value].bond_class


def get_display(value: int) -> str:
    """Get display representation for a byte value."""
    return BYTE_BY_VALUE[value].display


def bytes_in_category(category: ByteCategory) -> list[ByteCode]:
    """Get all byte codes in a category."""
    return BYTE_BY_CATEGORY.get(category, [])


def bytes_in_bond_class(bond_class: BondClass) -> list[ByteCode]:
    """Get all byte codes in a bond class."""
    return BYTE_BY_BOND_CLASS.get(bond_class, [])


def print_table(category: ByteCategory | None = None) -> None:
    """Print the byte code table (or subset by category)."""
    entries = BYTE_BY_CATEGORY.get(category, BYTE_TABLE) if category else BYTE_TABLE

    print(f"{'Address':<20} {'Hex':<6} {'Cat':<14} {'Bond':<12} {'Display'}")
    print("-" * 70)
    for bc in entries:
        print(f"{bc.address:<20} {bc.hex:<6} {bc.category.value:<14} "
              f"{bc.bond_class.value:<12} {bc.display}")


def print_summary() -> None:
    """Print a summary of byte code categories."""
    print("Byte Code Table Summary")
    print("=" * 50)
    for cat in ByteCategory:
        codes = BYTE_BY_CATEGORY.get(cat, [])
        if codes:
            low = min(c.value for c in codes)
            high = max(c.value for c in codes)
            print(f"  {cat.value:<14} {len(codes):>3} bytes  "
                  f"(0x{low:02X}-0x{high:02X})")
    print()
    for bc in BondClass:
        codes = BYTE_BY_BOND_CLASS.get(bc, [])
        if codes:
            print(f"  {bc.value:<12} {len(codes):>3} bytes")
