# Sense→Concept Conversion Pilot — Results

2026-06-11 — results for [sense-conversion-pilot-plan-2026-06-11.md](sense-conversion-pilot-plan-2026-06-11.md).
The pipeline ran end to end on live shard data: re-link → scaffold fold → formula → key →
compare → cross-tag/mint → idempotent re-run. **It works.** v0 (payload-bag formulas) is good
enough to seed the registry and to route the obvious cases; its known weakness (no structure)
showed up exactly where the plan predicted and nowhere else.

## Test outcomes

| # | test | result |
|---|---|---|
| P1 | Re-link coverage | **PASS** — 306/311 (was 250). The 5 gaps are exactly the multiword structural primes (A_LONG_TIME, A_SHORT_TIME, BE_SOMETHING, BE_SOMEWHERE, DONT_WANT) — composites that ARE formulas; no gloss needed. +417 candidate links, tagged `candidate-20260611`. |
| P2 | Conversion rate | **PASS** — 1,239/1,239 senses converted, 99.8% with payload (avg 1.3 concept addresses + 4.7 content lemmas per formula). |
| P3 | Cross-token collapse | **PASS** — 6 collisions total: THERE_IS/exist (the predicted known dup), hold/take "to grasp or grip" (a real shared sense), 4 degenerate empty-payload groups (excluded by min-payload≥2 rule). |
| P4 | Polysemy preserved | **PASS** — 99.5% of multi-gloss words keep distinct glosses on distinct keys (176/179 fully distinct). First run read 80% until inspection showed the "merges" were *literal duplicate gloss rows in the source data* — the key was deduplicating wiktionary against itself. |
| P5 | Cross-tag | **PASS with 1 error** — afraid, frightened, scared all cross-tag to **fear** (after broad-phase gating, fear is the *only* surviving match for each). One false positive: one worry-sense cross-tagged to `work`. One under-tag: container matched container at 1.000 but minted because the dominance rule demands a unique survivor (needs a dominant-margin clause: top ≥ 2× second). |
| P6 | Mint | **PASS** — minted: hope, worry, grief, surprise, disgust, jealous, proud (+ jar senses). This list matches the CEF stress-test predictions (claim 530) — these ARE new concepts, distinct from core. |
| P7 | Idempotency | **PASS** — full re-run inserts 0 rows (869 seeded + 27 minted, unchanged). |
| P8 | Round trip | **PASS** — e.g. fear(n) decodes to {cause, strong, danger, threat, actual, unpleasant, feeling, perceived, emotion}; push decodes to {SOMEONE, SOMETHING, force, apply, move-away}. Readable content skeletons. |

Pilot registry state: `cx_concept_pilot` = **869 core-seam concepts + 27 pilot mints**; decision
log in `cx_decisions`; all artifacts additive, `cx_`-prefixed, in hcp_english.

## Findings beyond the pass/fail

1. **Broad-phase gating (507) is what makes routing decisive, not the similarity score.**
   Ungated, fear ranked #1 for afraid at a mushy 0.182 among noise. Gated on shared concept
   addresses, fear became the *only* match. The doctrine's address-arithmetic include/exclude
   did the work; Jaccard just ranked survivors. This is the envelope effect running on real data.
2. **The ladder is real and self-enforcing.** *terrified* produced no concept addresses — its
   gloss is "extremely frightened," and *frightened* isn't a concept yet. It cannot be routed
   until frightened is minted/cross-tagged. Conversion order = dependency order = exactly why
   LTWF's ring structure is the population path (each ring defined in the previous).
3. **Source data-quality find:** the existing tokenized_gloss pass linked short/capitalized
   function words to junk entries — IN→"Indiana", OF→"Old French", FOR→"Fellowship of
   Reconciliation", UP→"Upper Peninsula". These decode as content noise, deflating similarity
   scores. The conversion queue needs a re-tokenization pass with lowercase/pos priors. This is
   the single highest-leverage v1 fix — it was degrading every comparison silently.
4. **Dedup happens at three levels for free:** identical gloss rows (P4), identical formulas
   across words (P3 — hold/take), duplicate core tokens (THERE_IS/exist, claim 532). One
   mechanism, the key, catches all three.

## v1 queue (in leverage order)

1. Re-tokenization pass (finding 3) — fixes scores everywhere.
2. Dominant-margin clause in the routing rule (fixes container; likely fixes worry→work).
3. CEF structure in the key (claim 530 normal form) — bag → sectioned formula; tightens
   discrimination; enables the structural collapse the bag can't see.
4. Transitive ladder runs: convert ring-by-ring along LTWF so terrified-class words route
   through their just-minted definienda.
5. Promote pilot tables out of hcp_english into the real registry shard once Patrick signs off
   on the layout (533 migration).
