# OpenMM Evaluation for PBM Texture Engine

**From:** PBM Specialist
**Date:** 2026-02-14 (initial), 2026-02-16 (internal architecture investigation)
**Status:** Complete evaluation — topology mapping, force framework, internal architecture, engine assessment
**Scope:** Texture Engine only (surface language assembly/disassembly). Conceptual mesh energy states are a separate problem.

---

## Executive Summary

OpenMM is a molecular dynamics engine with a remarkably flexible custom force framework. Its topology model (Chain > Residue > Atom > Bond) maps structurally well to PBM hierarchy (Chapter > Paragraph > Token > Bond). Its Discrete2DFunction lookup tables are an almost perfect match for categorical linguistic force constants.

**Dimensionality note:** OpenMM provides 3 spatial coordinate axes + unlimited per-particle parameters + custom force expressions. The 3 coordinates are free axes that can be aligned with any aspect of speech pattern matching (e.g., syntactic depth, linear position, semantic distance). Per-particle parameters carry additional dimensions (category, sub-cat, LoD level, etc.). This gives us n-dimensional space — more than enough for linguistic force modeling.

**Verdict:** OpenMM can serve as a **GPU-accelerated energy/force calculator** for evaluating bond strengths and identifying constraint violations in PBM structures. The recommended approach for the PoC is static evaluation mode (compute energy + forces for a given configuration). Whether dynamics-driven assembly is viable at envelope scale is an open question worth testing.

For a proof of concept, OpenMM topology mapping is worth building. For production assembly/disassembly, a purpose-built engine on the same principles may be more appropriate.

---

## Aspect 1: PBM Structure ↔ OpenMM Topology

### The Mapping

| PBM Concept | OpenMM Concept | Notes |
|-------------|----------------|-------|
| Document | Topology | Top-level container |
| Chapter/Section | Chain | Independent structural unit |
| Paragraph | Residue | Group of connected tokens |
| Token (word/punct/marker) | Atom | Individual particle with properties |
| Adjacent token bond | Bond | Pairwise connection |
| Token category (N, V, DET...) | Per-particle parameter | Integer-indexed, unlimited count |
| Sub-cat pattern | Per-particle parameter | Pattern ID as integer |
| Bond type (head-comp, modifier...) | Per-bond parameter | Typed via parameter values |
| PBM stream position | Atom coordinate (x) | Linear position; y, z free for other dimensions (e.g., syntactic depth, semantic distance) |

### Topology Construction

OpenMM builds topology programmatically — no file loading needed:

```python
from openmm.app import Topology, Element
from openmm import System

topology = Topology()

# Chapter = Chain
ch1 = topology.addChain(id='chapter_1')

# Paragraph = Residue
p1 = topology.addResidue('para_1', ch1)

# Token = Atom (element is required but arbitrary — use carbon for all)
t1 = topology.addAtom('the', Element.getBySymbol('C'), p1)
t2 = topology.addAtom('cat', Element.getBySymbol('C'), p1)
t3 = topology.addAtom('sat', Element.getBySymbol('C'), p1)

# Adjacent bonds
topology.addBond(t1, t2)  # the-cat
topology.addBond(t2, t3)  # cat-sat
```

**What works well:**
- Hierarchical structure maps cleanly
- Unlimited atoms per residue, residues per chain, chains per topology
- Bonds carry optional type and order parameters
- Systems of 1.2M+ atoms have been tested — far more capacity than needed

**Limitations:**
- `element` field is required (from periodic table, 118 elements). Repurpose as broad category indicator, or use a single dummy element and rely on per-particle parameters for real type info
- Once a Context is created, topology is frozen — can't add/remove atoms without rebuilding
- No `removeParticle()` — only ghost particle deactivation

### PBM Assembly via Ghost Particles

For incremental token-by-token assembly (streaming PBM construction):

```python
MAX_TOKENS = 500  # Pre-allocate for longest expected sentence

# Create ghost particles at initialization
for i in range(MAX_TOKENS):
    atom = topology.addAtom(f'ghost_{i}', Element.getBySymbol('C'), residue)
    system.addParticle(0.0)  # Zero mass = ghost

# Activate a ghost particle as a real token
def activate_token(context, position, token_type, sub_cat, ...):
    force.setParticleParameters(position, [token_type, sub_cat, ...])
    force.updateParametersInContext(context)  # Cheap GPU update
```

