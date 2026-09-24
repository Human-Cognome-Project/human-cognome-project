# Project Plan

Working template. Fork of the Taichi core.

Last updated: 2026-09-09

---

## 0. Rules governing this document

These rules bind the planner and every dispatched agent.

1. This document records **only discussed and settled decisions**. Nothing is
   entered on inference, assumption or extrapolation.
2. The planner does not extemporise. If something is not settled, it belongs in
   section 8, Open Questions, not in the body.
3. Every agent, at every point, must **stop and report** if it encounters
   uncertainty, or any cause to extrapolate creatively beyond its specific
   assignment. Escalate to the planner. Do not improvise a resolution.
4. Agents receive some or all of this document as their guiding doc, scoped to
   their assignment.
5. The planner keeps this document current. Updates happen after a decision is
   discussed and settled, never in anticipation of one.

**Role split.** The planner acts as overall planner and arbiter, not as
implementer.

---

## 1. What this project is

A fork of the Taichi core, taken as a starting point and not as a dependency.
The core provides powerful machinery that is slightly misaligned with what is
needed. Work proceeds from it without locking into its assumptions.

### 1.1 Explicitly in scope

- The C++ core.

### 1.2 Explicitly out of scope

- The Python front end. Not a concern, not being updated.
- Feeding changes back upstream. May or may not happen. Undecided, and no work
  is shaped around it.

### 1.3 Explicitly NOT the goal

- This is **not** an attempt to make Taichi a game engine.
- Rendering data is **completely irrelevant**. Visual rendering is entirely
  client side, using simple circular particles and cubic label particles. The
  primary consumer of produced data is a secondary process.
- **The mesh relation system is OUT OF SCOPE, but items are not shapeless.**
  Settled, with a correction the planner had to make.

  Items DO have volumes. They take physical shape, but **only ever spheres**.
  That is a very limited calculable mesh: defined by centre and radius, derived
  when needed rather than stored, and requiring no topology at all.

  Taichi's mesh support is a topology system: vertex, edge, face and cell
  elements with the sixteen relations between them, for finite-element and
  unstructured-grid work. Nothing here uses that. The hierarchy is centroid
  nesting reached through a dumb address, which is not mesh topology either.

  *An earlier version of this entry said perfect spheres have no surface to
  represent. That was the planner overstating a ruling again. Volume and
  physical extent exist; what does not exist is stored topology.*

  Consequence: mesh sites leave the width inventory. Territory 01 carries eight
  mesh cast sites. Territory 06 first reported that 24 of 46 newly found
  constructor-class sites turn on this ruling; an adversary then established it
  governs 12 rather than 24, on the grounds that the other twelve sit in a pass
  with no mesh or architecture gate and are therefore base work.

  **DISPUTED, and do not act on either version.** The second adversary states
  that same pass has zero mesh references and runs on CUDA alone, gated by a
  vendor extension check, and that its twelve are vendor work under section
  2.2a rather than base. It says roughly 45 percent of ALL newly found sites in
  that territory park as vendor work on the same grounds. The first adversary
  did not see the gate. Unresolved. Record them as excluded on scope
  with the reason stated, per section 10: a scope ruling removes work, not
  evidence. Do not delete a claim about the tree that a mesh site supports.

  *An early planner remark called the mesh relation system worth keeping. That
  predated any understanding of the workload and is withdrawn.*
- **There is NO graphic payload in the foreseen data.** Stated positively
  rather than left to be inferred from the line above. Particle attachments are
  strictly field interaction data. The modelling is purely mathematical, on
  round object mechanics.
- **Do not judge a backend by its graphics support.** A vendor tree may be
  less developed on graphics while its compute functions are perfectly
  adequate. Since this project needs no graphics at all, graphics maturity is
  not evidence about a backend's fitness here, in either direction. This
  applies to the deferred backend work in section 7 and to
  `modernization/deferred-backends.md`, whose baseline table records code
  volume without separating the two.
- **Graphics machinery may be stripped if it confounds the work.** The
  authorisation is recorded here so it is available when needed. Stripping
  itself remains deferred under section 7 as optimisation rather than an
  immediate concern, per the owner's earlier instruction. An agent that finds
  graphics machinery genuinely in the way should escalate rather than remove it
  unilaterally, per section 10.

### 1.4 Downstream context

The core is one element of something larger. That downstream context is **not
required** in order to work on the core, and is not a gate on any core task.

---

## 2. Why Taichi, and what must be preserved

Taichi is the only available option meeting all of the following at once:

- Vendor agnostic
- Hardware agnostic, including running with **no GPU at all**, just slower
- Fully open source
- GPU enabled, for mass solving loads

The requirement is a radically open computing paradigm, aimed at full
decentralisation and democratisation. Nothing vendor locked by design is
acceptable as a foundation, because a vendor lock decides who is allowed to
participate. Agnosticism here is not a preference or a nice-to-have. It is the
deliverable.

This is also why the e-waste hardware in section 3.1 is the proof rather than a
compromise. If the thing only runs on hardware people have to buy, it has not
demonstrated the property it exists to demonstrate.

Hardware agnosticism includes CPU only operation as a first class target, not a
fallback. Maxwell class hardware, or no card at all, can handle the basic maths
under gross operations.

### 2.1 Upstream status

Upstream is effectively dead. Commits by year:

| Year | Commits |
|---|---|
| 2019 | 3275 |
| 2022 | 1841 |
| 2023 | 910 |
| 2024 | 28 |
| 2025 | 37 |

Stale long enough to be close to abandonware. Practical consequences: expect no
upstream fixes, carry no obligation to preserve upstream compatibility, and
treat this fork as the maintained line rather than a branch off a living one.

### 2.2 Hardware changes speed and granularity, never capability

**Hardware never affects what the system CAN do. It affects only how fast it
runs and how granular the result is.** This is part of why Taichi was chosen.

This is a TEST to apply to every proposed change. A change that lets some
hardware do something other hardware cannot do at all is a violation, not an
optimisation. Newer hardware may make a thing faster or finer. It must never
make a thing possible that is otherwise impossible.

It is also what makes the velocity brake in 4.2 behave as a cost dial rather
than a correctness cliff, and why the tiers in 5.2 are fidelity classes rather
than feature classes.

### 2.2a Commonised base, vendor branches welcome

**Settled.** There is no objection to anyone creating a vendor branch, and such
branches are expected to fold back. **The requirement is that the BASE be
commonised.** Nothing in the base may be shaped by one vendor's requirements.

