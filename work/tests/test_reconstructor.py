"""Tests for hcp.assembly.reconstructor module.

Tests expression reconstruction from PairBondMap structures.

Contributors: Silas & Planner (DI-cognome)
"""
import pytest

from hcp.assembly.reconstructor import (
    ReconstructionResult,
    Reconstructor,
    ByteReconstructor,
    reconstruct_from_pbm,
    pbm_to_bytes,
    pbm_to_string,
)
from hcp.core.token_id import TokenID
from hcp.core.pair_bond import PairBondMap, create_pbm_from_text


class TestReconstructionResult:
    """Test ReconstructionResult dataclass."""

    def test_create_result(self):
        """Test basic result creation."""
        tokens = [TokenID.byte(65), TokenID.byte(66)]
        result = ReconstructionResult(
            tokens=tokens,
            success=True,
            ambiguities=0,
            method="sequence"
        )
        assert result.success is True
        assert result.ambiguities == 0
        assert result.method == "sequence"
        assert len(result.tokens) == 2

    def test_to_bytes(self):
        """Test byte conversion."""
        tokens = [TokenID.byte(72), TokenID.byte(105)]  # Hi
        result = ReconstructionResult(
            tokens=tokens, success=True, ambiguities=0, method="sequence"
        )
        assert result.to_bytes() == b"Hi"

    def test_to_bytes_non_byte_token_raises(self):
        """Test that non-byte tokens raise error."""
        tokens = [TokenID.nsm(0)]  # NSM token
        result = ReconstructionResult(
            tokens=tokens, success=True, ambiguities=0, method="sequence"
        )
        with pytest.raises(ValueError, match="non-byte"):
            result.to_bytes()

    def test_to_string(self):
        """Test string conversion."""
        tokens = [TokenID.byte(72), TokenID.byte(105)]  # Hi
        result = ReconstructionResult(
            tokens=tokens, success=True, ambiguities=0, method="sequence"
        )
        assert result.to_string() == "Hi"


class TestReconstructor:
    """Test Reconstructor class."""

    def test_reconstruct_from_sequence(self):
        """Test reconstruction using stored sequence."""
        pbm = create_pbm_from_text("Hello")
        reconstructor = Reconstructor()
        result = reconstructor.reconstruct(pbm)

        assert result.success is True
        assert result.method == "sequence"
        assert result.ambiguities == 0
        assert result.to_string() == "Hello"

    def test_reconstruct_empty_pbm(self):
        """Test reconstruction of empty PBM."""
        pbm = PairBondMap()
        reconstructor = Reconstructor()
        result = reconstructor.reconstruct(pbm)

        assert result.success is True
        assert result.tokens == []

    def test_reconstruct_single_char(self):
        """Test reconstruction of single character."""
        pbm = create_pbm_from_text("A")
        reconstructor = Reconstructor()
        result = reconstructor.reconstruct(pbm)

        # Single char has no bonds, but sequence should be stored
        assert result.to_string() == "A"

    def test_roundtrip_simple(self):
        """Test simple text round-trip."""
        text = "Hello"
        pbm = create_pbm_from_text(text)
        reconstructor = Reconstructor()
        result = reconstructor.reconstruct(pbm)

        assert result.to_string() == text

    def test_roundtrip_with_spaces(self):
        """Test round-trip with spaces."""
        text = "Hello World"
        pbm = create_pbm_from_text(text)
        reconstructor = Reconstructor()
        result = reconstructor.reconstruct(pbm)

        assert result.to_string() == text

    def test_roundtrip_with_punctuation(self):
        """Test round-trip with punctuation."""
        text = "Hello, World!"
        pbm = create_pbm_from_text(text)
        reconstructor = Reconstructor()
        result = reconstructor.reconstruct(pbm)

        assert result.to_string() == text

    def test_roundtrip_repeated_chars(self):
        """Test round-trip with repeated characters."""
        text = "aaa bbb"
        pbm = create_pbm_from_text(text)
        reconstructor = Reconstructor()
        result = reconstructor.reconstruct(pbm)

        assert result.to_string() == text

    def test_tokens_to_bytes(self):
        """Test token sequence to bytes conversion."""
        reconstructor = Reconstructor()
        tokens = [TokenID.byte(72), TokenID.byte(105)]
        assert reconstructor.tokens_to_bytes(tokens) == b"Hi"

    def test_tokens_to_string(self):
        """Test token sequence to string conversion."""
        reconstructor = Reconstructor()
        tokens = [TokenID.byte(72), TokenID.byte(105)]
        assert reconstructor.tokens_to_string(tokens) == "Hi"


