> **Recovered September 2026 record.** This document preserves the native-engine development/vetting context in which it was written. Absolute `/opt/project/...` paths, agent-routing instructions, and statements about branch/commit status are historical. Current paths/status are indexed in [README.md](README.md); current repository policy is in the root `AGENTS.md` and `CONTRIBUTING.md`.

# Operational plan — the field-analysis harness

**The single consolidated plan.** Patrick has covered this material across several
prior contexts; the point of this document is to record **all of it in one place**
so it stops scattering and being re-derived. It consolidates: the model, the
vetted engine reality, the current build state, and the forward build sequence.

Literal backing lives in the sibling docs and is not repeated in full here:
`HARNESS-NOTES.md` (dialogue-level literal model record), `DRIFT-AUDIT.md` (the
existing code's drift + the Unit A/B/C corrections), `VETTING-PLAN.md` (the vetted
engine facts), and the four engine-reference docs (`README`, `CONFIGURATION`,
`DEVICES`, `ENGINE-NOTES`, `api-reference/NOTES`). **This is the plan; those are the
sources.**

Status date: 2026-09-14.

---

## 0.1 Status ledger — the single source of what is settled vs open

**Rule (the one this whole plan exists to enforce):** an item here is **RESOLVED** or
**OPEN**. RESOLVED = Patrick answered it; **build on it, never re-ask it, re-open it, or
invent around it.** The moment Patrick answers an OPEN item, it moves to RESOLVED with
his answer. Do not carry a resolved item as open.

**RESOLVED — settled; build on these:**

| Item | Resolution |
|---|---|
| The law | `m1·m2/d²` to the field's virtual particle; no exceptions |
| d=0 gate | exact branchless `sgn(d²)`, v=0 at coincidence — **SHIPPED (Unit A)** |
| Rotation | no running spin; `(share, offset)` kept, off-centre share translates — **SHIPPED (Unit B)** |
| Brake target | destination = sum of the `m1·m2/d²` vectors = the resultant; reach = its magnitude — **SHIPPED (Unit C)** |
| Momentum | carries between ticks (inertial, second-order) |
| Sphere | diameter 1, unit, no per-sphere constants |
| Amalgamation | end-of-tick centroids ARE next tick's m2; force centroid == mass centroid (invariant) |
| Universal centroid | one always-active field effect, **pegged to origin (0,0,0) by definition** (the single exception to computed centroids), mass = **total construct mass read as the TOP aggregation, each element once** (NOT the sum of nested/field centroids — that compounds mass); its orbital field pulls things into relative alignment; dampens runaway kinetics; establishes a locus of observation distinct from the commonality focal point |
| Root of analysis / focus | the locus of observation, **placed or travelled relative to the universal centroid**; LoD aggregation established by **neighbourhood** of the root (near granular, far aggregated), demand-driven — mechanism settled, CPU-resident implementation is future |
| Telos: complete stasis | the system's goal is **complete stasis** — every element has an **exact place it belongs**; orbital/other motion is the **symptom of under-constraint**, suppressed as more fields pin positioning exactly; full placement = complete analysis = rest |
| Motion = analytical signal | any motion means a component **occupies more than one theoretical position** → investigate it for the **commonality** (the shared, often uncalibrated dimension pulling it); how the system surfaces the not-yet-explicable |
| Testing needs complexity | invariant relationships / real settling can only be tested with a **complex interconnected field structure**; current unit tests validate **mechanics only** — green unit tests ≠ validated model |
| Phasing / 'subspace' level | now: base mechanics at the **single base 'subspace' LoD**, so progressively larger multi-LoD structures compose from it; **next major phase (once solid): the mathematical viewport into the construct** (§3.3a observation mechanism) |
| Particle pool | fixed `N` particles sized to the card, **allocated once**; unused = `0x`/null hex (no mass/presence/location), naturally inert (mass 0); claim/release as LoD needs change **converts allocation into declaration in the working space** — no live allocation; centroid virtual particles + LoD orbs drawn from it |
| SNode tree = relative-LoD-by-perspective | pool → claim/release → **SNode tree** → viewport; the tree is the mechanic that **exposes LoD relative to the observation perspective** (near = fine, far = coarse), recomposing as the perspective travels — renders, does not store, a fixed hierarchy |
| DB path-agnostic / SNode slices per focus | main DB **explicit, favours no path**; composed SNode tree **slices the data into the aggregation shape for the current focus**; **time is just a dimension** — temporal analysis = the SNode tree rolling up the time axis, same mechanism, not special-cased |
| Participation | parents = predicates (mass ratio, offset); siblings = full-mass by kind; mass = force |
| Siblings | also carry the base definitions (race/class) that configure body + gear |
| Force activation | scalar launch argument; **NO json / config surface** |
| Force set | Patrick knows many; set up when the DB connections are established |
| Discovery | the **system's** job — surface more than is currently explicable |
| The DB | pairwise discovery tax ledger; combinatoric control via centroid aggregation; manual-seed now → self-discovery as density grows |
| DB scope | storage → working → resident (selected twice) |
| Sibling vs SNode rollup | a *selected* rollup; selection policy in the routines, not the controller |
| DB incorporation | governed by instance flexibility (capacity budget + restart authority) |
| Co-structure | = reduction on a plane; bond strength IS the analytical measurement |
| Corona (what it is) | separator shell ~0.5; analytical separation + defined contact distances; NOT a brake or force term |
| Polarity (what it is) | parent-predicated, from component numbering; reversed (like faces like, mirrored); meters closeness, not attraction |
| Three scenarios (behaviour) | defined = invariant identity / body-parts; overlap = strong; abut = loose aggregation of like kinds |
| Kinetic contact | simple force transfer; separate, needs nothing special |

**Corona effect — EXPLAINED and SHIPPED (2026-09-14).** The corona separator is built and
verified (CPU + CUDA green, kernel read directly): regime-gated standoff (`d0` = flush for
aligned/defined, `1 + 2·corona` for opposed), a finite separator push resisting closer
approach, and the **defined-only invariant hold** (shear-resistant). The field provides the
attraction; the corona is *just the separator*. Radius/stiffnesses are trial values to
calibrate.

**Universal centroid — EXPLAINED and SHIPPED (2026-09-14).** The inward `1/d²` pull toward
the pegged origin (0,0,0) is **suspended** (kept, not deleted — may be reactivated) and
**converted to a total-mass radial velocity dampening** in the integrate step (bounds outward
runaway, inert on the settled bulk; §3.3a). Built + validated: CPU + CUDA, 57/57. The total
mass `M` is read as the **top aggregation, each element once** (not the compounded sum of
field centroids). Dampening form is a **draft flagged for Patrick's review**; acceptance =
minimal disturbance + outlier returns in reasonable time (calibrate `reach_strength`).

**OPEN — genuinely awaiting Patrick:** none right now.

**Mine (translation, minor):** payload-shape confirmation; the trial constants — corona
radius (0.5 vs 1), separator/hold stiffnesses, and `reach_strength` — to calibrate with
Patrick.

---

## 1. Scope, layering, working rules

**One specific implementation.** The updated engine and this harness serve one
model — n-dimensional field *analysis* expressed in 3 Newtonian dimensions. Not a
general-purpose Taichi wrapper; any benefit others take is incidental.

