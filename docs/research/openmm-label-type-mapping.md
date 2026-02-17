# OpenMM Text Input via Labels: Feasibility Report

**From:** PBM Specialist
**Date:** 2026-02-17
**Status:** Complete — research only, no code written
**Task:** Can OpenMM accept words as atom labels and map token IDs as element types via its native typing system?

---

## Executive Summary

OpenMM has **two separate type systems** at different architectural layers:

1. **ForceField (Python-only):** Label → atom type → force parameters. Designed for chemical template matching. CAN work with non-chemical labels (element=None supported throughout), but the matching infrastructure (element signatures, bond-topology graph isomorphism) creates friction for a 1.4M-token vocabulary.

2. **Per-particle parameters + Discrete2DFunction (C++ / GPU):** Integer token IDs as doubles on Force objects, used as indices into lookup tables. This is where computation actually happens. Clean, scalable, no vocabulary limit beyond GPU memory.

**Finding:** The word label → token ID mapping does NOT belong in OpenMM's ForceField. It belongs in the thin glue layer (DB lookup). What OpenMM handles natively is the token ID → force computation path via per-particle parameters and tabulated functions.

**The pipeline clarification:**

```
Text ──split on spaces──→ word sequence
  → DB lookup (glue layer): surface form → token_id
  → OpenMM topology: addAtom(name=word, element=None)
  → OpenMM System: addParticle(mass=1.0)
  → CustomNonbondedForce: addParticle([token_id])
  → Discrete2DFunction: interaction_table(token_id1, token_id2) → bond strength
```

---

## OpenMM's Architecture: Two Worlds

### The Python App Layer (openmm.app)

This layer — Element, Topology, ForceField — is **pure Python**. It has no C++ counterpart. It exists to read chemical file formats (PDB, CIF) and produce a C++ System.

| Component | What it stores | Chemical purpose | Our relevance |
|-----------|---------------|-----------------|--------------|
| Element | symbol, mass, atomic_number | Periodic table identity | None — can be `None` |
| Topology.Atom | name (any string), element, residue | Structural position | `name` = surface form (metadata only) |
| ForceField | atom types, residue templates, force generators | Chemical parameterization | Could work, but fighting the framework |

### The C++ Core Layer (openmm)

This is where computation happens. **It knows nothing about labels, elements, or types.**

| Component | What it stores | How identity works |
|-----------|---------------|-------------------|
| System.addParticle(mass) | A mass value. That's it. | Integer index only |
| CustomNonbondedForce | Per-particle parameter vectors (doubles) | Parameters carry all identity |
| Discrete2DFunction | Integer-indexed lookup table | `table(param1, param2)` → value |
| CustomManyParticleForce | Explicit integer `type` field per particle | Built-in type filtering |

**The C++ System is a bag of anonymous masses + forces. All "identity" flows through per-particle parameters on Force objects.**

---

## Investigation: Can ForceField Do Label→Type Mapping?

### What Works

1. **Element is optional everywhere.** Atom types can be registered with `element=None`:
   ```python
   ff.registerAtomType({'name': 'AB.AC.BF.AA.AB', 'class': 'noun', 'mass': 1.0})
   # No element needed
   ```

2. **Atom names are arbitrary strings.** `topology.addAtom("the", element=None, residue=r)` is valid. No validation.

3. **Fully programmatic API.** Types, templates, and generators can all be registered in Python:
   ```python
   template = ForceField._TemplateData('the')
   template.addAtom(ForceField._TemplateAtomData('w0', 'AB.AC.BF.AA.AB', None))
   ff.registerResidueTemplate(template)
   ```

4. **Type/class hierarchy maps to our needs:**
   - Type = specific token ID (unique per vocabulary entry)
   - Class = part-of-speech or semantic category
   - Force parameters can be defined at either level

5. **Custom template matchers** (`registerTemplateMatcher`) can override the default matching logic entirely — bypassing both element signatures and graph isomorphism.

6. **Custom template generators** (`registerTemplateGenerator`) can create templates on-the-fly for unknown residues.

7. **`UseAttributeFromResidue`** lets force parameters come from the template level (per-occurrence) rather than the type level (global).

### What Creates Friction

1. **Element signature collapse.** Templates are indexed by element signature (e.g., "C6H12O6"). When `element=None`, ALL templates share the empty string `""` as their signature. Every residue match would search ALL 1.4M templates linearly. This is O(V) per match — unacceptable.

   **Fix:** Custom template matcher bypasses this entirely. But then we're writing a custom matcher, not using OpenMM's native resolution.

2. **Graph isomorphism matching is overkill.** The compiled Cython matcher does bond-topology subgraph matching with backtracking. For our use case (one word = one residue = one atom), this is unnecessary. A simple name lookup suffices.

   **Fix:** Again, custom template matcher bypasses this.

3. **One residue template per word.** With 1.4M vocabulary entries, that's 1.4M Python `_TemplateData` objects, each with `_TemplateAtomData` children. Memory cost: ~500+ bytes per template × 1.4M ≈ 700MB+ of Python objects just for the template registry.

