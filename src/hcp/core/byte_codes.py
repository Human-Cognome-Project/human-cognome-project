"""Byte code definitions and classification for all 256 values.

Each byte code gets a token in the core database under the encoding
tables scope. Classification determines bonding behaviour.
"""

from dataclasses import dataclass
from enum import Enum


class ByteCategory(Enum):
    CONTROL = "control"
    WHITESPACE = "whitespace"
    LETTER_UPPER = "letter_upper"
    LETTER_LOWER = "letter_lower"
    DIGIT = "digit"
    PUNCTUATION = "punctuation"
    UTF8_LEAD2 = "utf8_lead2"
    UTF8_LEAD3 = "utf8_lead3"
    UTF8_LEAD4 = "utf8_lead4"
    UTF8_CONT = "utf8_cont"
    INVALID = "invalid"


class BondClass(Enum):
    ALPHA = "alpha"
    NUMERIC = "numeric"
    SEPARATOR = "separator"
    DELIMITER = "delimiter"
    COVALENT = "covalent"
    INERT = "inert"
    UNSTABLE = "unstable"


@dataclass(frozen=True)
class ByteCode:
    value: int
    hex: str
    category: ByteCategory
    bond_class: BondClass
    display: str
    name: str
    ascii_char: str | None


# Control character names
_CONTROL_NAMES = {
    0x00: "NULL", 0x01: "START OF HEADING", 0x02: "START OF TEXT",
    0x03: "END OF TEXT", 0x04: "END OF TRANSMISSION", 0x05: "ENQUIRY",
    0x06: "ACKNOWLEDGE", 0x07: "BELL", 0x08: "BACKSPACE",
    0x09: "CHARACTER TABULATION", 0x0A: "LINE FEED",
    0x0B: "LINE TABULATION", 0x0C: "FORM FEED",
    0x0D: "CARRIAGE RETURN", 0x0E: "SHIFT OUT", 0x0F: "SHIFT IN",
    0x10: "DATA LINK ESCAPE", 0x11: "DEVICE CONTROL ONE",
    0x12: "DEVICE CONTROL TWO", 0x13: "DEVICE CONTROL THREE",
    0x14: "DEVICE CONTROL FOUR", 0x15: "NEGATIVE ACKNOWLEDGE",
    0x16: "SYNCHRONOUS IDLE", 0x17: "END OF TRANSMISSION BLOCK",
    0x18: "CANCEL", 0x19: "END OF MEDIUM", 0x1A: "SUBSTITUTE",
    0x1B: "ESCAPE", 0x1C: "INFORMATION SEPARATOR FOUR",
    0x1D: "INFORMATION SEPARATOR THREE",
    0x1E: "INFORMATION SEPARATOR TWO",
    0x1F: "INFORMATION SEPARATOR ONE",
    0x7F: "DELETE",
}

_PUNCT_NAMES = {
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
    0x5F: "LOW LINE", 0x60: "GRAVE ACCENT",
    0x7B: "LEFT CURLY BRACKET", 0x7C: "VERTICAL LINE",
    0x7D: "RIGHT CURLY BRACKET", 0x7E: "TILDE",
}


def classify_byte(value: int) -> ByteCode:
    """Classify a single byte value."""
    h = f"0x{value:02X}"

    if value in (0x09, 0x0A, 0x0D, 0x20):
        names = {0x09: "CHARACTER TABULATION", 0x0A: "LINE FEED",
                 0x0D: "CARRIAGE RETURN", 0x20: "SPACE"}
        return ByteCode(value, h, ByteCategory.WHITESPACE, BondClass.SEPARATOR,
                        repr(chr(value)), names[value],
                        chr(value) if value == 0x20 else None)

    if value in _CONTROL_NAMES:
        return ByteCode(value, h, ByteCategory.CONTROL, BondClass.INERT,
                        f"<{_CONTROL_NAMES[value]}>", _CONTROL_NAMES[value], None)

    if 0x21 <= value <= 0x7E:
        char = chr(value)
        if 0x41 <= value <= 0x5A:
            return ByteCode(value, h, ByteCategory.LETTER_UPPER, BondClass.ALPHA,
                            char, f"LATIN CAPITAL LETTER {char}", char)
        if 0x61 <= value <= 0x7A:
            return ByteCode(value, h, ByteCategory.LETTER_LOWER, BondClass.ALPHA,
                            char, f"LATIN SMALL LETTER {char}", char)
        if 0x30 <= value <= 0x39:
            return ByteCode(value, h, ByteCategory.DIGIT, BondClass.NUMERIC,
                            char, f"DIGIT {char}", char)
        name = _PUNCT_NAMES.get(value, f"PUNCTUATION {char}")
        return ByteCode(value, h, ByteCategory.PUNCTUATION, BondClass.DELIMITER,
                        char, name, char)

    if 0x80 <= value <= 0xBF:
        return ByteCode(value, h, ByteCategory.UTF8_CONT, BondClass.COVALENT,
                        f"<CONT {value - 0x80:02d}>",
                        f"UTF8 CONTINUATION {value - 0x80}", None)
    if 0xC0 <= value <= 0xDF:
        return ByteCode(value, h, ByteCategory.UTF8_LEAD2, BondClass.COVALENT,
                        f"<LEAD2 {value - 0xC0:02d}>",
                        f"UTF8 2-BYTE LEAD {value - 0xC0}", None)
    if 0xE0 <= value <= 0xEF:
        return ByteCode(value, h, ByteCategory.UTF8_LEAD3, BondClass.COVALENT,
                        f"<LEAD3 {value - 0xE0:02d}>",
                        f"UTF8 3-BYTE LEAD {value - 0xE0}", None)
    if 0xF0 <= value <= 0xF7:
        return ByteCode(value, h, ByteCategory.UTF8_LEAD4, BondClass.COVALENT,
                        f"<LEAD4 {value - 0xF0:02d}>",
                        f"UTF8 4-BYTE LEAD {value - 0xF0}", None)

    return ByteCode(value, h, ByteCategory.INVALID, BondClass.UNSTABLE,
                    f"<INVALID {value:02X}>", f"INVALID BYTE 0x{value:02X}", None)


# Build the complete table
BYTE_TABLE = [classify_byte(v) for v in range(256)]
