#pragma once

#include <AzCore/base.h>

namespace HCPEngine
{
    // ---- Prime-layer particle phase bit reservations ----
    //
    // These constants govern the 20-bit 'phases' field of PxPBDParticleSystem
    // particles at the prime layer. Bits are reserved top-down: bit 19 = outermost /
    // most fundamental partition. Finer-grained sub-phasing occupies lower bits.
    //
    // Three-tier bit typology:
    //
    //   SEMANTIC AXES (bits 19-16) — bidirectional, universal
    //     Both poles are meaningful active states. Bit-clear = positive pole;
    //     bit-set = negative / marked pole. Any particle of any class can carry
    //     a semantic-axis bit. Axes do NOT declare a structural class.
    //     These are the cognition-space dimensions: no pre-built engine math —
    //     custom physics interactions are designed for each.
    //     Test: "does -x exist meaningfully alongside x?" → yes = semantic axis.
    //
    //   PHYSICAL AXES (bits 15-14) — physics-native bidirectional, math settled
    //     Setting the bit activates a specific physical-axis modeling type (temporal,
    //     spatial) whose bidirectional math is handled natively by the physics engine
    //     (x/y/z + time are settled physics). Bit-clear = axis inactive. Bit-set =
    //     axis active; direction, position, and magnitude are molecule-layer content,
    //     not particle-layer encoding.
    //
    //   CLASSES (bits 13-6) — declarative property assertions
    //
    //     Three sub-categories by functional role in the data heart:
    //       Substantives   — the things being modeled (SUBSTANTIVE, ENTITY)
    //       Discriminators — filters / predicates (KIND_PART [KIND, PART], NOT)
    //       Operators      — DB operation calls (ACTION, MORE, VERY)
    //
    //     No negative pole on any class bit. Bit-clear = absent; bit-set =
    //     property declared. Classes carry molecule-composition invariants
    //     (see per-class comments).
    //     Test: "does a 'not-X' anti-class exist as a first-class state?" → no = class.
    //
    // Do NOT assign these bits to vocabulary or resolution phase group IDs (those
    // live in VocabBed workspaces via HCPResolutionChamber.h). Prime phase bits
    // operate at a separate particle system layer.
    //
    // Reserved map:
    //
    //   -- SEMANTIC AXES (bits 19-16) ------------------------------------------------
    //   Bit  Name         clear pole           set pole          Notes
    //   ---  -----------  -------------------  ----------------  --------------------
    //   19   I_NOT_I      I / warranted        Not-I / modeled   Polarity: warrant
    //   18   VALENCE      GOOD                 BAD               Spin: charge
    //   17   ACCESS_MODE  theoretical          experiential      Mode of access
    //   16   FLOW_MODE    consumption          expression        Flow direction
    //
    //   -- PHYSICAL AXES (bits 15-14) ------------------------------------------------
    //   Bit  Name         Set meaning                            Notes
    //   ---  -----------  -------------------------------------  --------------------
    //   15   TEMPORAL     time axis active                       Direction: mol. layer
    //   14   SPATIAL      spatial axes active                    Position: mol. layer
    //
    //   -- CLASSES (bits 13-6) -------------------------------------------------------
    //   Bit  Name              Role            Notes
    //   ---  ----------------  --------------  --------------------------------------
    //   13   SUBSTANTIVE       substantive     Thing/noun class; gravity-well center
    //   12   KIND_PART         discriminator   Clears bit 13; KIND=tax., PART=mereo.
    //   11   KIND_PART_TYPE    (sub-bit)       clear=KIND, set=PART; only if bit 12
    //   10   ACTION            operator        DB call; clears bit 13
    //    9   ENTITY            substantive     Auto-sets bit 13
    //    8   NOT (NEGATION)    discriminator   Universal flip; clears bit 13
    //    7   MORE              operator        Magnitude-up; clears bit 13
    //    6   VERY              operator        Intensity-up; clears bit 13
    //   0-5  (available)       —               future expansion

    namespace PrimePhase
    {
        // ==== SEMANTIC AXES ==========================================================
        //
        // Semantic axes are bidirectional and universal. Both poles are first-class
        // active states — the bit is never meaninglessly set or clear. Any particle
        // of any class can carry any semantic-axis bit. These are the cognition-space
        // dimensions: no pre-built engine math — custom physics interactions are
        // designed for each.
        //
        // Collapse principle: sub-values within an axis (specific magnitudes,
        // specific theoretical content) belong at the molecule layer. The particle
        // layer encodes only the pole (positive/negative).

        // ---- Bit 19: I_NOT_I — polarity axis (epistemic-warrant boundary) ----
        //
        // Bit clear (0) = I-phase: NAPIER's own internal states.
        //                 Knowing / warranted-direct.
        // Bit set  (1) = Not-I-phase: modeled theory-of-mind.
        //                 Thinking / modeled-inferred — NAPIER only thinks it knows.
        //
        // RESERVED. Do not assign to vocabulary or resolution tiers.