4. **createSystem() iterates all atoms through template matching.** Even with a custom matcher doing O(1) lookup, the ForceField still runs the full pipeline: match templates → assign types → create force generators → populate System. This adds Python overhead that doesn't exist if we build the System directly.

5. **The ForceField XML format assumes chemical conventions.** Using it means encoding our vocabulary in a chemical-shaped container — technically possible but semantically misleading.

### Verdict on ForceField Path

**Possible but wrong.** We'd be registering a custom template matcher (our own code) to bypass OpenMM's native matching, defining 1.4M templates (wasteful), and running through createSystem()'s full pipeline when we could just call addParticle() directly. The ForceField doesn't add value — it adds overhead and conceptual mismatch.

---

## Investigation: Per-Particle Parameters as Token IDs

### How It Works

The C++ `CustomNonbondedForce` supports arbitrary per-particle parameters:

```cpp
force->addPerParticleParameter("token_id");      // Define parameter slot
force->addParticle({42.0});                       // Particle 0: token_id = 42
force->addParticle({87.0});                       // Particle 1: token_id = 87
```

In the energy expression, parameters get `1`/`2` suffixes for the two interacting particles:
```
"interaction_table(token_id1, token_id2)"
```

`Discrete2DFunction` rounds inputs to nearest integer and does table lookup:
```cpp
// Table: vocab_size × vocab_size → interaction strength
force->addTabulatedFunction("interaction_table",
    new Discrete2DFunction(vocab_size, vocab_size, table_values));
```

**This is confirmed from source code analysis of the Lepton expression parser and all platform kernels (Reference, Common/CUDA).** Per-particle parameters are regular expression variables, identical in status to `r` (distance). They can appear anywhere in expressions, including as function arguments.

### Properties

| Property | Status |
|----------|--------|
| Token ID as per-particle parameter | Works — doubles represent integers exactly up to 2^53 |
| Used as Discrete2DFunction index | Works — function rounds to nearest integer |
| Number of per-particle parameters | No coded limit (vector storage) |
| GPU compatibility | Full — parameters uploaded to GPU, used in kernel expressions |
| Vocabulary size limit | GPU memory only (~1.4M × 1.4M table = 15.7TB, far too large for full pairwise. Category-level tables work: 30×30 = 7.2KB) |
| Expression symmetry requirement | Table must be symmetric: `table[i,j] == table[j,i]`. PBM bond counts are already symmetric. |

### CustomManyParticleForce: Built-in Integer Types

This force class has an explicit type system separate from per-particle parameters:
```cpp
int addParticle(const std::vector<double>& parameters, int type=0);
void setTypeFilter(int index, const std::set<int>& types);
```

Types are integers used for filtering which particle combinations to evaluate. Could be used for category-level filtering (only evaluate noun-verb pairs, skip noun-noun, etc.).

---

## The Label Question: Where Does Mapping Happen?

Patrick's goal: "feed raw text as labels, token ID system defines the chemistry, OpenMM handles everything natively."

There are two mappings in the pipeline:

### Mapping 1: Surface Form → Token ID

`"the"` → `AB.AC.BF.AA.AB` → (integer index for per-particle parameter)

This is a **vocabulary lookup**. It's a DB operation:
```sql
SELECT token_id FROM hcp_english.tokens WHERE label = 'the'
UNION
SELECT token_id FROM hcp_core.tokens WHERE label = 'the'
```

**This cannot be OpenMM's job.** OpenMM has no database access, no string-matching engine, and its ForceField template system is the wrong tool (see friction analysis above). This lookup is inherently a glue-layer operation.

### Mapping 2: Token ID → Force Parameters

`token_id = 42` → `{attraction: 0.8, binding_energy: 0.6, category: 3}`

This IS OpenMM's job, and it does it natively via:
- Per-particle parameters carry the token_id
- Discrete2DFunction tables map (token_id_A, token_id_B) → interaction strength
- CustomForce expressions compute energy from these values

**This is where "OpenMM IS the encoder" applies.** The force computation — evaluating bond strengths, computing energies, running minimization — is native OpenMM molecular function. The token ID is the chemical identity.

### Where Atom Labels Fit

`topology.addAtom(name="the", element=None, residue=r)` stores the surface form as metadata on the Topology. This metadata:
- Never reaches the C++ System (System only gets mass)
- Never reaches the GPU
- Is useful for human-readable output and debugging
- Can be used to reconstruct the text from the topology structure

The atom `name` IS the word. But it's **metadata**, not a computational input. The computational input is the token_id per-particle parameter.

---

## Revised Pipeline Understanding

