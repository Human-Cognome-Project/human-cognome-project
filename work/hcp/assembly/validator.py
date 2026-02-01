"""
Validator: verify lossless reconstruction via hash comparison.

Ensures byte-perfect reconstruction from PBM.
"""
from __future__ import annotations

import hashlib
from dataclasses import dataclass
from typing import Sequence

from ..core.token_id import TokenID
from ..core.pair_bond import PairBondMap
from .reconstructor import Reconstructor, ReconstructionResult


@dataclass
class ValidationResult:
    """Result of a validation check."""
    valid: bool
    original_hash: str
    reconstructed_hash: str
    original_length: int
    reconstructed_length: int
    reconstruction_method: str
    error_message: str | None = None

    @property
    def match(self) -> bool:
        """Check if hashes match."""
        return self.original_hash == self.reconstructed_hash

    def __str__(self) -> str:
        status = "VALID" if self.valid else "INVALID"
        msg = f"[{status}] "
        if self.valid:
            msg += f"Hash: {self.original_hash[:16]}... ({self.original_length} bytes)"
        else:
            msg += f"Original: {self.original_hash[:16]}..., "
            msg += f"Reconstructed: {self.reconstructed_hash[:16]}..."
            if self.error_message:
                msg += f" - {self.error_message}"
        return msg


class Validator:
    """
    Validate that reconstruction is lossless.

    Uses SHA-256 hashes to verify byte-perfect reconstruction.
    """

    def __init__(self, reconstructor: Reconstructor | None = None) -> None:
        self.reconstructor = reconstructor or Reconstructor()

    @staticmethod
    def compute_hash(data: bytes) -> str:
        """Compute SHA-256 hash of data."""
        return hashlib.sha256(data).hexdigest()

    def validate(
        self,
        original: bytes,
        pbm: PairBondMap,
    ) -> ValidationResult:
        """
        Validate that PBM can be reconstructed to original bytes.
        """
        original_hash = self.compute_hash(original)

        try:
            result = self.reconstructor.reconstruct(pbm)
            reconstructed = result.to_bytes()
            reconstructed_hash = self.compute_hash(reconstructed)

            return ValidationResult(
                valid=original_hash == reconstructed_hash,
                original_hash=original_hash,
                reconstructed_hash=reconstructed_hash,
                original_length=len(original),
                reconstructed_length=len(reconstructed),
                reconstruction_method=result.method,
            )
        except Exception as e:
            return ValidationResult(
                valid=False,
                original_hash=original_hash,
                reconstructed_hash="",
                original_length=len(original),
                reconstructed_length=0,
                reconstruction_method="failed",
                error_message=str(e),
            )

    def validate_text(self, original: str, pbm: PairBondMap) -> ValidationResult:
        """Validate text reconstruction."""
        return self.validate(original.encode("utf-8"), pbm)

    def validate_roundtrip(
        self,
        original: bytes,
        tokenize_fn,
    ) -> ValidationResult:
        """
        Validate complete roundtrip: bytes -> tokens -> PBM -> tokens -> bytes.

        Args:
            original: Original bytes
            tokenize_fn: Function that takes bytes and returns (tokens, pbm)
        """
        tokens, pbm = tokenize_fn(original)
        return self.validate(original, pbm)


class RoundtripValidator:
    """
    Validate complete tokenization-reconstruction roundtrip.
    """

    def __init__(self) -> None:
        from ..atomizer.byte_atomizer import ByteAtomizer
        self._atomizer = ByteAtomizer()

    def validate_bytes(self, data: bytes) -> ValidationResult:
        """Validate byte-level roundtrip."""
        from ..core.pair_bond import create_pbm_from_text

        # Create PBM using byte-level tokens
        pbm = self._atomizer.to_pbm(data)

        validator = Validator()
        return validator.validate(data, pbm)

    def validate_text(self, text: str) -> ValidationResult:
        """Validate text roundtrip."""
        return self.validate_bytes(text.encode("utf-8"))

    def validate_all_bytes(self) -> list[ValidationResult]:
        """
        Validate that all 256 byte values can be roundtripped.

        Returns list of results for any failures.
        """
        failures = []
        for i in range(256):
            data = bytes([i])
            result = self.validate_bytes(data)
            if not result.valid:
                failures.append((i, result))

        return failures


def validate_reconstruction(original: bytes, pbm: PairBondMap) -> bool:
    """Convenience function: check if reconstruction is valid."""
    validator = Validator()
    return validator.validate(original, pbm).valid


def validate_text_roundtrip(text: str) -> ValidationResult:
    """Convenience function: validate text roundtrip."""
    validator = RoundtripValidator()
    return validator.validate_text(text)


def assert_lossless(original: bytes, pbm: PairBondMap) -> None:
    """Assert that reconstruction is lossless, raise if not."""
    result = Validator().validate(original, pbm)
    if not result.valid:
        raise AssertionError(f"Lossy reconstruction: {result}")
