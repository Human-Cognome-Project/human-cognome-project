"""Tests for core module: token_id and pair_bond."""
import pytest

from hcp.core.token_id import (
    TokenID,
    encode_base20,
    decode_base20,
    byte_token,
    char_token,
)
from hcp.core.pair_bond import PairBond, BondRecurrence, PairBondMap, create_pbm_from_text


class TestBase20:
    """Test base-20 encoding/decoding."""

    def test_encode_zero(self):
        assert encode_base20(0) == "0"
        assert encode_base20(0, min_length=4) == "0000"

    def test_encode_small_numbers(self):
        assert encode_base20(1) == "1"
        assert encode_base20(9) == "9"
        assert encode_base20(10) == "A"
        assert encode_base20(19) == "J"
        assert encode_base20(20) == "10"

    def test_encode_larger_numbers(self):
        assert encode_base20(400) == "100"  # 20*20
        assert encode_base20(8000) == "1000"  # 20*20*20

    def test_decode_roundtrip(self):
        for n in [0, 1, 19, 20, 42, 100, 399, 400, 1000, 12345]:
            assert decode_base20(encode_base20(n)) == n

    def test_decode_case_insensitive(self):
        assert decode_base20("1a") == decode_base20("1A")
        assert decode_base20("abc") == decode_base20("ABC")


class TestTokenID:
    """Test TokenID class."""

    def test_create_nsm_token(self):
        token = TokenID.nsm(0)
        assert token.mode == 0
        assert token.value == 0
        assert token.is_nsm()

    def test_create_byte_token(self):
        token = TokenID.byte(65)  # ASCII 'A'
        assert token.mode == 1
        assert token.value == 65
        assert token.is_byte()

    def test_create_glyph_token(self):
        token = TokenID.glyph(ord("A"))
        assert token.mode == 2
        assert token.value == 65
        assert token.is_glyph()

    def test_byte_token_bounds(self):
        TokenID.byte(0)
        TokenID.byte(255)
        with pytest.raises(ValueError):
            TokenID.byte(256)
        with pytest.raises(ValueError):
            TokenID.byte(-1)

    def test_to_string_format(self):
        token = TokenID.byte(65)
        s = token.to_string()
        assert "-" in s
        assert s.startswith("01-")  # Mode 01

    def test_from_string_roundtrip(self):
        token = TokenID(mode=5, value=12345)
        s = token.to_string()
        parsed = TokenID.from_string(s)
        assert parsed.mode == token.mode
        assert parsed.value == token.value

    def test_immutable(self):
        token = TokenID.byte(65)
        with pytest.raises(AttributeError):
            token.mode = 2

    def test_hashable(self):
        t1 = TokenID.byte(65)
        t2 = TokenID.byte(65)
        t3 = TokenID.byte(66)
        assert hash(t1) == hash(t2)
        assert t1 == t2
        assert t1 != t3
        # Can use as dict key
        d = {t1: "A"}
        assert d[t2] == "A"


class TestPairBond:
    """Test PairBond class."""

    def test_create_bond(self):
        left = TokenID.byte(65)
        right = TokenID.byte(66)
        bond = PairBond(left, right)
        assert bond.left == left
        assert bond.right == right

    def test_reversed_bond(self):
        left = TokenID.byte(65)
        right = TokenID.byte(66)
        bond = PairBond(left, right)
        rev = bond.reversed()
        assert rev.left == right
        assert rev.right == left

    def test_bond_immutable(self):
        bond = PairBond(TokenID.byte(65), TokenID.byte(66))
        with pytest.raises(AttributeError):
            bond.left = TokenID.byte(67)


class TestBondRecurrence:
    """Test BondRecurrence class."""

    def test_increment(self):
        bond = PairBond(TokenID.byte(65), TokenID.byte(66))
        rec = BondRecurrence(bond)
        assert rec.count == 0
        rec.increment()
        assert rec.count == 1
        rec.increment(position=5)
        assert rec.count == 2
        assert rec.positions == [5]


class TestPairBondMap:
    """Test PairBondMap class."""

    def test_empty_pbm(self):
        pbm = PairBondMap()
        assert pbm.total_bonds == 0
        assert pbm.unique_bonds == 0

    def test_add_bond(self):
        pbm = PairBondMap()
        left = TokenID.byte(65)
        right = TokenID.byte(66)
        pbm.add_bond(left, right)
        assert pbm.unique_bonds == 1
        assert pbm.total_bonds == 1

    def test_add_same_bond_twice(self):
        pbm = PairBondMap()
        left = TokenID.byte(65)
        right = TokenID.byte(66)
        pbm.add_bond(left, right)
        pbm.add_bond(left, right)
        assert pbm.unique_bonds == 1
        assert pbm.total_bonds == 2

    def test_add_sequence(self):
        pbm = PairBondMap()
        tokens = [TokenID.byte(65), TokenID.byte(66), TokenID.byte(67)]
        pbm.add_sequence(tokens)
        # A->B and B->C
        assert pbm.unique_bonds == 2
        assert pbm.total_bonds == 2

    def test_bond_strength(self):
        pbm = PairBondMap()
        a = TokenID.byte(65)
        b = TokenID.byte(66)
        c = TokenID.byte(67)
        # A -> B twice, A -> C once
        pbm.add_bond(a, b)
        pbm.add_bond(a, b)
        pbm.add_bond(a, c)
        assert pbm.bond_strength(a, b) == pytest.approx(2/3)
        assert pbm.bond_strength(a, c) == pytest.approx(1/3)
        assert pbm.bond_strength(b, a) == 0.0  # No backward bond

    def test_sequence_preserved(self):
        pbm = PairBondMap()
        tokens = [TokenID.byte(b) for b in b"hello"]
        pbm.add_sequence(tokens)
        assert pbm.sequence() == tokens

    def test_serialization_roundtrip(self):
        pbm = PairBondMap()
        tokens = [TokenID.byte(b) for b in b"hello world"]
        pbm.add_sequence(tokens)

        data = pbm.to_dict()
        restored = PairBondMap.from_dict(data)

        assert restored.unique_bonds == pbm.unique_bonds
        assert restored.total_bonds == pbm.total_bonds


class TestCreatePBMFromText:
    """Test the convenience function."""

    def test_simple_text(self):
        pbm = create_pbm_from_text("AB")
        assert pbm.unique_bonds == 1
        # Bond from byte 65 (A) to byte 66 (B)
        bond = pbm.get_bond(TokenID.byte(65), TokenID.byte(66))
        assert bond is not None
        assert bond.count == 1

    def test_repeated_pattern(self):
        pbm = create_pbm_from_text("ABAB")
        # A->B appears twice, B->A appears once
        assert pbm.get_bond(TokenID.byte(65), TokenID.byte(66)).count == 2
        assert pbm.get_bond(TokenID.byte(66), TokenID.byte(65)).count == 1
