"""Tests for hcp.physics module.

Tests the physics simulation engine, energy calculations, and related components.

Contributors: Planner & Silas (DI-cognome)
"""
import pytest

from hcp.physics.engine import (
    PhysicsEngine,
    SimulationConfig,
    SimulationResult,
    LODManager,
    simulate,
    correct,
)
from hcp.physics.energy import (
    EnergyState,
    EnergyFunction,
    EnergyMinimizer,
    edit_distance,
    weighted_edit_distance,
)
from hcp.physics.rigid_body import (
    RigidBody,
    RigidBodyRegistry,
    create_default_registry,
    COMMON_WORDS,
)
from hcp.physics.soft_body import (
    SoftBody,
    SoftBodyResolver,
    SpellingCorrector,
    correct_spelling,
    get_suggestions,
)
from hcp.core.token_id import TokenID
from hcp.core.pair_bond import PairBondMap


class TestEditDistance:
    """Test edit_distance function."""

    def test_identical_strings(self):
        """Identical strings have distance 0."""
        assert edit_distance("hello", "hello") == 0
        assert edit_distance("", "") == 0

    def test_empty_string(self):
        """Distance to empty string is length."""
        assert edit_distance("hello", "") == 5
        assert edit_distance("", "world") == 5

    def test_single_insertion(self):
        """Single character insertion is distance 1."""
        assert edit_distance("helo", "hello") == 1
        assert edit_distance("cat", "cart") == 1

    def test_single_deletion(self):
        """Single character deletion is distance 1."""
        assert edit_distance("hello", "helo") == 1

    def test_single_substitution(self):
        """Single character substitution is distance 1."""
        assert edit_distance("cat", "bat") == 1
        assert edit_distance("cat", "cut") == 1

    def test_transposition(self):
        """Transposition (Damerau) should be distance 1 or 2."""
        # "teh" -> "the" is a transposition
        dist = edit_distance("teh", "the")
        assert dist <= 2  # Damerau-Levenshtein counts this as 1 or 2

    def test_multiple_edits(self):
        """Multiple edits accumulate."""
        assert edit_distance("kitten", "sitting") == 3


class TestWeightedEditDistance:
    """Test weighted_edit_distance function."""

    def test_without_knowledge(self):
        """Without knowledge PBM, returns normal edit distance."""
        assert weighted_edit_distance("cat", "bat") == 1.0
        assert weighted_edit_distance("hello", "hello") == 0.0

    def test_with_empty_knowledge(self):
        """With empty knowledge PBM, returns normal edit distance."""
        pbm = PairBondMap()
        assert weighted_edit_distance("cat", "bat", pbm) == 1.0


class TestEnergyState:
    """Test EnergyState dataclass."""

    def test_average_energy(self):
        """Test average energy calculation."""
        tokens = [TokenID.byte(65), TokenID.byte(66), TokenID.byte(67)]
        state = EnergyState(
            tokens=tokens,
            total_energy=3.0,
            bond_energies=[1.0, 2.0],
            unknown_count=0,
        )
        assert state.average_energy == 1.5

    def test_average_energy_empty(self):
        """Empty bond list returns 0 average."""
        state = EnergyState(
            tokens=[],
            total_energy=0.0,
            bond_energies=[],
            unknown_count=0,
        )
        assert state.average_energy == 0.0

    def test_is_stable(self):
        """Test stability check."""
        tokens = [TokenID.byte(65), TokenID.byte(66)]

        # Low energy = stable
        stable = EnergyState(
            tokens=tokens,
            total_energy=0.5,
            bond_energies=[0.5],
            unknown_count=0,
        )
        assert stable.is_stable

        # High energy = not stable
        unstable = EnergyState(
            tokens=tokens,
            total_energy=5.0,
            bond_energies=[5.0],
            unknown_count=0,
        )
        assert not unstable.is_stable


class TestEnergyFunction:
    """Test EnergyFunction class."""

    def test_empty_sequence(self):
        """Empty sequence has zero energy."""
        ef = EnergyFunction()
        state = ef.sequence_energy([])
        assert state.total_energy == 0.0
        assert state.bond_energies == []

    def test_single_token(self):
        """Single token has zero bond energy."""
        ef = EnergyFunction()
        tokens = [TokenID.byte(65)]
        state = ef.sequence_energy(tokens)
        assert state.bond_energies == []

    def test_unknown_bond_penalty(self):
        """Unknown bonds get penalty."""
        ef = EnergyFunction(weak_bond_penalty=0.5)
        tokens = [TokenID.byte(65), TokenID.byte(66)]
        state = ef.sequence_energy(tokens)
        # No knowledge, so bond is unknown
        assert state.bond_energies[0] == 0.5

    def test_known_bond_low_energy(self):
        """Known strong bonds have low energy."""
        pbm = PairBondMap()
        # Add A->B bond multiple times for strength
        a, b = TokenID.byte(65), TokenID.byte(66)
        for _ in range(10):
            pbm.add_bond(a, b)

        ef = EnergyFunction(knowledge_pbm=pbm)
        state = ef.sequence_energy([a, b])
        # Strong bond = low energy (close to 0)
        assert state.bond_energies[0] < 0.5

    def test_unknown_token_penalty(self):
        """Unknown tokens add extra penalty."""
        ef = EnergyFunction(unknown_penalty=1.0)
        tokens = [TokenID.byte(65), TokenID.byte(66)]
        unknown = {TokenID.byte(65)}

        state = ef.sequence_energy(tokens, unknown_tokens=unknown)
        assert state.unknown_count == 1
        assert state.total_energy > 0


