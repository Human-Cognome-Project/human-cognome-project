"""
Conceptual Decomposer: break tokens down to NSM primitives.

Provides paths from any token to its grounding in semantic primitives.
"""
from __future__ import annotations

from dataclasses import dataclass, field
from typing import Sequence

from ..core.token_id import TokenID
from ..core.nsm_primitives import (
    NSMPrimitive,
    decompose_word,
    get_primitive,
    is_primitive,
    get_abstraction_level,
    all_primitives,
)


@dataclass
class DecompositionNode:
    """A node in a decomposition tree."""
    word: str
    primitives: list[NSMPrimitive]
    children: list[DecompositionNode] = field(default_factory=list)
    level: int = 0

    @property
    def is_primitive(self) -> bool:
        """Check if this node is a primitive (leaf)."""
        return len(self.primitives) == 1 and is_primitive(self.word)

    @property
    def is_decomposed(self) -> bool:
        """Check if this node has a decomposition."""
        return len(self.primitives) > 0

    def to_dict(self) -> dict:
        """Convert to dictionary for serialization."""
        return {
            "word": self.word,
            "primitives": [str(p) for p in self.primitives],
            "level": self.level,
            "children": [c.to_dict() for c in self.children],
        }


@dataclass
class DecompositionResult:
    """Result of decomposing a text into NSM primitives."""
    original: str
    nodes: list[DecompositionNode]
    total_primitives: int
    max_depth: int
    coverage: float  # Percentage of words with decompositions

    def to_dict(self) -> dict:
        """Convert to dictionary."""
        return {
            "original": self.original,
            "nodes": [n.to_dict() for n in self.nodes],
            "total_primitives": self.total_primitives,
            "max_depth": self.max_depth,
            "coverage": self.coverage,
        }


class Decomposer:
    """
    Decompose words and texts into NSM semantic primitives.
    """

    def __init__(self) -> None:
        self._cache: dict[str, list[NSMPrimitive] | None] = {}

    def decompose_word(self, word: str) -> DecompositionNode:
        """Decompose a single word into primitives."""
        word_lower = word.lower()

        # Check cache
        if word_lower not in self._cache:
            self._cache[word_lower] = decompose_word(word_lower)

        primitives = self._cache[word_lower] or []
        level = get_abstraction_level(word_lower)

        return DecompositionNode(
            word=word,
            primitives=primitives,
            level=level if level >= 0 else -1,
        )

    def decompose_text(self, text: str) -> DecompositionResult:
        """Decompose a text into NSM primitives."""
        import re
        words = re.findall(r"\b[a-zA-Z]+\b", text)

        nodes = []
        total_primitives = 0
        max_depth = 0
        decomposed_count = 0

        for word in words:
            node = self.decompose_word(word)
            nodes.append(node)

            if node.primitives:
                total_primitives += len(node.primitives)
                max_depth = max(max_depth, node.level)
                decomposed_count += 1

        coverage = decomposed_count / len(words) if words else 0.0

        return DecompositionResult(
            original=text,
            nodes=nodes,
            total_primitives=total_primitives,
            max_depth=max_depth,
            coverage=coverage,
        )

    def get_primitive_path(self, word: str) -> list[str]:
        """
        Get the path from word to primitives as strings.

        Returns list like: ["happy", "->", "feel", "good"]
        """
        node = self.decompose_word(word)
        if not node.primitives:
            return [word, "->", "?"]

        result = [word, "->"]
        result.extend(str(p) for p in node.primitives)
        return result

    def visualize_decomposition(self, word: str) -> str:
        """Create a visual representation of a word's decomposition."""
        node = self.decompose_word(word)

        if not node.primitives:
            return f"{word}: [no decomposition]"

        prims = " + ".join(str(p) for p in node.primitives)
        return f"{word} (level {node.level}): {prims}"

    def visualize_text(self, text: str) -> str:
        """Visualize decomposition of entire text."""
        result = self.decompose_text(text)

        lines = [f"Text: {result.original}", ""]
        for node in result.nodes:
            lines.append(self.visualize_decomposition(node.word))

        lines.append("")
        lines.append(f"Coverage: {result.coverage:.0%}")
        lines.append(f"Total primitives: {result.total_primitives}")
        lines.append(f"Max abstraction level: {result.max_depth}")

        return "\n".join(lines)


@dataclass
class AbstractionMeter:
    """
    Measure abstraction levels in text.

    Higher abstraction = more primitives needed to express meaning.
    """

    def measure(self, text: str) -> dict:
        """Measure abstraction metrics for text."""
        decomposer = Decomposer()
        result = decomposer.decompose_text(text)

        # Count words at each level
        level_counts: dict[int, int] = {}
        for node in result.nodes:
            level = node.level
            level_counts[level] = level_counts.get(level, 0) + 1

        # Calculate average abstraction
        total_level = sum(
            node.level for node in result.nodes
            if node.level >= 0
        )
        measured_count = sum(1 for n in result.nodes if n.level >= 0)
        avg_abstraction = total_level / measured_count if measured_count else 0

        return {
            "word_count": len(result.nodes),
            "coverage": result.coverage,
            "average_abstraction": avg_abstraction,
            "max_abstraction": result.max_depth,
            "level_distribution": level_counts,
            "primitive_count": result.total_primitives,
        }


def decompose(word: str) -> list[NSMPrimitive] | None:
    """Convenience function to decompose a word."""
    return decompose_word(word)


def visualize(text: str) -> str:
    """Convenience function to visualize decomposition."""
    return Decomposer().visualize_text(text)
