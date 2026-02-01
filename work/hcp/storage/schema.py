"""
SQLite and DuckDB schemas for HCP storage.

SQLite: Token registry, point lookups
DuckDB: Bond analytics, aggregations on FBR
"""
from __future__ import annotations

import sqlite3
from contextlib import contextmanager
from pathlib import Path
from typing import Iterator

# SQLite schema for token registry
SQLITE_SCHEMA = """
-- Token registry: maps token IDs to their content/metadata
CREATE TABLE IF NOT EXISTS tokens (
    token_id TEXT PRIMARY KEY,
    mode INTEGER NOT NULL,
    value INTEGER NOT NULL,
    content BLOB,  -- Original content (for byte/glyph tokens)
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_tokens_mode ON tokens(mode);
CREATE INDEX IF NOT EXISTS idx_tokens_value ON tokens(mode, value);

-- Pair bonds table for persistence
CREATE TABLE IF NOT EXISTS pair_bonds (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    left_token TEXT NOT NULL,
    right_token TEXT NOT NULL,
    count INTEGER DEFAULT 1,
    UNIQUE(left_token, right_token),
    FOREIGN KEY (left_token) REFERENCES tokens(token_id),
    FOREIGN KEY (right_token) REFERENCES tokens(token_id)
);

CREATE INDEX IF NOT EXISTS idx_bonds_left ON pair_bonds(left_token);
CREATE INDEX IF NOT EXISTS idx_bonds_right ON pair_bonds(right_token);

-- Bond positions for reconstruction
CREATE TABLE IF NOT EXISTS bond_positions (
    bond_id INTEGER NOT NULL,
    position INTEGER NOT NULL,
    sequence_id INTEGER,  -- Links to which sequence this position belongs to
    FOREIGN KEY (bond_id) REFERENCES pair_bonds(id)
);

CREATE INDEX IF NOT EXISTS idx_positions_bond ON bond_positions(bond_id);
CREATE INDEX IF NOT EXISTS idx_positions_sequence ON bond_positions(sequence_id);

-- Sequences table for tracking input sequences
CREATE TABLE IF NOT EXISTS sequences (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    source_hash TEXT NOT NULL,  -- SHA256 of original input
    token_count INTEGER NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- NSM primitives (mode-00 tokens)
CREATE TABLE IF NOT EXISTS nsm_primitives (
    value INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    category TEXT,
    description TEXT
);
"""

# DuckDB schema for analytics (columnar, aggregation-optimized)
DUCKDB_SCHEMA = """
-- Denormalized bond analytics table
CREATE TABLE IF NOT EXISTS bond_analytics (
    left_token VARCHAR NOT NULL,
    right_token VARCHAR NOT NULL,
    left_mode INTEGER NOT NULL,
    right_mode INTEGER NOT NULL,
    count INTEGER NOT NULL,
    total_left_bonds INTEGER,  -- Total bonds from left token
    bond_strength DOUBLE,  -- count / total_left_bonds
);

-- Aggregated token statistics
CREATE TABLE IF NOT EXISTS token_stats (
    token_id VARCHAR PRIMARY KEY,
    mode INTEGER NOT NULL,
    forward_bond_count INTEGER,  -- Number of unique forward bonds
    backward_bond_count INTEGER,  -- Number of unique backward bonds
    total_forward_occurrences INTEGER,
    total_backward_occurrences INTEGER,
);
"""


