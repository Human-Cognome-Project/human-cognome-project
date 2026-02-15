"""
Context retrieval through bond traversal and activation spreading.

This module implements the "what's relevant" question for cognition:
- Given a query, which bonds/tokens are most relevant?
- Uses physics-inspired activation spreading through bond network
- Supports identity-based filtering (what matters to this agent)
"""
from __future__ import annotations

from dataclasses import dataclass, field
from typing import Sequence, Iterator
from collections import defaultdict
import math

from ..core.token_id import TokenID
from ..core.pair_bond import PairBondMap, PairBond, BondRecurrence


@dataclass
class ActivatedBond:
    """A bond with activation level (salience score)."""
    bond: PairBond
    recurrence: BondRecurrence
    activation: float  # 0.0 to 1.0, higher = more relevant
    path_length: int   # hops from query tokens

    def __str__(self) -> str:
        return f"{self.bond} (act={self.activation:.3f}, depth={self.path_length})"


@dataclass
class ContextResult:
    """Result of context retrieval."""
    query_tokens: list[TokenID]
    activated_bonds: list[ActivatedBond]
    total_activation: float
    max_depth: int

    def top_bonds(self, n: int = 10) -> list[ActivatedBond]:
        """Get top N bonds by activation."""
        return sorted(self.activated_bonds, key=lambda b: -b.activation)[:n]

    def bonds_above_threshold(self, threshold: float = 0.1) -> list[ActivatedBond]:
        """Get bonds with activation above threshold."""
        return [b for b in self.activated_bonds if b.activation >= threshold]


