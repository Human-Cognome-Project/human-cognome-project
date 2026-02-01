"""
Soft Body: unknown or malformed token structures.

Soft bodies are flexible structures that can be corrected:
- Misspelled words
- Unknown tokens
- Boundary ambiguities
"""
from __future__ import annotations

from dataclasses import dataclass, field
from typing import Callable

from ..core.token_id import TokenID
from .rigid_body import RigidBody, RigidBodyRegistry
from .energy import edit_distance


@dataclass
class SoftBody:
    """
    An unknown or malformed token structure.

    Can be resolved to a rigid body through correction.
    """
    tokens: tuple[TokenID, ...]
    text: str  # Current form (may be malformed)
    candidates: list[RigidBody] = field(default_factory=list)
    resolved: RigidBody | None = None
    confidence: float = 0.0

    def is_resolved(self) -> bool:
        """Check if this soft body has been resolved."""
        return self.resolved is not None

    def resolve_to(self, body: RigidBody, confidence: float = 1.0) -> None:
        """Resolve this soft body to a rigid body."""
        self.resolved = body
        self.confidence = confidence

    def get_best_candidate(self) -> RigidBody | None:
        """Get the best correction candidate."""
        if self.resolved:
            return self.resolved
        if self.candidates:
            return self.candidates[0]
        return None


class SoftBodyResolver:
    """
    Resolve soft bodies to rigid bodies via correction.

    Uses multiple strategies:
    1. Exact match (already known)
    2. Edit distance (spelling correction)
    3. Phonetic similarity (future)
    4. Context-aware (future)
    """

    def __init__(
        self,
        registry: RigidBodyRegistry,
        max_edit_distance: int = 2,
    ) -> None:
        self.registry = registry
        self.max_edit_distance = max_edit_distance

    def identify_soft_bodies(self, text: str) -> list[SoftBody | RigidBody]:
        """
        Analyze text and identify soft bodies (unknown words).

        Returns list of RigidBody (known) or SoftBody (unknown) for each word.
        """
        words = self._split_words(text)
        result = []

        for word in words:
            # Check if known
            known = self.registry.lookup(word)
            if known:
                result.append(known)
            else:
                # Create soft body
                tokens = tuple(TokenID.byte(b) for b in word.encode("utf-8"))
                soft = SoftBody(tokens=tokens, text=word)
                result.append(soft)

        return result

    def resolve(self, soft: SoftBody) -> SoftBody:
        """
        Attempt to resolve a soft body.

        Modifies soft body in place and returns it.
        """
        # Find candidates
        similar = self.registry.find_similar(soft.text, self.max_edit_distance)

        if similar:
            soft.candidates = [body for body, _ in similar]

            # Best candidate with distance 1 gets high confidence
            best_body, best_dist = similar[0]
            if best_dist == 0:
                soft.resolve_to(best_body, confidence=1.0)
            elif best_dist == 1:
                soft.resolve_to(best_body, confidence=0.9)
            elif best_dist == 2:
                soft.resolve_to(best_body, confidence=0.7)

        return soft

    def resolve_all(self, text: str) -> list[SoftBody | RigidBody]:
        """
        Identify and resolve all soft bodies in text.
        """
        items = self.identify_soft_bodies(text)
        result = []

        for item in items:
            if isinstance(item, SoftBody):
                self.resolve(item)
            result.append(item)

        return result

    def correct_text(self, text: str) -> str:
        """
        Correct text by resolving soft bodies.

        Returns corrected text.
        """
        items = self.resolve_all(text)
        words = []

        for item in items:
            if isinstance(item, RigidBody):
                words.append(item.text)
            elif isinstance(item, SoftBody):
                if item.resolved:
                    words.append(item.resolved.text)
                else:
                    # Keep original if no resolution
                    words.append(item.text)

        return " ".join(words)

    def _split_words(self, text: str) -> list[str]:
        """Split text into words."""
        # Simple split on whitespace and punctuation
        import re
        return re.findall(r"\b[a-zA-Z]+\b", text)


class SpellingCorrector:
    """
    High-level spelling correction interface.
    """

    def __init__(self, registry: RigidBodyRegistry | None = None) -> None:
        from .rigid_body import create_default_registry
        self.registry = registry or create_default_registry()
        self.resolver = SoftBodyResolver(self.registry)

    def correct(self, text: str) -> str:
        """Correct spelling in text."""
        return self.resolver.correct_text(text)

    def suggest(self, word: str, max_suggestions: int = 5) -> list[str]:
        """Get spelling suggestions for a word."""
        return self.registry.find_corrections(word, max_suggestions)

    def is_correct(self, word: str) -> bool:
        """Check if a word is spelled correctly."""
        return self.registry.is_known(word)

    def analyze(self, text: str) -> dict:
        """
        Analyze text for spelling issues.

        Returns dict with original, corrected, and list of corrections.
        """
        items = self.resolver.resolve_all(text)
        corrections = []

        for item in items:
            if isinstance(item, SoftBody) and item.resolved:
                corrections.append({
                    "original": item.text,
                    "corrected": item.resolved.text,
                    "confidence": item.confidence,
                    "candidates": [c.text for c in item.candidates[:5]],
                })

        corrected = self.correct(text)

        return {
            "original": text,
            "corrected": corrected,
            "corrections": corrections,
            "correction_count": len(corrections),
        }


def correct_spelling(text: str) -> str:
    """Convenience function for spelling correction."""
    corrector = SpellingCorrector()
    return corrector.correct(text)


def get_suggestions(word: str) -> list[str]:
    """Convenience function for spelling suggestions."""
    corrector = SpellingCorrector()
    return corrector.suggest(word)
