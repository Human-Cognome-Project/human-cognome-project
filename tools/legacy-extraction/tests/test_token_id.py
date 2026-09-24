"""Tests for the base-50 Token ID encoding system."""

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
    encode_word_token_id,
    encode_name_token_id,
    MODE_UNIVERSAL,
    MODE_TEXT,
    MODE_SOURCE_PBM,
)


class TestAlphabet:
    def test_length(self):
        assert len(ALPHABET) == 50

    def test_no_O_or_o(self):
        assert "O" not in ALPHABET
        assert "o" not in ALPHABET

    def test_sorted_uppercase_first(self):
        assert ALPHABET == "ABCDEFGHIJKLMNPQRSTUVWXYZabcdefghijklmnpqrstuvwxyz"

    def test_reverse_lookup_complete(self):
        assert len(CHAR_TO_INDEX) == 50
        for i, c in enumerate(ALPHABET):
            assert CHAR_TO_INDEX[c] == i


class TestEncodePair:
    def test_zero(self):
        assert encode_pair(0) == "AA"

    def test_one(self):
        assert encode_pair(1) == "AB"

    def test_base_minus_one(self):
        assert encode_pair(49) == "Az"

    def test_base(self):
        assert encode_pair(50) == "BA"

    def test_max_value(self):
        assert encode_pair(2499) == "zz"

    def test_negative_raises(self):
        with pytest.raises(ValueError):
            encode_pair(-1)

    def test_overflow_raises(self):
        with pytest.raises(ValueError):
            encode_pair(2500)


class TestDecodePair:
    def test_AA(self):
        assert decode_pair("AA") == 0

    def test_AB(self):
        assert decode_pair("AB") == 1

    def test_zz(self):
        assert decode_pair("zz") == 2499

    def test_roundtrip_all_values(self):
        for v in range(2500):
            assert decode_pair(encode_pair(v)) == v

    def test_wrong_length_raises(self):
        with pytest.raises(ValueError):
            decode_pair("A")

    def test_invalid_char_raises(self):
        with pytest.raises(ValueError):
            decode_pair("AO")  # O excluded from alphabet

    def test_three_chars_raises(self):
        with pytest.raises(ValueError):
            decode_pair("AAA")


class TestEncodeTokenId:
    def test_single_pair(self):
        assert encode_token_id(0) == "AA"

    def test_two_pairs(self):
        assert encode_token_id(0, 1) == "AA.AB"

    def test_five_pairs(self):
        result = encode_token_id(0, 1, 2, 3, 4)
        assert result == "AA.AB.AC.AD.AE"

    def test_zero_pairs_raises(self):
        with pytest.raises(ValueError):
            encode_token_id()

    def test_six_pairs_raises(self):
        with pytest.raises(ValueError):
            encode_token_id(0, 0, 0, 0, 0, 0)


class TestDecodeTokenId:
    def test_single_pair(self):
        assert decode_token_id("AA") == (0,)

    def test_two_pairs(self):
        assert decode_token_id("AA.AB") == (0, 1)

    def test_five_pairs(self):
        assert decode_token_id("AA.AB.AC.AD.AE") == (0, 1, 2, 3, 4)

    def test_roundtrip(self):
        values = (100, 200, 300, 400, 500)
        assert decode_token_id(encode_token_id(*values)) == values


class TestTokenDepth:
    def test_depth_1(self):
        assert token_depth("AA") == 1

    def test_depth_3(self):
        assert token_depth("AA.AB.AC") == 3

    def test_depth_5(self):
        assert token_depth("AA.AB.AC.AD.AE") == 5


class TestModeConstants:
    def test_universal(self):
        assert MODE_UNIVERSAL == "AA"

    def test_text(self):
        assert MODE_TEXT == "AB"

    def test_source_pbm(self):
        assert MODE_SOURCE_PBM == "zA"


class TestEncodeWordTokenId:
    def test_noun_token(self):
        result = encode_word_token_id(2, 0, 0, 0)  # Layer C (word), Sub A (noun)
        assert result.startswith("AB.AB.CA.")

    def test_verb_token(self):
        result = encode_word_token_id(2, 1, 0, 0)  # Layer C (word), Sub B (verb)
        assert result.startswith("AB.AB.CB.")

    def test_format_five_pairs(self):
        result = encode_word_token_id(2, 0, 0, 0)
        assert len(result.split(".")) == 5


class TestEncodeNameTokenId:
    def test_first_name(self):
        result = encode_name_token_id(0)
        assert result.startswith("yA.")
        assert len(result.split(".")) == 3

    def test_sequential(self):
        r1 = encode_name_token_id(0)
        r2 = encode_name_token_id(1)
        assert r1 != r2
