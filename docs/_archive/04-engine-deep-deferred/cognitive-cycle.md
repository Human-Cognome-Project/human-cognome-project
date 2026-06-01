# The Cognitive Cycle

NAPIER runs on a fixed cognitive **clock** — but the clock is a heartbeat for the *outside world*,
not a throttle on cognition. Getting this distinction right is essential.

Sources: claims 265 (11 ms clock), 274 (tick is reconciliation, not work), 273 (envelope is a
db query+filter set), 267/268/269 (modality streams), 270 (timely interrupt), 275 (sufficiency /
temporal greedy-LoD), 271 (the envelope is large), 276/277 (deeming policy).

---

## The clock: ~11.1 ms, ~90 Hz

> *Sensory and cognitive data run in cycles on a fixed cognitive clock of ~11.1 ms per cycle
> (~90 Hz).* — claim 265

If the engine receives a distinct input type and processes any outputs every 11.1 ms, it
maintains the same clock rate as human cognition. Input elements are **batched across cycles** so
they arrive at appropriate frame rates (paced like a sensory framerate), **not** as data gluts.

**Crucial:** the fixed cadence is a **heartbeat, not a throttle.** The engine never waits or idles
between inputs. When foreground load is lighter, the **spare capacity** is used for **background
processes** — the subconscious / cold-shard cross-link crawl (claim 52), consolidation. A steady
human-rate beat with background work absorbing the slack.

A side effect worth naming: this **times NAPIER to human cognition**, which matters for ToM
co-timing in dialogue and for running the perception/imagination cycle (claims 253/254) at a
life-like rate.

---

## The tick is reconciliation, NOT work

> *The 11.1 ms tick is a **reconciliation / I-O signal, not a work signal.** NAPIER works on
> whatever it deems appropriate — cognition is self-determined and continuous, not tick-gated.* — claim 274

This **corrects** any reading of the clock as pacing cognition. Between reconciliation beats,
NAPIER does **thousands of self-determined actions at native speed** (claim 271). The cycle exists
for the *outside*: input management and conversational fluidity with other entities — a ~90 Hz,
human-cognition-like scheduling framework for **input cycling.** Sensorimotor impulses are
received on the cycle and emitted on an **extended** version of it.

So: the tick marks where NAPIER **reconciles with the world** at a human-compatible rate. It does
**not** pick the workspace and does **not** pace the cognition.

---

## What NAPIER works on each moment: the envelope

> *"Envelope" is used in the oldest db-function sense: the combination of **queries and filters**
> that shape a **workspace.*** — claim 273

This is literal, not metaphor (no-analogy-default, claim 13). An HCP activity envelope is a
query+filter set defining the working space loaded for a context (the fluid-Venn cognitive imagery
of claims 39/41/50 sits *on* this precise db meaning). What NAPIER works on is an envelope — the
query+filter combination that shapes a bounded-n workspace. Envelopes are **self-selected and
continuous**, *not* one-per-tick. (The deeper composition mechanics of inner envelope *deeming* are
not yet fully specified — see [../06-status/deferred-and-open.md](../06-status/deferred-and-open.md).)

---

## The 11 ms envelope is large

> *11.1 ms is a **large** work envelope at native C++/GPU speeds.* — claim 271

It is a full 90 Hz game frame, and games run whole-scene physics + AI + render per frame on
consumer hardware. Each cycle is **thousands of bounded-n db/physics actions.** The contrast with
current AI is stark: an LLM can take a minute or more on one answer (an unindexed full-weight
pass); ~60 s is ~5,400 cycles, and a 6-minute loop is >30,000 cycles — tens of millions of native
actions NAPIER could run in that time. The envelope is large **because the design curates what
enters each cycle** (bounded-n; tiering, claims 120/121; reduce-n). The limiter is latency, not
compute.

---

## Modality streams and flexible scheduling