```
Input: "The cat sat"

Step 1: Split on spaces (the ONLY preprocessing)
  → ["The", "cat", "sat"]

Step 2: Glue layer — DB lookup for each surface form
  → [("The", token_id=142), ("cat", token_id=87), ("sat", token_id=203)]
  (hcp_english for words, hcp_core for punctuation/structural markers)

Step 3: Build OpenMM Topology (Python app layer — metadata)
  chain = topology.addChain()
  residue = topology.addResidue("document", chain)
  a0 = topology.addAtom("The", element=None, residue)    # label = surface form
  a1 = topology.addAtom("cat", element=None, residue)
  a2 = topology.addAtom("sat", element=None, residue)
  topology.addBond(a0, a1)   # adjacent pair
  topology.addBond(a1, a2)   # adjacent pair

Step 4: Build OpenMM System (C++ core — computation)
  system = System()
  system.addParticle(1.0)  # particle 0 — anonymous
  system.addParticle(1.0)  # particle 1 — anonymous
  system.addParticle(1.0)  # particle 2 — anonymous

Step 5: Attach force with token IDs as per-particle parameters
  force = CustomNonbondedForce("bond_strength(token_id1, token_id2)")
  force.addPerParticleParameter("token_id")
  force.addParticle([142.0])  # "The"
  force.addParticle([87.0])   # "cat"
  force.addParticle([203.0])  # "sat"
  force.addTabulatedFunction("bond_strength",
      Discrete2DFunction(vocab_size, vocab_size, compiled_pbm_table))
  system.addForce(force)

Step 6: OpenMM native operations (pair walk, energy evaluation, etc.)
  → THIS is where OpenMM is the encoder
```

---

## Key Findings Summary

| Question | Answer |
|----------|--------|
| Can we register 1.4M custom Elements? | Technically yes, but pointless — Element never reaches C++/GPU |
| Can ForceField map word labels to types? | Yes with custom matchers, but wasteful (1.4M templates, 700MB+ Python objects) |
| Is ForceField the right mechanism? | **No** — adds overhead without value. Direct System construction is simpler |
| Can per-particle params carry token IDs? | **Yes** — doubles represent integers exactly, work in expressions and as table indices |
| Can Discrete2DFunction use token IDs? | **Yes** — inputs rounded to integer, per-particle params are valid arguments |
| Where does surface form → token ID mapping belong? | **Glue layer** (DB lookup), not OpenMM |
| Where does token ID → force computation belong? | **OpenMM** (per-particle params + Discrete2DFunction + CustomForce) |
| What is the atom name for? | **Metadata** — stores the surface form for human readability, never reaches GPU |

---

## Implications for Design Rules

The original design rule concept — "text IS the direct input, no separate tokenization step" — needs nuance:

1. **Text IS the input to the pipeline.** No preprocessing beyond splitting on spaces.
2. **The glue layer does vocabulary lookup** (surface form → token ID). This is a thin DB query, not "tokenization" in the NLP sense. It's looking up our periodic table.
3. **OpenMM receives token IDs as per-particle parameters.** The token ID IS the chemical identity — it determines all force interactions via lookup tables.
4. **Atom labels (surface forms) are metadata** preserved on the Topology for reconstruction.

The "no separate tokenization" principle holds: we're not running a BPE tokenizer or breaking words into subwords. We're doing a vocabulary lookup — the equivalent of looking up an element in the periodic table. The word "the" IS element 142 in our chemistry. The lookup is just reading the periodic table.

---

## Open Questions for Patrick

1. **Vocabulary-scale lookup tables.** A full 1.4M × 1.4M Discrete2DFunction table is impossible (15.7 TB). Options:
   - Category-level tables (~30 × 30 = 900 entries) — force interactions by PoS category
   - Hierarchical lookup: category table + token-specific adjustments as separate parameters
   - Sparse representation: only store non-zero interactions (but Discrete2DFunction is dense)

2. **Multiple per-particle parameters.** We can carry more than just token_id:
   - `token_id` — vocabulary lookup index
   - `category` — PoS category (for category-level force tables)
   - `sub_cat` — sub-categorization pattern
   - `position` — stream position

   How many dimensions does the force system need per particle?

3. **Expression symmetry.** CustomNonbondedForce requires symmetric energy expressions. PBM bond counts (token_A, token_B, count) — are these always symmetric? Is "the-cat" count == "cat-the" count? If not, we need CustomBondForce (per-bond, not symmetric) rather than CustomNonbondedForce.

---

## References

- OpenMM source: github.com/openmm/openmm
- Files examined:
  - `wrappers/python/openmm/app/element.py` — Element class (Python-only)
  - `wrappers/python/openmm/app/topology.py` — Topology class (Python-only)
  - `wrappers/python/openmm/app/forcefield.py` — ForceField (Python-only, 4721 lines)
  - `wrappers/python/openmm/app/internal/compiled.pyx` — Template matching (Cython)
  - `openmmapi/include/openmm/System.h` — C++ System (addParticle = mass only)
  - `openmmapi/include/openmm/CustomNonbondedForce.h` — Per-particle parameters
  - `openmmapi/include/openmm/CustomManyParticleForce.h` — Explicit integer types
  - `openmmapi/include/openmm/TabulatedFunction.h` — Discrete2DFunction
  - `platforms/common/src/CommonCalcCustomNonbondedForceKernel.cpp` — GPU kernel
  - `libraries/lepton/` — Expression parser (confirms params as function args)
- Prior report: docs/research/openmm-serialization-feasibility.md
- Prior evaluation: docs/research/openmm-evaluation.md
