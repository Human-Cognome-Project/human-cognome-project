"""
Experience Processing - Convert interactions to PBM bonds.

Every interaction becomes part of the DI's memory through bonds.
No external dependencies.

Usage:
    exp = ExperienceProcessor()
    exp.process_interaction(speaker="human", content="Hello, how are you?")
    exp.process_interaction(speaker="di", content="I am well, curious about your day")
    pbm = exp.get_experience_pbm()
"""
from __future__ import annotations

from dataclasses import dataclass, field
from typing import Iterator
from collections import defaultdict
import hashlib
import time
import re

from ..core.token_id import TokenID
from ..core.pair_bond import PairBondMap


# Token namespaces
WORD_PREFIX = (0, 0, 0, 3)      # Words: 0.0.0.3.{hash}
SPEAKER_PREFIX = (0, 0, 0, 4)   # Speakers: 0.0.0.4.{id}
TIME_PREFIX = (0, 0, 0, 5)      # Time markers: 0.0.0.5.{bucket}
CONCEPT_PREFIX = (0, 0, 0, 6)   # Abstract concepts: 0.0.0.6.{id}


@dataclass
class Interaction:
    """A single interaction (utterance)."""
    speaker: str
    content: str
    timestamp: float
    tokens: list[TokenID] = field(default_factory=list)


class Tokenizer:
    """
    Simple word-level tokenizer with vocabulary tracking.

    Maps words to stable TokenIDs across sessions.
    """

    def __init__(self) -> None:
        self._word_to_token: dict[str, TokenID] = {}
        self._token_to_word: dict[TokenID, str] = {}

    def tokenize(self, text: str) -> list[TokenID]:
        """Convert text to tokens."""
        # Extract words (lowercase, alphanumeric + apostrophes)
        words = re.findall(r"[a-z]+(?:'[a-z]+)?", text.lower())
        tokens = []

        for word in words:
            if word not in self._word_to_token:
                # Hash-based token ID (consistent across runs)
                word_hash = int(hashlib.md5(word.encode()).hexdigest()[:4], 16)
                token = TokenID(WORD_PREFIX + (word_hash,))
                self._word_to_token[word] = token
                self._token_to_word[token] = word

            tokens.append(self._word_to_token[word])

        return tokens

    def lookup(self, token: TokenID) -> str | None:
        """Get word for token."""
        return self._token_to_word.get(token)

    def reverse_lookup(self, word: str) -> TokenID | None:
        """Get token for word."""
        return self._word_to_token.get(word.lower())

    @property
    def vocabulary(self) -> dict[str, TokenID]:
        return dict(self._word_to_token)


class ExperienceProcessor:
    """
    Process interactions into PBM bonds.

    Creates bonds:
    - Sequential word bonds (word -> next_word)
    - Speaker bonds (speaker -> words they said)
    - Temporal bonds (time_bucket -> words)
    - Cross-utterance bonds (response words -> previous words)
    """

    def __init__(self) -> None:
        self.tokenizer = Tokenizer()
        self.interactions: list[Interaction] = []
        self._speaker_tokens: dict[str, TokenID] = {}

    def _get_speaker_token(self, speaker: str) -> TokenID:
        """Get or create token for speaker."""
        if speaker not in self._speaker_tokens:
            speaker_hash = int(hashlib.md5(speaker.encode()).hexdigest()[:2], 16)
            self._speaker_tokens[speaker] = TokenID(SPEAKER_PREFIX + (speaker_hash,))
        return self._speaker_tokens[speaker]

    def _get_time_token(self, timestamp: float) -> TokenID:
        """Get time bucket token (hourly buckets)."""
        hour_bucket = int(timestamp // 3600)
        return TokenID(TIME_PREFIX + (hour_bucket % 10000,))

    def process_interaction(
        self,
        speaker: str,
        content: str,
        timestamp: float | None = None,
    ) -> Interaction:
        """
        Process a single interaction.

        Args:
            speaker: Who said this ("human", "di", or custom name)
            content: What was said
            timestamp: When (defaults to now)

        Returns:
            Processed Interaction
        """
        ts = timestamp or time.time()
        tokens = self.tokenizer.tokenize(content)

        interaction = Interaction(
            speaker=speaker,
            content=content,
            timestamp=ts,
            tokens=tokens,
        )

        self.interactions.append(interaction)
        return interaction

    def build_pbm(self) -> PairBondMap:
        """
        Build PBM from all processed interactions.

        Creates multiple types of bonds:
        1. Sequential: word -> next_word (within utterance)
        2. Speaker: speaker_token -> each word
        3. Context: last words of prev utterance -> first words of current
        """
        pbm = PairBondMap()

        prev_interaction: Interaction | None = None

        for interaction in self.interactions:
            tokens = interaction.tokens
            speaker_token = self._get_speaker_token(interaction.speaker)

            # Sequential bonds within utterance
            for i in range(len(tokens) - 1):
                pbm.add_bond(tokens[i], tokens[i + 1])

            # Speaker -> word bonds
            for token in tokens:
                pbm.add_bond(speaker_token, token)

            # Cross-utterance context bonds
            if prev_interaction and prev_interaction.tokens and tokens:
                # Last 3 words of previous -> first 3 words of current
                prev_tail = prev_interaction.tokens[-3:]
                curr_head = tokens[:3]

                for p_tok in prev_tail:
                    for c_tok in curr_head:
                        pbm.add_bond(p_tok, c_tok)

            prev_interaction = interaction

        return pbm

    def get_vocabulary(self) -> dict[str, TokenID]:
        """Get the vocabulary built from interactions."""
        return self.tokenizer.vocabulary

    def clear(self) -> None:
        """Clear all interactions (but keep vocabulary)."""
        self.interactions.clear()


class ConceptExtractor:
    """
    Extract abstract concepts from interactions.

    Identifies patterns like:
    - Repeated words (salient concepts)
    - Question patterns (curiosity markers)
    - Emotion words (sentiment markers)
    """

    # Simple emotion/concept word lists
    POSITIVE = {"good", "great", "happy", "love", "like", "yes", "thanks", "helpful"}
    NEGATIVE = {"bad", "wrong", "no", "hate", "dislike", "problem", "error", "fail"}
    QUESTION = {"what", "why", "how", "when", "where", "who", "which", "can", "could"}

    def __init__(self, tokenizer: Tokenizer) -> None:
        self.tokenizer = tokenizer

    def extract_concepts(self, interactions: list[Interaction]) -> dict[str, float]:
        """
        Extract concepts with salience scores.

        Returns dict of concept -> salience (0.0 to 1.0)
        """
        word_counts: dict[str, int] = defaultdict(int)
        total_words = 0

        for interaction in interactions:
            for token in interaction.tokens:
                word = self.tokenizer.lookup(token)
                if word:
                    word_counts[word] += 1
                    total_words += 1

        if total_words == 0:
            return {}

        # Normalize and filter
        concepts = {}
        for word, count in word_counts.items():
            # Skip very common words
            if len(word) <= 2:
                continue

            salience = count / total_words

            # Boost conceptual words
            if word in self.POSITIVE or word in self.NEGATIVE:
                salience *= 2.0
            if word in self.QUESTION:
                salience *= 1.5

            if salience > 0.01:  # Threshold
                concepts[word] = min(1.0, salience)

        return concepts