**Scalable in form to all card sizes AND architectures (Patrick, 2026-09-14).** The design
must scale, *in form*, to any card size (e-waste → large) and any architecture — TaiChi is the
engine **precisely because it is vendor- and driver-agnostic by intent.** So every mechanism,
and any engine-source change (e.g. the i32→i64 widening, §3.11), must stay
**architecture-neutral** — working across TaiChi's backends (LLVM CPU, CUDA, AMD/Vulkan/Metal/…),
never a vendor- or driver-specific hack — and adapt by form, not per-card rewrites. (The
specific *model* on *universal* hardware; not in tension.) **Individual-hardware caveat:**
"scalable in form" does not mean every card works out of the box — specific hardware quirks
(e.g. the **GTX 750 Ti**'s refused first field allocation) are **fine to defer and resolve
later at the individual hardware-configuration level**; they do not block the general form.
The 750 Ti **reports full 64-bit compliance** but throws a **driver/compilation error specific
to that hardware family** (Maxwell / cc-5.0), most likely on the async-alloc path
(`cuMemAllocAsync`) — a per-family quirk, not a real capability gap; resolvable per-config
later (e.g. a non-async path for that family). **It may in fact be the engine's 64/32-bit
mix** (64-bit byte sizes + host pointers, but 32-bit logical indices — the partial
modernization) throwing on that family — in which case **completing the i32→i64 widening
(uniform 64-bit, §3.11 workstream) could resolve the 750 Ti as well**, i.e. the deferred quirk
and the i32 workstream may be the same issue.

**Three layers, one seam:**
- **Engine** — a fixed, built artifact (`engine_support`, from `/opt/project/taichi`).
  Pure mechanism: starts an instance, allocates fields, JIT-compiles and launches
  kernels, moves data across the host boundary (LLVM CPU + GTX 1070). The engine
  binary is **not** recompiled to change formulas.
- **Harness** — a separate C++ target linking `engine_support`. Feeds the engine the
  formulas and shapes (few functions, one operative shape, written directly in C++,
  no configurable formula surface). Owns the instance lifecycle and the authority to
  **shut down / restart** when the structure must be fundamentally updated.
- **Routines / user API** — drive *within* the designed structure via arguments;
  never redefine it. This is where analysis *selection policy* lives (see §3.8, §3.9).

**Working rules for this effort:**
- Examine only `/opt/project/repo/engine` and subfolders. Documents only; code
  changes are dispatched to agents on Patrick's instruction.
- **Agent model policy:** investigation on **Opus 4.8** (`opus-4-8`); coding on
  **Sonnet** (`sonnet`) — small, linear, fully-specified units, each validated
  (tests + review) before it is trusted. **Opus 5 / Fable 5 banned.**
- Canadian English. Tests on everything. Verify any file:line/behaviour claim
  against live code before asserting.
- **No single locus of truth; on conflict, stop and confirm with Patrick.** Absorb
  corrections **narrowly** — do not careen one correction into a sweeping
  restructuring or declare other parts "redundant." Patrick's explanation governs;
  do not re-open what he has already defined, and do not re-ask what is answered.

---

## 2. The engine reality (vetted — see `VETTING-PLAN.md` for evidence)

**The rate-of-change seam (P1.1–P1.4, confirmed):**
- Startup **capacities are frozen at instantiation** (`CompileConfig` copied into
  `Program` at construction). Changing them needs a fresh instance.
- **Only a layout (SNode-tree) change forces recompilation.** A loop bound, a scalar,
  or an array is a **launch argument** — so a force-activation flag is a scalar arg,
  no recompile.
- **Kernel name is the compilation key** — same name ⇒ shared compilation ⇒ the
  second silently runs the first's code. Every compiled kernel needs a distinct name.
- **Live `add_snode_tree`** materializes a new tree on the live program; **tree ids
  recycle**, but per-node **SNode ids are monotonic across destroyed layouts** within
  a Program's life. ⇒ repeated recomposition spends the id budget; budget generously
  up front or use the **restart authority** to reclaim it.

**The "64-bit" premise, reconciled:** logical indices are still 32-bit; the real win
is that the **default 1024 SNode-capacity ceiling is lifted** and replaced by
**configurable startup capacities**, honoured at compile + load and verified
end-to-end, plus a real paged `ListManager` directory. Dense i32/f32 on a ~12 GB card
is bounded by **card memory**, not fixed metadata. **Suffices with configuration** for
the MVP. Target run card: **RTX 3060** elsewhere; the **GTX 1070** is local dev.

**Hard constraints (break these and the run aborts/corrupts):**
- The outermost loop of a kernel must carry a thread count (`runtime.cpu_threads()`).
- `TI_LIB_DIR` mandatory; runtime ABI = 1; offline cache off (recompiled in-process).
- CUDA binds visible-ordinal-zero; select with `CUDA_VISIBLE_DEVICES` before CUDA
  starts.

**Cost model (Patrick's three rules): declaration < calculation < allocation.**
Allocate once up front and reuse (fixed pool, 32 MiB granularity); keep data resident,
don't re-gather; lean on launch arguments; minimise touches in kernels (prepaid
centroids reduce a group's pairwise touches to one).

**Envelope (1070, measured):** 7.84 GiB free; 4 bytes per i32 cell; f64 ≈ 4× f32
bandwidth-bound, ≈ 27× arithmetic-bound; pool fixed once at instantiation is the
intended pattern. Capacity is division: `pool bytes / (slots per particle × 4)`
particles.

---

## 3. The model (the operative thing being built)

All of §3 is **Patrick's literal model**, not analogy. It all stays inside **one
law**; everything is the same simple math with the **derivative products used
correctly** — the analytical readouts (destination, reach, bond significance,
reducibility) fall out as **byproducts of the one field calculation**, never as
parallel machinery.

### 3.1 Foundation
N-dimensional field analysis expressed in 3 Newtonian dimensions. **A dimension is
any plane of commonality between ≥2 points** — defined relationally, not as a
pre-given axis. "n-dimensional" = as many dimensions as there are planes of
commonality; "expressed in 3 Newtonian dimensions" = rendered in ordinary 3-space
(the expression layer), while the analysis is n-dimensional over the commonalities.

### 3.2 Geometry and level of detail
Spatially 3D; the sole shape is the **perfect sphere (orb)**, **diameter 1** (radius
0.5), no per-sphere size parameter. At the LoD where an object is *primary* it **is
exactly one particle = one sphere**. Composition is recursive (a particle may contain
any number of inner particles); at the **bottom active LoD everything is an orb**.
LoD is a hierarchy of nested spheres; "primary" is level-relative.

### 3.3 The one law — and the universal field effect
**Every interaction is `m1 · m2 / d²`**, one law without exception. **m2 is an
aggregate**: the cumulative mass at the **force centroid** of the field (all members
of the dimension being calculated), carried by a **virtual particle** (§3.5). `d` =
distance from m1 to that virtual particle. **Like-attracts-like is the universal
field effect the whole system runs** — every like kind pulling toward its own,
everywhere. Everything in §3.8 modulates only *how close* that universal attraction
can bring two particles; none of it is a separate attraction.

### 3.3a The universal centroid (the one always-active field effect)
The engine incorporates **one field effect that is always active: the universal
centroid.** It is **pegged to the origin (0,0,0) by definition** — its position is **not
computed**, it is fixed at 0,0,0 — and its mass is the **total mass of the construct.**
That mass, pulling from the origin by the one law (`m1·m2/d²`, target = the origin), draws
everything into **relative alignment within its orbital field.** Two jobs: it gathers
everything toward a single **calculable focal reference point** — the origin — and it
**dampens runaway kinetics**, a global attractor that bounds the system and organizes the
relative configuration.

**Two dampenings, distinct (Patrick, 2026-09-14):** "runaway" means escape toward
**unbounded space** — the universal centroid dampens *that* by keeping everything gathered
so nothing flies to infinity. It is about **boundedness, not speed.** **Speed** is dampened
by the **brake** (§3.7), separately, as a particle nears its **target coordinates**. High
transient infall speed is fine; the invariant the universal centroid guarantees is that
positions stay bounded (controlled Lyapunov oscillation, not escape).

**Discretization divergence (Patrick, 2026-09-14):** two backends (or two tick sizes) may
**diverge** over a long chaotic run — the effect of **discretization slowing settling**. A
coarser tick takes **more ticks to settle**, but **in time both settle into IDENTICAL
RELATIVE ALIGNMENT, even though the exact positions (and the settling time) diverge.** The
relative configuration is the invariant; absolute position is not. It scales with tick
coarseness and must stay **limited** (bounded), not blow up.

**Test invariant that follows:** cross-backend / cross-tick agreement is checked on the
**settled relative structure** (e.g. the relative configuration once settled), never on
absolute positions or same-tick states. Stability asserts: positions stay **bounded** (no
escape) and both settle to the **same relative alignment**.

**This is the single exception** to "a centroid is computed from its members" (§3.5): the
universal centroid's position is **pegged, not derived**. (Patrick flagged this as the one
exception he had skipped past.)

Its deeper purpose: a stable, fixed global reference **establishes a current locus of
observation that is distinct from the mathematical focal point of commonality.** The
commonality focal point is a field's centroid (m2) — *what* is being analyzed; the locus
of observation is where the analysis observes *from*. Without a fixed global anchor the
two collapse together; the pegged universal centroid supplies the frame that keeps the
observer position separate from the (moving, computed) commonality centroids.

From that established frame, the **root of analysis** (the locus of observation) can be
**placed or travelled relative to the universal centroid**, and **LoD aggregation is then
established by neighbourhood** relative to that root: near the root, elements resolve to
granular orbs; farther out, they aggregate to their centroids — demand-driven and
focus-relative (§3.9). Travelling the root moves the analysis through the construct, and
the neighbourhood LoD tracks with it. (The mechanism is settled; its CPU-resident
implementation is part of the upper half — §5.)

Translation (Patrick, 2026-09-14): a **virtual particle pegged at (0,0,0)**, **included in
every particle's calculation as a valid centroid** — one more m2 in the `m1·m2/d²` sum,
target fixed at the origin. Position is a constant.

**Mass — corrected 2026-09-14 (compounding bug):** the mass is the **total mass of the
construct, read as the TOP aggregation — each element counted exactly once.** It is NOT the
sum of every field/group centroid mass: a particle participates in many fields, so summing
those **compounds** its mass across all of them (the first implementation did this and
over-pulled). Read the top aggregation as the entire group, not the nested centroids
beneath it. Flat implementation = Σ particle mass (each once); the LoD implementation reads
the **top-level rollup node's mass**, never the nested centroids.

**Near-origin caveat (Patrick, 2026-09-14):** the pull is `1/d²` toward the origin, so a
particle sitting very close to true 0 takes a huge (finite) force. The exact `sgn(d²)` d=0
gate covers exact coincidence (zero force there), and the brake + the universal centroid's
own kinetic dampening bound the step — but a near-origin transient is a real **early**
risk. Once data density reaches any balance nothing practically occupies that spot, so it
becomes a non-issue. **No epsilon/softening is added for it** (that would break the exact
law); it is an accepted, self-resolving early-transient.

**Flyaway → total-mass radial dampening (Patrick, 2026-09-14 — CONFIRMED, build now).** The
`1/d²` universal pull weakens with distance, so a high-energy particle can escape (flyaway).
Flyaway is a **math artifact** (§3.10), and the moderation is a **conversion of the universal
centroid**:

- **Suspend the universal centroid's inward `1/d²` pull — suspend, do NOT remove** (keep the
  kernel; it may be re-added later — activation is a toggle, the formula stays). This also
  **eliminates the current double-compression artifact**: when the universal centroid and the
  largest group are identical (as in the ring), the mass-centroid pull was applied twice.
