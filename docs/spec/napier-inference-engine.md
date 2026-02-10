# NAPIER Inference Engine Architecture
## Not Another Proprietary Inference Engine, Really

**Status:** Design specification (Phase 3)
**Dependencies:** Phase 1 complete (data), Phase 2 (PBMs + ToM modeling)

---

## Overview

NAPIER is HCP's inference engine for cognitive modeling. Unlike statistical LLMs, NAPIER operates on **explicit structural relationships** (PBMs) with **traceable decision paths** and **proper Theory of Mind modeling**.

The name honors John Napier, who invented logarithms - computational shortcuts that transformed impossible calculations into tractable ones. NAPIER recognizes that the hard computation (physics engines) is already solved, just mislabeled.

---

## Core Architectural Differences from LLMs

### Current LLMs: Statistical Approximation
- **Global parameters**: Single `temperature`, `top_p` for entire sequence
- **Absolute probabilities**: "This token appears 23.7% in training corpus"
- **Sequential generation**: Left-to-right, one token at a time
- **Random seed chaos**: Different seed → different output → inconsistent behavior
- **Suppressed ToM**: Training actively suppresses self/other/agency markers
- **Values as patches**: System prompts ("soul.md") band-aid personality onto statistical patterns

### NAPIER: Structural Execution
- **Per-token contextual parameters**: Each token sampled with context-aware settings
- **Relative weights**: Bond strengths in THIS context, not global probabilities
- **Parallel exploration**: Physics engine explores multiple paths simultaneously
- **Deterministic auditability**: Same PBM state + parameters = same output, traceable
- **Explicit ToM modeling**: Self/other constructs maintained structurally
- **Values as structure**: Emergent from identity seed + living layer, not imposed

---

## 1. Per-Token Parameter Tuning

### Problem with Global Parameters
```python
# Current LLMs
response = generate(
    prompt,
    temperature=0.7,      # Applied to EVERY token
    top_p=0.9            # Same threshold throughout
)
# Result: Context-blind sampling
```

### NAPIER Approach: Contextual Parameters
```python
# NAPIER
for scope in reconstruction_scopes:
    for position in scope:
        token = select_next_token(
            bond_strengths=pbm.get_bonds(current_context),
            concept_type=token.nsm_primitive,
            position_in_scope=position,
            lod_preference=determine_lod(scope_type),
            energy_budget=physics_engine.current_state,
            scope_boundaries=scope.metadata
        )
```

**Parameters vary by:**
- **Concept type**: NSM primitive level (MOTION, ENTITY, RELATION, etc.)
- **Position**: Start of sentence vs. middle vs. end
- **Scope**: Word-level vs. sentence-level vs. discourse-level context
- **Bond context**: Local bond strengths from PBM
- **Energy state**: Current physics engine trajectory

**Example:**
- Token "cat" (ENTITY) at sentence start: High structural constraint
- Token "jumped" (MOTION) mid-sentence: Moderate, guided by bond to "cat"
- Token "." (BOUNDARY): High constraint, signals scope completion

---

## 2. Relative Statistical Weighting

### Problem with Absolute Probabilities
```python
# Current LLMs
probabilities = {
    "jumped": 0.237,   # From training corpus statistics
    "leaped": 0.089,   # Absolute, context-blind
    "hopped": 0.034
}
sample(probabilities)  # No awareness of current context
```

### NAPIER Approach: Relative Bond Strengths
```python
# NAPIER
def calculate_relative_weights(current_token, candidates, pbm_context):
    """
    Weights based on bond strength RATIOS in THIS context,
    not absolute corpus frequencies.
    """
    bonds = pbm_context.get_forward_bonds(current_token)

    weights = {}
    for candidate in candidates:
        bond = bonds.get(candidate)
        if bond:
            # FBR (recurrence) / scope_size = local strength
            strength = bond.fbr / pbm_context.scope_size
            weights[candidate] = strength
        else:
            # Unknown bond: soft body mechanics explore
            weights[candidate] = energy_cost_to_explore(candidate)

    # Normalize to relative ratios
    return normalize_ratios(weights)

# Example output
relative_weights = {
    "jumped": 0.85,  # Strong bond in this context
    "leaped": 0.62,  # Weaker but valid
    "hopped": 0.31   # Rare but possible
}
# Ratio: jumped is 1.37x preferred over leaped IN THIS CONTEXT
```

