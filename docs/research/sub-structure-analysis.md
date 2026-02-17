# Sub-Structure Analysis for PBM Boilerplate Detection

**From:** PBM Specialist
**Date:** 2026-02-17
**Status:** Complete — research only, no code written
**Primary use case:** Compression — detecting common sub-structures (boilerplate, repeated phrasing) across document PBMs to store once as sub-PBMs
**Secondary use cases:** Cross-domain structural comparison, plagiarism detection

---

## Executive Summary

OpenMM has **zero native sub-structure analysis capabilities**. It is a dynamics engine — it builds topologies and computes forces, but cannot search, compare, or fingerprint molecular structures. Sub-structure analysis is the domain of cheminformatics (RDKit) and graph theory (NetworkX) libraries.

**RDKit** is the strongest candidate: native MCS detection (N-way, with custom atom/bond comparators), VF2 subgraph isomorphism, Morgan/ECFP fingerprinting — all with hooks to replace chemical logic with arbitrary token-type matching. The constraint is scale: MCS is NP-complete and will timeout on molecules with 1000+ atoms.

**NetworkX** is the practical fallback for large-scale work: VF2++ subgraph isomorphism (14x faster than VF2, handles large graphs), Weisfeiler-Lehman graph hashing (structurally equivalent to Morgan fingerprints), and ISMAGS for MCS. Pure Python, so slower per-operation, but no chemistry assumptions.

**Critical insight for PBM:** Document PBMs are primarily **linear chain graphs** (token→next_token). Common sub-structure detection on linear chains reduces to **common substring detection** — a well-solved O(n) problem via suffix trees/arrays, not NP-complete graph isomorphism. The graph tools become necessary only when PBM structure goes beyond linear adjacency (cross-references, hierarchical bonds, etc.).

---

## What OpenMM Offers: Nothing

OpenMM's Topology class provides:
- `atoms()`, `bonds()`, `residues()`, `chains()` — iteration
- `getNumBonds()`, `getNumAtoms()` — counting
- `addAtom()`, `addBond()` — construction
- `createStandardBonds()`, `createDisulfideBonds()` — chemistry-specific bond generation

There is **no** substructure search, pattern matching, fingerprinting, comparison, or graph analysis of any kind. The ForceField's compiled Cython template matcher does bond-topology subgraph isomorphism, but only within the residue-template-matching context — it cannot be repurposed for general sub-structure detection.

OpenMM-adjacent tools (ParmEd, OpenMMTools) add structure manipulation and file format conversion, not analysis.

**OpenMM's role boundary:** Build topology, compute forces, run dynamics. Analysis of the topology graph is outside its scope. This is consistent with how the molecular dynamics ecosystem works — OpenMM simulates, other tools (RDKit, MDAnalysis, MDTraj) analyze.

---

## RDKit: The Cheminformatics Powerhouse

RDKit (rdkit.org) is the leading open-source cheminformatics toolkit. All three capabilities we need exist natively, and all support custom atom/bond comparison to replace chemical logic with token-type matching.

### 1. Maximum Common Substructure (MCS)

**Module:** `rdkit.Chem.rdFMCS.FindMCS()`

| Feature | Details |
|---------|---------|
| Algorithm | Seed-growth with backtracking (breadth-first, largest-seed-first) |
| N-way MCS | Yes — accepts a list of 2+ molecules |
| Threshold | `threshold=0.8` finds MCS present in 80% of inputs (tolerates outliers) |
| Timeout | Configurable (default 3600s), returns best result so far |
| Return value | SMARTS pattern + atom/bond count + query molecule |
| Custom atom matching | Subclass `MCSAtomCompare` — full control over what "same atom" means |
| Custom bond matching | Subclass `MCSBondCompare` — full control over bond comparison |

**For HCP:** Custom `MCSAtomCompare` would compare by token_id (stored as atom isotope label or IntProp), not by chemical element. The MCS machinery handles the graph search; our comparator defines what "same" means.

**Scale limitation:** MCS is NP-complete. Works well for drug-like molecules (20-50 atoms). For PBM molecules (1000-100,000+ atoms), it will hit the timeout before finding the true MCS. Viable only on small subsets or pre-filtered candidates.

**Alternative:** `rdRascalMCES.FindMCES()` — pairwise only, bond-based (Maximum Common Edge Substructure), faster, returns multi-fragment results. Good for pairwise document comparison.

### 2. Subgraph Matching / Substructure Search

**Module:** `mol.GetSubstructMatches(pattern, params)`

| Feature | Details |
|---------|---------|
| Algorithm | VF2 subgraph isomorphism (C++ implementation) |
| Custom matching | `SubstructMatchParameters.extraAtomCheckOverridesDefaultCheck = True` replaces ALL chemical logic |
| Threading | `params.numThreads = 0` for auto-detect |
| Bulk screening | `SubstructLibrary` — fingerprint pre-filtering + multithreaded VF2 |
| Safety | `maxMatches` parameter prevents combinatorial explosion |

