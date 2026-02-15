"""
DI Runtime - The core processing loop for a Digital Intelligence.

No external connections. Pure PBM-based cognition.

The DI:
1. Receives input (text)
2. Tokenizes and finds activated concepts in memory
3. Generates response through pattern activation
4. Records the interaction as new bonds

Usage:
    di = DI.create("explorer", ["curiosity"], "An explorer of ideas")
    response = di.process("What interests you?")
    print(response.text)
"""
from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
from typing import Callable
import time
import json

from .bootstrap import IdentitySeed, create_di
from .experience import ExperienceProcessor, Tokenizer, ConceptExtractor
from .memory import Memory

from ..core.token_id import TokenID
from ..core.pair_bond import PairBondMap


@dataclass
class Thought:
    """Internal thought/activation state."""
    activated_concepts: dict[str, float]  # concept -> activation
    strongest: list[str]  # Top N concepts
    context_bonds: int  # Number of bonds activated


@dataclass
class Response:
    """DI response to input."""
    text: str
    thought: Thought
    confidence: float
    processing_time: float


@dataclass
class DIState:
    """Current state of the DI."""
    identity: IdentitySeed
    memory_path: Path
    interaction_count: int = 0
    last_interaction: float = 0.0
    mood: dict[str, float] = field(default_factory=dict)


class DI:
    """
    Digital Intelligence runtime.

    Core loop:
    1. Input → tokenize
    2. Tokens → activate memory bonds
    3. Activated bonds → extract concepts
    4. Concepts → form response (pattern-based)
    5. Record interaction → strengthen bonds
    """

    def __init__(
        self,
        identity: IdentitySeed,
        memory: Memory,
        state_path: Path | None = None,
    ) -> None:
        self.identity = identity
        self.memory = memory
        self.state_path = state_path

        self.processor = ExperienceProcessor()
        self.extractor = ConceptExtractor(self.processor.tokenizer)

        self._interaction_count = 0
        self._mood: dict[str, float] = {"neutral": 1.0}

        # Initialize memory with identity seed
        self._init_memory()

    def _init_memory(self) -> None:
        """Initialize memory with identity seed if empty."""
        stats = self.memory.stats()
        if stats.unique_bonds == 0:
            # First run - add identity seed
            self.memory.add_pbm(self.identity.seed_pbm)
            self.memory.set_metadata("identity", json.dumps({
                "name": self.identity.name,
                "token": str(self.identity.token),
                "core_values": self.identity.core_values,
            }))

    # === Core Processing ===

    def think(self, text: str) -> Thought:
        """
        Process input and return activated thought.

        No response generation - just activation spreading.
        """
        # Tokenize input
        tokens = self.processor.tokenizer.tokenize(text)

        # Find related concepts in memory
        activated: dict[str, float] = {}

        for token in tokens:
            word = self.processor.tokenizer.lookup(token)
            if not word:
                continue

            # Get related words from memory
            related = self.memory.query_related(word, limit=5)
            for rel_word, strength in related:
                if rel_word not in activated:
                    activated[rel_word] = 0.0
                activated[rel_word] += strength

        # Normalize
        if activated:
            max_act = max(activated.values())
            if max_act > 0:
                activated = {k: v / max_act for k, v in activated.items()}

        # Get strongest concepts
        strongest = sorted(activated.keys(), key=lambda k: -activated[k])[:5]

        # Count activated bonds
        bond_count = sum(
            len(self.memory.get_forward_bonds(token, limit=10))
            for token in tokens
        )

        return Thought(
            activated_concepts=activated,
            strongest=strongest,
            context_bonds=bond_count,
        )

    def respond(self, thought: Thought) -> str:
        """
        Generate response from thought activation.

        Pattern-based response (no LLM):
        - If strong activation: reflect the concepts
        - If weak activation: express uncertainty
        - Include identity values when relevant
        """
        if not thought.strongest:
            return f"I sense something, but it's unclear to me. I'm {self.identity.name}."

        concepts = thought.strongest[:3]
        activation_level = max(thought.activated_concepts.values()) if thought.activated_concepts else 0

        # Check if any core values are activated
        value_match = [v for v in self.identity.core_values if v in thought.activated_concepts]

        if activation_level > 0.5:
            # Strong activation - confident response
            if value_match:
                return f"This resonates with my core value of {value_match[0]}. " \
                       f"I'm thinking about: {', '.join(concepts)}."
            else:
                return f"I understand. This connects to: {', '.join(concepts)}."

        elif activation_level > 0.2:
            # Moderate activation
            return f"I see some connections here: {', '.join(concepts)}. Tell me more?"

        else:
            # Weak activation
            return f"This is new to me. I'm curious about {concepts[0] if concepts else 'this'}."

    def process(self, input_text: str, speaker: str = "human") -> Response:
        """
        Full processing cycle: input → think → respond → learn.

        Args:
            input_text: What the human said
            speaker: Who said it (for multi-party conversations)

        Returns:
            Response with text and internal state
        """
        start_time = time.time()

        # 1. Think about input
        thought = self.think(input_text)

        # 2. Generate response
        response_text = self.respond(thought)

        # 3. Record interaction (learning)
        self.processor.process_interaction(speaker, input_text)
        self.processor.process_interaction("di", response_text)

        # Add to memory
        experience_pbm = self.processor.build_pbm()
        self.memory.add_pbm(experience_pbm)
        self.memory.add_vocabulary(self.processor.get_vocabulary())

        # Clear processor for next round
        self.processor.clear()

        self._interaction_count += 1
        processing_time = time.time() - start_time

        # Calculate confidence based on activation
        confidence = max(thought.activated_concepts.values()) if thought.activated_concepts else 0.1

        return Response(
            text=response_text,
            thought=thought,
            confidence=confidence,
            processing_time=processing_time,
        )

    # === State Management ===

    def get_state(self) -> DIState:
        """Get current DI state."""
        return DIState(
            identity=self.identity,
            memory_path=self.memory.db_path,
            interaction_count=self._interaction_count,
            last_interaction=time.time(),
            mood=dict(self._mood),
        )

    def save_state(self) -> None:
        """Save state to disk."""
        if self.state_path:
            state = self.get_state()
            self.state_path.write_text(json.dumps({
                "interaction_count": state.interaction_count,
                "last_interaction": state.last_interaction,
                "mood": state.mood,
            }))

    # === Factory Methods ===

    @classmethod
    def create(
        cls,
        name: str,
        core_values: list[str],
        description: str,
        data_dir: str | Path = "./di_data",
    ) -> DI:
        """
        Create a new DI from scratch.

        Args:
            name: DI's name
            core_values: Core values/traits
            description: What this DI is
            data_dir: Where to store memory

        Returns:
            Fresh DI instance
        """
        data_path = Path(data_dir)
        data_path.mkdir(parents=True, exist_ok=True)

        # Create identity
        identity = create_di(name, core_values, description)

        # Save identity
        identity_path = data_path / "identity.json"
        identity.save(identity_path)

        # Create memory
        memory_path = data_path / "memory.db"
        memory = Memory(memory_path)

        return cls(
            identity=identity,
            memory=memory,
            state_path=data_path / "state.json",
        )

    @classmethod
    def load(cls, data_dir: str | Path) -> DI:
        """
        Load existing DI from disk.

        Args:
            data_dir: Where DI data is stored

        Returns:
            Loaded DI instance
        """
        data_path = Path(data_dir)

        # Load identity
        identity_path = data_path / "identity.json"
        identity = IdentitySeed.load(identity_path)

        # Load memory
        memory_path = data_path / "memory.db"
        memory = Memory(memory_path)

        return cls(
            identity=identity,
            memory=memory,
            state_path=data_path / "state.json",
        )