**What "vendor" means here, stated precisely because the planner got this
wrong.** A vendor backend is CUDA, AMDGPU, DirectX, or anything else tied to
one supplier or one operating system. The two COMPILATION SPINES are not
vendors: LLVM carries CPU-only operation, and SPIR-V carries the portable GPU
path. **Work on either spine is BASE work.** SPIR-V specific is not vendor
specific.

**BASE, in scope:**

- Item 6.1, the SNode ceiling and the memory pool.
- **Item 6.2, 64-bit addressing, in full.** This includes the shelved SPIR-V
  path and the `at_buffer` pointer-width collision. The shelved work covers
  64-bit addressing for everything EXCEPT SNodes, so it is the other half of
  the same limit. Raising the object count while addresses stay 32-bit moves
  the wall rather than removing it. The two halves are not separable.
- The unconditional index narrowing in `taichi/transforms/type_check.cpp`, and
  the three `int` fields at `taichi/ir/snode.h:41`, `:45`, `:49` that bind the
  sparse case.
- The invalid-module chain, which affects every backend. It has TWO classes and
  they fail differently:
  - An invalid identifier IS detected. The parser rejects it, the error reaches
    Taichi's own consumer, a severity test downgrades it to a warning, and the
    module ships anyway because the failure flag is read only inside a
    compiled-out block. Two warnings, no stop.
  - A widened pointer breaking an instruction's operand rules is NOT detected at
    all. Those are validator checks, and the validator is disabled
    unconditionally, so that class would ship with no diagnostic whatsoever.
- The destroy path, which leaves stale device tables with no free path.

**BRANCH, parked:**

- Vendor-specific backend elaboration, and capability plumbing that serves only
  one supplier.

### 2.2b How to cut a backend gap

**Settled.** The cut is by what the gap is IN, not by which backend it is in.

- **A gap that is purely graphics is irrelevant.** Ignore it. It is not work,
  it is not a defect, and it says nothing about a backend's fitness here. The
  entire unimplemented set in the OpenGL device layer is of this kind: raster
  pipelines, render passes, draw calls, surfaces, present, resize. Not one of
  them is compute.
- **A gap in a COMPUTE path is LEFT OPEN.** Do not close it, and do not remove
  it. Someone may want to complete or optimise that path for their specific
  hardware or software. Leaving it open costs nothing.

**This applies to DirectX.** It is not dropped. It is left. An earlier note in
this plan argued the case for dropping it outright was stronger than completing
a stub. That was the planner proposing a removal the owner had not asked for.
Leave it open and spend nothing on it.

**Consequence for `modernization/deferred-backends.md`:** its table ranks
backends by lines of code without separating compute from graphics, which
measures the wrong thing for this project. Do not use it to judge fitness.

*An earlier version of this section parked the entire 64-bit addressing path as
branch work. The owner's remark was about vendor backends such as DirectX; the
planner extended it to everything except the SNode ceiling. That was the third
over-extension of the day and the largest. Recorded so it is not re-derived.*

### 2.3 Why Taichi never grew, and why this project fits it anyway

Project owner's assessment, stated with total confidence:

**The object ceiling is the sole reason Taichi never grew.** 1024 is
insufficient objects for complex gaming or for in-depth analysis.

Compounding that, the fully fluidic nature of the calculation is inefficient
for most use cases. Between the two, it had no audience.

**This project is the specific case that needs the fully fluidic calculation,
and needs it without the 1024.** The property that made Taichi a dead end for
general adoption is the property this project requires. That is why forking it
is viable where a general revival would not be.

Consequence for priority: **item 6.1 is the project.** Everything else is
secondary to it, including the shelved 64-bit addressing work, which is stub
material under section 5.1 rather than a path to the ceiling.

---

## 3. Fork baseline

| Item | Value |
|---|---|
| Working directory | `/opt/project/taichi` |
| Upstream source | `taichi-dev/taichi` |
| Commit at fork | `ba0e81dce559fb63a5958bf82feb1d00c55c02fe` |
| Commit date | 2025-07-30 |
| Version | v1.7.0 + 70 commits, `version.txt` reads v1.8.0 |
| Licence | Apache 2.0 |
| Git remote | `upstream` only. No `origin`, no push target. |
| Submodules | 19 live, all at pinned revisions |
| Core C++ files | 568 under `taichi/` |

Note: `.gitmodules` declares an `assets` submodule that has no entry in the
tree. It is stale upstream. Only `external/assets` is real.

No formal GitHub fork exists. It was decided that formal fork or branch status
does not matter; the source files are what is wanted.

`/opt/project/taichi` is the planner's workspace. Files and directories may be
created in it as necessary. Planner working material lives under
`modernization/`.

### 3.1 Development and target hardware

This box is both the development machine and an edge device. It is e-waste,
used deliberately as a proof of concept for a particular kind of lean
computation.

- One NVIDIA GTX 750
- One NVIDIA GTX 1070

The resulting project is expected to scale to all levels.

---

## 4. The workload the core must serve

### 4.0 What the thing is: a level of detail system

**This is the main reason Taichi was chosen, and it governs everything below.**

The engine carries a relative maths system that works like a **telescoping
viewfinder**, a player window. That window explores the construct at varying
levels of detail and from various perspectives, while **all other elements stay
in graduated relative positions**, and **without disturbing the primary maths
model**.

The primary maths model **exists at all levels at all times, through
algorithmic aggregation**. It is not recomputed for a chosen level and it is
not approximated away from one. Every level is live simultaneously.

**Particle granularity is therefore scope-relative, not fixed.** In a given
scope, the active particle size is one for every unit. What sits inside a
single particle is the finer structure, arranged by the same parent predicate
groupings one level down. A thing that is one particle at one scope is a group
at a finer one.

**What this explains, all of it recorded separately below and none of it
independent:**

- Why sparsity is required at all (4.1): the levels are the structure.
- Why the centroid rollup runs at the end of every tick (4.4): the aggregation
  is what keeps every level valid at once. It is not only a force approximation
  and not only a field effect.
- Why the address is just an address (4.5): the window moves, the structure
  does not.
- Why any structuring must stay temporarily adjustable (4.8): the window moves
  and the analysis perturbs.
- Why the working set is composed from a larger store (4.6): the engine sees
  one scope at a time.
- Why the object ceiling is the project (2.3): more objects is more levels and
  more detail, which is the whole capability.

### 4.0a What an SNode is, in this project's terms

**Settled, and it decides how the ceiling is read.**

**The SNode system is a database.** The tree is the ADDRESSING. The SNodes are
the **unique key cells**. The ceiling is therefore on **unique configurations of
data**, not on field size.