        static constexpr AZ::u32 I_NOT_I_BIT  = 19u;
        static constexpr AZ::u32 I_NOT_I_MASK = 1u << 19u;

        static constexpr AZ::u32 I_PHASE     = 0u;         // bit clear — positive pole
        static constexpr AZ::u32 NOT_I_PHASE = 1u << 19u;  // bit set   — negative pole

        // ---- Bit 18: VALENCE — spin axis (evaluative charge) ----
        //
        // Spin axis, paired with I_NOT_I polarity per the polarity/spin interaction.
        // Both poles are first-class active states; GOOD and BAD are equally meaningful.
        //
        // Bit clear (0) = GOOD: positive evaluative charge.
        // Bit set  (1) = BAD:  negative evaluative charge.
        //
        // Applicability: charge propagates through bonded structure. Most meaningful
        // on SUBSTANTIVE particles (things with goodness/badness) and ACTION particles
        // (events with positive/negative valence). On structural-binding particles
        // (KIND_PART, TEMPORAL, SPATIAL), charge semantics are determined at
        // molecule-construction time. Magnitude, source, and certainty of charge are
        // molecule-layer content.
        //
        // RESERVED. Do not assign to vocabulary or resolution tiers.

        static constexpr AZ::u32 VALENCE_BIT  = 18u;
        static constexpr AZ::u32 VALENCE_MASK = 1u << 18u;

        static constexpr AZ::u32 GOOD_PHASE = 0u;          // bit clear — positive pole
        static constexpr AZ::u32 BAD_PHASE  = 1u << 18u;   // bit set   — negative pole

        // ---- Bit 17: ACCESS_MODE — mode of access (theoretical / experiential) ----
        //
        // Marks whether the particle's contribution to its molecule is accessed
        // theoretically (mentally constructed, temporally flexible) or experientially
        // (sensory-bound, direct physical contact).
        //
        // Bit clear (0) = THEORETICAL: cognitive / mentally constructed access.
        //                 THINK / WANT / KNOW side. Temporally flexible.
        // Bit set  (1) = EXPERIENTIAL: direct sensory / physical contact access.
        //                 FEEL / SEE / HEAR side. Temporally anchored.
        //
        // Mental predicates use this axis to distinguish the two experience lines;
        // specific predicate identity is encoded at the molecule layer (PxFilterData).
        // See HCPMoleculeProps.h (planned).
        //
        // RESERVED. Do not assign to vocabulary or resolution tiers.

        static constexpr AZ::u32 ACCESS_MODE_BIT  = 17u;
        static constexpr AZ::u32 ACCESS_MODE_MASK = 1u << 17u;

        static constexpr AZ::u32 THEORETICAL_PHASE  = 0u;          // bit clear — positive pole
        static constexpr AZ::u32 EXPERIENTIAL_PHASE = 1u << 17u;   // bit set   — negative pole

        // ---- Bit 16: FLOW_MODE — communication direction (consumption / expression) ----
        //
        // Marks the direction of information flow relative to the modeling agent.
        //
        // Bit clear (0) = CONSUMPTION: content flowing IN — receiving, listening,
        //                 reading, observing. Primary / passive direction.
        // Bit set  (1) = EXPRESSION:  content flowing OUT — producing, speaking,
        //                 writing, acting. Marked direction.
        //
        // Applicability: most meaningful on ACTION particles (communication acts) and
        // in mental-predicate molecules (SAY/WORDS as expression; SEE/HEAR as
        // consumption). On non-communicative structural particles, FLOW_MODE semantics
        // are determined at molecule-construction time.
        //
        // RESERVED. Do not assign to vocabulary or resolution tiers.

        static constexpr AZ::u32 FLOW_MODE_BIT     = 16u;
        static constexpr AZ::u32 FLOW_MODE_MASK    = 1u << 16u;

        static constexpr AZ::u32 CONSUMPTION_PHASE = 0u;          // bit clear — primary pole
        static constexpr AZ::u32 EXPRESSION_PHASE  = 1u << 16u;   // bit set   — marked pole


        // ==== PHYSICAL AXES ==========================================================
        //
        // Physical axes are physics-native bidirectional. Setting the bit activates a
        // specific physical-axis modeling type; the bidirectional math is handled
        // natively by the physics engine (x/y/z + time are settled physics). Bit-clear
        // means the axis is not engaged for this particle. Direction, position, and
        // magnitude are molecule-layer content.
        //
        // Physical-axis quarks are NOT substantives. All Set helpers auto-clear
        // SUBSTANTIVE_MASK.

