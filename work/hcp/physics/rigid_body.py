"""
Rigid Body: known, stable token structures.

Rigid bodies are well-formed patterns that resist deformation:
- Dictionary words
- Common phrases
- Known identifiers
"""
from __future__ import annotations

from dataclasses import dataclass
from typing import Iterator

from ..core.token_id import TokenID
from ..core.pair_bond import PairBondMap


@dataclass(frozen=True)
class RigidBody:
    """A known, stable token structure."""
    tokens: tuple[TokenID, ...]
    text: str  # Human-readable form
    frequency: int = 1  # How often this pattern occurs
    category: str = "word"  # 'word', 'phrase', 'idiom', etc.

    def __hash__(self) -> int:
        return hash(self.tokens)

    def __eq__(self, other: object) -> bool:
        if isinstance(other, RigidBody):
            return self.tokens == other.tokens
        return False

    def to_pbm(self) -> PairBondMap:
        """Convert to PairBondMap."""
        pbm = PairBondMap()
        pbm.add_sequence(list(self.tokens))
        return pbm


class RigidBodyRegistry:
    """
    Registry of known rigid bodies (dictionary).

    Provides lookup for spell checking and pattern matching.
    """

    def __init__(self) -> None:
        self._by_text: dict[str, RigidBody] = {}
        self._by_first_token: dict[TokenID, list[RigidBody]] = {}
        self._by_length: dict[int, list[RigidBody]] = {}

    def register(self, body: RigidBody) -> None:
        """Register a rigid body."""
        self._by_text[body.text.lower()] = body

        if body.tokens:
            first = body.tokens[0]
            if first not in self._by_first_token:
                self._by_first_token[first] = []
            self._by_first_token[first].append(body)

        length = len(body.tokens)
        if length not in self._by_length:
            self._by_length[length] = []
        self._by_length[length].append(body)

    def register_word(self, word: str, frequency: int = 1) -> RigidBody:
        """Register a word as a rigid body (using byte tokens)."""
        tokens = tuple(TokenID.byte(b) for b in word.encode("utf-8"))
        body = RigidBody(
            tokens=tokens,
            text=word,
            frequency=frequency,
            category="word",
        )
        self.register(body)
        return body

    def register_words(self, words: list[str]) -> None:
        """Register multiple words."""
        for word in words:
            self.register_word(word)

    def lookup(self, text: str) -> RigidBody | None:
        """Look up a rigid body by text."""
        return self._by_text.get(text.lower())

    def is_known(self, text: str) -> bool:
        """Check if text matches a known rigid body."""
        return text.lower() in self._by_text

    def find_by_prefix(self, token: TokenID) -> list[RigidBody]:
        """Find rigid bodies starting with a given token."""
        return self._by_first_token.get(token, [])

    def find_by_length(self, length: int) -> list[RigidBody]:
        """Find rigid bodies of a specific length."""
        return self._by_length.get(length, [])

    def find_similar(self, text: str, max_distance: int = 2) -> list[tuple[RigidBody, int]]:
        """
        Find rigid bodies within edit distance of text.

        Returns list of (body, distance) tuples sorted by:
        1. Edit distance (lower is better)
        2. First letter match (0 if match, 1 if not)
        3. Length difference (smaller is better)
        4. Frequency (higher is better)
        """
        from .energy import edit_distance

        text_lower = text.lower()
        text_len = len(text_lower)
        first_char = text_lower[0] if text_lower else ""
        results = []

        for known_text, body in self._by_text.items():
            dist = edit_distance(text_lower, known_text)
            if dist <= max_distance:
                len_diff = abs(len(known_text) - text_len)
                first_match = 0 if (known_text and known_text[0] == first_char) else 1
                results.append((body, dist, first_match, len_diff))

        # Sort by: distance, first letter match, length diff, negative frequency
        results.sort(key=lambda x: (x[1], x[2], x[3], -x[0].frequency))
        return [(body, dist) for body, dist, _, _ in results]

    def find_corrections(self, text: str, max_results: int = 5) -> list[str]:
        """Find spelling corrections for text."""
        similar = self.find_similar(text, max_distance=2)
        return [body.text for body, _ in similar[:max_results]]

    def all_bodies(self) -> Iterator[RigidBody]:
        """Iterate over all registered rigid bodies."""
        return iter(self._by_text.values())

    def count(self) -> int:
        """Return total number of registered rigid bodies."""
        return len(self._by_text)


