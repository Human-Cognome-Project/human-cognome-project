"""PBM reassembly — pair bond map back to token sequence and text.

Reassembly is the same soft body operation as disassembly, opposite direction.
Bond pairs spawn as paired particles. Each pair looks for matches at both ends.
When matching tokens meet, pairs merge at that point. The separation event IS
the whitespace.

For the MVP glue layer, this is implemented as a weighted graph walk from
stream_start to stream_end. The physics engine will replace this with actual
particle simulation.
"""

from collections import defaultdict
from .vocab import VocabularyCache, STREAM_START, STREAM_END


def reassemble_sequence(pbm_data):
    """Reconstruct token_id sequence from PBM bonds.

    Uses the bond graph to walk from stream_start to stream_end.
    At each node, follows the bond with the highest remaining count
    (greedy walk with count decrement).

    Args:
        pbm_data: dict from load_pbm() — must have bonds, first_fpb

    Returns:
        List of token_id strings (without anchors)
    """
    # Build adjacency: token_a → [(token_b, remaining_count)]
    adj = defaultdict(list)
    for a, b, count in pbm_data["bonds"]:
        adj[a].append([b, count])

    # Sort each adjacency list by count descending (prefer high-count bonds)
    for a in adj:
        adj[a].sort(key=lambda x: -x[1])

    sequence = []
    current = STREAM_START

    # Walk the bond graph
    max_steps = sum(c for _, _, c in pbm_data["bonds"]) + 1
    steps = 0

    while current != STREAM_END and steps < max_steps:
        if current != STREAM_START:
            sequence.append(current)

        # Find next: highest remaining count from current
        neighbors = adj.get(current, [])
        found = False
        for entry in neighbors:
            if entry[1] > 0:
                next_token = entry[0]
                entry[1] -= 1  # Decrement count
                current = next_token
                found = True
                break

        if not found:
            break
        steps += 1

    # Don't include stream_end in output
    return sequence


def reassemble_text(token_sequence, vocab):
    """Convert a token_id sequence back to text.

    Applies spacing rules based on token categories:
    - Space between words (unless preceded by opening punct or followed by closing punct)
    - No space before punctuation (comma, period, etc.)
    - No space after opening brackets/quotes
    - Structural whitespace (newline, tab) rendered directly

    Args:
        token_sequence: List of token_id strings
        vocab: VocabularyCache instance

    Returns:
        Reconstructed text string
    """
    if not token_sequence:
        return ""

    SPACE_TOKEN = "AA.AA.AA.AA.Ah"
    NEWLINE_TOKEN = "AA.AA.AA.AA.AK"
    CR_TOKEN = "AA.AA.AA.AA.AN"
    TAB_TOKEN = "AA.AA.AA.AA.AJ"

    # Punctuation that suppresses preceding space
    NO_SPACE_BEFORE = {
        "AA.AA.AA.AA.Au",  # COMMA
        "AA.AA.AA.AA.Aw",  # FULL STOP
        "AA.AA.AA.AA.BJ",  # SEMICOLON
        "AA.AA.AA.AA.BI",  # COLON
        "AA.AA.AA.AA.Ai",  # EXCLAMATION MARK
        "AA.AA.AA.AA.BN",  # QUESTION MARK
        "AA.AA.AA.AA.Ar",  # RIGHT PARENTHESIS
        "AA.AA.AA.AA.Bt",  # RIGHT SQUARE BRACKET
        "AA.AA.AA.AA.CB",  # RIGHT CURLY BRACKET
    }

    # Punctuation that suppresses following space
    NO_SPACE_AFTER = {
        "AA.AA.AA.AA.Aq",  # LEFT PARENTHESIS
        "AA.AA.AA.AA.Bs",  # LEFT SQUARE BRACKET
        "AA.AA.AA.AA.CA",  # LEFT CURLY BRACKET
    }

    # Structural whitespace — render directly, no extra space
    STRUCTURAL_WS = {NEWLINE_TOKEN, CR_TOKEN, TAB_TOKEN}

    parts = []
    prev_tid = None

    for tid in token_sequence:
        surface = vocab.surface(tid)

        if tid in STRUCTURAL_WS:
            # Structural whitespace renders directly
            parts.append(surface)
            prev_tid = tid
            continue

        if prev_tid is not None:
            # Decide whether to insert a space
            need_space = True

            if tid in NO_SPACE_BEFORE:
                need_space = False
            elif prev_tid in NO_SPACE_AFTER:
                need_space = False
            elif prev_tid in STRUCTURAL_WS:
                need_space = False
            elif prev_tid is None:
                need_space = False

            # Quote handling: check category
            cat = vocab.category(tid)
            prev_cat = vocab.category(prev_tid) if prev_tid else None

            if cat == "pbm_anchor" or prev_cat == "pbm_anchor":
                need_space = False

            if need_space:
                parts.append(" ")

        parts.append(surface)
        prev_tid = tid

    return "".join(parts)