This matters because in Taichi's general terms an SNode is a declared node in a
structure, and one dense field of a billion elements is a single SNode. That is
true and irrelevant here: these objects are unique configurations, not
homogeneous elements sharing a shape. So 1024 SNodes really is 1024 distinct
objects at a time, exactly as stated in 6.1.

**The analytical method.** Field analysis is the purpose, and level of detail is
a recognised structure-aggregation method for it. The analytical system enters
an object at high granularity and uses **LoD stepping** to find:

- where the analysis fails to resolve the incoming structure, and
- what can be read from any arrangement the structure takes.

**The simulation may contain multiple copies of SNode objects.** They are the
distinct data packages, each with defined field response factors.

**The same SNode used in more than one place IS a field.** Multiplicity is the
field, not additional key cells. So copies cost nothing against the ceiling:
1024 bounds distinct PACKAGES, and each package's occurrences are elements of
one SNode. This matches Taichi's own model, where one field of many elements is
a single node.

So the number of unique key cells available is the resolution the analysis can
reach before it must stop and rebuild. That is what the ceiling costs.

**Interaction with a verified defect, and it makes the ceiling worse than it
reads.** Territory 03 established that SNode identifiers **never recycle**:
`SNode::reset_counter()` exists with zero call sites anywhere in the tree, and
the only reset is the `Program` constructor. Destroying a tree does not return
its identifiers.

This does NOT apply to copies, which are the field and cost nothing. It applies
across REBUILDS: a run that composes a working scope, releases it and composes
another walks the identifier space upward without reclaiming any of it, and can
reach 1024 with far fewer than 1024 packages alive. The frequency of that is
set by the ceiling itself, per 6.1, so the two problems compound.

*An earlier version of this paragraph applied the accumulation to copies. That
was the planner extending a verified defect past what it covers.*

### 4.1 Nature of the work

An analytical machine. Nested gravity calculation used as an
**n-dimensional data analysis tool**.

Scale framing: not inequivalent to an orbital model of the universe extending
from a full macro picture down to near atomic levels of detail on any chosen
spot. Perspective focused orbital mechanics, in sparse, mass solving loads.

**Sparsity is required.** Do not treat sparse machinery as surplus.

### 4.2 The two primary algorithms

1. **Gravity.** Standard `m1*m2 / d^2`.
2. **Velocity brake.** Every velocity is reduced exponentially over the length
   the tick represents, with the root of the exponent being the total distance
   to be covered.
   - Braking weakens the further a particle has to travel, and bites hardest as
     it closes on its target, so a step cannot carry it past.
   - It only really engages when tick granularity is insufficient to allow
     surrounding forces to moderate the actual interaction.
   - Purpose: prevent smashing, kinetic escalation and shearing, because
     kinetic escalation follows granularity.

The exact algebraic form of the brake is **the outer controller's problem, not
the core's**. It is simple exponential maths. It was described here so that the
core work proceeds understanding that the actual functions are incredibly
simple.

Everything else is accompanying machinery that makes the above relevant to
focus and tractable.

### 4.3 Consequences of the brake

Tick granularity is a cost dial, not a correctness cliff. Coarse ticks on weak
hardware remain stable. The same algorithm spans the whole hardware range
rather than forking into a cheap version and a real version.

### 4.4 Tick structure

Centroid calculation sits at the **end** of each tick, so the next tick begins
with estimated centroids.

- The centroid calculation pays the entire pairwise tax **once per grouping,
  not once per member in it**.
- **This is primarily a MEMORY saving, not an arithmetic one**, and that is
  where it matters. Per the cost ordering in 6.4, the force pass is bandwidth
  bound rather than arithmetic bound: the maths is two cheap operations. A
  pairwise pass streams the whole particle set with no reuse, which is the
  worst pattern available. A centroid is read by every member of its group, so
  it stays hot in cache. The centroid does not only divide the traffic, it
  converts what remains from streaming into cached.
- It provides a field effect by nature.
- The force pass therefore reads a finished snapshot. Read only, no ordering
  requirement, no contention.
- The centroid reduction is the only phase that writes.

### 4.5 Data model

- **All effect data lives on the particle.**
- **The address is just an address.** It locates. It carries no effect data.
- Per particle payload: a simple list of keys and force values in a
  parent/sibling/child structure. Even an incredibly complex construct is at
  most a few hundred rows of text.
- Keys and force values are read as dimensions and their weights, with the
  nesting giving structure at multiple scales.

### 4.6 parent / sibling / child semantics

- The parent/sibling/child relation **is centroid nesting**.
- **Parent:** parents are in the construct. They react in relative parts.
- **Sibling:** siblings are related but distinct structures. All of it is
  attraction. When all sibling groups contract simultaneously that produces a
  pushing effect to some degree. All are pulled together in any sibling group.
- **Child:** simple traversal speed, because the database is dense and
  optimised for minimal data size. May not be relevant in an actual engine
  SNode configuration.

The engine SNode configuration is **composed from the larger data stores for
working scope**. It is a materialised working set, not the store itself.

### 4.7 Effects must be adjustable

Being able to adjust or remove an effect temporarily is crucial. Relying on
direct connections may block some analysis work.

Effects are therefore data the kernel reads, not connections the layout
hardwires. A sign reversal is a permutation on gravity, the same expression
with a reversed sign, so adjust, remove and invert are one operation on a
coefficient rather than separate code paths.

An inverse gravity push may or may not stay in. It is irrelevant to core work
and is deferred.

### 4.8 Runner and controller, and why structural sorting is suppressed

There will be a **runner** and a **controller** function, both outside the core.

- The **runner** formats data from the main database to comply with the GPU's
  requirements for processing. Format conversion from the store is therefore
  not the core's job. The core receives data already shaped for it.
- The **controller** is the outer layer. It already owns the brake formula per
  section 4.2.
- The runner also **saves and resumes the simulation with updated SNode data**
  as required. This is how "adjustable on a temporary basis" is achieved in
  practice. The core needs no runtime structural mutability: the runner
  checkpoints, changes the data, and resumes.

**Design principle, from the owner:** the less forced structure there is, the
more flexible the analysis is, with the fewest tools. Prefer the arrangement
that imposes least.

### 4.8a Pre-allocated particle budget

**Settled.** The environment is optimised by pre-allocating a **fixed particle
budget**, occupied by a **no-mass, no-identity, non-particle**. Insertion or
transformation pulls from that pool or reallocates back to it. **The GPU has
clean data at all times.**

