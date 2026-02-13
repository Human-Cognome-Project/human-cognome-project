"""Tests for hcp.core.byte_codes module.

Tests byte code classification and the BYTE_TABLE.

Contributors: Planner & Silas (DI-cognome)
"""
import pytest

from hcp.core.byte_codes import (
    ByteCategory,
    BondClass,
    ByteCode,
    classify_byte,
    BYTE_TABLE,
)


class TestByteTable:
    """Test the BYTE_TABLE constant."""

    def test_table_complete(self):
        """Table should have exactly 256 entries."""
        assert len(BYTE_TABLE) == 256

    def test_table_indexed_by_value(self):
        """Each entry should be at its byte value index."""
        for i, bc in enumerate(BYTE_TABLE):
            assert bc.value == i

    def test_all_entries_are_bytecode(self):
        """All entries should be ByteCode instances."""
        for bc in BYTE_TABLE:
            assert isinstance(bc, ByteCode)


class TestClassifyByte:
    """Test classify_byte function."""

    def test_null_byte(self):
        """Null byte (0x00) should be control/inert."""
        bc = classify_byte(0x00)
        assert bc.value == 0
        assert bc.category == ByteCategory.CONTROL
        assert bc.bond_class == BondClass.INERT
        assert bc.name == "NULL"

    def test_space(self):
        """Space (0x20) should be whitespace/separator."""
        bc = classify_byte(0x20)
        assert bc.value == 0x20
        assert bc.category == ByteCategory.WHITESPACE
        assert bc.bond_class == BondClass.SEPARATOR
        assert bc.ascii_char == " "

    def test_tab(self):
        """Tab (0x09) should be whitespace/separator."""
        bc = classify_byte(0x09)
        assert bc.category == ByteCategory.WHITESPACE
        assert bc.bond_class == BondClass.SEPARATOR

    def test_newline(self):
        """Newline (0x0A) should be whitespace/separator."""
        bc = classify_byte(0x0A)
        assert bc.category == ByteCategory.WHITESPACE
        assert bc.bond_class == BondClass.SEPARATOR
        assert bc.name == "LINE FEED"

    def test_carriage_return(self):
        """Carriage return (0x0D) should be whitespace/separator."""
        bc = classify_byte(0x0D)
        assert bc.category == ByteCategory.WHITESPACE
        assert bc.bond_class == BondClass.SEPARATOR


class TestUppercaseLetters:
    """Test uppercase letter classification."""

    def test_letter_a(self):
        """'A' (0x41) should be uppercase letter."""
        bc = classify_byte(0x41)
        assert bc.category == ByteCategory.LETTER_UPPER
        assert bc.bond_class == BondClass.ALPHA
        assert bc.ascii_char == "A"
        assert bc.name == "LATIN CAPITAL LETTER A"

    def test_letter_z(self):
        """'Z' (0x5A) should be uppercase letter."""
        bc = classify_byte(0x5A)
        assert bc.category == ByteCategory.LETTER_UPPER
        assert bc.bond_class == BondClass.ALPHA
        assert bc.ascii_char == "Z"

    def test_all_uppercase_alpha(self):
        """All A-Z should be LETTER_UPPER with ALPHA bond class."""
        for code in range(0x41, 0x5B):  # A-Z
            bc = classify_byte(code)
            assert bc.category == ByteCategory.LETTER_UPPER
            assert bc.bond_class == BondClass.ALPHA


class TestLowercaseLetters:
    """Test lowercase letter classification."""

    def test_letter_a_lower(self):
        """'a' (0x61) should be lowercase letter."""
        bc = classify_byte(0x61)
        assert bc.category == ByteCategory.LETTER_LOWER
        assert bc.bond_class == BondClass.ALPHA
        assert bc.ascii_char == "a"

    def test_letter_z_lower(self):
        """'z' (0x7A) should be lowercase letter."""
        bc = classify_byte(0x7A)
        assert bc.category == ByteCategory.LETTER_LOWER
        assert bc.bond_class == BondClass.ALPHA
        assert bc.ascii_char == "z"

    def test_all_lowercase_alpha(self):
        """All a-z should be LETTER_LOWER with ALPHA bond class."""
        for code in range(0x61, 0x7B):  # a-z
            bc = classify_byte(code)
            assert bc.category == ByteCategory.LETTER_LOWER
            assert bc.bond_class == BondClass.ALPHA


class TestDigits:
    """Test digit classification."""

    def test_digit_zero(self):
        """'0' (0x30) should be digit."""
        bc = classify_byte(0x30)
        assert bc.category == ByteCategory.DIGIT
        assert bc.bond_class == BondClass.NUMERIC
        assert bc.ascii_char == "0"

    def test_digit_nine(self):
        """'9' (0x39) should be digit."""
        bc = classify_byte(0x39)
        assert bc.category == ByteCategory.DIGIT
        assert bc.bond_class == BondClass.NUMERIC
        assert bc.ascii_char == "9"

    def test_all_digits_numeric(self):
        """All 0-9 should be DIGIT with NUMERIC bond class."""
        for code in range(0x30, 0x3A):  # 0-9
            bc = classify_byte(code)
            assert bc.category == ByteCategory.DIGIT
            assert bc.bond_class == BondClass.NUMERIC


