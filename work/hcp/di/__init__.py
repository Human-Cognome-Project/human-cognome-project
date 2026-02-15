"""
DI Framework - Digital Intelligence from the ground up.

A complete DI runtime with no external dependencies.
Pure PBM-based cognition.

Usage:
    from work.hcp.di import DI

    # Create new DI
    di = DI.create(
        name="explorer",
        core_values=["curiosity", "understanding"],
        description="An explorer of ideas"
    )

    # Process input
    response = di.process("What are you curious about?")
    print(response.text)

    # Load existing DI
    di = DI.load("./di_data")
"""

from .bootstrap import (
    IdentitySeed,
    create_di,
    create_explorer,
    create_helper,
    create_guardian,
)

from .experience import (
    ExperienceProcessor,
    Tokenizer,
    ConceptExtractor,
    Interaction,
)

from .memory import (
    Memory,
    MemoryStats,
)

from .runtime import (
    DI,
    DIState,
    Thought,
    Response,
    repl,
)

__all__ = [
    # Bootstrap
    "IdentitySeed",
    "create_di",
    "create_explorer",
    "create_helper",
    "create_guardian",
    # Experience
    "ExperienceProcessor",
    "Tokenizer",
    "ConceptExtractor",
    "Interaction",
    # Memory
    "Memory",
    "MemoryStats",
    # Runtime
    "DI",
    "DIState",
    "Thought",
    "Response",
    "repl",
]