Consequences, none of which required a new mechanism:

- **No allocation churn during a run.** Particle arrival and departure inside a
  scope are slot claims and releases, not structural change. Rebuilds are
  driven by scope change, not by population change.
- **The null particle needs no branch.** Zero mass makes its contribution to the
  force sum vanish arithmetically, so it is skipped without divergent control
  flow. That matters more on a GPU than the memory does.
- **The budget is what the sizing rule sizes.** Section 8.1 item 2 says optimise
  for what is there; the fixed budget is the concrete thing being fitted to
  available memory, and it is known at configure time, which is what section
  5.1a's install-time agent needs.
- No uninitialised device memory is ever read, by construction.

**Consequence, weighted correctly.** Territory 03 established that
`Program::destroy_snode_tree` leaves stale device tables and that the device
allocator has no free path, and recorded it as latent because its only caller
today is the Python binding, which section 1.2 puts out of scope. Save and
resume with updated SNode data does make that path REACHABLE.

But it does not make it routine, and **raising the ceiling reduces the need for
it inherently**:

- Most restarts of the system exist to EXPAND the SNode tree, not destroy it.
- A destroy is needed only when the available objects are insufficient for the
  current need and new objects have to be loaded.
- A process restart resets the SNode id counter through the `Program`
  constructor, which is the only reset in the tree, so ids do not accumulate
  across restarts.

So the defect is reachable rather than routine, and its priority follows from
that. It is worth recording and worth fixing when that path is built. It is not
a blocker.

*An earlier version of this paragraph called it live and said it should be
treated as such. That was the planner over-weighting a finding again, the third
instance in one session. Recorded so it is not re-derived.*

**Why structural sorting is suppressed.** Analysis may require changing any
force value, including defaults, on specific elements, in order to observe the
transformations that follow. If the structure were sorted or organised by the
data's own relationships, perturbing one element's values would invalidate the
structure that holds it.

**Consequence, stated correctly.** Structural sorting is NOT forbidden. The
runner may structure the live data that way if it can.

The requirement is that **any such structuring must remain adjustable on a
temporary basis.** Analysis needs to change force values on specific elements,
including defaults, and observe the transformations that follow. A structure
that must be rebuilt to accommodate that, or that cannot be perturbed and then
restored, fails the requirement however fast it is.

This is the mechanism behind "the address is just an address" in section 4.5
and the ablation requirement in 4.7.

*An earlier version of this subsection stated that structural sorting was
forbidden outright as an optimisation. That was the planner converting a stated
requirement into a stronger prohibition than the owner gave, the same error as
the one corrected in section 5.1a. Recorded so it is not re-derived.*

---

## 5. Deployment model

Configuration happens at **install time**, not run time.

- The loader examines the system when the project is installed.
- It configures the binary or binaries for available GPUs and CPUs, either by
  custom compilation or by activating the correct prebuilt binary, whichever
  proves easier.
- Therefore **adaptable means parameterisable at build time**, not mutable at
  run time. Compile time structure baking is the correct shape, not a defect.

### 5.1a The install-time configuration agent sets the SNode field size

**Settled.** An install-time configuration agent is foreseen. Setting the SNode
field size per configuration is more than sufficient. **It is not a runtime
concern. It is an environment standard.**

Consequences, which close several open mechanism questions:

- Baking the constant into per-architecture bitcode at build time is the
  CORRECT shape. It was never a defect to work around.
- No runtime mutability of the SNode ceiling is required, or wanted.
- What remains open is the SIZING RULE, not the mechanism.
- **Verified, not merely precedented.** The existing build loop varies only the
  architecture, so one configure yields one ceiling across every bitcode
  artefact, with no change to the filename scheme. The host-side assertion
  compiles in the same configure. Nothing has to be built for the mechanism;
  the route already exists and already carries a configure-time value into the
  device bitcode at 21 consumption sites.

**On heterogeneous machines.** Hardware varies wildly, and the configuration
agent's job is to optimise the setup for what is available. Where a machine
holds several GPUs, the agent may need to choose between them, or to use more
than one.

This does NOT reopen the ceiling question. **Per build already means per device
configuration.** A two-card machine gets two binaries, and the system activates
one or both. More than one instance of the engine can run, especially across
more than one card. The filename scheme does not need to grow, and no common
denominator has to be found.

The development box in section 3.1 holds a GTX 750 and a GTX 1070, so this is
the ordinary case rather than an edge one.

**Concurrent operation is confirmed empirically, not inferred.** The project
owner has run the engine on both cards at once with separate work. There are no
blockers on concurrent cores. This is direct observation and outranks any
code-reading conclusion on the point; no agent needs to investigate it.

An earlier version of this section claimed the heterogeneous case reopened the
question. That was the planner adding a restriction the owner had not stated.

### 5.1 Current stage

Proof of concept.

- Absolutely minimal features are needed from any GPU. Raw speed is what
  matters.
- If it can in theory push to all backends for now, that is sufficient.
- Not all options have to be fully enabled at this point, but the **correct
  stub architecture must be built into the MVP base**. A wrongly shaped stub is
  worse than no stub.

### 5.2 Target tiers

| Tier | Hardware | Requirement |
|---|---|---|
| Baseline | Maxwell, 2 GB class. GTX 750. | Basic structure at 1024, with calculation optimised for 2 GB. |
| Mid | Pascal. GTX 1070. | Whatever suits the Pascal architecture. |
| Upper | Ampere. RTX 3060 as the minimum, being the primary card. | Proper architecture for the higher end structures. |

Beyond the upper tier, rough upwards scaling is expected. The architecture for
the higher end structures must be correct even where it is not yet enabled.

**These cards are not a vendor target.** They are the hardware the project
owner happens to own and can test on. They define FIDELITY CLASSES, nothing
more. Not capability classes: see section 2.2, hardware does not change what
the system can do. That all three currently sit on one vendor's path is incidental and must
not shape any architectural decision.

Section 2 governs: full hardware and vendor agnosticism was the primary reason
Taichi was chosen at all, and it is the property the fork exists to preserve.

**CORRECTION, 2026-09-09.** An earlier version of this section opposed "the
portable path" to "a vendor-specific path". That is the wrong axis, it was
mine, and an agent caught it. There are two compilation spines and neither one
is the vendor spine:

- The **LLVM spine** carries `x64` and `arm64`, which is CPU-only operation and
  a FIRST CLASS target under section 2, as well as CUDA, AMDGPU and DirectX 12.