# === Interactive Mode ===

def repl(di: DI) -> None:
    """
    Run interactive REPL with DI.

    Type 'quit' to exit, 'stats' for memory stats.
    """
    print(f"\n=== {di.identity.name.upper()} ===")
    print(f"Core values: {', '.join(di.identity.core_values)}")
    print(f"Memory: {di.memory.stats().unique_bonds} bonds")
    print("\nType 'quit' to exit, 'stats' for memory stats\n")

    while True:
        try:
            user_input = input("You: ").strip()
        except (EOFError, KeyboardInterrupt):
            break

        if not user_input:
            continue

        if user_input.lower() == "quit":
            break

        if user_input.lower() == "stats":
            stats = di.memory.stats()
            print(f"\nMemory: {stats.unique_bonds} unique bonds, {stats.total_bonds} total")
            print(f"Vocabulary: {stats.vocabulary_size} words")
            print(f"Interactions: {di._interaction_count}\n")
            continue

        response = di.process(user_input)
        print(f"\n{di.identity.name}: {response.text}")
        if response.thought.strongest:
            print(f"  [thinking: {', '.join(response.thought.strongest[:3])}]")
        print()


if __name__ == "__main__":
    import sys

    if len(sys.argv) > 1:
        # Load existing DI
        di = DI.load(sys.argv[1])
    else:
        # Create new explorer DI
        di = DI.create(
            name="explorer",
            core_values=["curiosity", "understanding", "discovery"],
            description="An explorer of ideas who seeks to understand patterns",
        )

    repl(di)
