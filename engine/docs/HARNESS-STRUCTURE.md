> **Recovered September 2026 record.** This document preserves the native-engine development/vetting context in which it was written. Absolute `/opt/project/...` paths, agent-routing instructions, and statements about branch/commit status are historical. Current paths/status are indexed in [README.md](README.md); current repository policy is in the root `AGENTS.md` and `CONTRIBUTING.md`.

# Harness structure (open skeleton)

**Status: provisional and OPEN.** This is the incorporation of the model so far
(see `HARNESS-NOTES.md` for the literal model record and `VETTING-PLAN.md` for
the vetted engine facts). It is deliberately left open: more components are
coming — several **CPU-resident functions** — and the DB source location, exact
payload layout, and other inputs are **not yet wired**. Nothing here is locked;
extension points are marked `[OPEN]`.

Design laws this obeys (from `HARNESS-NOTES.md`): cost model
(declaration < calculation < allocation), physics-as-analytical-tool (adapt
only without distorting the display), one-kernel replication for the primary
calc, CPU **follows, never searches**, no special-cases (gates live in the
equation).

## Layers

- **Engine** — fixed built artifact (`engine_support`); mechanism only. Vetted.
- **Harness** — this target; links `engine_support`. Holds the CPU-resident
  functions + the GPU kernels + the host boundary + the instance lifecycle.
- **User terminal / API** — drives within the structure via arguments; can
  request shut-down/restart through the harness.

## Components

### CPU-resident functions (in the harness) — the growing set

These run on the engine's `cpu_threads` in C++ (Python stays front-end only).
`[OPEN]` = a slot Patrick will fill.

1. **DB traversal** — follow-only (no search) over the flat `token_id` arrayed
   store, n-dimensionally via **child listings**.
2. **Composed-tree construction** — build the SNode grouping(s) / LoD-rollup
   **chains for the current analysis** from the flat store (any required shape;
   live `add_snode_tree`).
3. **Amalgamation orchestration (= the next-tick setup)** — end-of-tick: drive
   the per-field **force/mass-centroid → virtual particle** roll-up (kept in the
   tick tail, reusing hot data; see `HARNESS-NOTES`). The minted centroids ARE
   the next tick's m2 sources — this is the whole of the next-tick setup.
4. **Focus / neighbourhood selection** — choose the analysis focus and which
   chains expand to granular vs stay at their primary centroid. `[OPEN — detail]`
5. **Marshalling / boundary** — stage positions (and the minimal working set)
   host⇄device; `upload`/`readback`.
6. `[OPEN]` **— further CPU-resident functions (Patrick, several coming).**

### GPU kernels

- **Primary kernel (one, replicated per particle):**
  `Σ (m · m2 / d²)` over active fields' virtual particles → **resultant
  velocity** → **exponential overshoot-attenuation + d=0 gate** → **move**
  (`x += F_net/m`, 1:1, momentum carries). Uniform, **branchless**, one launch,
  distinct name.
- `[OPEN]` **Auxiliary kernels** — e.g. the centroid/amalgamation compute if it
  runs on-device, any setup passes. TBD as components land.

## State

- **Fields:** dense, `token_id`-addressed arrays (fits engine dense fields +
  i32 indices; capacities chosen up front, frozen at instantiation).
- **Composed SNode tree:** any shape, built live from the flat store; tree ids
  recycle. Per-node SNode ids are monotonic — budget capacity or use the
  restart authority to reclaim (see vetting P2.3).
- **Positions = the dynamic state** and the **save state** (checkpoint =
  positional data). Everything else (predicates/fields/forces/child-listings)
  is standing structure that recomposes.

## Lifecycle

- The harness owns one `engine::Runtime`. Startup capacities + device-memory
  pool are chosen **before** construction and **allocated once** (cost model).
- **Argument-level (no recompile, no restart):** force on/off toggles, sizes,
  loop bounds, focus parameters — all launch args.
- **Restart (harness-driven):** only a capacity change or SNode-id reclamation
  — i.e. a fundamental structural update / virtual-node promotion beyond budget.

## Tick loop (provisional)

1. Compose / refresh the analysis chains (CPU traversal, as needed).
2. Launch the **primary kernel** over the active set (device-resident).
3. **In the tick tail — this IS the next-tick setup (Patrick, 2026-09-13):**
   amalgamate each active field → virtual particles, whose centroids **are the
   m2 sources the next tick reads**, reusing hot data — does not gate next tick.
   There is no separate setup step; the amalgamation closes the loop on its tail.
4. Advance. Readback positions only when a save/inspection is needed.
   - `[OPEN]` the **"2 things added per tick"** — the amalgamation (= next-tick
     setup) is settled; whether it is one of that pair and the identity of the
     other are not yet pinned.

## Work decomposition — for Sonnet assignments (Patrick, 2026-09-13)

Sonnet is good at **linear coding** but **requires validation** and **does not
handle large structure well.** So:

- **Opus 4.8 (me) owns the structure and the decomposition** — I hold the
  architecture and define each unit's interface, inputs/outputs, and invariants.
- **Sonnet gets small, linear, fully-specified units** — one self-contained
  sequential task at a time, no large-structure judgment required.
- **Every unit is validated** — tests + review before it is trusted (tests on
  everything; mandatory, since Sonnet needs it).

Initial unit breakdown (grows as components land; each = one Sonnet assignment
with its own tests):

- `U1` — CMake harness target linking `engine_support` (skeleton; builds empty).
- `U2` — Instance lifecycle wrapper (capacities up front, one pool, restart hook).
- `U3` — Dense `token_id`-addressed field storage + payload layout (once fixed).
- `U4` — **Primary kernel IR** (`Σ m·m2/d²` → resultant → exp attenuation + d=0
  gate → move); distinct name; thread count on the top loop. The one replicated
  kernel.
- `U5` — Host boundary (upload/readback positions).
- `U6` — End-of-tick amalgamation (virtual-particle mint) — kernel/orchestration.
- `U7..` — the **CPU-resident functions** (traversal, composition, focus, …) —
  one unit each, defined as Patrick specifies them.

I sequence to the cost model and define interfaces so units compose without
Sonnet doing structural work; each is validated in isolation before the next.

## Deferred / open (do not wire yet)

- DB **source location** and other external inputs (Patrick: not yet).
- Exact **payload layout** — leaning to a list of `(field token_id,
  participating-mass)` entries + position, not a fixed-slot row; my call to firm
  up.
- **Contact mechanics** (Patrick flagged; distance done, contact pending).
- **Numbering → analytical coupling** (deferred).
- Exact **algebraic form** of the exponential attenuation and the d=0 gate.
- The additional **CPU-resident functions** to come.
