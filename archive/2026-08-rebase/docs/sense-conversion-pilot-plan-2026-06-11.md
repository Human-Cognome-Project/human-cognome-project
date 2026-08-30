# Sense→Concept Conversion Pilot — Plan & Tests

2026-06-11 — executes the architecture settled in claims 531/532/533: senses do not belong in
the English shard; each sense is a conceptual payload to be converted to formula, compared
against defined concepts, and **cross-tagged or minted**. This pilot proves the mechanics end to
end on live data, additively (nothing destroyed or overwritten; pilot artifacts are clearly
named work tables).

## Population ladder

`311 core (65 primes + 246 molecules) → LTWF first ~2000 → outward (Longman/kaikki queue)`

LTWF is the ladder by construction — it is NSM-ordered, each ring defined in terms of the
previous, and it intersects Longman's defining vocabulary by design (Patrick 2026-06-11; LTWF
structure per claim 461). Longman gloss swaps in wherever a kaikki gloss is troublesome (531).

## Pipeline under test (the four solved pieces, composed)

```
sense (tokenized_gloss: address sequence, already exists)
  → lemma-resolve     (form addresses → base-entry addresses, via entries.form_of)
  → scaffold fold     (closed pattern set → ISA ops / slot markers / DROP; from 532 mining)
  → formula           (v0: ops bag + content lemma set — CEF §4 normal form is v1; see Scope)
  → collapse key      (deterministic over the formula)
  → compare           (key match → cross-tag skin onto existing concept; else MINT)
```

## Scope honesty (v0 vs CEF)

The v0 formula is a **payload bag** (ops multiset + content-address set), not the CEF's full
sectioned structure. This is deliberate: the pilot tests the *plumbing* (resolution, folding,
keying, compare-or-mint, idempotency) at corpus scale. Bag-collisions are a SUPERSET of true
CEF-collisions, so v0 errs toward over-merging — T2 below measures exactly that risk. Structure
(slot order, Δ parameters) tightens keys in v1; it cannot create merges v0 missed.

## Work tables (all in hcp_english, all additive, prefix `cx_`)

- `cx_scaffold` — word-address → {DROP | OP:<prime> | SLOT:<marker>}
- `cx_formula` — one row per converted sense: ops[], content[], key, provenance
- `cx_concept_pilot` — the pilot registry: key → concept id, skins (cross-tagged words), minted_from
- new seam rows tagged `relevance = 'candidate-20260611'`

## Tests (defined before the build)

| # | test | pass condition |
|---|---|---|
| P1 | Re-link coverage | ≥300/311 core tokens linked; every remaining gap individually explained (e.g. structural primes like DONT_WANT that ARE formulas and need no gloss) |
| P2 | Conversion rate | ≥95% of linked senses produce a formula with ≥1 content address |
| P3 | Collapse (cross-token) | THERE_IS/exist collide (known dup); total cross-token collisions small and every one inspectable as a real dup or a v0-bag artifact |
| P4 | Polysemy preserved | same word's distinct senses yield distinct keys in ≥90% of multi-sense words (bag over-merge measure) |
| P5 | Cross-tag | afraid/scared/terrified (non-core) convert → nearest-concept comparison ranks fear's concept #1 |
| P6 | Mint | stress emotions absent from core (hope, worry, grief, surprise…) mint NEW pilot concepts (no false cross-tag) |
| P7 | Idempotency | re-running conversion+compare changes zero rows (same key → no-op) |
| P8 | Round trip | 5 random formulas decode back to words and remain readable as the gloss's content skeleton |

P5/P6 are the live versions of CEF tests T2/T3 (claim 530) — same predictions, real corpus.

## Deliverables

Work tables populated; test report doc; graph claims; docs committed.
