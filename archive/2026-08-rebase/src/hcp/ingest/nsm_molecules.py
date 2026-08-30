"""Identify NSM semantic molecules by walking the definition graph.

Starting from the 65 NSM primes, walk outward through the gloss/sense data
to find words defined using primes (first-order molecules), then words defined
using primes + first-order molecules (second-order), and so on.

This computes the abstraction depth of every word in the english shard:
how many decomposition layers between it and the NSM primitives.

The definition graph is built from the tokenized glosses in the senses table.
Each word's "definition vocabulary" is the set of distinct tokens appearing
across all its glosses.

"""

import json
from collections import defaultdict

from ..db.postgres import connect as connect_core
from ..db.english import connect as connect_english


def load_prime_exponent_ids(core_conn) -> set[str]:
    """Get all English shard token IDs that are exponents of NSM primes."""
    prime_ids = set()
    with core_conn.cursor() as cur:
        cur.execute("""
            SELECT metadata->'english_token_ids' as refs
            FROM tokens
            WHERE category = 'nsm_prime'
        """)
        for (refs,) in cur.fetchall():
            if refs:
                for word, token_ids in refs.items():
                    for tid in token_ids:
                        prime_ids.add(tid)
    return prime_ids


def build_definition_graph(eng_conn) -> dict[str, set[str]]:
    """Build the definition graph: word_token -> set of tokens used in its definitions.

    For each word that has entries with senses, collect all distinct tokens
    appearing in the gloss_tokens arrays across all its senses.
    """
    print("  Loading definition graph from senses...")
    graph = defaultdict(set)

    with eng_conn.cursor() as cur:
        cur.execute("""
            SELECT e.word_token, s.gloss_tokens
            FROM entries e
            JOIN senses s ON s.entry_id = e.id
            WHERE e.word_token IS NOT NULL
            AND s.gloss_tokens IS NOT NULL
        """)

        row_count = 0
        for word_token, gloss_tokens in cur:
            if gloss_tokens:
                for gt in gloss_tokens:
                    if gt != word_token:  # skip self-references
                        graph[word_token].add(gt)
            row_count += 1
            if row_count % 200000 == 0:
                print(f"    {row_count:,} sense rows processed...")

    print(f"    {row_count:,} total sense rows")
    print(f"    {len(graph):,} words with definitions")
    return dict(graph)


