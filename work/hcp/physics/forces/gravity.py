"""
Gravity Force: clustering and filtering.

In HCP physics, gravity represents attraction between semantically
related tokens. Tokens with strong bonds cluster together.

Used for:
- Topic clustering
- Semantic grouping
- Filtering irrelevant tokens
"""
from __future__ import annotations

from dataclasses import dataclass, field
from typing import Sequence

from ...core.token_id import TokenID
from ...core.pair_bond import PairBondMap


@dataclass
class Cluster:
    """A cluster of gravitationally bound tokens."""
    tokens: set[TokenID]
    center: TokenID | None = None
    mass: float = 0.0  # Sum of bond strengths
    cohesion: float = 0.0  # Internal bond density

    def __len__(self) -> int:
        return len(self.tokens)

    def add(self, token: TokenID) -> None:
        """Add a token to the cluster."""
        self.tokens.add(token)

    def merge(self, other: Cluster) -> None:
        """Merge another cluster into this one."""
        self.tokens.update(other.tokens)
        self.mass += other.mass


@dataclass
class GravityField:
    """The gravitational field of a token system."""
    clusters: list[Cluster]
    token_to_cluster: dict[TokenID, int]  # Token -> cluster index
    total_mass: float

    def get_cluster(self, token: TokenID) -> Cluster | None:
        """Get the cluster containing a token."""
        idx = self.token_to_cluster.get(token)
        if idx is not None:
            return self.clusters[idx]
        return None


class GravityCalculator:
    """
    Calculate gravitational clustering of tokens.

    Uses bond strength as gravitational mass.
    Strongly bonded tokens attract each other.
    """

    def __init__(
        self,
        attraction_threshold: float = 0.1,
        merge_threshold: float = 0.3,
    ) -> None:
        """
        Initialize gravity calculator.

        Args:
            attraction_threshold: Minimum bond strength for attraction
            merge_threshold: Minimum cohesion to merge clusters
        """
        self.attraction_threshold = attraction_threshold
        self.merge_threshold = merge_threshold

    def calculate_attraction(
        self,
        t1: TokenID,
        t2: TokenID,
        pbm: PairBondMap,
    ) -> float:
        """Calculate gravitational attraction between two tokens."""
        # Attraction based on bond strength in both directions
        forward = pbm.bond_strength(t1, t2)
        backward = pbm.bond_strength(t2, t1)
        return (forward + backward) / 2

    def cluster(self, pbm: PairBondMap) -> GravityField:
        """
        Cluster tokens by gravitational attraction.

        Uses a simple agglomerative approach.
        """
        tokens = list(pbm.all_tokens())
        if not tokens:
            return GravityField(clusters=[], token_to_cluster={}, total_mass=0.0)

        # Start with each token in its own cluster
        clusters = [Cluster(tokens={t}) for t in tokens]
        token_to_cluster = {t: i for i, t in enumerate(tokens)}

        # Calculate pairwise attractions
        attractions = []
        for i, t1 in enumerate(tokens):
            for t2 in tokens[i + 1:]:
                attr = self.calculate_attraction(t1, t2, pbm)
                if attr >= self.attraction_threshold:
                    attractions.append((attr, t1, t2))

        # Sort by attraction (highest first)
        attractions.sort(key=lambda x: -x[0])

        # Merge clusters with strong attraction
        for attr, t1, t2 in attractions:
            c1_idx = token_to_cluster[t1]
            c2_idx = token_to_cluster[t2]

            if c1_idx != c2_idx:
                # Merge smaller cluster into larger
                c1, c2 = clusters[c1_idx], clusters[c2_idx]
                if len(c1) >= len(c2):
                    c1.merge(c2)
                    for t in c2.tokens:
                        token_to_cluster[t] = c1_idx
                    c2.tokens.clear()
                else:
                    c2.merge(c1)
                    for t in c1.tokens:
                        token_to_cluster[t] = c2_idx
                    c1.tokens.clear()

        # Remove empty clusters and recalculate indices
        active_clusters = [c for c in clusters if c.tokens]
        new_mapping = {}
        for i, c in enumerate(active_clusters):
            for t in c.tokens:
                new_mapping[t] = i

        # Calculate cluster masses and cohesion
        total_mass = 0.0
        for cluster in active_clusters:
            mass = 0.0
            internal_bonds = 0
            possible_bonds = len(cluster.tokens) * (len(cluster.tokens) - 1)

            for t1 in cluster.tokens:
                for t2 in cluster.tokens:
                    if t1 != t2:
                        strength = pbm.bond_strength(t1, t2)
                        mass += strength
                        if strength > 0:
                            internal_bonds += 1

            cluster.mass = mass
            cluster.cohesion = internal_bonds / possible_bonds if possible_bonds else 0
            total_mass += mass

            # Find center (token with most internal connections)
            if cluster.tokens:
                best_center = None
                best_connections = -1
                for t in cluster.tokens:
                    connections = sum(
                        1 for other in cluster.tokens
                        if t != other and pbm.bond_strength(t, other) > 0
                    )
                    if connections > best_connections:
                        best_connections = connections
                        best_center = t
                cluster.center = best_center

        return GravityField(
            clusters=active_clusters,
            token_to_cluster=new_mapping,
            total_mass=total_mass,
        )

    def filter_by_gravity(
        self,
        pbm: PairBondMap,
        min_cluster_size: int = 2,
    ) -> list[Cluster]:
        """
        Filter out low-gravity (isolated) tokens.

        Returns only clusters above minimum size.
        """
        field = self.cluster(pbm)
        return [c for c in field.clusters if len(c) >= min_cluster_size]


class TopicDetector:
    """
    Detect topics using gravitational clustering.

    Topics are dense clusters of related tokens.
    """

    def __init__(self) -> None:
        self._gravity = GravityCalculator()

    def detect(self, pbm: PairBondMap) -> list[set[TokenID]]:
        """Detect topic clusters in PBM."""
        field = self._gravity.cluster(pbm)
        return [c.tokens for c in field.clusters if len(c) >= 2]

    def get_main_topic(self, pbm: PairBondMap) -> set[TokenID]:
        """Get the largest topic cluster."""
        topics = self.detect(pbm)
        if not topics:
            return set()
        return max(topics, key=len)


def cluster_tokens(pbm: PairBondMap) -> list[set[TokenID]]:
    """Convenience function to cluster tokens."""
    calc = GravityCalculator()
    field = calc.cluster(pbm)
    return [c.tokens for c in field.clusters]


def filter_isolated(pbm: PairBondMap) -> list[TokenID]:
    """Get tokens that aren't isolated (have connections)."""
    calc = GravityCalculator()
    clusters = calc.filter_by_gravity(pbm, min_cluster_size=2)
    result = set()
    for c in clusters:
        result.update(c.tokens)
    return list(result)
