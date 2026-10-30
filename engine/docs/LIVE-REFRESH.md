# Coherent source/hierarchy refresh (C3-2)

Status: **accepted in #198**, merge `5ea9c76`, reviewed head `f4c5ba9`, under the accepted
[#197 plan](../../docs/live-hierarchy-refresh-plan.md). This extends the
[installed hierarchy](LIVE-HIERARCHY.md) with one combined owner-exclusive edit.
The [source-pinned CPU record](../tests/live_hierarchy/refresh-v1/README.md) retains
local 101/101 regression and eight default/scalar refresh modes. Final hosted
run 41123391538 at workflow source `904f2bc` passed 101/101 in 3355.27 seconds,
all four extra no-unroll hierarchy and four refresh modes, compiler collection/
audit, 180 integration checks and CPU-only backend verification. All nine checks
passed. The earlier initialization failure and job-budget cancellation remain
separate retained evidence. Native operational admission/contact-safe ticks follow.
Actual refresh execution on CUDA remains unqualified.

## Entry point and references

`Harness::refresh_live_hierarchy(expected_generation, commands, count,
LiveRefreshSpec, work_limit)` validates the prospective source ledger and complete
replacement hierarchy together. It returns `UpdateResult` with one committed
generation, status/failing budget, charged work and transfer counts. Resolved
definition-command handles are borrowed from `prepared_storage().resolved` until
the next synchronized operation. Requests and their pointed-to arrays must be
caller-owned and stable for the duration of the call.

The execution must be ready, prepared and have an installed f64 trial hierarchy.
Ordinary nonempty `update_prepared` remains rejected while installed. Raw upload,
reseed/reset, replacement focus and rebase remain guarded. There is no uninstall/
reinstall workaround or second physical execution.

- `LiveRefreshNode` retains `CascadeNode`'s identity, precision and spans. Existing
  owners use its full `field` handle. A pending owner leaves `field` empty and
  names a `DefineField` through `field_command`.
- `LiveRefreshContribution::occurrence` is an existing full handle or a typed
  pending definition reference. An untouched contribution supplies its committed
  generation/row/ordinal. A new/replaced participation instead names that command
  through `source_command`. Removed/replaced-as-old references reject.
- Preflight maps these references to one final ordered source ledger. Exact
  counts and denominators come from that ledger. Aliases coalesce by source row;
  ordinals alone are not identity. Weights never come from rounded f32 shares.

The existing complete expressed-tree subset is retained: one owner per present
field, leaf participation for every body, checked coverage sums, compatible
fractional overlap and one common operative frame. Children precede parents;
their declared order determines arithmetic. Same-level expressed contexts remain
supported. Anonymous groups, partial owner ledgers, an empty replacement tree
and unsupported precision/frame conversion reject before mutation.

## Stable packets and native repair

Full field handle, comparison identity and level match surviving comparisons.
Logical serialization order is separate from stable packet slots. The new
inspection `order[0..node_count)` maps logical topological order to physical
packet locations, including holes. Source/response descriptors are remapped;
surviving producer order is preserved across serialization-only permutations.

Preflight compares ordered contributions, exact provenance, masks and children,
then computes the affected closure. Native edit-time kernels read current accepted
bodies, including sleepers, and repair affected leaves/ancestors once. An unchanged
dirty child contributes its current complete selected publication, retaining lag
and its pending bit. No global reseed or ordinary per-depth scheduler is added.

The candidate kernel writes only separate prepared packet/history/status/derived
scratch. The common updater's flat repair is disabled for hierarchy-owned fields;
its focus and universal-mass checks use the staged publications/current bodies.
Only genuinely new numerical fixtures read the private test start supplier.

Unchanged packets retain both slots, selector, previous XYZ/mass, mirror and
pending work. A changed survivor stages two complete equal new slots, preserves
its selector and retains its pre-refresh selected packet as history. Equal output
preserves existing slots/history even when provenance changed. New comparisons
start complete without claiming a previous publication. Retired slots/history
are cleared before reuse.

All old readers/history-dependent producer checks finish at the synchronized
exclusive boundary. Existing particle/centroid bits are retained and required
source/contact/coverage/response invalidation is unioned through native activation.
Future body work evaluates all current inputs. Numerically equal count edits still
invalidate their inputs. No force cache, awake registry or scheduling epoch is
introduced.

## Commit and failure

After structural/work and native numerical preflight, native maps claim only
previously available records. Same-batch retirement cannot fund claims. Reserved
remaps convert scratch names to actual handles. Source relationships/counts,
hierarchy descriptors/ownership, complete repaired packets/history/mirrors and
activity effects publish under the same owner boundary. Source CSR and installed
hierarchy share one generation increment after final synchronization. Completed
tick count does not advance. Surviving positions, velocities, accepted displacement,
cached speed, physical mass/radius/type and last force are not overwritten.

No-op requests retain generation and state after reference validation; a real
hierarchy-only replacement can commit without structural commands. Ordinary
reference/domain/work/capacity rejection keeps the old execution ready, including
last report and pending work. Unexpected failure after claims/native writes makes
the execution unready. Tick, inspection and further edits then refuse use; teardown
remains valid. No rollback is claimed.

Successful commits invalidate old candidate/geometry/capture bindings. Rejection
preserves them. The completed-state capture adapter follows logical packet-slot
mapping and observes the same execution; it cannot turn an old source binding into
a new operational owner.

## Prepared resources and verification

LiveResponses enables the refresh reservation; zero preserves the legacy layout
and signatures. The manifest includes source-origin/remap tables, simultaneous
old/prospective descriptors, two native candidate packet slots, selector, history,
coverage/response identities and both prepared kernel contexts. Packet/selector
storage is 68 bytes per comparison per owned copy, with 32 additional history
bytes. Existing candidate/geometry/mirror surfaces remain charged.

CascadeWork charges a conservative source-preflight reservation before scans/sort,
metadata visits and a native repair/commit bound before dispatch. These are bounded
work charges, not FLOPs or timing. Current/final demand, old-plus-new claims and
combined descriptor reference peaks are checked independently. Ordinary refresh
does not compile, resize or rebuild a layout.

`live_hierarchy_test --case refresh --threads 1|2 [--direct]` runs the C3-2
acceptance cases. `--compiler-no-unroll` repeats them under scalar LLVM loop
shapes. The ordinary hierarchy suite remains separate. The tests cover dirty
history, exact counts, overlap/root coverage, combined integration, contacts,
independent retained-letter copies, 32 reuse rounds, source/field retirement,
ordinary rejection, budget exhaustion, injected failure and stale observation.

Actual refresh LLVM checks protect candidate inputs/committed state, commit
survivor state and complete packet/history/selector delivery. LLVM-verified
negative controls remove packet/selector writes or add candidate body writes;
safe duplicate stores remain valid. These checks complement the numerical and
state-preservation matrix; they are not a universal compiler-equivalence proof.
`--case ir` retains the ordinary three modules plus both refresh modules.

This is the numerical-fixture refresh foundation. Native nearest-free admission,
contact-safe candidate ticks, enduring source/occurrence ownership, operational
letter commands, pacing and Docker remain subsequent slices. The accepted
displacement/dt motion rule is unchanged. The separate mixed-precision subnormal
failure and #145 hardware/transfer qualification remain open.