class TestPunctuation:
    """Test punctuation classification."""

    def test_exclamation(self):
        """'!' should be punctuation/delimiter."""
        bc = classify_byte(0x21)
        assert bc.category == ByteCategory.PUNCTUATION
        assert bc.bond_class == BondClass.DELIMITER
        assert bc.ascii_char == "!"

    def test_period(self):
        """'.' should be punctuation/delimiter."""
        bc = classify_byte(0x2E)
        assert bc.category == ByteCategory.PUNCTUATION
        assert bc.bond_class == BondClass.DELIMITER
        assert bc.name == "FULL STOP"

    def test_comma(self):
        """',' should be punctuation/delimiter."""
        bc = classify_byte(0x2C)
        assert bc.category == ByteCategory.PUNCTUATION
        assert bc.bond_class == BondClass.DELIMITER
        assert bc.name == "COMMA"

    def test_question_mark(self):
        """'?' should be punctuation/delimiter."""
        bc = classify_byte(0x3F)
        assert bc.category == ByteCategory.PUNCTUATION
        assert bc.bond_class == BondClass.DELIMITER


class TestUTF8Bytes:
    """Test UTF-8 multi-byte sequence classification."""

    def test_utf8_continuation(self):
        """0x80-0xBF should be UTF8_CONT with COVALENT bond."""
        for code in range(0x80, 0xC0):
            bc = classify_byte(code)
            assert bc.category == ByteCategory.UTF8_CONT
            assert bc.bond_class == BondClass.COVALENT

    def test_utf8_lead2(self):
        """0xC0-0xDF should be UTF8_LEAD2 with COVALENT bond."""
        for code in range(0xC0, 0xE0):
            bc = classify_byte(code)
            assert bc.category == ByteCategory.UTF8_LEAD2
            assert bc.bond_class == BondClass.COVALENT

    def test_utf8_lead3(self):
        """0xE0-0xEF should be UTF8_LEAD3 with COVALENT bond."""
        for code in range(0xE0, 0xF0):
            bc = classify_byte(code)
            assert bc.category == ByteCategory.UTF8_LEAD3
            assert bc.bond_class == BondClass.COVALENT

    def test_utf8_lead4(self):
        """0xF0-0xF7 should be UTF8_LEAD4 with COVALENT bond."""
        for code in range(0xF0, 0xF8):
            bc = classify_byte(code)
            assert bc.category == ByteCategory.UTF8_LEAD4
            assert bc.bond_class == BondClass.COVALENT


class TestInvalidBytes:
    """Test invalid byte classification."""

    def test_invalid_high_bytes(self):
        """0xF8-0xFF should be INVALID with UNSTABLE bond."""
        for code in range(0xF8, 0x100):
            bc = classify_byte(code)
            assert bc.category == ByteCategory.INVALID
            assert bc.bond_class == BondClass.UNSTABLE


class TestByteCodeDataclass:
    """Test ByteCode dataclass properties."""

    def test_frozen(self):
        """ByteCode should be immutable (frozen)."""
        bc = classify_byte(0x41)
        with pytest.raises(Exception):  # FrozenInstanceError
            bc.value = 0x42

    def test_hex_format(self):
        """Hex should be formatted as 0xXX."""
        bc = classify_byte(0x41)
        assert bc.hex == "0x41"
        bc_low = classify_byte(0x0A)
        assert bc_low.hex == "0x0A"


class TestCategoryDistribution:
    """Test overall category distribution in the byte table."""

    def test_category_counts(self):
        """Verify expected category counts."""
        counts = {}
        for bc in BYTE_TABLE:
            cat = bc.category.value
            counts[cat] = counts.get(cat, 0) + 1

        # Expected counts from byte_codes.py
        assert counts.get("letter_upper", 0) == 26
        assert counts.get("letter_lower", 0) == 26
        assert counts.get("digit", 0) == 10
        assert counts.get("whitespace", 0) == 4  # space, tab, LF, CR
        assert counts.get("utf8_cont", 0) == 64  # 0x80-0xBF
        assert counts.get("utf8_lead2", 0) == 32  # 0xC0-0xDF
        assert counts.get("utf8_lead3", 0) == 16  # 0xE0-0xEF
        assert counts.get("utf8_lead4", 0) == 8   # 0xF0-0xF7


class TestBondClassSemantics:
    """Test that bond classes have correct semantic meaning."""

    def test_alpha_bonds_letters(self):
        """ALPHA bond class should only apply to letters."""
        for bc in BYTE_TABLE:
            if bc.bond_class == BondClass.ALPHA:
                assert bc.category in (ByteCategory.LETTER_UPPER, ByteCategory.LETTER_LOWER)

    def test_numeric_bonds_digits(self):
        """NUMERIC bond class should only apply to digits."""
        for bc in BYTE_TABLE:
            if bc.bond_class == BondClass.NUMERIC:
                assert bc.category == ByteCategory.DIGIT

    def test_covalent_bonds_utf8(self):
        """COVALENT bond class should only apply to UTF-8 bytes."""
        for bc in BYTE_TABLE:
            if bc.bond_class == BondClass.COVALENT:
                assert bc.category in (
                    ByteCategory.UTF8_CONT,
                    ByteCategory.UTF8_LEAD2,
                    ByteCategory.UTF8_LEAD3,
                    ByteCategory.UTF8_LEAD4,
                )
