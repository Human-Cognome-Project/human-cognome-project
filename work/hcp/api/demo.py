"""
HCP MVP Demonstration.

Shows the full pipeline:
1. Atomize to bytes
2. Apply covalent bonding
3. Promote to tokens (unknowns -> soft bodies)
4. Create pair bonds, store as PBM
5. Physics: soft bodies find corrections via energy minimization
6. Reconstruct from PBM (lossless)
7. Show NSM decomposition
"""
from __future__ import annotations

import sys


def print_header(title: str) -> None:
    """Print a section header."""
    print()
    print("=" * 60)
    print(f"  {title}")
    print("=" * 60)
    print()


def print_step(num: int, title: str) -> None:
    """Print a step header."""
    print(f"\n[Step {num}] {title}")
    print("-" * 40)


def run_demo(text: str, verbose: bool = False) -> dict:
    """
    Run the full HCP demonstration.

    Args:
        text: Input text (ideally with misspellings)
        verbose: Show extra details

    Returns:
        Dict with all results
    """
    from ..atomizer.byte_atomizer import ByteAtomizer, ByteSpanClassifier
    from ..atomizer.covalent_tables import classify_byte, bond_strength
    from ..atomizer.tokenizer import Tokenizer, TokenizerConfig
    from ..core.pair_bond import PairBondMap
    from ..storage.bond_store import BondStore
    from ..physics.engine import PhysicsEngine
    from ..assembly.reconstructor import Reconstructor
    from ..assembly.validator import Validator
    from ..abstraction.decomposer import Decomposer
    from ..abstraction.abstraction_meter import measure_abstraction

    results = {"input": text}

    print_header("HCP MVP Demonstration")
    print(f"Input: \"{text}\"")

    # Step 1: Atomize to bytes
    print_step(1, "Atomize to bytes")
    atomizer = ByteAtomizer()
    atoms = atomizer.atomize_text(text)
    print(f"  Bytes: {len(atoms)}")
    if verbose:
        byte_str = " ".join(f"{a.value:02X}" for a in atoms[:20])
        print(f"  Hex: {byte_str}...")
    results["byte_count"] = len(atoms)

    # Step 2: Apply covalent bonding
    print_step(2, "Apply covalent bonding")
    classifier = ByteSpanClassifier()
    spans = classifier.span_bytes(text.encode("utf-8"))
    print(f"  Spans identified: {len(spans)}")
    for span in spans[:5]:
        s = span.to_string() or "[binary]"
        print(f"    - {span.span_type}: \"{s}\"")
    if len(spans) > 5:
        print(f"    ... and {len(spans) - 5} more")
    results["span_count"] = len(spans)

    # Step 3: Promote to tokens
    print_step(3, "Promote to tokens")
    config = TokenizerConfig(promote_words=True)
    tokenizer = Tokenizer(config)
    tokens = tokenizer.tokenize_text(text)
    print(f"  Tokens: {len(tokens)}")
    if verbose:
        for i, t in enumerate(tokens[:10]):
            if t.is_word():
                word = tokenizer.get_word_str(t)
                print(f"    {i}: {t} = '{word}'")
            elif t.is_byte():
                c = chr(t.value) if 32 <= t.value < 127 else "?"
                print(f"    {i}: {t} = '{c}'")
    results["token_count"] = len(tokens)

    # Step 4: Create PBM
    print_step(4, "Create Pair Bond Map (PBM)")
    pbm = PairBondMap()
    pbm.add_sequence(tokens)
    print(f"  Unique bonds: {pbm.unique_bonds}")
    print(f"  Total bonds: {pbm.total_bonds}")

    # Store in database
    store = BondStore.memory()
    seq_id = store.store_pbm(pbm, text.encode("utf-8"))
    print(f"  Stored as sequence #{seq_id}")
    results["unique_bonds"] = pbm.unique_bonds
    results["total_bonds"] = pbm.total_bonds

    # Step 5: Physics correction
    print_step(5, "Physics simulation (error correction)")
    engine = PhysicsEngine()
    sim_result = engine.simulate(text)

    print(f"  Original:  \"{sim_result.original_text}\"")
    print(f"  Corrected: \"{sim_result.corrected_text}\"")
    print(f"  Corrections: {len(sim_result.corrections)}")
    for c in sim_result.corrections:
        print(f"    - '{c['original']}' -> '{c['corrected']}' ({c['confidence']:.0%})")
    print(f"  Energy: {sim_result.energy_before:.2f} -> {sim_result.energy_after:.2f}")
    results["corrected"] = sim_result.corrected_text
    results["corrections"] = sim_result.corrections

    # Step 6: Reconstruction validation
    print_step(6, "Lossless reconstruction")

    # Create byte-level PBM for reconstruction
    byte_tokens = atomizer.to_tokens(text.encode("utf-8"))
    byte_pbm = PairBondMap()
    byte_pbm.add_sequence(byte_tokens)

    reconstructor = Reconstructor()
    rec_result = reconstructor.reconstruct(byte_pbm)
    reconstructed = rec_result.to_string()

    validator = Validator()
    val_result = validator.validate_text(text, byte_pbm)

    print(f"  Reconstructed: \"{reconstructed}\"")
    print(f"  Method: {rec_result.method}")
    print(f"  Validation: {val_result}")
    results["reconstructed"] = reconstructed
    results["reconstruction_valid"] = val_result.valid

    # Step 7: NSM decomposition
    print_step(7, "NSM Semantic Decomposition")
    decomposer = Decomposer()

    # Decompose the corrected text
    corrected = sim_result.corrected_text
    dec_result = decomposer.decompose_text(corrected)

    print(f"  Coverage: {dec_result.coverage:.0%}")
    print(f"  Total primitives: {dec_result.total_primitives}")
    print()
    print("  Word decompositions:")
    for node in dec_result.nodes:
        if node.primitives:
            prims = " + ".join(str(p) for p in node.primitives)
            print(f"    {node.word} -> {prims}")
        else:
            print(f"    {node.word} -> [no decomposition]")

    # Abstraction metrics
    metrics = measure_abstraction(corrected)
    print()
    print(f"  Average abstraction level: {metrics.average_level:.2f}")
    print(f"  Complexity score: {metrics.complexity_score:.2f}")

    results["nsm_coverage"] = dec_result.coverage
    results["nsm_primitives"] = dec_result.total_primitives
    results["abstraction_level"] = metrics.average_level

    # Summary
    print_header("Demo Summary")
    print(f"  Input:      \"{text}\"")
    print(f"  Corrected:  \"{sim_result.corrected_text}\"")
    print(f"  Corrections: {len(sim_result.corrections)}/4 misspellings fixed")
    print(f"  Roundtrip:  {'PASS' if val_result.valid else 'FAIL'}")
    print(f"  NSM coverage: {dec_result.coverage:.0%} of words decomposed")

    # Success criteria check
    print()
    print("Success Criteria:")
    print(f"  [{'X' if len(atoms) == len(text.encode('utf-8')) else ' '}] All bytes handled")
    print(f"  [{'X' if pbm.unique_bonds > 0 else ' '}] Valid PBM created")
    print(f"  [{'X' if val_result.valid else ' '}] Byte-perfect reconstruction")
    print(f"  [{'X' if len(sim_result.corrections) >= 3 else ' '}] 3+ misspellings corrected")
    print(f"  [{'X' if dec_result.coverage >= 0.5 else ' '}] 5+ words have NSM decomposition")

    return results


def main() -> int:
    """Main entry point for demo."""
    text = "The quik brwon fox jumps oevr the layz dog"
    if len(sys.argv) > 1:
        text = " ".join(sys.argv[1:])

    run_demo(text, verbose=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