# Common English words for basic spell checking
COMMON_WORDS = [
    # Articles, prepositions, conjunctions
    "the", "a", "an", "and", "or", "but", "in", "on", "at", "to", "for",
    "of", "with", "by", "from", "as", "is", "was", "are", "were", "be",
    "been", "being", "have", "has", "had", "do", "does", "did", "will",
    "would", "could", "should", "may", "might", "must", "can", "this",
    "that", "these", "those", "it", "its", "if", "then", "than", "so",
    "not", "no", "yes", "all", "any", "some", "many", "much", "more",
    "most", "other", "another", "such", "what", "which", "who", "whom",
    "whose", "when", "where", "why", "how", "each", "every", "both",
    "few", "several", "own", "same", "different",

    # Common verbs
    "go", "goes", "going", "gone", "went", "come", "comes", "coming",
    "came", "get", "gets", "getting", "got", "make", "makes", "making",
    "made", "take", "takes", "taking", "took", "taken", "see", "sees",
    "seeing", "saw", "seen", "know", "knows", "knowing", "knew", "known",
    "think", "thinks", "thinking", "thought", "want", "wants", "wanting",
    "wanted", "use", "uses", "using", "used", "find", "finds", "finding",
    "found", "give", "gives", "giving", "gave", "given", "tell", "tells",
    "telling", "told", "say", "says", "saying", "said", "work", "works",
    "working", "worked", "seem", "seems", "seeming", "seemed", "feel",
    "feels", "feeling", "felt", "try", "tries", "trying", "tried",
    "leave", "leaves", "leaving", "left", "call", "calls", "calling",
    "called", "keep", "keeps", "keeping", "kept", "let", "lets", "letting",
    "begin", "begins", "beginning", "began", "begun", "show", "shows",
    "showing", "showed", "shown", "hear", "hears", "hearing", "heard",
    "play", "plays", "playing", "played", "run", "runs", "running", "ran",
    "move", "moves", "moving", "moved", "live", "lives", "living", "lived",
    "believe", "believes", "believing", "believed",

    # Common nouns
    "time", "year", "people", "way", "day", "man", "woman", "child",
    "world", "life", "hand", "part", "place", "case", "week", "company",
    "system", "program", "question", "work", "government", "number",
    "night", "point", "home", "water", "room", "mother", "area", "money",
    "story", "fact", "month", "lot", "right", "study", "book", "eye",
    "job", "word", "business", "issue", "side", "kind", "head", "house",
    "service", "friend", "father", "power", "hour", "game", "line",
    "end", "member", "law", "car", "city", "community", "name", "team",

    # Common adjectives
    "good", "new", "first", "last", "long", "great", "little", "own",
    "old", "right", "big", "high", "small", "large", "next", "early",
    "young", "important", "few", "public", "bad", "same", "able",
    "human", "local", "late", "hard", "major", "better", "economic",
    "strong", "possible", "whole", "free", "military", "true", "federal",
    "international", "full", "special", "easy", "clear", "recent",
    "certain", "personal", "open", "red", "difficult", "available",
    "likely", "short", "single", "medical", "current", "wrong", "private",
    "past", "foreign", "fine", "common", "poor", "natural", "significant",

    # The pangram words
    "quick", "brown", "fox", "jumps", "over", "lazy", "dog",

    # Common greetings and interjections
    "hello", "hi", "hey", "goodbye", "bye", "thanks", "thank", "please",
    "sorry", "excuse", "welcome", "okay", "ok", "yes", "no", "yeah",
]


def create_default_registry() -> RigidBodyRegistry:
    """Create a registry with common English words."""
    registry = RigidBodyRegistry()
    registry.register_words(COMMON_WORDS)
    return registry