class ActivationSpreader:
    """
    Spread activation through bond network from seed tokens.

    Inspired by spreading activation in semantic networks:
    - Activation starts at query tokens
    - Spreads through bonds proportional to bond strength
    - Decays with distance (path length)
    - Accumulates at highly-connected nodes
    """

    def __init__(
        self,
        pbm: PairBondMap,
        decay_factor: float = 0.7,
        min_activation: float = 0.01,
        max_depth: int = 3,
        max_tokens: int = 500,
        max_degree: int = 100,
    ) -> None:
        """
        Initialize activation spreader.

        Args:
            pbm: The pair bond map to traverse
            decay_factor: Activation multiplier per hop (0-1)
            min_activation: Stop spreading below this level
            max_depth: Maximum traversal depth
            max_tokens: Maximum tokens to activate (prevents explosion)
            max_degree: Skip high-degree nodes (hub avoidance)
        """
        self.pbm = pbm
        self.decay_factor = decay_factor
        self.min_activation = min_activation
        self.max_depth = max_depth
        self.max_tokens = max_tokens
        self.max_degree = max_degree

    def spread(self, seed_tokens: Sequence[TokenID]) -> dict[TokenID, float]:
        """
        Spread activation from seed tokens through bond network.

        Returns dict mapping tokens to their final activation levels.

        OPTIMIZED: Limits total tokens, skips high-degree hubs.
        """
        # Initialize activation at seed tokens
        activation: dict[TokenID, float] = defaultdict(float)
        for token in seed_tokens:
            activation[token] = 1.0

        # Track tokens to process at each depth
        current_wave = set(seed_tokens)

        for depth in range(self.max_depth):
            if not current_wave:
                break

            # Early termination if we have enough tokens
            if len(activation) >= self.max_tokens:
                break

            next_wave: set[TokenID] = set()
            wave_decay = self.decay_factor ** (depth + 1)

            for token in current_wave:
                token_activation = activation[token]

                # Spread forward (token -> what follows)
                forward_bonds = self.pbm.get_forward_bonds(token)

                # Skip high-degree hubs (common words like "the", "and")
                if len(forward_bonds) > self.max_degree:
                    continue

                for next_token, recurrence in forward_bonds.items():
                    strength = self.pbm.bond_strength(token, next_token)
                    spread_amount = token_activation * strength * wave_decay

                    if spread_amount >= self.min_activation:
                        activation[next_token] += spread_amount
                        next_wave.add(next_token)

                # Spread backward (what precedes -> token)
                backward_bonds = self.pbm.get_backward_bonds(token)

                if len(backward_bonds) > self.max_degree:
                    continue

                for prev_token, recurrence in backward_bonds.items():
                    strength = self.pbm.bond_strength(prev_token, token)
                    spread_amount = token_activation * strength * wave_decay

                    if spread_amount >= self.min_activation:
                        activation[prev_token] += spread_amount
                        next_wave.add(prev_token)

            current_wave = next_wave

        # Normalize activations
        if activation:
            max_act = max(activation.values())
            if max_act > 0:
                for token in activation:
                    activation[token] /= max_act

        return dict(activation)

    def get_activated_bonds(
        self,
        seed_tokens: Sequence[TokenID],
        max_bonds: int = 100,
    ) -> list[ActivatedBond]:
        """
        Get bonds sorted by activation level.

        Activation of a bond = geometric mean of endpoint activations.

        OPTIMIZED: Only checks bonds connected to activated tokens,
        not all bonds in the PBM.
        """
        token_activation = self.spread(seed_tokens)
        seed_set = set(seed_tokens)

        # Only process tokens with activation (not all tokens)
        activated_tokens = set(token_activation.keys())

        activated_bonds = []
        seen_bonds: set[tuple] = set()

        # Iterate only over activated tokens' bonds
        for token in activated_tokens:
            token_act = token_activation[token]

            # Forward bonds from this token
            for right_token, recurrence in self.pbm.get_forward_bonds(token).items():
                bond_key = (token, right_token)
                if bond_key in seen_bonds:
                    continue
                seen_bonds.add(bond_key)

                right_act = token_activation.get(right_token, 0.0)
                if token_act > 0 or right_act > 0:
                    bond_activation = (
                        math.sqrt(token_act * right_act)
                        if token_act > 0 and right_act > 0
                        else max(token_act, right_act) * 0.5
                    )

                    activated_bonds.append(ActivatedBond(
                        bond=recurrence.bond,
                        recurrence=recurrence,
                        activation=bond_activation,
                        path_length=self._estimate_path_length(recurrence.bond, seed_set, token_activation),
                    ))

        # Sort and limit
        activated_bonds.sort(key=lambda b: -b.activation)
        return activated_bonds[:max_bonds]

    def _estimate_path_length(
        self,
        bond: PairBond,
        seeds: set[TokenID],
        activations: dict[TokenID, float],
    ) -> int:
        """Estimate path length from seeds based on activation decay."""
        if bond.left in seeds or bond.right in seeds:
            return 0

        # Estimate from activation level (activation ~= decay^depth)
        max_act = max(activations.get(bond.left, 0), activations.get(bond.right, 0))
        if max_act <= 0:
            return self.max_depth

        # Invert: depth ~= log(activation) / log(decay)
        if self.decay_factor > 0 and self.decay_factor < 1:
            estimated_depth = int(-math.log(max_act) / math.log(self.decay_factor))
            return min(estimated_depth, self.max_depth)

        return 1


