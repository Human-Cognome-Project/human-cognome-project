# Outreach drafts — 2026-06-11 (UNTRACKED ON PURPOSE — do not commit until approved;
# the repo is public, so a committed draft is a published draft)

---

## MEDIUM ARTICLE DRAFT

# Cognition Is Computationally Tractable. Here's a Minute of Evidence.

Last week, a machine in my office converted the entire English dictionary — 577,000
definitions — into executable structural formulas. It found 306,421 distinct concepts,
detected twenty thousand synonym groups, and resolved the full definitional dependency
graph of the language.

It took under a minute. On one thread. Of a 2011 AMD processor. The machine's two GPUs
sat idle the whole time — the workload never needed to wake them.

Run it again and you get the same 306,421 concepts, bit-identical, checksum-verifiable.
The code is public, and you can reproduce the result this afternoon.

I want to make a careful claim about what this means, because the claim matters more
than the benchmark: **the computational cost of semantic work was never the computation.
It was the disorganization.**

## The assumption everyone stopped questioning

The modern AI industry is built on an unexamined premise: that meaning is expensive.
That extracting structure from language requires datacenters, that "understanding" a
dictionary's worth of knowledge is a megawatt problem. The scaling curves are treated
as laws of nature.

But look at what those systems actually spend their compute on. Every training run
statistically *reconstructs* the structure of language from scratch — from unorganized
streams, with nothing indexed, nothing addressed, nothing kept. The expense isn't the
semantics. The expense is refusing to write anything down.

We tried the opposite bet: organize the data once, explicitly, and see what semantic
work costs afterward.

## What we actually did

The Human Cognome Project is building what we call a structural symbol library for
concepts — the analogue, for meaning, of what asset libraries and physics engines
already are for 3D worlds. The core is small: roughly ten irreducible database
operations (assert a node, link two nodes with a typed edge, set a value on an axis,
apply a force toward a target, open a gated scope...), over which the ~65 semantic
primes of natural language — and everything built from them — are expressed as pure
functions.

A dictionary definition, read this way, is not prose. It is a formula: an ordered,
sectioned composition of those operations, written in words. So we wrote a parser
kernel — a few hundred lines of C++ — that converts every definition into its formula
and assigns each formula an identity key computed from its own structure.

Then the kernel does something simple and decisive: it ladders. A definition is
*complete* when every content word in it is itself a defined concept. Complete
definitions mint concepts; newly minted concepts complete other definitions; repeat.
The dictionary bootstraps itself outward from a seed of about three hundred semantic
primitives, in thirty passes, like a wave.

Identity-by-structure does surprising work for free. Two words whose definitions have
the same structure collapse to the same concept — synonymy detected by primary-key
collision. One of my favorite spot checks: a sense of *knowledge* parsed to the
formula `justified, TRUE, belief` — with TRUE resolved to its concept address. The
philosophy-textbook definition, found by a parser that wasn't looking for it.

## What the formulas are — and aren't

I call these formulas an **alpha example of conceptual geometry definitions**, and
both words are doing real work.

*Geometry*, because concepts in this system have positions, not descriptions. A
concept is an address in a structured space; its KIND and PART edges are its
coordinates; categories are regions; similarity is distance; reading an address at
shorter prefix length literally gives you a coarser concept — level-of-detail for
meaning. Emotions, in the affect region, decompose into force configurations: *fear*
is an anticipated negative delta against your goal structure with no available
counter-force. The geometry is literal, not metaphorical — it runs.

*Alpha*, because these are first-generation formulas with known roughness. Multiword
expressions are deliberately deferred. Proper names and labels are excluded by a
preflight rule. Some formulas are crude. The system's design principle is not "zero
error" — it's **addressable error**: every mistake is a row you can find, diff, and
fix, rather than a behavior smeared invisibly through a billion weights. (That
principle earned its keep during this work: a 0.01% nondeterminism between runs was
found by checksum, diagnosed to a database scan-order leak, fixed, and verified
byte-identical — in ten minutes.)

## The history that makes this interesting

Here's the part I find genuinely strange. The research program this work descends
from — symbols, structure, composition — was abandoned by mainstream AI in the
mid-1990s, right around the time Pentium-class processors arrived. Statistical methods
suddenly looked better, not because the symbolic program was refuted, but because
compute got cheap enough to skip the organizational work and still ship benchmarks.
Moore's law subsidized disorganization for twenty-five years. When that subsidy
flattened, the bill came due — and the industry named the bill "the scaling wall" and
bought GPUs.

The abandoned path, meanwhile, inherited three decades of hardware it never got to
run on. The machine that did this conversion was mid-range when Obama was president.

## Why "tractable" is the right word

A dictionary-wide conversion is this architecture's *worst case* — a deliberate
touch-everything pass over an entire language. It costs a minute. Actual cognition
never does that: thought is local, touching hundreds or thousands of nodes per cycle,
not millions. A thought stream is the *sparse* case of the operation whose dense case
you just watched. Run the arithmetic and a real-time cognitive workload lands two to
four orders of magnitude below what one thread of fifteen-year-old silicon just
demonstrated.

Cognition, over organized data, is not a moonshot workload. It is a modest one. That
is the claim, and as of last week it has a benchmark, a reproduction path, and a
public repository.

We're building the engine next.

*— Patrick (Human Cognome Project). Code, docs, and the kernel:
github.com/Human-Cognome-Project/human-cognome-project*

---

## LINKEDIN POST DRAFT (pointer)

Last week a 2011-era CPU in my office converted the entire English dictionary —
577,000 definitions — into 306,421 executable concept structures.

Under a minute. One thread. Both GPUs in the machine left idle. Bit-identical on every re-run.

The interesting part isn't the speed — it's what the speed implies. The cost of
semantic work was never the computation. It was the disorganization. Organize the
data once, explicitly, and the workload the industry prices in datacenters becomes a
coffee-break job for hardware older than Instagram.

The formulas themselves are an alpha of something we call conceptual geometry:
concepts as addresses in structured space — categories as regions, similarity as
distance, meaning with coordinates. First-generation, rough edges documented, every
error addressable as data.

Full story in the article below. Code is public — reproduce it this afternoon.

[link to Medium article]

#AI #CognitiveScience #ComputationalSemantics #KnowledgeRepresentation

---

## NOTES FOR PATRICK'S EDIT

- Both drafts are in your voice; edit freely — especially the personal beats ("my
  office", spot-check choices).
- Deliberately NOT claimed: meaning extraction, AGI-anything, LLM-bashing (the
  contrast is factual and readers do the arithmetic themselves).
- Deliberately claimed: tractability thesis (your framing), alpha conceptual
  geometry (your framing), reproducibility, addressable error.
- Release tag intentionally absent per your call — article points at the repo, not
  at a release.
- The knowledge = justified-TRUE-belief spot check is machine-real (kernel output,
  claim 542) — safe to cite.
- FACT CORRECTION (2026-06-11): physics HAS two GPUs; they were simply not engaged.
  Say "GPUs idle / no GPU involvement" — never "no GPU in the box". The idle framing
  is stronger anyway: parallel hardware was available and the workload never asked.
