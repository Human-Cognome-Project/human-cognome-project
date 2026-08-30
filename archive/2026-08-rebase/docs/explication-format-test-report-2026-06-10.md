# Explication Format — Test Suite & Run Report

2026-06-10 — executes the next step after the affect pilot (claim 529, docs/affect-shape-force-
library-2026-06-10.md). Method: define a candidate format (v0.1 = the pilot's affect_config
generalized), devise tests that can *fail*, run them, and let every failure rewrite the format.
The surviving spec is in [concept-explication-format.md](concept-explication-format.md).

Test material: 8 newly-authored stress emotions (hope, worry, jealous, proud, ashamed, grief,
surprise, disgust — none exist in core, verified), synonym variants (afraid/scared/terrified),
and 5 cross-domain concepts pulled live from hcp_core (push, decide, win, laugh + contain/INSIDE
per claim 488).

Verdict up front: **8 tests, 5 clean passes, 3 failures — all 3 failures were format bugs, not
concept bugs, and each produced a spec rule.** No test broke the underlying thesis (concept =
shape + force configuration over ISA ops).

---

## The suite

| # | test | what it would mean to fail |
|---|---|---|
| T1 | Demarcation (524) | a config that doesn't regenerate its concept without the English name |
| T2 | Synonym collapse | afraid/scared/fear authored independently → different keys |
| T3 | Discrimination | fear/worry (or sad/grief, happy/proud) → same key |
| T4 | Composition equivalence | derived authoring (base ⊕ overrides) keys differently than direct authoring |
| T5 | Axis necessity | surprise (valence-neutral) unrepresentable, or forces a fake valence |
| T6 | Shader compliance (527) | any op in any config outside the ISA floor+convenience set |
| T7 | Cross-domain | format can't hold a force-only (push), shape-only (contain), epistemic (decide), multi-agent (win), or cascade (laugh) concept |
| T8 | LoD truncation (507) | a truncated key reads as something *false* rather than something coarser |

## Runs

### T1 Demarcation — PASS (8/8)

Each stress emotion authored as a config tuple only; check = the tuple regenerates the concept's
distinguishing behavior with no name attached.

| concept | config (final form, post-fixes) | regenerates |
|---|---|---|
| hope | Δ(+, ref:WANT, anticipated) · counter: blocked-now · readout + | "wants it, can't make it certain, it may come" ✓ |
| worry | Δ(−, ref:WANT, anticipated) · aspect: sustained · readout −·mid | iterated THINK about possible bad ✓ |
| jealous | Δ(−, ref:WANT) over comparison frame (OTHER HAS X, x WANT) · locus: other · spawn: WANT(have) | ✓ |
| proud | Δ(+, ref:WANT, settled) · locus: **self** · readout + | "good happened because of what I did" ✓ |
| ashamed | Δ(−, settled) · locus: self · **evaluator: others' ToM** · readout − | shame vs guilt falls out of evaluator bit ✓ |
| grief | Δ(−, settled) · counter: **blocked-always** · readout −·hi | permanent loss ≠ sad (blocked-now) ✓ |
| surprise | Δ(±|0, **ref:THINK**, settled, magnitude hi) · valence: ∅ | pure expectation violation ✓ |
| disgust | stimulus readout − · spawn: avoid/expel | mirrors `beautiful` on the negative side; episode in affect, descriptor "disgusting" grounds into it per 467 ✓ |

Side finding: **proud = sorry's valence mirror** (settled Δ, self-locus, sign flipped) — the
composition lattice from the pilot extends.

### T2 Synonym collapse — PASS

afraid / scared / fear authored independently from their dictionary senses: all three produce
Δ(−, anticipated) · counter: blocked-now · spawn: avoid → **identical key**. *terrified* =
same key + magnitude attr (attrs are not key material). The exponent list is exactly the skin
set; surface duplication has nothing to attach to.

### T3 Discrimination — FAIL → fixed → PASS

First run: fear and worry **collided** (both: anticipated −Δ, blocked counter). v0.1 had no axis
to separate punctate-imminent from sustained-diffuse.
**Fix (spec rule R4): harvested axis `aspect` {punctual, sustained, standing}.** This also
absorbed the pilot's open "episode vs stance" question — love/jealous/trust are `standing`;
no separate class bit needed. Re-run: fear `[−, anticipated, punctual]` vs worry
`[−, anticipated, sustained]` — distinct. sad/grief separate on counter
(blocked-now vs blocked-always), happy/proud on locus (none vs self). PASS.

### T4 Composition equivalence — FAIL → fixed → PASS

