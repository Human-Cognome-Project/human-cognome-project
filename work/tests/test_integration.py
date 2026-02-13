"""Integration tests for the full HCP pipeline.

NOTE: These tests are aspirational - they test modules that are planned
but not yet implemented. All tests are skipped until the corresponding
modules exist in work/hcp/.

See work/implementation-plan.md for the roadmap.
"""
import pytest

# Skip all tests in this module - implementation pending
pytestmark = pytest.mark.skip(reason="Modules not yet implemented (see implementation-plan.md)")


class TestMVPPipeline:
    """Test the full MVP pipeline."""

    def test_misspelled_pangram_correction(self):
        """Test the main demo case: correcting misspelled pangram."""
        from hcp.physics.engine import correct

        text = "The quik brwon fox jumps oevr the layz dog"
        result = correct(text)

        assert "quick" in result
        assert "brown" in result
        assert "over" in result
        assert "lazy" in result

    def test_byte_roundtrip(self):
        """Test that bytes round-trip correctly."""
        from hcp.core.pair_bond import create_pbm_from_text
        from hcp.assembly.reconstructor import pbm_to_string

        text = "Hello, World!"
        pbm = create_pbm_from_text(text)
        reconstructed = pbm_to_string(pbm)

        assert reconstructed == text

    def test_all_256_bytes(self):
        """Test that all 256 byte values can be handled."""
        from hcp.assembly.validator import RoundtripValidator

        validator = RoundtripValidator()
        failures = validator.validate_all_bytes()

        assert len(failures) == 0, f"Failed bytes: {[f[0] for f in failures]}"

    def test_nsm_decomposition(self):
        """Test NSM decomposition coverage."""
        from hcp.abstraction.decomposer import Decomposer

        decomposer = Decomposer()
        result = decomposer.decompose_text("the quick brown fox")

        # Should have good coverage
        assert result.coverage >= 0.75
        assert result.total_primitives > 0

    def test_full_demo(self):
        """Test that the full demo runs successfully."""
        from hcp.api.demo import run_demo

        results = run_demo("The quik brwon fox", verbose=False)

        assert results["reconstruction_valid"]
        assert len(results["corrections"]) >= 2


class TestSpellingCorrection:
    """Test spelling correction specifically."""

    def test_transposition_correction(self):
        """Test that transpositions are corrected."""
        from hcp.physics.engine import correct

        assert "lazy" in correct("the layz dog")
        assert "over" in correct("oevr the")

    def test_substitution_correction(self):
        """Test that substitutions are corrected."""
        from hcp.physics.engine import correct

        assert "quick" in correct("quik")
        assert "brown" in correct("brwon")

    def test_known_words_unchanged(self):
        """Test that correct words aren't changed."""
        from hcp.physics.engine import correct

        result = correct("the quick brown fox")
        assert "quick" in result
        assert "brown" in result


class TestTokenization:
    """Test tokenization."""

    def test_byte_tokenization(self):
        """Test byte-level tokenization."""
        from hcp.atomizer.byte_atomizer import bytes_to_tokens

        tokens = bytes_to_tokens(b"AB")
        assert len(tokens) == 2
        assert tokens[0].value == 65  # A
        assert tokens[1].value == 66  # B

    def test_word_tokenization(self):
        """Test word-level tokenization."""
        from hcp.atomizer.tokenizer import tokenize

        tokens = tokenize("Hello world")
        # Should have 3 tokens: Hello, space, world
        assert len(tokens) == 3

    def test_utf8_handling(self):
        """Test UTF-8 multi-byte handling."""
        from hcp.atomizer.byte_atomizer import atomize

        # Cafe with accent (é = 2 bytes in UTF-8)
        atoms = atomize("Café")
        assert len(atoms) == 5  # C, a, f, 0xC3, 0xA9


class TestPairBondMap:
    """Test PBM functionality."""

    def test_bond_creation(self):
        """Test that bonds are created correctly."""
        from hcp.core.pair_bond import create_pbm_from_text

        pbm = create_pbm_from_text("AB")
        assert pbm.unique_bonds == 1
        assert pbm.total_bonds == 1

    def test_bond_recurrence(self):
        """Test that recurring bonds are counted."""
        from hcp.core.pair_bond import create_pbm_from_text

        pbm = create_pbm_from_text("ABAB")
        # A->B appears twice
        from hcp.core.token_id import TokenID
        bond = pbm.get_bond(TokenID.byte(65), TokenID.byte(66))
        assert bond.count == 2

    def test_serialization(self):
        """Test PBM serialization roundtrip."""
        from hcp.core.pair_bond import PairBondMap, create_pbm_from_text

        original = create_pbm_from_text("test")
        data = original.to_dict()
        restored = PairBondMap.from_dict(data)

        assert restored.unique_bonds == original.unique_bonds


class TestAbstraction:
    """Test abstraction measurement."""

    def test_primitive_detection(self):
        """Test that primitives are detected."""
        from hcp.core.nsm_primitives import is_primitive

        assert is_primitive("think")
        assert is_primitive("know")
        assert not is_primitive("happy")

    def test_abstraction_levels(self):
        """Test abstraction level calculation."""
        from hcp.core.nsm_primitives import get_abstraction_level

        assert get_abstraction_level("think") == 0  # Primitive
        assert get_abstraction_level("happy") == 2  # feel + good
        assert get_abstraction_level("xyz") == -1  # Unknown
