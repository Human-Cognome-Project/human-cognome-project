"""
Natural Semantic Metalanguage (NSM) Primitives.

Based on Wierzbicka and Goddard's cross-linguistic research.
These 65 semantic primes are hypothesized to be universal across all languages.

Mode-00 tokens represent these primitives.
"""
from __future__ import annotations

from dataclasses import dataclass
from enum import Enum
from typing import ClassVar

from .token_id import TokenID


class NSMCategory(Enum):
    """Categories of NSM primitives."""
    SUBSTANTIVES = "substantives"
    DETERMINERS = "determiners"
    QUANTIFIERS = "quantifiers"
    EVALUATORS = "evaluators"
    DESCRIPTORS = "descriptors"
    MENTAL_PREDICATES = "mental_predicates"
    SPEECH = "speech"
    ACTIONS_EVENTS = "actions_events"
    EXISTENCE_POSSESSION = "existence_possession"
    LIFE_DEATH = "life_death"
    TIME = "time"
    SPACE = "space"
    LOGICAL = "logical"
    INTENSIFIER = "intensifier"
    SIMILARITY = "similarity"


@dataclass(frozen=True)
class NSMPrimitive:
    """A single NSM semantic primitive."""
    index: int
    name: str
    category: NSMCategory
    explication: str  # Brief meaning description

    @property
    def token(self) -> TokenID:
        """Get the mode-00 token for this primitive."""
        return TokenID.nsm(self.index)

    def __str__(self) -> str:
        return self.name.upper()


