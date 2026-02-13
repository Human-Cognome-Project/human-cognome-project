"""Tests for hcp.atomizer.byte_atomizer module.

Tests byte-level atomization, span classification, and token conversion.

Contributors: Silas & Planner (DI-cognome)
"""
import pytest

from hcp.atomizer.byte_atomizer import (
    ByteAtom,
    ByteAtomizer,
    ByteSpan,
    ByteSpanClassifier,
    atomize,
    bytes_to_tokens,
)
from hcp.core.token_id import TokenID


class TestByteAtom:
    """Test ByteAtom dataclass."""

    def test_create_atom(self):
        """Test basic atom creation."""
        atom = ByteAtom(value=65, position=0)
        assert atom.value == 65
        assert atom.position == 0

    def test_char_printable(self):
        """Test char property for printable ASCII."""
        atom = ByteAtom(value=65, position=0)
        assert atom.char == 'A'

    def test_char_non_printable(self):
        """Test char property for non-printable bytes."""
        atom = ByteAtom(value=0, position=0)
        assert atom.char is None
        atom = ByteAtom(value=127, position=0)
        assert atom.char is None

    def test_is_ascii(self):
        """Test is_ascii property."""
        assert ByteAtom(value=0, position=0).is_ascii is True
        assert ByteAtom(value=127, position=0).is_ascii is True
        assert ByteAtom(value=128, position=0).is_ascii is False
        assert ByteAtom(value=255, position=0).is_ascii is False

    def test_is_printable(self):
        """Test is_printable property."""
        assert ByteAtom(value=32, position=0).is_printable is True  # space
        assert ByteAtom(value=126, position=0).is_printable is True  # ~
        assert ByteAtom(value=31, position=0).is_printable is False
        assert ByteAtom(value=127, position=0).is_printable is False

    def test_is_whitespace(self):
        """Test is_whitespace property."""
        assert ByteAtom(value=9, position=0).is_whitespace is True   # tab
        assert ByteAtom(value=10, position=0).is_whitespace is True  # LF
        assert ByteAtom(value=13, position=0).is_whitespace is True  # CR
        assert ByteAtom(value=32, position=0).is_whitespace is True  # space
        assert ByteAtom(value=65, position=0).is_whitespace is False

    def test_token_property(self):
        """Test conversion to TokenID."""
        atom = ByteAtom(value=65, position=0)
        token = atom.token
        assert isinstance(token, TokenID)
        assert token.value == 65
        assert token.is_byte()

    def test_str_printable(self):
        """Test string representation for printable."""
        atom = ByteAtom(value=65, position=0)
        assert str(atom) == "'A'"

    def test_str_non_printable(self):
        """Test string representation for non-printable."""
        atom = ByteAtom(value=0, position=0)
        assert str(atom) == "0x00"
        atom = ByteAtom(value=255, position=0)
        assert str(atom) == "0xFF"

    def test_frozen(self):
        """Test that ByteAtom is immutable."""
        atom = ByteAtom(value=65, position=0)
        with pytest.raises(Exception):  # FrozenInstanceError
            atom.value = 66


class TestByteAtomizer:
    """Test ByteAtomizer class."""

    def test_atomize_empty(self):
        """Test atomizing empty bytes."""
        atomizer = ByteAtomizer()
        result = atomizer.atomize(b"")
        assert result == []

    def test_atomize_single_byte(self):
        """Test atomizing single byte."""
        atomizer = ByteAtomizer()
        result = atomizer.atomize(b"A")
        assert len(result) == 1
        assert result[0].value == 65
        assert result[0].position == 0

    def test_atomize_multiple_bytes(self):
        """Test atomizing multiple bytes."""
        atomizer = ByteAtomizer()
        result = atomizer.atomize(b"Hello")
        assert len(result) == 5
        assert [a.value for a in result] == [72, 101, 108, 108, 111]
        assert [a.position for a in result] == [0, 1, 2, 3, 4]

    def test_atomize_binary(self):
        """Test atomizing arbitrary binary data."""
        atomizer = ByteAtomizer()
        result = atomizer.atomize(bytes([0, 128, 255]))
        assert len(result) == 3
        assert result[0].value == 0
        assert result[1].value == 128
        assert result[2].value == 255

    def test_atomize_text(self):
        """Test atomizing text (UTF-8)."""
        atomizer = ByteAtomizer()
        result = atomizer.atomize_text("Hi")
        assert len(result) == 2
        assert result[0].value == 72  # H
        assert result[1].value == 105  # i

    def test_atomize_text_unicode(self):
        """Test atomizing Unicode text."""
        atomizer = ByteAtomizer()
        result = atomizer.atomize_text("é")  # 2 bytes in UTF-8
        assert len(result) == 2
        assert result[0].value == 0xC3
        assert result[1].value == 0xA9

    def test_to_tokens(self):
        """Test conversion to tokens."""
        atomizer = ByteAtomizer()
        tokens = atomizer.to_tokens(b"AB")
        assert len(tokens) == 2
        assert all(isinstance(t, TokenID) for t in tokens)
        assert tokens[0].value == 65
        assert tokens[1].value == 66

    def test_to_pbm(self):
        """Test conversion to PairBondMap."""
        atomizer = ByteAtomizer()
        pbm = atomizer.to_pbm(b"AB")
        assert pbm.unique_bonds == 1
        assert pbm.total_bonds == 1

    def test_to_pbm_repeated(self):
        """Test PBM with repeated patterns."""
        atomizer = ByteAtomizer()
        pbm = atomizer.to_pbm(b"ABAB")
        # A->B appears twice, B->A appears once
        assert pbm.unique_bonds == 2
        assert pbm.total_bonds == 3

    def test_iter_atoms(self):
        """Test iterator interface."""
        atomizer = ByteAtomizer()
        atoms = list(atomizer.iter_atoms(b"Hi"))
        assert len(atoms) == 2
        assert atoms[0].value == 72
        assert atoms[1].value == 105