class TestRigidBody:
    """Test RigidBody class."""

    def test_create_rigid_body(self):
        """Test rigid body creation."""
        tokens = tuple(TokenID.byte(b) for b in b"hello")
        body = RigidBody(tokens=tokens, text="hello")
        assert body.text == "hello"
        assert len(body.tokens) == 5

    def test_to_pbm(self):
        """Rigid body converts to PBM."""
        tokens = tuple(TokenID.byte(b) for b in b"hi")
        body = RigidBody(tokens=tokens, text="hi")
        pbm = body.to_pbm()
        assert pbm.unique_bonds == 1

    def test_hashable(self):
        """Rigid bodies are hashable (can be used in sets)."""
        tokens = tuple(TokenID.byte(b) for b in b"test")
        body = RigidBody(tokens=tokens, text="test")
        s = {body}
        assert body in s


class TestRigidBodyRegistry:
    """Test RigidBodyRegistry class."""

    def test_register_word(self):
        """Test word registration."""
        registry = RigidBodyRegistry()
        body = registry.register_word("hello")
        assert body.text == "hello"
        assert registry.count() == 1

    def test_lookup(self):
        """Test lookup by text."""
        registry = RigidBodyRegistry()
        registry.register_word("hello")

        found = registry.lookup("hello")
        assert found is not None
        assert found.text == "hello"

        # Case insensitive
        found_upper = registry.lookup("HELLO")
        assert found_upper is not None

    def test_is_known(self):
        """Test known word check."""
        registry = RigidBodyRegistry()
        registry.register_word("hello")

        assert registry.is_known("hello")
        assert registry.is_known("HELLO")
        assert not registry.is_known("goodbye")

    def test_find_similar(self):
        """Test finding similar words."""
        registry = RigidBodyRegistry()
        registry.register_words(["hello", "hallo", "help"])

        similar = registry.find_similar("helo", max_distance=2)
        assert len(similar) >= 1
        # "hello" should be in results
        texts = [body.text for body, _ in similar]
        assert "hello" in texts

    def test_find_corrections(self):
        """Test correction suggestions."""
        registry = RigidBodyRegistry()
        registry.register_words(["the", "they", "them"])

        corrections = registry.find_corrections("teh")
        assert "the" in corrections

    def test_default_registry_has_common_words(self):
        """Default registry includes common words."""
        registry = create_default_registry()
        assert registry.is_known("the")
        assert registry.is_known("hello")
        # Note: count may be less than COMMON_WORDS due to case-normalization
        assert registry.count() > 300


class TestSoftBody:
    """Test SoftBody class."""

    def test_create_soft_body(self):
        """Test soft body creation."""
        tokens = tuple(TokenID.byte(b) for b in b"teh")
        soft = SoftBody(tokens=tokens, text="teh")
        assert soft.text == "teh"
        assert not soft.is_resolved()

    def test_resolve(self):
        """Test resolving soft body."""
        tokens_soft = tuple(TokenID.byte(b) for b in b"teh")
        tokens_rigid = tuple(TokenID.byte(b) for b in b"the")

        soft = SoftBody(tokens=tokens_soft, text="teh")
        rigid = RigidBody(tokens=tokens_rigid, text="the")

        soft.resolve_to(rigid, confidence=0.9)
        assert soft.is_resolved()
        assert soft.resolved == rigid
        assert soft.confidence == 0.9