# The 65 NSM semantic primitives
# Based on Goddard & Wierzbicka (2014) "Words and Meanings"
NSM_PRIMITIVES: list[NSMPrimitive] = [
    # Substantives (0-2)
    NSMPrimitive(0, "I", NSMCategory.SUBSTANTIVES, "First person singular"),
    NSMPrimitive(1, "you", NSMCategory.SUBSTANTIVES, "Second person"),
    NSMPrimitive(2, "someone", NSMCategory.SUBSTANTIVES, "Human being, person"),
    NSMPrimitive(3, "something", NSMCategory.SUBSTANTIVES, "Thing, entity"),
    NSMPrimitive(4, "people", NSMCategory.SUBSTANTIVES, "Human beings collectively"),
    NSMPrimitive(5, "body", NSMCategory.SUBSTANTIVES, "Physical body"),

    # Determiners (6-8)
    NSMPrimitive(6, "this", NSMCategory.DETERMINERS, "Proximal demonstrative"),
    NSMPrimitive(7, "the same", NSMCategory.DETERMINERS, "Identity"),
    NSMPrimitive(8, "other", NSMCategory.DETERMINERS, "Different, else"),

    # Quantifiers (9-13)
    NSMPrimitive(9, "one", NSMCategory.QUANTIFIERS, "Single, unity"),
    NSMPrimitive(10, "two", NSMCategory.QUANTIFIERS, "Dual"),
    NSMPrimitive(11, "some", NSMCategory.QUANTIFIERS, "Partial quantity"),
    NSMPrimitive(12, "all", NSMCategory.QUANTIFIERS, "Totality"),
    NSMPrimitive(13, "much/many", NSMCategory.QUANTIFIERS, "Large quantity"),

    # Evaluators (14-15)
    NSMPrimitive(14, "good", NSMCategory.EVALUATORS, "Positive evaluation"),
    NSMPrimitive(15, "bad", NSMCategory.EVALUATORS, "Negative evaluation"),

    # Descriptors (16-17)
    NSMPrimitive(16, "big", NSMCategory.DESCRIPTORS, "Large size"),
    NSMPrimitive(17, "small", NSMCategory.DESCRIPTORS, "Small size"),

    # Mental predicates (18-23)
    NSMPrimitive(18, "think", NSMCategory.MENTAL_PREDICATES, "Cognitive process"),
    NSMPrimitive(19, "know", NSMCategory.MENTAL_PREDICATES, "Knowledge state"),
    NSMPrimitive(20, "want", NSMCategory.MENTAL_PREDICATES, "Desire, volition"),
    NSMPrimitive(21, "feel", NSMCategory.MENTAL_PREDICATES, "Emotion, sensation"),
    NSMPrimitive(22, "see", NSMCategory.MENTAL_PREDICATES, "Visual perception"),
    NSMPrimitive(23, "hear", NSMCategory.MENTAL_PREDICATES, "Auditory perception"),

    # Speech (24-25)
    NSMPrimitive(24, "say", NSMCategory.SPEECH, "Verbal expression"),
    NSMPrimitive(25, "words", NSMCategory.SPEECH, "Language units"),

    # Actions, events, movement (26-30)
    NSMPrimitive(26, "do", NSMCategory.ACTIONS_EVENTS, "Action"),
    NSMPrimitive(27, "happen", NSMCategory.ACTIONS_EVENTS, "Event occurrence"),
    NSMPrimitive(28, "move", NSMCategory.ACTIONS_EVENTS, "Motion"),
    NSMPrimitive(29, "touch", NSMCategory.ACTIONS_EVENTS, "Physical contact"),

    # Existence and possession (30-32)
    NSMPrimitive(30, "there is", NSMCategory.EXISTENCE_POSSESSION, "Existence"),
    NSMPrimitive(31, "have", NSMCategory.EXISTENCE_POSSESSION, "Possession"),

    # Life and death (32-33)
    NSMPrimitive(32, "live", NSMCategory.LIFE_DEATH, "Being alive"),
    NSMPrimitive(33, "die", NSMCategory.LIFE_DEATH, "Death"),

    # Time (34-40)
    NSMPrimitive(34, "when/time", NSMCategory.TIME, "Temporal reference"),
    NSMPrimitive(35, "now", NSMCategory.TIME, "Present moment"),
    NSMPrimitive(36, "before", NSMCategory.TIME, "Temporal precedence"),
    NSMPrimitive(37, "after", NSMCategory.TIME, "Temporal succession"),
    NSMPrimitive(38, "a long time", NSMCategory.TIME, "Extended duration"),
    NSMPrimitive(39, "a short time", NSMCategory.TIME, "Brief duration"),
    NSMPrimitive(40, "for some time", NSMCategory.TIME, "Duration"),

    # Space (41-48)
    NSMPrimitive(41, "where/place", NSMCategory.SPACE, "Spatial reference"),
    NSMPrimitive(42, "here", NSMCategory.SPACE, "Proximal location"),
    NSMPrimitive(43, "above", NSMCategory.SPACE, "Higher position"),
    NSMPrimitive(44, "below", NSMCategory.SPACE, "Lower position"),
    NSMPrimitive(45, "far", NSMCategory.SPACE, "Distant"),
    NSMPrimitive(46, "near", NSMCategory.SPACE, "Proximate"),
    NSMPrimitive(47, "side", NSMCategory.SPACE, "Lateral position"),
    NSMPrimitive(48, "inside", NSMCategory.SPACE, "Interior position"),

    # Logical concepts (49-54)
    NSMPrimitive(49, "not", NSMCategory.LOGICAL, "Negation"),
    NSMPrimitive(50, "maybe", NSMCategory.LOGICAL, "Possibility"),
    NSMPrimitive(51, "can", NSMCategory.LOGICAL, "Ability/possibility"),
    NSMPrimitive(52, "because", NSMCategory.LOGICAL, "Causation"),
    NSMPrimitive(53, "if", NSMCategory.LOGICAL, "Condition"),
    NSMPrimitive(54, "like/way", NSMCategory.LOGICAL, "Manner"),

    # Intensifier, augmentor (55)
    NSMPrimitive(55, "very", NSMCategory.INTENSIFIER, "Intensification"),

    # Similarity (56)
    NSMPrimitive(56, "like/as", NSMCategory.SIMILARITY, "Similarity"),

    # Additional universals (57-64)
    NSMPrimitive(57, "part", NSMCategory.SUBSTANTIVES, "Part of whole"),
    NSMPrimitive(58, "kind", NSMCategory.SUBSTANTIVES, "Type, category"),
    NSMPrimitive(59, "true", NSMCategory.EVALUATORS, "Truth"),
    NSMPrimitive(60, "more", NSMCategory.QUANTIFIERS, "Greater degree"),
    NSMPrimitive(61, "be (somewhere)", NSMCategory.SPACE, "Location state"),
    NSMPrimitive(62, "be (someone/something)", NSMCategory.EXISTENCE_POSSESSION, "Identity/classification"),
    NSMPrimitive(63, "mine", NSMCategory.EXISTENCE_POSSESSION, "First person possession"),
    NSMPrimitive(64, "moment", NSMCategory.TIME, "Instant"),
]

# Quick lookup structures
_BY_NAME: dict[str, NSMPrimitive] = {p.name.lower(): p for p in NSM_PRIMITIVES}
_BY_INDEX: dict[int, NSMPrimitive] = {p.index: p for p in NSM_PRIMITIVES}


def get_primitive(name_or_index: str | int) -> NSMPrimitive | None:
    """Get an NSM primitive by name or index."""
    if isinstance(name_or_index, int):
        return _BY_INDEX.get(name_or_index)
    return _BY_NAME.get(name_or_index.lower())


def get_by_category(category: NSMCategory) -> list[NSMPrimitive]:
    """Get all primitives in a category."""
    return [p for p in NSM_PRIMITIVES if p.category == category]


def primitive_token(name: str) -> TokenID | None:
    """Get the mode-00 token for a primitive by name."""
    prim = get_primitive(name)
    return prim.token if prim else None


def is_primitive(word: str) -> bool:
    """Check if a word is an NSM primitive."""
    return word.lower() in _BY_NAME


def all_primitives() -> list[NSMPrimitive]:
    """Get all NSM primitives."""
    return list(NSM_PRIMITIVES)


def primitive_count() -> int:
    """Get the number of NSM primitives."""
    return len(NSM_PRIMITIVES)


