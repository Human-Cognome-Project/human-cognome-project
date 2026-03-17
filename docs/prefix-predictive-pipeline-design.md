# Prefix Stripping + Predictive Pipeline Design
# 2026-03-13

## Current state (what exists)

- `TryInflectionStrip` — suffix stripping only (PAST/PROG/PLURAL/etc.)
- `TryVariantNormalize` — V-1 g-drop, V-3 archaic (-eth)
- `DetectSignals` — fires at len >= 4, tense/register only (was/were/had → past, etc.)
- Short-pass hook — single dominant signal → single envelope activation → inject into
  m_vocabByLength. Single-fire flag (shortPassSignalFired).
- LMDB priority order: Wikipedia + OpenSubtitles frequency blend, static at compile time.
- Resolve order: shortest-first ascending. Function words (2-4) establish context before
  longer forms are processed.

---

## A. Prefix Stripping (TryPrefixStrip)

### Order: prefix FIRST, then suffix
Prefix check is cheaper (12-string startswith, near-zero cost). Fire before TryInflectionStrip.
Most words won't start with a known prefix → falls through to suffix strip immediately.

### Bound prefixes (the 12)
    un  re  pre  mis  dis  de  non  in  im  il  ir  anti

Free-morpheme prefixes (over/under/out/up) excluded — too many false positives.

### Clean-strip class (non/anti/de/pro/post and similar)
These strip cleanly and don't remain as separate tokens. Base resolves; prefix encodes
as morpheme class on the positional modifier. The prefix string itself is recoverable
from the surface form / token_variants entry — no separate emission needed.

### Morpheme classes (working bits, not stored)
    PFX_NEG       — un-, in-, im-, il-, ir-   (negation)
    PFX_ITER      — re-                        (iteration/again)
    PFX_PRE       — pre-                       (before)
    PFX_MIS       — mis-                       (wrongly)
    PFX_NEG_DIS   — dis-                       (negation/reversal)
    PFX_REV       — de-                        (reversal/removal)
    PFX_NEG_NON   — non-                       (negation)
    PFX_ANTI      — anti-                      (against)

Class = what's relevant at resolution time and in concept space.
Specific prefix string = defined by the word (implicit). "misdo" isn't valid not because
of a rule, but because it's not in token_variants and doesn't resolve.

### Validation
Same pattern as TryInflectionStrip: strip prefix, call LookupWordLocal on base.
If base resolves → valid strip. PBD IS the existence check.
No pre-validation needed — the vocabulary is the filter.

### LoD
- Resolution layer (working): base token + PFX_CLASS bit
- DB layer (source of truth): token_variants defines valid combinations
- Concept space: class becomes a force vector on the root concept
  (negation force, iteration force, etc.). Prefix string dissolved at this layer.

### Wiring into resolve loop
Same pattern as inflection strip:
1. Try prefix strip on unresolved run
2. If base found: push to inflection queue with PFX_CLASS morph
3. Inject synthetic manifest entry for base if not already resolved
4. Dep-resolution pass attaches class to result

---

## B. Extended Signal Detection

### Current limitation
Single dominant signal: past > progressive > present → one envelope → done.
No person detection. No multi-signal composition.

### Target signals
Tense (existing):
    Past:        was, were, had, did, got, went
    Future:      will, shall, would, could, might
    Present:     is, are, am, has, does
    Progressive: being, going (+ -ing surface forms)
    Archaic:     hath, doth, thou, thee, hast, dost, wilt, wast

Person (new):
    First:   I, me, my, we, our, us
    Second:  you, your, yourself
    Third:   he, she, they, it, his, her, their

Register (new):
    Formal:   therefore, however, wherein, whereas, heretofore
    Casual:   gonna, wanna, gotta, yeah, nah, ain't
    Dialect:  recognized EYE_DIALECT markers already in token_variants