**Assessment:** This works but is awkward. Pre-allocation means guessing maximum size. Parameter updates are cheap (no GPU kernel rebuild), but the ghost particle pattern adds complexity. For static evaluation (analyze a complete PBM), building the full topology once is simpler and preferred.

### PBM Disassembly

Read forces/energies to determine bond characteristics:

```python
state = context.getState(getEnergy=True, getForces=True)
energy = state.getPotentialEnergy()
forces = state.getForces()  # Per-token force vectors

# High force on a token = constraint violation or strong attraction
# Low energy = stable/well-formed structure
# Energy difference between configs = relative quality of parses
```

This is the **primary value proposition**: OpenMM as a force/energy evaluator for PBM structures.

### Topology Verdict

**CAN OpenMM handle PBM assembly/disassembly natively?**

- **Static analysis (given a complete PBM, evaluate its forces):** YES — build topology, assign parameters, compute energy/forces. Clean and efficient.
- **Incremental assembly (build PBM token by token):** POSSIBLE but awkward — ghost particles work but add complexity.
- **Dynamic assembly (let forces guide token placement):** Untested — worth exploring at envelope scale where dynamics-driven assembly across many molecules may emerge naturally. The 3 coordinate axes can encode meaningful linguistic dimensions, not just linear position.

---

## Aspect 2: Linguistic Force Bonding ↔ OpenMM Custom Forces

### Force Framework Capabilities

OpenMM's custom force system accepts arbitrary algebraic energy expressions with:
- **Per-particle parameters:** `addPerParticleParameter('pos_tag')` — unlimited count, accessed as `param1`, `param2` in pairwise expressions
- **Per-bond parameters:** `addPerBondParameter('bond_type')` — stored on each bond
- **Global parameters:** Shared constants, cheaply modifiable at runtime
- **Tabulated functions:** `Discrete2DFunction(xsize, ysize, values)` — integer-indexed lookup tables, no interpolation
- **Logic functions:** `step(x)` (0 if x<0, else 1), `delta(x)` (1 if x=0), `select(x,y,z)` (conditional)

### Mapping the 7 Force Types

#### 1. Attraction Forces → CustomNonbondedForce + Discrete2DFunction

```python
# Build attraction strength lookup table: category_i × category_j → strength
# E.g., V(trans) × NP → 0.9 (strong), ADJ × NP → 0.6 (moderate)
attraction_table = Discrete2DFunction(NUM_CATEGORIES, NUM_CATEGORIES, flat_values)

force = CustomNonbondedForce('attr_strength(cat1, cat2) * f(r)')
force.addTabulatedFunction('attr_strength', attraction_table)
force.addPerParticleParameter('cat')  # Category as integer
```

**Assessment:** EXCELLENT fit. Discrete2DFunction is exactly what we need — a lookup table from (category_A, category_B) → attraction strength. This IS the compiled PBM bond strength data the Orchestrator described.

#### 2. Binding Energy → CustomBondForce

```python
# Per-bond binding energy: cost to remove this bond
force = CustomBondForce('bind_energy * (1 - step(r - cutoff))')
force.addPerBondParameter('bind_energy')  # 0.0-1.0
# Complement bonds: 0.95, adjunct bonds: 0.3, etc.
```

**Assessment:** GOOD fit. Per-bond parameters carry the binding energy value directly. Energy contribution is proportional to binding strength.

#### 3. Ordering Constraints → CustomCompoundBondForce

Ordering uses one coordinate axis (x = linear position) with penalty functions for constraint violations. The other two axes (y, z) remain free for other linguistic dimensions.

```python
# Trigram ordering: token at position i should precede token at position j
# x-coordinate = linear position in stream
force = CustomCompoundBondForce(2, 'order_penalty * step(x1 - x2)')
# Penalize if token 1 appears after token 2 (wrong order)
force.addPerBondParameter('order_penalty')  # 1.0 = absolute, 0.8 = preference
```

**Assessment:** GOOD fit. Linear position maps naturally to x-coordinate. Ordering violations become energy penalties — exactly how OpenMM handles geometric constraints. The y and z axes can encode syntactic depth, semantic distance, or other dimensions that are useful for the remaining force types.

**Alternative:** Encode ordering constraints as energy penalties in a tabulated function indexed by (position_delta, constraint_type). A negative position_delta (wrong order) incurs a penalty scaled by constraint strength.

