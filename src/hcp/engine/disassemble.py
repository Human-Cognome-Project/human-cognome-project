"""PBM disassembly — token sequence to pair bond map.

Disassembly is a physics operation: walk consecutive pairs, count bonds.
The result is the PBM (token_A, token_B, count) — a molecule.
"""

from collections import Counter


def disassemble(token_ids):
    """Disassemble a token_id sequence into PBM bond triples.

    Every token splits into two instances of itself — one as the right end
    of its left pair, one as the left end of its right pair. Keep them
    paired with neighbors. Count occurrences. The result is the PBM.

    Args:
        token_ids: List of token_id strings (with stream anchors)

    Returns:
        dict with:
          - bonds: list of (token_a, token_b, count) tuples
          - first_fpb: (token_a, token_b) — crystallization seed
          - unique_tokens: set of all token_ids
          - total_pairs: total pair count before aggregation
    """
    pair_counts = Counter()
    first_fpb = None

    for i in range(len(token_ids) - 1):
        a = token_ids[i]
        b = token_ids[i + 1]
        if first_fpb is None:
            first_fpb = (a, b)
        pair_counts[(a, b)] += 1

    bonds = [(a, b, count) for (a, b), count in pair_counts.items()]
    unique_tokens = set()
    for tid in token_ids:
        unique_tokens.add(tid)

    return {
        "bonds": bonds,
        "first_fpb": first_fpb,
        "unique_tokens": unique_tokens,
        "total_pairs": len(token_ids) - 1,
    }
