# Fixed installed trial hierarchy (C3-1)

Status: **implementation and coherent-dynamics acceptance for review** under
[#194](../../docs/production-hierarchy-response-plan.md) and the superseding
[maintainer direction](../../docs/coherent-dynamics-gpu-snapshots.md).
Moving studies use finite observation windows; continuing motion is recorded
separately from invariant failures. Held-input convergence, arithmetic, packet/
history, pending inputs, numerical domains and resource assertions remain hard
requirements. The original failing results are retained unchanged alongside the
[new source-pinned observations](../tests/live_hierarchy/README.md).

This is an opt-in path in `field_core`, not operational owner/admission completion.
Uninstalled executions retain the existing flat forces and centroid phases.
C3-2 structural refresh, #156's remaining operations and #145 GPU qualification
are not released by this work. No physics coefficient, overshoot brake, radial
damping, contact law or production precision policy is changed.

## Boundary and identity

Reserve `Budget::LiveResponses` together with weighted Cascade capacities and a
f64 trial profile before constructing `Harness`. Zero leaves the original storage
and signatures unchanged. `PreparedStorage::required_scratch` includes the new
arrays and prepared contexts. Installation currently supports actual LLVM/x64
execution; other actual backends return `Unsupported`.

`install_live_hierarchy(expected_generation, WeightedCascadeSpec, work_limit)`
validates through the existing weighted candidate binder. The installed subset
requires one expressed context per distinct present native field, complete owner
ledgers, one rooted calculation tree, and leaf participation for every present
body. Full handles, source generation/row/ordinal, aliases and checked integer
coverage retain their existing meaning. Distinct ordinals and physical copies
remain distinct. Parent coverage equals the sum of child coverage per occurrence.
Counts are not inferred from f32 shares.

Every expressed node has its own comparison identity and Q setting. Same-level
expressed comparisons are permitted only through the private live validation
variant; the public candidate-only level contract is unchanged. Anonymous
grouping, aliased outputs, partial owner ledgers, incompatible coverage and stale
references reject. Levels and storage depth select no distance multiplier.
All coordinates already belong to the execution's common operative frame.

Installation is one-time in this slice. Both slots are seeded from current native
bodies through the explicit candidate operation, then mirrors/history/ownership
publish under the exclusive synchronized boundary. Existing physical motion is
preserved and changed force ownership invalidates bodies. Failed validation leaves
flat execution healthy. Failed replacement attempts preserve an installed tree.

While installed, every nonempty `update_prepared` returns `Unsupported` before
structural mutation. Raw prepared publication, upload, seed and focus replacement
remain rejected. Source generation is checked before body motion. There is no
repair-after-motion workaround, uninstall or live representation exchange here.
Native failure after mutation makes execution unready; no rollback is claimed.

## Force and phase ownership

For a child n and parent p, the proposed force accepted for implementation in
#194 is the existing inclusive-centroid attraction applied to the represented
child whole. Distribution to occurrence i uses its current physical mass and
validated child coverage:

```text
w(n,i) = m_i * f64(C(n,i)) / f64(D_i)
F(n <- p) = G * M_n * M_p * (c_p-c_n) / |c_p-c_n|^3
F(i <- n,p) = w(n,i) / M_n * F(n <- p)
```

The native expression factors out M_n and calculates
`G * w(n,i) * M_p * delta / |delta|^3`. No second participation share is applied.
Only leaves retain body-to-field direct forces. Internal direct body forces are
replaced by immediate child-to-parent influences, not added to them. Each body
gathers every covering ancestor edge once, plus its ordered local rows and
canonical contacts, into the existing force accumulator, then integrates once.
Universal mass still counts physical occurrences once.

Exact coincidence, zero G, a parent with only one physical occurrence, and no
complementary parent coverage give zero response before inverse-distance work.
In a validated positive-coverage tree, a unary parent has no complement; multiple
nonempty children provide one. Local singleton exclusion is shared with C1.
Compatible overlaps can distribute several distinct influences to one body;
equal vectors are not duplicate identity.

Ordinary phases are:

1. Focal controls read the completed mirror. Active bodies gather current complete
   publications, add contacts, integrate, and publish Q once after the unchanged
   brake/damping. Final speed is written before quiet-body consumption.
2. Accepted movement activates its leaf fields. Existing two-sided contact
   propagation follows body calculation.
3. One traversal of the existing centroid NativeSet consumes each entered bit
   before acquiring inputs. Leaves include sleeping bodies; parents use published
   children. Each expressed destination combines before its Q and retains
   untrimmed participating mass.
4. A changed XYZ or mass writes a complete inactive packet, exchanges its selector,
   notifies its parent, and checks receiving bodies. Their phase has finished, so
   activations remain for a following body phase.
5. All readers finish before another ordered poll can reuse a slot.

The explicit initial candidate solve is not a per-tick scheduler. There are no
per-depth ordinary dispatches, additional awake sets, queues or scheduling epochs.
Diagnostic visit/publication counts never choose work.

## Publication, history and recipient checks

Each node owns two f64 XYZ/mass slots and an i32 selector. Own, child and response
readers acquire through native SC OR-zero; publication uses native SC XOR-one.
One writer visits/publishes at most once per poll. Old and new packets remain
immutable for all readers until the ordered completion cut. Parent inputs can be
different generations of complete sibling publications; a later change must
leave native calculation work pending.

Old XYZ/mass history is separate from reduction scratch and discarded positional
detail. Existing flat field XYZ/mass is a completed projection, read by focus,
geometry and synchronized inspection. Old accumulator rows are unused zeroed
scratch for installed nodes. Existing group flags describe the last entered
evaluation/publication; pending work is the NativeSet bit, not those flags.

A publishing node enumerates covered recipients and calculates paired complete
old/new combined attractions. Pairwise cancellation alone cannot establish that
all cumulative inputs equal a body's last evaluated input. The implementation
also gathers the current full attraction plus canonical current contacts and
compares it with the body's retained `kForceXYZ` from its last body evaluation.
That existing accumulator is immutable throughout the centroid phase; no new
force cache is allocated. Mass changes independently invalidate recipients.

For a recipient whose final inputs changed, consider the last relevant publisher
in the SC order. Its subsequent selector acquisitions can see all preceding
relevant publications, and no later relevant publication is omitted without
performing another check. The fresh combined force is compared against the
unchanged last-evaluation baseline. This preserves cumulative input even when a
paired deletion from mixed inputs gives zero. Existing contact/focal invalidation
remains independent. Force differences are not filtered by an activity epsilon.

The tests retain an unsafe-consumption control with a unary top root: its own
paired response difference is zero, but the baseline differs and recipient work
must be restored. Normal before/after-parent-consume controls separately verify
that genuine later notifications survive without that unsafe intervention.

The actual Program-retained LLVM modules are audited for each installed fixture.
Every selector memory access is attributed to an argument/index role; ordinary,
unknown or demoted accesses reject. Response child/parent columns are traced
through the argument-buffer shape stride. XYZ/mass component coverage must derive
from one dominating SC acquire at the same node/slot. CFG dominance and
post-dominance tie each emitted force, child-reduction, owner and wake iteration
to its required readers through the existing native work-counter operations;
reader reuse across iterations rejects. Every production node descriptor also
requires its matching reader. Inactive packet writes must precede the owning
publication exchange and cover all four components. These checks do not depend
on a fixed number of compiler clones, epilogues or duplicated packet loads.
LLVM-verified module controls remove individual child/parent paths, break
node/slot provenance, demote ordering, insert ordinary/unknown accesses and
damage publication ownership/completeness. Positive controls retain complete
callback clones and redundant safe loads. The test runner's
`--compiler-no-unroll` uses LLVM's test knobs to exercise scalar loops with the
same production kernels and numerical criteria; it is not a runtime policy.
`--case ir` retains optimized modules/ABI metadata; `--audit-ir DIR` parses and
checks those modules without executing foreign IR. End-to-end numerical tests
remain necessary: the structural audit does not prove arbitrary compiler
equivalence or detect deletion of an entire loop together with all its anchors.
This is qualified CPU IR, not executable disassembly or CUDA ordering evidence.

## Other readers and resources

`inspect_live_hierarchy` exposes complete slot/history/selector observations,
per-comparison precision/identity, and numerical work diagnostics. Returned arrays
are borrowed until the next synchronized operation; use the installation's
logical node count. The immutable execution profile retains its default field
precision; per-node overrides are reported by this new inspection, not by
relabeling the global default.

Prepared physical inspection/download, focal calculation and read-only geometry
use completed typed mirrors. Candidate queries own separate values and cannot
repair live publications. Query-isolation tests preserve physical, source,
history, activity and Demand state. Existing operational ActiveSession/capture
construction does not install this opt-in hierarchy; owner/report integration
remains under #156. No browser interface changes are included.

The manifest charges every host/native array, source identity, CSR, response
reference and prepared binding. Publication alone costs 68H native bytes; retained
old history adds 32H. Inspection copies are also charged. Response descriptors
contain four i32 controls and two u64 ratio entries per reserved reference, with
separate body offsets. Existing body, group, contact, activity and flag arrays are
borrowed through frozen contexts, not counted as new owned arrays.

There are sixteen new owned native arrays. Fixed object/vector headers, ndarray
wrappers, compiler/runtime objects and pool reservation are reported separately
from typed payload. Inline counter bytes are included in fixed object sizes and
also identified in the payload manifest, so those figures must not simply be
added. Ordinary host vectors are not pinned memory.

At the retained fixture capacities (70 bodies/fields/nodes, 140 child links,
512 source/coverage entries and 1,024 responses), those arrays occupy 60,344
native bytes. Host array/identity/inspection payload is 79,072 bytes. The complete
source scratch manifest is 504,028 bytes, including 16,624 bytes of actual prepared
binding payload and 2,960 bytes of unused binding reservation. These are bounded
capacity observations, not bytes per live particle or total process memory.

Setup, first installation/use, ordinary kernels, transfers, inspection/reference
work and 32-round reuse are measured separately. Linked C++ new/new[] observations
and known native requested/list/pool/cursor/tree/compiler counters have their
existing limits; they do not measure arbitrary malloc, page faults or driver
residency. C3-2 simultaneous committed/prospective refresh storage is not reserved
or implemented by this fixed-tree slice.

The current moving contract observes up to 4,096 ticks per round, ending early
only after 32 quiet observations, and completes all 32 perturbation/reuse rounds.
No frozen solve runs between those rounds. A labelled terminal frozen diagnostic
then checks held-input convergence; its modified state is never resumed as the
moving study. Quiet cases also check the resolved hierarchy reference. Evolving
cases check the actual complete packets available to that live tick, not an
instantaneous global solve. Q8/Q6 remains a coarse stress setting. Signed/cell-
boundary probes report displacement and absolute publication error in local
ULPs, without using an almost-zero attempted-step ratio as an error criterion.
No physical law, damping, precision primitive or scheduling policy is changed.
#172/#177 and #156/#145 remain open.

## Observe the active execution

The [shared capture adapter](../../tools/viewport/native/snapshot.h) takes a
reference to the already running Harness and a validated immutable source/slot
binding. It synchronizes and copies that execution's completed state, preserving
sleepers, radii/masses, frame, native generation/tick, actual backend and per-node
precision. The exporter constructs no Program, advances no simulation and performs
no candidate numerical solve. The legacy owner-checked ActiveSession path uses
the same completed-state value/serializer and its own completed-tick counter.
Capture cost and requested readback payload are reported separately from ticks.

A [bounded native runner and local handoff](../../docs/c3-cpu-capture-handoff.md)
exercise the same C3 letter/byte execution and existing atomic snapshot writer /
offline viewer. This is not #156 admission/owner completion. Actual CUDA C3
installation remains explicitly unsupported until its native publication/activity/
f64 contract is qualified locally; the backend-independent reader does not
constitute that qualification.