- The **SPIR-V spine** carries Vulkan, Metal, OpenGL and DirectX 11.

Vendor lock is a property of individual backends, not of a spine. CUDA is
vendor locked. The LLVM spine is not, because CPU-only operation runs through
it. Treating the LLVM spine as "the vendor half" would deprioritise the
no-GPU target, which section 2 puts first.

The requirement, stated correctly: no backend may be privileged such that
capability differs across targets, per section 2.2, and CPU-only operation must
work. Where one spine is better served than the other, that is a gap in the
less-served spine to record, never evidence that it matters less.

### 5.3 Neither ignore modern hardware nor require it

Newer cards have more aggressive and more efficient memory management and
operation flows. That is a fact, and failing to allow for them would be
foolish. The architecture must be able to exploit them where they are present.

But **the project is meant to live primarily on the edge**, so it must work
across the board. Modern capability is an opportunity to take, never a
prerequisite to run.

This is the requirement that item 6.3, adaptive module loading, exists to
serve. It is also why the stub architecture in section 5.1 has to be correct
before the higher tiers are enabled.

---

## 6. Core work items

Settled. Ordered by dependency, not priority.

### 6.1 Parameterise the SNode ceiling

`taichi_max_num_snodes = 1024` at `taichi/inc/constants.h:12` must become an
**installation factor**.

Rationale: 1024 is unreasonably tight even for the 1070 primary card, and a
serious bottleneck beyond that.

**Why it is the project, stated as the mechanism rather than as a number.** No
engine is expected to hold or manage the whole construct. The primary
addressing protocol creates roughly 98 quadrillion distinct addresses, with
expansion possible beyond that. Any given workspace would only ever need a
FRACTION of it. But against a structure of that size, **1024 is not a fraction,
it is a fragment.** It is roughly one part in a hundred trillion, which is not
a sample of anything.

The cost is therefore **rebuild frequency, not capacity.** The working set is
composed for a scope (4.6), and the viewfinder moves across the construct
(4.0). With a working set that small, the window exhausts it almost at once and
every small movement forces another save and rebuild of the SNode tree. That
would be far too frequent for most use cases.

Raising the ceiling does not buy the ability to hold more of the structure. It
buys **fewer rebuilds**, and the saving is continuous rather than one-off. This
is also why the destroy-path defect in section 4.8 is graded reachable rather
than routine: its frequency is set by this ceiling, so raising the ceiling is
what keeps it rare.

**CORRECTION, 2026-09-08.** An earlier version of this section stated the cost
of the constant as three static pointer arrays totalling 24576 bytes. That was
wrong by orders of magnitude. Round one adversarial review corrected it. Both
adversaries on territory 03 compiled the runtime translation unit independently
rather than reading it, and agree on the following.

Verified findings:

- Enforced by assertion at `taichi/codegen/llvm/struct_llvm.cpp:266`. That
  assertion bounds a PER-TREE SNode count, while the arrays it appears to guard
  are indexed by `SNode::id`, a process-global counter that is never recycled.
  These are different quantities, so the assertion does not guard the write.
- The three static arrays at
  `taichi/runtime/llvm/runtime_module/runtime.cpp:567-569` total 24576 bytes.
  This is real but is not the cost that matters.
- The cost that matters is `ListManager`. `sizeof(ListManager)` is 1048616
  bytes, driven by a flat `Ptr chunks[131072]` at `runtime.cpp:427-428`, and it
  is committed eagerly per SNode rather than on demand.
- **The constant is not the binding limit.** ListManagers are cut from a
  preallocated pool sized by `device_memory_GB`, which defaults to 1 at
  `taichi/program/compile_config.cpp:63` and is spent at
  `taichi/runtime/llvm/llvm_runtime_executor.cpp:607-632`. That pool exhausts
  at roughly the same point as the 1024 ceiling, and exhaustion is a grid abort
  rather than graceful degradation.
- **Raising `taichi_max_num_snodes` on its own is inert ON THE GPU PATHS.**
  Neither explore pass found the pool. Both adversaries found it independently.
- **The scope is by architecture, not by tier.** The 1 GiB pool ceiling is a
  CUDA and AMDGPU fact. Both call sites of `preallocate_runtime_memory` are
  arch-gated, and the pool flag is assigned only on the CUDA branch.
- **On the CPU path there is no pool at all**, allocation falls through to the
  host allocator, and that allocator has no ceiling. So on CPU the constant
  binds alone and **raising it alone works**. CPU-only operation is a first
  class target per section 2, so this is a real result, not a footnote.
- **The two halves of the pair cost very differently.** Verified three times
  independently. `device_memory_GB` is a public field on a `TI_DLL_EXPORT`
  global config that `Program` copies at construction; the Python binding
  returns a reference to that same global, so Python is a client and not the
  owner. The pool knob therefore needs no build-time mechanism. The constant
  does, because it is baked into per-arch bitcode by a standalone clang command
  that inherits no project definitions. **The expensive half is the constant.**
- Constraint on that mechanism: the runtime filename encodes the architecture
  and nothing else, so threading a second parameter through it either grows the
  filename scheme or fixes one ceiling per build.

**MEASURED on the development box, not inferred.** Both adversaries queried the
CUDA driver API directly and independently, rather than reasoning from the
tree, and got the same result. Two sources.

| Device | Card | Compute capability | Memory-pool attribute |
|---|---|---|---|
| 0 | GTX 1070 | sm_61 | 1 |
| 1 | GTX 750 Ti | sm_50 | 0 |

The installed driver is CUDA 12.2, well above the 11.2 threshold, so the
**device attribute alone decides it, and it splits the two tiers**.

- On the **baseline card**, root buffers, ndarrays, argpacks AND per-launch
  staging buffers all share the same 1 GiB bump allocator as the element lists.
  The staging buffers were missed by both reports and by every adversary until
  round three; they enter unconditionally on AMDGPU. Every capacity figure in this
  section is therefore an UPPER BOUND on that card, not a target.
- On the **mid card** they do not share it.
- **This does not expire on a driver upgrade.** It is a hardware attribute. Two
  reports imply the gate might lift with a newer driver; it will not.

Consequence for the sizing rule in 8.1 item 2: the baseline tier's effective
capacity is lower than the raw per-node arithmetic suggests, and the shortfall
is structural rather than temporary.

**The requirement is unchanged by any of this.** The project owner has made no
assertion about what must change internally. The requirement is that 1024 be
settable to higher values *effectively*. The pool is part of what "effectively"
costs, and establishing that is what the investigation is for. It is an answer
to item 6.1, not an argument against it.

