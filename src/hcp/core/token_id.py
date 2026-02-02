"""Base-50 Token ID encoding and manipulation.

Alphabet: A-N, P-Z, a-n, p-z (52 Latin letters minus O/o)
Each pair of characters encodes a value 0-2499.
A Token ID is 1-5 dot-separated pairs.
"""

# The 50-symbol alphabet, ASCII sort order (uppercase first)
ALPHABET = "ABCDEFGHIJKLMNPQRSTUVWXYZabcdefghijklmnpqrstuvwxyz"
BASE = len(ALPHABET)  # 50

# Reverse lookup: character -> index
CHAR_TO_INDEX = {c: i for i, c in enumerate(ALPHABET)}


def encode_pair(value: int) -> str:
    """Encode an integer 0-2499 as a two-character base-50 pair."""
    if not 0 <= value < BASE * BASE:
        raise ValueError(f"Pair value must be 0-{BASE * BASE - 1}, got {value}")
    high = value // BASE
    low = value % BASE
    return ALPHABET[high] + ALPHABET[low]


def decode_pair(pair: str) -> int:
    """Decode a two-character base-50 pair to an integer."""
    if len(pair) != 2:
        raise ValueError(f"Pair must be 2 characters, got {len(pair)}")
    high = CHAR_TO_INDEX.get(pair[0])
    low = CHAR_TO_INDEX.get(pair[1])
    if high is None or low is None:
        raise ValueError(f"Invalid characters in pair: {pair!r}")
    return high * BASE + low


def encode_token_id(*values: int) -> str:
    """Encode 1-5 integer values as a dotted Token ID string."""
    if not 1 <= len(values) <= 5:
        raise ValueError(f"Token ID requires 1-5 pairs, got {len(values)}")
    return ".".join(encode_pair(v) for v in values)


def decode_token_id(token_id: str) -> tuple[int, ...]:
    """Decode a dotted Token ID string to a tuple of integer values."""
    pairs = token_id.split(".")
    if not 1 <= len(pairs) <= 5:
        raise ValueError(f"Token ID requires 1-5 pairs, got {len(pairs)}")
    return tuple(decode_pair(p) for p in pairs)


def token_depth(token_id: str) -> int:
    """Return the number of pairs (LoD depth) in a Token ID."""
    return len(token_id.split("."))


# Common base addresses
MODE_UNIVERSAL = encode_pair(0)   # "AA" — universal/computational
MODE_SOURCE_UNIVERSAL = encode_pair(BASE * BASE - BASE)  # "zA" — source PBMs for universal
