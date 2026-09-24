# Drift audit — the existing `src/field/field.cpp`

**Context (Patrick, 2026-09-13).** The current `field::Harness`
(`src/field/field.cpp` + `field.h`, with `tests/field_test.cpp`) was produced by
the degraded model that this harness effort was brought in to replace. Its
failure mode is the fatal one for new models: **generally correct in shape,
specifically drifted — "mostly right but unreliable in just the most important
bits."** Its tests pass (CPU + CUDA, architectures agree, ~15.5 s) but that
proves little: the tests were authored alongside the code and **encode the same
drifted specifics** (e.g. the calibration test sets `softening = 1e-6` to dodge
the very distortion the shipped default `0.25` causes). Green ≠ correct here.

This audit anchors each finding to Patrick's literal model (`HARNESS-NOTES.md`),
ranked by how load-bearing it is. Corrections are code changes → dispatched to
Sonnet on Patrick's instruction, one validated unit at a time; tests must be
**model-anchored**, not self-grading.

## 1. It breaks the one operative formula. — DRIFT (vs explicit spec)

The core law is `m1·m2/d²`, one law without exception. The force kernel
(`field.cpp:150-170`) computes
`|force| = gravity·m1·gm·|d| / (d² + softening)^1.5` with `softening = 0.25`
(`field.h:65`) added to **every** squared separation, unconditionally. That is
not inverse-square — it is a softened law that bends the force at **all**
distances (worst at close range), and it is exactly the **epsilon-softening
Patrick rejected** for the d=0 gate ("exact zero, NOT epsilon-softening";
`field-engine-no-exceptions`, `works-in-shape-of-math`).

- **Tell:** the code already knows the exact technique — it gates the
  sole-member case branchlessly with `engaged = sgn(|gm − m1|)`
  (`field.cpp:163`) — then reaches for blanket softening anyway.
- **Fix:** keep `d²` exact; multiply the interaction term by `sgn(d²)` (exactly 1
  for d²>0, exactly 0 at coincidence) — branchless, no distortion. Near-contact
  behaviour belongs to the **pending contact mechanics**, not a blanket epsilon.
- **Same family (review):** `corona = 0.5` added in the brake ratio
  (`field.cpp:265`); `kWeightFloor = 1e-20` on every denominator
  (`field.cpp:36`, `:253`, `:371`). Keep only where a denominator is genuinely
  a group total that dwarfs the floor; remove where it distorts.
- **Status:** DONE + verified 2026-09-14 (**Unit A**, Sonnet). Exact `sgn(d²)`
  gate, `softening` removed, coincidence test added. Edit read directly and
  confirmed NaN-free and epsilon-free; all checks green (CPU + CUDA). Diagnostic:
  `check_stability` still passes without softening — the brake alone holds it
  there.

## 2. It invented rigid-body rotation. — DRIFT (Patrick ruled 2026-09-13)

The code carries `kInertia`, `kTorque*`, `kAng*` and integrates torque →
angular velocity → carried spin (`field.cpp:176-181`, `:240-249`), spinning a
particle when a share sits off-centre.

**Ruling (Patrick, 2026-09-13):** there is **no running tick-to-tick rotation**
— nothing spins with conserved angular momentum between ticks. Rotation is an
**ordering-and-alignment trick** in the ordering-and-connection family: the
parent predicates are operationally an **ordered line across the sphere's
diameter, oriented ("rotating") about the centre**; "position vs centre" is a
component's ordered place along that diameter, not a lever arm. The rigid-body
integration is imported machinery filling that gap = drift.

- **Fix:** strip the running angular dynamics (inertia/torque/angular-velocity
  integration). Keep the `(share, offset)` edge shape (the Parent/Sibling
  unification is correct). The ordered-line / alignment content is **deferred**
  — Patrick will specify it "as it is closer"; do not build it now.
- **Status:** DONE + verified 2026-09-14 (**Unit B**, Sonnet), after Unit A.
  Removed `kInertia`/`kAng*`/`kTorque*`, the torque accumulation, the angular
  integration; **offset kept** in force-position and centroid (location-aware
  translation, the magnet example — NOT the deferred ordering-line). `check_spin`
  replaced by `check_offset_pull` (off-centre share still translates the whole
  body). Edit read directly and confirmed; all checks green (CPU + CUDA).

## 3. The brake / destination — RESOLVED (2026-09-14)

**Composition RESOLVED (Patrick, 2026-09-14). Not yet implemented — dispatch as
Unit C.** The code still carries the drifted `kTarget*` weighted blend and the
brake aimed at it — unchanged until Unit C runs. **Read the pinballing warning
first** (this file's discussion careened repeatedly; the resolution below is the
plain reading of what Patrick has restated across many contexts — do not
re-open it or re-insert invented machinery).