- **Add a total-mass radial velocity dampening**, pegged at 0,0,0: a **multiplicative
  dampening on the *outward* radial velocity component** (the brake's own trick, applied to
  the radial-from-origin component). ≈1 inside the reach (**bulk untouched — relative physics
  retained**), falling off as a particle tries to leave, **bleeding off escape velocity.** It
  is a velocity dampening, **not an added inward force**, so it does not double-compress.
- **Reach = the extent of what the total mass can exert as a field**, self-scaling with `M`.
  Beyond it an outlier's escape is damped and it **acquires inward motion from the
  residual/real fields** — exactly as any complex system reels an outlier back. Keeps `M`
  (still summed in the clear pass) for the reach scale.
- Exact algebraic form (the dampening function + reach threshold) is a **draft, flagged for
  Patrick's review**; validate that the ring stays bounded and fluid with the bulk's relative
  structure intact and no NaN.
- **Secondary boundary (Patrick, 2026-09-14):** in addition to the field-strength reach, the
  effect has a **fixed geometric boundary — the volumetric space that all pool particles, if
  active, would occupy packed together as one sphere.** It is set by the pool's particle count
  `N` (the packed radius follows from the total particle volume: `N` unit spheres → radius
  ≈ `(N/8)^(1/3)` at ideal packing), so it is fixed, not mass-dynamic. **This is the *same*
  sphere that sets the instance's particle budget** — the same `N` that sizes the pool
  (§3.11), card-sized. So the boundary is determined by the instance's particle budget and
  **self-scales with instance capacity** (bigger budget → bigger pool → bigger boundary;
  §3.9). **It operates as the *outer* boundary of the mass effect — the real edge (Patrick,
  2026-09-14).** The field-strength reach is the **calculated** edge; **regardless of the
  calculated edge, the real edge is reality** — nothing extends past the physical space the
  particles actually occupy. So the packing sphere is the **hard outer cap**, and the
  field-strength reach is the softer edge within it.
- **Growth relationship (Patrick, 2026-09-14):** the softer (field-strength) edge is what
  binds **until the construct is large enough to occupy the full reach** (the packing sphere).
  **Right now the active mass is quite limited inside a large potential space**, so the soft
  edge is operative; as the construct fills out toward its budget it approaches the hard cap.
  The **potential space** (the packing sphere of the full particle budget `N`) **must be
  calculated per card**, since `N` depends on the card's capacity (§2/DEVICES, §3.11 pool).
  Budget-probe numbers: the packing radius is **~60–180 particle-diameters** depending on
  payload width (≈63 @ 1024 slots, ≈100 @ 256, ≈158 @ 64, on the 1070; similar on the 3060 —
  see §3.11 budget-probe results). So the hard outer edge sits tens-to-low-hundreds of
  diameters out — a large potential space relative to the currently-sparse active mass.