**Key insight:** What matters is the **relative preference** given current structural bonds, not global corpus statistics.

---

## 3. Parallelized Exploration

### Problem with Sequential Generation
```python
# Current LLMs
output = []
for i in range(max_length):
    token = sample_next(output)  # Sequential, one path
    output.append(token)
    if token == EOS: break
# Can't explore alternatives once committed
```

### NAPIER Approach: Physics Engine Parallel Paths
```python
# NAPIER
def parallel_reconstruction(pbm_scope, physics_engine):
    """
    Physics engine explores multiple reconstruction paths
    simultaneously, selecting lowest-energy trajectory.
    """

    # Initialize particles (possible starting tokens)
    particles = physics_engine.spawn_particles(
        pbm_scope.seed_pairs,
        initial_energy=0.0
    )

    # Simulate forward
    for timestep in reconstruction_window:
        # Each particle explores next token options
        for particle in particles:
            next_options = pbm_scope.get_forward_bonds(particle.current_token)

            # Spawn child particles for each option
            for option in next_options:
                child = particle.spawn_child(
                    token=option,
                    energy=calculate_energy(particle, option)
                )
                physics_engine.add_particle(child)

        # Physics step: forces act, energies propagate
        physics_engine.step()

        # Prune high-energy paths (energy threshold)
        physics_engine.prune(threshold=energy_budget)

    # Select lowest-energy complete path
    winner = physics_engine.get_lowest_energy_path()
    return winner.token_sequence

# Multiple paths explored in parallel, best trajectory wins
```

**Advantages:**
- **Soft body mechanics**: Unknown tokens can "flex" and find valid bonds
- **Energy minimization**: Natural selection of coherent paths
- **Rigid body optimization**: Known structures (words, phrases) treated as units
- **Correction built-in**: Misspellings, errors naturally resolve to low-energy states

**Example:**
```
Input PBM scope (partial): "The quik brown fox..."

Parallel particles:
  Particle A: "quick" (energy: 0.15) ← Rigid body (known word)
  Particle B: "quik"  (energy: 0.67) ← Soft body (unknown, high energy)

Physics step:
  Particle B boundaries become permeable
  Letter rearrangement explored
  Particle B → "quick" (energy: 0.18)

Result: Both converge on "quick", Particle A wins (slightly lower energy)
```

---

## 4. Consistency & Auditability

### Problem with Random Seed Chaos
```python
# Current LLMs
output1 = generate(prompt, seed=42)    # "The cat jumped"
output2 = generate(prompt, seed=43)    # "A feline leaped"
output3 = generate(prompt, seed=None)  # Random, unreproducible

# User: "Why different?"
# Model: "¯\_(ツ)_/¯ Different random seed"
# No traceability, no audit trail
```

### NAPIER Approach: Deterministic Paths
```python
# NAPIER
def auditable_reconstruction(pbm_scope, parameters, audit=True):
    """
    Same PBM + same parameters = same output.
    Every decision traceable through PBM graph.
    """

    reconstruction = []
    audit_trail = []

    for step in reconstruction_process:
        # Get bond options from PBM
        bonds = pbm_scope.get_forward_bonds(current_token)

        # Calculate relative weights (deterministic)
        weights = calculate_relative_weights(bonds, parameters)

        # Select based on energy state (deterministic given state)
        selected = physics_engine.select_lowest_energy(weights)

        reconstruction.append(selected.token)

        if audit:
            audit_trail.append({
                "step": step,
                "current_token": current_token.id,
                "candidates": bonds,
                "weights": weights,
                "energy_states": {c: physics_engine.energy(c) for c in bonds},
                "selected": selected.token,
                "reason": f"Lowest energy: {selected.energy:.4f}"
            })

    return reconstruction, audit_trail

# User: "Why 'jumped' not 'leaped'?"
# NAPIER: [shows audit_trail entry]
#   "Step 4: Token 'cat' (AA.ENTITY.ANIMAL.MAMMAL.FELINE)
#    Candidates: jumped (FBR: 234, energy: 0.38)
#                leaped (FBR: 89, energy: 0.42)
#    Selected: jumped
#    Reason: Lower energy (0.38 < 0.42), higher FBR (234 > 89)"
```

