"""
Abstraction Meter: count and analyze abstraction layers.

Measures how many layers of semantic abstraction separate
surface expressions from their primitive groundings.
"""
from __future__ import annotations

from dataclasses import dataclass
from typing import Sequence

from .decomposer import Decomposer, DecompositionResult


@dataclass
class AbstractionMetrics:
    """Metrics about abstraction in a text."""
    word_count: int
    covered_words: int
    coverage: float
    total_primitives: int
    average_level: float
    max_level: int
    level_histogram: dict[int, int]
    complexity_score: float  # Higher = more abstract

    def __str__(self) -> str:
        return (
            f"AbstractionMetrics:\n"
            f"  Words: {self.word_count} ({self.coverage:.0%} covered)\n"
            f"  Primitives: {self.total_primitives}\n"
            f"  Avg level: {self.average_level:.2f}\n"
            f"  Max level: {self.max_level}\n"
            f"  Complexity: {self.complexity_score:.2f}"
        )


class AbstractionMeter:
    """
    Measure abstraction depth in text.

    Abstraction levels:
    - Level 0: NSM primitive (most concrete)
    - Level 1: Maps to single primitive
    - Level 2+: Requires multiple primitives
    - Level -1: Unknown (no decomposition)
    """

    def __init__(self) -> None:
        self._decomposer = Decomposer()

    def measure(self, text: str) -> AbstractionMetrics:
        """Compute abstraction metrics for text."""
        result = self._decomposer.decompose_text(text)

        # Build histogram of levels
        histogram: dict[int, int] = {}
        total_level = 0
        covered = 0

        for node in result.nodes:
            level = node.level
            histogram[level] = histogram.get(level, 0) + 1
            if level >= 0:
                total_level += level
                covered += 1

        # Average level (excluding unknown)
        avg_level = total_level / covered if covered else 0.0

        # Complexity score: weighted sum favoring higher abstraction
        complexity = sum(
            level * count
            for level, count in histogram.items()
            if level > 0
        ) / len(result.nodes) if result.nodes else 0.0

        return AbstractionMetrics(
            word_count=len(result.nodes),
            covered_words=covered,
            coverage=result.coverage,
            total_primitives=result.total_primitives,
            average_level=avg_level,
            max_level=result.max_depth,
            level_histogram=histogram,
            complexity_score=complexity,
        )

    def compare(self, text1: str, text2: str) -> dict:
        """Compare abstraction levels of two texts."""
        m1 = self.measure(text1)
        m2 = self.measure(text2)

        return {
            "text1": {
                "text": text1[:50] + "..." if len(text1) > 50 else text1,
                "metrics": m1,
            },
            "text2": {
                "text": text2[:50] + "..." if len(text2) > 50 else text2,
                "metrics": m2,
            },
            "comparison": {
                "coverage_diff": m1.coverage - m2.coverage,
                "avg_level_diff": m1.average_level - m2.average_level,
                "complexity_diff": m1.complexity_score - m2.complexity_score,
                "more_abstract": "text1" if m1.complexity_score > m2.complexity_score else "text2",
            },
        }

    def simplify_suggestions(self, text: str) -> list[dict]:
        """
        Suggest simpler words for high-abstraction terms.

        Returns list of suggestions for words that could be simplified.
        """
        result = self._decomposer.decompose_text(text)
        suggestions = []

        for node in result.nodes:
            if node.level > 2:  # High abstraction
                # Suggest using the primitives directly
                if node.primitives:
                    suggestions.append({
                        "word": node.word,
                        "level": node.level,
                        "primitives": [str(p) for p in node.primitives],
                        "suggestion": f"Consider simpler words: {', '.join(str(p) for p in node.primitives[:3])}",
                    })

        return suggestions


def measure_abstraction(text: str) -> AbstractionMetrics:
    """Convenience function to measure abstraction."""
    return AbstractionMeter().measure(text)


def get_complexity(text: str) -> float:
    """Get complexity score for text."""
    return AbstractionMeter().measure(text).complexity_score