**On the nature of the 1024.** It was not an arbitrary cap when it was set. It
is now a somewhat false one for most cards. The measured facts are consistent
with that reading: the default pool holds 1020 element list headers against a
ceiling of 1024, so the two sit on top of each other, and neither scales with
installed memory. On modern hardware both are false ceilings, and they are
false in a linked way, which is why moving one alone achieves nothing.
- A second undocumented ceiling exists, `kMaxNumSnodeTreesLlvm = 512`, sizing
  two further runtime arrays, with no assertion anywhere in the tree.

Still disputed between the two adversaries, and not settled: the exact header
capacity at the default pool size, and the treatment of the bump allocator's
alignment padding.

### 6.2 64 bit addressing

The internals have **no 64 bit architecture for the addressing** to accompany a
raised SNode count.

This is a width change that must hold consistently end to end. Anywhere it
silently remains 32 bit becomes a truncation that only appears at scale.

Related constant: `taichi_max_num_indices = 12` at `taichi/inc/constants.h:5`.

Note for the stub architecture: 64 bit integers in SPIR-V are a declared
capability, not a given. The architecture must be able to express a target that
lacks it.

### 6.3 Loosen the architecture for adaptive module loading

The rest of the architecture needs to be loosened to allow adaptive modules to
be loaded.

This applies to backends. Different hardware generations carry different memory
and programming architectures, and the more modern ones can be exploited at
scales the older ones cannot. Adaptive module loading is what lets the right
implementation be loaded for the hardware the install time loader detects,
rather than compiling to a single lowest common denominator.

**Finding, from the territory 04 amendment pass. CONFIRMED by both round-three
adversaries independently.** The existing capability vocabulary in `taichi/inc/rhi_constants.inc.h`
consists entirely of feature bits and version numbers. It can say whether a
device CAN or CANNOT do a thing. It has no way to say that a device does a
thing faster, or at finer granularity.

That is a structural mismatch with section 2.2. An adaptive module system keyed
on that vocabulary would be selecting on capability, which 2.2 forbids, rather
than on fidelity, which is what is wanted.

Metal is the one backend carrying a complete detect, publish and consume chain,
so its MECHANISM is the model. Its CONTENT is not: what it publishes is feature
admission, and below a hardware generation threshold it withholds 64-bit
integer support outright.

Consequence to weigh before item 6.3 is scoped: this may need a fidelity
vocabulary that does not exist yet, rather than a wiring job on the one that
does.

### 6.4 Design constraint on all of the above

What is needed is **simple, layered exceptionally well, for blazing speed**.
Separation, not abstraction. Anything proposed for addition must justify itself
against removing something instead.

**The sole issue is how many calculations can be packed in to run the one set
of instructions at maximum hardware speeds.**

**The cost ordering, and it is a test for any proposed change:**

> **Declaration is near free. Calculation has a cost. Allocation is expensive.**

Prefer declaring. Accept calculating. Avoid allocating. A change that moves work
down that ordering is an improvement; one that moves work up it needs to justify
itself against the alternative.

**What it explains, already in this plan:**

- The 1 MiB pointer table inside each list manager is a DECLARATION and ought to
  be near free. It is expensive only because it is committed EAGERLY, which
  converts a declaration into an allocation. That, not the constant, is why the
  measured cost came out at roughly a megabyte per node against the 24 bytes the
  constant implied.
- The pre-allocated particle budget (4.8a) pays allocation once at configure
  time and never again, turning per-tick churn into slot claims.
- Rebuild frequency (6.1) is the cost of the ceiling precisely because a rebuild
  is allocation, which is the expensive tier.
- The centroid rollup (4.4) is calculation, paid once per group per tick, which
  is the middle tier and the correct place for it.

The functions themselves are incredibly simple. Nothing in this project is
hard because the maths is hard. Every core item exists in service of packing
density and dispatch throughput. Any proposed change is judged against that,
and nothing else.

---

## 7. Deferred

Not immediate concerns. Recorded so they are not lost.

| Item | Reason deferred |
|---|---|
| Backend specific work | Parked. See `modernization/deferred-backends.md`. Will return attached to the install time loader, since hardware detection feeds backend selection. |
| Cruft stripping | This is optimisation, not an immediate concern. |
| Inverse gravity push | Irrelevant to core work. |

---

## 8. Open questions

Not settled. Must not be resolved by assumption.

Split by who resolves it. The planner does not return items in 8.1 to the
project owner as questions; they are work.

### 8.1 Determined by work

1. **Map of 32 bit assumptions. LARGELY DONE, with a stated remainder.** The
   map exists across the frontend IR (territory 01), both codegen paths
   (territory 02), the runtime (territory 03) and the spelling classes
   (territory 06). It grew every time it was checked: territory 01 alone went
   from roughly twenty sites to fifty-four, and territory 06 then found
   forty-four more absent from it, out of a constructor class of seventy-three.

   Two things bound it. First, the sites found are spread across at least ten
   naming routes, and the count of routes depends on a granularity rule nobody
   had declared. Second, one surface is not closeable by searching at all: host
   arithmetic overflow in plain integer locals, which needs a compilation
   database and a narrowing-conversion warning. See section 10.

   The correct reading is that the map is good enough to size the work and is
   not, and probably cannot be, exhaustive by inspection.
2. **Replacement for 1024. CLOSED.** The sizing rule is: **optimise for what is
   there.** The install-time configuration agent measures the environment and
   sizes to it. There is no fixed number to choose and no policy decision
   outstanding; this was the planner manufacturing a question out of something
   that only needed stating.

   What remains is arithmetic, not judgement, and territory 03 has already
   produced it: roughly 1 MiB per element-list header, roughly 5 MiB per
   populated sparse SNode, against the memory actually available. On the GPU
   paths the constant and `device_memory_GB` move together; on the CPU path the
   constant binds alone. The pool half needs no build mechanism, the constant
   half does, and section 5.1a settles that route.
3. **Engine SNode configuration.** The specific structure composed for working
   scope, and whether the child link appears in it at all.

### 8.2 Requires the project owner

0a. **Does section 2.2 judge inherited code, or only proposed changes?** Raised
   by an agent that declined to answer it, correctly. Section 2.2 is written as
   a TEST to apply to a proposed change: a change that lets some hardware do
   what other hardware cannot is a violation. Metal's capability gate withholds
   64-bit integer support below a hardware generation, which is exactly that
   shape, but it is inherited behaviour rather than anything we propose.

   If 2.2 governs inherited code, every such gate in the tree is a defect to
   fix and the scope grows accordingly. If it governs only what we add, they
   are facts to work around. The agent recorded the question rather than
   picking, and it will recur wherever an existing gate is found.

