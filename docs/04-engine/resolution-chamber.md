# The Resolution Chamber

The resolution chamber is how raw input text becomes **identified tokens.** It is a pure
pattern-matcher: its job is *identification*, not meaning. This is the linguistic-composition stage
of the [linguistic/conceptual separation](../02-architecture/linguistic-vs-conceptual.md).

Sources: claims 85 (progressive composition), 169 (identification not meaning), 168 (20-char
slots), 153 (scan→manifest→sort→resolve), 150/154 (whitespace), 175–181 (the resolution tiers),
183 (multi-word, planned), 187 (output storage), 56 (see-it-mint-it), 162 (unit bonding).

> **Scope marker (claim 145):** everything above the byte-code layer here is **implicitly
> English-structured** — it assumes space-separated words and standard Latin-script punctuation as
> boundary markers. Latin-script space-separated languages inherit it for free; non-space-delimited
> (Chinese, Japanese, Thai), RTL (Arabic, Hebrew), and significantly different punctuation
> conventions need language-specific layers that **do not yet exist**. This is a real scope, not a
> universal claim. Other-language support is post-proof-of-concept.

---

## What the chamber is for (and what it is *not*)

> *The chamber is bounded to identifying which known token(s) the input byte codes match, in the
> largest unit they fit. **Meaning is irrelevant at this stage.*** — claim 169

Two purposes only:

1. **Reproduce** the input — preserve enough to reconstruct the original exactly (modulo
   whitespace, which is regenerated — see below).
2. **Analyze structural knowledge** — flag what the system *does* know (segments matching known
   tokens) vs what it *does not* (unknown forms). The output is a **knowledge map at the word
   level.**

Explicitly **downstream** of the chamber, not its job: homograph meaning disambiguation, PoS / class
assignment, token→prime/molecule mapping, concept resolution, idiom recognition. Chamber output is a
position-preserving sequence of identified-token-or-unknown markers.

---

## The pipeline: scan → manifest → sort → resolve

Above the byte-code layer the order is (claim 153):

1. **FAST SCAN** — pass over the canonical byte stream, carving it into a word/punctuation
   **manifest** using space + English punctuation as boundary markers (claim 150). Output: a catalog
   of word/punctuation candidates with position/length metadata. This step is *structural*
   (boundary detection), not semantic — which is why it is fast.
2. **SORT** — sort the manifest by **length** and **start-letter**, the same axes the chamber uses
   (claim 85), organizing candidates into the chamber's bucket structure.
3. **RESOLVE** — chamber composition determines what the words actually are: progressive composition
   through orders of construction up to the largest identified unit, mining every minted surface form
   via the (length, start-letter) buckets.

Two-stage by design: cheap boundary detection produces the input shape; expensive composition runs
only against well-organized candidates.

**Whitespace is elided** (claims 150/154): used as a boundary marker during the scan, then *dropped*
— it does not enter the substrate. On output, whitespace is **regenerated** from production rules
(English baseline: space between words, no space before period/comma, etc.). Round-trip is
*canonical* whitespace, not original formatting. (Contrast see-it-mint-it, claim 56: a token for
every surface form *except* whitespace, which is structural, not content.)

---

## Chamber slot structure: 20-char, front-padded

Primary chambers use **20-character slots**, identical on input and resolution sides (claim 168).
20 covers most English words. Shorter candidates are **front-padded**: padding fills the leading
positions, the word occupies the trailing positions, and **every word ends at the slot's last
position.** This avoids restructuring the chamber when inserting variable-length candidates — uniform
slots, O(1) insertion, cache-friendly, no compaction.

The length × start-letter axes fall out naturally: word length determines where the start character
sits (position `20 − length`); bucketing by (length × start-letter) becomes **direct addressing**
into a fixed grid. This is the same canonical-fixed-width-with-front-padding pattern used at the byte
scale (claim 141). Words longer than 20 chars are handled by overflow machinery (not yet detailed).

---

## The three-tier resolution strategy

The chamber is bounded — cheap mechanisms first, a few bounded fallbacks, then graceful stop
(greedy-LoD, claim 16). **Output is always lossless**, even when identification is incomplete
(claim 181).

### Tier 1 — Direct match

Identify the word as one contiguous piece, filtered by easy-to-spot factors. Indexed by length,
start-letter, and **significant characters and their positions:**

- **First stage targets Labels** (claim 175): capitalized words catalogued as components of *proper
  constructs* (names, places, brands). Label candidates are the non-positionally-explained
  capitalized words (mid-sentence caps, mixed-case like "iPhone", acronyms like "NASA"); a
  sentence-initial cap whose lowercase form is a known word is *excluded*. Resolved early so "Bill"
  the name doesn't compete with "bill" the noun. Common case is small + high-reuse (a novel reuses
  ~50 names thousands of times → flat cross-length comparison); a census-sized candidate set
  escalates to length-aware comparison.
