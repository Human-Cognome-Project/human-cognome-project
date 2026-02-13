"""Tests for hcp.core.token_id module.

Tests the base-50 Token ID encoding system.

Contributors: Planner & Silas (DI-cognome)
"""
import pytest

from hcp.core.token_id import (
    ALPHABET,
    BASE,
    CHAR_TO_INDEX,
    encode_pair,
    decode_pair,
    encode_token_id,
    decode_token_id,
    token_depth,
    MODE_UNIVERSAL,
    MODE_TEXT,
    MODE_PEOPLE,
    MODE_PLACES,
    MODE_THINGS,
    MODE_NAMES,
    MODE_SOURCE_PBM,
    LANG_ENGLISH,
    LAYER_WORD,
    SUB_NOUN,
    encode_word_token_id,
    encode_name_token_id,
)


class TestAlphabet:
    """Test alphabet constants."""

    def test_alphabet_length(self):
        """Alphabet should have exactly 50 characters."""
        assert len(ALPHABET) == 50
        assert BASE == 50

    def test_alphabet_excludes_o(self):
        """Alphabet should exclude O and o (confusion with zero)."""
        assert 'O' not in ALPHABET
        assert 'o' not in ALPHABET

    def test_alphabet_order(self):
        """Alphabet should be ASCII sorted (uppercase first)."""
        assert ALPHABET[:25] == "ABCDEFGHIJKLMNPQRSTUVWXYZ"
        assert ALPHABET[25:] == "abcdefghijklmnpqrstuvwxyz"

    def test_char_to_index_complete(self):
        """CHAR_TO_INDEX should map all alphabet characters."""
        assert len(CHAR_TO_INDEX) == 50
        for i, c in enumerate(ALPHABET):
            assert CHAR_TO_INDEX[c] == i


class TestEncodePair:
    """Test encode_pair function."""

    def test_encode_zero(self):
        """Zero should encode to 'AA'."""
        assert encode_pair(0) == "AA"

    def test_encode_one(self):
        """One should encode to 'AB'."""
        assert encode_pair(1) == "AB"

    def test_encode_base_minus_one(self):
        """49 should encode to 'Az' (last char of first row)."""
        assert encode_pair(49) == "Az"

    def test_encode_base(self):
        """50 should encode to 'BA' (first char of second row)."""
        assert encode_pair(50) == "BA"

    def test_encode_max_value(self):
        """2499 should encode to 'zz' (maximum pair value)."""
        assert encode_pair(2499) == "zz"

    def test_encode_middle_values(self):
        """Test some middle values."""
        # Value 100 = 2*50 + 0 = "CA"
        assert encode_pair(100) == "CA"
        # Value 1250 = 25*50 + 0 = "aA" (first lowercase)
        assert encode_pair(1250) == "aA"

    def test_encode_negative_raises(self):
        """Negative values should raise ValueError."""
        with pytest.raises(ValueError, match="must be 0-2499"):
            encode_pair(-1)

    def test_encode_too_large_raises(self):
        """Values >= 2500 should raise ValueError."""
        with pytest.raises(ValueError, match="must be 0-2499"):
            encode_pair(2500)


class TestDecodePair:
    """Test decode_pair function."""

    def test_decode_aa(self):
        """'AA' should decode to 0."""
        assert decode_pair("AA") == 0

    def test_decode_ab(self):
        """'AB' should decode to 1."""
        assert decode_pair("AB") == 1

    def test_decode_zz(self):
        """'zz' should decode to 2499."""
        assert decode_pair("zz") == 2499

    def test_decode_wrong_length_raises(self):
        """Non-2-character strings should raise ValueError."""
        with pytest.raises(ValueError, match="must be 2 characters"):
            decode_pair("A")
        with pytest.raises(ValueError, match="must be 2 characters"):
            decode_pair("AAA")
        with pytest.raises(ValueError, match="must be 2 characters"):
            decode_pair("")

    def test_decode_invalid_char_raises(self):
        """Invalid characters (including O/o) should raise ValueError."""
        with pytest.raises(ValueError, match="Invalid characters"):
            decode_pair("AO")  # O excluded
        with pytest.raises(ValueError, match="Invalid characters"):
            decode_pair("Ao")  # o excluded
        with pytest.raises(ValueError, match="Invalid characters"):
            decode_pair("A0")  # digit
        with pytest.raises(ValueError, match="Invalid characters"):
            decode_pair("A!")  # punctuation

    def test_roundtrip_all_values(self):
        """Encode then decode should return original value for all valid inputs."""
        for v in range(2500):
            assert decode_pair(encode_pair(v)) == v


class TestEncodeTokenId:
    """Test encode_token_id function."""

    def test_single_pair(self):
        """Single value should produce single pair."""
        assert encode_token_id(0) == "AA"
        assert encode_token_id(1) == "AB"

    def test_two_pairs(self):
        """Two values should produce dot-separated pairs."""
        assert encode_token_id(0, 1) == "AA.AB"
        assert encode_token_id(1, 1) == "AB.AB"

    def test_five_pairs(self):
        """Five values should produce five dot-separated pairs."""
        result = encode_token_id(0, 1, 100, 0, 1)
        assert result == "AA.AB.CA.AA.AB"
        assert result.count(".") == 4

    def test_zero_values_raises(self):
        """Zero values should raise ValueError."""
        with pytest.raises(ValueError, match="requires 1-5 pairs"):
            encode_token_id()

    def test_six_values_raises(self):
        """More than 5 values should raise ValueError."""
        with pytest.raises(ValueError, match="requires 1-5 pairs"):
            encode_token_id(0, 0, 0, 0, 0, 0)