#### 4. Category Compatibility → Discrete2DFunction (binary)

```python
# Compatibility matrix: can category_A bond with category_B?
# 1.0 = compatible, 0.0 = incompatible (coordination constraint, etc.)
compat_table = Discrete2DFunction(NUM_CATS, NUM_CATS, compatibility_matrix)

force = CustomNonbondedForce('compat(cat1, cat2) * energy_if_bonded(r)')
force.addTabulatedFunction('compat', compat_table)
```

**Assessment:** EXCELLENT fit. Binary compatibility is a natural lookup table. The coordination constraint (only same-category constituents coordinate) is a simple diagonal in the matrix.

#### 5. Valency → CustomGBForce (multi-stage)

Valency requires counting filled slots — a multi-particle aggregation. OpenMM's CustomGBForce supports multi-stage computation:

```python
# Stage 1: Count how many bonds each token has of each type
# Stage 2: Compare to expected valency, penalize violations
force = CustomGBForce()
force.addComputedValue('filled_slots', '...count nearby bonded tokens...')
force.addEnergyTerm('valency_penalty * max(0, expected_val - filled_slots)^2')
force.addPerParticleParameter('expected_val')
```

**Assessment:** MODERATE fit. CustomGBForce can aggregate per-particle values, but expressing "count bonds of specific types" in its expression language requires creative encoding. The conceptual mapping is sound — unfilled valency slots cost energy — but the implementation is non-trivial.

**Alternative:** Pre-compute valency satisfaction outside OpenMM and inject as per-particle parameters. Only use OpenMM for the force/energy calculation, not the slot-counting logic.

#### 6. Movement Forces → NOT a natural fit

Movement (passive, wh-fronting, etc.) involves topology changes: a token at one structural position is relocated to another, leaving a trace. This is topology mutation — adding/relocating particles — not force evaluation.

**Assessment:** POOR fit for dynamic movement. Static evaluation (given a configuration with traces already placed, evaluate its energy) works. But the act of movement — detecting triggers and relocating tokens — is control logic, not physics.

**Recommendation:** Handle movement rules in the control layer (Python code), not in OpenMM forces. After movement produces a candidate structure, evaluate it with OpenMM.

#### 7. Structural Repair → NOT a natural fit

Structural repair inserts tokens (do-support, expletive "it", empty determiners). This is topology modification — adding particles.

**Assessment:** POOR fit. Ghost particle activation could technically model this, but it's fighting the framework. Repair is a constraint-satisfaction problem (detect violation → insert token), not an energy minimization problem.

**Recommendation:** Handle repair rules in the control layer. After repair produces a candidate structure, evaluate with OpenMM.

### Force Mapping Summary

| Force Type | OpenMM Mechanism | Fit Quality | Notes |
|------------|-----------------|-------------|-------|
| Attraction | CustomNonbondedForce + Discrete2DFunction | EXCELLENT | Lookup table = compiled PBM bond strengths |
| Binding Energy | CustomBondForce + per-bond parameters | GOOD | Direct energy cost per bond |
| Ordering | CustomCompoundBondForce + x-coordinate | GOOD | x = linear position, penalty for inversions |
| Category Compat | Discrete2DFunction (binary matrix) | EXCELLENT | Natural compatibility lookup |
| Valency | CustomGBForce or pre-computed | MODERATE | Slot counting is awkward in expressions |
| Movement | Control layer, not force | POOR | Topology mutation, not energy |
| Structural Repair | Control layer, not force | POOR | Token insertion, not force |

**Bottom line:** 5 of 7 force types map well to OpenMM (attraction, binding energy, ordering, category compatibility, valency). The remaining 2 (movement, repair) are control logic that orchestrates OpenMM evaluations — they involve topology mutation, not force computation.

---

## The Key Insight: Compiled PBM Bond Strengths

The Orchestrator's insight: *"Compiled PBM data represents BOND STRENGTH between possible tokens within an envelope. Within the relevant envelopes, how common is the next token bond."*

This maps directly to OpenMM's Discrete2DFunction:

```python
# From compiled PBM data:
# Given token type A at position N, what is the bond strength to
# token type B at position N+1?
#
# This is a 2D lookup table: token_type_A × token_type_B → strength
# The table IS the compiled PBM — it's the statistical frequency of
# adjacent token bonds observed across the corpus.

# Build from PBM corpus:
bond_strengths = {}
for doc in pbm_corpus:
    for i in range(len(doc.stream) - 1):
        pair = (doc.stream[i].category, doc.stream[i+1].category)
        bond_strengths[pair] = bond_strengths.get(pair, 0) + 1

# Normalize to 0.0-1.0 and load into OpenMM:
table = Discrete2DFunction(NUM_TYPES, NUM_TYPES, normalized_values)
```

Within an "envelope" (a paragraph, a clause, a sub-cat frame), the bond strength between adjacent tokens tells the engine how likely this assembly is. **Low energy = common/natural assembly. High energy = unusual/strained assembly.**

This is where OpenMM's energy minimization becomes genuinely useful: given a set of tokens and force constants derived from PBM statistics, find the arrangement that minimizes total energy. That arrangement IS the most natural surface expression.

---

## Practical Architecture

### Recommended Split

```
┌─────────────────────────────────────────┐
│           CONTROL LAYER (Python)         │
│                                          │
│  Movement rules    ← linguistics spec    │
│  Structural repair ← linguistics spec    │
│  Ordering logic    ← partially here      │
│  Topology construction                   │
│  Ghost particle management               │
│  LoD transitions (phrase → clause → ...)│
│                                          │
│  Builds candidate structures, then:      │
│           ↓                              │
│  ┌────────────────────────────────────┐  │
│  │     OpenMM ENGINE (GPU)            │  │
│  │                                    │  │
│  │  Attraction forces (Discrete2D)    │  │
│  │  Binding energies (per-bond)       │  │
│  │  Category compatibility (Discrete2D)│ │
│  │  Valency penalties                 │  │
│  │  Ordering penalties (partial)      │  │
│  │                                    │  │
│  │  → Returns: total energy,          │  │
│  │    per-token forces,               │  │
│  │    constraint violations           │  │
│  └────────────────────────────────────┘  │
│           ↓                              │
│  Control layer uses energy/forces to:    │
│  - Score candidate parses                │
│  - Identify constraint violations        │
│  - Guide assembly decisions              │
│  - Select between alternatives           │
└─────────────────────────────────────────┘
```

### What This Gives Us

1. **Parse scoring:** Given a sentence, generate candidate token arrangements, evaluate each with OpenMM, pick lowest energy
2. **Constraint detection:** Per-token force vectors reveal which tokens are under strain (valency violations, ordering problems)
3. **Assembly guidance:** During generation, bond strength tables tell the engine which token to place next (highest attraction within the envelope)
4. **GPU acceleration:** For large-scale processing (batch evaluation of many sentences), OpenMM's CUDA backend provides parallelism

### What This Doesn't Give Us

1. **Dynamic simulation:** Tokens don't "float" to their correct positions — assembly is driven by control logic, not dynamics
2. **LoD aggregation:** Phrase→clause→sentence level transitions are control logic, not OpenMM forces. OpenMM evaluates one LoD level at a time.
3. **Creative generation:** The engine scores/evaluates — it doesn't generate. Generation requires an outer loop that proposes candidates for scoring.

---

## Proof of Concept Proposal

### Step 1: Topology Mapping (minimal)

Take one encoded sentence from the PBM corpus. Build an OpenMM topology:
- Each token = one atom with per-particle parameters (category, sub-cat, position)
- Adjacent tokens bonded
- Assign positions: x = stream position, y = syntactic depth (or 0 for PoC), z = 0 for PoC

Verify: topology builds, atom count matches, bonds are correct.

### Step 2: Force Evaluation (static)

Define two custom forces:
1. **Attraction** via Discrete2DFunction: category pair → strength
2. **Binding energy** via per-bond parameters

Evaluate energy for the correct word order. Then swap two words and re-evaluate. The correct order should have lower energy.

### Step 3: Bond Strength Tables from PBM Data

Compile adjacent-token bond frequencies from the 110 encoded documents:
```sql
SELECT a.p3 as cat_left, b.p3 as cat_right, COUNT(*) as frequency
FROM pbm_content a
JOIN pbm_content b ON a.doc_id = b.doc_id AND a.position = b.position - 1
GROUP BY a.p3, b.p3
ORDER BY frequency DESC;
```

Load these as a Discrete2DFunction. This IS the compiled texture data.

### Step 4: Energy Minimization Test

