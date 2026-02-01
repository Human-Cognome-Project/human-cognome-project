"""
Token registry: high-level interface for token storage and retrieval.
"""
from __future__ import annotations

from pathlib import Path
from typing import Iterator

from ..core.token_id import TokenID
from .schema import TokenDatabase, create_database


class TokenStore:
    """
    High-level token registry interface.

    Wraps the database layer with TokenID-aware operations.
    """

    def __init__(self, db: TokenDatabase | None = None) -> None:
        self._db = db or create_database()
        self._cache: dict[str, TokenID] = {}

    @classmethod
    def open(cls, path: str | Path) -> TokenStore:
        """Open a token store from a file path."""
        db = create_database(path)
        return cls(db)

    @classmethod
    def memory(cls) -> TokenStore:
        """Create an in-memory token store."""
        return cls(create_database(":memory:"))

    def close(self) -> None:
        """Close the underlying database."""
        self._db.close()

    def register(self, token: TokenID, content: bytes | None = None) -> None:
        """Register a token, optionally with its source content."""
        token_str = token.to_string()
        if token_str not in self._cache:
            self._db.register_token(
                token_id=token_str,
                mode=token.mode,
                value=token.value,
                content=content,
            )
            self._cache[token_str] = token

    def register_byte(self, byte_value: int) -> TokenID:
        """Register and return a byte token."""
        token = TokenID.byte(byte_value)
        self.register(token, content=bytes([byte_value]))
        return token

    def register_glyph(self, char: str) -> TokenID:
        """Register and return a glyph token for a character."""
        if len(char) != 1:
            raise ValueError(f"Expected single character, got {len(char)}")
        token = TokenID.glyph(ord(char))
        self.register(token, content=char.encode("utf-8"))
        return token

    def get(self, token: TokenID) -> dict | None:
        """Get token metadata from storage."""
        return self._db.get_token(token.to_string())

    def get_by_string(self, token_str: str) -> TokenID | None:
        """Get or create TokenID from string representation."""
        if token_str in self._cache:
            return self._cache[token_str]

        data = self._db.get_token(token_str)
        if data:
            token = TokenID.from_string(token_str)
            self._cache[token_str] = token
            return token
        return None

    def exists(self, token: TokenID) -> bool:
        """Check if a token is registered."""
        return token.to_string() in self._cache or self._db.get_token(token.to_string()) is not None

    def count(self) -> int:
        """Get total number of registered tokens."""
        return self._db.token_count()

    def all_tokens_by_mode(self, mode: int) -> Iterator[TokenID]:
        """Iterate over all tokens of a specific mode."""
        conn = self._db.connect()
        rows = conn.execute(
            "SELECT token_id, mode, value FROM tokens WHERE mode = ?",
            (mode,),
        ).fetchall()
        for row in rows:
            yield TokenID.from_string(row["token_id"])

    @property
    def db(self) -> TokenDatabase:
        """Access underlying database."""
        return self._db


def ensure_byte_tokens(store: TokenStore) -> None:
    """Pre-register all 256 byte tokens."""
    for i in range(256):
        store.register_byte(i)