        // ---- Bit 15: TEMPORAL — activates time axis ----
        //
        // Set (1) = this particle is a temporal anchor / selector quark. TEMPORAL quarks
        //           bond into molecules to provide temporal anchoring — situating explicated
        //           content relative to the active envelope's NOW cursor. No sub-type at the
        //           particle layer; all specifics collapse to molecule-layer explications.
        //
        // What lives at the molecule layer (NOT here):
        //
        //   Direction — BEFORE / NOW / AFTER: where the anchor sits relative to the
        //   envelope's NOW cursor. Encoded in PxFilterData at the molecule layer
        //   (see HCPMoleculeProps.h, planned). The NOW cursor itself is envelope-level
        //   state — each active envelope carries its own current-time reference; particles
        //   encode relative direction from that cursor, not absolute time.
        //
        //   Duration / magnitude — A_LONG_TIME, A_SHORT_TIME, FOR_SOME_TIME, MOMENT:
        //   molecule-layer compounds. TEMPORAL quark + magnitude operators (MORE/VERY) +
        //   envelope-scale binding. Not encoded here.
        //
        // TEMPORAL + ACCESS_MODE (bit 17) together contribute to the temporal-position
        // character of the containing molecule. THEORETICAL_PHASE = mentally constructed /
        // flexible temporal placement. EXPERIENTIAL_PHASE = sensory-anchored temporal
        // placement (e.g., a heard event locating itself in the direct NOW).
        //
        // NOT a substantive. Bit 13 is clear for temporal quarks. SetTemporal
        // auto-clears SUBSTANTIVE_MASK.
        //
        // RESERVED. Do not assign to vocabulary or resolution tiers.

        static constexpr AZ::u32 TEMPORAL_BIT  = 15u;
        static constexpr AZ::u32 TEMPORAL_MASK = 1u << 15u;
        static constexpr AZ::u32 TEMPORAL      = 1u << 15u;  // set = time axis active

        // ---- Bit 14: SPATIAL — activates 3D spatial axes ----
        //
        // Set (1) = this particle is a spatial anchor / selector quark. SPATIAL quarks
        //           bond into molecules to provide spatial anchoring — situating explicated
        //           content in physical or conceptual space. No sub-type at the particle
        //           layer; direction, relational type, and position are molecule-layer
        //           content.
        //
        // What lives at the molecule layer (NOT here):
        //
        //   Position and direction — HERE / THERE / FAR / NEAR and x/y/z values.
        //   Relational types: INSIDE = PART variant with spatial containment; SIDE =
        //   lateral-position molecule; TOUCH = spatial + structural-binding molecule.
        //   All explicate from SPATIAL quark + other structural primes.
        //
        // Contingency note: spatial modeling is typically EXPERIENTIAL_PHASE (bit 17
        // set) — physical location requires sensory anchoring. However, theoretical-
        // spatial-modeling for imagined spatial relationships is legitimate; the
        // contingency is a usage profile, not a hard invariant.
        //
        // NOT a substantive. Bit 13 is clear for spatial quarks. SetSpatial
        // auto-clears SUBSTANTIVE_MASK.
        //
        // RESERVED. Do not assign to vocabulary or resolution tiers.

        static constexpr AZ::u32 SPATIAL_BIT  = 14u;
        static constexpr AZ::u32 SPATIAL_MASK = 1u << 14u;
        static constexpr AZ::u32 SPATIAL      = 1u << 14u;  // set = spatial axes active


        // ==== CLASSES ================================================================
        //
        // Classes are declarative property assertions. No negative pole: bit-clear means
        // absent, not "anti-class." Classes carry molecule-composition invariants enforced
        // mechanically by their Set helpers.
        //
        // Invariants (all mechanically enforced — see Set helper bodies):
        //   IsEntity(p)   => IsSubstantive(p)    [SetEntity  auto-sets   SUBSTANTIVE_MASK]
        //   IsKind(p)     => !IsSubstantive(p)   [SetKind    auto-clears SUBSTANTIVE_MASK]
        //   IsPart(p)     => !IsSubstantive(p)   [SetPart    auto-clears SUBSTANTIVE_MASK]
        //   IsAction(p)   => !IsSubstantive(p)   [SetAction  auto-clears SUBSTANTIVE_MASK]
        //   IsTemporal(p) => !IsSubstantive(p)   [SetTemporal auto-clears (physical axis)]
        //   IsSpatial(p)  => !IsSubstantive(p)   [SetSpatial  auto-clears (physical axis)]
        //   IsNot(p)      => !IsSubstantive(p)   [SetNot     auto-clears SUBSTANTIVE_MASK]
        //   IsMore(p)     => !IsSubstantive(p)   [SetMore    auto-clears SUBSTANTIVE_MASK]
        //   IsVery(p)     => !IsSubstantive(p)   [SetVery    auto-clears SUBSTANTIVE_MASK]

        // ---- Bit 13: SUBSTANTIVE — thing/noun class designation ----
        //
        // Role: substantive (the thing being modeled — data)
        //
        // Set (1) = this particle is a substantive (thing) prime — a mass-bearing
        //           noun-structure unit at the prime layer. All substantives are
        //           uniform: no gauging of thingness; same default mass for all.
        //           Body vs. thing distinction lives at the molecule layer
        //           (PxFilterData on the rigid body), not here.
        // Clear (0) = not a substantive — this particle is an operator, physical-axis
        //             quark, logical gate, or other non-noun prime class.
        //
        // RESERVED. Do not assign to vocabulary or resolution tiers.