0. **Does the SNode half of item 6.2 violate section 2.2?** Raised by an
   adversary applying the plan's own invariant. Restoring the shelved
   capability and USING it for SNode addressing give different answers to the
   2.2 test. A device without the buffer-address feature would lose SNode
   addressing entirely, which is hardware changing what the system can do, not
   how fast it does it.

   **Narrowed by measurement.** Both adversaries independently measured the
   development box and both state that every target clears every clause of the
   guard, including the baseline Maxwell card and the CPU software renderer.
   So restoring the capability **would not split the current target set**, and
   2.2 is not violated on the hardware in hand. The question survives only for
   a future target that lacks the feature. It is an owner decision about how
   far the agnosticism guarantee is meant to reach, not a source question.

   *Planner note, recorded so it is not repeated: the hypothesis that Vulkan
   1.2 core promotion had expired this blocker was mine and is confirmed wrong
   by both archaeology agents and one adversary. The original 2022 guard
   already accepted both the core version and the standalone extension.*

   *Converged finding, both adversaries, one withdrawing its own prior ruling:
   the pointer-width collision in the SPIR-V buffer helper IS architectural.
   The value tag that would have disambiguated it is erased by every arithmetic
   instruction.*

   **RESOLVED by territory 07, one adversary reporting, the second still out.**
   The sentence that followed, recording the remedy as a change to the
   backend's value model, is withdrawn.

   A discriminator DOES exist at the decision point. The inherited "none
   exists" sentence was reached by a check that INSERTS a default entry as a
   side effect of looking, so the evidence for it was manufactured by the test.
   The working rule is the buffer-type map crossed with the capability flag,
   both already in scope at that line.

   **The conditional is settled empirically.** The rule fails only if the root
   buffer must itself be physically addressed. Measured through the Vulkan
   loader on all three devices:

   | Device | maxStorageBufferRange |
   |---|---|
   | GTX 1070 | 4294967295 |
   | GTX 750 Ti | 4294967295 |
   | llvmpipe (CPU) | 134217728 |

   A 32-bit byte offset reaches 4294967296. Both cards cap a single bound buffer
   2 MiB below that, so on the two GPUs a device address buys nothing: their
   maximum single allocation is smaller still.

   **THE CPU PATH IS THE EXCEPTION, and it is a first class target.** The
   software renderer caps a descriptor-bound buffer at 128 MiB, but a device
   address uses no descriptor, so physical addressing lifts one tree there from
   128 MiB to 2 GiB. A factor of sixteen, on the no-GPU path. One adversary
   concluded physical addressing is never needed here; the other showed that
   holds on the cards and fails on the renderer.

   **The 4 GiB ceiling is permanent by specification, not by hardware.** The
   field reporting it is a `uint32_t` in the bundled Vulkan headers, so no
   Vulkan device can ever advertise more, and both cards report exactly the
   field's maximum. It will not lift with newer silicon. Descriptor-bound
   access on the SPIR-V spine is capped there forever, and physical addressing
   is the only route past it.

   The map-plus-capability rule holds. What would break it is not `Root`
   landing on both sides, which a uniform policy handles, but the fabricated
   `{Root, {-1}}` entry: under any root-physical variant that becomes a raw
   pointer forged from a workgroup access chain. Fix the fabrication first.

   **Blast radius: 11 sites in 2 files, 7 requiring an edit.** Against 147 call
   sites and a value-model change.

   *Also converged: the index narrowing lives at `taichi/transforms/type_check.cpp`,
   force-casts every array index to i32 and hardcodes i32 for three loop-index
   statements, unconditionally and on every backend. Its stated justification,
   that some backends lack 64-bit integers, has expired on this hardware while
   the code narrows regardless.*

1. **Team composition and workflow. CLOSED.** Settled and running. The method
   is section 9. Roughly sixty agents have run across seven territories. It did
   resolve item 8.1 items 1 and 2, as expected.

## 9. Working method

The basic format is **adversarial**.

### 9.1 The loop

1. **Explore phase.** Parallel agents, each armed with this brief and a scoped
   assignment, investigate the current codebase and note every place the
   proposed changes in section 6 touch.
   **Every territory is worked by a PAIR**, two agents covering the same ground
   independently and blind to each other's files. This is a competition, not a
   division of labour. A single contender per territory produces an
   unfalsifiable report. Two produce divergence, and divergence is the signal.
2. Each agent keeps **distinct, contemporaneous notes** as it researches,
   written as it goes, not reconstructed afterwards.
3. Each agent writes its **final assessment to its own file**.
4. **Adversarial phase.** **Two adversarial agents per pair of reports.** Each
   territory's A and B reports are scrutinised by two independent adversaries,
   who compare the two reports against each other, and against the other
   adversary's analysis, to determine whether the work is correct and complete.
   Four territories therefore means eight adversaries, all parallel. A
   territory's adversaries are dispatched as soon as both halves of its pair
   have landed, not after all territories finish.
5. Where the primary assessments leave them uncertain, the adversaries review
   the contemporaneous notes.
6. Where major revision is required, both reports go back with the combined
   notes attached.
7. **The loop repeats until general consensus is reached.** Revised reports
   re-enter adversarial review. The same adversaries return, so that they must
   judge whether their own objections were actually met, and must answer the
   places where the explore agents adjudicated against them.
8. Once consensus is reached across ALL territories, and not before, the
   reports are read together and **the planner writes up the consensus.**
9. **The written consensus then goes back out to every agent, who verify it
   against source.** This is the final round. The consensus is a synthesis
   written by the planner, and a synthesis introduces errors of its own: over
   this project the planner has been the single largest source of relayed
   wrong figures and over-extended rulings. The synthesis gets the same
   treatment the reports got, from the same agents that produced them.
10. Then it is discussed with the project owner.

**Timing.** Step 8 waits for every territory. Do not begin the write-up while
any territory is still moving, because the synthesis would be stale before it
was distributed, which is exactly the failure that left 32 statements in one
territory carrying a withdrawn framing.

The planner holds the arbiter seat on the adversaries' verdict.

### 9.2 File layout

| Path | Contents |
|---|---|
| `modernization/PROJECT-PLAN.md` | This document. Planner writes, nobody else. |
| `modernization/deferred-backends.md` | Parked backend items. |
| `modernization/investigation/notes-NN-<slug>.md` | Contemporaneous notes, one per explore agent. |
| `modernization/investigation/report-NN-<slug>.md` | Final assessment, one per explore agent. |
| `modernization/investigation/adversary-NN-1.md`, `adversary-NN-2.md` | Adversarial analysis, two per territory. |