### Multi-signal composition
Multiple signals can be active simultaneously. All signals that cross threshold
contribute to envelope selection. Composite profile:

    person + tense → specific morpheme prediction
    e.g. third-person + past → heavy -ed + -'s/-'d on common verbs
         second-person + past → heavy "you + past" forms (Patrick's favourite)
         first-person + present → heavy -'m, -'ve, -'ll contractions

Signal detection stays in the DetectSignals function; ShortPassSignal struct
gains person flags. Envelope selection becomes: build list of applicable
envelopes from composite signal, activate in priority order.

### No separate pre-pass needed
Shortest-first ordering already surfaces person/tense signals naturally.
By length 4-5: have seen he/she/you/I, was/were/did, dialect markers.
Profile builds as side effect of normal resolution. Signal fires at len >= 4
exactly as now — just extend what it detects and how many envelopes it activates.

---

## C. Priority List from PBMs

### Current state
LMDB vocab ordered by Wikipedia + OpenSubtitles frequency blend.
Static at compile time. All texts get the same priority order.

### Target: PBM-composed priority

Layer 1 — PBM-frequency ranked (pre-loaded, warm before text starts):
    Default:      frequency across ALL ingested PBMs (global blend — same result as now)
    Topic-scoped: filter PBMs by genre/domain tags → frequency from that subset only
    The more domain PBMs ingested, the sharper the topic-specific priority

Layer 2 — First-occurrence order from current text (dynamically built):
    Non-PBM tokens: when first encountered in text → appended to working priority
    First occurrence = cold miss. All subsequent occurrences = warm hit.
    Early-occurring novel words warm faster → correct, they'll recur more by end of text.

### What this requires
- PBM token frequency accessible at resolve time
  (new query path OR pre-computed into a "pbm_topic_freq" envelope)
- Topic tag on PBM documents for filtering
- BedManager accepts a topic hint (from envelope manager or caller)
  → selects appropriate priority envelope on initialization

### Compounding effect
Ingesting more PBMs in a domain improves resolution SPEED for that domain (not just coverage).
Topic-scoped priority → high-frequency domain terms sit at top of vocab bed → fewer phases
needed to resolve them. PBM corpus = frequency data source, no separate training needed.

---

## D. Predictive Pre-Assembly

### Concept
Pre-populate LMDB with surface forms as direct entries before they're encountered.
"walked" → (walk, PAST) already in LMDB = direct vocab bed hit, no strip path needed.

### Trigger
Signal detection fires → composite profile built → select PBM subset → for top-N
frequency verbs/forms in that subset, inject predicted morpheme surface forms into LMDB.

e.g. Past tense signal + third person:
    walked, talked, said, looked, came, took, saw, knew, ... (top-N past forms)
    → inject all as pre-assembled entries at their word lengths

### Benefit
Strip logic is fallback only for unpredicted forms. High-frequency domain forms
(which appear hundreds or thousands of times in a novel) get direct hits on every
occurrence after the first. Even 60% prediction coverage on a past-tense narrative
meaningfully reduces strip operations.

### Implementation note
Injected entries go into m_vocabByLength at the appropriate length, same as the
existing tense pre-fetch injection. Priority: injected pre-assembled entries should
sit below PBM-ranked vocab (they're surface forms, not canonical roots) but above
the unranked remainder.

---

## Implementation sequence

1. **TryPrefixStrip** — new function alongside TryInflectionStrip. Slot into
   resolve loop before suffix strip. Wire synthetic injection + dep resolution.

2. **Extended DetectSignals** — add person flags to ShortPassSignal, extend
   kPast/kPresent etc. to kPerson sets. Multi-envelope activation (loop over
   applicable envelopes, inject all, additive).

3. **PBM frequency path** — new envelope query type that pulls token frequency
   from pbm_starters/pbm_documents filtered by topic. Pre-computed "pbm_all_freq"
   envelope for default case.

4. **Predictive pre-assembly** — extend multi-envelope activation to include
   surface form prediction from PBM frequency top-N.

Steps 1 and 2 are independent and can be done in parallel.
Steps 3 and 4 depend on PBM frequency query infrastructure.

---

## Open questions

- Morph bits for prefixes: bits 12-15 currently reserved. Assign prefix CLASS bits there,
  or keep prefix class as an enum in the dep-resolution result only (not stored to PBM)?
  Patrick: bits are working only; DB/token_variants = source of truth.
  → Prefix class probably doesn't need a stored bit. Concept space handles it as force.

- Topic detection: manual flag vs auto-detect from signal profile?
  Current thinking: auto-detect from composite signal; flag override available for
  explicit topic scoping (e.g. ingesting a known medical corpus).

- When to fire PBM frequency query: at Initialize() with topic hint, or mid-resolve
  same as tense pre-fetch? Probably Initialize() for the base priority, mid-resolve
  for the predicted surface form injection.