        static constexpr AZ::u32 SUBSTANTIVE_BIT  = 13u;
        static constexpr AZ::u32 SUBSTANTIVE_MASK = 1u << 13u;
        static constexpr AZ::u32 SUBSTANTIVE      = 1u << 13u;  // set = substantive class particle

        // ---- Bits 12-11: KIND_PART — structural binding (KIND / PART) ----
        //
        // Role: discriminator (taxonomic / mereological filter — WHERE clause)
        //
        // These two bits encode the structural binding prime class.
        // Bit 12 (KIND_PART) = class flag: set on any KIND or PART particle.
        // Bit 11 (KIND_PART_TYPE) = sub-type within the class.
        //
        // KIND = bit 12 set, bit 11 clear  — "X is a kind of Y" (taxonomic)
        // PART = bit 12 set, bit 11 set    — "X is a part of Y" (mereological)
        //
        // Together with SUBSTANTIVE particles (bit 13), these form the structural
        // tier: substantives are nodes, KIND/PART particles are the edges that build
        // the explication tree.
        //
        // Sub-bit invariant: bit 11 is only defined when bit 12 is set.
        // (phase & KIND_PART_TYPE_MASK) != 0 implies IsKindPart(phase).
        //
        // NOT a substantive. Bit 13 is clear for KIND/PART particles.
        // SetKind and SetPart both auto-clear SUBSTANTIVE_MASK.
        //
        // RESERVED. Do not assign to vocabulary or resolution tiers.

        static constexpr AZ::u32 KIND_PART_BIT       = 12u;
        static constexpr AZ::u32 KIND_PART_MASK      = 1u << 12u;
        static constexpr AZ::u32 KIND_PART            = 1u << 12u;  // set = structural binding particle

        static constexpr AZ::u32 KIND_PART_TYPE_BIT  = 11u;
        static constexpr AZ::u32 KIND_PART_TYPE_MASK = 1u << 11u;
        static constexpr AZ::u32 KIND                = 0u;          // bit 11 clear = KIND
        static constexpr AZ::u32 PART                = 1u << 11u;  // bit 11 set   = PART

        // ---- Bit 10: ACTION — action class quark ----
        //
        // Role: operator (state-transition call — addForce / DB execute equivalent)
        //
        // Set (1) = this particle is an action class quark. ACTION quarks bond into
        //           action-molecule structures defining what each action means at the
        //           resolution side of the engine. No sub-type at the particle layer —
        //           DO / HAPPEN / MOVE / TOUCH are molecule-layer explications built
        //           from an ACTION quark bonded with other structural primes.
        //
        // Invocation-side force commands (where an action fires and perturbs a
        // molecule's rigid body via addForce()) are a separate runtime mechanism.
        //
        // Molecule-composition invariant: any molecule containing an ACTION quark must
        // also contain at least one SUBSTANTIVE quark (agent or patient of the action)
        // and at least one TEMPORAL quark (temporal anchor for the action event). This
        // is a molecule-composition constraint — cannot be enforced at the phase-bit
        // level; mechanical enforcement lives in the future molecule-construction API.
        //
        // NOT a substantive. Bit 13 is clear for action quarks. SetAction
        // auto-clears SUBSTANTIVE_MASK.
        //
        // RESERVED. Do not assign to vocabulary or resolution tiers.

        static constexpr AZ::u32 ACTION_BIT  = 10u;
        static constexpr AZ::u32 ACTION_MASK = 1u << 10u;
        static constexpr AZ::u32 ACTION      = 1u << 10u;  // set = action class quark