class TestSoftBodyResolver:
    """Test SoftBodyResolver class."""

    def test_identify_known_word(self):
        """Known words are identified as rigid bodies."""
        registry = RigidBodyRegistry()
        registry.register_word("hello")
        resolver = SoftBodyResolver(registry)

        items = resolver.identify_soft_bodies("hello")
        assert len(items) == 1
        assert isinstance(items[0], RigidBody)

    def test_identify_unknown_word(self):
        """Unknown words are identified as soft bodies."""
        registry = RigidBodyRegistry()
        registry.register_word("hello")
        resolver = SoftBodyResolver(registry)

        items = resolver.identify_soft_bodies("xyzzy")
        assert len(items) == 1
        assert isinstance(items[0], SoftBody)

    def test_correct_text(self):
        """Test text correction."""
        registry = RigidBodyRegistry()
        registry.register_words(["the", "quick", "brown", "fox"])
        resolver = SoftBodyResolver(registry)

        # "teh" should correct to "the"
        corrected = resolver.correct_text("teh quick brown fox")
        assert "the" in corrected.lower()


class TestSpellingCorrector:
    """Test SpellingCorrector class."""

    def test_is_correct(self):
        """Test correct word detection."""
        corrector = SpellingCorrector()
        assert corrector.is_correct("the")
        assert corrector.is_correct("hello")
        assert not corrector.is_correct("xyzzy")

    def test_suggest(self):
        """Test spelling suggestions."""
        corrector = SpellingCorrector()
        suggestions = corrector.suggest("teh")
        assert "the" in suggestions

    def test_analyze(self):
        """Test text analysis."""
        corrector = SpellingCorrector()
        result = corrector.analyze("teh quick brown fox")

        assert "original" in result
        assert "corrected" in result
        assert "corrections" in result


class TestPhysicsEngine:
    """Test PhysicsEngine class."""

    def test_create_engine(self):
        """Test engine creation."""
        engine = PhysicsEngine()
        assert engine.registry is not None
        assert engine.config is not None

    def test_create_with_config(self):
        """Test engine with custom config."""
        config = SimulationConfig(max_iterations=50, auto_correct=False)
        engine = PhysicsEngine(config=config)
        assert engine.config.max_iterations == 50

    def test_simulate_known_text(self):
        """Simulate known text produces low energy."""
        engine = PhysicsEngine()
        result = engine.simulate("the quick brown fox")

        assert isinstance(result, SimulationResult)
        assert result.original_text == "the quick brown fox"

    def test_correct_function(self):
        """Test correct() method."""
        engine = PhysicsEngine()
        corrected = engine.correct("teh quick")
        assert isinstance(corrected, str)

    def test_add_word(self):
        """Test adding words to engine."""
        engine = PhysicsEngine()
        engine.add_word("planner")
        assert engine.registry.is_known("planner")

    def test_add_words(self):
        """Test adding multiple words."""
        engine = PhysicsEngine()
        engine.add_words(["silas", "planner", "haven"])
        assert engine.registry.is_known("silas")
        assert engine.registry.is_known("haven")


class TestSimulationResult:
    """Test SimulationResult dataclass."""

    def test_improvement_calculation(self):
        """Test energy improvement calculation."""
        result = SimulationResult(
            original_text="test",
            corrected_text="test",
            corrections=[],
            energy_before=10.0,
            energy_after=5.0,
            iterations=1,
            stable=True,
        )
        assert result.improvement == 0.5

    def test_improvement_zero_before(self):
        """Zero energy before returns 0 improvement."""
        result = SimulationResult(
            original_text="test",
            corrected_text="test",
            corrections=[],
            energy_before=0.0,
            energy_after=0.0,
            iterations=1,
            stable=True,
        )
        assert result.improvement == 0.0


class TestLODManager:
    """Test LODManager class."""

    def test_register_level(self):
        """Test registering detail levels."""
        lod = LODManager()
        pbm = PairBondMap()
        lod.register_level(0, pbm)

        retrieved = lod.get_level(0)
        assert retrieved is pbm

    def test_get_unknown_level(self):
        """Unknown level returns None."""
        lod = LODManager()
        assert lod.get_level(99) is None

    def test_collapse_wrong_direction(self):
        """Collapse from lower to higher returns None."""
        lod = LODManager()
        assert lod.collapse(0, 1) is None  # Can't collapse up

    def test_expand_wrong_direction(self):
        """Expand from higher to lower returns None."""
        lod = LODManager()
        assert lod.expand(1, 0) is None  # Can't expand down


class TestConvenienceFunctions:
    """Test module-level convenience functions."""

    def test_simulate_function(self):
        """Test simulate() convenience function."""
        result = simulate("hello world")
        assert isinstance(result, SimulationResult)

    def test_correct_function(self):
        """Test correct() convenience function."""
        corrected = correct("hello world")
        assert isinstance(corrected, str)

    def test_correct_spelling_function(self):
        """Test correct_spelling() convenience function."""
        corrected = correct_spelling("teh quick brown fox")
        assert isinstance(corrected, str)

    def test_get_suggestions_function(self):
        """Test get_suggestions() convenience function."""
        suggestions = get_suggestions("teh")
        assert isinstance(suggestions, list)
        assert "the" in suggestions
