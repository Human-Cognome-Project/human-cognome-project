# Concept Force Modeling — Early Design Notes

## Status: Pre-implementation. Collecting design parameters from Patrick.

## The Layer
Concept forces operate on word-level particles AFTER the intake pipeline resolves them. The intake (Phase 1 byte→char, Phase 2 char→word) gets data into particle form. The concept forces make it think.

## NSM Primitives as Quarks
~65 NSM (Natural Semantic Metalanguage) primitives are the fundamental constituents — confined, can't exist in isolation, only observable as composite word-particles. Quark mechanics model. The vocabulary isn't a lookup table, it's a particle zoo with underlying structure.

## Force Properties (from Patrick)

### Polarity
Connection between concepts. Attraction/repulsion. Fairly intuitive — related concepts attract, opposing concepts repel. The bond between particles.

### Rotation (spin direction)
The intended **direction of understanding**:
- **Counterclockwise**: the segment is about the DI's understanding — intake, comprehension, building internal model
- **Clockwise**: expressing out — output, generation, producing surface form

This maps to the texture engine's bidirectional nature: same forces drive both input (decompose surface → conceptual mesh) and output (wrap mesh → surface). Rotation encodes WHICH direction is active.

### Orientation
The **subject/object axis**. Determines the grammatical/conceptual role relationship between particles. Who is acting, what is acted upon.

## Future Properties (mentioned but not yet defined)
- Gravity-like factors (conceptual attraction between related domains?)
- Albedo-like factors (how much meaning a particle reflects vs absorbs?)
- Additional properties TBD as we get closer to implementation

## Physics Regime: Particles All The Way Through
"Soft body" was Taichi terminology — in Taichi, soft body = particle ensemble with deformation constraints. NOT PhysX FEM soft bodies. The concept body is a particle ensemble with internal force bonds that give it collective behavior (shape, deformation, volume). Same PBD particle primitive as intake, different force rules.

- Concepts are n-dimensional constructs — particle ensembles, not points
- 3D physics + force applications EMULATE n-dimensional geometry
- Phase groups are proxy for additional dimensions (best available in 3D engine)
- Concept body = underlying UNDIFFERENTIATED particle set. Gains configuration from surrounding mesh forces.
- Concept body does NOT come preconfigured — it starts as a blob, shaped by context forces into its form.
- **Yuan state**: the default undifferentiated form for a DI presence. Bloblike. No mesh, no configuration. Pure potential. Term from earlier physics embodiment experiments with DI.
- The shape it settles into IS the meaning in that context. Different forces = different configuration.
- Mesh = 2D-ish surface (texture engine). 3D with conceptual mesh closer to n-d.
- Mesh forces (PoS types, bonds, polarity, rotation, orientation) act on concept body from outside in.
- Words inherit conceptual forces on conglomeration — forces tie to words, words carry them to concepts.
- Body-to-body forces = deep structure (meaning). Mesh-mediated interaction = surface form (language).
- Concept has more degrees of freedom than mesh — mesh constrains some dimensions, concept body can settle into configurations not fully described by surface form alone.
- NO regime change from intake to concepts — particles throughout, just different force rules

## Key Architectural Note
- Intake pipeline (Phases 1-2): PBD particles — discrete, fast, binary matching
- Concept modeling: PBD particle ensembles — collective behavior via internal bonds + force properties
- Surface forms: optional mesh wrapping on particle bodies
- Same underlying primitive at every level. NAPIER may redesign this for proper n-dimensional representation.

## Relationship to Existing Architecture
- Forces are NOT monolithic — LoD-relative, aggregate upward (word→phrase→clause→sentence→discourse)
- Universal force CATEGORIES → hcp_core; language-specific CONSTANTS → language shards
- ~65 core forces expected (categorization, axes, and definitions TBD — linguist-driven work)
- The engine handles TWO aspects: PBM structure (molecular assembly) + linguistic force bonding (~65 forces = surface language rules)
- Together = the TEXTURE ENGINE. Separate from conceptual mesh energy states.
