# The Linguistic / Conceptual Separation

The architecture draws a hard line between **language** and **concept**. Understanding this
line — and why the two currently look fused — is essential to reading everything in
[03-concept-substrate](../03-concept-substrate/).

Sources: claims 171 (layer separation), 209 (AA/AB namespaces), 223 (semiotic triangle),
240 (conceptual hub), 242 (concept-direct vs language-derived), 243 (NSM as current
compiler), 256 (primes and PoS are non-aligned).

---

## The line

> *Linguistic composition is distinct from concept manipulation, and the second cannot happen
> without the first.* — claim 171

- **Linguistic composition** assembles meaning-bearing units (words) from byte codes by
  identifying which known tokens a letter sequence spells. It answers the question
  *"what word do these letters spell?"* It produces **keys** — token identifiers — without
  resolving what they point at. (This is exactly the job of the
  [resolution chamber](../04-engine/resolution-chamber.md), claim 169.)

- **Concept manipulation** operates on what those keys *point at* — meanings, prime/molecule
  structures, world-model relationships. Meaning work happens **here**, not in the chamber.

You cannot resolve *"what does this sentence mean"* without first knowing *"what tokens are in
this sentence."* Keys before referents. This is the **keys-vs-referents** seam, and it recurs
all the way down: keys are token IDs; referents are concept nodes; the key→referent mapping is
the substrate's lookup machinery (the db-function dispatch of claim 5).

---

## Two namespaces: AA (concept) and AB (text)

The separation is realized concretely in the namespace layout (claim 209, decision 002):

- **AA-namespace** tokens live in `hcp_core`. They are the universal computational concepts.
- **AB-namespace** tokens live in `hcp_english`. They are how English *expresses* those
  concepts.

A shared surface label does **not** mean a shared entity. The NSM prime **WANT** (AA,
`hcp_core`) and the English word **want** (AB, `hcp_english`) carry the same spelling but are
distinct entities with distinct roles.

> *"Words in the core database are labels for understanding; they are not the thing itself."*

---

## The three layers (semiotic triangle)

The classic ontological / conceptual / linguistic triangle maps directly onto HCP's storage
tiering (claim 223):

| Semiotic layer | HCP layer | Storage tier |
|----------------|-----------|--------------|
| **Ontological** | primes-as-forces (physical-reality grounding) | cold |
| **Conceptual** | molecules-as-stable-configurations (concept space) | warm |
| **Linguistic** | lexicalized tokens (the text stream) | hot |

The ontological layer is the grounding *beneath* the conceptual one — primes are forces
before they are concepts.

---

## Why language and concept currently look fused

Today the linguistic (English-text) and conceptual layers sit **very tightly bound** beside
each other. This is **a temporary artifact of scope**, not the end state (claim 240): the
project is currently encoding *one* expression type (text) in *one* language (English). With
only one language in the system, the English-skin and the concept-core have nothing to be
*distinguished against*.

The relationship is a **compilation-distance** one (claim 242):

> Concept and language are two compilations off the **same** db-primitive base, at different
> distances — structurally like the C++/Python relation (a structural equivalence, not a loose
> metaphor):
> - the **db primitives** are the C++ functions;
> - **pure concept** is the *more direct* compilation (close to the primitives — C++ or a
>   tightly-derived child language);
> - **English-text expression** is the *more derived* compilation (more layers out — Python).

Both genuinely compile from the same base; one is just nearer the metal. With only the two
endpoints visible (direct-concept and distant-English) and nothing bridging them, their shared
base is hard to see — which is exactly why they look fused-yet-hard-to-separate.

**Adding more expressions clarifies rather than complicates.** Each new language or modality is
another derived compilation off the same base; each re-exposes the shared primitives. So the
conceptual DB becomes the **hub that hovers between all expressions** — language- and
mode-neutral, connected to each but bound to none (claim 240). More modes entering makes the
common base *obvious*.

This is also why the English expression skin can be **messy without being wrong** (claim 241):
it is the more-derived, Python-distance layer, expected to be more elaborate than the
near-direct concept layer.

---

## Primes and parts-of-speech are *non-aligned* carvings

A subtle but load-bearing point (claim 256): the ~65 NSM primes and English parts-of-speech
are **different, non-aligned separations of the same conceptual space.** Going through the
primes, they cross over and subdivide PoS in odd ways. All the PoS content is "in there," but
the carving is not the same.

This generalizes: **every language has its own PoS-style constructs** with distinct rules,
similar to but not identical to English's. Consequently each language is its own database with
a distinct-but-related schema, related through the shared conceptual core and distinct in its
per-language carving. The conceptual hub relates *many* distinct language-databases, each of
which carves expression differently.

---

## NSM as the current compiler (an on-ramp, not the destination)

> *NSM primitives are being used as the closest available low-level compiler for the db
> functions of the conceptual tree.* — claim 243

The eventual destination is **language-neutral** db primitives (claim 200). NSM is the current
best vehicle for getting there, for two reasons: (a) the work is from an English base, and
(b) English is itself a *direct application* of the underlying principles — it has one of the
clearest db-construction structures of any language (claim 241). So an English-grounded NSM is
the best current compiler for building the conceptual tree, even though NSM's English glosses
are an artifact to be worked past, not gospel.

The current active work is precisely **extracting the language-independent base out of
English** (claim 257) — see [../03-concept-substrate/explication.md](../03-concept-substrate/explication.md)
and [../06-status/status.md](../06-status/status.md).

---

## See also

- [interface-and-tom.md](interface-and-tom.md) — why binding concept to a single interface
  (English alone) would *be* the architecture's core failure mode, and how the hub enables
  cross-interface translation.
- [../03-concept-substrate/primes-and-molecules.md](../03-concept-substrate/primes-and-molecules.md)
  — the prime/molecule taxonomy that populates the conceptual layer.