class TokenDatabase:
    """SQLite-based token registry and bond storage."""

    def __init__(self, db_path: str | Path = ":memory:") -> None:
        self.db_path = str(db_path)
        self._conn: sqlite3.Connection | None = None

    def connect(self) -> sqlite3.Connection:
        """Get or create database connection."""
        if self._conn is None:
            self._conn = sqlite3.connect(self.db_path)
            self._conn.row_factory = sqlite3.Row
            self._conn.execute("PRAGMA foreign_keys = ON")
            self._conn.executescript(SQLITE_SCHEMA)
        return self._conn

    def close(self) -> None:
        """Close database connection."""
        if self._conn:
            self._conn.close()
            self._conn = None

    @contextmanager
    def transaction(self) -> Iterator[sqlite3.Connection]:
        """Context manager for transactions."""
        conn = self.connect()
        try:
            yield conn
            conn.commit()
        except Exception:
            conn.rollback()
            raise

    def register_token(
        self,
        token_id: str,
        mode: int,
        value: int,
        content: bytes | None = None,
    ) -> None:
        """Register a token in the database."""
        with self.transaction() as conn:
            conn.execute(
                """
                INSERT OR IGNORE INTO tokens (token_id, mode, value, content)
                VALUES (?, ?, ?, ?)
                """,
                (token_id, mode, value, content),
            )

    def get_token(self, token_id: str) -> dict | None:
        """Retrieve a token by ID."""
        conn = self.connect()
        row = conn.execute(
            "SELECT * FROM tokens WHERE token_id = ?",
            (token_id,),
        ).fetchone()
        return dict(row) if row else None

    def add_pair_bond(
        self,
        left_token: str,
        right_token: str,
        position: int | None = None,
        sequence_id: int | None = None,
    ) -> int:
        """Add or increment a pair bond, return bond ID."""
        with self.transaction() as conn:
            # Upsert the bond
            conn.execute(
                """
                INSERT INTO pair_bonds (left_token, right_token, count)
                VALUES (?, ?, 1)
                ON CONFLICT(left_token, right_token)
                DO UPDATE SET count = count + 1
                """,
                (left_token, right_token),
            )

            # Get the bond ID
            row = conn.execute(
                "SELECT id FROM pair_bonds WHERE left_token = ? AND right_token = ?",
                (left_token, right_token),
            ).fetchone()
            bond_id = row["id"]

            # Record position if provided
            if position is not None:
                conn.execute(
                    "INSERT INTO bond_positions (bond_id, position, sequence_id) VALUES (?, ?, ?)",
                    (bond_id, position, sequence_id),
                )

            return bond_id

    def get_forward_bonds(self, token_id: str) -> list[dict]:
        """Get all forward bonds from a token."""
        conn = self.connect()
        rows = conn.execute(
            """
            SELECT right_token, count
            FROM pair_bonds
            WHERE left_token = ?
            ORDER BY count DESC
            """,
            (token_id,),
        ).fetchall()
        return [dict(row) for row in rows]

    def get_backward_bonds(self, token_id: str) -> list[dict]:
        """Get all backward bonds to a token."""
        conn = self.connect()
        rows = conn.execute(
            """
            SELECT left_token, count
            FROM pair_bonds
            WHERE right_token = ?
            ORDER BY count DESC
            """,
            (token_id,),
        ).fetchall()
        return [dict(row) for row in rows]

    def create_sequence(self, source_hash: str, token_count: int) -> int:
        """Create a new sequence entry, return its ID."""
        with self.transaction() as conn:
            cursor = conn.execute(
                "INSERT INTO sequences (source_hash, token_count) VALUES (?, ?)",
                (source_hash, token_count),
            )
            return cursor.lastrowid

    def register_nsm_primitive(
        self,
        value: int,
        name: str,
        category: str | None = None,
        description: str | None = None,
    ) -> None:
        """Register an NSM primitive."""
        with self.transaction() as conn:
            conn.execute(
                """
                INSERT OR REPLACE INTO nsm_primitives (value, name, category, description)
                VALUES (?, ?, ?, ?)
                """,
                (value, name, category, description),
            )

    def get_nsm_primitive(self, value: int) -> dict | None:
        """Get an NSM primitive by value."""
        conn = self.connect()
        row = conn.execute(
            "SELECT * FROM nsm_primitives WHERE value = ?",
            (value,),
        ).fetchone()
        return dict(row) if row else None

    def get_nsm_by_name(self, name: str) -> dict | None:
        """Get an NSM primitive by name."""
        conn = self.connect()
        row = conn.execute(
            "SELECT * FROM nsm_primitives WHERE name = ?",
            (name,),
        ).fetchone()
        return dict(row) if row else None

    def token_count(self) -> int:
        """Get total number of registered tokens."""
        conn = self.connect()
        row = conn.execute("SELECT COUNT(*) as cnt FROM tokens").fetchone()
        return row["cnt"]

    def bond_count(self) -> int:
        """Get total number of unique pair bonds."""
        conn = self.connect()
        row = conn.execute("SELECT COUNT(*) as cnt FROM pair_bonds").fetchone()
        return row["cnt"]


def create_database(db_path: str | Path = ":memory:") -> TokenDatabase:
    """Create and initialize a new token database."""
    db = TokenDatabase(db_path)
    db.connect()  # This initializes the schema
    return db
