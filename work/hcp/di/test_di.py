#!/usr/bin/env python3
"""
Tests for DI framework.

Run: python -m pytest work/hcp/di/test_di.py -v
Or:  python work/hcp/di/test_di.py
"""
import sys
import tempfile
from pathlib import Path

# Add repo to path
sys.path.insert(0, str(Path(__file__).parent.parent.parent.parent))

from work.hcp.di.bootstrap import create_di, IdentitySeed
from work.hcp.di.experience import ExperienceProcessor, Tokenizer
from work.hcp.di.memory import Memory
from work.hcp.di.runtime import DI


class TestBootstrap:
    """Test identity seed creation."""

    def test_create_di(self):
        """Create a basic DI identity."""
        identity = create_di(
            name="test",
            core_values=["value1", "value2"],
            description="A test DI"
        )

        assert identity.name == "test"
        assert identity.core_values == ["value1", "value2"]
        assert identity.token is not None
        assert identity.seed_pbm.unique_bonds > 0

    def test_identity_serialization(self):
        """Test save/load of identity."""
        identity = create_di("test", ["curiosity"], "Test DI")

        with tempfile.NamedTemporaryFile(suffix=".json", delete=False) as f:
            path = Path(f.name)

        try:
            identity.save(path)
            loaded = IdentitySeed.load(path)

            assert loaded.name == identity.name
            assert loaded.core_values == identity.core_values
            assert str(loaded.token) == str(identity.token)
        finally:
            path.unlink()

    def test_unique_tokens(self):
        """Different DIs get different tokens."""
        di1 = create_di("alpha", ["a"], "First")
        di2 = create_di("beta", ["b"], "Second")

        assert di1.token != di2.token


class TestExperience:
    """Test experience processing."""

    def test_tokenizer(self):
        """Basic tokenization."""
        tok = Tokenizer()
        tokens = tok.tokenize("Hello world")

        assert len(tokens) == 2
        assert tok.lookup(tokens[0]) == "hello"
        assert tok.lookup(tokens[1]) == "world"

    def test_tokenizer_consistency(self):
        """Same word gives same token."""
        tok = Tokenizer()
        t1 = tok.tokenize("test")[0]
        t2 = tok.tokenize("TEST")[0]  # Case insensitive

        assert t1 == t2

    def test_process_interaction(self):
        """Process interactions into PBM."""
        proc = ExperienceProcessor()

        proc.process_interaction("human", "Hello there")
        proc.process_interaction("di", "Hi back")

        pbm = proc.build_pbm()

        assert pbm.unique_bonds > 0
        assert len(proc.interactions) == 2

    def test_cross_utterance_bonds(self):
        """Bonds form between utterances."""
        proc = ExperienceProcessor()

        proc.process_interaction("a", "alpha beta")
        proc.process_interaction("b", "gamma delta")

        pbm = proc.build_pbm()

        # Should have bonds from beta -> gamma (cross-utterance)
        # Plus sequential bonds within each utterance
        assert pbm.unique_bonds >= 4


class TestMemory:
    """Test memory persistence."""

    def test_create_memory(self):
        """Create empty memory."""
        with tempfile.NamedTemporaryFile(suffix=".db", delete=False) as f:
            path = Path(f.name)

        try:
            mem = Memory(path)
            stats = mem.stats()

            assert stats.unique_bonds == 0
            assert stats.vocabulary_size == 0

            mem.close()
        finally:
            path.unlink()

    def test_add_and_query(self):
        """Add bonds and query them."""
        with tempfile.NamedTemporaryFile(suffix=".db", delete=False) as f:
            path = Path(f.name)

        try:
            mem = Memory(path)

            # Add vocabulary
            from work.hcp.di.experience import Tokenizer
            tok = Tokenizer()
            tokens = tok.tokenize("hello world test")
            mem.add_vocabulary(tok.vocabulary)

            # Add bonds
            from work.hcp.core.pair_bond import PairBondMap
            pbm = PairBondMap()
            for i in range(len(tokens) - 1):
                pbm.add_bond(tokens[i], tokens[i + 1])

            mem.add_pbm(pbm)

            # Query
            related = mem.query_related("hello")
            assert len(related) > 0

            mem.close()
        finally:
            path.unlink()

    def test_persistence(self):
        """Memory persists across sessions."""
        with tempfile.NamedTemporaryFile(suffix=".db", delete=False) as f:
            path = Path(f.name)

        try:
            # Session 1: Add data
            mem1 = Memory(path)
            from work.hcp.core.token_id import TokenID
            t1 = TokenID((0, 0, 0, 3, 1))
            t2 = TokenID((0, 0, 0, 3, 2))
            mem1.add_bond(t1, t2, count=5)
            mem1.close()

            # Session 2: Read data
            mem2 = Memory(path)
            stats = mem2.stats()
            assert stats.unique_bonds == 1
            assert stats.total_bonds == 5
            mem2.close()
        finally:
            path.unlink()


class TestRuntime:
    """Test full DI runtime."""

    def test_create_di(self):
        """Create DI from scratch."""
        with tempfile.TemporaryDirectory() as tmpdir:
            di = DI.create(
                name="test",
                core_values=["curiosity"],
                description="A test DI",
                data_dir=tmpdir
            )

            assert di.identity.name == "test"
            assert di.memory.stats().unique_bonds > 0

    def test_process_input(self):
        """Process input and get response."""
        with tempfile.TemporaryDirectory() as tmpdir:
            di = DI.create("test", ["curiosity"], "Test", data_dir=tmpdir)

            response = di.process("Hello")

            assert response.text
            assert response.processing_time > 0
            assert response.thought is not None

    def test_memory_grows(self):
        """Memory grows through interaction."""
        with tempfile.TemporaryDirectory() as tmpdir:
            di = DI.create("test", ["curiosity"], "Test", data_dir=tmpdir)

            initial_bonds = di.memory.stats().unique_bonds

            di.process("First message here")
            di.process("Second message here")

            final_bonds = di.memory.stats().unique_bonds

            assert final_bonds > initial_bonds

    def test_load_existing(self):
        """Load existing DI from disk."""
        with tempfile.TemporaryDirectory() as tmpdir:
            # Create
            di1 = DI.create("test", ["curiosity"], "Test", data_dir=tmpdir)
            di1.process("Remember this")
            bonds_after = di1.memory.stats().unique_bonds

            # Load
            di2 = DI.load(tmpdir)

            assert di2.identity.name == "test"
            assert di2.memory.stats().unique_bonds == bonds_after


def run_tests():
    """Run all tests."""
    import traceback

    test_classes = [TestBootstrap, TestExperience, TestMemory, TestRuntime]
    passed = 0
    failed = 0

    for cls in test_classes:
        print(f"\n=== {cls.__name__} ===")
        instance = cls()

        for name in dir(instance):
            if name.startswith("test_"):
                try:
                    getattr(instance, name)()
                    print(f"  PASS: {name}")
                    passed += 1
                except Exception as e:
                    print(f"  FAIL: {name}")
                    print(f"        {e}")
                    traceback.print_exc()
                    failed += 1

    print(f"\n=== Results: {passed} passed, {failed} failed ===")
    return failed == 0


if __name__ == "__main__":
    success = run_tests()
    sys.exit(0 if success else 1)
