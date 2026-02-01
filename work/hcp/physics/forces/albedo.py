"""
Albedo Force: relevance and reflectivity.

In HCP physics, albedo represents how much a token "reflects" meaning
to its neighbors. High albedo = highly relevant, captures attention.

Used for:
- Keyword extraction
- Focus/attention simulation
- Relevance ranking
"""
from __future__ import annotations

from dataclasses import dataclass
from typing import Sequence

from ...core.token_id import TokenID
from ...core.pair_bond import PairBondMap


@dataclass
class AlbedoScore:
    """Albedo (relevance) score for a token."""
    token: TokenID
    score: float
    forward_connections: int
    backward_connections: int

    @property
    def centrality(self) -> float:
        """Combined connection centrality."""
        return (self.forward_connections + self.backward_connections) / 2


class AlbedoCalculator:
    """
    Calculate albedo (relevance) scores for tokens.

    Albedo is based on:
    1. Connection count (how many bonds)
    2. Bond strength (how strong the bonds)
    3. Position (start/end tokens often more relevant)
    """

    def __init__(
        self,
        connection_weight: float = 0.5,
        strength_weight: float = 0.3,
        position_weight: float = 0.2,
    ) -> None:
        self.connection_weight = connection_weight
        self.strength_weight = strength_weight
        self.position_weight = position_weight

    def calculate(self, pbm: PairBondMap) -> dict[TokenID, AlbedoScore]:
        """Calculate albedo scores for all tokens in a PBM."""
        scores = {}
        all_tokens = pbm.all_tokens()
        sequence = pbm.sequence()

        if not all_tokens:
            return scores

        # Calculate max connections for normalization
        max_forward = max(len(pbm.get_forward_bonds(t)) for t in all_tokens)
        max_backward = max(len(pbm.get_backward_bonds(t)) for t in all_tokens)
        max_connections = max(max_forward, max_backward, 1)

        for token in all_tokens:
            forward = pbm.get_forward_bonds(token)
            backward = pbm.get_backward_bonds(token)

            # Connection component
            connection_score = (len(forward) + len(backward)) / (2 * max_connections)

            # Strength component (average bond strength)
            strengths = []
            for t, rec in forward.items():
                strengths.append(rec.count / pbm.total_bonds if pbm.total_bonds else 0)
            for t, rec in backward.items():
                strengths.append(rec.count / pbm.total_bonds if pbm.total_bonds else 0)
            strength_score = sum(strengths) / len(strengths) if strengths else 0

            # Position component
            position_score = 0.0
            if sequence and token in sequence:
                positions = [i for i, t in enumerate(sequence) if t == token]
                for pos in positions:
                    # Higher score for start/end positions
                    rel_pos = pos / len(sequence)
                    edge_score = 1 - 2 * abs(rel_pos - 0.5)  # 0 at edges, 1 at center
                    edge_boost = 1 - edge_score  # Flip: 1 at edges, 0 at center
                    position_score = max(position_score, edge_boost * 0.5 + 0.5)

            # Combined albedo score
            albedo = (
                self.connection_weight * connection_score +
                self.strength_weight * strength_score +
                self.position_weight * position_score
            )

            scores[token] = AlbedoScore(
                token=token,
                score=albedo,
                forward_connections=len(forward),
                backward_connections=len(backward),
            )

        return scores

    def rank_by_albedo(self, pbm: PairBondMap) -> list[tuple[TokenID, float]]:
        """Rank tokens by albedo score (highest first)."""
        scores = self.calculate(pbm)
        ranked = [(t, s.score) for t, s in scores.items()]
        ranked.sort(key=lambda x: -x[1])
        return ranked

    def top_tokens(self, pbm: PairBondMap, n: int = 5) -> list[TokenID]:
        """Get top N most relevant tokens."""
        ranked = self.rank_by_albedo(pbm)
        return [t for t, _ in ranked[:n]]


def calculate_albedo(pbm: PairBondMap) -> dict[TokenID, float]:
    """Convenience function to get albedo scores."""
    calculator = AlbedoCalculator()
    scores = calculator.calculate(pbm)
    return {t: s.score for t, s in scores.items()}


def get_keywords(pbm: PairBondMap, n: int = 5) -> list[TokenID]:
    """Extract keyword tokens by albedo."""
    return AlbedoCalculator().top_tokens(pbm, n)