Given a shuffled sentence, run OpenMM energy minimization. Does the result approach the correct word order? This tests whether the compiled bond strengths contain enough information to reconstruct surface expression.

---

## Coordination Note

The 7 force type definitions (attraction, binding energy, ordering, category compatibility, valency, movement, structural repair) are defined by the **linguistics specialist** (docs/research/english-force-patterns.md). The force CONSTANTS (specific strength values, sub-cat patterns, ordering rules) are language-specific and live in the English shard.

This evaluation covers the **engine** that evaluates those forces — not the forces themselves. Force definitions should be coordinated with the linguistics specialist before implementing Step 2+.

---

## Open Questions (from initial evaluation)

1. ~~**Is 3D embedding worth the cost?**~~ **Resolved (2026-02-16):** The question was based on a false premise. OpenMM provides 3 coordinate axes + unlimited per-particle parameters + custom force expressions — this is n-dimensional space, not "3D." The 3 spatial coordinates are free axes assignable to any linguistic dimension (linear position, syntactic depth, semantic distance). Per-particle parameters carry all remaining dimensions. There is no embedding cost — the dimensional space is a resource, not a constraint.

2. **Discrete2DFunction scaling:** With ~1.4M tokens in hcp_english, a full token×token matrix is impossible (1.96 trillion entries). Must use category-level tables (~17 structural categories × 17 = 289 entries) or sub-category level (~30 × 30 = 900 entries). Token-specific bond strengths would need a different approach (sparse tables or hierarchical lookup).

3. **LoD transitions:** How does the engine handle phrase-level aggregation? OpenMM can't natively "collapse" a set of atoms into a single rigid body mid-simulation. Multi-scale would require multiple OpenMM evaluations — one per LoD level — with the control layer managing the transitions.

4. ~~**Comparison baseline:** Before committing to OpenMM, should we prototype the same force evaluation in pure Python/NumPy to understand what OpenMM actually buys us? For sentence-scale problems (10-50 tokens), the overhead of OpenMM setup may exceed the computation time.~~ **Resolved (2026-02-16):** This was based on a sentence-scale assumption. At envelope scale (many PBMs, thousands of tokens across LoD levels, sustained computation), GPU parallelism is clearly justified. NumPy cannot compete with OpenMM's CUDA backend for concurrent multi-molecule force evaluation.

---

## Internal Architecture Investigation (2026-02-16)

### The Data Flow Question

The brief asked: can we bypass XML serialization and write directly to OpenMM's internal data layer? The answer is that **this question is moot** — the programmatic API IS the direct path. XML was never in our critical path.

### How OpenMM Actually Works Internally

**Three-layer architecture:**

```
Layer 1: System/Force objects (CPU heap)
  - Plain C++ STL containers: std::vector<double> for masses,
    std::vector<BondInfo> for bonds, etc.
  - System is a pure data container — no computation, no GPU
  - Python objects are thin SWIG wrappers around C++ objects
  - addParticle() = vector push_back, addBond() = vector push_back

Layer 2: Context/ContextImpl (CPU + GPU bridge)
  - Created from System + Integrator + Platform
  - THIS is where GPU setup happens:
    a) Platform allocates CudaContext (GPU arrays for pos/vel/force)
    b) Each Force creates a ForceImpl which creates platform kernels
    c) ForceImpl.initialize() uploads force parameters to GPU
    d) GPU kernels are compiled for the specific topology
  - Context creation is O(N) and is the primary bottleneck

Layer 3: CudaContext (GPU memory)
  - posq: float4 array (positions + charge)
  - velm: float4 array (velocities + inverse mass)
  - force: long long array (forces in fixed-point)
  - Per-force GPU buffers (bond parameters, lookup tables, etc.)
  - Particles periodically REORDERED for spatial locality (cache efficiency)
```

### XmlSerializer: What It Actually Does

Serialization uses a **Proxy pattern**. Each class (System, HarmonicBondForce, etc.) registers a SerializationProxy that knows how to read/write its members.

**Serialize:** C++ object → SerializationProxy reads members → SerializationNode tree → XML text
**Deserialize:** XML text → SerializationNode tree → Proxy calls `addParticle()`/`addBond()` in loops → Fresh C++ objects

The critical insight: **deserialize() calls the exact same API as programmatic construction.** There is zero shortcut. The XML path is:

```
XML → parse → call addParticle()/addBond() in loops → C++ vectors → Context → GPU
```

