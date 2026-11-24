# Live editable CPU study

Contributor implementation under #343/#156, following the
[#343 live-study handoff](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343#issuecomment-7137040893)
and the [November 24 direction in #425](https://github.com/Human-Cognome-Project/human-cognome-project/blob/dcbcef71f0ed1c1752935a20da28bdc93dc9c182/docs/core-local-study-continuation-plan.md).
Implementation/qualification pin: `36c387151ba2d9b6cbeecab6ee53b88764637a91`,
tree `a342200e1385a35f0238976d65ff3f5a7e9d88a0`, on main `98cdbe03`.
The preserved local `72cae3ed` has the identical tree. Main's #423 integration
changed five Markdown files from executable `01dc9783`; implementation source
matches the earlier preserved `04ab220c`/public `5b073db5` code exactly.

## Run one study and edit it

Build the matching native CPU SDK and configure the workspace with
`HCP_WITH_VIEWPORT=ON`. The CLI needs no database, warm client, browser or GPU.

```sh
cmake --build engine/build --target viewport_live_study --parallel 2
source engine/build/engine-env.sh
engine/build/viewport/viewport_live_study --threads 2 --direct \
  --commands tools/viewport/examples/live-study.commands
```

Omit `--commands` to read commands from stdin. `--help` prints the grammar.
The process creates one prepared English-pair study from the existing pure
letter fixture and English-pair preset. Its aggregate entries 2 and 10 start
live. The composition retains English A's ordered byte/nibble source structure;
the current execution uses the existing root-level aggregate binding and two
explicit comparisons. This is fixture input, not a retained database acquisition
or a new nested native representation.

```text
observe
tick 1
add 20 2
tick 1
remove 20
tick 1
list
quit
```

`add ENTRY [TEMPLATE_ENTRY]` introduces a distinct root occurrence of a declared
template; new entries default to template 2. A declared entry defaults to its
own template. `remove ENTRY` withdraws that live occurrence. Its definition stays
pinned and can create/reintroduce an occurrence later. `tick [N]` advances the
same study; `observe` and `list` expose its current completed state/catalogue.
An already satisfied presence request is a no-op. Ordinary refusals are printed
and subsequent commands can continue; the process returns 1 if any command was
rejected. No automatic restart follows a failed native mutation.

Each JSON line includes command/status, completed tick, native generation, full
live occurrence identities and context/incarnation/address/record claims, selected
owner metadata bytes/peak, and the existing completed observation. The latter
retains particles, fields, frame, comparisons and the existing combined diagnostic.
The adapter's display coverage follows the completed live mapping. Display or
captured values never become admission/placement inputs.

## Preset and bounds

The editable preset uses physical mass **2** and exact source mass **2** per
English occurrence, retaining the source baseline. Its geometry/comparison/focus
choices come from the existing English-pair preset: radius 0.125, root units 1,
SiblingDefinition English → ParentValue byte, comparison IDs 100/101, Q52, and
ParentValue focus. The older demonstration preset's physical mass 1.5 remains
separate. New occurrences copy declared intrinsic/source data, not a live body's
position, velocity or a saved capture. Native generated placement chooses the
nearest supported unoccupied position at the current focus.

| Configured surface | Editable preset |
| --- | ---: |
| Potential / particles / fields | 128 / 4 / 4 |
| Participation / contacts / references | 16 / 4 / 1,024 |
| Focus / ordinary batch / bootstrap commands | 2 / 32 / 32 |
| Cascade nodes / links / levels | 4 / 4 / 4 |
| Cascade references / coverage / live responses | 32 / 32 / 64 |
| Placement work / cells | 100,000,000 / 65,536 |
| Tick / geometry / hierarchy work | 1,000,000,000 / 1,000,000 / 1,000,000 |
| Owner metadata bytes / cumulative allocation work | 16 MiB / 16 MiB |
| Plan / owner / per-edit metadata work | 10,000,000 each |
| Native scratch | Existing required-scratch calculation + 256 MiB; actual value in capture |

The batch bound accommodates define/participation or relationship/contact removal
commands; the original batch-one preset remains unchanged. Instance key buffers,
native-address declarations, command/cohort scratch and next-map storage are
reserved before readiness, with capacity tied to the existing particle/batch/
cascade bounds. Native record allocation/recycling remains with NodeManager.
The metadata slot/index, occurrence entry, potential address and actual native
record are distinct identities. Retired instance metadata slots can be reused;
the original immutable source templates remain available.

The printed owner metadata live/peak bytes describe that reservation domain,
including setup overlap. They are not total native memory, compiler/ORC bytes,
RSS or admission credit. This slice does not revise general preallocation policy;
whole-field LoD exchange still needs actual old-plus-incoming peak observations.

## Authorization, commit and observation

`ActiveSession::edit_prepared` resolves the authorized prepared owner. Its edit
checks the logical session/study/generation and expected native generation before
preparing a bounded proposal. New identities name a full declared root template;
the owner supplies exact pins, physical intrinsics and ordered participation.
Definition, geometry, exposure and field topology are not arbitrary edit inputs.

Addition calls the existing private generated-admission path. Removal names the
current full native handle and exact participation/contact rows, then uses the
existing hierarchy-aware updater. Both operate synchronously between ticks.
After native success, the owner resolves actual claims and publishes its reserved
live identity/handle mapping through nonallocating assignments/swaps. Successful
edits advance native generation once and do not advance completed tick. Survivor
position, velocity, force, mass and extent remain bit-identical at that boundary;
following ticks continue the current state.

The immutable source is the definition/template catalogue. The owner's mutable
instance map is the current live set, including new root entries. Reports attach
the exact source pin to newly introduced identities; removal does not edit the
source definition or fabricate another warm recipe revision. Existing source
templates can be used even while their original occurrence is absent.

Prepared `ActiveReport` now carries native generation and completed-tick stamps.
`ActiveSession::accept` rejects a report as current after an edit or later tick;
detached historical values can still be inspected/serialized. Association loans
retain their existing generation/tick invalidation. The live view indexes the
reserved catalogue without copying pins during commit. Existing catalogue-address
assertions now compare the underlying table through that view; no source-pin or
stale-view assertion was removed.

Ordinary work/capacity/staleness refusals retain the ready model and mappings.
Default prepared submissions keep editing disabled. Unsupported last-cohort
removal preserves the fixed positive-mass comparison/focus subset. Unexpected
postclaim or publication failure makes the Harness/owner unready; it cannot be
used as a healthy partial edit. Numerical kernels, native allocator, Taichi fork,
force/brake law and publication primitive implementations are unchanged.

## Captured operator run and qualification

The [command file](../../tools/viewport/examples/live-study.commands) and
[raw eight-record JSONL capture](../../tools/viewport/examples/live-study-capture.jsonl)
come from the clean Clang two-worker direct run at the implementation pin above.
One process/session/logical handle survives all rows. Both original occurrences
remain active. The audit compares full native claims and all exported particle
properties at the two edit boundaries; the native test additionally checks all
eleven physical fields bit for bit.

| Command | Completed tick | Native generation | Live occurrences |
| --- | ---: | ---: | ---: |
| create | 0 | 1 | 2 |
| observe | 0 | 1 | 2 |
| tick | 1 | 1 | 2 |
| add | 1 | 2 | 3 |
| tick | 2 | 2 | 3 |
| remove | 2 | 3 | 2 |
| tick | 3 | 3 | 2 |
| list | 3 | 3 | 2 |

The configured native scratch is **272355732 bytes**. The selected owner domain
holds **11670 live bytes**, with **14179 setup peak bytes**, unchanged across
this trace. The executable SHA-256 is
`0f0de5c13380befcf9bb21f9a6331a32275c2163d48a1cfcd5d11e23c5c03add`.

A separate `remove 999; observe; tick 1; quit` capture returns the expected exit 1
for the ordinary invalid removal while retaining the same two occurrences and
completing tick 1. Its raw commands/stdout/stderr are in the local bundle.

| Local qualification | Result |
| --- | --- |
| Existing quick CI | 3/3 CTests plus placement math and placement capacity controls |
| Clang 15, warm/viewport ON | 23/23 focused CTests |
| GCC 11, warm/viewport ON | 20/20 focused CTests |
| Clang 15, warm/viewport OFF | 3/3 native core CTests |

All four runs pin the same clean source and finish with unchanged HEAD and
worktree. The full configurations cover all twelve new profiles: core owner,
public session and command-file CLI, each with one/two workers and bulk/direct
paths. The minimal build additionally covers core one-worker bulk/two-worker
direct and the existing owner control. Existing owner, submission, plan lifetime,
origin, completed snapshot, original preset and selected logical/diagnostic
controls are included. These 49 local CTests and two placement controls are
scoped CPU evidence; the complete hosted native/database/browser matrices
remain authoritative and are not claimed from this local run.

New native tests cover distinct entry creation, repeated removal/re-addition,
exact definition pins and fresh native incarnations; moving-survivor physical
bits; completed snapshots, stale reports/views and full path/study/owner checks;
one-short/exact metadata preparation, allocation work and bytes; native capacity
refusal and retired metadata reuse; and postclaim failure/unready behavior.
Linked C++ new/new[] guards cover recurring edits. The core witness separately
checks unchanged sampled native backing, SNode/tree and compilation counters.
These observations are not a complete arbitrary-malloc/device/RSS census.

The core edit test links only the native prepared-owner target and runs with
warm integration/viewport OFF. Public session and command-file tests separately
use the existing completed viewport observer. Pure ActiveAssembly/projection/
membership inputs remain independent of native execution; their sources/targets
are unchanged. No private/cold source or persistent database was accessed.

Source-pinned evidence: `/tmp/hcp-live-study-36c38715-evidence.tar.gz`,
SHA-256 `e06b251942cbcc5681a762be79df980a091d5d0c8054f976fe63b5dac0295771`. A fresh extraction verifies the root and all four
nested manifests. It includes commands/configurations, source and fork pins,
runtime bitcode and executable hashes, raw test/capture output, refusal and
development histories, the implementation patch, and a Git bundle preserving
local/public implementation commits. The root audit checks test selection/counts,
source cleanliness, captured identities, generation/tick transitions, source
baseline, survivor observations and refusal continuation. The CLI and minimal
core link checks show no PostgreSQL client dependency. The unchanged-source
check covers `engine/src/field`, `engine/src/engine`, `engine/taichi` and workflows.
The archive is machine-local; its hash does not make it remotely available. The
bounded operator command file and raw eight-record capture are committed for
remote review.

The initial presence-only CLI run exposed the immutable-catalogue display coverage
assumption after removal. The adapter now uses completed live identities and
declared-template display metadata; the existing snapshot coverage checks remain
strict. Development compile/interface/orchestration errors and their corrected
logs are retained separately from clean-source qualification. Post-PASS native
cache-lock warnings are retained without cache cleanup.

Return this bounded implementation for provisional maintainer review and
authoritative GitHub CI. Next is whole-field LoD exchange in the same study,
followed by the finer-level confounding/next-level structural trace alongside
targets and motion. Target/brake arithmetic, peripheral rollup, formal pattern/
PoC acceptance, general definition/revision/geometry adoption, GPU/concurrency
and complete resource admission are not qualified by this first operator slice.
All merges remain maintainer-only.
