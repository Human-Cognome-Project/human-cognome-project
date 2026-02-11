"""Test apostrophe handling in tokenizer."""

import sys
sys.path.insert(0, '/home/patrick/gits/human-cognome-project/src')

from hcp.ingest.gutenberg_ingest_pbm import Tokenizer

def test_apostrophes():
    tokenizer = Tokenizer()

    test_cases = [
        ("don't worry", ["don't", "worry"]),
        ("cat's tail", ["cat's", "tail"]),
        ("cats' tails", ["cats'", "tails"]),
        ("'hello world'", ["'", "hello", "world", "'"]),
        ("it's fine", ["it's", "fine"]),
        ("Alice's adventures", ["Alice's", "adventures"]),
        ("I'm here", ["I'm", "here"]),
        ("_emphasis_ text", ["_", "emphasis", "_", "text"]),
        ('"quoted text"', ['"', "quoted", "text", '"']),
    ]

    for text, expected in test_cases:
        tokens = tokenizer.tokenize(text)
        result = [t.string for t in tokens]
        status = "✓" if result == expected else "✗"
        print(f"{status} '{text}'")
        print(f"  Expected: {expected}")
        print(f"  Got:      {result}")
        if result != expected:
            print(f"  MISMATCH!")
        print()

if __name__ == "__main__":
    test_apostrophes()
