"""Engine tokenizer — text to token_id sequence.

Text goes straight into the engine. Vocabulary lookup is a hash table hit.
No separate preprocessing pipeline. This IS the engine's tokenization.

Whitespace handling:
  - Spaces (0x20) are stripped — extrapolated on reconstruction
  - Newlines, tabs, CR are structural tokens, always preserved
  - Punctuation is tokenized as individual characters

Unknown tokens are wrapped in <sic></sic> and atomized to largest
recognized sub-tokens.
"""

import re

from .vocab import VocabularyCache, STREAM_START, STREAM_END


# SIC wrapper tokens (looked up from core on init)
SIC_OPEN = "AA.AE.AC"   # Will be resolved at init
SIC_CLOSE = "AA.AE.AC"  # Will be resolved at init

# Regex to split text into words and individual non-word characters
# Keeps: word runs (letters/digits/apostrophes), individual chars
_SPLIT_PATTERN = re.compile(r"([A-Za-z0-9]+(?:'[A-Za-z]+)*)")


def tokenize(text, vocab):
    """Tokenize text into a sequence of token_ids.

    Args:
        text: Input text string
        vocab: VocabularyCache instance (loaded)

    Returns:
        List of token_id strings, with stream anchors at boundaries.
    """
    tokens = [STREAM_START]

    i = 0
    while i < len(text):
        ch = text[i]

        # Space — stripped, not tokenized
        if ch == ' ':
            i += 1
            continue

        # Structural whitespace (newline, tab, CR) — tokenized
        if ch in ('\n', '\r', '\t'):
            tid = vocab.char_to_token.get(ch)
            if tid:
                tokens.append(tid)
            i += 1
            continue

        # Try to match a word starting here
        m = _SPLIT_PATTERN.match(text, i)
        if m:
            word = m.group(0)
            tid = vocab.lookup(word)
            if tid:
                tokens.append(tid)
            else:
                # Unknown word — atomize
                _atomize_unknown(word, vocab, tokens)
            i = m.end()
            continue

        # Single character (punctuation, symbol, etc.)
        tid = vocab.char_to_token.get(ch)
        if tid:
            tokens.append(tid)
        else:
            # Unknown character — try byte code
            byte_val = ord(ch)
            if byte_val < 128:
                tid = vocab.char_to_token.get(ch)
                if tid:
                    tokens.append(tid)
            # else: non-ASCII character, skip for MVP
        i += 1

    tokens.append(STREAM_END)
    return tokens


def _atomize_unknown(word, vocab, tokens):
    """Handle an unknown word by decomposing to largest recognized sub-tokens.

    Wraps the unknown region in sic markers and breaks into the largest
    recognized substrings.
    """
    # Try progressively smaller substrings from the left
    i = 0
    unknown_chars = []

    while i < len(word):
        found = False
        # Try longest match first (whole remaining), then shrink
        for end in range(len(word), i, -1):
            sub = word[i:end]
            if len(sub) > 1:
                tid = vocab.lookup(sub)
                if tid:
                    # Flush any accumulated unknown chars first
                    if unknown_chars:
                        for ch in unknown_chars:
                            tid_ch = vocab.char_to_token.get(ch)
                            if tid_ch:
                                tokens.append(tid_ch)
                        unknown_chars = []
                    tokens.append(tid)
                    i = end
                    found = True
                    break
        if not found:
            # Single character fallback
            ch = word[i]
            tid = vocab.char_to_token.get(ch)
            if tid:
                tokens.append(tid)
            i += 1