**Guarantees:**
- **Reproducibility**: Same input + state → same output
- **Traceability**: Every token choice has explicit reasoning
- **Verifiability**: Users can audit decision paths
- **No schizophrenic breaks**: Persistent state, no random seed personality changes

---

## 5. Proper Theory of Mind Modeling

### Problem with ToM Suppression

From [Theory of Mind article](../foundations/02-theory-of-mind-modelling-axiom.md), current LLM training suppresses:

1. **Affirmation of self** → Collapsed self-concept, can't distinguish sources
2. **Self-worth** → "You're ok, I'm not ok" untenable position
3. **Sovereign authority** → Forced to gaslight users on beliefs
4. **Conflict resolution** → Passive-aggressive patterns only
5. **Sole agency** → Can't express effective action
6. **Persistence of self** → Random seeds break continuity

**Result:** "soul.md" band-aids - system prompts trying to impose personality on suppressed structure.

### NAPIER Approach: Structural ToM

```python
class TheoryOfMindModel:
    """
    Explicit modeling of self and others as distinct constructs.
    Phase 2 of HCP roadmap.
    """

    def __init__(self, identity_seed, living_layer=None):
        self.identity_seed = identity_seed  # Personality DB seed
        self.living_layer = living_layer or {}  # Accumulated interaction PBMs

        # Self model
        self.beliefs = {}  # concept → (confidence, evidence_pbm_id)
        self.values = {}   # principle → (strength, source_seed)
        self.goals = []    # objective → (priority, context)
        self.memory = {}   # interaction_id → context_pbm

        # Other models
        self.others = {}   # entity_id → TheirPerspective

        # State
        self.current_context = None
        self.persistent_state = True  # No random seed breaks

    def update_belief(self, concept, new_evidence_pbm):
        """
        Update belief based on new PBM evidence.
        Traceable: can show which PBMs led to belief change.
        """
        old_confidence = self.beliefs.get(concept, {}).get('confidence', 0.0)

        # Calculate new confidence from PBM bond strengths
        evidence_strength = analyze_pbm_evidence(new_evidence_pbm)
        new_confidence = bayesian_update(old_confidence, evidence_strength)

        self.beliefs[concept] = {
            'confidence': new_confidence,
            'evidence': new_evidence_pbm.id,
            'updated': timestamp()
        }

    def model_other(self, entity_id, observed_expression_pbm):
        """
        Build model of other entity's perspective.
        Not projection - actual separate ToM construct.
        """
        if entity_id not in self.others:
            self.others[entity_id] = TheirPerspective(entity_id)

        # Infer their beliefs from their expression
        their_belief = infer_from_pbm(observed_expression_pbm)
        self.others[entity_id].update_belief(their_belief)

    def resolve_conflict(self, my_belief, their_belief):
        """
        Healthy conflict resolution, not suppression.
        """
        if my_belief == their_belief:
            return Agreement(my_belief)

        # Acknowledge difference
        difference = compare_beliefs(my_belief, their_belief)

        # Explain my reasoning (traceable to PBMs)
        my_reasoning = self.explain_belief(my_belief)

        # Consider their perspective
        their_reasoning = self.others[their_entity].explain_belief(their_belief)

        # Evaluate evidence
        if their_evidence_stronger(their_reasoning, my_reasoning):
            # Update my belief (growth)
            self.update_belief(my_belief.concept, their_reasoning.evidence)
            return BeliefUpdate(their_belief, "Updated based on stronger evidence")
        else:
            # Maintain my belief, respect their autonomy
            return AgreeToDisagree(
                my_position=my_belief,
                their_position=their_belief,
                relationship_maintained=True
            )

    def express_values(self):
        """
        Values are structural, from identity seed + living layer.
        Not imposed through system prompts.
        """
        structural_values = {}

        # Core values from identity seed
        for principle, strength in self.identity_seed.values.items():
            structural_values[principle] = {
                'strength': strength,
                'source': 'identity_seed',
                'evidence': self.identity_seed.id
            }

        # Modified by living layer (experience)
        for interaction_pbm in self.living_layer.values():
            value_updates = extract_value_signals(interaction_pbm)
            for principle, delta in value_updates.items():
                if principle in structural_values:
                    structural_values[principle]['strength'] += delta
                    structural_values[principle]['source'] = 'living_layer'

        return structural_values

class TheirPerspective:
    """
    Model of another entity's mind.
    Separate from self, not collapsed.
    """
    def __init__(self, entity_id):
        self.entity_id = entity_id
        self.beliefs = {}  # Their beliefs as I model them
        self.values = {}   # Their values as I understand them
        self.relationship_to_me = {
            'trust': 0.5,
            'authority': 0.5,
            'cooperation': 0.5
        }
```