- **Per-modality separation** (claim 267): each data stream — video, audio, other inputs — is
  packaged separately, each separable from or batchable with others. The *process* handling each
  modality is flexible/open (e.g. Taichi might provide visual while a separate process handles
  audio). The invariant is the per-modality **separation**, not the engine assignment. Effect: the
  engine is never **starved** (something to process each cycle) and never **glutted** (no dumps).
  When time is intrinsic to the data (video/audio framerates), the streams **self-pace** the feed.
- **Flexible per-input frequency** (claim 268): *any* input can be mapped at *any point* in the
  cycle, each at its own appropriate, **adjustable** frequency. Not every input arrives every
  cycle — a defensive/safety-critical feed might run every single 11.1 ms tick while others are
  paced slower. The cycle is a **flexible scheduling frame, not fixed slots.** Substrate note:
  modalities reduce to **physics** (audio is waves of force), so the physics substrate is genuinely
  modality-universal (claims 42/192) — no bespoke per-modality processes required.
- **Chunked and streaming inputs both fit** (claim 269): a **discrete/chunked** input (a text
  prompt is event-driven — unseen until the sender hits enter, then dropped in as a complete unit)
  and a **continuous stream** (audio/video parsed across cycles at its framerate) both fit without
  special-casing, because any input maps in at its appropriate adjustable frequency. Because the
  engine is *always cycling at human rate* rather than waiting for input, it supports natural
  conversational adjustment — turn-taking, mid-exchange correction — not the rigid prompt-response
  lockstep of a text-only model.

---

## Why streaming beats written: timely interrupt

> *Letting someone finish an incorrect thought **calcifies** it; verbal/streaming communication
> allows **timely interrupt** — correction before the thought completes.* — claim 270

When a wrong thought completes, it hardens into a committed structure and becomes load-bearing
(you build on it), so later correction means tearing down everything stacked on top. The
always-cycling streaming mode lets the engine interrupt *before* calcification — internally
(revise a forming inference before it commits; live-rebalancing, claim 80) and in dialogue (ToM
co-regulation, claims 82–83; active-listening, claim 15). Correction cost rises sharply after
calcification.

---

## Frequency is sufficiency, not maximal resolution

> *Frequency is set by **sufficiency for a continuous impression**, not maximal temporal
> resolution.* — claim 275

The ~90 Hz baseline is **tunable** — higher when appropriate, and the **extended (slower) cycle**
applies both ways (input sampling *and* output emission drop to slower rates when that suffices).
Like a human, NAPIER does not need every-nanosecond placement of an object in flight (unless the
task demands it) — it needs enough snapshots to form a continuous impression. This is greedy-LoD
(claim 16) applied to **temporal** sampling: sample only as finely as the impression requires,
finer only on demand. The world model **interpolates between snapshots** to yield continuity — the
same gap-filling as perception/imagination (claims 253/254, see
[../02-architecture/world-model-and-imagination.md](../02-architecture/world-model-and-imagination.md)).

---

## Who decides what gets a tick: the deeming policy

> *For all who-decides questions: **for now, us** (Patrick + team set the policy); **in future,
> NAPIER may self-determine**, but that is left open.* — claim 276

The **deeming / prioritization** layer — which feed earns a tick, what NAPIER attends to out of a
huge concurrent input field — is a **further gear** above the input-cycling mechanism. The
mechanism is built; the deeming policy is **human-set for now.** Concretely, the current policy is
(claim 277): **load balancing** (distribute inputs across available capacity) + setting each
feed's **frequency** to what seems necessary for the current need. Not a learned/autonomous
attention model yet — pragmatic, capacity-aware, need-driven scheduling with humans setting the
dials.

> The **deeper deeming/weighting math** — what gets evaluated, with what weight, under what
> circumstances — is **explicitly deferred** and is the project's acknowledged math gap (claim 286).
> See [../06-status/deferred-and-open.md](../06-status/deferred-and-open.md).

---

## See also

- [reconciliation-loop.md](reconciliation-loop.md) — the mechanism the tick actually reconciles
  through.
- [../02-architecture/world-model-and-imagination.md](../02-architecture/world-model-and-imagination.md)
  — the world model the cycle steps forward.