class TestByteSpan:
    """Test ByteSpan dataclass."""

    def test_create_span(self):
        """Test span creation."""
        atoms = [ByteAtom(value=65, position=0), ByteAtom(value=66, position=1)]
        span = ByteSpan(start=0, end=2, atoms=atoms, span_type="word")
        assert span.start == 0
        assert span.end == 2
        assert span.span_type == "word"

    def test_length(self):
        """Test length property."""
        atoms = [ByteAtom(value=65, position=0), ByteAtom(value=66, position=1)]
        span = ByteSpan(start=0, end=2, atoms=atoms, span_type="word")
        assert span.length == 2

    def test_to_bytes(self):
        """Test byte reconstruction."""
        atoms = [ByteAtom(value=65, position=0), ByteAtom(value=66, position=1)]
        span = ByteSpan(start=0, end=2, atoms=atoms, span_type="word")
        assert span.to_bytes() == b"AB"

    def test_to_string_valid(self):
        """Test string conversion for valid UTF-8."""
        atoms = [ByteAtom(value=72, position=0), ByteAtom(value=105, position=1)]
        span = ByteSpan(start=0, end=2, atoms=atoms, span_type="word")
        assert span.to_string() == "Hi"

    def test_to_string_invalid(self):
        """Test string conversion for invalid UTF-8."""
        atoms = [ByteAtom(value=255, position=0)]
        span = ByteSpan(start=0, end=1, atoms=atoms, span_type="binary")
        assert span.to_string() is None


class TestByteSpanClassifier:
    """Test ByteSpanClassifier class."""

    def test_classify_whitespace(self):
        """Test whitespace classification."""
        classifier = ByteSpanClassifier()
        assert classifier.classify_byte(32) == "whitespace"  # space
        assert classifier.classify_byte(9) == "whitespace"   # tab
        assert classifier.classify_byte(10) == "whitespace"  # LF
        assert classifier.classify_byte(13) == "whitespace"  # CR

    def test_classify_word(self):
        """Test word character classification."""
        classifier = ByteSpanClassifier()
        assert classifier.classify_byte(65) == "word"   # A
        assert classifier.classify_byte(97) == "word"   # a
        assert classifier.classify_byte(48) == "word"   # 0
        assert classifier.classify_byte(95) == "word"   # _

    def test_classify_punctuation(self):
        """Test punctuation classification."""
        classifier = ByteSpanClassifier()
        assert classifier.classify_byte(33) == "punctuation"  # !
        assert classifier.classify_byte(46) == "punctuation"  # .
        assert classifier.classify_byte(44) == "punctuation"  # ,

    def test_classify_binary(self):
        """Test binary (high byte) classification."""
        classifier = ByteSpanClassifier()
        assert classifier.classify_byte(128) == "binary"
        assert classifier.classify_byte(255) == "binary"

    def test_span_bytes_empty(self):
        """Test spanning empty bytes."""
        classifier = ByteSpanClassifier()
        result = classifier.span_bytes(b"")
        assert result == []

    def test_span_bytes_single_type(self):
        """Test spanning single type."""
        classifier = ByteSpanClassifier()
        result = classifier.span_bytes(b"Hello")
        assert len(result) == 1
        assert result[0].span_type == "word"
        assert result[0].to_bytes() == b"Hello"

    def test_span_bytes_mixed(self):
        """Test spanning mixed types."""
        classifier = ByteSpanClassifier()
        result = classifier.span_bytes(b"Hello World")
        assert len(result) == 3
        assert result[0].span_type == "word"
        assert result[0].to_string() == "Hello"
        assert result[1].span_type == "whitespace"
        assert result[2].span_type == "word"
        assert result[2].to_string() == "World"

    def test_span_bytes_with_punctuation(self):
        """Test spanning with punctuation."""
        classifier = ByteSpanClassifier()
        result = classifier.span_bytes(b"Hi!")
        assert len(result) == 2
        assert result[0].span_type == "word"
        assert result[1].span_type == "punctuation"


class TestConvenienceFunctions:
    """Test module-level convenience functions."""

    def test_atomize_bytes(self):
        """Test atomize with bytes."""
        result = atomize(b"Hi")
        assert len(result) == 2
        assert result[0].value == 72

    def test_atomize_string(self):
        """Test atomize with string."""
        result = atomize("Hi")
        assert len(result) == 2
        assert result[0].value == 72

    def test_bytes_to_tokens_bytes(self):
        """Test bytes_to_tokens with bytes."""
        tokens = bytes_to_tokens(b"AB")
        assert len(tokens) == 2
        assert tokens[0].value == 65
        assert tokens[1].value == 66

    def test_bytes_to_tokens_string(self):
        """Test bytes_to_tokens with string."""
        tokens = bytes_to_tokens("AB")
        assert len(tokens) == 2
        assert tokens[0].value == 65
        assert tokens[1].value == 66