- **Acceptance criterion (Patrick, 2026-09-14): single-tick capture is NOT required.** The
  form is intentionally **position-based** (it bleeds an escapee's outward speed over
  subsequent ticks, not the jump itself) because the goal is **minimal disturbance to the
  proper physics** — inert on the settled bulk, only reeling in an outlier. A single violent
  close-encounter kick carrying a particle far in one tick is fine **so long as the outlier
  returns in reasonable time**; that (tunable via `reach_strength`), not one-tick capture, is
  what to confirm. **BUILT + validated 2026-09-14** (`unit-dampen`): universal pull suspended
  (kept), dampening in integrate, 57/57 cpu+cuda; typical ring excursion improved ~4300→~1521,
  one cuda outlier ~38028 — expected first-cut behaviour, return-time to calibrate.

**General principle (Patrick, 2026-09-14):** near-coincidence applies *to a degree* to
**every** centroid, not just the universal one — but the **more complex the member
assignments, the less likely it is to occur in the math** (a multi-member centroid sits
between its members, not on any single one, so complexity makes it self-avoiding). The
exact `sgn(d²)` gate is the sole and sufficient handling everywhere; the universal centroid
is only singled out because, being pegged at a fixed point, it is the most exposed early
before balance.

### 3.4 Participation — Parent and Sibling
The particle collapses to a **position + a set of field listings**, each carrying a
**participating mass (= its force in that field)**:
- **Parent relationship** — the **predicates**: internal predicate components,
  **partial by mass ratio**, tied to the particle's inner components; the field acts
  on the component's mass fraction and the **whole body responds in proportion**
  (magnet-in-non-magnetic-material). A component can sit **off-centre**; its pull is
  felt there while the whole body **translates** (no rotation — see §3.7/Unit B). The
  ordered positions of parent components are the polarity source (§3.8).
- **Sibling relationship** — full, equal participation by the **whole body**, keyed by
  **group name (kind)**. Does two jobs (§3.8): the base race/class **definitions** and
  the **kind** for emergent bonds.
- **Mapping (resolved):** parents = predicates; **every listing is a field**;
  **the participating mass is the force** (not a separate datum).

