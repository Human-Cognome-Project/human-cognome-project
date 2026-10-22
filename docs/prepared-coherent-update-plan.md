# Prepared execution: coherent updates between ticks

Status: **draft for review, 2026-10-22**. This develops the remaining
coherent-update section of the [accepted #163 foundation](prepared-capacity-foundation-plan.md)
under [#162](https://github.com/Human-Cognome-Project/human-cognome-project/issues/162).
Baseline: `main` at `4e7b1932efe2527786a98660a54ef042c985b718`, the merge of
#166. This PR changes documents only; the update boundary below is not built.

## Accepted baseline and next result

#166 supplies [combined backing and reusable native bindings](../engine/docs/PREPARED-EXECUTION.md),
empty preparation, initial fixture population and 32 ticks through existing
kernels. Its origin/runtime/target-ABI correction was
[accepted in follow-up review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/166#issuecomment-6340084122).
All eight hosted checks passed at the accepted head `34c01e0`, including all
37 CPU suites. Its prospective workspace remains a reservation, not an updater.

The next result is one execution that accepts bounded structural changes
between complete ticks, publishes one coherent generation, then continues
ticking. Add and retire occurrence/field records, change participation and
contacts, and reuse released native records without replacing the Harness or
replaying survivors' supplied starts.

This remains a numerical-fixture foundation. New fixture starts stay behind
the private test adapter under `engine/tests`; structural commands carry no
position/velocity requirement. Production initialization/placement conventions,
nearest-unoccupied insertion, source/owner/report integration, general LoD
exchanges, databases and private-manager behavior remain under the separate
#156 scope. Focus selection remains fixed for this slice; unsupported selection
replacement/rebasing fails before mutation. #162 and #156 remain open until
their respective implementation and acceptance requirements are fulfilled.

## 1. Definitions, occurrences and references

[Patrick's carry-forward criterion](https://github.com/Human-Cognome-Project/human-cognome-project/issues/162#issuecomment-6340088128)
requires simultaneous independent uses of the same assembled definition.

| Identity | Meaning at this boundary |
|---|---|
| Immutable definition | Reusable letter/byte/assembly structure. Several live occurrences may refer to it. |
| Potential address | An execution location for one live occurrence, independent of its definition's cold/warm address. Two simultaneous occurrences use distinct locations. |
| Native record handle | Existing context/incarnation/address/record validation. It identifies one live numerical occurrence or field and remains valid across unrelated commits. |
| Relationship reference | A typed ledger row qualified by the expected committed execution generation. Compaction invalidates old row references; source parent ordinals remain separate metadata. |

Keep definition identities in the test/source adapter. The bounded runtime
boundary takes occurrence/field handles and transaction-local references to
new commands; it does not acquire a general identity registry or implement
held #123. Repeated definition values never select an existing live particle
for overwrite. Reusing a retired numerical slot must invalidate its old handle.

Membership targets deduplicate within each occurrence using the existing
projection/membership rules. Two occurrences in one membership field each
contribute their own mass. Distinct construction occurrences remain distinct
even when endpoint values or source ordinals repeat. Parent ordinal is neither
a globally unique relationship ID nor a numerical-slot ordering key.

## 2. Bounded commands and preflight

Introduce a distinct internal update operation on the prepared Harness. Keep
`publish`, `upload`, reseeding and focal rebasing rejected on this path.

Support definitions of new particle/typed-field records; insertion, removal and
share/endpoint replacement of participation rows; insertion/removal of explicit
contact pairs; and retirement of records whose prospective relationships permit
it. Surviving body mass/radius/type and physical state are not rewritten by
these structural edits. Replacing an occurrence uses a new definition and an
explicit retirement, with separate handles and peak capacity charges.

A batch supplies its expected committed generation and at most `B` fixed-width
commands. New-record references use validated command-local indexes resolved
through the already reserved result/reference buffers. Existing references use
complete native handles. Relationship edits include their family, current row
and expected generation; duplicate/conflicting edits are rejected as a whole.
New relationship insertions specify a bounded structural position in the
prospective ledger. Preserve unchanged relative order and exact source ordinals
through compaction; build CSR from that order, never numerical-slot order.

Preflight validates the entire prospective view: reference ownership and
incarnations, command dependencies, unique potential locations, allowed field
kinds, shares, finite supported fixture inputs, ledger positions and contact
endpoints. Every retained endpoint must remain present. Retirement requires
explicit removal/replacement of affected relationships; no implicit cascade
deletes other occurrences. Selected field handles stay valid and retain
positive participating mass. Reject unsupported focus changes and stale batch
generations before claims or committed writes.

Check `P/G/E/C/R/F/B` independently, including old plus pending particle/field
claims. A same-batch retirement does not finance a native claim while its old
record is still needed. Committed and prospective relationship copies each fit
their own `E/C` reservation; account for both copies in peak scratch rather than
requiring a second allocator. Use the conservative logical reference demand

```text
R_peak = (P_live + P_claims) + (G_live + G_claims)
       + 2*max(E_old,E_candidate) + 2*max(C_old,C_candidate) + F_selected
```

Check current and final reference counts too; the separate committed/prospective
copies are additionally charged by their actual scratch bytes. Repeated endpoint
occurrences count separately. Empty batches preserve generation and perform no
physical work. Wholly no-op edits also preserve generation; unchanged fields
are identified by their logical contribution sequence, not shifted ledger rows.
Ordinary invalid-input/budget results preserve committed values,
handles, readiness and generation.

## 3. Candidate storage, native claims and publication

Use the existing `PreparedStorage` committed/prospective headers, bounded
command/results/reference buffers and CSR workspace. Replace or partition the
opaque backend reserve with supported typed arrays/views during preparation;
do not leave an unused blob alongside unaccounted replacement arrays. Existing
native view facilities are preferred; setup-time typed allocations are valid
when no suitable lifetime-safe alias exists. Ordinary updates allocate neither.

Retain the native map's allocator and recycle/collect mechanism. Add batched
read-only reference/claim validation only where the existing interface requires
a narrow extension. Do not introduce another free queue or slot allocator.
After successful preflight, claim only previously available native records and
retain provisional handles until completion. No tick, callback or inspection
may observe an intermediate update. Unexpected claim/dispatch failure after
mutation begins follows the unready rule below.

Resolve new references, build the prospective ordered ledgers and all four CSR
families using reserved counts/cursors. Numerical repair reads current execution
values, including sleeping contributors. BulkCopy performs this work on backend
state and stages structural changes plus new fixture rows; it cannot download
survivors and upload their initial copies. CpuDirect operates on its stable
borrowed execution arrays with the same publication rules.

At the exclusive between-tick boundary, publish the validated ledgers/indexes,
new rows, affected derived values, counts and references; remove retired
relationships before recycling their endpoints. Clear retired numerical/control
rows and native presence/awake bits before they become reusable. Run native GC
once per affected allocator/bounded retirement batch. Complete the backend work
before incrementing the global committed generation once and returning results.
Map bookkeeping generations do not replace this cross-family generation.

Carry forward the measured map costs: bounded host duplicate checks are
`O(B²)`, general address lookup visits present references, and serial native
collection is worst-case `O(P+B)` per release batch. Use direct native handle
validation where available and measure remaining lookup/index/collection work.

There is one synchronized execution owner. Intermediate writes are protected
by that boundary, not a claim of lock-free publication or an overlapping queue.
Any backend/invariant failure after mutation begins marks execution unready
unless restoration is actually proved using reserved storage. Current ticks and
inspection then refuse use; the last completed inspection is historical. No
healthy partial result or automatic full-study restart is a recovery path.

## 4. Numerical repair and native wake effects

Refactor the existing strict reduction/finalization helpers for changed
topology; retain the force, contact, integration and precision rules. Repair
does not run a second solver or advance particle motion at commit.

For each affected existing field, retain its published centre as the reduction
anchor. Reduce the prospective participants' **current** positions and shares
in structural order, including sleepers; recompute inclusive mass, then use the
existing widened final-origin addition and residue calculation. Current member
positions already contain previous motion, so this fresh reduction absorbs the
old pending residue without adding that residue again. Publish the new pending
residue and previous-centre history for that field once. Untouched fields keep
their numerical values, pending residue and history bit-for-bit.

New fields use the existing first-member seed anchor and current participants;
this creates no body placement convention. The existing zero-mass convention
applies to empty nonselected fields. Selected fields cannot become massless.
Rebuild universal mass using the existing present-record reduction semantics;
record ordering and source contribution ordering remain distinct. The update
preserves every survivor's position, velocity, determiner and motion history;
only new/released rows and necessary derived/control state change.

Wake new bodies and the live union of old/new affected field members and contact
endpoints. Removal of a field contribution, a share change or contact removal
can require work even when a centre does not move. Do not depend solely on
`Changed` centroid geometry to detect those changes. Preserve genuinely unrelated
sleepers and retained awake bits; retirement removes only its own pending work.

Compute selected focal data using the existing ordered relative-centroid
arithmetic, with the same selected handles/frame. Changed universal mass or
focal origin requires broader activation when the radial dampener uses those
inputs. Test that case separately from local edits with unchanged global inputs.
Publish the required focal/control changes together with their wake effects so
the next `Activity::begin` cannot miss an update already written to control data.

Use the existing single particle awake set and single centroid set. Temporary
affected-row marks belong to validation scratch, not another persistent work
bank. Repair publishes its field result once and schedules necessary particle
work for the following particle phase; ordinary motion then activates centroids
through the established phase order. No particle force/integration body runs
early during contact propagation or structural repair.

## 5. Complete the reservation and inspection manifest

Keep the accepted `V/P/G/E/C/R/F/B` evidence profiles. Revise exact scratch and
prepared-binding formulas before implementation evidence; the #166 byte totals
are the old baseline, not a spare allowance for the updater.

The proposed candidate numerical extent is `14*max(G,1)+12` f32 values: eight
group values and six history/residue values per field, plus universal mass and
the eleven focal/control values. This replaces the old `8G+1` derived extent.
Use the `G` validation segment for temporary affected/change flags. Add bounded
fixture staging for at most `B` new body rows and their initial determiner/control
values; it contains no copied survivor bank and remains behind the test-only
initialization adapter.

Inventory the actual command/reference tag types, committed/prospective copies,
new-row staging, structural reorder indexes, validation/status outputs and
inspection rows. Derive each byte term from physical extents, `sizeof`/native
element width and retained host/backend copies. Update the prepared context and
parameter counts for all added operation families; validate actual binding
payload against the resulting reserve. Check overflow, layout limits and the
full scratch requirement before creating native layouts. No unexplained margin
or fallback allocation is an exhaustion response.

Extend bounded inspection to include generation, complete live handles/counts,
ordered relationship bindings, numerical values and relevant activity/history.
Reserve all output before ready. Formatting and optional readback remain
separately measured; existing ActiveReport/ActiveSession integration is later
work. Preparation ends empty and warms/provisions every new family, including
rejection, maximum batch/recycle capacity and the first previously unused block.

## 6. Implementation order and acceptance

1. **Validated prospective structure:** define fixed-width commands/references,
   finish the manifest and prepare native validation/index/inspection bindings.
   Prove ordinary rejection without claims or partial publication.
2. **Numerical commit:** add selective new/retired-row initialization, affected
   mass/centroid/residue repair and required wake effects to existing kernels.
   Commit across families at one exclusive boundary and preserve survivors.
3. **Repeated updates and failure evidence:** exercise at least 32 update/tick
   cycles in the same execution, both CPU host paths and both accepted capacities;
   submit reviewable native/boundary, numerical and proof commits.

Required cases include:

- Aggregate-letter, byte-only and mixed forms; nonmonotonic addresses and holes,
  unused native blocks, the last valid record, and repeated release/reuse.
- At least two simultaneous occurrences of one immutable assembled definition
  with distinct handles and independent numerical/activity state. Insert another
  copy while ticking; modify/retire one while preserving another. Each occurrence
  adds its own mass/participation; duplicate membership paths deduplicate only
  within that occurrence. Definition contents remain unchanged.
- Exact boundary preservation of survivors and untouched centroid residue/
  history, including a local-f32 pending-motion case. Independently expected
  inclusive masses/centres and comparison with established kernels after the
  next tick; reseeded golden trajectories alone do not prove history retention.
- Existing/new fields, changed shares and removed contributions, contacts on
  both index sides/across blocks, true unrelated sleepers and separate global
  mass/focal-origin activation. No wrong-phase force/integration work.
- Every budget exhausted independently at its applicable boundary while another
  relevant budget has spare capacity, including old-plus-new claim peaks and
  ordinary reference exhaustion. Fixed focal capacity and insufficient scratch
  are also checked at setup/initial binding; unsupported focus replacement is
  an ordinary update rejection, not a new selection policy.
  Reject duplicate/conflicting edits, dangling/stale/foreign refs, stale ledger
  generations, unsupported focus changes and selected-field mass loss atomically.
- Failure injection before mutation and after claims/partial backend writes:
  unchanged healthy rejection versus explicit unready state, refused current
  tick/inspection, and owned resource cleanup. No GPU fault-recovery claim.
- Setup, first nonempty update, each ordinary/rejected batch, calculation and
  inspection measured separately. Require no new native backing, layouts,
  compilation or recurring C++ control/launch allocation in the tested boundary.
  Record structural/index visits, transfer bytes and synchronization/collection
  work; bounded CSR rebuilding and native lookup/GC are real costs.

Run the existing native CPU regression suite. Pin source/backend identities,
new manifests and raw evidence; keep malloc, residency and driver-observation
limits explicit. Local CUDA commands must reject CPU fallback and record driver
pool/allocation behavior under #145. CPU success does not qualify GPU execution.

This draft requests review of the command/reference boundary, selective history
repair, global wake rules, peak reservations and repeated-occurrence acceptance.
It does not approve new production initialization or focus-selection conventions.