- ashamed authored directly vs as `sorry ⊕ {evaluator: others-ToM}` → same expanded key ✓.
- hope authored as `fear ⊕ valence-mirror` → **key mismatch**: the spawn slot didn't mirror
  (fear spawns WANT(avoid); hope spawns nothing — flipping valence doesn't flip the spawn).
  Diagnosis: **spawned forces are derivable** from (valence × prospect × counter) — putting
  derivable structure in the key lets two correct authors diverge.
  **Fix (spec rule R2): the collapse key is the GENERATIVE MINIMUM.** Anything the walkers can
  compute (spawns, magnitudes, default inferences) is excluded from the key and computed at
  runtime — which is the shader doctrine (527) applied to the key itself.
  Re-run with spawns out of the key: hope = fear ⊕ valence-mirror exactly. PASS.

### T5 Axis necessity — FAIL → fixed → PASS

surprise has no valence; v0.1 made valence a required bit. Forcing one would be a fake.
Diagnosis was deeper than "make it optional": surprise's Δ is read against the **expectation
(THINK) graph, not the WANT graph**. **Fix (spec rule R3): Δ is parameterized by reference
graph** — `Δ(sign, ref ∈ {WANT, THINK}, prospect, magnitude)`. Valence then comes from the
WANT-graph component and is naturally absent when ref=THINK only. Bonus coverage for free:
*disappointment* = Δ(−, ref:WANT) ⊕ Δ(−, ref:THINK) (wanted AND expected, both failed) —
a concept we never authored falls out of the algebra. PASS.

### T6 Shader compliance — PASS (mechanical)

Op inventory extracted from all 13 authored configs + 6 live core explications:
WANT/DONT_WANT, THINK, KNOW, FEEL, SEE/HEAR, HAPPEN, DO, CAUSE(BECAUSE-edge), CAN, NOT, MAYBE,
VERY, SAME/OTHER, BEFORE/AFTER, FOR_SOME_TIME/MOMENT, BE_SOMEWHERE/NEAR/SIDE/TOUCH/MOVE,
THERE_IS(=EXIST), ALL/SOME. Every one resolves to the ISA floor per prime-db-functions.md.
**Zero per-concept ops; zero ops outside the ISA.**

### T7 Cross-domain — PASS (with one format generalization)

- **push**: SHAPE (participants, sides, adjacent place) + FORCE (TOUCH constraint, addForce →
  CAUSE MOVE) + READOUT ∅. Force-heavy, readout-empty: legal.
- **contain/INSIDE** (488): SHAPE only (encompass-on-axis + stance bit) + FORCE ∅. Shape-only: legal.
- **decide**: forces on the *confidence axis* settling to KNOW — addForce→settle in the epistemic
  frame, literally Sweetser's "the evidence forces the conclusion" (already flagged in 513). The
  cleanest validation in the suite: an epistemic action is the same force schema in the cognitive
  frame (490), no new machinery.
- **win**: needed multi-envelope SHAPE (competing WANT forces over a non-shareable resource;
  settle order determines per-envelope readout). Already legal — ToM envelopes are substantive
  primes; ashamed had imported one first.
- **laugh**: a **cascade** — surprise-config (Δ ref:THINK) → resolution (KNOW NOT BAD) →
  readout + motor spawn. The stored core explication contains the surprise tuple verbatim-in-
  structure. **Configs compose across concepts**, not just within affect (spec rule R5: a config
  may reference another concept's config by address).

### T8 LoD truncation — PASS (with ordering constraint)

grief truncated → [−, settled, blocked] reads "sad-like" ✓; ashamed truncated before the
evaluator bit → "sorry-like" → "sad-like" ✓; worry → "fear-like" ✓. Constraint discovered:
truncation only stays *true* if bit order = harvest-stability order (valence before locus before
evaluator). Encoded as spec rule R6.

---

## What the failures bought (summary of spec rules they produced)

| rule | source |
|---|---|
| R2 key = generative minimum; derivables excluded | T4 hope/fear mirror failure |
| R3 Δ parameterized by reference graph {WANT, THINK} | T5 surprise |
| R4 `aspect` axis {punctual, sustained, standing}; absorbs episode-vs-stance | T3 fear/worry collision |
| R5 configs compose across concepts by address | T7 laugh |
| R6 bit order = harvest-stability order | T8 |
| counter-force axis refined {available, blocked-now, blocked-always} | T1 grief |
| evaluator locus may reference a ToM envelope | T1 ashamed |

Open, carried forward: the full canonical-ordering procedure for arbitrary-size configs
(canonical-SMILES problem, 511-adjacent) is specified only for the sections and axes used here;
it needs a worked hard case (a big multi-clause social concept, e.g. *promise*) before being
trusted at scale.
