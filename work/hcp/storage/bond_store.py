"""
Bond storage: PBM persistence with FBR tracking.
"""
from __future__ import annotations

import hashlib
from pathlib import Path

from ..core.pair_bond import PairBondMap, BondRecurrence
from ..core.token_id import TokenID
from .schema import TokenDatabase
from .token_store import TokenStore


class BondStore:
    """
    Persistent storage for pair bond maps.

    Integrates with TokenStore to ensure all bonded tokens are registered.
    """

    def __init__(self, token_store: TokenStore) -> None:
        self._tokens = token_store
        self._db = token_store.db

    @classmethod
    def open(cls, path: str | Path) -> BondStore:
        """Open a bond store from a file path."""
        token_store = TokenStore.open(path)
        return cls(token_store)

    @classmethod
    def memory(cls) -> BondStore:
        """Create an in-memory bond store."""
        return cls(TokenStore.memory())

    def close(self) -> None:
        """Close underlying storage."""
        self._tokens.close()

    def store_pbm(self, pbm: PairBondMap, source: bytes | None = None) -> int:
        """
        Store a PairBondMap to the database.

        Returns the sequence ID for this storage operation.
        """
        # Calculate source hash
        if source is not None:
            source_hash = hashlib.sha256(source).hexdigest()
        else:
            # Hash the sequence if no source provided
            seq_bytes = b"".join(t.to_string().encode() for t in pbm.sequence())
            source_hash = hashlib.sha256(seq_bytes).hexdigest()

        # Create sequence entry
        sequence_id = self._db.create_sequence(source_hash, len(pbm.sequence()))

        # Register all tokens and store bonds
        for recurrence in pbm.all_bonds():
            bond = recurrence.bond

            # Ensure tokens are registered
            self._tokens.register(bond.left)
            self._tokens.register(bond.right)

            # Store each occurrence
            for pos in recurrence.positions:
                self._db.add_pair_bond(
                    left_token=bond.left.to_string(),
                    right_token=bond.right.to_string(),
                    position=pos,
                    sequence_id=sequence_id,
                )

        return sequence_id

    def load_pbm(self, sequence_id: int | None = None) -> PairBondMap:
        """
        Load a PairBondMap from the database.

        If sequence_id is None, loads all bonds (aggregate).
        """
        pbm = PairBondMap()
        conn = self._db.connect()

        if sequence_id is not None:
            # Load bonds for a specific sequence
            rows = conn.execute(
                """
                SELECT pb.left_token, pb.right_token, bp.position
                FROM pair_bonds pb
                JOIN bond_positions bp ON pb.id = bp.bond_id
                WHERE bp.sequence_id = ?
                ORDER BY bp.position
                """,
                (sequence_id,),
            ).fetchall()

            for row in rows:
                left = TokenID.from_string(row["left_token"])
                right = TokenID.from_string(row["right_token"])
                pbm.add_bond(left, right, position=row["position"])
        else:
            # Load aggregate bonds
            rows = conn.execute(
                """
                SELECT left_token, right_token, count
                FROM pair_bonds
                """,
            ).fetchall()

            for row in rows:
                left = TokenID.from_string(row["left_token"])
                right = TokenID.from_string(row["right_token"])
                for _ in range(row["count"]):
                    pbm.add_bond(left, right)

        return pbm

    def get_forward_bonds(self, token: TokenID) -> list[tuple[TokenID, int]]:
        """Get forward bonds from a token as (target, count) pairs."""
        rows = self._db.get_forward_bonds(token.to_string())
        return [
            (TokenID.from_string(row["right_token"]), row["count"])
            for row in rows
        ]

    def get_backward_bonds(self, token: TokenID) -> list[tuple[TokenID, int]]:
        """Get backward bonds to a token as (source, count) pairs."""
        rows = self._db.get_backward_bonds(token.to_string())
        return [
            (TokenID.from_string(row["left_token"]), row["count"])
            for row in rows
        ]

    def bond_strength(self, left: TokenID, right: TokenID) -> float:
        """Calculate bond strength from stored data."""
        conn = self._db.connect()

        # Get total forward bonds from left
        total_row = conn.execute(
            "SELECT SUM(count) as total FROM pair_bonds WHERE left_token = ?",
            (left.to_string(),),
        ).fetchone()
        total = total_row["total"] or 0

        if total == 0:
            return 0.0

        # Get count for this specific bond
        bond_row = conn.execute(
            "SELECT count FROM pair_bonds WHERE left_token = ? AND right_token = ?",
            (left.to_string(), right.to_string()),
        ).fetchone()
        count = bond_row["count"] if bond_row else 0

        return count / total

    @property
    def token_store(self) -> TokenStore:
        """Access the underlying token store."""
        return self._tokens

    def unique_bond_count(self) -> int:
        """Get number of unique pair bonds."""
        return self._db.bond_count()

    def total_bond_count(self) -> int:
        """Get total bond occurrences."""
        conn = self._db.connect()
        row = conn.execute("SELECT SUM(count) as total FROM pair_bonds").fetchone()
        return row["total"] or 0
