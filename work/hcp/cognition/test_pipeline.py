#!/usr/bin/env python3
"""
Test full cognition pipeline: context → Ollama → decision recording.
"""
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent.parent.parent))

from work.hcp.cognition.ingest import load_from_sqlite
from work.hcp.cognition.context import ContextRetriever
from work.hcp.cognition.bridge import OllamaBridge, Bond, GenerationConfig
from work.hcp.cognition.decisions import DecisionRecorder
from work.hcp.cognition.identity import create_agent_seed


def main():
    db_path = Path(__file__).parent.parent.parent.parent / "data" / "full_pbm.db"

    print("=== Full Cognition Pipeline Test ===\n")

    # 1. Load PBM data
    print("1. Loading PBM data...")
    start = time.time()
    pbm, tokenizer = load_from_sqlite(db_path)
    print(f"   Loaded {pbm.total_bonds:,} bonds, {tokenizer.vocab_size:,} vocab")
    print(f"   Time: {time.time() - start:.1f}s\n")

    # 2. Create identity seed
    print("2. Creating agent identity...")
    identity = create_agent_seed(
        name="planner",
        description="Architecture and analysis agent focused on patterns",
        core_concepts=["architecture", "analysis", "literature", "patterns"],
    )
    print(f"   Agent: {identity.name}")
    print(f"   Core concepts: {identity.core_concepts}\n")

    # 3. Context retrieval
    print("3. Retrieving context for query...")
    query_word = "monster"
    if query_word not in tokenizer._word_to_id:
        print(f"   '{query_word}' not in vocabulary, using 'love'")
        query_word = "love"

    query_token = tokenizer._word_to_id[query_word]
    retriever = ContextRetriever(pbm)

    start = time.time()
    context_result = retriever.get_context([query_token])
    print(f"   Query: '{query_word}'")
    print(f"   Activated bonds: {len(context_result.activated_bonds)}")
    print(f"   Time: {time.time() - start:.3f}s")

    # Convert to bridge Bond format
    context_bonds = []
    for ab in context_result.top_bonds(5):
        left_word = tokenizer.lookup(ab.bond.left) or "?"
        right_word = tokenizer.lookup(ab.bond.right) or "?"
        context_bonds.append(Bond(
            name=f"{left_word}->{right_word}",
            value=f"activation={ab.activation:.2f}"
        ))
        print(f"   - {left_word} -> {right_word} (act={ab.activation:.3f})")
    print()

    # 4. Generate with Ollama
    print("4. Generating response via Ollama...")
    config = GenerationConfig(
        model="tinyllama:latest",  # Fast for testing
        temperature=0.7,
        max_tokens=150
    )
    bridge = OllamaBridge(config)

    # Check health
    if not bridge.health_check():
        print("   ERROR: Ollama not available")
        return

    prompt = f"Based on the literary context around '{query_word}', what themes does this word evoke? Be concise."

    start = time.time()
    result = bridge.generate(
        prompt=prompt,
        context_bonds=context_bonds,
        identity_token="dA.AA.AA.AB"
    )
    duration = time.time() - start

    print(f"   Prompt: {prompt[:60]}...")
    print(f"   Response: {result.response[:200]}...")
    print(f"   Tokens: {result.tokens_generated}")
    print(f"   Time: {duration:.1f}s\n")

    # 5. Record decision
    print("5. Recording decision...")
    recorder = DecisionRecorder(agent_name="planner")
    decision = recorder.record(
        action=f"analyzed_theme:{query_word}",
        rationale="Testing cognition pipeline with literary corpus",
        context=context_result,
        output=result.response[:100],
        confidence=0.85
    )
    print(f"   Decision ID: {decision.decision_id[:12]}...")
    print(f"   Confidence: {decision.confidence}")
    print()

    print("=== Pipeline Complete ===")
    print(f"Flow: PBM({pbm.total_bonds:,}) → Context({len(context_result.activated_bonds)}) → LLM({result.tokens_generated}tok) → Decision")


if __name__ == "__main__":
    main()
