#!/usr/bin/env python3
"""
Test cognition module against ingested Alice in Wonderland PBM.
"""
import sys
from pathlib import Path

# Add repo to path
sys.path.insert(0, str(Path(__file__).parent.parent.parent.parent))

from work.hcp.cognition.ingest import load_from_sqlite
from work.hcp.cognition.context import ActivationSpreader, ContextRetriever


def main():
    db_path = Path(__file__).parent.parent.parent.parent / "data" / "test_pbm.db"

    if not db_path.exists():
        print(f"Database not found: {db_path}")
        print("Run ingest first: python -m work.hcp.cognition.ingest ...")
        return

    print(f"Loading from: {db_path}")
    pbm, tokenizer = load_from_sqlite(db_path)

    print(f"Loaded: {pbm.total_bonds:,} bonds, {tokenizer.vocab_size:,} vocab")
    print()

    # Test activation spreading on some words
    test_words = ["alice", "rabbit", "queen", "tea", "mad"]

    for word in test_words:
        if word not in tokenizer._word_to_id:
            print(f"'{word}' not in vocabulary")
            continue

        token = tokenizer._word_to_id[word]
        print(f"=== Activation spreading from '{word}' ===")

        spreader = ActivationSpreader(pbm, decay_factor=0.6, max_depth=3)
        activations = spreader.spread([token])

        # Sort by activation, show top 10
        sorted_acts = sorted(activations.items(), key=lambda x: -x[1])[:10]

        for tok, act in sorted_acts:
            word_lookup = tokenizer.lookup(tok)
            if word_lookup:
                print(f"  {word_lookup}: {act:.3f}")
        print()

    # Test context retrieval
    print("=== Context Retrieval Test ===")
    retriever = ContextRetriever(pbm)

    # Query with "rabbit" token
    if "rabbit" in tokenizer._word_to_id:
        rabbit_token = tokenizer._word_to_id["rabbit"]
        result = retriever.get_context([rabbit_token])

        print(f"Query: 'rabbit'")
        print(f"Total activation: {result.total_activation:.2f}")
        print(f"Activated bonds: {len(result.activated_bonds)}")
        print("Top 5 bonds:")
        for ab in result.top_bonds(5):
            left_word = tokenizer.lookup(ab.bond.left) or str(ab.bond.left)
            right_word = tokenizer.lookup(ab.bond.right) or str(ab.bond.right)
            print(f"  {left_word} -> {right_word} (act={ab.activation:.3f})")


if __name__ == "__main__":
    main()