class TestDecodeTokenId:
    """Test decode_token_id function."""

    def test_single_pair(self):
        """Single pair should decode to single-element tuple."""
        assert decode_token_id("AA") == (0,)
        assert decode_token_id("AB") == (1,)

    def test_multiple_pairs(self):
        """Multiple pairs should decode to tuple."""
        assert decode_token_id("AA.AB") == (0, 1)
        assert decode_token_id("AA.AB.CA.AA.AB") == (0, 1, 100, 0, 1)

    def test_zero_pairs_raises(self):
        """Empty string should raise ValueError."""
        with pytest.raises(ValueError):
            decode_token_id("")

    def test_six_pairs_raises(self):
        """More than 5 pairs should raise ValueError."""
        with pytest.raises(ValueError, match="requires 1-5 pairs"):
            decode_token_id("AA.AA.AA.AA.AA.AA")

    def test_roundtrip(self):
        """Encode then decode should return original values."""
        values = (0, 1, 100, 2000, 2499)
        assert decode_token_id(encode_token_id(*values)) == values


class TestTokenDepth:
    """Test token_depth function."""

    def test_depth_one(self):
        assert token_depth("AA") == 1

    def test_depth_two(self):
        assert token_depth("AA.AB") == 2

    def test_depth_five(self):
        assert token_depth("AA.AB.CA.AA.AB") == 5


class TestModeConstants:
    """Test mode constant values."""

    def test_mode_universal(self):
        """MODE_UNIVERSAL should be 'AA' (value 0)."""
        assert MODE_UNIVERSAL == "AA"
        assert decode_pair(MODE_UNIVERSAL) == 0

    def test_mode_text(self):
        """MODE_TEXT should be 'AB' (value 1)."""
        assert MODE_TEXT == "AB"
        assert decode_pair(MODE_TEXT) == 1

    def test_mode_people(self):
        """MODE_PEOPLE should be 'vA' (value 2250 = 45*50)."""
        assert MODE_PEOPLE == "vA"
        assert decode_pair(MODE_PEOPLE) == 2250

    def test_mode_places(self):
        """MODE_PLACES should be 'wA' (value 2300 = 46*50)."""
        assert MODE_PLACES == "wA"
        assert decode_pair(MODE_PLACES) == 2300

    def test_mode_things(self):
        """MODE_THINGS should be 'xA' (value 2350 = 47*50)."""
        assert MODE_THINGS == "xA"
        assert decode_pair(MODE_THINGS) == 2350

    def test_mode_names(self):
        """MODE_NAMES should be 'yA' (value 2400 = 48*50)."""
        assert MODE_NAMES == "yA"
        assert decode_pair(MODE_NAMES) == 2400

    def test_mode_source_pbm(self):
        """MODE_SOURCE_PBM should be 'zA' (value 2450 = 49*50)."""
        assert MODE_SOURCE_PBM == "zA"
        assert decode_pair(MODE_SOURCE_PBM) == 2450

    def test_entity_modes_at_end(self):
        """Entity modes (v-z) should be at upper range of namespace."""
        assert decode_pair(MODE_PEOPLE) > 2000
        assert decode_pair(MODE_SOURCE_PBM) < 2500


class TestEncodeWordTokenId:
    """Test encode_word_token_id function."""

    def test_basic_noun(self):
        """Basic English noun token."""
        # Layer=WORD(2), Sub=NOUN(0), counts=0,1
        result = encode_word_token_id(LAYER_WORD, SUB_NOUN, 0, 1)
        # Should be AB.AB.CA.AA.AB
        # AB = text mode (1)
        # AB = English (1)
        # CA = layer 2 * 50 + 0 = 100
        # AA = 0
        # AB = 1
        assert result == "AB.AB.CA.AA.AB"

    def test_format_five_pairs(self):
        """Word token IDs should always be 5 pairs."""
        result = encode_word_token_id(0, 0, 0, 0)
        assert token_depth(result) == 5


class TestEncodeNameTokenId:
    """Test encode_name_token_id function."""

    def test_first_name(self):
        """First name component (count=0)."""
        result = encode_name_token_id(0)
        # yA.AA.AA
        assert result.startswith("yA.")
        assert token_depth(result) == 3

    def test_sequential_names(self):
        """Sequential name components should increment."""
        name0 = encode_name_token_id(0)
        name1 = encode_name_token_id(1)
        name2 = encode_name_token_id(2)

        # All should start with yA (names namespace)
        assert name0.startswith("yA.")
        assert name1.startswith("yA.")
        assert name2.startswith("yA.")

        # Should be different
        assert name0 != name1 != name2

    def test_large_count(self):
        """Large counts should work within address space."""
        # 6.25M addresses available
        result = encode_name_token_id(1000000)
        assert result.startswith("yA.")
        assert token_depth(result) == 3
