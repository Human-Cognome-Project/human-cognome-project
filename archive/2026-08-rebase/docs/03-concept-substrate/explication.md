# Explication: Meaning as a Database Instruction Set

This is one of the most load-bearing realizations in the current work. It defines what a word's
*meaning* actually **is** in NAPIER terms.

Sources: claims 258 (explication as db-instruction-set), 221 (acyclic DAG + necessity),
259 (bit-work as clean categorization), 261 (construction vs usage), 291 (active work area).

---

## A word's meaning *is* a db query

> *An NSM explication set for a word **is** a set of database instructions that find the
> commonly-tagged symbol (the concept) — read with level-of-detail in mind.* — claim 258

The explication's component lines are database operations whose **converging result** is the one
symbol they all commonly tag. A word's meaning is not *described by* its explication — it **is**
that db-instruction-set / query, resolving to the concept by **tag convergence.**

This is the db-functions keystone (claim 192) and grammar-is-db-function-api (claim 5) brought
all the way down to the single word/concept. It composes within the acyclic explication DAG
(below).

### Level-of-detail is the truncation rule

> *You run the query only as far as needed to **uniquely pin** the symbol at the active LoD.* — claim 258

The full prime-decomposition is the *maximal* instruction set. Greedy-LoD (claim 16) stops it the
moment the symbol is uniquely tagged — you don't decompose to primes if a shallower query already
isolates the concept. This is the same "curate a small n" discipline that makes the determination
engine tractable (claim 79, see
[../02-architecture/intelligence.md](../02-architecture/intelligence.md)).

---

## Structural constraints on explication

From the NSM-ISKO grounding (claim 221):

1. **Acyclic.** The prime-decomposition DAG must be acyclic — reject cycles at insertion, store
   with topological ordering. This is Goodman's no-circularity argument and Goddard's design
   constraint, and it prevents infinite regress.
2. **Necessity-bounded.** Decompose *only* when primes alone are insufficient. Over-decomposition
   (carrying analysis too far) is a recognized failure mode. When a meaning needs more than ~4
   levels of primes to express, introduce an intermediate **lexicalized molecule** instead of
   pushing deeper.
3. **Abstraction level ties to confidence.** A token's abstraction level = the number of
   combinatoric layers between it and the primes. Higher abstraction implies lower inherent
   confidence absent sufficient lower-level grounding — a groundedness metric wired into
   confidence dynamics (claims 22/79).

---

## Two kinds of rule: construction and usage

Explication is the **usage** side of a two-sided picture (claim 261):

- **Construction** = the linguistic/PoS blueprint = structural assembly ops (place + bind +
  cohere), **content-blind**. This is why resolution/reproduction is meaning-irrelevant (the
  chamber identifies words without resolving meaning, claim 169) and why PoS containers are
  "already-built, deferrable" (abundant open-source grammar exists). This side is
  grammar-is-db-function-api (claim 5).
- **Usage / why** = NSM / explication = the explanation of *what the structure is for* (the
  semantic/purpose rationale). This side is explication-as-db-query (claim 258).

The elemental db operations must serve **both** kinds. This is the same keys-vs-referents seam as
the [linguistic/conceptual separation](../02-architecture/linguistic-vs-conceptual.md), stated as
blueprint-vs-purpose. Translation works precisely because the two are separable: the **usage plan
is the invariant**, the construction is what varies and what you swap (claim 262).

---

## The bits *are* the clean categorization of explication

> *NSM's categories are muddy — they do not cleanly distinguish — but explication chains **can**
> be categorized by specific content type and structure. The bit-work is precisely the attempt to
> refine the elemental units so that every word in the db can be expressed as db operations that
> translate to explication statements.* — claim 259

So the [bit-classes](bit-classes.md) **are** the clean content-type-plus-structure categorization
of explication chains that NSM leaves muddy: a word's bit-layout encodes its
explication-as-db-operations. This is why the bit-layout *is* the LoD (claim 60), and why the
bit-class review (claim 237) is not a side task but the **core mechanism** of the
explication/db-operations goal.

---

## This is the live edge of the work

> *The area currently being actively worked is the **definition of the primitive functions** —
> the core db elemental operations and how they combine.* — claim 291

That means: the construction-blueprint ops + the usage/explication ops + the
punctuation/combinator functions that NSM assumes but does not specify (see
[punctuation-nonverbal.md](punctuation-nonverbal.md)), such that **every word resolves to db
operations that translate to explication statements** (claim 258). The deeper deeming/weighting
math (claim 286) sits *beyond* this, deferred — see
[../06-status/deferred-and-open.md](../06-status/deferred-and-open.md).

---

## See also

- [primes-and-molecules.md](primes-and-molecules.md) — the symbols explication resolves to.
- [bit-classes.md](bit-classes.md) — the categorization mechanism (**under active review**).