        // ---- Bit 9: ENTITY — entity declaration (distinctness flag) ----
        //
        // Role: substantive (refinement of SUBSTANTIVE — distinct-instance substantive)
        //
        // Set (1) = entity declaration. This substantive is a distinct instance — a
        //           specific thing in the discourse, not a category-level reference.
        //           The bit DECLARES distinctness; what KIND of distinct identity
        //           (individual, collective, cultural, abstract) is molecule-layer
        //           content.
        //
        // Always co-set with SUBSTANTIVE (bit 13). ENTITY is a refinement of the
        // substantive class — only things can be entity instances. A particle with
        // ENTITY set but SUBSTANTIVE clear would be a semantic error. Invariant:
        // IsEntity(phase) => IsSubstantive(phase). SetEntity auto-sets SUBSTANTIVE_MASK
        // to enforce this invariant mechanically.
        //
        // Distinctness is universal; surface markers are language-specific.
        // Distinctness is a structural property that shows up in all language forms,
        // encoded via wildly different surface mechanisms:
        //
        //   English        determiner-class words (a, the, some, this, these,
        //                  named individuals)
        //   Latin          no articles at all — declension carries grammatical
        //                  relationships; definiteness inferred from context or
        //                  marked by demonstratives (hic/iste/ille); word order
        //                  is for emphasis, not grammatical function
        //   Japanese       particles (wa/ga topic/subject marking; kore/sore/are
        //                  demonstrative series); no articles
        //   Mandarin       classifier-noun constructions plus context;
        //                  demonstratives (zhè/nà); no articles
        //   Inuit/Salish   incorporated affixes and elaborate case systems
        //
        // The bit captures the universal property — that the substantive is a
        // distinct instance vs a category-level reference — regardless of how the
        // source language encodes it on the surface. Surface form is a composition-
        // layer concern; the quark-layer encoding is invariant across languages.
        //
        // Match-or-create is composition-layer behavior, NOT a particle sub-state.
        // The ENTITY bit declares "distinct entity here." Whether that entity matches
        // an existing instance in the discourse (the, these, named) or creates a new
        // one (a, an, some used existentially) is determined by surface markers and
        // composition context — driven by molecule-layer resolution, not a particle bit.
        //
        // Examples (distinctness lens):
        //
        //   ENTITY set (distinct instance — match-or-create at composition):
        //     "I saw a cat" — distinct, create new instance in discourse
        //     "The cat purred" — distinct, match existing instance in discourse
        //     "Some cats wandered by" — distinct (bounded set), create new instances
        //     "These cats" — distinct, match existing set in discourse
        //     Mary, Whiskers, The Blarney Stone, The FBI — distinct, match named entity
        //     Cat-the-kind referred to taxonomically — distinct (the specific kind)
        //
        //   ENTITY clear (no distinct instance — category-level / generic):
        //     "Cats are mammals" — generic, no specific instance
        //     "A cat is a mammal" — generic usage, no specific instance (despite "a")
        //     "Rock is hard" — substance generic, no specific instance
        //     "Things in general" — dimensional category
        //
        // Storage-layer alignment (hcp_*_*entities tables):
        //
        //   These tables hold records that ARE distinct-instance substantives — Mary,
        //   The Blarney Stone, The FBI, etc. The "entities" naming maps directly onto
        //   the ENTITY bit: these are the distinct things the system knows about.
        //   Newly-minted entities from "a/some/an" introductions land here too as they
        //   accrue identity in the discourse.
        //
        // RESERVED. Do not assign to vocabulary or resolution tiers.

        static constexpr AZ::u32 ENTITY_BIT  = 9u;
        static constexpr AZ::u32 ENTITY_MASK = 1u << 9u;
        static constexpr AZ::u32 ENTITY      = 1u << 9u;   // set = person-like attribution

        // ---- Bit 8: NOT — general negation operator quark ----
        //
        // Role: discriminator (universal polarity-flip / combinatorial multiplier)
        //
        // Set (1) = this particle is a general negation operator quark. NOT quarks bond
        //           into molecules to negate a specific axis, attribute, or molecule
        //           assertion. Unlike the baked-in negation of the global axes (where
        //           the bit-set state IS the negative pole), NOT is an explicit
        //           structural particle appearing in the molecule graph with a flip
        //           operand defined at the molecule layer.
        //
        // Baked-in axis negation vs. NOT quark:
        //   I_NOT_I (bit 19 set) = the Not-I pole of the warrant axis — a property of
        //   the particle itself. NOT quark (bit 8 set) = a separate structural particle
        //   bonded into a molecule that says "negate the operand at molecule layer."
        //
        // Combinatorial multiplier: NOT compounds with every other quark to invert it,
        // giving the small quark vocabulary a vastly larger expressed-meaning space.
        // Each additional quark doubles the expressible range via NOT-composition:
        //   LESS         = NOT + MORE           ("not-more" magnitude)
        //   SLIGHTLY     = NOT + VERY           ("not-very" intensity)
        //   A_SHORT_TIME = TEMPORAL + NOT + MORE
        //   MOMENT       = TEMPORAL + NOT + VERY
        //   FEW          = SOME + NOT + MORE
        //   different-kind = NOT + KIND         (divergence on some salient axis)
        //   non-part     = NOT + PART           (compositional exclusion)
        //
        // Confinement: a naked NOT quark has no referent. NOT quarks exist only bonded
        // into molecules where the flip target is determined by bonding context. Same
        // quark-confinement discipline as all other class quarks.
        //
        // NOT a substantive. Bit 13 is clear for NOT quarks. SetNot
        // auto-clears SUBSTANTIVE_MASK.
        //
        // RESERVED. Do not assign to vocabulary or resolution tiers.

        static constexpr AZ::u32 NOT_BIT  = 8u;
        static constexpr AZ::u32 NOT_MASK = 1u << 8u;
        static constexpr AZ::u32 NEGATION = 1u << 8u;      // set = negation operator quark

