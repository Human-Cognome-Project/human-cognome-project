# Affect: Shape & Force Library Pilot (draft, non-canonical)

2026-06-10 — pilot synthesis for the cognitive asset library (claims 526/527/528). Domain: emotion.
Sources: our own L2 affective tranche (hcp_core, 7 nodes), appraisal theory (OCC-family), Talmy
force dynamics (already converged per 513), dimensional affect models, Gärdenfors conceptual
spaces. NSM scripts are the method substrate throughout.

> Status: rough draft of the library *format*, validated against one domain. The point of the
> pilot is the format, not coverage.

---

## 1. The finding first: the data already has the target shape

The first-pass L2 explications (authored before any of this doctrine existed, and graded
content-dirty per 416) all share one form:

```
FEEL(valence) BECAUSE <condition over WANT / HAPPEN / THINK / KNOW / CAN>
```

Every affective `db_function` in core opens with "affect-space region (…axes…)" and then gives an
appraisal condition. Nobody designed that against claims 513/527/528 — the explication method
produced it on its own. The seven nodes:

| name | addr | force reading of the stored explication |
|---|---|---|
| happy | `…AB.NY` | force equilibrium: no unsettled WANT in scope (`FOR_SOME_TIME NOT THINK(OTHER thing x WANT)`) |
| angry | `…AB.NZ` | world delta opposed a force (`HAPPEN thing x DONT_WANT`) + attribution (`WANT KNOW(WHO CAUSE)`) + spawned counter-force (`WANT CAUSE(them FEEL BAD)`) |
| fear | `…AB.Na` | anticipated opposing force (`MAYBE NOW or A_SHORT_TIME AFTER`) + no interrupting constraint (`NOT CAN control`) + spawned avoidance force (`WANT BE OTHER WHERE`) |
| sad | `…AB.Nc` | force defeated, no counter-force available (`KNOW NOT(THERE_IS x CAN DO NOW change this)`) — the WANT persists, constraint-blocked |
| sorry | `…AB.Pi` | sad + self-attribution of the causal edge (`IF X HAPPEN BECAUSE something x DO`) |
| love | `…AB.Nf` | standing force complex toward X: persistent WANTs (proximity, X's good, reciprocity) |
| beautiful | `…AB.NN` | stimulus-triggered readout: `SEE(x)\|HEAR(x) → FEEL GOOD` — see §6, boundary case |

This is the evidence the format below is cut right: **an emotion is not a thing with a feature
list; it is a named region in the readout of the agent's force system.** The explications are
already generative rules (524 demarcation: each emotion reconstructs losslessly from its force
configuration). The library's job is to normalize this, not invent it.

## 2. Shape side (statics): what an affect node is

An affect node is **a region in affect state-space**, where:

- the **readout channel** already exists in the ISA: `FEEL = write_channel(affect, s)` — the
  channel is defined; this library defines the shape of `s`.
- the **axes** are harvested, not stipulated (513: implement forces, collect the recurring
  directions of change). Harvesting from our own seven explications yields:

| axis | values | attested in |
|---|---|---|
| valence | +/− | all seven (`FEEL GOOD` / `FEEL BAD`) |
| arousal / intensity | hi / lo | annotated in the stored functions (`high-arousal`, `low-arousal`); `VERY` |
| temporal orientation | prospective / present / retrospective / standing | fear (`MAYBE … AFTER`), happy (present), sad·angry·sorry (a thing `HAPPEN`-ed), love (standing) |
| counter-force availability | available / blocked / n.a. | angry (spawns one), fear+sad (`NOT CAN`), happy (n.a.) |
| attribution locus | self / other / world / none | sorry (self), angry (other), fear (other/world), happy·sad (none) |
| approach–avoid | toward / away / hold | love (toward), fear (away), happy (hold) |

  Independent sources land on the same axes — dimensional models give valence/arousal, OCC gives
  desirability/agency/prospect (= valence, locus, orientation), Talmy gives the
  agonist/antagonist/counter-force structure. Convergence of independent derivations = test
  passed, per 514.

- the **tree** (navigability, 528): the affect subtree under the registry class slice uses these
  axes as its conditional bit tree per 507, address segments in harvest-stability order
  (valence → arousal → orientation → locus). LoD = prefix depth behaves correctly here: truncate
  "grief" to `[−, lo]` and you read "sad-like" — which is exactly what a coarse pass should get.

Per the 507 criterion, only envelope-invariant axes go in address bits. *Which* specific forces,
their magnitudes, the particular WANT targets — graded, revisable — stay **edges/attrs**.

## 3. Force side (dynamics): the identity of an emotion

An emotion's collapse key is its **force configuration** — a normalized tuple over ops we already
have (nothing new in the ISA):

```
affect_config := (
  Δ:        world/goal delta            — HAPPEN vs WANT/DONT_WANT (edge: causal | BECAUSE)
  prospect: settled | anticipated       — MAYBE/AFTER scope vs perfective
  locus:    attribution of the causal edge — self | other | world | none
  counter:  available counter-force?    — CAN / NOT CAN (affordance attr)
  spawn:    forces the state emits      — new WANTs (pending addForce)
  readout:  write_channel(affect, valence·arousal)
)
```

Worked, in those terms:

- **happy** = `(Δ: none-pending, prospect: settled, locus: —, counter: —, spawn: ∅, readout: +)`
- **angry** = `(Δ: opposed, settled, locus: other, counter: sought, spawn: WANT(retaliate), readout: −·hi)`
- **fear** = `(Δ: opposed, anticipated, locus: other/world, counter: blocked, spawn: WANT(avoid), readout: −·hi)`
- **sad** = `(Δ: opposed, settled, locus: none, counter: blocked, spawn: ∅, readout: −·lo)`
- **sorry** = sad ⊕ `locus: self`
- **love** = standing force complex; not a Δ-readout but a persistent spawn-set toward a target
  (see §6)

Two doctrine points fall out:

1. **Sadness is literally 513's "strain at equilibrium":** the WANT persists, the constraint
   blocks it, the system is at rest under tension. The residual is the feeling. This is the
   envelope-effect residual (498) showing up as phenomenology.
2. **Defeasibility works unmodified:** "x lost the game" defaults toward sad by uninterrupted
   motion in affect space, and an envelope constraint ("…but x wanted to lose") interrupts it.
   No special emotion-inference machinery.

## 4. Shader compliance (527)

No per-emotion function exists anywhere above. The generic walkers evaluate the force
configuration against an agent's live WANT graph; an emotion *name* is what a configuration
collapses to when a skin is loaded. The functions called are: `addForce`, `settle`, `edge(causal)`,
`set_attr(affordance)`, `write_channel(affect)`, scope ops. All floor/convenience ISA. The asset
(affect node) is its dispatch table: its edges say which configuration pattern it names.

## 5. Registry treatment

- These seven **leave core** (493) and become registry nodes in the same first tranche as the
  contain family. Collapse key = normalized `affect_config`, NOT the English name; `happy/
  happiness`, `love/loves/loving` etc. (already in `english_exponents`) attach at the lemma seam
  as skins.
- Address bits: the §2 axis tree under the affect slice. Recommendation: **axes start as edges,
  get promoted to address bits only once stable across the domain** — promotion is a plain
  migration under 503, and harvesting-then-promoting is the 513-sanctioned order.
- Gloss doctrine check (528): each stored explication is exactly "a series of words, each carrying
  envelope data, combinatorially pointing at the region" — the gloss is the address written in the
  skin. Same config from two skins ⇒ same node; surface-form duplication has nothing to attach to.

## 6. Boundary cases the pilot surfaced (wanted: these are the test points)

- **beautiful** is not an emotion: it is a stimulus-property readout (`SEE|HEAR → FEEL GOOD`),
  i.e. a descriptor grounding in affective space per 467 — it belongs with the descriptors, with
  an edge into this subtree, not an address in it.
- **love** is not a Δ-readout: it is a **standing force complex** (a persistent spawn-set bound to
  a target). Likely a second shape-class inside affect: *episode* (happy/angry/fear/sad/sorry) vs
  *attachment/stance* (love, hate, trust…). The bit tree needs a class bit above the episode axes.
- **sorry = sad ⊕ self-locus** suggests composition works inside the domain: derived emotions as
  base-config + axis overrides. If that holds for the next tranche (guilt/shame/embarrassment as
  self-locus variants; hope as fear's valence-mirror: anticipated +Δ), the library compresses hard.

## 7. Next tranche (stress the axis set before trusting it)

hope, worry, jealous, proud, ashamed, grief, surprise, disgust — chosen to probe: valence-mirrors
(hope/fear), social-comparison forces (jealous/proud), self-locus family (ashamed vs sorry),
intensity-only variants (grief vs sad), and the hard case **surprise** (valence-neutral, pure
Δ-magnitude readout — tests whether valence is really a required bit or just a common one).

Open problems carried: normal form for config collapse keys (the canonical-SMILES problem; 511
adjacent); episode-vs-stance class bit placement; whether arousal is an axis or just force
magnitude read out.
