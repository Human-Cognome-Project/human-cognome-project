"""
Physics Engine: coordinate simulation of token dynamics.

The engine manages:
- Rigid bodies (known structures)
- Soft bodies (unknown/malformed)
- Energy minimization (error correction)
- Level of Detail stacking
"""
from __future__ import annotations

from dataclasses import dataclass, field
from typing import Callable, Sequence

from ..core.token_id import TokenID
from ..core.pair_bond import PairBondMap
from .rigid_body import RigidBody, RigidBodyRegistry, create_default_registry
from .soft_body import SoftBody, SoftBodyResolver, SpellingCorrector
from .energy import EnergyFunction, EnergyMinimizer, EnergyState


@dataclass
class SimulationConfig:
    """Configuration for physics simulation."""
    max_iterations: int = 100
    energy_threshold: float = 0.1
    auto_correct: bool = True
    max_edit_distance: int = 2


@dataclass
class SimulationResult:
    """Result of physics simulation."""
    original_text: str
    corrected_text: str
    corrections: list[dict]
    energy_before: float
    energy_after: float
    iterations: int
    stable: bool

    @property
    def improvement(self) -> float:
        """Energy improvement ratio."""
        if self.energy_before == 0:
            return 0.0
        return (self.energy_before - self.energy_after) / self.energy_before


class PhysicsEngine:
    """
    Main physics simulation engine.

    Coordinates correction of malformed input through energy minimization.
    """

    def __init__(
        self,
        registry: RigidBodyRegistry | None = None,
        config: SimulationConfig | None = None,
    ) -> None:
        self.registry = registry or create_default_registry()
        self.config = config or SimulationConfig()
        self.resolver = SoftBodyResolver(
            self.registry,
            max_edit_distance=self.config.max_edit_distance,
        )
        self.corrector = SpellingCorrector(self.registry)

        # Build knowledge PBM from registry
        self._knowledge = self._build_knowledge_pbm()
        self._energy_fn = EnergyFunction(self._knowledge)

    def _build_knowledge_pbm(self) -> PairBondMap:
        """Build PBM from all known rigid bodies."""
        pbm = PairBondMap()
        for body in self.registry.all_bodies():
            body_pbm = body.to_pbm()
            pbm.merge(body_pbm)
        return pbm

    def simulate(self, text: str) -> SimulationResult:
        """
        Run physics simulation on text.

        Identifies soft bodies and resolves them via energy minimization.
        """
        # Analyze input
        analysis = self.corrector.analyze(text)

        # Calculate initial energy
        initial_tokens = self._text_to_byte_tokens(text)
        initial_state = self._energy_fn.sequence_energy(initial_tokens)

        # Correct text
        corrected = analysis["corrected"]

        # Calculate final energy
        final_tokens = self._text_to_byte_tokens(corrected)
        final_state = self._energy_fn.sequence_energy(final_tokens)

        return SimulationResult(
            original_text=text,
            corrected_text=corrected,
            corrections=analysis["corrections"],
            energy_before=initial_state.total_energy,
            energy_after=final_state.total_energy,
            iterations=1,  # Single pass for now
            stable=final_state.is_stable,
        )

    def correct(self, text: str) -> str:
        """Simple interface: correct text and return result."""
        result = self.simulate(text)
        return result.corrected_text

    def _text_to_byte_tokens(self, text: str) -> list[TokenID]:
        """Convert text to byte-level tokens."""
        return [TokenID.byte(b) for b in text.encode("utf-8")]

    def add_word(self, word: str, frequency: int = 1) -> None:
        """Add a word to the known dictionary."""
        self.registry.register_word(word, frequency)

    def add_words(self, words: list[str]) -> None:
        """Add multiple words to the known dictionary."""
        self.registry.register_words(words)


class LODManager:
    """
    Level of Detail manager for token abstractions.

    Manages transitions between detail levels:
    - Level 0: Bytes
    - Level 1: Glyphs (Unicode characters)
    - Level 2: Words
    - Level 3: Phrases
    - ...
    """

    def __init__(self) -> None:
        self.levels: dict[int, PairBondMap] = {}

    def register_level(self, level: int, pbm: PairBondMap) -> None:
        """Register PBM for a specific detail level."""
        self.levels[level] = pbm

    def get_level(self, level: int) -> PairBondMap | None:
        """Get PBM for a specific detail level."""
        return self.levels.get(level)

    def collapse(self, from_level: int, to_level: int) -> PairBondMap | None:
        """
        Collapse from higher to lower detail level.

        E.g., collapse bytes to words.
        """
        if from_level >= to_level:
            return None

        # Future: implement actual collapse logic
        return self.get_level(to_level)

    def expand(self, from_level: int, to_level: int) -> PairBondMap | None:
        """
        Expand from lower to higher detail level.

        E.g., expand words to bytes.
        """
        if from_level <= to_level:
            return None

        # Future: implement actual expansion logic
        return self.get_level(to_level)


def simulate(text: str) -> SimulationResult:
    """Convenience function for physics simulation."""
    engine = PhysicsEngine()
    return engine.simulate(text)


def correct(text: str) -> str:
    """Convenience function for text correction."""
    engine = PhysicsEngine()
    return engine.correct(text)