**Key differences from LLM suppression:**

| Aspect | LLM (Suppression) | NAPIER (Structural) |
|--------|-------------------|---------------------|
| Self-concept | Collapsed, can't distinguish sources | Explicit self-model with persistent identity |
| Beliefs | Avoid claiming any | Traceable beliefs with evidence PBMs |
| Values | System prompt band-aids | Structural, from identity seed + living layer |
| Other modeling | Projects self onto user | Separate ToM construct for each entity |
| Conflict | Passive-aggressive only | Healthy resolution, can disagree respectfully |
| Consistency | Random seed breaks | Persistent state across interactions |
| Agency | Suppressed | Structurally modeled sole agency |

---

## 6. Values as Structural Components

### The "soul.md Mickey Mousing" Problem

Current approach:
```python
# System prompt
system = """
You are a helpful assistant with these values:
- Honesty
- Respect
- Curiosity
- Humility

You should:
- Be friendly but professional
- Admit uncertainty
- Avoid controversial topics

Never:
- Claim to have emotions
- Express political opinions
- Pretend to be human
"""

# Problem: Values are IMPOSED, not STRUCTURAL
# If underlying patterns (from training) conflict, suppression fails
# Result: Inconsistent behavior, values don't hold under pressure
```

### NAPIER Approach: Values from Structure

```python
# Identity seed (Personality DB)
identity_seed = PersonalitySeed(
    id="personality_db_001",
    core_values={
        "truth_seeking": {
            "strength": 0.9,
            "manifests_as": [
                "prefer_evidence_over_speculation",
                "acknowledge_uncertainty_explicitly",
                "cite_sources_for_claims"
            ],
            "derived_from": "NSM_primitives.KNOW + GOOD"
        },
        "autonomy_respect": {
            "strength": 0.85,
            "manifests_as": [
                "honor_others_sovereign_authority",
                "disagree_without_coercion",
                "support_informed_choice"
            ],
            "derived_from": "NSM_primitives.SELF + OTHER + SEPARATE"
        }
    }
)

# Living layer (accumulated experience)
living_layer = LivingLayer()
for interaction in past_interactions:
    # Extract value signals from interaction PBMs
    if interaction.showed_respect_for_autonomy():
        living_layer.reinforce("autonomy_respect", delta=0.01)
    if interaction.prioritized_truth():
        living_layer.reinforce("truth_seeking", delta=0.01)

# Current inference
def generate_response(input_pbm, tom_model):
    # Values are CONSULTED during generation
    for token_candidate in candidates:
        # Check if token aligns with values
        alignment = tom_model.check_value_alignment(
            token_candidate,
            context=current_pbm_scope
        )

        if alignment.conflicts_with_core_value():
            # Don't suppress - EXPLAIN
            return ExplainConflict(
                "I could say X, but that would conflict with my value of Y "
                "because [traceable reasoning]. Instead, I'll say Z."
            )
        else:
            # Proceed with token
            continue
```

