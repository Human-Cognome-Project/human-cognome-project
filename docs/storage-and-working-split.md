# Storage construct and working construct

Working notes, not a specification. Patrick's statements recorded as given;
lines marked **derived** are mine and need confirming. Nothing has been built.

2026-09-13 — companion to `parent-structure-notes.md` and
`field-physics-and-tick-notes.md`.

## Two constructs, not one

**The storage database is explicit at every level.** It carries its own parent,
child and sibling relationships, so the CPU can traverse it n-dimensionally at
exactly one level. Nothing is elided and nothing is implied: every level is
materialized and every relationship is present to be walked.

The n here is the n of commonality — a dimension being any agreed plane of
comparison between two or more points, of which the three the simulation is
drawn in are one agreed set and not a privileged one. Storage is walked across
the commonality dimensions; the device carries the three. See
`field-physics-and-tick-notes.md`, "What a dimension is".

**The working SNode tree is a compression.** It shows whatever compression of
intermediate steps is most effective for the budget and the analysis intended.
It is a projection chosen per analysis, not a mirror of storage.

So the earlier question about how far a membership reaches up the containment
chain has two answers, and the split is where they live. In storage, every
level, explicitly. In the working tree, whatever the chosen compression keeps.

## This is the mandated split, not a new one

`physics-basis.md` cites the prior paradigm arriving at "identity lives on the
CPU; the GPU knows position" and calls it the storage split the architecture
mandates. This is that split in operation: identity, structure and full
relational depth on the CPU side; position, mass and motion on the device.

**Derived.** Building the working tree is therefore a projection step, taken
per analysis rather than per tick. The tick reads a working tree that already
exists. What varies between analyses is which intermediate levels survived the
compression.

**The return path is the ledger.** The pairwise tax of discovery is paid once
and ledgered permanently: a relationship found in operation becomes explicit in
the database and is a lookup from then on. That is why storage can be explicit
in every relationship without every relationship having been known up front.
Granularity is the individual pairwise connection.

### The database compresses as it fills

Two mechanisms, both algorithmic:

- **Greedy LoD.** Repeated substructure is promoted into its own construct and
  referenced rather than repeated. The denser the data, the more repetition
  there is to find, so the compression improves with the thing that would
  otherwise cost more.
- **Delta-only storage.** A construct stores only what it adds over the
  construct it references. This is already visible in the byte-cluster
  construction: each level is a new element plus the next smaller block, so a
  long sequence costs one element and one reference per level rather than the
  whole sequence at every level.

**This compression does not trade against accuracy, and that is unusual.**
Compression normally buys space by losing fidelity. Here it cannot: a composite
is an *exact* rollup of its parents, and a delta is an exact difference. So
both curves move the right way as density increases. Faster because there is
less to hold and less to traverse; more accurate because centroids are better
determined and more relationships are explicit.

### Four independent reasons depth pays

They act on different things, which is why they compound rather than overlap:

| Mechanism | What it does |
|---|---|
| Viewer-bounded activation | Bounds the work per tick, so the active set does not grow with the store |
| The discovery ledger | Retires work permanently, driving the pairwise tax toward zero |
| Greedy LoD and delta storage | Shrinks the store itself per unit of content |
| Stabilization | Cuts the number of ticks an analysis needs |

**Stabilization.** As a construct stabilizes, analysis against it gets cheaper,
because the effect becomes mathematically obvious within a few ticks. A settled
structure is already near balance, so a query does not relax from nothing: the
perturbation resolves and the answer shows itself quickly.

**The first three cut work per tick or size of store. The fourth cuts ticks.**
Total cost is ticks multiplied by work per tick, and these attack both factors,
so the improvement is multiplicative rather than additive.

It is also the return on the settling dynamics and the brake. A system built to
converge is a system whose analyses terminate early, and the same jitter
suppression that keeps fine structure from tearing is what lets an answer
become obvious instead of oscillating.

It also gives depth a second reason to pay. Viewer-bounded activation keeps the
active set from growing with the store; the ledger drives the discovery cost
toward zero as the store fills. One bounds the work per tick, the other retires
work permanently.

## The engine consequence, and it is a startup decision

This is where the split touches the runtime configuration, and it has to be
settled before an instance exists rather than after. Read from the live engine
source, not from the workspace documentation.

### What the update changed

The engine was a 32-bit engine that has taken a theoretical 64-bit addressing
update together with raises to the SNode capacity. In the working tree of
`/opt/project/taichi`, uncommitted:

- `taichi_max_num_snodes` (1024) and `kMaxNumSnodeTreesLlvm` (512) are **gone
  as compile-time constants**. Both are now runtime configuration, validated at
  tree load and at structure compilation with messages naming the offending
  identifier and the configured limit.
- The runtime's preallocation is **computed from the capacities** by
  `runtime_get_memory_requirements`, rather than baked into a fixed struct.
- `ListManager` no longer embeds its chunk directory. It was a flat
  `Ptr chunks[131072]`, one mebibyte inside every manager. It is now a
  two-level directory: 2 KiB of roots plus a lazily allocated 4 KiB page per
  512 chunks. That is roughly a five-hundred-fold cut in the fixed cost of
  having a manager at all, which is what makes many trees affordable.
- The directory pointers are published and loaded with 64-bit atomics, with an
  explicit workaround in the source for LLVM 15 being unable to lower an
  acquire atomic pointer load on Pascal. That path is exercised on exactly the
  card in this machine.

### The capacity arithmetic

From `runtime_get_memory_requirements` in
`taichi/runtime/llvm/runtime_module/runtime.cpp`, each capacity contributes
linearly, page-rounded at 4096 bytes:

| Slot | Arrays | Bytes per slot |
|---|---|---|
| SNode | three pointer tables | 24 |
| SNode tree | one pointer table, one size table | 16 |

Which puts the capacity-driven part of the preallocation at:

| SNode capacity | Tree capacity | Capacity cost |
|---|---|---|
| 1024 / 512 (defaults) | | 32 KiB |
| 8192 / 2048 | | 224 KiB |
| 1,000,000 / 100,000 | | about 25 MB |

A million SNode identifiers fits inside a single 32 MiB reservation granule on
this card. That is why `DEVICES.md` measured no change in reserved memory when
the capacities were raised: not a coincidence of the measurement, but a linear
cost too small to cross a granule boundary.

### The corrected recommendation

**Superseded.** An earlier version of this note treated identifier exhaustion
as a real constraint and offered planned process recycling as the alternative.
That was reasoning from the engine's defaults rather than from the update. With
the raises in place the ceiling is a budget line rather than a wall, and
recycling is not a consideration.

Set both capacities high at construction, sized to identifiers issued over the
whole process life including destroyed layouts, and treat the cost as tens of
megabytes rather than as a trade. The counter itself is `std::atomic<int>`, so
the type permits a little over two billion identifiers; memory binds long
before the type does.

**Worth carrying as risk, not as a defect.** The 64-bit addressing is
theoretical, meaning widened but not proven at scale. The paged directory and
its Pascal atomic workaround are the parts that would show a fault first, and
they are on the hot path for any structure with many chunks. Anything built on
this should exercise that path deliberately rather than assume it.

### The 1024 default is a 750 Ti concession

1024 works on the GTX 750 Ti because that card has a 64-bit addressing hiccup.
The GTX 1070 takes a raised capacity without complaint. So the default is not a
neutral starting point; it is the number that keeps the deferred card alive,
and the target card does not need it.

`DEVICES.md` already defers the 750 Ti for an unrelated allocation failure, so
nothing is lost by setting a working base the 1070 is happy with.

### What this workspace currently sets: nothing

Read from the source in `engine/`:

- `InstanceSettings` leaves both capacities as unset optionals, so an instance
  built with defaults inherits the engine's own 1024 and 512.
- `apps/engine_devices.cpp` exposes both as command-line overrides but sets no
  default of its own.
- `tests/engine_smoke_test.cpp` sets 4096 and 1024 in one case, purely to
  exercise the setting, and checks the environment path at 2048.

So there is no working base anywhere. Everything inherits the 750 Ti's number.

### Recommended working base

Sized against identifiers issued over a whole process life, with a working tree
projected per analysis, and costed from the arithmetic above:

| Setting | Proposed | Cost |
|---|---|---|
| `llvm_snode_capacity` | 1,048,576 | about 24 MB |
| `llvm_snode_tree_capacity` | 65,536 | about 1 MB |

Together about 25 MB, which still sits inside a single 32 MiB reservation
granule on the 1070, so it does not show up as a measurable startup cost at
all. A tree of eight leaves runs to roughly ten identifiers, so a million
identifiers is on the order of a hundred thousand projections in one process
run.

**Not applied.** This is a source change inside `engine/`, which this session
was told to read rather than modify. It is one default in `InstanceSettings`,
or one assignment at each construction site.

**Open.** The expected number of analyses per process run, if it would put the
base somewhere other than the proposal above.