def walk_abstraction_layers(graph: dict[str, set[str]],
                            prime_ids: set[str],
                            max_depth: int = 100) -> tuple[dict[str, int], dict[str, int]]:
    """Walk from primes outward using inverted dependency tracking.

    Instead of scanning all unresolved words each pass, track how many
    unresolved dependencies each word has. When a word resolves, decrement
    the count for everything that depends on it. Words with count=0 are ready.

    Runs both strict (100%) and relaxed (threshold) simultaneously.

    Returns: (strict_depths, relaxed_depths) dicts of token_id -> depth
    """
    # Build reverse index: token -> set of words whose definitions use it
    print("  Building reverse dependency index...")
    used_by = defaultdict(set)
    dep_count = {}  # word -> number of unresolved definition tokens
    dep_total = {}  # word -> total number of definition tokens

    for word_token, def_tokens in graph.items():
        dep_count[word_token] = len(def_tokens)
        dep_total[word_token] = len(def_tokens)
        for dt in def_tokens:
            used_by[dt].add(word_token)

    # Layer 0: prime exponents
    strict_resolved = {}
    relaxed_resolved = {}
    ready_strict = set()

    # Seed with primes
    for pid in prime_ids:
        strict_resolved[pid] = 0
        relaxed_resolved[pid] = 0

    # Decrement counts for everything primes appear in
    for pid in prime_ids:
        for dependent in used_by.get(pid, set()):
            if dependent in dep_count:
                dep_count[dependent] -= 1
                if dep_count[dependent] == 0:
                    ready_strict.add(dependent)

    print(f"\n  Layer 0 (primes): {len(prime_ids)} tokens seeded")
    print(f"  {len(ready_strict)} words ready after prime propagation")

    # Strict walk
    print("\n  --- Strict mode (100% resolution) ---")
    depth = 0
    while ready_strict:
        depth += 1
        next_ready = set()

        for wt in ready_strict:
            strict_resolved[wt] = depth

            # Propagate: decrement dependents
            for dependent in used_by.get(wt, set()):
                if dependent not in strict_resolved and dependent in dep_count:
                    dep_count[dependent] -= 1
                    if dep_count[dependent] == 0:
                        next_ready.add(dependent)

        print(f"  Layer {depth}: {len(ready_strict):,} tokens "
              f"(total: {len(strict_resolved):,})")
        ready_strict = next_ready

        if depth >= max_depth:
            break

    # Relaxed walk (rebuild counts, use threshold)
    THRESHOLD = 0.8
    print(f"\n  --- Relaxed mode (>={THRESHOLD:.0%} threshold) ---")

    # Reset dep counts
    dep_remaining = {}
    for word_token, def_tokens in graph.items():
        unresolved_count = sum(1 for dt in def_tokens if dt not in relaxed_resolved)
        dep_remaining[word_token] = unresolved_count

    # Find initially ready words
    ready_relaxed = set()
    for word_token in graph:
        if word_token in relaxed_resolved:
            continue
        total = dep_total.get(word_token, 0)
        if total == 0:
            continue
        remaining = dep_remaining.get(word_token, total)
        resolved_ratio = (total - remaining) / total
        if resolved_ratio >= THRESHOLD:
            ready_relaxed.add(word_token)

    depth = 0
    while ready_relaxed:
        depth += 1
        next_ready = set()

        for wt in ready_relaxed:
            relaxed_resolved[wt] = depth

        # After resolving this batch, check dependents
        for wt in ready_relaxed:
            for dependent in used_by.get(wt, set()):
                if dependent not in relaxed_resolved and dependent in dep_remaining:
                    dep_remaining[dependent] -= 1
                    total = dep_total.get(dependent, 1)
                    remaining = dep_remaining[dependent]
                    resolved_ratio = (total - remaining) / total
                    if resolved_ratio >= THRESHOLD:
                        next_ready.add(dependent)

        print(f"  Layer {depth}: {len(ready_relaxed):,} tokens "
              f"(total: {len(relaxed_resolved):,})")
        ready_relaxed = next_ready

        if depth >= max_depth:
            break

    return strict_resolved, relaxed_resolved


def run():
    """Run the molecule identification pipeline."""
    from collections import Counter

    print("Connecting to core database...")
    core_conn = connect_core()

    print("Connecting to English shard...")
    eng_conn = connect_english()

    print("\nLoading NSM prime exponent IDs...")
    prime_ids = load_prime_exponent_ids(core_conn)
    print(f"  {len(prime_ids)} English tokens map to primes")

    print("\nBuilding definition graph...")
    graph = build_definition_graph(eng_conn)

    print("\n=== Walking abstraction layers ===")
    strict_depths, relaxed_depths = walk_abstraction_layers(graph, prime_ids)

    print(f"\n=== STRICT RESULTS ===")
    print(f"  Total resolved: {len(strict_depths):,} / {len(graph):,} "
          f"({100 * len(strict_depths) / len(graph):.1f}%)")
    layer_counts = Counter(strict_depths.values())
    for layer in sorted(layer_counts.keys()):
        print(f"    Layer {layer}: {layer_counts[layer]:,}")

    print(f"\n=== RELAXED RESULTS (>=80% threshold) ===")
    print(f"  Total resolved: {len(relaxed_depths):,} / {len(graph):,} "
          f"({100 * len(relaxed_depths) / len(graph):.1f}%)")
    layer_counts = Counter(relaxed_depths.values())
    for layer in sorted(layer_counts.keys()):
        print(f"    Layer {layer}: {layer_counts[layer]:,}")

    # Sample some first-order molecules
    print(f"\n=== Sample first-order molecules (strict) ===")
    layer1_tokens = [t for t, d in strict_depths.items() if d == 1][:20]
    if layer1_tokens:
        placeholders = ','.join(['%s'] * len(layer1_tokens))
        with eng_conn.cursor() as cur:
            cur.execute(f"SELECT token_id, name FROM tokens WHERE token_id IN ({placeholders})",
                        layer1_tokens)
            for tid, name in cur.fetchall():
                print(f"    {tid}  {name}")

    core_conn.close()
    eng_conn.close()


if __name__ == "__main__":
    run()