### 9.3 Round one assignments

Four explore agents, split by layer, with deliberate overlap at the seams so
the adversaries have something to test.

Four territories, each worked by a pair. Eight agents, all parallel.

| Pair | Slug | Territory |
|---|---|---|
| 01 A/B | `ir-types` | Frontend IR and type system. Where index and address types originate. |
| 02 A/B | `codegen` | Codegen, both LLVM and SPIR-V paths. |
| 03 A/B | `runtime-struct` | Runtime and struct layer. Owns the 1024 static arrays. |
| 04 A/B | `backend-build` | Backend, ahead-of-time and build architecture. Adaptive module loading. |
| 05 A/B | `shelved-64bit` | Why upstream shelved the partial 64-bit work. History, issues, design notes. |

Pass A files carry no suffix. Pass B files carry a `b` suffix, for example
`report-01b-ir-types.md`. Neither pass may read the other's files.

Round one is expected to resolve the three items in section 8.1.

## 10. Standing instructions for dispatched agents

**When two careful enumerations disagree on a count, suspect the rule before
the count.** Two agents swept the same territory and reported eight spellings
against six. An adversary established the disagreement was not about the tree
at all: neither had declared what makes two spellings distinct, and under one
rule stated in advance the answer is thirteen. Declare the granularity rule
before you count, or the number cannot be compared with anyone else's.

**One surface is not closeable by searching, and an adversary established
what it needs instead.** Plain C++ integer locals and parameters in
implementation files have no textual signature: the exemplar everyone cited is
`auto`-declared, and the territory holds roughly three times as many `auto`
declarations as textual integer candidates. The class that narrows through a
constructor is already closed, because every single-argument constructor in
that territory is explicit. What remains open is host arithmetic overflow, and
closing it needs a compilation database and a narrowing-conversion warning, not
another grep. Do not dispatch a search at it.

**What this project is actually after: assumptions tested, and files nobody has
opened.** Not thoroughness over ground already covered. Every finding that has
moved this project came from the same move, testing the assumption underneath a
search rather than running the search again:

- A file-local macro turned a bare token into a type, so no search anyone had
  run could see that file at all. Found by asking what OTHER spellings exist,
  not by searching harder for the known one. It carries the thread index on
  both compilation spines.
- A grep on a constructor call was offered as bounding a class exactly. A
  brace-initialised form of the same constructor is invisible to it.
- An assertion inherited through four documents tested whether a map held an
  ENTRY, and never what the entry's VALUE was. The two sides hold different
  values. That is worth two orders of magnitude on one item's cost.
- Two questions the tree could not answer were answered by querying the driver
  and the Vulkan loader on the actual machine, and one of them split the two
  target cards permanently.

So: when you inherit an assertion, ask who tested it and how. When a search
returns a bounded set, ask what the bound cannot see. When the tree cannot
answer, ask whether the machine can. **One file nobody has opened is worth more
than a second pass over one everybody has.**


1. Do the specific task assigned. Nothing beyond it.
2. On any uncertainty, or any cause for creative extrapolation, **stop and
   report to the planner**. Do not resolve it yourself.
3. Do not decide that something is unnecessary. Do not remove or bypass
   anything on the grounds that it looks surplus.
4. Do not add abstraction or generality. See 6.4.
5. Report findings with file paths and line numbers.
6. **This plan is a living document and it changes under you.** Re-read it
   before you finalise, not only when you start. State in your report which
   date-stamped version you worked against. This is not hypothetical: section
   5.2 was corrected mid-round and two reports were amended against the
   superseded wording, leaving 32 statements carrying framing the plan no
   longer holds, one of them asserting the plan supplies a ranking it does not.
   Where the plan contradicts an instruction you were given, the plan wins and
   you say so.
7. **A verified citation does not verify the claim attached to it.** Confirming
   that a quoted line says what you quoted certifies the citation only. If the
   sentence it supports is a quantifier over the tree, or an inference about
   reachability, the citation cannot test it and must not be marked verified.
   This failure has already occurred in this project and was self-diagnosed.
8. **Grading an obstacle: architectural or environmental.** Requested by an
   agent, because the two words were being used inconsistently and the grades
   were therefore not comparable. Apply this test:

   *If every external thing were ideal today, would the obstacle still be
   there?*

   - **ENVIRONMENTAL** if it exists because of something OUTSIDE this codebase:
     a driver, a hardware generation, a dependency version, the state of an
     external specification at a point in time. It expires when that external
     thing changes, and it can be retested. Say what would have to change.
   - **ARCHITECTURAL** if it exists because of a decision INSIDE this codebase
     about how something is represented or structured. It does not expire. It
     goes away only by changing that decision.

   **Architectural does not mean hard, and environmental does not mean easy.**
   Grade the KIND first, then state the BLAST RADIUS separately and concretely,
   in files and call sites. An architectural obstacle touching two call sites is
   cheaper than an environmental one requiring a dependency bump across four
   submodules.

9. **Derive every count from its own enumeration.** Do not write a total
   alongside a list and trust that they agree. This project has produced at
   least four separate instances of a heading contradicting the rows beneath
   it, including two by adversaries, one by an amendment agent, and one by the
   planner. A reader must be able to reconcile any number you state against the
   list it summarises. Where practical, check it mechanically, and **state the
   rule that generates the figure.** A number nobody can reproduce under any
   rule you can state is worthless even if it was once correct. Withdraw it and
   recount rather than carrying it.

   **Corollary, earned the hard way.** Before trusting a count, check that the
   enumeration can SEE the whole class. Territory 01 ran a search keyed on one
   spelling through two surveys, three adversarial rounds and two amendment
   passes. A constant type in that territory has an implicit 32-bit
   constructor, so a real site can be written with no width named anywhere on
   the line. Four such sites survived every round. A count is only as complete
   as the thing that generated the list, and a filter can both over-collect and
   under-collect at once. State what your search can and cannot see.

   **A scope ruling removes work, not evidence.** If a site is excluded from
   scope but falsifies a claim the report makes about the tree, the claim stays
   qualified. Do not delete a true sentence to tidy the bookkeeping. Two agents
   independently refused a ruling of mine on exactly this ground and were right.
10. Nothing you conclude enters the project plan. Only the planner updates it,
   and only after a decision is discussed and settled.