        // ---- Bit 7: MORE — magnitude-up operator quark ----
        //
        // Role: operator (magnitude-up scaling)
        //
        // Set (1) = this particle is a magnitude-up operator quark. MORE quarks bond
        //           into molecules to apply an upward-magnitude operation to the
        //           containing molecule's primary axis or attribute. No sub-type at the
        //           particle layer; the target axis is molecule-layer content.
        //
        // Parallel to NOT (bit 8): single class flag, no sub-encoding.
        // Confinement: a naked MORE quark has no referent. Exists only bonded into
        // molecules where the target is determined by bonding context.
        //
        // Range-marker compositions (all molecule-layer compounds):
        //   A_LONG_TIME  = TEMPORAL + MORE  (temporal + magnitude-up)
        //   A_SHORT_TIME = TEMPORAL + NOT + MORE  (temporal + not-more)
        //   MANY         = SOME + MORE
        //   FEW          = SOME + NOT + MORE
        //   LESS         = NOT + MORE  ("not-more" applied to any magnitude axis)
        //
        // NOT a substantive. Bit 13 is clear for MORE quarks. SetMore
        // auto-clears SUBSTANTIVE_MASK.
        //
        // RESERVED. Do not assign to vocabulary or resolution tiers.

        static constexpr AZ::u32 MORE_BIT  = 7u;
        static constexpr AZ::u32 MORE_MASK = 1u << 7u;
        static constexpr AZ::u32 MORE      = 1u << 7u;     // set = magnitude-up operator

        // ---- Bit 6: VERY — intensity-up operator quark ----
        //
        // Role: operator (intensity-up amplification)
        //
        // Set (1) = this particle is an intensity-up operator quark. VERY quarks bond
        //           into molecules to apply an upward-intensity operation — a sharpening
        //           or amplification of the containing molecule's primary quality.
        //           Distinct from MORE (magnitude scale) in that VERY applies to
        //           quality intensity, not quantity. No sub-type at the particle layer;
        //           the target quality is molecule-layer content.
        //
        // Parallel to NOT (bit 8) and MORE (bit 7): single class flag, no sub-encoding.
        // Confinement: a naked VERY quark has no referent. Exists only bonded into
        // molecules where the target is determined by bonding context.
        //
        // Intensity compositions (all molecule-layer compounds):
        //   VERY GOOD   = GOOD-valence molecule + VERY
        //   SLIGHTLY    = NOT + VERY  ("not-very" = attenuated intensity)
        //   MOMENT      = TEMPORAL + NOT + VERY  (minimal temporal intensity)
        //
        // NOT a substantive. Bit 13 is clear for VERY quarks. SetVery
        // auto-clears SUBSTANTIVE_MASK.
        //
        // RESERVED. Do not assign to vocabulary or resolution tiers.

        static constexpr AZ::u32 VERY_BIT  = 6u;
        static constexpr AZ::u32 VERY_MASK = 1u << 6u;
        static constexpr AZ::u32 VERY      = 1u << 6u;     // set = intensity-up operator

    } // namespace PrimePhase


    // ---- Inline helpers ----

    // I_NOT_I (global axis) -----------------------------------------------------------

    //! True if this phase value is on the I-side (bit 19 clear). Warranted-direct.
    inline bool IsIPhase(AZ::u32 phase)    { return (phase & PrimePhase::I_NOT_I_MASK) == 0u; }

    //! True if this phase value is on the Not-I-side (bit 19 set). Modeled ToM.
    inline bool IsNotIPhase(AZ::u32 phase) { return (phase & PrimePhase::I_NOT_I_MASK) != 0u; }

    //! Return phase with bit 19 cleared (I-side). Other bits unchanged.
    inline AZ::u32 SetIPhase(AZ::u32 phase)    { return phase & ~PrimePhase::I_NOT_I_MASK; }

    //! Return phase with bit 19 set (Not-I-side). Other bits unchanged.
    inline AZ::u32 SetNotIPhase(AZ::u32 phase) { return phase | PrimePhase::I_NOT_I_MASK; }

    // VALENCE (global axis) -----------------------------------------------------------

    //! True if this phase is on the GOOD side (bit 18 clear).
    inline bool IsGood(AZ::u32 phase) { return (phase & PrimePhase::VALENCE_MASK) == 0u; }

    //! True if this phase is on the BAD side (bit 18 set).
    inline bool IsBad(AZ::u32 phase)  { return (phase & PrimePhase::VALENCE_MASK) != 0u; }

    //! Return phase with bit 18 cleared (GOOD pole). Other bits unchanged.
    inline AZ::u32 SetGood(AZ::u32 phase) { return phase & ~PrimePhase::VALENCE_MASK; }

    //! Return phase with bit 18 set (BAD pole). Other bits unchanged.
    inline AZ::u32 SetBad(AZ::u32 phase)  { return phase | PrimePhase::VALENCE_MASK; }

    // ACCESS_MODE (global axis) -------------------------------------------------------

    //! True if this phase is on the theoretical side (bit 17 clear). THINK / WANT / KNOW.
    inline bool IsTheoretical(AZ::u32 phase)  { return (phase & PrimePhase::ACCESS_MODE_MASK) == 0u; }

