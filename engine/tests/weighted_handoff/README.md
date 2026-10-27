# C2b-2 weighted native handoff CPU evidence

Implementation/evidence **for review**, released under accepted #191/#192.
See [behavior, resources and limits](../../docs/WEIGHTED-NATIVE-HANDOFF.md).
The production engine/fork and live hierarchy remain unchanged.

Runtime/test source: `bb5ba2ed33ae68651e9e1dd9980c76382f63fac7`.
C2b-1 accepted merge: `41a7b25bd45d21b4ae9a3ca28e4ff2062de4c245`.
Backend tree: `6784224121f68de1bf1aa0765b54e80d20e355ad`.
Clang/LLVM 15.0.6, Release, CPU-only matched build; CTest concurrency two.
The later evidence/documentation commit changes no tested source. Contributor
evidence is separate from hosted review/acceptance.

All **92 CPU suites passed in 930.44 seconds** on that source. The 29,184 input
transitions reached the checked published reference in **1–5 ordinary polls**,
then stayed quiet through eight additional polls. This is held-input numerical
convergence, not coupled physical settling.

| Artifact | Contents |
|---|---|
| [acceptance.json](acceptance.json) | Pins/hashes, full regression result, four mode matrices, compiler access mapping, representative acquired packets, root references, typed inventories and native counters. |
| [cpu-ctest.txt](cpu-ctest.txt) | Complete 92-suite CPU regression, including all previous 88 suites. |
| [cpu-native-output.txt](cpu-native-output.txt) | All four weighted native modes from that same run, preserving raw sample/control/cost order. |

Four modes cross one/two native CPU workers and BulkCopy/CpuDirect source paths.
Each checks actual CPU selection and uses one persistent numerical probe across
18 source fixtures/layouts. There are 76 cases per mode, 304 total. Each case
has 32 reset/reuse rounds and three input changes: 29,184 transitions overall,
alternating positive/negative coordinates between rounds. A 16-poll limit is
followed by eight quiet observation polls for every input.

The layouts cover both row orders, same-word and adjacent/distant blocks, final
valid row 69, two/three levels, forward/reverse five-node chains, fan-in and a
permuted 63-node binary tree. Exact quarter weights, thirds, repeated ordinals,
source aliases, compatible overlap and independent occurrences retain validated
integer coverage. The explicit quarter-weight witness has two normalized
movements of `3h/4`; their sum crosses the parent's Q_4 cell. Ordered controls
also cancel complete opposing inputs before Q and preserve pending native work
on both sides of the consume cut.

Every poll checks each observed child packet against its complete pre/post-poll
publication, with a matching selector. Parent arithmetic uses those actual
acquired inputs for its independent reference. Raw `acquired_input` records
sample the first non-cancelling input of the single-notification/leaves-only case,
including complete observed/before/after XYZ/mass. They are representative samples;
every acquired packet is checked even when it is not serialized. Final root
values/reference traces are retained for all input stages. Structured root
records coalesce duplicate identical finals across notification/activity variants;
the raw output retains every occurrence.

The audit inspects actual retained Program LLVM before launcher executable copy.
It maps native argument-buffer offsets and indirect child-array data flow, checks
every required selector path/clone, and rejects ordinary accesses or demotion.
Each mode retains one own acquire, three indirect-child acquire sites and one
publication exchange, all SC. Selector data is argument 1 at ABI offset 24;
indirect indexes derive from argument 3. This is bounded CPU qualification,
not a CUDA memory-ordering or arbitrary future-kernel guarantee.

Seven bad bindings leave the probe unbound without mutating source/publication:
output alias, stale field handle/source row, cycle, missing root connectivity,
incompatible coverage and unsupported anonymous grouping. A committed source
edit and an inert source-only tick reject stale polls before further native
work, then permit explicit rebind. Those existing provenance observations never
select per-node work. No coupled moving-body response is installed.

Source state preservation hashes inspect actual native physical/control/history/
activity state, full source headers/counts, frame, generation, completed ticks,
Demand and universal mass. Only explicit fixture perturbations/updates occur
outside those preservation windows. Reference arithmetic uses long-double
rational weights/first moments, with error bounds for f64 reduction and exact
quantized-cell checks. No bound becomes an activity threshold or positional tail.

The measured `visits`, `publications` and `notifications` are native-work results.
Structured `derived_*` read totals are separately labeled logical derivations:
each leaf is seeded/consumed once per input, has no incoming child notifications,
and each fixture has a uniform internal child degree. Thus source reads equal
inputs × normalized leaf references; child reads equal measured non-leaf visits
× that degree. They are not independently serialized per-row counter totals.

The probe owns 29,280 host and 29,280 native array bytes, 2,592 prepared binding
bytes and 7,000 host before-poll/reference bytes. Native slot/selector storage is
68N bytes, with N=70; H=140 and R=512. Source execution manifests/shapes, 35,680
bytes of retained source fixture tables, graph copies/input headers and fixed
objects are separate. Source scratch is 305,692 bytes, including its 9,776 actual
binding bytes and unused binding reservation. Ordinary source/probe memory and
initialization use do not acquire new backing/layout/compiler state.

Each poll inspection reads 14,280 bytes. Configuration uploads 15,000 bytes;
snapshot/initial-values uploads are 2,240/4,480 bytes. Cases record 128 snapshots,
32 initializations and 153,440 candidate-query/inspection readback bytes, separately
from poll and initialization inspections. Source fixture transfers are described
in the behavior document; they are separate from probe-only kernel/inspection
timings. First-poll, initialization, setup and total elapsed costs are recorded
under concurrent CTest execution, not promoted to throughput guarantees.

Before/after metric columns are native requested bytes, known list chunk bytes,
known allocator elements, pool allocation calls/requested/raw calls/reserved,
aligned host cursor, tree count, issued SNode IDs and compilation seconds. Each
source fixture retains equal first-use and 32-round reuse values. Guarded native
boundaries observe zero linked C++ new/new[] including workers. This excludes
arbitrary malloc, drivers, residency/page faults, setup construction temporaries,
compiler/runtime objects and reference stack space. Host vectors are unpinned.

The original C2a unsafe controls and all earlier tests remain in the regression.
No browser/report interface changed, so browser tests were not rerun; existing
native capture checks remain in the full CPU suite. No database or GPU execution
is claimed. Production all-reader lifetimes, response ownership, one combined
integration and coupled multilevel settling/recovery remain a subsequent plan.
#172/#177/#156/#145 remain open.

Reproduce using the [native commands](../../docs/WEIGHTED-NATIVE-HANDOFF.md#resource-accounting-and-reproduction).
