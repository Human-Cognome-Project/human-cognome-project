# Coherent source and live-hierarchy refresh (C3-2)

Status: **accepted in #197, 2026-10-29**, reviewed head `01b8928`, merge `0eea42a`. This develops
slice A of the [core continuation](core-local-study-continuation-plan.md#a-coherent-sourcehierarchy-refresh)
and the accepted [#194 structural-refresh contract](production-hierarchy-response-plan.md#4-initialization-structural-changes-and-failure).
It changes no executable behavior. Owners remain #172/#177, with subsequent
operational admission under #156 and hardware qualification under #145.

The [C3-2 implementation](../engine/docs/LIVE-REFRESH.md) is accepted and merged
in #198 at `5ea9c76`; its complete hosted CPU result and retained earlier failures
are recorded separately from this planning contract. CPU contact-safe ticks are accepted in #201; native placement/admission
remain the active continuation. Planning-baseline instructions below retain
their original scope and do not reopen C3-2.

The result is one prepared execution that can commit a source edit and its
replacement calculation tree between completed ticks, then continue with its
surviving state intact. Operational letter Add will build on this boundary
together with native placement, contact-safe ticks and an enduring owner.

## 1. Reviewed results and implementation baseline

Planning baseline: main `9e3076d5e815f3fb3732c8441dc149509baaf573`.
C3-1 is accepted in #195 at merge `6edb9d4`. The next implementation must use
current main including #196's accepted corrections (merge `68e4104`).

The [#196 source review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/196#pullrequestreview-5705358350)
at `1af1cacef98d34928d8b0bbbbb5753c6790c61fa` found no new blocking source
defect. Its tested code is `758ad053629a6ca9beea5422de659aa38ba15e48`.
The [retained local evidence](https://github.com/Human-Cognome-Project/human-cognome-project/blob/1af1cacef98d34928d8b0bbbbb5753c6790c61fa/engine/tests/cuda_local/README.md)
and all 16 entries in its artifact checksum manifest were inspected for this
plan. Local results and independently inspected hosted results are separate:

| Record | Result and limit |
|---|---|
| CPU on the CUDA-enabled substrate | 100/100 tests, 2398.13 seconds. This is separate from a CPU-only substrate build. |
| Actual GTX 1070/sm_61 | 4/4 qualification suites, 423.86 seconds; ten requested C3 selections report eleven passing case results. Default C3 CUDA admission remains guarded. |
| Same-execution capture and browser | 64 GPU ticks, nine captures, f64 round-trip and observation neutrality; 89 browser checks. Automated fixture/watch checks do not prove human observation of a running study. |
| NAS integration | 180 checks; retained cold-table counts/fingerprints unchanged. No new cold-data write is needed. |
| Mixed-precision boundary | f32-to-f64 promotion of `0x1p-149` becomes zero. This remains a failed, independent backend case. |
| Hosted CPU-only validation | Run [40958048270](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/40958048270/job/123743238316) completed successfully on `1af1cac`: **97/97** in 1482.34 seconds, all four extra no-unroll modes, 180 integration checks and actual x64 backend verification. All nine hosted checks passed. This configuration has a different test inventory from the local CUDA-enabled build. |

The maintainer independently verified hosted completion and accepted/merged
#196 at `68e4104`. This plan is accepted in #197 and releases C3-2
implementation from current main; another baseline run is not requested. Final-source
GPU compiler artifacts must accompany later hardware acceptance; #196's retained
historical PTX excerpts cannot establish that result.

Preserve #196's authorized wide-profile rule: for positive dt retain accepted
published displacement/dt; for zero dt retain incoming velocity. Refresh does
not advance simulation time or integrate a body, so surviving position, velocity,
accepted displacement, cached speed and last evaluated force remain exact across
the edit. No new quantizer, damping law, activity epsilon or settling requirement
is proposed. Uninstalled/legacy paths retain their existing behavior.

## 2. One combined operation and one prospective ledger

Add an owner-exclusive Harness operation, provisionally named
`refresh_live_hierarchy(expected_generation, commands, replacement, work_limit)`.
It requires a healthy installed hierarchy and takes the existing bounded
`UpdateCommand` batch plus a complete replacement hierarchy. Return one result
with status, precise failing budget, committed generation, resolved command
handles and charged validation/repair/transfer work through reserved buffers.
An empty structural batch can still carry a meaningful hierarchy-only change.

Refactor `prepared_update.cpp` into reusable preflight/staging and commit phases.
Give hierarchy validation a read-only source view over either the committed
ledger or that exact prospective ledger. Extract the weighted identity/coverage
checks from `CentroidCascade::bind_impl`; calling the public candidate binder
and then installing or reseeding a tree is not a refresh. Candidate queries
retain their public contract and cannot overwrite live or refresh state.

The replacement input needs explicit prospective references:

- Field and occurrence references accept a complete existing `RecordHandle` or
  an `UpdateRef::pending` definition-command index of the correct family.
- A contribution names either a surviving committed source row with its expected
  generation/ordinal, or a participation insert/replace command in this batch.
  Preflight maps it into the final ordered ledger. Removed, replaced-as-old,
  foreign, stale or ambiguous references reject. There is no caller-supplied
  numerical count/weight and no guessed future native handle.
- Carry old-row/command-to-final-row mappings through compaction. Aliases of one
  contribution coalesce; distinct contribution rows/ordinals and independent
  physical occurrences remain distinct under the existing weighted contract.
  Successful source references receive the one committed generation.

Validate the complete final tree, every owner ledger, full identities, child
order, allowed precision/profile, exact integer coverage and checked sums.
Every present field still has exactly one expressed owner and every present body
has leaf participation. Preserve the existing rooted-tree/common-frame subset,
compatible overlap and same-level expressed comparisons. Anonymous groups,
partial owner ledgers, unreviewed scale conversion and an empty replacement tree
remain unsupported. Existing focus selection stays fixed and valid; replacement
focus/rebase remains rejected before mutation.

Keep ordinary nonempty `update_prepared` guarded while a hierarchy is installed.
Only the combined operation may cross this ownership boundary. Preserve guards
on raw upload, seed and reset; do not uninstall/reinstall around source edits.

## 3. Identity matching, repair and pending work

Match comparisons by **full field handle, comparison identity and conceptual
level**. An equal row number, address or comparison number alone is insufficient.
Keep logical input order separate from stable live packet locations, remapping
parent/child, owner and response references through reserved indexes. Reordering
the replacement specification must not reset or exchange unrelated publications.
Preserve the declared summation order of surviving local contributions and
response producers; serialization-only node permutations cannot change it.

Compare ordered normalized contributions, exact numerator/denominator provenance,
precision and immediate child identities/order to identify numerical changes.
Track response-ownership/coverage changes separately: they can invalidate a
body's inputs even when every XYZ/mass packet stays equal. Compute the affected
ancestor closure during bounded structural validation, not as a second runtime
activity scheduler.

| Comparison class | Required treatment |
|---|---|
| Surviving and outside the repair closure | Preserve both slots, selector, old XYZ/mass history, mirror and native pending bit exactly, including a dirty comparison whose publication lags its bodies. Only validated source-row references may be remapped. |
| Changed surviving leaf | Reduce the prospective ordered contributions from current accepted native bodies, including sleepers; only new fixture rows use the private test start supplier. Use exact source counts and Q once at this comparison. |
| Changed/affected ancestor | Repair once in dependency order from complete repaired child packets or the unchanged child's current selected packet. Do not recursively recompute an unchanged dirty child from bodies. |
| New comparison | Initialize complete equal slots and defined initial history with no fabricated prior publication. |
| Retired/recycled comparison | Remove old influence/ownership and clear old selector/history before later reuse; validate the new incarnation independently. |

Stage repairs on the current backend with the same anchored f64 arithmetic,
untrimmed mass and normal-or-zero domain checks as the installed path. This
bounded edit-time dependency pass is separate from ordinary native polling.
Do not run the flat `launch_update_candidate`/commit repair over hierarchy-owned
fields. Reuse its body/contact validation where appropriate; selected focus and
derived field checks must use the prospective hierarchy publications.

When a surviving comparison's repaired packet differs, capture its pre-refresh
selected XYZ/mass as old history and stage two complete equal repaired slots;
its selector can retain its existing valid value at this exclusive boundary.
If the packet stays equal, preserve its existing slots/selector/history and
apply only the required metadata changes/invalidation. Finish old readers and
deliver any history-dependent invalidation before reusing history storage.
Preserve outstanding native particle/centroid work and union required
new-generation invalidation; repair cannot silently consume a pending bit.
Metadata-only count, contact, parent/response or coverage changes independently
invalidate all consequential surviving recipients, even with unchanged centres.
Use existing native activation and full next-phase input evaluation; no added
awake set, queue, scheduling epoch or persistent force cache.

Surviving physical mass/radius/type remain immutable under the existing update
contract. Test changed physical/participating mass through explicit occurrence
replacement, changed participation or coverage; do not add a survivor-mass setter.
Changing one occurrence must preserve another use of the same letter definition.

## 4. Exclusive commit, no-op and failure

The operation has these ordered phases, with no concurrent tick, inspection,
capture or callback across the boundary:

1. Validate generation, all commands/prospective references, topology, ownership,
   limits and affected closure in reserved structural scratch. Check old-plus-new
   claims before releasing anything. Drain current native readers.
2. Read current native state and calculate complete prospective repairs/status
   in separate prepared buffers. Ordinary invalid numerics or exhausted work
   rejects before native claims or committed writes. No host replay of survivors.
3. Claim previously available native records using `PreparedRecordMap` and its
   NodeManager; resolve pending references through the reserved result/remap
   buffers. A same-batch retirement cannot fund a claim.
4. Initialize new rows and publish staged source topology/counts, hierarchy
   ownership/links/response adjacency, repaired slots/history/mirrors and required
   activity effects. Retain unchanged live packets and surviving physical state.
   Recycle retired records only when old readers/references can no longer use them.
5. Synchronize and advance source and installed-hierarchy generation together
   exactly once. Invalidate stale source/query views, preserve completed-tick
   count, and expose only the complete new generation. The next ordinary tick
   checks generation before body motion as it does now.

An empty or wholly semantic no-op preserves generation, publications/history,
activity and completed-tick count after validating its references. A hierarchy
change with zero structural commands must not accidentally return `NoChange`.
Serialization-only node reordering is distinct from a changed arithmetic order
or changed source ledger requiring new provenance.

Ordinary rejection preserves healthy readiness, Demand, source and hierarchy
generation, native handles, both packets/selectors/history, contacts, frame/focus,
survivor state, activity and the last completed report. A subsequent valid edit
and tick must succeed. Unexpected failure after claims or live writes makes the
execution unready; no rollback or healthy partial commit is claimed. Further
ticks, observation and updates refuse use, while orderly teardown remains safe.
The last completed exported report remains historical evidence.

## 5. Prepared storage, bounded work and ownership

Extend `PreparedStorage`/`LiveHierarchy::append_layout`, scratch arithmetic and
prepared bindings before enabling refresh. Reserve committed and prospective
source/tree/response descriptors simultaneously, identity/remap/affected indexes,
candidate XYZ/mass and history, status and transfer/result buffers. Preserve
the native map allocator; add no parallel free list. Match full identities with
bounded reserved tables or charged scans, not allocating maps during a refresh.

Use existing Batch, References, CascadeNodes/Links/References/Coverage,
LiveResponses and CascadeWork budgets where their definitions apply. Charge
validation, identity matching, coverage/influence construction, repair and
invalidation work with checked bounds before mutation. Name any additional
independent reservation explicitly rather than hiding work behind an unlimited
loop. Count current/final demand and old-plus-pending claims separately.

The manifest must include actual host/native copies and ABI widths. A live
two-slot XYZ/mass packet plus selector is 68 bytes per comparison per copy;
old XYZ/mass history is another 32 bytes. Candidate repair and old/new topology
coexist with them. Charge mirrors, existing candidate/geometry storage and
binding contexts if retained; scratch sharing requires proven disjoint lifetimes.
Storage scales with prepared working capacity, not potential-address payloads.
Keep zero-LiveResponses/legacy reservations unchanged.

Measure construction/preparation, first refresh, ordinary ticks and reuse
separately, including first use of previously unused native blocks. Warm compiled
kernels/bindings and native high-water capacity during preparation. Require zero
observed recurring linked C++ new/new[] and stable known native backing, layout,
compiler and binding counters through 32 within-capacity refresh/recovery rounds.
No layout rebuild, JIT compilation or implicit expansion belongs in the measured
ordinary refresh. Report transfer/synchronization, source/response validation,
numerical repair and bit/adjacency work separately. These counters do not prove
malloc/driver residency, pinning or GPU performance.

## 6. Acceptance matrix and evidence

Implement focused C3-2 cases beside the existing prepared-update and hierarchy
tests, using the private fixture adapter only for genuinely new test rows. Run
all cases on BulkCopy and CpuDirect with one/two CPU workers. Preserve the
compiler-shape semantic audits and negative controls; exercise actual refresh
specializations as well as ordinary force/poll kernels. Retain default/scalar
compiler evidence and representative end-to-end no-unroll refresh coverage.

| Case | Required witness |
|---|---|
| No-op and hierarchy-only | Empty/identical requests retain state/generation; a real replacement without source commands commits once. |
| Source metadata and compaction | Exact count changes above f32 distinguishability, u64 accounting boundaries, shifted rows, distinct ordinals/aliases and compatible overlap; no f32-derived weights. |
| Unchanged dirty subtree | Deliberately retain lag, both slots/selector/history and pending native work across an unrelated edit/reorder; subsequent polls consume the outstanding input. A global reseed must fail this test. |
| Repair closure | Two/three levels, changed leaves/child order/precision, new comparison identities and changed root coverage. Independently compute expected repaired packets from frozen native bodies and unchanged published children. |
| Recipient invalidation | XYZ-equal count/mass changes, changed response ownership, cancellation followed by consequential final inputs, both contact endpoints and pending notifications before/after consume. No lost input. |
| Occurrences and reuse | Insert/retire/reinsert holes, native word/block boundaries and last valid records over 32 rounds. Two uses of one retained letter survive editing a third; retired handles and source references reject after reuse. |
| Ordinary rejection | Stale/foreign/recycled handles, removed/replaced source rows, bad pending-command families, cycles, incomplete owner/root coverage, unsupported focus/rebase/profile and invalid numerics leave the entire committed snapshot healthy. Valid retry succeeds. |
| Limits and late failure | Independently exhaust batch, old-plus-new native claims, source/coverage/response/scratch and work limits. Inject failure before claims, after claims, numerical writes, hierarchy/topology writes and before final generation publication. Check preservation or unready disposition at the correct boundary. |
| Continued execution/resources | Tick before/after each accepted edit, check one combined integration against an independent reference using accepted-displacement motion, preserve survivor/history state exactly at the edit, and retain stable prepared resources. Observe settled/evolving separately. |

Inspection/capture tests must see either completed generation, never a mixed
source/tree view. Stale candidate/geometry query references reject after a real
commit. Preserve read-only query behavior and no-op/rejection provenance.

The implementation PR retains source/build/backend pins, complete CPU logs,
per-case results, failure controls and before/after resource measurements.
Run the complete CPU regression and existing PostgreSQL-to-active integration
against retained read-only cold fixtures, then use the normal hosted workflow
for that implementation source. Do not rerun the full native suite for this
docs-only proposal. Historical CPU/GPU failures and earlier evidence stay intact.

## 7. Implementation order and review exit

Use focused commits within one C3-2 implementation PR:

1. Factor prospective source validation and add typed refresh references plus
   fully accounted prepared storage/bindings, preserving existing public behavior.
2. Add identity matching, bounded repair staging and the exclusive combined
   commit with history/invalidation and failure handling. Enable the new entry
   point only when it is coherent end to end.
3. Add the acceptance cases and controls, run CPU/integration validation and
   retain source-pinned evidence; update Built/Active/Held guidance from results.

This plan is accepted for implementation. Its implementation exit is an accepted
combined refresh with the matrix above passing. It does not claim operational
Add, nearest-free placement, candidate contact-safe ticks, an enduring source
owner, focus/rebase, controlled restart, a paced runner or a Docker release.
Those continue in slices B–E of the governing continuation plan.

CPU acceptance supports subsequent local execution of the same refresh cases
on GTX 1070 with no fallback, final-source LLVM/PTX and ordering controls;
CUDA-enabled compilation or prior fixed-tree success is insufficient. Preserve
the explicit qualification guard. Mixed-precision conversion repair remains
independent and must retain its failing oracle until corrected. This work
closes none of #172/#177/#156/#145 and adds no new data population or analyst
behavior.