    //! True if this phase is on the experiential side (bit 17 set). FEEL / SEE / HEAR.
    inline bool IsExperiential(AZ::u32 phase) { return (phase & PrimePhase::ACCESS_MODE_MASK) != 0u; }

    //! Return phase with bit 17 cleared (theoretical side). Other bits unchanged.
    inline AZ::u32 SetTheoretical(AZ::u32 phase)  { return phase & ~PrimePhase::ACCESS_MODE_MASK; }

    //! Return phase with bit 17 set (experiential side). Other bits unchanged.
    inline AZ::u32 SetExperiential(AZ::u32 phase) { return phase | PrimePhase::ACCESS_MODE_MASK; }

    // FLOW_MODE (global axis) ---------------------------------------------------------

    //! True if this phase is on the consumption side (bit 16 clear). Content received.
    inline bool IsConsumption(AZ::u32 phase) { return (phase & PrimePhase::FLOW_MODE_MASK) == 0u; }

    //! True if this phase is on the expression side (bit 16 set). Content produced.
    inline bool IsExpression(AZ::u32 phase)  { return (phase & PrimePhase::FLOW_MODE_MASK) != 0u; }

    //! Return phase with bit 16 cleared (consumption side). Other bits unchanged.
    inline AZ::u32 SetConsumption(AZ::u32 phase) { return phase & ~PrimePhase::FLOW_MODE_MASK; }

    //! Return phase with bit 16 set (expression side). Other bits unchanged.
    inline AZ::u32 SetExpression(AZ::u32 phase)  { return phase | PrimePhase::FLOW_MODE_MASK; }

    // TEMPORAL (physical axis) --------------------------------------------------------

    //! True if this phase marks a temporal anchor / selector quark (bit 15 set).
    inline bool IsTemporal(AZ::u32 phase) { return (phase & PrimePhase::TEMPORAL_MASK) != 0u; }

    //! Return phase with bit 15 set and SUBSTANTIVE cleared — time axis active.
    //! Enforces: IsTemporal(p) => !IsSubstantive(p).
    inline AZ::u32 SetTemporal(AZ::u32 phase)
    {
        return (phase | PrimePhase::TEMPORAL_MASK) & ~PrimePhase::SUBSTANTIVE_MASK;
    }

    //! Return phase with bit 15 cleared — time axis inactive.
    inline AZ::u32 ClearTemporal(AZ::u32 phase) { return phase & ~PrimePhase::TEMPORAL_MASK; }

    // SPATIAL (physical axis) ---------------------------------------------------------

    //! True if this phase marks a spatial anchor / selector quark (bit 14 set).
    inline bool IsSpatial(AZ::u32 phase) { return (phase & PrimePhase::SPATIAL_MASK) != 0u; }

    //! Return phase with bit 14 set and SUBSTANTIVE cleared — spatial axes active.
    //! Enforces: IsSpatial(p) => !IsSubstantive(p).
    inline AZ::u32 SetSpatial(AZ::u32 phase)
    {
        return (phase | PrimePhase::SPATIAL_MASK) & ~PrimePhase::SUBSTANTIVE_MASK;
    }

    //! Return phase with bit 14 cleared — spatial axes inactive.
    inline AZ::u32 ClearSpatial(AZ::u32 phase) { return phase & ~PrimePhase::SPATIAL_MASK; }

    // SUBSTANTIVE (class) -------------------------------------------------------------

    //! True if this phase marks a substantive (thing) particle (bit 13 set).
    inline bool IsSubstantive(AZ::u32 phase)       { return (phase & PrimePhase::SUBSTANTIVE_MASK) != 0u; }

    //! Return phase with bit 13 set — marks as a substantive (thing) class particle.
    inline AZ::u32 SetSubstantive(AZ::u32 phase)   { return phase | PrimePhase::SUBSTANTIVE_MASK; }

    //! Return phase with bit 13 cleared — removes substantive class designation.
    inline AZ::u32 ClearSubstantive(AZ::u32 phase) { return phase & ~PrimePhase::SUBSTANTIVE_MASK; }

    // KIND_PART (class) ---------------------------------------------------------------

    //! True if this phase marks a structural binding particle (bit 12 set).
    inline bool IsKindPart(AZ::u32 phase) { return (phase & PrimePhase::KIND_PART_MASK) != 0u; }

    //! True if this phase marks a KIND particle (bit 12 set, bit 11 clear).
    inline bool IsKind(AZ::u32 phase)
    {
        return (phase & (PrimePhase::KIND_PART_MASK | PrimePhase::KIND_PART_TYPE_MASK))
               == PrimePhase::KIND_PART_MASK;
    }

    //! True if this phase marks a PART particle (bits 12 and 11 both set).
    inline bool IsPart(AZ::u32 phase)
    {
        return (phase & (PrimePhase::KIND_PART_MASK | PrimePhase::KIND_PART_TYPE_MASK))
               == (PrimePhase::KIND_PART_MASK | PrimePhase::KIND_PART_TYPE_MASK);
    }