### 3.5 Virtual particles / amalgamation = next-tick setup
At the **end of every tick**, for each field at play, compute the **force centroid**
(position) and **effective mass**, and assign them to a **virtual particle** (no real
mass/presence, carries the group's effective mass). **That virtual particle is the m2
the next tick reads** — the loop **closes on its own tail**. It runs at tick end
because all factors are **still resident** (reuse hot data, no re-gather, doesn't gate
the next launch). **Invariant: force centroid == mass centroid, always** (mass = force;
divergence is a defect) — a natural test point. The centroid **prepays the pairwise
tax faithfully**, because a full pairwise sum of a group's field interactions resolves,
by inverse-square superposition, to the relationship with the centroid — the centroid
**is the end structure the group develops into**.

### 3.6 Units and the per-tick move
**1:1 ratio: 1 force moves a 1-mass particle 1 space in 1 tick.** Per-tick move =
`x += F_net / m`. **Momentum carries between ticks** (inertial / second-order, no
decay). **Superposition: the motion vector is the vector sum of all vectors acting on
the particle** (plus carried momentum) — the same linearity that collapses a group's
pairwise sum to its centroid. No special per-force ratios; combined forces can present
emergent variance.

### 3.7 The brake and the d=0 gate (physics core — DONE, Units A–C)

> **2026-09-25 review:** Units A–C are built, but the destination/force
> composition and brake timing below are under review. The current
> `exp(-(proposed_travel/|F_net|)^4)` brake attenuates before the proposed
> step reaches `|F_net|`; for example, at 80% of that reach its multiplier
> is about 0.66. Patrick clarified that the model needs both an effective
> destination and motive force and that the brake corrects discretization
> overshoot. Slowing on approach can be correct if later ticks settle to the
> effective centroid; current tests do not establish exact convergence. See
> [the working record](../../docs/napier-system-discussion.md)
> before treating the old destination equivalence as a final design rule.

- **d=0 gate (exact, in the equation):** if d=0, v=0 — covers a sole-member centroid
  and a particle on the centroid. Branchless exact zeroing via `sgn(d²)` (1 for d²>0,
  0 at coincidence). **Not** epsilon-softening. **Shipped (Unit A).**
- **No running rotation:** rigid-body spin was drift; stripped. The `(share, offset)`
  edge shape is kept — an off-centre share **translates** the whole body. **Shipped
  (Unit B).** (The ordered-line "rotation" is the alignment expression of §3.8's
  composite regime, not conserved angular momentum.)
- **The brake (discretization / overshoot correction):** the destination is the **sum
  of the `m1·m2/d²` vectors = the resultant**; the brake's **reach = the magnitude of
  that resultant** (the total distance being sought); the brake is **exponential to
  that distance**, so it is **≈1 in any tick that does not overshoot** and bites only
  on a would-be overshoot. Branchless; goes to unity in the small-step limit; touches
  only the discretization artifact, not the physics. The drifted second (weighted)
  accumulator was removed. **Shipped (Unit C).**

### 3.8 Contact and bonding (Patrick, 2026-09-14 — IN PROGRESS)

> **The corona effect is EXPLAINED and being implemented in C++ (code agent).** The
> corona is *just a separator*: the **regime-gated standoff** the bond is suspended at.
> Like-attraction is the field effect (already built); the corona meters how close that
> pull brings two like particles — aligned polarity → coronas overlap, standoff collapses
> to flush → strong; opposed → coronas abut at the corona distance, loose and breakable;
> defined → suspended (flush) and invariantly held (shear-resistant). "Kinetic contact"
> (simple force transfer) is the separate simple case.

This is the **analytical form of the data expressed as contact.** Three
contact/bonding scenarios (not counting **kinetic contact**, which is simple force
transfer and needs no special treatment).

**The corona — the separator.** A shell around each particle, **~0.5 particles** thick
(trial 0.5, may go to 1). Purpose: **analytical separation and defined contact
distances** — free particles meet corona-to-corona, so the corona sets the standoff,
not the bodies. (Could act as a spin dampener, but spin isn't conserved, so not
required.) Its size tunes how tightly type-3 aggregation holds. **It is not a brake
term** — the degraded code buried a `corona` epsilon in the brake ratio, where it
distorted the overshoot correction; Unit C removed it there. The number 0.5 was right,
the placement was wrong; its true home is here.

**Positional alignment — the polarity (parent-predicated).** A **polarity-like effect
derived from component numbering** of the ordered chain, not a spatial quantity. Any
construct **above a single hex value is always a composite**, so it has ordered ends:
**position 1 = negative end**, top position = positive end. The rule is **reversed
from ordinary polarity: like faces like (mirrored)** — negative to negative, positive
to positive; the chains join as **mirror images** at like poles. The **whole construct
must mirror-match** to connect as one piece; where it doesn't, only the matching
stretch connects. **Polarity does one narrow job: it meters how close the universal
like-attraction can bring two particles** — it is not itself an attraction.

**The three scenarios:**
1. **Defined bond — the invariant identity bond.** A **read of a cell's parent
   content from the DB**; the ordered chain (beads on a string) that is *what makes the
   composite what it is* (Patrick's analogy: **body parts, not gear**). Two LoD
   regimes: **as a composite (higher LoD)** it is a straight line, beads flush, no
   gaps, and a pull on part moves the whole as **rotation + translation** (ordered-line
   reorientation, **not** rigid-body spin); **at primary LoD** the beads stay connected
   but the joints go **flexible** — trialled as the two faces **sliding directly, 
   frictionless**, corona **suspended** across the bonded contact. It is an **invariant
   connection**, so it needs its **own definition and a stronger gate**: it can be
   **subject to shearing** and the gate must hold it in its defined order through
   differential forces. **This is the one genuinely special definition** — it does not
   emerge from proximity; it is declared standing structure.
2. **Overlap bond — kind + aligned (mirror-matched) polarity.** Coronas **overlap**,
   the operative distance closes, and the one law delivers a bond that **climbs faster
   than distance alone would imply** (informally "exponential"; still `m1·m2/d²`,
   amplified by the closing `d`). Strong, graded by overlap depth.
3. **Abut aggregation — kind + opposing polarity (or any non-bonding pair).** Coronas
   **abut, no overlap**: **general aggregation of like kinds**, holding unless
   **stronger surrounding forces** pull or push it free. Corona size tunes the hold.

**Parent and Sibling, expanded.** Parent content = the invariant defined bonds (the
parts). **Siblings do two jobs:** (a) the **base definitions the body relies on** — the
race → subrace → class categorical groupings that **gather and configure** which body
parts and gear a composite has (an "elf" = race + subdivisions + connected classes,
collectively defining body + gear); and (b) the **kind** governing emergent bonds. A
composite's identity = its invariant parent chains **selected and arranged by** its
sibling categories, with emergent bonds keyed on those same categories.

**Co-structure is reduction (the analytical payoff).** Two elements that form a
**significant co-structure are reducible on that plane** — they behave as one on that
dimension, so analysis collapses them to a **single representative (their centroid /
virtual particle)** there. Same reduction as the §3.5 roll-up. **Bond strength IS an
analytical measurement** — how reducible two elements are on a plane — and a
co-structure significant/relevant enough is what earns **promotion** into the model
(§3.9). The physics and the analysis are the same act.

### 3.9 Storage vs working DB, selection, and promotion
**What the DB is: a pairwise discovery tax ledger.** The main DB **ledgers each
discovered pairwise cross-connection once** — it pays the "discovery tax" a single time
and stores it, so the cost is never re-paid. This is the mechanism of **combinatoric
control of mass datasets**: rather than recomputing O(n²) pairwise, analysis **aggregates
the ledgered pairs into effective centroids for anything analyzed from any angle**. It is
the same relationship as "the centroid prepays the pairwise tax faithfully" (§3.5) — the
ledger is where the paid tax lives, and the centroid is the aggregate of it.

**The analytical goal, and the ledger's trajectory.** Known dimensions/forces are
**declared in the DB** (Patrick knows many; set up when the DB connections are
established). The system's purpose is to **discover MORE than is currently explicable** —
the **uncalibrated** dimensions and emergent co-structures the running analysis surfaces.
These are the two ends of the *same ledger filling in*, not two systems:
- **Now (the DB is just being rooted):** many connections are **entered manually** to lay
  a **base geometry** and reuse work already done, rather than rediscovering it.
- **As data density increases:** the system turns **more analytical** — it **identifies
  and encodes the pairwise cross-connections itself**, which then aggregate into centroids
  for any-angle analysis. Manual seeding gives way to self-discovery; discovery is the
  system's function (carried by virtual nodes, promoted when significant — see below).

**Three tiers of scope:**
1. **Storage DB** — the entire content library: the **flat, `token_id`-keyed, arrayed**
   ledger of *all* known relationships. A DB entry = a particle's data {predicates,
   fields, forces} **+ child listings** ("this appears in these constructs, as a whole
   unit") that make the store **n-dimensionally traversable by CPU threads**. (Ubisoft's
   whole library.)
2. **Working DB** — the composed library for **one analysis** ("one game"): a **selected
   subset** of storage relevant to the current analysis. This is the composed-tree /
   selected-rollup scope; **the selection policy is the routines' instruction set**, not
   baked into the controller.
3. **Resident subset** — even the working DB **loads as needed in subsets**: the portion
   streamed into instance/device residency for the current **focus / neighbourhood** —
   what the SNode rollup and device pool actually hold at a moment.

Selection happens **twice** (storage → working, working → resident), which is why the
**CPU follows, never searches** (it follows child listings / orderings the instructions
name — no scan), why the cost model only pays for resident subsets, and why **instance
flexibility governs the ceiling** (§2: capacity budget frozen at instantiation +
monotonic node-id spend + restart authority + device memory). **How fully the DB can be
incorporated is a function of how flexible any one instance can be.**

**Sibling categories vs the SNode rollup:** they **may overlap, and in any one construct
can and should be made to coincide**, but the DB holds far more connections than an SNode
rollup can carry as a simple projection — hence *selected*, not mirrored.

**The DB is path-agnostic; the SNode tree slices per focus (Patrick, 2026-09-14).** The main
DB is **explicit in every way and favours no one path** — the flat, complete store, no
privileged traversal or aggregation. The **composed SNode tree slices the data into the
shape desired for the current focus of aggregation** (the selected rollup). And **time is
just another dimension** (a plane of commonality, §3.1): when **temporality is a dimension
being considered, the SNode tree's rollup of time is the relevant shape** — the *same*
slicing mechanism applied to the time axis. Temporal analysis is **not special-cased**; it
is the SNode tree rolling up the time dimension like any other. **The model becomes a model
across time simply by including time in the face of the model** (Patrick, 2026-09-14) — the
composed/exposed set of dimensions. No temporal machinery is added; the same
field-balancing, rollup, and centroid mechanics operate across the time axis as across any
other. Time in the face → the model spans time.

**Two rates of change:**
- **Fast, live, no restart:** which **forces are active** for an aspect of analysis — a
  scalar **launch argument**. The **formula is fixed** (hardcoded C++); only *activation*
  is manipulable.
- **Slow, structural, rebuild:** a virtual node accumulating enough **relevance/
  significance** graduates from the ledger into the resident SNode model → **SNode
  reconstruction** via the harness's restart/reclaim authority.

### 3.10 The telos: complete stasis (Patrick, 2026-09-14)
The **goal (telos) of the system is complete stasis** — because **every element has an
exact place it belongs.** The dynamics exist to converge there.

- **Orbital patterns and other motions are not features — they are the symptom of
  under-constraint.** They are the leftover *variation* where too few fields are pinning an
  element down.
- **As more fields come into play, positioning is more exactly controlled and that
  variation is suppressed** — the motion dies toward stasis. A fully-constrained element
  (all its fields in play) sits at its exact place with no net force: rest.
- So a lively/orbital run is a **sparse-field regime**, not the end state. This is the
  settling goal made literal; the brake (§3.7) only eases the discretization en route, and
  "settle to identical relative alignment" (§3.3a) is, at the limit, this stasis
  configuration.
- Ties to the analytical purpose (§3.9): stasis is reached when every element's place is
  determined — i.e. when the analysis has resolved where everything belongs. **Full
  placement = complete analysis = stasis.**

**Motion is the analytical signal (Patrick, 2026-09-14).** In the analytical use, **any
motion is a signal that a component occupies more than one theoretical position** — it is
pulled toward several places because it shares commonality across them and has not resolved
to its one exact place. Such a component **is to be investigated for the commonality** — the
shared, often **uncalibrated** dimension causing it. This is how the system **surfaces more
than is currently explicable** (§3.9): where an element will not settle, an undiscovered
commonality is pulling it. Stasis = every commonality resolved.

**Symmetric degeneracy → fluid orbit is expected (Patrick, 2026-09-14).** A stable-ish
lattice / ring orbit (as in the 64-particle test ring) is **expected physics**, not a
defect: there are **nominal points of balance**, but where **no element has a privileged
position beyond its distance from the core**, the elements are interchangeable and **float
fluidly between the balance points** rather than each settling into one. Stasis needs each
element to have an *exact* place; perfect symmetry denies that, so the symmetric case is
correctly **fluid, not settled.** This is exactly why the test ring does not settle — a
symmetric degenerate fixture, i.e. a **mechanics** test; a settling test needs distinguishing
fields that give each element a unique place.

**Governing principle (Patrick, 2026-09-14) — instability vs artifact.** Instability *where
commonality is undefined* is expected: **bounded** fluid motion between nominal balance
points is the correct analytical signal that nothing pins the element (§3.10 extreme). But
**runaways / explosions are artifacts of *our math*** (discretization, `1/d²`), **not real
effects** — and the job is to **moderate the math artifacts while retaining the relative
nature of the physics** (the relative configuration is the content; suppress only the
artifact, never distort the relative structure — the same "physics as an analytical tool"
license as the brake). So bounded fluid instability is *kept*; a runaway is *moderated* by
the **radial push-back (§3.3a)**, which must leave the settled bulk's relative config
untouched and only reel back an escapee. **Build it when the current path produces runaways**
(the test ring does). **It is a *conversion*, not an addition (Patrick, 2026-09-14):** the
universal centroid's inward `1/d²` attraction is **replaced** by a total-mass radial
dampening. This also removes a *current* artifact — right now the **universal centroid and
the largest group are identical** (same members, same mass, same origin), so the
mass-centroid pull is applied **twice** (double compression); converting the universal pull
to the radial dampening **eliminates** that doubled inward pull rather than adding to it.

**Testing the real behaviour needs complexity (Patrick, 2026-09-14).** Properly testing
what the system does — settling and **invariant relationships** — requires a **complex
interconnected field structure**; a few fields cannot establish those relationships. The
current unit tests validate **mechanics** (kernels correct, boundedness, cpu/cuda agreement
on the mechanism), **not** the model's settling/invariant behaviour, which only emerges with
a rich field structure. Green unit tests ≠ validated model; that richer validation is a
future scenario, not these unit tests.

### 3.11 The particle pool — fixed allocation, claim/release (Patrick, 2026-09-14)
To ease live-allocation math and make **declaration** cheap (cost model: declaration ≪
allocation), the **entire workspace is a fixed number of particles** — whatever makes sense
for the card — **allocated once.** There is **no live allocation**; the simulation
**claims and releases particles from this pool** as LoD needs change (expanding a composite
into granular orbs claims particles; collapsing releases them).

**The core win: this converts allocation into declaration within the working space.** Once
the pool is allocated (once), every later "allocation" of a particle is just a field-set on
an existing slot — **declaration, near-free** — never a real allocation with its cascading
cost (cost model, §2). That is the whole reason for the fixed pool.

- An **unused particle is `0x` (a null hex character)** in the DB: **no inherent mass,
  presence, or location** — a virtual particle in the free pool.
- **Nulls are naturally inert — no special-casing.** Mass 0 ⇒ it contributes nothing to any
  (mass-weighted) centroid, exerts no force (`m1·m2/d² = 0`), feels none, and adds 0 to the
  universal mass. **Claim** = set token_id + mass + position; **release** = zero them back
  to `0x`. Both are **declaration** (flip fields in place), never allocation.
- **The claim/release IS the LoD rollup mechanism (Patrick, 2026-09-14).** The per-tick
  centroid virtual particles and the LoD-expanded orbs are all drawn from this pool: rolling
  up **claims** a virtual particle for the aggregate centroid; expanding a composite
  **claims** its granular orbs; collapsing **releases** them to `0x` — all declaration,
  near-free. LoD rollup is a claim/release dance over the pool.
- **Foundational to the viewport (§5, next phase).** As the observation root travels and the
  neighbourhood LoD shifts (near = granular, far = aggregated, §3.3a), the **mathematical
  viewport claims and releases particles from this same pool** to expose exactly the detail
  in view. The viewport is built on this pool mechanic.
- **The SNode tree services this directly — it is the mechanic for exposing relative LoD by
  perspective (Patrick, 2026-09-14).** LoD is **not absolute; it is relative to the
  observation root.** The same construct exposes **fine detail near the perspective and
  coarse aggregates far from it**, and **travelling the perspective recomposes the tree** to
  expose the LoD relative to the new vantage — the tree does not store one fixed hierarchy,
  it *renders* perspective-relative LoD on demand. It is the engine's mechanism for the
  composed rollup: its structure *is* the LoD grouping, built live (`add_snode_tree`, any
  shape) over the fixed pool — near-root-granular ↔ fine nodes, far-aggregated ↔ coarse
  nodes. So the whole chain is: **pool (particle backing) →
  claim/release (the operation, declaration) → SNode tree (engine structure servicing the
  rollup) → viewport (drives it by neighbourhood).** On the vetted seam (§2): live
  `add_snode_tree` **recycles tree ids** (rollups recompose cheaply), while per-node SNode
  ids are **monotonic** — churning rollups spend that budget, which is why the harness holds
  the **restart authority** to reclaim it.
- **Modernization — per-particle size (Patrick, 2026-09-14):** the **original engine allocated
  a full ~1 MB per particle**, far exceeding this project's needs (even **a thousand force
  definitions is a fraction of a MB**, and most particles far less). The engine modernization's
  **lazy paging** (the paged `ListManager` — small root directory + lazy ~4 KiB pages, added
  deliberately) lets each particle occupy only its **actual** data (KB-scale, grown by adding
  pages nominally), so the per-card budget `N` is **far larger** than 1 MB/particle would give.