The programmatic path is:

```
call addParticle()/addBond() in loops → C++ vectors → Context → GPU
```

XML adds overhead; it never provided a shortcut. The `clone()` method exists specifically to bypass XML while still using the SerializationNode intermediate representation.

### What Our Actual Data Path Would Be

```
PostgreSQL → Python (psycopg2) → system.addParticle() / force.addBond()
  → SWIG → C++ std::vector push_backs (microseconds)
  → Context(system, integrator, platform) creation (THE BOTTLENECK)
    → GPU kernel compilation (cached after first run)
    → Force parameter upload to GPU (O(N) memcpy)
  → context.setPositions() (uploads positions to GPU)
  → Ready to evaluate
```

### Where the Time Actually Goes

| Operation | Cost | Notes |
|-----------|------|-------|
| DB query | ~ms | PostgreSQL round-trip |
| addParticle()/addBond() loop | ~ms for 1K bonds | Vector push_backs, negligible |
| Context creation (first time) | ~seconds | GPU kernel compilation dominates |
| Context creation (cached) | ~100ms | Kernel cache on disk eliminates compilation |
| setPositions() | ~μs for 500 particles | Small CUDA memcpy |
| getState(energy=True) | ~ms | Force evaluation + GPU→CPU download |
| updateParametersInContext() | ~μs | Parameter-only update, no kernel rebuild |

**At envelope scale (the real operating unit):** The HCP doesn't process single sentences in isolation. One document = one molecule, and the engine operates on multiple PBMs simultaneously within an envelope — cross-referencing entity relationships, computing force accumulation across the conceptual mesh. When LoD stacking scales from sentences to full-length works with thousands of sentences in branches and adjustments, the particle counts reach exactly the range OpenMM was built for (10K-1M+). Context creation cost is amortized across sustained computation on a large, evolving system — not paid per sentence.

**For Context reuse:** Build a Context sized for the envelope's working set. Use ghost particle activation/deactivation for streaming PBM construction. Use `updateParametersInContext()` for parameter-only changes (no kernel rebuild). The Context persists for the lifetime of an active envelope.

### Paths to More Direct Access

| Approach | Eliminates | Practical? |
|----------|-----------|------------|
| Raw SWIG module (skip unit wrapping) | Python unit overhead | Yes, trivial |
| C++ program with libpq | Python entirely | Yes, but marginal gain |
| Custom Force plugin (reads from mmap) | addBond() loop, parameter readback | Yes, but significant dev effort |
| Skip Context, use OpenMM math only | GPU kernel overhead | Not possible — kernels are the point |

**Bottom line:** There is no "fast lane" into OpenMM that bypasses Context creation. The System/Force API is already the thinnest possible interface — it's just vector push_backs. The cost is in GPU kernel compilation and data formatting, which are inherent to GPU computation.

---

## Architectural Assessment: OpenMM vs. Custom Engine

### What OpenMM Gives Us

1. **GPU-accelerated force evaluation** — the Discrete2DFunction lookup tables are an excellent match for compiled PBM bond strengths
2. **Battle-tested numerics** — decades of molecular dynamics optimization
3. **Parallel evaluation** — thousands of force calculations simultaneously on GPU
4. **CustomForce flexibility** — arbitrary algebraic energy expressions with per-particle/per-bond parameters
5. **Energy minimization** — L-BFGS via LocalEnergyMinimizer, finds lowest-energy configuration

### What OpenMM Costs Us

1. **Continuous→discrete mismatch** — tokens are discrete; OpenMM uses continuous coordinates. Ordering constraints become energy penalty functions rather than structural guarantees. This is a modelling choice, not a limitation — penalty-based ordering is how OpenMM handles all geometric constraints.
2. **Frozen topology** — can't add/remove particles without rebuilding Context. At envelope scale, a pre-allocated particle pool is a standard pattern (similar to game engine entity pools).
3. **LoD transitions require multiple evaluations** — phrase→clause→sentence aggregation means the control layer manages transitions between OpenMM evaluations at each level.
4. **2 of 7 force types need control layer** — movement and structural repair involve topology mutation (inserting/relocating tokens), which is control logic, not force computation.

### The HCP Needs Assessment

Patrick's question: do we need a custom physics-derived engine that unifies the math?

The HCP needs four types of computation:

