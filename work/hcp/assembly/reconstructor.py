"""
Reconstructor: rebuild expressions from PairBondMap structures.

Key challenge: PBM stores pairs and recurrence, but reconstruction may be ambiguous.
We use positional hints stored in PBM to achieve lossless reconstruction.
"""
from __future__ import annotations

from dataclasses import dataclass
from typing import Iterator, Sequence

from ..core.token_id import TokenID
from ..core.pair_bond import PairBondMap


@dataclass
class ReconstructionResult:
    """Result of a reconstruction attempt."""
    tokens: list[TokenID]
    success: bool
    ambiguities: int  # Number of points where multiple paths existed
    method: str  # 'sequence' | 'position' | 'heuristic'

    def to_bytes(self) -> bytes:
        """Convert token sequence back to bytes (for byte-level tokens)."""
        result = []
        for token in self.tokens:
            if token.is_byte():
                result.append(token.value)
            else:
                raise ValueError(f"Cannot convert non-byte token to bytes: {token}")
        return bytes(result)

    def to_string(self) -> str:
        """Convert to string (for byte-level tokens)."""
        return self.to_bytes().decode("utf-8")


class Reconstructor:
    """
    Reconstruct expressions from PairBondMap.

    Strategies:
    1. Sequence-based: Use stored sequence if available (lossless)
    2. Position-based: Use bond positions to order tokens
    3. Heuristic: Follow highest-frequency bonds (lossy)
    """

    def __init__(self, word_lookup: dict[TokenID, bytes] | None = None) -> None:
        """
        Initialize reconstructor.

        Args:
            word_lookup: Map from word tokens to their byte representation
        """
        self.word_lookup = word_lookup or {}

    def reconstruct(self, pbm: PairBondMap) -> ReconstructionResult:
        """
        Reconstruct token sequence from PBM.

        Uses the best available strategy based on PBM contents.
        """
        # Strategy 1: Direct sequence (if stored)
        sequence = pbm.sequence()
        if sequence:
            return ReconstructionResult(
                tokens=sequence,
                success=True,
                ambiguities=0,
                method="sequence",
            )

        # Strategy 2: Position-based reconstruction
        result = self._reconstruct_from_positions(pbm)
        if result.success:
            return result

        # Strategy 3: Heuristic (follow bonds)
        return self._reconstruct_heuristic(pbm)

    def _reconstruct_from_positions(self, pbm: PairBondMap) -> ReconstructionResult:
        """Reconstruct using stored position information."""
        # Collect all bonds with positions
        positioned_bonds = []
        for recurrence in pbm.all_bonds():
            for pos in recurrence.positions:
                positioned_bonds.append((pos, recurrence.bond))

        if not positioned_bonds:
            return ReconstructionResult(
                tokens=[],
                success=False,
                ambiguities=0,
                method="position",
            )

        # Sort by position
        positioned_bonds.sort(key=lambda x: x[0])

        # Build sequence
        tokens = []
        for i, (pos, bond) in enumerate(positioned_bonds):
            if i == 0:
                tokens.append(bond.left)
            tokens.append(bond.right)

        return ReconstructionResult(
            tokens=tokens,
            success=True,
            ambiguities=0,
            method="position",
        )

    def _reconstruct_heuristic(self, pbm: PairBondMap) -> ReconstructionResult:
        """
        Heuristic reconstruction: follow highest-frequency bonds.

        This is lossy - may not reproduce exact original.
        """
        all_tokens = pbm.all_tokens()
        if not all_tokens:
            return ReconstructionResult(
                tokens=[],
                success=True,
                ambiguities=0,
                method="heuristic",
            )

        # Find likely start token (one with no or few backward bonds)
        start_candidates = []
        for token in all_tokens:
            backward = pbm.get_backward_bonds(token)
            start_candidates.append((len(backward), token))

        start_candidates.sort(key=lambda x: x[0])
        start_token = start_candidates[0][1]

        # Follow forward bonds greedily
        tokens = [start_token]
        visited = {start_token}
        ambiguities = 0
        current = start_token

        while True:
            forward = pbm.get_forward_bonds(current)
            if not forward:
                break

            # Sort by count (highest first)
            candidates = sorted(
                forward.items(),
                key=lambda x: -x[1].count,
            )

            # Count ambiguities
            if len(candidates) > 1:
                top_count = candidates[0][1].count
                ambig_count = sum(1 for _, r in candidates if r.count == top_count)
                if ambig_count > 1:
                    ambiguities += 1

            # Take highest frequency next token
            next_token = candidates[0][0]

            # Avoid infinite loops
            if next_token in visited:
                # Try next candidate
                for cand, _ in candidates[1:]:
                    if cand not in visited:
                        next_token = cand
                        break
                else:
                    break

            tokens.append(next_token)
            visited.add(next_token)
            current = next_token

        return ReconstructionResult(
            tokens=tokens,
            success=True,
            ambiguities=ambiguities,
            method="heuristic",
        )

    def tokens_to_bytes(self, tokens: Sequence[TokenID]) -> bytes:
        """Convert a token sequence to bytes."""
        result = []
        for token in tokens:
            if token.is_byte():
                result.append(token.value)
            elif token.is_glyph():
                # Convert codepoint to UTF-8 bytes
                char = chr(token.value)
                result.extend(char.encode("utf-8"))
            elif token.is_word():
                # Look up word bytes
                word_bytes = self.word_lookup.get(token)
                if word_bytes:
                    result.extend(word_bytes)
                else:
                    raise ValueError(f"Unknown word token: {token}")
            else:
                raise ValueError(f"Cannot convert token to bytes: {token}")

        return bytes(result)

    def tokens_to_string(self, tokens: Sequence[TokenID]) -> str:
        """Convert a token sequence to string."""
        return self.tokens_to_bytes(tokens).decode("utf-8")


class ByteReconstructor(Reconstructor):
    """Specialized reconstructor for byte-level tokens only."""

    def reconstruct_bytes(self, pbm: PairBondMap) -> bytes:
        """Reconstruct directly to bytes."""
        result = self.reconstruct(pbm)
        return result.to_bytes()

    def reconstruct_string(self, pbm: PairBondMap) -> str:
        """Reconstruct directly to string."""
        return self.reconstruct_bytes(pbm).decode("utf-8")


def reconstruct_from_pbm(pbm: PairBondMap) -> list[TokenID]:
    """Convenience function for reconstruction."""
    reconstructor = Reconstructor()
    return reconstructor.reconstruct(pbm).tokens


def pbm_to_bytes(pbm: PairBondMap) -> bytes:
    """Reconstruct PBM to bytes (for byte-level tokens)."""
    reconstructor = ByteReconstructor()
    return reconstructor.reconstruct_bytes(pbm)


def pbm_to_string(pbm: PairBondMap) -> str:
    """Reconstruct PBM to string (for byte-level tokens)."""
    return pbm_to_bytes(pbm).decode("utf-8")
