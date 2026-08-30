# Architecture

**One operation, one field, one addressing scheme.** Prerequisite reading:
[physics-basis.md](physics-basis.md).

## The structure of the space

**The base particle is an undefined single hex character** — a nibble, in superposition: allocated,
but with no hex ID attached. Undefined particles transmit any imbalance as radial force but never
change themselves unless *pulled into a specific value by force*. They are the medium — the space's
"vacuum" is undefined nibbles, which is the physical-vacuum-is-field claim implemented at four bits.

**Two nibbles defined together make a full hex code — the valid, active particle.** One centre is
monopolar: force but no signal. Two centres are the minimum from which periodicity can develop —
the smallest thing that can *mean*. Pairing is **positional** (the adjacent nibble in the
calculation's frame): nibbles are pre-meaning, and relations are a privilege of identities, so below
the pair threshold only geometry and force exist. No chain, view, address, or composition list ever
references a nibble; the byte is the smallest relational object.

The practical driver is exact: **UTF-8 uses two hex codes per byte.** The active particle is the
atomic unit of the world's text storage. Every document on Earth is already a stream of two-nibble
particles; intake needs no transcoding layer and no imposed tokenizer at the bottom, and UTF-8's own
layering (bytes → code points → graphemes → words) stacks directly onto the composition rule below.

**There is no assignment operator.** The only write primitive is force: state changes happen when
surrounding imbalance pulls an undefined particle into a value. Writing is condensation; intake is
applying boundary imbalances that force definition; a half-defined pair is a differential that
recruits its own completion.

## Addressing: arrayed pairs

Every reference is an address, and the canonical form is an **array of two-character base-50 pairs**
(1–5 pairs deep in the current data; the alphabet is the 52 letters minus O/o — O reads as 0 and
creates friction for humans; 50 is a clean number). The familiar dotted string (`AB.cd.EF`) is the
**display form only**, generated at the boundary and never persisted. Storage that persists the
display form is the defect the previous era demonstrated.

Properly arrayed, the address system *is* the rooted tree the runtime needs:

- **Zoom is truncation**: the first k pairs name the level-k container.
- **Neighbourhood is shared prefix**; LoD membership is prefix arithmetic.
- **Lazy consumption**: an operation uses only as much of the address as it requires — a far-field
  pass reads two pairs where a fine pass reads five — and anything b-treeable or octree-able
  benefits directly, because each level is a typed sort key and address depth = tree depth.

The executable statement of this convention is [`extraction/token_id.py`](../extraction/token_id.py),
carried forward from the previous era with its tests.

## Storage: one flat pool, many logical chains

Physical storage is a **flat pool**: one element per identity, allocated once, never duplicated —
the single source of truth. All structure lives in **logical LoD chains**: index maps maintained by
a CPU bookkeeper, each chain one dimension's distance metric over the same pool. An element
participates in as many chains as have connections to it and knows about none of them. Chains are
derived and rebuildable — never precious; a structural rethink is a new map, and the ledger never
moves. Elements join and leave active chains by threshold: a relaxed region is computationally
silent.

Cross-chain coherence is per engine tick (kernel launch), not per instruction. That lag is not a
workaround — propagation delay is the physics; the launch boundary is the tick.

**LoD is not for building.** Level-of-detail compression is linear containment aggregation and must
never be the authority on membership. Structural aggregation runs: **transitive chains decide
membership** (closure over real connections, indifferent to address distance) → **a new first-class
value is declared for the grouping** (own address, own amount = the linear sum, flagged as
declared/derived with its derivation recorded) → that declared value may then be LoD-compressed.
Group values go stale when members change and are current again after re-derivation — adjustment
climbing the composition levels with lag, by design.

## Composition and compression

**Every entry is a composition list of token ids exactly one level down.** No level-skipping.
Expansion to any depth is iterated one-level lookups; a group entry is just an entry whose
composition list names its members. Each unique composition exists once and everything else
references it by id — the pool is a dictionary over its own contents, and **deduplication is the
balancing operation on the amount side**: two identical compositions are a differential the field
removes by merging. Perfect compression is the teleological limit; the incompressible remainder is
the real information. Structurally sorted arrayed addresses give long shared-prefix runs (store the
prefix once, delta-encode the rest), and the address-assignment policy that moves co-occurring
elements together *is* the attraction law — compression ratio is a live gauge of field balance.

## The singularity and NAPIER

Sites that aggregate and refine — Wiktionary/Kaikki, Gutenberg, arXiv, Wikipedia — are
black-hole/white-hole pairs: they draw in continuous, unstructured material and emit
signal-equivalent structure. Refinement *is* the continuous→periodic conversion. HCP is the big one
at the end, ingesting all of knowledge at the moving present edge and emitting one navigable field —
and it holds itself to the standard it applies to the others: its compression path is fully declared
(see [data-protocol.md](data-protocol.md)).

**The DI architecture lives on the other side of the singularity.** Its substrate contract is the
compressed field — it never touches raw intake, which resolves sampler saturation at the root. Its
cognition is the same single balancing operation running in the emitted field; its own productions
are placed elements at the open face of "now," subject to the same physics as every creator behind
it. The inference layer is **NAPIER** — *Not Another Proprietary Inference Engine, Really!* —
solving target flows between placed elements: pin the endpoints history gives you, reconstruct the
flux between them, and treat an unresolvable differential as the detection of an unread source.

## Engine substrate

The foundation is **Taichi** — specifically for its wave-field machinery and sparse spatial
hierarchies (SNode LoD stacking). `taichi_core` is pure C++ with Python as a tooling front end;
the work here is primitive enough that direct C++ against the core is the probable path (evaluated
as we go; the Python-in-the-hot-path ban stands meanwhile). One kernel — the balancing op —
compounded as appropriate; the exponent ladder is the API, not a zoo of forces.

Engineering practice carried from the previous era, mandated for every kernel: **oracle-first
validation** — a portable CPU reference as deterministic oracle, the GPU kernel as its mirror, an
equivalence harness asserting GPU == CPU on real hardware. And every artifact ships with tests.