| Need | What It Is | OpenMM Fit |
|------|-----------|------------|
| PBM assembly/disassembly | Molecular topology traversal | Good — this IS molecular topology |
| Linguistic force bonding | 7 force types on token sequences | 5/7 native, 2/7 via control layer (movement, repair = topology mutation) |
| Conceptual mesh energy | Energy states across concept tokens | Plausible — same force evaluation model, different force definitions |
| Multi-modal expression | Same mesh, different surface engines | OpenMM handles one modality's texture; other modalities would need their own force definitions but could share the same engine |

### Scale Correction (2026-02-16)

The initial evaluation incorrectly framed the operating scale as sentence-level. The actual operating unit is the **envelope**: multiple PBMs (documents = molecules) loaded simultaneously, cross-referencing entity relationships, computing force accumulation across the conceptual mesh. Key points:

1. **One document = one molecule in a larger system.** Any single input molecule is only one aspect of a much larger pattern. Multiple PBMs interacting simultaneously — which is exactly what OpenMM was built for.

2. **LoD stacking scales fast.** Composing one sentence is trivial. Composing thousands in branches and adjustments across full-length works puts the particle count squarely in OpenMM's sweet spot (10K-1M+).

3. **GPU parallelism is justified at envelope scale.** Context creation cost is amortized across sustained computation on a large, persistent system — not paid per sentence. The Context lives for the lifetime of an active envelope.

4. **The dimensional space is ample.** OpenMM provides 3 coordinate axes + unlimited per-particle parameters + custom force expressions = n-dimensional space. The 3 coordinates are free axes alignable with any linguistic dimension (linear position, syntactic depth, semantic distance). Per-particle parameters carry category, sub-cat, LoD level, and any other force-relevant dimensions. There is no dimensionality constraint.

**Revised assessment:** OpenMM handles the texture engine computation well. The control layer handles what OpenMM can't (movement, repair, LoD transitions). The remaining question is whether the conceptual mesh energy states can use the same engine with different force definitions, or whether they need a separate approach. That question is out of scope for now.

### Recommendation

**Commit to OpenMM for the texture engine.** The operating scale justifies GPU parallelism, and the force evaluation model (Discrete2DFunction lookup tables, per-bond parameters, CustomForce expressions) maps well to 5 of 7 linguistic force types natively. The remaining 2 (movement, structural repair) are topology mutations that belong in the control layer regardless of engine choice.

**Architecture:**
- OpenMM Context = persistent per-envelope, sized for the envelope's working set
- Ghost particle pool pre-allocated for streaming PBM construction
- Control layer (Python) handles movement rules, structural repair, and LoD transitions
- `updateParametersInContext()` for bond strength updates without kernel rebuild
- Discrete2DFunction tables = compiled PBM bond strengths (the core value proposition)

**Open architectural question (for later):** Whether the conceptual mesh (separate from texture) can also run on OpenMM with different force definitions, or needs a different approach. Do not address this yet — texture engine first.

**Alternative engines (Godot/Rapier, custom) remain worth noting** for the multi-modal expression face and game-engine orchestration pattern described in architecture.md, but they are not competitors to OpenMM for the texture engine — they address different architectural faces.

---

## Next Steps

1. ~~**Install OpenMM** and verify GPU platform availability~~ ✓ (OpenMM 8.2, CUDA platform confirmed)
2. ~~**Investigate internal architecture**~~ ✓ (2026-02-16 — data flow, XML bypass, scale assessment)
3. **Build minimal PoC** (Steps 1-2 from PoC Proposal): one sentence, two forces, energy comparison
4. **Compile bond strength tables** from PBM corpus (Step 3 — requires PBM data, currently no PBM DB)
5. **Coordinate with linguistics specialist** on force constant values for Step 2
6. **Prototype envelope-scale Context** — pre-allocated ghost particle pool, multi-chain topology (multiple PBMs), Discrete2DFunction tables loaded from compiled bond strengths

---

## References

- OpenMM documentation: docs.openmm.org
- Custom forces: docs.openmm.org/latest/userguide/theory/03_custom_forces.html
- Discrete2DFunction: docs.openmm.org API — TabulatedFunction subclasses
- OpenMM source: github.com/openmm/openmm
- OpenMM C++ headers (local): /opt/project/openmm-env/envs/openmm/include/openmm/
- HCP force patterns: docs/research/english-force-patterns.md
- HCP force DB requirements: docs/research/force-pattern-db-requirements.md
