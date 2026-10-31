# Native selection workspace — B1 qualification

Status: **accepted in #200, 2026-10-31**, under the accepted
[#199 plan](../../docs/native-admission-contact-transaction-plan.md). This is
B1's first native selection gate. It does **not** enable contact-safe ticks,
operational admission or a new Harness entry point. Typed contact/work rejection,
complete candidate numerical/control staging and one tick publication remain
the following B1 integration work.

[Source-pinned CPU evidence](../tests/native_selection/v1/README.md) retains the
complete regression, eight qualification modes, actual compiler modules,
unsafe/safe controls, native resource observations and PostgreSQL integration.
Hosted validation is recorded separately from these local observations.

Hosted run 41162316957/job 124422351715 at `ada217a` passed all eight checks: 105/105 CPU suites in 3576.79 seconds, compiler collection/audit, extra no-unroll hierarchy/refresh/selection modes, 180 PostgreSQL integration checks with zero private reads, and final CPU-only backend verification.

`detail::NativeSelectionWorkspace` prepares two transient native bitmasked type
sets in the same Program. Input sets belong to that Runtime, have the declared
logical bounds and provisioned backing/list high-water, and remain the committed
authority. Constructor validation rejects negative bounds and incompatible padded
native layouts before creating a tree; exact logical bounds and Runtime ownership
remain caller preconditions. Actual execution currently requires x64.

- `stage()` clears scratch leaves and copies current membership by native
  `struct_for` and activation. It discards the preceding attempt's selection.
- `abandon()` clears scratch membership. Committed bits remain unchanged.
- `publish()` replaces committed membership from the completed candidate sets.
  The owner must have validated/completed its entire candidate transaction and
  drained old readers first. Failure during publication cannot be called healthy
  rollback. This primitive alone cannot establish numerical tick atomicity.

The implementation uses the existing pointer/bitmasked operations and native
list generation. Pointer ancestors are activated explicitly. Preparation warms
all scratch blocks and traversal high-water; publication bindings warm with
committed writes disabled. Parent backing remains provisioned when leaves clear.
No opaque runtime/NodeManager memory is cloned, and no queue, scheduling epoch,
loaded-array awake scan or second physical population is introduced. Scratch
membership has no authority outside its owner's attempt.

## Shared calculation and executable qualification

The existing installed hierarchy now builds its force/control/poll kernels through
one builder receiving native type selections. Ordinary execution uses its original
sets. Private candidate tests invoke the same builder with scratch selections and
fixed candidate bindings, preserving its equations, packet/history protocol and
same-poll progress. Live argument binding remains shared for BulkCopy/CpuDirect.

The qualification runs six case groups with one/two CPU workers, both host paths
and default/scalar LLVM shapes:

| Case | Required witness |
|---|---|
| Isolation/publication | Stage, native particle calculation cut, later contact wake, full same-poll centroid operation and at-most-one visit/publication. Abandon/retry leaves the committed snapshot exact; explicit publication delivers complete candidate values and membership. |
| Unsafe runtime controls | Reject peer calculation inside the particle phase, consume after notification, propagation-only centroid consumption, a frozen centroid work cut and a committed-target activation. The deterministic frozen-cut witness uses one worker; two-worker cases retain all ordering/delivery checks. |
| Dirty packets/force | Nonzero selector, unequal slots, distinct history, dirty published lag and pending input survive staging/rejection. Actual candidate C3 forces match the independent combined-force reference at body8/field6 precision. |
| Reuse | 32 rejection/commit rounds with retained letter/UTF-8 intrinsic fixtures and permuted native order; held-input ordinary polls discharge remaining notifications. Every poll checks at-most-one visitation/publication. |
| Native capacities | Empty, 70/70, 4096/70 and 70/4096 particle/centroid capacities; 32 rounds each, source-word/block boundaries, final usable rows, exact membership, inactive padding and three pre-allocation setup rejections per profile. |
| Compiler controls | Actual mask root/ambient ownership and SC writes; existing complete C3 packet/selector semantic audit and its negative/safe controls on the candidate kernels. |

The small private particle/contact driver increments test positions to expose
selection cuts. It is labelled test code and is not an alternative integration
law or a production candidate tick. Candidate bodies, mirrors, packets, selectors,
previous packets and diagnostic outputs are copied through native test bindings;
committed bodies/history/control/selection/source/report hashes are checked
through rejection. There is no host snapshot restore path.

## Compiler and resource scope

The mask audit follows root-memory and ambient-node provenance in the pinned
LLVMRuntime ABI (`roots` field 7, `ambient_elements` field 15), checks target
ownership, rejects plain root-word stores, and requires SC native mask RMWs.
LLVM-verified controls redirect a target tree or ambient node, weaken an RMW or
replace it with a plain mask update. A redundant atomic OR-zero remains valid.
These are bounded native-mask checks, not a universal pointer-alias or compiler
equivalence proof; native list/allocator bookkeeping is distinct from model bits.
The concurrent numerical packet checks remain the existing semantic C3 audit.

`--case selection_ir` retains 21 actual modules: nine workspace/C3/test-driver
modules plus three mask modules for each of four capacity profiles, with target,
native tree/SNode and argument/shape ABI metadata. `HCP_B1_IR_DIR` supplies its
output directory. Functional qualification is `--case selection`; use
`--threads 1|2`, `--direct` and `--compiler-no-unroll` for the matrix.

The primitive owns selection metadata and three fixed scalar-only contexts;
observed prepared binding payload is 132 bytes. Native root/list/allocator
storage is measured through actual runtime counters, not estimated as one bit
per potential address. Its working bounds are P/G, not virtual V. The candidate
numerical arrays and inspection buffers in this qualification are test setup;
they are not an enabled production reservation or a complete tick manifest.
Harness does not construct this workspace, so its existing payload reservations,
kernel signatures and public operational behavior stay unchanged.

Measured prepared reuse requires zero observed linked C++ new/new[] and stable
known native backing, pool, layout, compiler and binding counters. This excludes
arbitrary malloc, residency/page faults and GPU driver/pinned-transfer claims.
The next B1 integration must reserve every simultaneous candidate/live numerical,
control, contact and native selection surface before enabling a tick transaction.
Actual CUDA execution/ordering qualification remains separate under #145.