class IdentityFilter:
    """
    Filter context by identity-specific weights.

    An identity seed biases which bonds are considered relevant
    based on the agent's "interests" encoded as token weights.
    """

    def __init__(self, identity_weights: dict[TokenID, float] | None = None) -> None:
        """
        Initialize identity filter.

        Args:
            identity_weights: Token -> weight mapping (higher = more important to this identity)
        """
        self.weights = identity_weights or {}

    def apply(self, bonds: list[ActivatedBond]) -> list[ActivatedBond]:
        """Apply identity weights to bond activations."""
        if not self.weights:
            return bonds

        result = []
        for ab in bonds:
            left_weight = self.weights.get(ab.bond.left, 1.0)
            right_weight = self.weights.get(ab.bond.right, 1.0)

            # Boost activation by identity weight
            identity_boost = (left_weight + right_weight) / 2
            new_activation = min(1.0, ab.activation * identity_boost)

            result.append(ActivatedBond(
                bond=ab.bond,
                recurrence=ab.recurrence,
                activation=new_activation,
                path_length=ab.path_length,
            ))

        return sorted(result, key=lambda b: -b.activation)

    @classmethod
    def from_seed_pbm(cls, seed_pbm: PairBondMap, boost_factor: float = 2.0) -> IdentityFilter:
        """
        Create identity filter from a seed PBM.

        Tokens that appear frequently in the seed get higher weights.
        """
        weights = {}
        if seed_pbm.total_bonds > 0:
            for token in seed_pbm.all_tokens():
                # Count occurrences
                forward = seed_pbm.get_forward_bonds(token)
                backward = seed_pbm.get_backward_bonds(token)
                occurrences = sum(r.count for r in forward.values()) + sum(r.count for r in backward.values())

                # Normalize and apply boost
                weight = 1.0 + (occurrences / seed_pbm.total_bonds) * boost_factor
                weights[token] = weight

        return cls(weights)


class ContextRetriever:
    """
    Main context retrieval interface.

    Combines activation spreading with optional identity filtering.
    """

    def __init__(
        self,
        pbm: PairBondMap,
        identity_filter: IdentityFilter | None = None,
        spreader_config: dict | None = None,
    ) -> None:
        """
        Initialize context retriever.

        Args:
            pbm: Knowledge base (pair bond map)
            identity_filter: Optional identity-based filtering
            spreader_config: Config for activation spreader
        """
        self.pbm = pbm
        self.identity_filter = identity_filter

        config = spreader_config or {}
        self.spreader = ActivationSpreader(
            pbm,
            decay_factor=config.get('decay_factor', 0.7),
            min_activation=config.get('min_activation', 0.01),
            max_depth=config.get('max_depth', 5),
        )

    def get_context(
        self,
        query_tokens: Sequence[TokenID],
        max_bonds: int = 50,
        min_activation: float = 0.05,
    ) -> ContextResult:
        """
        Retrieve relevant context for query tokens.

        Args:
            query_tokens: Tokens representing the query
            max_bonds: Maximum number of bonds to return
            min_activation: Minimum activation threshold

        Returns:
            ContextResult with activated bonds
        """
        # Spread activation from query (pass limit for early termination)
        activated = self.spreader.get_activated_bonds(query_tokens, max_bonds=max_bonds * 2)

        # Apply identity filter if present
        if self.identity_filter:
            activated = self.identity_filter.apply(activated)

        # Filter by threshold and limit
        filtered = [b for b in activated if b.activation >= min_activation][:max_bonds]

        return ContextResult(
            query_tokens=list(query_tokens),
            activated_bonds=filtered,
            total_activation=sum(b.activation for b in filtered),
            max_depth=max(b.path_length for b in filtered) if filtered else 0,
        )


def get_relevant_context(query: str, identity_token: str | None = None) -> list[ActivatedBond]:
    """
    High-level interface: get relevant bonds for a text query.

    This is the main entry point for bridge.py integration.

    Args:
        query: Text query to find context for
        identity_token: Optional identity string to bias results

    Returns:
        List of ActivatedBond objects, sorted by relevance
    """
    # TODO: This needs a knowledge PBM to be useful
    # For now, create from query itself (self-referential)
    # In production, this would load from storage

    from ..core.pair_bond import create_pbm_from_text

    # Tokenize query
    query_tokens = [TokenID.byte(b) for b in query.encode('utf-8')]

    # Create retriever with query's own structure
    # (Placeholder until we have proper knowledge base)
    query_pbm = create_pbm_from_text(query)

    # Create identity filter if provided
    identity_filter = None
    if identity_token:
        identity_pbm = create_pbm_from_text(identity_token)
        identity_filter = IdentityFilter.from_seed_pbm(identity_pbm)

    retriever = ContextRetriever(query_pbm, identity_filter)
    result = retriever.get_context(query_tokens)

    return result.activated_bonds
