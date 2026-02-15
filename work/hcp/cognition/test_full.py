#!/usr/bin/env python3
"""
Test cognition module against full 10-book PBM dataset.
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent.parent.parent))

from work.hcp.cognition.ingest import load_from_sqlite
from work.hcp.cognition.context import ActivationSpreader, ContextRetriever


def main():
    db_path = Path(__file__).parent.parent.parent.parent / "data" / "full_pbm.db"

    print(f"Loading from: {db_path}")
    pbm, tokenizer = load_from_sqlite(db_path)
    print(f"Loaded: {pbm.total_bonds:,} bonds, {tokenizer.vocab_size:,} vocab\n")

    # Test words that appear across multiple books
    test_queries = [
        # Characters/themes that span books
        ("whale", "Moby Dick theme"),
        ("monster", "Frankenstein theme"),
        ("marriage", "Common Victorian theme"),
        ("murder", "Crime theme"),
        ("love", "Universal theme"),
        ("death", "Universal theme"),
        ("sister", "Family theme - Little Women"),
        ("gentleman", "Victorian society"),
    ]

    for word, description in test_queries:
        if word not in tokenizer._word_to_id:
            print(f"'{word}' not in vocabulary\n")
            continue

        token = tokenizer._word_to_id[word]
        spreader = ActivationSpreader(pbm, decay_factor=0.5, max_depth=2)
        activations = spreader.spread([token])

        print(f"=== '{word}' ({description}) ===")
        sorted_acts = sorted(activations.items(), key=lambda x: -x[1])[:8]

        for tok, act in sorted_acts:
            word_lookup = tokenizer.lookup(tok)
            if word_lookup and act > 0.01:
                print(f"  {word_lookup}: {act:.3f}")
        print()

    # Cross-book context test
    print("=== Cross-Book Context Retrieval ===")
    retriever = ContextRetriever(pbm)

    # Multi-word query simulation
    query_words = ["dark", "night"]
    query_tokens = [tokenizer._word_to_id[w] for w in query_words if w in tokenizer._word_to_id]

    if query_tokens:
        result = retriever.get_context(query_tokens)
        print(f"Query: {query_words}")
        print(f"Total activation: {result.total_activation:.2f}")
        print(f"Activated bonds: {len(result.activated_bonds)}")
        print("Top 10 bonds:")
        for ab in result.top_bonds(10):
            left = tokenizer.lookup(ab.bond.left) or "?"
            right = tokenizer.lookup(ab.bond.right) or "?"
            print(f"  {left} -> {right} (act={ab.activation:.3f})")


if __name__ == "__main__":
    main()
