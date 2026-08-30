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
MODE_TEXT = encode_pair(1)         # "AB" — text mode

# Proper noun / entity namespaces (upper range)
# v* = People, w* = Places, x* = Things, y* = Name components, z* = PBMs
MODE_PEOPLE = encode_pair(45 * BASE)      # "vA" — people entities (2250)
MODE_PLACES = encode_pair(46 * BASE)      # "wA" — place entities (2300)
MODE_THINGS = encode_pair(47 * BASE)      # "xA" — thing entities (2350)
MODE_NAMES = encode_pair(48 * BASE)       # "yA" — name components (2400)
MODE_SOURCE_PBM = encode_pair(49 * BASE)  # "zA" — source PBMs (2450)

# Text mode: Language families
LANG_ENGLISH = 1  # AB.AB — English language family

# English word layers (by derivation, bottom to top)
# Layer code is 1st char of 3rd pair, sub-category is 2nd char
LAYER_AFFIX = 0       # A* — affixes (prefix, suffix, infix, interfix, circumfix)
LAYER_FRAGMENT = 1    # B* — fragments (incomplete words) - reserved
LAYER_WORD = 2        # C* — words (noun, verb, adj, adv, etc.)
LAYER_DERIVATIVE = 3  # D* — derivatives (abbreviation, initialism, acronym, etc.)
LAYER_MULTIWORD = 4   # E* — multi-word (phrase, prep_phrase, proverb)

# Sub-categories within each layer (2nd char of 3rd pair)
# Layer A: Affixes
SUB_PREFIX = 0     # AA
SUB_SUFFIX = 1     # AB
SUB_INFIX = 2      # AC
SUB_INTERFIX = 3   # AD
SUB_CIRCUMFIX = 4  # AE
SUB_AFFIX = 5      # AF (generic)

# Layer C: Words (POS)
# Note: Base-50 alphabet has no 'O', so P=14, Q=15, R=16, etc.
SUB_NOUN = 0       # CA
SUB_VERB = 1       # CB
SUB_ADJ = 2        # CC
SUB_ADV = 3        # CD
SUB_PREP = 4       # CE
SUB_CONJ = 5       # CF
SUB_DET = 6        # CG
SUB_PRON = 7       # CH
SUB_INTJ = 8       # CI
SUB_NUM = 9        # CJ
SUB_SYMBOL = 10    # CK
SUB_PARTICLE = 11  # CL
SUB_PUNCT = 12     # CM
SUB_ARTICLE = 13   # CN
SUB_POSTP = 14     # CP
SUB_CHARACTER = 15 # CQ (letters as words)

# Layer D: Derivatives
SUB_ABBREVIATION = 0  # DA
SUB_INITIALISM = 1    # DB
SUB_ACRONYM = 2       # DC
SUB_CONTRACTION = 3   # DD
SUB_CLIPPING = 4      # DE

# Layer E: Multi-word
SUB_PHRASE = 0        # EA
SUB_PREP_PHRASE = 1   # EB
SUB_PROVERB = 2       # EC
SUB_ADV_PHRASE = 3    # ED


def encode_word_token_id(layer: int, sub: int, count_high: int, count_low: int) -> str:
    """Encode a word Token ID in the English language family.

    Format: AB.AB.{layer*50+sub}.{count_high}.{count_low}

    The 3rd pair uses double-duty encoding:
    - 1st character: layer (A=affix, B=fragment, C=word, D=derivative, E=multiword)
    - 2nd character: sub-category within layer
    """
    pair3 = layer * BASE + sub
    return encode_token_id(1, LANG_ENGLISH, pair3, count_high, count_low)


def encode_name_token_id(count: int) -> str:
    """Encode a name component Token ID.

    Format: yA.{count_high}.{count_low}

    Name components are flat-addressed with a simple sequential count.
    6.25M addresses available (2500 * 2500).
    """
    count_high = count // (BASE * BASE)
    count_low = count % (BASE * BASE)
    return encode_token_id(48 * BASE, count_high, count_low)
