"""Tests for byte code classification."""

import pytest
from hcp.core.byte_codes import (
    ByteCategory,
    BondClass,
    ByteCode,
    classify_byte,
    BYTE_TABLE,
)


class TestByteTable:
    def test_has_256_entries(self):
        assert len(BYTE_TABLE) == 256

    def test_all_are_bytecodes(self):
        for bc in BYTE_TABLE:
            assert isinstance(bc, ByteCode)

    def test_values_sequential(self):
        for i, bc in enumerate(BYTE_TABLE):
            assert bc.value == i


class TestClassifyByte:
    def test_null(self):
        bc = classify_byte(0x00)
        assert bc.category == ByteCategory.CONTROL
        assert bc.bond_class == BondClass.INERT

    def test_space(self):
        bc = classify_byte(0x20)
        assert bc.category == ByteCategory.WHITESPACE
        assert bc.bond_class == BondClass.SEPARATOR
        assert bc.ascii_char == " "

    def test_tab(self):
        bc = classify_byte(0x09)
        assert bc.category == ByteCategory.WHITESPACE
        assert bc.bond_class == BondClass.SEPARATOR

    def test_newline(self):
        bc = classify_byte(0x0A)
        assert bc.category == ByteCategory.WHITESPACE

    def test_uppercase_A(self):
        bc = classify_byte(0x41)
        assert bc.category == ByteCategory.LETTER_UPPER
        assert bc.bond_class == BondClass.ALPHA
        assert bc.ascii_char == "A"

    def test_lowercase_z(self):
        bc = classify_byte(0x7A)
        assert bc.category == ByteCategory.LETTER_LOWER
        assert bc.bond_class == BondClass.ALPHA
        assert bc.ascii_char == "z"

    def test_digit_0(self):
        bc = classify_byte(0x30)
        assert bc.category == ByteCategory.DIGIT
        assert bc.bond_class == BondClass.NUMERIC
        assert bc.ascii_char == "0"

    def test_exclamation(self):
        bc = classify_byte(0x21)
        assert bc.category == ByteCategory.PUNCTUATION
        assert bc.bond_class == BondClass.DELIMITER

    def test_utf8_continuation(self):
        bc = classify_byte(0x80)
        assert bc.category == ByteCategory.UTF8_CONT
        assert bc.bond_class == BondClass.COVALENT

    def test_utf8_2byte_lead(self):
        bc = classify_byte(0xC0)
        assert bc.category == ByteCategory.UTF8_LEAD2
        assert bc.bond_class == BondClass.COVALENT

    def test_utf8_3byte_lead(self):
        bc = classify_byte(0xE0)
        assert bc.category == ByteCategory.UTF8_LEAD3

    def test_utf8_4byte_lead(self):
        bc = classify_byte(0xF0)
        assert bc.category == ByteCategory.UTF8_LEAD4

    def test_invalid_byte(self):
        bc = classify_byte(0xF8)
        assert bc.category == ByteCategory.INVALID
        assert bc.bond_class == BondClass.UNSTABLE

    def test_delete_is_control(self):
        bc = classify_byte(0x7F)
        assert bc.category == ByteCategory.CONTROL

    def test_all_printable_ascii_have_char(self):
        for v in range(0x21, 0x7F):
            bc = classify_byte(v)
            assert bc.ascii_char is not None

    def test_hex_format(self):
        bc = classify_byte(0x0A)
        assert bc.hex == "0x0A"

    def test_frozen(self):
        bc = classify_byte(0x41)
        with pytest.raises(AttributeError):
            bc.value = 99
