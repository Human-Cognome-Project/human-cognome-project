"""
Pair Bond data structures: FPB (Forward Pair Bond), FBR (Forward Bond Recurrence), PBM (Pair Bond Map).

The PBM encodes structural relationships between tokens through pair bonds.
Each pair bond records which tokens appear adjacent to each other and how often.
"""
from __future__ import annotations

from collections import defaultdict
from dataclasses import dataclass, field
from typing import Iterator, Sequence

from .token_id import TokenID


@dataclass(frozen=True, slots=True)
class PairBond:
    """
    A Forward Pair Bond (FPB) represents an ordered adjacency relationship.

    left -> right indicates that 'right' follows 'left' in some expression.
    """
    left: TokenID
    right: TokenID

    def __str__(self) -> str:
        return f"({self.left} -> {self.right})"

    def __repr__(self) -> str:
        return f"PairBond({self.left!r}, {self.right!r})"

    def reversed(self) -> PairBond:
        """Return the reverse bond (right -> left)."""
        return PairBond(self.right, self.left)


@dataclass
class BondRecurrence:
    """
    Forward Bond Recurrence (FBR) tracks how many times a pair bond occurs.

    Also tracks positional information for reconstruction.
    """
    bond: PairBond
    count: int = 0
    # Positions where this bond occurs (for reconstruction)
    positions: list[int] = field(default_factory=list)

    def increment(self, position: int | None = None) -> None:
        """Increment the recurrence count, optionally recording position."""
        self.count += 1
        if position is not None:
            self.positions.append(position)

    def __str__(self) -> str:
        return f"{self.bond} x{self.count}"


class PairBondMap:
    """
    Pair Bond Map (PBM) - the core data structure for storing token relationships.

    Maps each token to its forward bonds and their recurrence counts.
    This enables both statistical analysis and structural reconstruction.
    """

    def __init__(self) -> None:
        # left_token -> right_token -> BondRecurrence
        self._bonds: dict[TokenID, dict[TokenID, BondRecurrence]] = defaultdict(dict)
        # Token sequence for reconstruction
        self._sequence: list[TokenID] = []
        # Total bond count
        self._total_bonds: int = 0

    def add_bond(self, left: TokenID, right: TokenID, position: int | None = None) -> BondRecurrence:
        """Add or increment a pair bond."""
        if right not in self._bonds[left]:
            self._bonds[left][right] = BondRecurrence(PairBond(left, right))

        recurrence = self._bonds[left][right]
        recurrence.increment(position)
        self._total_bonds += 1
        return recurrence

    def add_sequence(self, tokens: Sequence[TokenID]) -> None:
        """Add a sequence of tokens, creating pair bonds between adjacent tokens."""
        self._sequence.extend(tokens)
        for i in range(len(tokens) - 1):
            self.add_bond(tokens[i], tokens[i + 1], position=i)

    def get_bond(self, left: TokenID, right: TokenID) -> BondRecurrence | None:
        """Get the recurrence for a specific bond, or None if it doesn't exist."""
        return self._bonds.get(left, {}).get(right)

    def get_forward_bonds(self, token: TokenID) -> dict[TokenID, BondRecurrence]:
        """Get all forward bonds from a token."""
        return dict(self._bonds.get(token, {}))

    def get_backward_bonds(self, token: TokenID) -> dict[TokenID, BondRecurrence]:
        """Get all backward bonds to a token (which tokens precede this one)."""
        result = {}
        for left, rights in self._bonds.items():
            if token in rights:
                result[left] = rights[token]
        return result

    def bond_strength(self, left: TokenID, right: TokenID) -> float:
        """
        Calculate normalized bond strength (0.0 to 1.0).

        Strength is the proportion of times 'left' is followed by 'right'
        relative to all tokens that follow 'left'.
        """
        forward = self._bonds.get(left, {})
        if not forward:
            return 0.0

        total = sum(br.count for br in forward.values())
        bond = forward.get(right)
        if not bond:
            return 0.0

        return bond.count / total

    def all_bonds(self) -> Iterator[BondRecurrence]:
        """Iterate over all bond recurrences."""
        for rights in self._bonds.values():
            yield from rights.values()

    def all_tokens(self) -> set[TokenID]:
        """Get all unique tokens in the PBM."""
        tokens = set(self._bonds.keys())
        for rights in self._bonds.values():
            tokens.update(rights.keys())
        return tokens

    def sequence(self) -> list[TokenID]:
        """Get the original token sequence (if recorded)."""
        return list(self._sequence)

    @property
    def total_bonds(self) -> int:
        """Total number of bond occurrences (sum of all FBR counts)."""
        return self._total_bonds

    @property
    def unique_bonds(self) -> int:
        """Number of unique pair bonds."""
        return sum(len(rights) for rights in self._bonds.values())

    def merge(self, other: PairBondMap) -> None:
        """Merge another PBM into this one."""
        for left, rights in other._bonds.items():
            for right, recurrence in rights.items():
                for pos in recurrence.positions:
                    self.add_bond(left, right, position=pos)

    def to_dict(self) -> dict:
        """Serialize to a dictionary for storage."""
        bonds = []
        for left, rights in self._bonds.items():
            for right, recurrence in rights.items():
                bonds.append({
                    "left": left.to_string(),
                    "right": right.to_string(),
                    "count": recurrence.count,
                    "positions": recurrence.positions,
                })
        return {
            "bonds": bonds,
            "sequence": [t.to_string() for t in self._sequence],
        }

    @classmethod
    def from_dict(cls, data: dict) -> PairBondMap:
        """Deserialize from a dictionary."""
        pbm = cls()
        pbm._sequence = [TokenID.from_string(s) for s in data.get("sequence", [])]
        for bond_data in data.get("bonds", []):
            left = TokenID.from_string(bond_data["left"])
            right = TokenID.from_string(bond_data["right"])
            for pos in bond_data.get("positions", [None] * bond_data["count"]):
                pbm.add_bond(left, right, position=pos)
        return pbm

    def __str__(self) -> str:
        lines = [f"PairBondMap({self.unique_bonds} unique bonds, {self.total_bonds} total):"]
        for recurrence in sorted(self.all_bonds(), key=lambda r: -r.count)[:10]:
            lines.append(f"  {recurrence}")
        if self.unique_bonds > 10:
            lines.append(f"  ... and {self.unique_bonds - 10} more")
        return "\n".join(lines)

    def __repr__(self) -> str:
        return f"PairBondMap(unique={self.unique_bonds}, total={self.total_bonds})"


def create_pbm_from_text(text: str) -> PairBondMap:
    """
    Quick helper to create a PBM from text using byte-level tokens.

    This is a convenience function; the full pipeline uses the atomizer.
    """
    pbm = PairBondMap()
    tokens = [TokenID.byte(b) for b in text.encode("utf-8")]
    pbm.add_sequence(tokens)
    return pbm
