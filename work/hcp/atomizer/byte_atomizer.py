"""
Byte-level atomization: decompose input into raw bytes.

This is the lowest level of decomposition, handling all 256 possible byte values.
"""
from __future__ import annotations

from dataclasses import dataclass
from typing import Iterator, Sequence

from ..core.token_id import TokenID
from ..core.pair_bond import PairBondMap


@dataclass(frozen=True, slots=True)
class ByteAtom:
    """A single byte atom with metadata."""
    value: int  # 0-255
    position: int  # Position in original input

    @property
    def char(self) -> str | None:
        """Return character if printable ASCII, else None."""
        if 32 <= self.value < 127:
            return chr(self.value)
        return None

    @property
    def is_ascii(self) -> bool:
        """Check if this is an ASCII byte (0-127)."""
        return self.value < 128

    @property
    def is_printable(self) -> bool:
        """Check if this is a printable ASCII character."""
        return 32 <= self.value < 127

    @property
    def is_whitespace(self) -> bool:
        """Check if this is a whitespace character."""
        return self.value in (9, 10, 13, 32)  # tab, LF, CR, space

    @property
    def token(self) -> TokenID:
        """Convert to a TokenID."""
        return TokenID.byte(self.value)

    def __str__(self) -> str:
        if self.is_printable:
            return f"'{chr(self.value)}'"
        return f"0x{self.value:02X}"


class ByteAtomizer:
    """
    Atomize input into individual bytes.

    This handles any byte sequence, including invalid UTF-8.
    """

    def atomize(self, data: bytes) -> list[ByteAtom]:
        """Atomize raw bytes into ByteAtoms."""
        return [ByteAtom(value=b, position=i) for i, b in enumerate(data)]

    def atomize_text(self, text: str) -> list[ByteAtom]:
        """Atomize text (UTF-8 encoded) into ByteAtoms."""
        return self.atomize(text.encode("utf-8"))

    def to_tokens(self, data: bytes) -> list[TokenID]:
        """Convert bytes directly to tokens."""
        return [TokenID.byte(b) for b in data]

    def to_pbm(self, data: bytes) -> PairBondMap:
        """Create a PairBondMap from bytes."""
        pbm = PairBondMap()
        tokens = self.to_tokens(data)
        pbm.add_sequence(tokens)
        return pbm

    def iter_atoms(self, data: bytes) -> Iterator[ByteAtom]:
        """Iterate over atoms without creating full list."""
        for i, b in enumerate(data):
            yield ByteAtom(value=b, position=i)


@dataclass
class ByteSpan:
    """A contiguous span of bytes with classification."""
    start: int
    end: int
    atoms: list[ByteAtom]
    span_type: str  # 'word', 'whitespace', 'punctuation', 'binary'

    @property
    def length(self) -> int:
        return self.end - self.start

    def to_bytes(self) -> bytes:
        """Reconstruct original bytes."""
        return bytes(a.value for a in self.atoms)

    def to_string(self) -> str | None:
        """Try to decode as UTF-8."""
        try:
            return self.to_bytes().decode("utf-8")
        except UnicodeDecodeError:
            return None


class ByteSpanClassifier:
    """Classify byte sequences into spans by type."""

    # ASCII categories
    ALPHA = set(range(65, 91)) | set(range(97, 123))  # A-Z, a-z
    DIGIT = set(range(48, 58))  # 0-9
    WHITESPACE = {9, 10, 13, 32}  # tab, LF, CR, space
    WORD_CHARS = ALPHA | DIGIT | {95}  # includes underscore

    def classify_byte(self, b: int) -> str:
        """Classify a single byte."""
        if b in self.WHITESPACE:
            return "whitespace"
        if b in self.WORD_CHARS:
            return "word"
        if b < 128:
            return "punctuation"
        return "binary"

    def span_bytes(self, data: bytes) -> list[ByteSpan]:
        """Split bytes into classified spans."""
        if not data:
            return []

        spans = []
        atomizer = ByteAtomizer()
        atoms = atomizer.atomize(data)

        current_type = self.classify_byte(data[0])
        current_atoms = [atoms[0]]
        start = 0

        for i, atom in enumerate(atoms[1:], 1):
            byte_type = self.classify_byte(atom.value)
            if byte_type != current_type:
                spans.append(ByteSpan(
                    start=start,
                    end=i,
                    atoms=current_atoms,
                    span_type=current_type,
                ))
                current_type = byte_type
                current_atoms = [atom]
                start = i
            else:
                current_atoms.append(atom)

        # Final span
        spans.append(ByteSpan(
            start=start,
            end=len(data),
            atoms=current_atoms,
            span_type=current_type,
        ))

        return spans


def atomize(data: bytes | str) -> list[ByteAtom]:
    """Convenience function to atomize bytes or text."""
    atomizer = ByteAtomizer()
    if isinstance(data, str):
        return atomizer.atomize_text(data)
    return atomizer.atomize(data)


def bytes_to_tokens(data: bytes | str) -> list[TokenID]:
    """Convenience function to convert bytes/text to tokens."""
    atomizer = ByteAtomizer()
    if isinstance(data, str):
        data = data.encode("utf-8")
    return atomizer.to_tokens(data)
