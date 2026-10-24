# Coherent prepared structural updates

Status: **accepted in #184 under #162, 2026-10-22**. The contract was
accepted in [#167](https://github.com/Human-Cognome-Project/human-cognome-project/pull/167#issuecomment-6342453057)
and merged at `284b45d`. This extends the accepted [#166 backing/binding
baseline](PREPARED-EXECUTION.md). #162 and #156 remain open pending their
respective acceptance and remaining integration work.

A prepared Harness can now change its occurrence records, fields, ordered
participation and contacts between complete ticks and continue using current
execution state. `update_prepared(expected_generation, commands, count)`
returns one committed generation or an ordinary rejection. It does not run a
particle force/integration phase or reset surviving bodies.

This is the agreed numerical-fixture foundation. Supplied starts for **new**
rows are available only through the private test adapter in `engine/tests`.
The structural command has no position or velocity fields. Production
nearest-unoccupied placement/initialization, source/owner/report integration,
focus replacement/rebasing, general LoD exchanges and private-manager behavior
remain outside this slice. FixedStudy/ActiveSession retain their historical
behavior. No database or Python runtime is added.

The [#183 Segment A extension](SOURCE-COUNTS.md) is accepted in #184: optional
exact structural mass/count provenance commits with source generation and
survives ordered-row compaction, without changing the legacy f32 arithmetic.
Its resource/evidence record follows this historical baseline separately.

## Reference and commit boundary

[Commands and references](../src/field/prepared_update.h) distinguish full native
handles from command-local indexes naming new definitions in the same batch.
A potential address names one execution occurrence, independently of its
reusable source definition. Two uses of one definition never select one another
for overwrite. Surviving mass, radius, kind and body/control state are immutable
at this structural boundary; replacing a body requires a new record and explicit
retirement.

Participation replacement/removal and contact removal name a ledger row and the
expected committed generation. Inserts name an index in the final ledger;
unchanged rows retain their relative order. Ordinals remain source metadata and
may repeat. Duplicate row edits, insertion positions, potential claims and
invalid dependencies fail together. Complete handle comparison uses the owned
committed headers; only this boundary and initial test population mutate the
underlying maps. No native address lookup or second identity registry is needed
for ordinary reference validation.

Preflight builds the entire prospective view in reserved host buffers. Every
retained relationship must have present endpoints; retirement has no implicit
cascade. Selected field handles remain fixed and must retain positive finite
mass. Candidate numerical work reads current backend values, including sleepers,
and writes only scratch. Numerical rejection therefore precedes native claims.
Empty and wholly no-op batches perform no physical dispatch/transfer and keep
history and generation unchanged.

Current/final logical demands and old-plus-pending claims are checked
independently. A retirement cannot fund a claim in the same batch. The reference
peak is

```text
(P_live + P_claims) + (G_live + G_claims)
+ 2*max(E_old,E_candidate) + 2*max(C_old,C_candidate) + selected_F
```

Candidate row indexes only name scratch values. Actual ownership/reuse comes
from the existing native NodeManager, and command results resolve to its handles.
This adds no allocator or free queue. After preflight:

1. Claim available particle/field records and retain their resolved handles.
2. Initialize new rows, clear retired rows, publish affected derived/control
   values and wake effects using prepared native bindings.
3. Publish ordered relationships and all four CSR directions, then recycle
   retired endpoints, with one native collection per affected map batch.
4. Synchronize, publish counts and advance the global generation once.

The caller exclusively owns this synchronized boundary. No tick, inspection or
callback may overlap its intermediate stages. Unexpected backend/invariant
failure marks execution unready; current tick, inspection and further updates
refuse use. There is no automatic restart or healthy partial generation.
The last successfully completed inspection is historical evidence, not recovery
state. Failure injection covers the boundary before claims and after claims,
numerical writes and topology writes. RAII retains the existing binding-before-
array destruction order and owned map cleanup; Program-owned activity layouts
remain scoped to the Runtime.

## Numerical repair and activity

Changed existing fields reduce prospective members' current positions around
their old published centre in ledger order. New fields use the existing
first-member anchor. Repair recomputes inclusive mass and uses the shared
[strict centroid finalization](../src/field/centroid_math.h), including widened
final addition and f32 rounding residue. A fresh reduction already contains
member motion: it does not add old pending residue again. Empty nonselected
fields retain the existing zero-mass convention.

Untouched fields are identified by their logical ordered contribution sequence,
not shifted ledger rows. Commit skips their numerical values, residue, history,
flags and awake state. It never writes survivors' position, velocity, force,
determiner, Moving/PriorMoving or displacement history. BulkCopy reads backend
state rather than host staging; the tests deliberately poison stale host staging
before an update to check this. CpuDirect uses its stable borrowed arrays.

New bodies and the live union of old/new affected members and edited contact
endpoints wake through the existing native particle set. A share/removal edit
can wake members with no centroid movement. Unrelated sleepers remain sleeping,
and existing awake bits are retained. Affected centroid results are published
once; ordinary motion then activates centroids through the established phase
order. There is still one native awake set per calculable type.

Universal mass is reduced in actual present-record order. Selected focal values
use the established ordered relative-centroid arithmetic and fixed handles/frame.
When radial damping is enabled, changed universal mass or focal origin wakes all
present bodies during commit. This wake is already present when the next
Activity::begin recalculates focal controls; writing the new origin cannot hide
it. These global dependencies are tested separately from local edits.

## Reservation and inspection

The opaque mirrored byte reserve is replaced by actual typed backend arrays.
Host-only headers/commands are charged as host copies; candidate derived values
are backend-only and fixed focal references have one host copy. No duplicate
blob or unused candidate bank is retained. [The manifest](../src/field/prepared_storage.cpp) inventories candidate
headers/ledgers, exact command/result/reference types, CSR/count/cursor storage,
validation marks, scratch-to-native destinations, claim inputs, new-row staging,
status and inspection outputs. It uses physical sentinels for empty extents.

The derived extent is `14*max(G,1)+12` f32: eight group values plus six
residue/history values per field, universal mass and eleven focal controls.
New-row staging is `12*max(B,1)` f32: eleven body values plus its initial speed.
This is bounded new-row input, not a copied survivor bank. Validation is
`max(2*P+G+E+C,1)` i32. Complete numerical/activity readback is reserved
before readiness. Inspection stamps its generation only after successful
readback and exposes live counts/handles, ordered ledgers, body/group values,
determiners, native presence/awake flags, motion/centroid history and focus.

The binding reserve now derives from **151 parameters in 22 contexts**:
#166's 122/20 plus candidate 14 and commit 15. Each parameter reserves at most
24 ABI bytes and the actual scalar/array metadata widths. CUDA additionally
reserves device arguments and per-context minimum buffers/pointer storage.
Actual binding payload and unused reserve remain separate manifest entries.
Insufficient scratch and unsupported typed axes fail before native layouts.
The per-axis workspace limit is `2^30` elements; B is limited to `2^20`.

The two accepted capacity profiles now reserve:

| P | G / E / C / R / F / B | Scratch, both CPU paths | Actual bindings / reserve |
|---:|---|---:|---:|
| 70 | 9 / 280 / 140 / 928 / 9 / 32 | 110,964 bytes | 8,128 / 9,664 bytes |
| 4,096 | 257 / 16,384 / 8,192 / 53,762 / 257 / 256 | 4,815,924 bytes | 8,128 / 9,664 bytes |

V remains 1,048,576 and 16,777,216 respectively. The raw manifests include
all retained payload/copy terms. These totals supersede #166's historical
reservation figures.
As before, scratch is a payload/binding reservation, not a process-wide quota:
C++ container/object overhead, manifest metadata and compiler memory are outside
it. Native roots, chunk/pool alignment and reservations are measured separately.

## CPU evidence and real costs

[Retained acceptance, source hashes and raw output](../tests/prepared_updates)
pin the tested implementation and CPU backend. The six update suites cover:

- 32 update/tick cycles per profile in both host paths: P=70 and P=4,096,
  aggregate letters, byte-only and mixed forms (320 cycles altogether).
- Simultaneous occurrences with separate handles/state and their own shared
  field contributions; a further copy enters while ticking. A dedicated English
  case copies every sibling/value-parent/membership binding, then modifies and
  retires one complete occurrence while preserving the other two and immutable
  source content. Projection fixtures retain
  per-occurrence membership deduplication and repeated source ordinals.
- Nonmonotonic addresses, holes, full maximum batches, every usable record,
  previously unused blocks, complete retirement and native reuse. The mixed
  fixture has one spare field slot, so it reuses that field; the other profiles
  exercise simultaneous old/new field claims and retirement.
- Bitwise survivor and untouched-field/history preservation, an exact local-f32
  pending-motion repair, independent inclusive masses/centres, and a subsequent
  tick comparison with established kernels. Reseeded trajectories alone are
  not the history oracle.
- Same-centre share changes, removed contributions, contacts on both index sides,
  true unrelated sleepers, global U/focal changes and correct particle phases.
- Independent P/G/E/C/R/B failures including peak claims/reference demand,
  stale/foreign/dangling/conflicting references, selected mass loss, no-ops,
  unsupported focus changes and pre/post-mutation failures. Existing execution
  tests cover F and pre-layout S rejection, zero capacity and binding guards.

Measured first/maximal updates, ordinary/rejected batches, ticks and inspection
observe zero recurring linked C++ new/new[] calls. Native requested bytes,
chunks, elements, pool calls/reservation, aligned cursor, trees/SNode IDs and
compilation counters stay unchanged. Setup, first update, ordinary updates,
calculation and inspection timings are reported separately. These observations
do not cover arbitrary direct malloc, residency/page faults or driver internals.

Structural work is not free. The current implementation compares field
contribution sequences in `O(G*(E_old+E_candidate))` host work; insertion-position
checks scan bounded commands for ledger rows (`O(B*(E+C))`). `structural_visits`
counts header/ledger comparison visits, not every memory access. `index_visits`
counts source/index passes in candidate group CSR and all four committed CSR
families, excluding zero fills/transfers. Uploaded/read-back byte counts and
explicit boundary synchronization/collection counts are returned; device API
internal barriers are not counted. Candidate reduction visits current members;
untouched fields are copied to scratch but skipped at commit. Native map
bounded duplicate checks remain O(B²), general lookup visits present references,
and serial native GC is worst-case O(P+B) per retirement batch. These are
structural boundary costs, not a replacement particle scheduler.

## Reproduction and remaining scope

Build the matched native fork and workspace as in [the backing recipe](PREPARED-EXECUTION.md#reproduction),
then run:

```sh
ctest --test-dir engine/build --output-on-failure
. engine/build/engine-env.sh
engine/build/prepared_update_test
engine/build/prepared_update_test --direct
engine/build/prepared_update_test --large
engine/build/prepared_update_test --large --direct
engine/build/prepared_update_test --letters
engine/build/prepared_update_test --letters --direct
```

With a matched CUDA-enabled build and local GPU, run the plain, `--large` and
`--letters` commands with `--cuda`. Tests print the actual backend and reject
CPU fallback; CpuDirect is CPU-only. Record GPU/driver, native/device pool mode,
actual driver allocations and transfers under #145. These CPU results do not
qualify GPU execution, driver allocation, pinning, browser interaction or failed-
device recovery. The structural fixture foundation does not implement #156's
production placement and source-to-report lifecycle.