**THE ANSWER (Patrick, 2026-09-14, restated many times across contexts):**
- The destination is the **sum of the m1·m2/d² vectors** — the same sum that is
  the resultant. One accumulation, not two. Those vectors, summed, point at the
  target. There is no second, separately-weighted quantity.
- The brake's reach is the **magnitude of that resultant** — the total distance
  being sought. The destination is the position plus the resultant.
- The brake is **exponential to that total distance sought**, so it is ≈1 in any
  tick that does not overshoot the destination and bites only on a would-be
  overshoot. That is its explicit and sole purpose.
- Every field effect is bidirectional (both masses move toward their shared
  mass-weighted centroid, each displacement inverse to its own mass); the
  exponential brake is retained.

**Implementation note (the drifted code, for Unit C):** the current `field.cpp`
carries a *second* accumulator alongside the force sum — a separately-weighted
blend and the brake taken off it (~`:185-195`, `:242-254`). That second
accumulator is the drift; remove it and take the brake's reach off the resultant.
(Its field names are the degraded model's, not the model's terms.)
   Its **sole** purpose is to correct **per-tick discretization ("time-shearing")
   error, which compounds** into kinetic instability. Without it a coarse tick
   settles only via controlled-but-finite Lyapunov cycles (bounded oscillation),
   and only if the tick is fine enough to emulate the field at the needed
   granularity. It is a dampening method for discretization — chosen **simple
   (exponential)** over the complex functions (implicit / adaptive / RK) that
   normally do this job, because relative configuration matters far more than
   exact position, so trading positional accuracy for stability costs the model
   nothing it cares about.
6. **The brake must NOT affect any motion predicated by the math.** Surgical:
   ≈unity for every legitimate step, biting only on the discretization over-step,
   and it must go to unity in the small-step limit (as dt→0 / travel small vs
   reach) — the tell that it touches only the artifact, not the physics.

**"OPEN" — but FIRST reconcile against the prior record; do NOT re-ask what is
already answered.** Patrick (2026-09-14): *"most of your open ones were addressed
before this context and you have ignored or lost them."* Several items below were
resolved in earlier sessions and are captured in `HARNESS-NOTES.md`; this
discussion lost them. Consult that resolved record BEFORE treating anything as
open or putting it to Patrick:
- **Momentum: ALREADY RESOLVED in HARNESS-NOTES** — velocity/momentum **carry
  between ticks; the move is inertial / second-order, not overdamped.** (I wrongly
  declared it "superseded / overdamped" mid-discussion — a careen against the
  record.) So the motion is inverse-square force + carried momentum; finding 3
  changes only the brake's target, not the motion.
- **The exponential attenuation mechanism is ALREADY partly specified in
  HARNESS-NOTES** (unity at tick start; aggressive only on overshoot; once per
  particle on the resultant; the d=0 gate). Reconcile with that; what genuinely
  remains is the exact target it measures against and any remaining constants.
- **Was "genuinely still-open" — now RESOLVED (2026-09-14):** the destination is
  the plain sum of the m1·m2/d² vectors (= the resultant); brake reach = its
  magnitude. See THE ANSWER above. Nothing left to firm up.

**Current code state (unchanged from Unit B):** `field.cpp` force pass still
accumulates the `kTarget*` weighted blend (`weight = scale·d²`, ~`:186-195`);
integrate pass still brakes off it with `exp(−ratio⁴)` and `corona` (~`:242-258`).
None of finding 3 is applied.

## Note on the `π²/128` constant (not a finding)

`gravity` (`field.h:61`) is a parameter that follows from the law; it re-derives
once the law is exact (1) and the brake is right (3). Not a drift finding.

## What is correct and must be preserved

Not to be thrown away — the shape is right:

- **SoA packed layout** (`f·count + i`), contiguous per-field runs — cost model.
- **`m1·m2/d²` against the group centroid**, m2 = previous tick's centroid
  (`kCen*`) — the virtual particle.
- **Amalgamation = next-tick setup**, built as a ping-pong (`kAcc*` written this
  tick → `kCen*` read next) — matches the settled tick loop.
- **Parent/Sibling unified as `(share, offset)`**, no special case: whole =
  share 1, offset 0; component = fraction + offset.
- **Superposition** (forces accumulate), **momentum carries, no decay**
  (`v += F/m`; `x += v·dt`) — matches Patrick's "no other decay factors".
- **One thread per edge, identical work, branchless**; CPU/CUDA agree.

## Disposition summary

| # | Finding | Verdict | Action |
|---|---|---|---|
| 1 | Blanket `softening` breaks inverse-square | DRIFT vs spec | Exact `sgn(d²)` gate; drop distorting epsilons |
| 2 | Rigid-body rotation | DRIFT (ruled) | Strip running angular dynamics; ordering deferred |
| 3 | Brake destination = sum of the m1·m2/d² vectors (= the resultant); reach = its magnitude | RESOLVED | Unit C: strip the second weighted accumulator; take brake reach off the resultant |
