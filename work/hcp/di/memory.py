"""
DI Memory - Persistent PBM storage that grows through experience.

The memory is a single PBM that accumulates all bonds from:
- Identity seed (birth)
- Interactions (experience)
- Self-reflection (internal processing)

No external dependencies. SQLite for persistence.

Usage:
    mem = Memory("./di_memory.db")
    mem.add_experience(pbm)
    context = mem.query("curiosity")
"""
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import sqlite3
import json
import time

from ..core.token_id import TokenID
from ..core.pair_bond import PairBondMap, PairBond, BondRecurrence


@dataclass
class MemoryStats:
    """Statistics about memory state."""
    total_bonds: int
    unique_bonds: int
    vocabulary_size: int
    oldest_bond: float
    newest_bond: float


class Memory:
    """
    Persistent memory store backed by SQLite.

    Schema:
    - bonds: (left_token, right_token, count, first_seen, last_seen)
    - vocabulary: (token_id, word)
    - metadata: (key, value)
    """

    def __init__(self, db_path: str | Path) -> None:
        self.db_path = Path(db_path)
        self._conn: sqlite3.Connection | None = None
        self._init_db()

    def _init_db(self) -> None:
        """Initialize database schema."""
        conn = self._get_conn()
        conn.executescript("""
            CREATE TABLE IF NOT EXISTS bonds (
                left_token TEXT NOT NULL,
                right_token TEXT NOT NULL,
                count INTEGER DEFAULT 1,
                first_seen REAL,
                last_seen REAL,
                PRIMARY KEY (left_token, right_token)
            );

            CREATE TABLE IF NOT EXISTS vocabulary (
                token_id TEXT PRIMARY KEY,
                word TEXT NOT NULL
            );

            CREATE TABLE IF NOT EXISTS metadata (
                key TEXT PRIMARY KEY,
                value TEXT
            );

            CREATE INDEX IF NOT EXISTS idx_bonds_left ON bonds(left_token);
            CREATE INDEX IF NOT EXISTS idx_bonds_right ON bonds(right_token);
        """)
        conn.commit()

    def _get_conn(self) -> sqlite3.Connection:
        """Get database connection."""
        if self._conn is None:
            self._conn = sqlite3.connect(self.db_path)
        return self._conn

    def close(self) -> None:
        """Close database connection."""
        if self._conn:
            self._conn.close()
            self._conn = None

    def _token_to_str(self, token: TokenID) -> str:
        return ".".join(str(s) for s in token.segments)

    def _str_to_token(self, s: str) -> TokenID:
        return TokenID(tuple(int(x) for x in s.split(".")))

    # === Core Operations ===

    def add_bond(
        self,
        left: TokenID,
        right: TokenID,
        count: int = 1,
        timestamp: float | None = None,
    ) -> None:
        """Add or strengthen a bond."""
        ts = timestamp or time.time()
        left_str = self._token_to_str(left)
        right_str = self._token_to_str(right)

        conn = self._get_conn()
        conn.execute("""
            INSERT INTO bonds (left_token, right_token, count, first_seen, last_seen)
            VALUES (?, ?, ?, ?, ?)
            ON CONFLICT(left_token, right_token) DO UPDATE SET
                count = count + excluded.count,
                last_seen = excluded.last_seen
        """, (left_str, right_str, count, ts, ts))
        conn.commit()

    def add_pbm(self, pbm: PairBondMap, timestamp: float | None = None) -> int:
        """
        Add all bonds from a PBM to memory.

        Returns number of bonds added.
        """
        ts = timestamp or time.time()
        conn = self._get_conn()
        count = 0

        for recurrence in pbm.all_bonds():
            left_str = self._token_to_str(recurrence.bond.left)
            right_str = self._token_to_str(recurrence.bond.right)

            conn.execute("""
                INSERT INTO bonds (left_token, right_token, count, first_seen, last_seen)
                VALUES (?, ?, ?, ?, ?)
                ON CONFLICT(left_token, right_token) DO UPDATE SET
                    count = count + excluded.count,
                    last_seen = excluded.last_seen
            """, (left_str, right_str, recurrence.count, ts, ts))
            count += 1

        conn.commit()
        return count

    def add_vocabulary(self, vocab: dict[str, TokenID]) -> None:
        """Add word -> token mappings."""
        conn = self._get_conn()
        for word, token in vocab.items():
            token_str = self._token_to_str(token)
            conn.execute("""
                INSERT OR REPLACE INTO vocabulary (token_id, word)
                VALUES (?, ?)
            """, (token_str, word))
        conn.commit()

    # === Queries ===

    def get_forward_bonds(self, token: TokenID, limit: int = 100) -> list[tuple[TokenID, int]]:
        """Get tokens that follow this token, with counts."""
        token_str = self._token_to_str(token)
        conn = self._get_conn()

        rows = conn.execute("""
            SELECT right_token, count FROM bonds
            WHERE left_token = ?
            ORDER BY count DESC
            LIMIT ?
        """, (token_str, limit)).fetchall()

        return [(self._str_to_token(r), c) for r, c in rows]

    def get_backward_bonds(self, token: TokenID, limit: int = 100) -> list[tuple[TokenID, int]]:
        """Get tokens that precede this token, with counts."""
        token_str = self._token_to_str(token)
        conn = self._get_conn()

        rows = conn.execute("""
            SELECT left_token, count FROM bonds
            WHERE right_token = ?
            ORDER BY count DESC
            LIMIT ?
        """, (token_str, limit)).fetchall()

        return [(self._str_to_token(l), c) for l, c in rows]

    def get_word_token(self, word: str) -> TokenID | None:
        """Look up token for word."""
        conn = self._get_conn()
        row = conn.execute(
            "SELECT token_id FROM vocabulary WHERE word = ?",
            (word.lower(),)
        ).fetchone()
        return self._str_to_token(row[0]) if row else None

    def get_token_word(self, token: TokenID) -> str | None:
        """Look up word for token."""
        token_str = self._token_to_str(token)
        conn = self._get_conn()
        row = conn.execute(
            "SELECT word FROM vocabulary WHERE token_id = ?",
            (token_str,)
        ).fetchone()
        return row[0] if row else None

    def query_related(self, word: str, limit: int = 10) -> list[tuple[str, int]]:
        """
        Find words related to the given word.

        Returns list of (word, connection_strength) tuples.
        """
        token = self.get_word_token(word)
        if not token:
            return []

        # Get both forward and backward bonds
        forward = self.get_forward_bonds(token, limit)
        backward = self.get_backward_bonds(token, limit)

        # Combine and look up words
        related: dict[str, int] = {}
        for tok, count in forward + backward:
            w = self.get_token_word(tok)
            if w and w != word:
                related[w] = related.get(w, 0) + count

        # Sort by strength
        return sorted(related.items(), key=lambda x: -x[1])[:limit]

    # === Memory Management ===

    def to_pbm(self) -> PairBondMap:
        """Export entire memory as PBM."""
        pbm = PairBondMap()
        conn = self._get_conn()

        for row in conn.execute("SELECT left_token, right_token, count FROM bonds"):
            left = self._str_to_token(row[0])
            right = self._str_to_token(row[1])
            for _ in range(row[2]):
                pbm.add_bond(left, right)

        return pbm

    def stats(self) -> MemoryStats:
        """Get memory statistics."""
        conn = self._get_conn()

        total = conn.execute("SELECT SUM(count) FROM bonds").fetchone()[0] or 0
        unique = conn.execute("SELECT COUNT(*) FROM bonds").fetchone()[0] or 0
        vocab = conn.execute("SELECT COUNT(*) FROM vocabulary").fetchone()[0] or 0

        times = conn.execute(
            "SELECT MIN(first_seen), MAX(last_seen) FROM bonds"
        ).fetchone()
        oldest = times[0] or 0.0
        newest = times[1] or 0.0

        return MemoryStats(
            total_bonds=total,
            unique_bonds=unique,
            vocabulary_size=vocab,
            oldest_bond=oldest,
            newest_bond=newest,
        )

    def set_metadata(self, key: str, value: str) -> None:
        """Store metadata."""
        conn = self._get_conn()
        conn.execute(
            "INSERT OR REPLACE INTO metadata (key, value) VALUES (?, ?)",
            (key, value)
        )
        conn.commit()

    def get_metadata(self, key: str) -> str | None:
        """Retrieve metadata."""
        conn = self._get_conn()
        row = conn.execute(
            "SELECT value FROM metadata WHERE key = ?", (key,)
        ).fetchone()
        return row[0] if row else None

    def decay(self, factor: float = 0.9, min_count: int = 1) -> int:
        """
        Apply decay to all bonds (forgetting).

        Reduces counts by factor, removes bonds below min_count.
        Returns number of bonds removed.
        """
        conn = self._get_conn()

        # Decay
        conn.execute("UPDATE bonds SET count = CAST(count * ? AS INTEGER)", (factor,))

        # Remove weak bonds
        removed = conn.execute(
            "DELETE FROM bonds WHERE count < ?", (min_count,)
        ).rowcount

        conn.commit()
        return removed