    //! Return phase encoding a KIND particle (bit 12 set, bits 11 and 13 cleared).
    //! Enforces: IsKind(p) => !IsSubstantive(p).
    inline AZ::u32 SetKind(AZ::u32 phase)
    {
        return (phase | PrimePhase::KIND_PART_MASK)
               & ~PrimePhase::KIND_PART_TYPE_MASK
               & ~PrimePhase::SUBSTANTIVE_MASK;
    }

    //! Return phase encoding a PART particle (bits 12 and 11 set, bit 13 cleared).
    //! Enforces: IsPart(p) => !IsSubstantive(p).
    inline AZ::u32 SetPart(AZ::u32 phase)
    {
        return (phase | PrimePhase::KIND_PART_MASK | PrimePhase::KIND_PART_TYPE_MASK)
               & ~PrimePhase::SUBSTANTIVE_MASK;
    }

    // ACTION (class) ------------------------------------------------------------------

    //! True if this phase marks an action class quark (bit 10 set).
    inline bool IsAction(AZ::u32 phase) { return (phase & PrimePhase::ACTION_MASK) != 0u; }

    //! Return phase with bit 10 set and SUBSTANTIVE cleared — action class quark.
    //! Enforces: IsAction(p) => !IsSubstantive(p).
    inline AZ::u32 SetAction(AZ::u32 phase)
    {
        return (phase | PrimePhase::ACTION_MASK) & ~PrimePhase::SUBSTANTIVE_MASK;
    }

    //! Return phase with bit 10 cleared — removes action class designation.
    inline AZ::u32 ClearAction(AZ::u32 phase) { return phase & ~PrimePhase::ACTION_MASK; }

    // ENTITY (class) ------------------------------------------------------------------

    //! True if this phase marks a substantive with person-like attribution (bit 9 set).
    //! Invariant: IsEntity(phase) => IsSubstantive(phase).
    inline bool IsEntity(AZ::u32 phase) { return (phase & PrimePhase::ENTITY_MASK) != 0u; }

    //! Return phase with bit 9 set and SUBSTANTIVE auto-set.
    //! Enforces: IsEntity(p) => IsSubstantive(p).
    inline AZ::u32 SetEntity(AZ::u32 phase)
    {
        return phase | PrimePhase::ENTITY_MASK | PrimePhase::SUBSTANTIVE_MASK;
    }

    //! Return phase with bit 9 cleared — removes entity attribution.
    inline AZ::u32 ClearEntity(AZ::u32 phase) { return phase & ~PrimePhase::ENTITY_MASK; }

    // NOT (class) ---------------------------------------------------------------------

    //! True if this phase marks a negation operator quark (bit 8 set).
    inline bool IsNot(AZ::u32 phase) { return (phase & PrimePhase::NOT_MASK) != 0u; }

    //! Return phase with bit 8 set and SUBSTANTIVE cleared — negation operator quark.
    //! Enforces: IsNot(p) => !IsSubstantive(p).
    inline AZ::u32 SetNot(AZ::u32 phase)
    {
        return (phase | PrimePhase::NOT_MASK) & ~PrimePhase::SUBSTANTIVE_MASK;
    }

    //! Return phase with bit 8 cleared — removes negation operator designation.
    inline AZ::u32 ClearNot(AZ::u32 phase) { return phase & ~PrimePhase::NOT_MASK; }

    // MORE (class) --------------------------------------------------------------------

    //! True if this phase marks a magnitude-up operator quark (bit 7 set).
    inline bool IsMore(AZ::u32 phase) { return (phase & PrimePhase::MORE_MASK) != 0u; }

    //! Return phase with bit 7 set and SUBSTANTIVE cleared — magnitude-up operator.
    //! Enforces: IsMore(p) => !IsSubstantive(p).
    inline AZ::u32 SetMore(AZ::u32 phase)
    {
        return (phase | PrimePhase::MORE_MASK) & ~PrimePhase::SUBSTANTIVE_MASK;
    }

    //! Return phase with bit 7 cleared — removes magnitude-up operator designation.
    inline AZ::u32 ClearMore(AZ::u32 phase) { return phase & ~PrimePhase::MORE_MASK; }

    // VERY (class) --------------------------------------------------------------------

    //! True if this phase marks an intensity-up operator quark (bit 6 set).
    inline bool IsVery(AZ::u32 phase) { return (phase & PrimePhase::VERY_MASK) != 0u; }

    //! Return phase with bit 6 set and SUBSTANTIVE cleared — intensity-up operator.
    //! Enforces: IsVery(p) => !IsSubstantive(p).
    inline AZ::u32 SetVery(AZ::u32 phase)
    {
        return (phase | PrimePhase::VERY_MASK) & ~PrimePhase::SUBSTANTIVE_MASK;
    }

    //! Return phase with bit 6 cleared — removes intensity-up operator designation.
    inline AZ::u32 ClearVery(AZ::u32 phase) { return phase & ~PrimePhase::VERY_MASK; }

} // namespace HCPEngine