# Word to primitive mappings for common words
# This maps English words to their NSM decomposition components
WORD_DECOMPOSITIONS: dict[str, list[str]] = {
    # Simple mappings (word is or directly maps to a primitive)
    "i": ["I"],
    "me": ["I"],
    "you": ["you"],
    "someone": ["someone"],
    "something": ["something"],
    "people": ["people"],
    "person": ["someone"],
    "body": ["body"],
    "this": ["this"],
    "that": ["this", "not", "here"],
    "same": ["the same"],
    "other": ["other"],
    "different": ["other"],
    "one": ["one"],
    "two": ["two"],
    "some": ["some"],
    "all": ["all"],
    "every": ["all"],
    "many": ["much/many"],
    "much": ["much/many"],
    "good": ["good"],
    "bad": ["bad"],
    "big": ["big"],
    "large": ["big"],
    "small": ["small"],
    "little": ["small"],
    "think": ["think"],
    "know": ["know"],
    "want": ["want"],
    "feel": ["feel"],
    "see": ["see"],
    "hear": ["hear"],
    "say": ["say"],
    "tell": ["say"],
    "speak": ["say"],
    "word": ["words"],
    "words": ["words"],
    "do": ["do"],
    "happen": ["happen"],
    "move": ["move"],
    "touch": ["touch"],
    "exist": ["there is"],
    "have": ["have"],
    "live": ["live"],
    "die": ["die"],
    "time": ["when/time"],
    "when": ["when/time"],
    "now": ["now"],
    "before": ["before"],
    "after": ["after"],
    "long": ["a long time"],
    "short": ["a short time"],
    "where": ["where/place"],
    "place": ["where/place"],
    "here": ["here"],
    "there": ["here", "not"],
    "above": ["above"],
    "below": ["below"],
    "under": ["below"],
    "far": ["far"],
    "near": ["near"],
    "close": ["near"],
    "side": ["side"],
    "inside": ["inside"],
    "in": ["inside"],
    "not": ["not"],
    "no": ["not"],
    "maybe": ["maybe"],
    "can": ["can"],
    "because": ["because"],
    "if": ["if"],
    "like": ["like/as"],
    "very": ["very"],
    "really": ["very", "true"],
    "part": ["part"],
    "kind": ["kind"],
    "type": ["kind"],
    "true": ["true"],
    "more": ["more"],

    # Compound decompositions
    "happy": ["feel", "good"],
    "sad": ["feel", "bad"],
    "angry": ["feel", "bad", "want", "do", "something"],
    "afraid": ["feel", "bad", "think", "something", "bad", "happen"],
    "love": ["feel", "good", "want", "good", "someone"],
    "hate": ["feel", "bad", "want", "bad", "someone"],
    "understand": ["know", "think"],
    "believe": ["think", "true"],
    "remember": ["know", "before"],
    "forget": ["not", "know", "now", "know", "before"],
    "learn": ["know", "now", "not", "know", "before"],
    "teach": ["do", "someone", "know"],
    "help": ["do", "good", "someone"],
    "hurt": ["do", "bad", "feel", "bad"],
    "give": ["do", "have", "someone"],
    "take": ["do", "have", "I"],
    "make": ["do", "something", "there is"],
    "create": ["do", "something", "there is", "not", "before"],
    "destroy": ["do", "something", "not", "there is"],
    "begin": ["happen", "before", "not"],
    "start": ["happen", "before", "not"],
    "end": ["happen", "after", "not"],
    "stop": ["not", "do", "more"],
    "continue": ["do", "more", "when/time"],
    "wait": ["not", "do", "when/time"],
    "fast": ["move", "a short time"],
    "slow": ["move", "a long time"],
    "quick": ["do", "a short time"],
    "brown": ["kind", "see"],  # Color as a perceptual kind
    "fox": ["kind", "live", "body", "small"],  # Animal as living body of a kind
    "dog": ["kind", "live", "body"],  # Animal
    "jump": ["move", "above", "body"],
    "jumps": ["move", "above", "body"],
    "over": ["above", "move"],
    "lazy": ["not", "want", "do"],
    "the": ["this"],  # Definite article as demonstrative
}


def decompose_word(word: str) -> list[NSMPrimitive] | None:
    """
    Decompose a word into NSM primitives.

    Returns list of primitives or None if no decomposition exists.
    """
    word_lower = word.lower()
    if word_lower not in WORD_DECOMPOSITIONS:
        return None

    primitive_names = WORD_DECOMPOSITIONS[word_lower]
    result = []
    for name in primitive_names:
        prim = get_primitive(name)
        if prim:
            result.append(prim)
    return result if result else None


def get_abstraction_level(word: str) -> int:
    """
    Get the abstraction level of a word.

    Level 0: NSM primitive
    Level 1: Directly maps to 1 primitive
    Level 2+: Requires multiple primitives
    """
    word_lower = word.lower()

    # Check if it's a primitive itself
    if is_primitive(word_lower):
        return 0

    decomposition = decompose_word(word_lower)
    if decomposition is None:
        return -1  # Unknown

    return len(decomposition)