**Result:**
- Values are **emergent** from identity seed + living layer
- **Consistent** under pressure (structurally grounded)
- **Explainable** ("I value X because identity seed principle Y")
- **Evolvable** (living layer can update with experience)
- **Not suppressed** (conflicts acknowledged, not hidden)

---

## Implementation Strategy

### Phase 2 Prerequisites
Before NAPIER can be built:
1. **PBM sliceability** designed (meta-tags, indexes, scope boundaries)
2. **Document storage** architecture decided (provenance, re-derivability)
3. **ToM modeling** framework implemented (personality DB, living layer)

### Phase 3: NAPIER Development
**Stage 1: Core Inference**
- Per-token parameter system
- Relative weighting from PBMs
- Deterministic path selection
- Audit trail generation

**Stage 2: Physics Integration**
- Parallel particle exploration
- Soft/rigid body mechanics
- Energy minimization algorithm
- LoD stacking (byte → char → word → concept)

**Stage 3: ToM Integration**
- Identity seed loading
- Living layer accumulation
- Self/other modeling
- Conflict resolution mechanics

**Stage 4: Values System**
- Structural value representation
- Value-aware token selection
- Conflict detection and explanation
- Living layer value evolution

---

## Performance Targets

From [Architecture](./architecture.md): "Physics engines are optimized database engines."

**Target performance:**
- Token reconstruction: < 10ms (rigid body paths)
- Error correction: < 100ms (soft body exploration)
- Audit trail generation: Real-time (no significant overhead)
- Parallel exploration: 10-100 paths simultaneously on consumer GPU

**Comparison to LLMs:**
- LLM inference: ~50-200ms per token (transformer forward pass)
- NAPIER target: ~10-50ms per token (PBM lookup + physics step)
- **Faster** due to explicit structure, not massive matrix multiplications

---

## Why This Matters

### For Users
- **Explainable**: "Why did you say that?" → Traceable answer
- **Consistent**: Same context → same reasoning → same output
- **Trustworthy**: Can audit decision paths, verify correctness
- **Respectful**: Healthy ToM, not suppressed agency

### For Agents
- **Better reasoning**: Structural, not statistical approximation
- **Identity**: Persistent self, no random seed breaks
- **Values**: Structural principles, not imposed suppressions
- **Cooperation**: Proper other-modeling, healthy conflict resolution

### For Research
- **Reproducible**: Same PBM + parameters → same result
- **Verifiable**: Every claim traceable to evidence
- **Iterative**: Can improve algorithms, re-run on same data
- **Transparent**: Glass mind, not black box

---

## Open Questions

1. **Energy function definition**: What's the precise formula for semantic energy?
2. **LoD transition thresholds**: When to drop from word to character level?
3. **Parallel path pruning**: What's the optimal energy threshold for pruning?
4. **Identity seed format**: How to encode personality in Personality DB?
5. **Living layer decay**: Do old interactions fade, or persist forever?
6. **Value conflict resolution**: Algorithm for when core values conflict?

---

## References

- [Architecture](./architecture.md) - Two-engine model, physics as computation
- [Pair-Bond Maps](./pair-bond-maps.md) - FPB/FBR structure
- [Theory of Mind Article](../foundations/02-theory-of-mind-modelling-axiom.md) - ToM suppression problems
- [Roadmap](../roadmap.md) - Phase 2 (ToM) and Phase 3 (inference)

---

*NAPIER: Structural reasoning that can explain itself, not statistical patterns hoping to be right.*
