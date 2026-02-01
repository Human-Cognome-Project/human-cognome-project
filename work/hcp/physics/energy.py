"""
Energy system for token physics.

Energy in HCP is metaphorical:
- Energy = edit distance weighted by bond strength
- Low energy = well-formed, strongly bonded structures
- High energy = malformed, weakly bonded or unknown structures

Minimization finds the lowest-energy interpretation of ambiguous input.
"""
from __future__ import annotations

from dataclasses import dataclass
from typing import Sequence, Callable
import math

from ..core.token_id import TokenID
from ..core.pair_bond import PairBondMap


@dataclass
class EnergyState:
    """Energy state of a token sequence."""
    tokens: list[TokenID]
    total_energy: float
    bond_energies: list[float]  # Energy at each bond position
    unknown_count: int  # Number of unknown/soft tokens

    @property
    def average_energy(self) -> float:
        """Average energy per bond."""
        if not self.bond_energies:
            return 0.0
        return self.total_energy / len(self.bond_energies)

    @property
    def is_stable(self) -> bool:
        """Check if state is low-energy (stable)."""
        return self.total_energy < 0.5 * len(self.tokens)


class EnergyFunction:
    """
    Compute energy of token configurations.

    Lower energy = better configuration.
    """

    def __init__(
        self,
        knowledge_pbm: PairBondMap | None = None,
        unknown_penalty: float = 1.0,
        weak_bond_penalty: float = 0.5,
    ) -> None:
        """
        Initialize energy function.

        Args:
            knowledge_pbm: Known good bonds from training data
            unknown_penalty: Energy penalty for unknown tokens
            weak_bond_penalty: Penalty for weak/missing bonds
        """
        self.knowledge = knowledge_pbm or PairBondMap()
        self.unknown_penalty = unknown_penalty
        self.weak_bond_penalty = weak_bond_penalty

    def bond_energy(self, left: TokenID, right: TokenID) -> float:
        """
        Calculate energy of a single bond.

        Strong known bonds = low energy
        Weak/unknown bonds = high energy
        """
        strength = self.knowledge.bond_strength(left, right)
        if strength > 0:
            # Known bond: energy inversely proportional to strength
            return 1.0 - strength
        else:
            # Unknown bond: penalty
            return self.weak_bond_penalty

    def sequence_energy(
        self,
        tokens: Sequence[TokenID],
        unknown_tokens: set[TokenID] | None = None,
    ) -> EnergyState:
        """Calculate total energy of a token sequence."""
        if not tokens:
            return EnergyState(
                tokens=list(tokens),
                total_energy=0.0,
                bond_energies=[],
                unknown_count=0,
            )

        unknown_tokens = unknown_tokens or set()
        bond_energies = []
        unknown_count = sum(1 for t in tokens if t in unknown_tokens)

        for i in range(len(tokens) - 1):
            left, right = tokens[i], tokens[i + 1]
            energy = self.bond_energy(left, right)

            # Extra penalty for unknown tokens
            if left in unknown_tokens or right in unknown_tokens:
                energy += self.unknown_penalty

            bond_energies.append(energy)

        total = sum(bond_energies) + unknown_count * self.unknown_penalty

        return EnergyState(
            tokens=list(tokens),
            total_energy=total,
            bond_energies=bond_energies,
            unknown_count=unknown_count,
        )


class EnergyMinimizer:
    """
    Find minimum-energy token configurations.

    Uses edit operations to explore configuration space.
    """

    def __init__(
        self,
        energy_fn: EnergyFunction,
        candidates_fn: Callable[[TokenID], list[TokenID]] | None = None,
        max_iterations: int = 100,
    ) -> None:
        """
        Initialize minimizer.

        Args:
            energy_fn: Function to compute energy
            candidates_fn: Function to get replacement candidates for a token
            max_iterations: Maximum optimization iterations
        """
        self.energy_fn = energy_fn
        self.candidates_fn = candidates_fn or (lambda t: [])
        self.max_iterations = max_iterations

    def minimize(
        self,
        tokens: list[TokenID],
        unknown_tokens: set[TokenID] | None = None,
    ) -> EnergyState:
        """
        Find minimum-energy configuration starting from given tokens.

        Uses greedy local search: try replacements that reduce energy.
        """
        unknown_tokens = unknown_tokens or set()
        current = list(tokens)
        current_state = self.energy_fn.sequence_energy(current, unknown_tokens)

        for _ in range(self.max_iterations):
            improved = False

            # Try to improve each unknown token
            for i, token in enumerate(current):
                if token not in unknown_tokens:
                    continue

                # Get candidates
                candidates = self.candidates_fn(token)
                if not candidates:
                    continue

                # Try each candidate
                best_candidate = None
                best_energy = current_state.total_energy

                for candidate in candidates:
                    # Create new sequence with replacement
                    test = list(current)
                    test[i] = candidate

                    # Update unknown set
                    test_unknown = unknown_tokens - {token}
                    test_state = self.energy_fn.sequence_energy(test, test_unknown)

                    if test_state.total_energy < best_energy:
                        best_energy = test_state.total_energy
                        best_candidate = candidate

                # Apply best improvement
                if best_candidate is not None:
                    current[i] = best_candidate
                    unknown_tokens = unknown_tokens - {token}
                    current_state = self.energy_fn.sequence_energy(current, unknown_tokens)
                    improved = True

            if not improved:
                break

        return current_state


def edit_distance(s1: str, s2: str) -> int:
    """Compute Damerau-Levenshtein edit distance (includes transpositions)."""
    m, n = len(s1), len(s2)

    # Handle empty strings
    if m == 0:
        return n
    if n == 0:
        return m

    # Create distance matrix
    dp = [[0] * (n + 1) for _ in range(m + 1)]

    for i in range(m + 1):
        dp[i][0] = i
    for j in range(n + 1):
        dp[0][j] = j

    for i in range(1, m + 1):
        for j in range(1, n + 1):
            cost = 0 if s1[i - 1] == s2[j - 1] else 1

            dp[i][j] = min(
                dp[i - 1][j] + 1,      # Deletion
                dp[i][j - 1] + 1,      # Insertion
                dp[i - 1][j - 1] + cost,  # Substitution
            )

            # Transposition (Damerau extension)
            if i > 1 and j > 1 and s1[i - 1] == s2[j - 2] and s1[i - 2] == s2[j - 1]:
                dp[i][j] = min(dp[i][j], dp[i - 2][j - 2] + cost)

    return dp[m][n]


def weighted_edit_distance(
    s1: str,
    s2: str,
    knowledge: PairBondMap | None = None,
) -> float:
    """
    Compute edit distance weighted by bond strengths.

    Operations on strongly-bonded pairs cost more.
    """
    if knowledge is None:
        return float(edit_distance(s1, s2))

    # Simple weighted version: base edit distance
    # Future: weight by disrupted bond strengths
    return float(edit_distance(s1, s2))