**For HCP:** Given a known sub-PBM (e.g., Gutenberg header pattern), search for it across all document PBMs. The `extraAtomCheckOverridesDefaultCheck` flag is critical — it lets VF2 handle the graph traversal while our comparator handles token-type matching.

**Performance:** Microseconds per search for typical molecules. `SubstructLibrary` benchmarked at ~990s for 2.4M molecules. For large PBM molecules, pathological cases (highly repetitive token sequences) could cause combinatorial issues.

### 3. Molecular Fingerprinting

**Module:** `rdkit.Chem.rdFingerprintGenerator`

| Type | Basis | HCP Relevance |
|------|-------|---------------|
| **Morgan (ECFP)** | Circular: hash neighborhood at each atom up to radius R | Local n-gram-like patterns around each token |
| **RDKit FP** | Path-based: enumerate all paths up to length L | Sequential token patterns (like n-grams) |
| **Atom Pairs** | Pairwise: (type_A, distance, type_B) for all pairs | Token co-occurrence at various distances |
| **Topological Torsion** | 4-atom paths | 4-token sequences |

**All fingerprint types accept custom atom/bond invariants:**
```python
fp = generator.GetFingerprint(mol,
    customAtomInvariants=[token_id for each atom])
```

This replaces chemical element hashing with token_id hashing. The fingerprint machinery (neighborhood expansion, path enumeration, hashing) operates identically on our token graph.

**Bit explanation:** `AdditionalOutput.GetBitInfoMap()` traces fingerprint bits back to specific atoms/substructures. For Morgan FPs: `{bit: [(center_atom, radius), ...]}`. This means we can identify WHICH token patterns triggered each fingerprint bit.

**Tanimoto similarity:** `BulkTanimotoSimilarity(fp1, fp_list)` for fast 1-vs-many screening. Linear in the number of comparisons, sub-second for large datasets.

### 4. Building Non-Chemical Molecules in RDKit

RDKit CAN build arbitrary graph structures:

```python
mol = Chem.RWMol()
for token_type in tokens:
    a = Chem.Atom(0)            # Dummy atom (atomic num 0) — bypasses valence checks
    a.SetIsotope(token_type)    # Integer label (0-65535) — or use SetIntProp for larger range
    mol.AddAtom(a)
for src, dst in bonds:
    mol.AddBond(src, dst, Chem.BondType.SINGLE)
mol.UpdatePropertyCache(strict=False)  # Skip chemical validation
```

- `Atom(0)` = dummy atom, valence checking disabled
- `SetIsotope()` = integer label (uint16, 0-65535) — alternative: `SetIntProp("token_id", value)` for full int range
- Sanitization can be fully skipped or selectively applied
- Bond types limited to RDKit's enum (SINGLE, DOUBLE, etc.) but custom bond invariants compensate

---

## NetworkX: Pure Graph Theory

NetworkX (networkx.org) operates on arbitrary graphs with no chemical assumptions. Pure Python, so slower than C++ tools, but no domain friction.

### 1. Subgraph Isomorphism

| Algorithm | Performance | Notes |
|-----------|-------------|-------|
| **VF2++** | Up to 14x faster than VF2, scales well to 2000+ nodes | Recommended. Non-recursive. |
| **VF2** | Classic, supports directed/undirected/multigraphs | `DiGraphMatcher` for directed graphs |
| **ISMAGS** | Handles symmetry reduction, also does MCS | Slower than VF2 on small graphs |

Custom node/edge matching via `node_match` and `edge_match` callables on `GraphMatcher`.

### 2. Maximum Common Subgraph

`ISMAGS.largest_common_subgraph()` — finds largest common induced subgraph. Pairwise. NP-complete, same scaling concerns as RDKit's MCS. Returns node mappings.

### 3. Graph Fingerprinting (Weisfeiler-Lehman Hashing)

| Function | Returns | Use |
|----------|---------|-----|
| `weisfeiler_lehman_graph_hash(G)` | Single hex string for entire graph | Fast whole-graph comparison |
| `weisfeiler_lehman_subgraph_hashes(G)` | `{node: [hash_depth_0, hash_depth_1, ...]}` | Local neighborhood fingerprints per node |

WL subgraph hashes are **structurally equivalent to Morgan/ECFP fingerprints** — iterative neighborhood aggregation at increasing radii. The key difference: NetworkX operates on arbitrary labeled graphs without any chemical overhead.

**For boilerplate detection:** Compute WL subgraph hashes for every node in every document PBM. Nodes with identical hash sequences at depth k have isomorphic k-hop neighborhoods. Clustering nodes by hash identifies repeated structural patterns across documents.

### 4. Performance Caveat