- **Budget-probe results (Opus 4.8 fork, read-only, 2026-09-14).** `N = min(pool_bytes /
  (slots×4), flat_index_ceiling / slots)`. Numbers: **GTX 1070** (7.84 GiB free, memory-bound)
  — 31.5M particles @ 64 slots, 7.9M @ 256, 2.0M @ 1024. **RTX 3060** (12 GiB, spec estimate —
  needs a host run) — ~33.6M / 8.4M / 2.1M. **Flag (layout, not memory):** `field.cpp`'s SoA
  index `f·count + i` is a single **i32**, capping total cells `N×slots` at ~**2.1e9**; on a
  12 GB+ card that binds *before* memory, so `N` is layout-capped there. Lifting it is a
  2D/64-bit-index layout decision (ties to the vetted "logical indices 32-bit" caveat, §2).
  - **Workstream — lift the i32 cap (Patrick, 2026-09-14).** The i32→i64 logical-index
    widening *should* have been part of the engine modernization but was **deemed out of scope
    by that request**. The current cards are essentially e-waste; **full computational access
    on larger cards requires lifting this cap.** The ~2.1e9-cell budget is **plenty for early
    needs** (not an early blocker), but on a **larger card doing complete, complex modeling** it
    **bites fast** — so the widening is forward-enabling for that regime, worth landing now while
    it's fresh rather than urgent for current work. **Scoped (Opus 4.8, read-only, 2026-09-14) — the fix is BOUNDED, not a pervasive rewrite:**
    - **Real binding limit is ONE engine codegen function:** `taichi/codegen/llvm/codegen_llvm.cpp`
      `visit(ExternalPtrStmt*)` builds the flattened `linear_index` in **i32** (per-axis `sizes[]`
      loaded as i32), so the engine flattens to i32 **regardless of what the harness passes** —
      a harness-side 2D/i64 layout alone does **not** suffice.
    - **NOT pervasive:** the deep engine i32s (`ListManager::num_elements`, `PhysicalCoordinates`)
      are the **sparse SNode path**, which the harness's **dense `Ndarray`s bypass**. So lifting
      the cap needs only the `ExternalPtr` offset widened, not the engine's logical coords.
      Per-axis sizes stay i32 (each axis ≪ 2.1B); only the **product** needs i64.
    - **Architecture-neutral:** `codegen_llvm.cpp` is the shared LLVM codegen — **one change
      covers CPU + CUDA**, no vendor/driver-specific code. (SPIR-V/Vulkan, Metal, DX12 have their
      own ExternalPtr codegen and would each need the same i64 treatment for full multi-backend
      agnosticism, but those backends are unqualified/unused — deferred, consistent with §1.)
    - **Sonnet units:** **U1** (engine — `visit(ExternalPtrStmt*)`: compute `linear_index` in i64,
      extend i32 sizes/index operands, i64 GEP; localized); **U2** (harness — switch the SoA to
      **2-axis dense `Ndarray`s** field×particle passing `{f,i}`, or an i64 flat index — 2D
      preferred); **U3** (rebuild the engine so `libllvm_codegen.a` carries U1, re-link the
      workspace). Sequence: **U1 → rebuild → U2 → validate**, each read-diffed + tested.
    - **STATUS (2026-09-14): U1 + rebuild DONE + verified.** The engine `ExternalPtr`
      `linear_index` is widened to i64 (`codegen_llvm.cpp` ~:1932-1954), `libllvm_codegen.a`
      rebuilt, workspace re-linked, **57/57 pass cpu+cuda — no regression**; engine build notes
      updated (`/opt/project/taichi/modernization/IMPLEMENTATION-STATUS.md`, `deferred-backends.md`).
      **The reusable root is fixed.**
    - **U2 DONE + verified (2026-09-14).** `field.cpp` refactored to a **2-axis dense layout**
      (`{field, item}`, addressed by `{f, i}`; host layout + upload/download byte-identical, so
      behaviour-preserving), and the workspace wrapper now threads dimensionality: `Param` gained
      `total_dim` and `CompiledKernel` passes it to `insert_ndarray_param` (was hardcoded 1),
      with the six 2-D arrays registered at dim 2 in every kernel (`src/engine/runtime.{h,cpp}`,
      `field.cpp`). **57/57 cpu+cuda, no regression, no value changes.** ⇒ the index cap is now
      lifted **structurally end-to-end** (harness 2-axis i32 axes × engine i64 flatten).
    - **Validation (`cap-validate`, 2026-09-14) found a SECOND engine i32 bug — the cap is NOT
      yet lifted end-to-end.** `taichi/program/ndarray.cpp` computes `nelement_` (a `size_t`) via
      `std::accumulate(shape, 1, std::multiplies<>())` **seeded with an int `1`**, so the product
      **overflows in 32-bit** for any array with >2^31 *total* elements — aborting at
      **construction**, before the widened codegen runs. The 2-axis layout bounds each *axis*
      but not the product. (Both `Ndarray` ctors, ~`:34-37` and `:72-75`; a neighbouring
      `1LL`-seeded check only *warns*. Same accumulate-seed pattern at `snode.cpp:98` — a warning
      there, real-bug status unverified.) **`cap-validate`'s sub-cap control (N=5e8) confirmed
      U1+U2 are correct** — both sentinels round-trip cpu+cuda under the cap. The new test
      `tests/index_cap_test.cpp` (+ CMake entry) is in place, red at >2^31.
    - **U4 (engine, one-liner ×2):** seed the `nelement_` accumulate 64-bit (`std::size_t(1)` /
      `1LL`, matching `nelement_`'s type) in both `Ndarray` ctors, rebuild the affected archive,
      re-link, then `index_cap_test` should go green (>2^31 array constructs + high sentinel
      round-trips cpu+cuda) with `field_test` still 57/57. Update the engine build notes. **Then
      the cap is lifted end-to-end.**
    - **DONE + VALIDATED end-to-end (2026-09-14).** U4 widened `nelement_` to 64-bit in both
      `Ndarray` ctors; incremental rebuild + re-link; **ctest 3/3** — `index_cap_test` PASSES on
      **cpu and cuda** (`{2, 1.09e9}` = 2.18e9-element u8 ndarray constructs; HIGH sentinel at
      flattened offset 2,179,999,999 > 2^31 round-trips correctly on both; LOW cell unaffected),
      and `field_test` still **57/57**. **The dense-ndarray index cap is lifted end-to-end** via
      two engine sites (U1 `ExternalPtr` i64 + U4 `nelement_` 64-bit) plus U2's 2-axis harness
      layout — all architecture-neutral (shared LLVM codegen). Engine build notes carry both
      entries (`IMPLEMENTATION-STATUS.md`). **Residual (non-blocking):** the soft
      `TaichiIndexWarning` still prints (a cosmetic int32-boundary warning, not a hard cap) —
      silenceable later. `tests/index_cap_test.cpp` + its CMake entry are the standing regression
      guard. **i32-cap workstream COMPLETE.**
    - **Validation needs a host run:** the cap only manifests past 2^31 cells, so a deliberately
      large ndarray (>2.1e9 elements) must be written/read/checked on **both CPU and CUDA** — a
      small-N unit test can't catch it.
    - **On the 750-Ti 64/32 hypothesis:** the binding i32 here is the **index-offset codegen**, a
      *different* subsystem from the 750-Ti **alloc** failure (`cuMemAllocAsync`); the scope did
      not examine the alloc path — so **no claim either way.** Drivers do interact in odd ways,
      so **test the 750 Ti after the widening** rather than assume; keep it a live possibility,
      not a dependency.
    - **Engine build notes are part of the work (Patrick, 2026-09-14).** Because U1/U3 change
      the **engine source** (`/opt/project/taichi`), those units MUST update the **engine's own
      build / modernization notes** to reflect **the change made** (the `ExternalPtr` i64
      widening) **and the deferred work** (sparse-path i32s left as-is; the other backends'
      ExternalPtr codegen still i32; the 750-Ti/64-32 question). The engine's documentation must
      not fall out of sync with its source — update-beside-and-preserve there too.
- **Lazy paging — confirmed nominal, with one distinction (source-verified, `runtime.cpp`).**
  Growing the **number of entries** (listings/references) is a nominal incremental page-add
  (2 KiB root + lazy 4 KiB chunks, no copy, no reallocation; a brief lock only when a new chunk
  is first touched). That is exactly the DB's variable part — participations stored by
  `token_id` reference into growable shared listings. **But** widening a **fixed dense payload
  width** is a layout change (a new field), not a lazy page-add. ⇒ variable reference/listing
  data grows lazily (nominal); the fixed numeric-slot payload stays fixed-width by design.
- Sizing: a fixed `N` particles sized to the card (§2/DEVICES: pool bytes / actual per-particle
  bytes — KB-scale, not 1 MB). Current `field.cpp` fixes the particle count at construction —
  that *is* the pool; the `token_id` / `0x`-null field and the claim/release mechanism are added when LoD
  dynamics land.

---

## 4. Current build state

`src/field/field.{cpp,h}` + `tests/field_test.cpp` — a real `field::Harness` (subspace-level
base mechanics; single LoD):
- **State:** packed SoA — `particle` (position, velocity, mass, force), `group` (the
  centroid ping-pong), `edge` (participation: particle, group, share, offset), `bond`
  (particle-a, particle-b, `defined` flag, polarity `align`), and a single `universal` mass
  cell (total construct mass, summed once per tick in clear). Host boundary: `upload` /
  `download` (positions are the save state).
- **Kernels:** **clear** (zeroes accumulators + sums the universal total mass); **force**
  (per-edge, the one law with exact d=0 + sole-member gates, participation via share/offset);
  **contact** (per-bond corona separator — regime-gated standoff + defined-bond invariant
  hold; the field provides the attraction); **integrate** (`v += F/m`, momentum carry, brake
  off the resultant, then the **radial velocity dampening**); **field_universal** (the inward
  pull — **SUSPENDED**, kept for possible reactivation); **determine** (derivative-product
  seam, placeholder); **centroid + publish** (amalgamation = next-tick setup).
- **DONE + verified (CPU + CUDA agree, 57/57 checks):** Unit A (exact law/gate), Unit B
  (rotation stripped, offset kept), Unit C (brake off the resultant), the **corona
  separator** (§3.8), and the **universal centroid → radial velocity dampening** (pull
  suspended, §3.3a). **The subspace-level physics core is complete and correct.**

Caveat on record (§3.6/3.7): `reach` is `|F|`; induced displacement is `|F|/mass`, so the
overshoot comparison is exact at unit mass and scales with mass otherwise — flag only if
non-unit masses come into play.

---

## 5. Forward build sequence

**Phasing (Patrick, 2026-09-14).** Real usage exposes **more than one LoD at once** (for
anything past trivial/Pong-level). We are deliberately establishing the **base mechanics at
a single base LoD — the 'subspace' level** — so **progressively larger structures** (more
LoDs, bigger constructs) compose from it. Once these base mechanics are solid, the **next
major phase is the mathematical viewport into the construct** — the realized observation
mechanism (the locus of observation / root of analysis, §3.3a: how you look *into* the
construct). Everything below is the subspace-level substrate.

Sequenced to the cost model; each unit **fully specified by Opus 4.8, coded by Sonnet,
validated (model-anchored tests) before the next.**

**Done + verified (cpu+cuda, 57/57):** Units A, B, C (physics core); the **corona separator**
(§3.8); the **universal centroid → radial velocity dampening** (inward pull suspended, §3.3a).
The subspace-level base mechanics are complete. Trial constants to calibrate: corona
radius/stiffnesses, `reach_strength`.

**Mine now (translation, no new Patrick input needed):**
- **Payload shape** — the edge list is already close to the model's `(field, participating
  mass, offset) + position` shape; confirm/firm it rather than a rigid fixed-slot row.
- **Primary-kernel consolidation** — move toward Patrick's "one replicated kernel" for the
  *per-particle* primary work (sum → resultant → exp attenuation + d=0 gate → move). Honest
  constraint: force accumulation is a **per-edge reduction**, a legitimately separate pass;
  "one kernel" is the per-particle calc, which is nearly there.
- **Determiner seam** — the place derivative-product analytics (bond significance,
  reducibility) are read off the primary pass; wire real outputs as the analysis needs land.

**Contact / corona and the universal centroid — DONE** (moved from this list; see "Done"
above, §3.8, §3.3a). Contact = the corona separator; the universal centroid's inward pull is
suspended and converted to the radial velocity dampening.

**CPU-resident half (U7+ — gated on the DB source location + Patrick's pacing):**
- Follow-only **DB traversal** (child listings + fixed orderings; no search).
- **Composed-tree construction** — build the *selected* working-DB rollup live
  (`add_snode_tree`, any shape); selection policy comes from the routines.
- **Focus / neighbourhood selection** — choose the resident subset (§3.9 tier 3).

**Lifecycle wrapper (U1/U2 — when wanted):** capacities chosen up front, one pool
allocated once, the **restart authority** (capacity re-flex / node-id reclaim / promotion).
Capacity sizing is a deliberate tuning decision against how much working/resident DB a
class of analysis needs live.

---

## 6. Open (small) and deferred (Patrick-paced)

**Patrick's to explain — NOT yet given, NOT mine to draft or build:**
- The **corona-effect mechanism** — how the corona/polarity produce closeness and
  bonding in the tick (§3.8). He has not finished explaining contact mechanics.
- The **invariant defined-bond hold** (shear-resistant) — its definition is his.

**Genuinely mine (translation, minor, not blockers):**
- Final **payload shape** confirmation.
- Corona **radius** value is a trial (0.5 vs 1) — but only once the mechanism exists.

**Deferred by Patrick's pacing — do not build now:**
- **DB source location** and other external inputs.
- The **specific force set** — Patrick knows many of the forces; they are **set up when
  the database connections are established** (part of wiring the DB source, deferred
  below) — not something to investigate here. (Distinct from this: the *system's* job is
  to **discover more than is currently explicable** — see §3.9; that is the analytical
  goal, not an open setup item.) Their **activation** is a **scalar launch argument**
  (§3.9), not a config file: there is **no JSON / config surface** in this design. (The
  repo-root `field/*.json` files are artifacts of the earlier Python prototype, out of
  scope and not part of the harness.)
- **Numbering → analytical coupling** beyond the polarity already given.
- **Focus / neighbourhood** detail.

---

## 7. Operating discipline (carried, non-negotiable)

Literal not analogy · **don't pinball** (absorb corrections narrowly; don't declare parts
final/redundant unless Patrick did) · don't re-ask what's answered · **no single locus;
on conflict stop and confirm** · verify against live code · record work completed · tests
on everything · Canadian English · Opus 4.8 investigates, Sonnet codes (validated),
Opus 5 / Fable 5 banned · Python is front-end feed only, never engine/data.
