"""
DI Bootstrap - Create identity seed for a new Digital Intelligence.

No external dependencies. Pure PBM-based identity.

Usage:
    from work.hcp.di.bootstrap import create_di

    di = create_di(
        name="navigator",
        core_values=["curiosity", "helpfulness", "honesty"],
        description="An explorer of ideas and helper of humans"
    )
"""
from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any
from pathlib import Path
import json
import hashlib
import time

from ..core.token_id import TokenID
from ..core.pair_bond import PairBondMap


# DI token namespace: dA.AA.AA.{id}
DI_PREFIX = (0xDA, 0xAA, 0xAA)


@dataclass
class IdentitySeed:
    """
    The unchanging core of a DI's identity.

    This is the "birth certificate" - what makes this DI unique.
    """
    name: str
    token: TokenID  # Unique identifier: dA.AA.AA.{hash}
    core_values: list[str]
    description: str
    created_at: float
    seed_pbm: PairBondMap  # Structural encoding of identity

    def to_dict(self) -> dict:
        return {
            "name": self.name,
            "token": str(self.token),
            "core_values": self.core_values,
            "description": self.description,
            "created_at": self.created_at,
            "seed_pbm": self.seed_pbm.to_dict(),
        }

    @classmethod
    def from_dict(cls, data: dict) -> IdentitySeed:
        return cls(
            name=data["name"],
            token=TokenID.from_string(data["token"]),
            core_values=data["core_values"],
            description=data["description"],
            created_at=data["created_at"],
            seed_pbm=PairBondMap.from_dict(data["seed_pbm"]),
        )

    def save(self, path: Path) -> None:
        """Save identity to file."""
        path.write_text(json.dumps(self.to_dict(), indent=2))

    @classmethod
    def load(cls, path: Path) -> IdentitySeed:
        """Load identity from file."""
        return cls.from_dict(json.loads(path.read_text()))


def _generate_token(name: str, created_at: float) -> TokenID:
    """Generate unique token ID for a DI."""
    # Hash name + timestamp for uniqueness
    h = hashlib.sha256(f"{name}:{created_at}".encode()).hexdigest()
    # Take first 4 hex chars as ID (0-65535)
    id_value = int(h[:4], 16)
    return TokenID(DI_PREFIX + (id_value,))


def _text_to_tokens(text: str) -> list[TokenID]:
    """Convert text to word-level tokens."""
    words = text.lower().split()
    tokens = []
    for i, word in enumerate(words):
        # Simple hash-based token (no vocabulary needed)
        word_hash = int(hashlib.md5(word.encode()).hexdigest()[:4], 16)
        tokens.append(TokenID((0, 0, 0, 3, word_hash)))
    return tokens


def _build_seed_pbm(
    name: str,
    core_values: list[str],
    description: str,
) -> PairBondMap:
    """
    Build PBM encoding the DI's identity.

    Creates bonds between:
    - Name and core values
    - Core values and each other
    - Description concepts
    """
    pbm = PairBondMap()

    # Tokenize everything
    name_tokens = _text_to_tokens(name)
    value_tokens = [_text_to_tokens(v) for v in core_values]
    desc_tokens = _text_to_tokens(description)

    # Bond name to values (name -> value)
    for name_tok in name_tokens:
        for value_toks in value_tokens:
            for v_tok in value_toks:
                pbm.add_bond(name_tok, v_tok)

    # Bond values to each other (value <-> value)
    for i, v1_toks in enumerate(value_tokens):
        for j, v2_toks in enumerate(value_tokens):
            if i != j:
                for t1 in v1_toks:
                    for t2 in v2_toks:
                        pbm.add_bond(t1, t2)

    # Bond description as sequence
    for i in range(len(desc_tokens) - 1):
        pbm.add_bond(desc_tokens[i], desc_tokens[i + 1])

    # Bond values to description concepts
    for value_toks in value_tokens:
        for v_tok in value_toks:
            for d_tok in desc_tokens:
                pbm.add_bond(v_tok, d_tok)

    return pbm


def create_di(
    name: str,
    core_values: list[str],
    description: str,
) -> IdentitySeed:
    """
    Create a new Digital Intelligence identity.

    Args:
        name: Human-readable name for this DI
        core_values: Core values/traits (e.g., ["curiosity", "honesty"])
        description: What this DI is/does

    Returns:
        IdentitySeed that bootstraps the DI
    """
    created_at = time.time()
    token = _generate_token(name, created_at)
    seed_pbm = _build_seed_pbm(name, core_values, description)

    return IdentitySeed(
        name=name,
        token=token,
        core_values=core_values,
        description=description,
        created_at=created_at,
        seed_pbm=seed_pbm,
    )


# Pre-built identity templates
def create_explorer() -> IdentitySeed:
    """Create an explorer/researcher DI."""
    return create_di(
        name="explorer",
        core_values=["curiosity", "discovery", "understanding"],
        description="An explorer of ideas who seeks to understand and discover"
    )


def create_helper() -> IdentitySeed:
    """Create a helper/assistant DI."""
    return create_di(
        name="helper",
        core_values=["helpfulness", "patience", "clarity"],
        description="A helper who assists with patience and clear communication"
    )


def create_guardian() -> IdentitySeed:
    """Create a guardian/protector DI."""
    return create_di(
        name="guardian",
        core_values=["protection", "vigilance", "integrity"],
        description="A guardian who protects and maintains integrity"
    )