- **Significant-character position indices** (claim 177) extend the axes for punctuation-bearing
  categories — e.g. apostrophe: `ends-with-'s` (John's, it's), `ends-with-s'` (kids'),
  `internal-'` (don't, they're); hyphen: `internal-` (well-known, mother-in-law). The punctuation
  *position* narrows matches before character comparison runs. Categories with no potential matches
  are skipped entirely.

### Tier 2 — Standard transformations

When direct match fails, apply a **bounded** set of transformations, each trying the next when it
doesn't yield a clean result:

- **Capitalization fallback** (claim 176): drop to lowercase, match the general word table, and
  record the original capitalization on a **lower-case map** (side metadata: "this token's surface
  form had cap pattern Y"). The concept engine later decides: new Label / error / other edge case.
- **Partial-match composition** (claim 179): when stages fail but partial matches compose the full
  input, provide the **largest atomic elements**, declare a **document var**, and flag it via
  contiguous manifest positions without whitespace (the structural signature of an unresolved
  composition).
- **Morpheme deconstruction for neologisms** (claim 180): split the failed word into
  `[morpheme + word]` (`unfriend → un- + friend`; `googleable → google + -able`). The structural
  split is informative *even when constituents are themselves unknown* — it tells the concept engine
  "morphologically-coherent neologism, not noise." This is the inverse of the unit-bonding mechanism
  (claim 162), using the same morpheme registry as iconographic composition (claim 165).

### Tier 3 — Spell it and call it a day

When no tier-2 transformation succeeds, **atomize the spelling** (constituent characters as a stored
sequence) and **flag for minting** (claim 181). The chamber does not pursue infinite resolution — it
preserves the data and stops gracefully.

**Resolution order:** clean token → partial match → morpheme deconstruction → atomized + mint flag.

---

## Priority order is an envelope factor

The order in which categories are tried is **not fixed** — it is part of what the active
[envelope](cognitive-cycle.md#what-napier-works-on-each-moment-the-envelope) contributes (claim 178,
envelopes as fluid Venn structures). A novel envelope prioritizes Labels; a technical-manual envelope
raises hyphenated compounds and possessives; a poetry/archaic envelope raises `'tis`/`'twas`; a
census envelope escalates Label comparison to length-distinguished earlier. Envelope-driven priority
shapes how resolution flows for the active context.

---

## The learning loop and output storage

The chamber has **three output channels** for the concept engine (claim 179), all lossless: clean
token IDs (main flow), document vars (partial-match metadata), atomized spellings with mint flags
(no-match metadata) — plus the lower-case map and morpheme deconstructions.

**Storage architecture** (claim 187), a clean write-surface split (source-locked, claim 114):

| Store | Tier | Chamber's access | Holds |
|-------|------|------------------|-------|
| **Dictionary** (token table) | warm reference DB ("what I can do") | **reads** only | persistent minted tokens — Labels, words, multi-word constructs |
| **Var DB** | warm ("what I am doing") | **writes** | all incomplete-identification metadata, scoped to the document/context |

Token *creation* flows through the concept engine, not the chamber: **the linguistic stage writes
the var DB and reads the dictionary; the conceptual stage reads the var DB and writes the
dictionary.** Together they implement the learning loop: unresolved input → atomized storage + mint
flag → concept engine decides (new token / typo / retain-as-composition) → minting → next pass
resolves cleanly. Document vars encode "try this combination first next pass." Over time the var
population shrinks.

**Two processing cadences** (claim 187): *interactive/single-document* mode processes vars eagerly
(real-time learning); *bulk-ingestion* mode lets vars accumulate as metadata for a later review pass
(faster ingestion + cross-document patterns become confidently mintable).

---

## Planned: higher-order multi-word constructs

Multi-word constructs ("New York City", "ice cream", idioms, set phrases) are **not currently
detected** (claim 183) — planned, an offshoot of the same chamber machinery with different
filtering. The plan: assemble an envelope-scoped construct list, index by `(first_token,
token_count)`, scan the single-word manifest, verify followup tokens, and collapse full matches to a
single construct token. Missed constructs go to the **cold shard** (claim 51) for longer-term
matching via the O(n²) cross-link crawl, tagged for future priority. This is the same
"good-and-adaptive, not perfect-first-pass" pattern (claim 184): good-enough first pass + a learning
loop that fills gaps. (See also the storage-compression win, claim 172.)

---

## See also

- [../02-architecture/linguistic-vs-conceptual.md](../02-architecture/linguistic-vs-conceptual.md)
  — why identification (keys) precedes meaning (referents).
- [../05-data-layer/var-and-continuation.md](../05-data-layer/var-and-continuation.md) — the var DB
  lifecycle and the continuation index behind multi-word detection.
- [implementation-baseline.md](implementation-baseline.md) — what the current C++ chamber actually
  implements today.