class TestByteReconstructor:
    """Test ByteReconstructor class."""

    def test_reconstruct_bytes(self):
        """Test direct byte reconstruction."""
        pbm = create_pbm_from_text("Hi")
        reconstructor = ByteReconstructor()
        result = reconstructor.reconstruct_bytes(pbm)

        assert result == b"Hi"

    def test_reconstruct_string(self):
        """Test direct string reconstruction."""
        pbm = create_pbm_from_text("Hi")
        reconstructor = ByteReconstructor()
        result = reconstructor.reconstruct_string(pbm)

        assert result == "Hi"


class TestConvenienceFunctions:
    """Test module-level convenience functions."""

    def test_reconstruct_from_pbm(self):
        """Test reconstruct_from_pbm function."""
        pbm = create_pbm_from_text("Hi")
        tokens = reconstruct_from_pbm(pbm)

        assert len(tokens) == 2
        assert tokens[0].value == 72  # H
        assert tokens[1].value == 105  # i

    def test_pbm_to_bytes(self):
        """Test pbm_to_bytes function."""
        pbm = create_pbm_from_text("Hello")
        result = pbm_to_bytes(pbm)

        assert result == b"Hello"

    def test_pbm_to_string(self):
        """Test pbm_to_string function."""
        pbm = create_pbm_from_text("Hello")
        result = pbm_to_string(pbm)

        assert result == "Hello"


class TestRoundTrips:
    """Test various round-trip scenarios."""

    @pytest.mark.parametrize("text", [
        "a",
        "ab",
        "abc",
        "Hello",
        "Hello World",
        "Hello, World!",
        "123",
        "abc123",
        "  spaces  ",
        "line1\nline2",
        "tab\there",
    ])
    def test_roundtrip_various_inputs(self, text):
        """Test round-trip with various inputs."""
        pbm = create_pbm_from_text(text)
        result = pbm_to_string(pbm)
        assert result == text

    def test_roundtrip_all_printable_ascii(self):
        """Test round-trip with all printable ASCII."""
        text = "".join(chr(i) for i in range(32, 127))
        pbm = create_pbm_from_text(text)
        result = pbm_to_string(pbm)
        assert result == text

    def test_roundtrip_unicode(self):
        """Test round-trip with Unicode."""
        text = "Café"
        pbm = create_pbm_from_text(text)
        result = pbm_to_string(pbm)
        assert result == text

    def test_roundtrip_emoji(self):
        """Test round-trip with emoji."""
        text = "Hello 👋"
        pbm = create_pbm_from_text(text)
        result = pbm_to_string(pbm)
        assert result == text


class TestEdgeCases:
    """Test edge cases and potential issues."""

    def test_empty_string(self):
        """Test empty string handling."""
        pbm = create_pbm_from_text("")
        result = pbm_to_string(pbm)
        assert result == ""

    def test_repeated_pattern(self):
        """Test repeated patterns preserve correctly."""
        text = "ABAB"
        pbm = create_pbm_from_text(text)
        result = pbm_to_string(pbm)
        assert result == text

    def test_long_string(self):
        """Test longer strings."""
        text = "The quick brown fox jumps over the lazy dog."
        pbm = create_pbm_from_text(text)
        result = pbm_to_string(pbm)
        assert result == text

    def test_binary_safe(self):
        """Test handling of binary data."""
        data = bytes([0, 1, 255, 128, 64])
        from hcp.atomizer.byte_atomizer import ByteAtomizer
        atomizer = ByteAtomizer()
        pbm = atomizer.to_pbm(data)
        result = pbm_to_bytes(pbm)
        assert result == data
