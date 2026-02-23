"""Validation utilities for PBM pipeline.

Verifies:
1. Disassembly accuracy (bond counts match original text)
2. DB roundtrip integrity (same bonds in = same bonds out)
3. Basic reconstruction sanity checks
"""

import sys
import time
from pathlib import Path
from collections import Counter

from .vocab import VocabularyCache, STREAM_START, STREAM_END
from .tokenizer import tokenize
from .disassemble import disassemble
from .storage import connect, store_pbm, load_pbm
from .pipeline import strip_gutenberg_boilerplate, guess_century


def validate_disassembly(text, vocab):
    """Verify disassembly produces correct bond counts.

    Re-walks the token sequence independently and checks every pair
    matches the disassembly output.
    """
    token_ids = tokenize(text, vocab)

    # Independent pair counting
    expected = Counter()
    for i in range(len(token_ids) - 1):
        expected[(token_ids[i], token_ids[i + 1])] += 1

    # Disassembly
    pbm = disassemble(token_ids)
    actual = {(a, b): c for a, b, c in pbm["bonds"]}

    # Compare
    errors = []
    for pair, count in expected.items():
        if pair not in actual:
            errors.append(f"  MISSING: {pair} (expected count={count})")
        elif actual[pair] != count:
            errors.append(f"  COUNT MISMATCH: {pair} expected={count} got={actual[pair]}")

    for pair in actual:
        if pair not in expected:
            errors.append(f"  EXTRA: {pair} (count={actual[pair]})")

    return {
        "passed": len(errors) == 0,
        "expected_pairs": len(expected),
        "actual_pairs": len(actual),
        "errors": errors,
    }


def validate_db_roundtrip(pbm_data, loaded_data):
    """Verify DB write → read produces identical bonds.

    Compares bond triples from disassembly against bonds loaded from DB.
    """
    original = set((a, b, c) for a, b, c in pbm_data["bonds"])
    loaded = set((a, b, c) for a, b, c in loaded_data["bonds"])

    missing = original - loaded
    extra = loaded - original

    errors = []
    if missing:
        errors.append(f"  {len(missing)} bonds in original but not in DB:")
        for a, b, c in list(missing)[:5]:
            errors.append(f"    {a} → {b} (count={c})")
    if extra:
        errors.append(f"  {len(extra)} bonds in DB but not in original:")
        for a, b, c in list(extra)[:5]:
            errors.append(f"    {a} → {b} (count={c})")

    return {
        "passed": len(errors) == 0,
        "original_bonds": len(original),
        "loaded_bonds": len(loaded),
        "errors": errors,
    }


def validate_token_coverage(token_ids, vocab):
    """Check how many tokens were successfully resolved."""
    total = len(token_ids) - 2  # Exclude anchors
    resolved = sum(1 for tid in token_ids[1:-1]
                   if not tid.startswith("AA.AA.AA.AA.")
                   or vocab.category(tid) in ("whitespace", "punctuation",
                                               "digit", "letter_lower",
                                               "letter_upper"))

    # Count actual words (AB.AB.*)
    words = sum(1 for tid in token_ids[1:-1] if tid.startswith("AB.AB."))
    # Count punctuation (AA.AA.AA.AA.*)
    punct = sum(1 for tid in token_ids[1:-1]
                if tid.startswith("AA.AA.AA.AA.")
                and vocab.category(tid) in ("punctuation",))
    # Count whitespace (newlines, etc.)
    ws = sum(1 for tid in token_ids[1:-1]
             if tid.startswith("AA.AA.AA.AA.")
             and vocab.category(tid) in ("whitespace",))
    # Count single chars used as fallback
    chars = sum(1 for tid in token_ids[1:-1]
                if tid.startswith("AA.AA.AA.AA.")
                and vocab.category(tid) in ("letter_lower", "letter_upper",
                                             "digit"))

    return {
        "total_tokens": total,
        "words": words,
        "punctuation": punct,
        "structural_ws": ws,
        "single_chars": chars,
        "char_pct": (chars / total * 100) if total > 0 else 0,
    }


def run_validation(text_file):
    """Run full validation suite on a text file."""
    text_path = Path(text_file)
    print(f"=== Validating PBM Pipeline: {text_path.name} ===\n")

    # Load vocab
    t0 = time.time()
    vocab = VocabularyCache()
    vocab.load()
    print(f"Vocabulary loaded ({time.time() - t0:.2f}s)")

    # Read text
    text = text_path.read_text(encoding="utf-8", errors="replace")
    text = strip_gutenberg_boilerplate(text)
    print(f"Text: {len(text):,} characters\n")

    # Tokenize
    token_ids = tokenize(text, vocab)

    # 1. Token coverage
    print("--- Token Coverage ---")
    coverage = validate_token_coverage(token_ids, vocab)
    print(f"  Total tokens: {coverage['total_tokens']:,}")
    print(f"  Words: {coverage['words']:,}")
    print(f"  Punctuation: {coverage['punctuation']:,}")
    print(f"  Structural whitespace: {coverage['structural_ws']:,}")
    print(f"  Single chars (fallback): {coverage['single_chars']:,} "
          f"({coverage['char_pct']:.1f}%)")
    if coverage["char_pct"] > 5:
        print("  WARNING: High single-char rate suggests vocabulary gaps")
    print()

    # 2. Disassembly accuracy
    print("--- Disassembly Accuracy ---")
    dis_result = validate_disassembly(text, vocab)
    if dis_result["passed"]:
        print(f"  PASSED: {dis_result['expected_pairs']:,} pairs verified")
    else:
        print(f"  FAILED: {len(dis_result['errors'])} errors")
        for e in dis_result["errors"][:10]:
            print(e)
    print()

    # 3. DB roundtrip
    print("--- DB Roundtrip ---")
    pbm_data = disassemble(token_ids)
    century = guess_century(text_path.name)
    conn = connect()
    try:
        doc_id = store_pbm(conn, text_path.stem, century, pbm_data,
                           metadata={"validation_run": True})
        loaded = load_pbm(conn, doc_id)
    finally:
        conn.close()

    rt_result = validate_db_roundtrip(pbm_data, loaded)
    if rt_result["passed"]:
        print(f"  PASSED: {rt_result['original_bonds']:,} bonds survived roundtrip")
    else:
        print(f"  FAILED: {len(rt_result['errors'])} errors")
        for e in rt_result["errors"][:10]:
            print(e)
    print()

    # Summary
    all_passed = dis_result["passed"] and rt_result["passed"]
    print(f"=== {'ALL VALIDATIONS PASSED' if all_passed else 'SOME VALIDATIONS FAILED'} ===")
    print(f"  Document stored as: {doc_id}")

    return all_passed


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python -m hcp.engine.validate <gutenberg_text_file>")
        sys.exit(1)
    success = run_validation(sys.argv[1])
    sys.exit(0 if success else 1)