NetworkX is pure Python. For graphs with 100K+ nodes, basic operations (BFS, connected components) work but are slow. Subgraph isomorphism on very large graphs may be impractical. For production scale, consider:
- **graph-tool** (C++ with Python bindings, much faster)
- **igraph** (C core, Python/R bindings)
- Custom suffix-tree algorithms for linear chain structures

---

## OpenBabel: Limited Value

OpenBabel (openbabel.org) offers:
- SMARTS-based substructure matching (functional, less feature-rich than RDKit)
- Fingerprinting (FP2 path-based, ECFP circular, customizable FP3/FP4 SMARTS)
- Programmatic molecule construction (`BeginModify()`/`EndModify()` + `SetIsPatternStructure()`)
- **No MCS** — must use RDKit or NetworkX for this

Not recommended over RDKit for this use case. RDKit has stronger analysis tools and better custom-comparator support.

---

## MDTraj: Not Relevant

MDTraj is a trajectory analysis tool (3D coordinates, RMSD, distances). No substructure search, no topology comparison, no graph algorithms. Its only relevant feature is `to_bondgraph()` which converts to a NetworkX graph — at which point you're just using NetworkX.

---

## The Linear Chain Insight

**PBM documents are primarily linear chain graphs** — each token bonded to the next token in document order. This has a major algorithmic implication:

Common sub-structure detection on linear chains = **common substring detection**.

Common substring detection is a **solved O(n) problem** via:
- **Suffix trees** — find all common substrings between two strings in O(n+m)
- **Suffix arrays** — same capability, more memory-efficient
- **Generalized suffix trees** — find common substrings across N strings simultaneously

These algorithms are orders of magnitude faster than graph MCS (NP-complete) for the same structural detection task, because they exploit the linear constraint.

**When graph tools become necessary:**
- If PBMs develop non-linear structure (cross-references, hierarchical bonds, sentence-level connections)
- If sub-PBM matching needs to be topology-aware (not just sequence-aware)
- For structural comparison at the conceptual mesh level (non-linear by nature)

**Recommended approach for boilerplate detection:**
1. **First pass:** Suffix-array-based common substring detection on the token sequence (fast, O(n), finds exact repeated phrases)
2. **Second pass (if needed):** RDKit Morgan fingerprints with custom token-type invariants for fuzzy structural similarity
3. **Targeted MCS:** Only on pre-filtered candidate pairs identified by fingerprint similarity

---

## Assessment Summary

| Capability | Best Tool | Scale Limit | Notes |
|------------|-----------|-------------|-------|
| **MCS (exact)** | RDKit `FindMCS()` | ~500 atoms before timeout | N-way, custom comparators, SMARTS result |
| **MCS (pairwise)** | RDKit `FindMCES()` or NetworkX ISMAGS | ~1000 atoms | Faster than N-way |
| **Subgraph search** | RDKit `GetSubstructMatches()` | 10K+ atoms viable | VF2, custom comparators, multithreaded |
| **Fingerprinting** | RDKit Morgan or NetworkX WL hashing | Unlimited (linear in atoms) | Custom invariants for token types |
| **Bulk similarity** | RDKit `BulkTanimotoSimilarity()` | Millions of comparisons | Sub-second per query |
| **Boilerplate detection** | Suffix trees/arrays on token sequences | Unlimited | O(n), exploits linear structure |
| **OpenMM native** | Nothing | N/A | Dynamics engine, not analysis tool |

### Where OpenMM's Role Boundary Is

OpenMM builds the topology and computes forces. It does not analyze topology structure. The pipeline is:

```
OpenMM: build topology → pair walk → extract bonds
  ↓
PostgreSQL: store bonds → GROUP BY → PBM
  ↓
Analysis tools (RDKit / NetworkX / suffix trees): find common sub-structures across PBMs
  ↓
PostgreSQL: store sub-PBMs as referenced components
```

OpenMM's contribution is the encoding step. Sub-structure analysis is a separate analytical layer that operates on the stored PBM data, not on live OpenMM topologies.

---

## References

- RDKit: rdkit.org, [FMCS docs](https://www.rdkit.org/docs/source/rdkit.Chem.rdFMCS.html), [Fingerprint tutorial](https://greglandrum.github.io/rdkit-blog/posts/2023-01-18-fingerprint-generator-tutorial.html), [Custom MCS callbacks](https://greglandrum.github.io/rdkit-blog/posts/2023-10-27-mcswhatsnew.html)
- NetworkX: networkx.org, [VF2++](https://blog.scientific-python.org/networkx/vf2pp/graph-iso-vf2pp/), [WL hashing](https://networkx.org/documentation/stable/reference/algorithms/graph_hashing.html), [ISMAGS](https://networkx.org/documentation/stable/reference/algorithms/isomorphism.ismags.html)
- OpenBabel: openbabel.org
- MDTraj: mdtraj.org
- Suffix trees: Ukkonen's algorithm (1995), generalized suffix trees for N-way comparison
