# Whole-field LoD in the live CPU study

## Preparation-budget correction — awaiting re-review

[Maintainer review 7144539532](https://github.com/Human-Cognome-Project/human-cognome-project/pull/430#issuecomment-7144539532)
holds `de9057b5` for uncharged comparisons in the live-slot and reserved-address
searches. The review checks the positive/refusal captures and finds the requested
first English A preset covered. #427 and #429 are now integrated at main
`b528d494`; #430 targets main. This correction returns for re-review and its own
authoritative GitHub CI. All merges remain maintainer-only.

Corrected executable source: `b8185fe95b43833ca2c46d9217b0e2306877d839`, tree
`2bc8b70b449c2277c2acaf2a3f4f21e7680bc348`. Preserved local `158d4e41`
has the identical tree. Main's tree equals the reviewed #429 parent tree.
The correction is limited to `prepared_lod_workspace.cpp` and its acceptance
test; native placement/numerical/runtime/fork source, layouts, interfaces,
capacity choices, allocation policy and workflow code are unchanged.

### Charged searches

The local membership helper charges one work unit before each actual primitive
comparison, including unsuccessful probes. It retains the existing first-match
semantics and adds no retained owner storage. It covers all four searches identified in review:

- Setup candidate-address lookup and final reserved-address uniqueness lookup.
- Recurring free-slot lookup and the separate selected-slot activity lookup.

The surrounding candidate, catalogue, declaration and outer-slot charges remain.
Budget exhaustion is caught at the existing preparation boundary, before native
publication. The same reserved metadata is reused on retry. There is no new
execution authority, source identity or native state import.

### Distinguishing budget witnesses

The regression uses the actual public prepared session, switches to fine
exposure and admits entries 20/30 to fill all four metadata/native body slots.
No synthetic live-map mutation is used. It independently counts the two study-key
equality gates, declaration scans and catalogue/outer visits: 158 work units for
this fixture. A fresh entry's full-slot search needs another 1+2+3+4=10 probes.
With 158 units it now exhausts preparation work, preserving the completed state.
With exactly 168 it finishes metadata inspection and returns the ordinary
five-required/four-available particle-capacity refusal without claims.

After removing entry 30, the free metadata slot is last. Its lookup inspects
1+2+3+3=9 live entries; the separate activity lookup inspects another three.
At 170 units the next key-copy charge refuses with charged=170 / next-required=171.
This distinguishes the inner work from the outer visits. A sufficient-work retry
charges 2,411 units and admits entry 40, preserving every survivor's full claim
and physical bits. A following native tick succeeds. Full field claims,
generation/tick, particle/group values and report currency are checked on refusal;
recurring C++ new/new[] and retained metadata growth remain guarded.

The setup regression also retries the previously published 28,541-unit metadata
allowance. It now refuses and releases every owner-domain allocation before
claims. The complete fixture requires 28,625 units: 84 additional actual address
comparisons. Existing one-short/exact setup bytes, allocation work, plan work and
metadata-work checks remain active, as do native backing and late owner-map
failure controls. Requested metadata bytes (32,890), cumulative allocation work
(35,840) and combined plan work (53,774) are unchanged. These are preparation
work units and allocator requests, separate from placement work, native bytes,
complete B and RSS.

The following are raw successful corrected-source witness rows (the same values
are checked in the worker/path profiles):

```text
whole_field_probe_budget,slots=4,outer_work=158,full_probes=10,full_required=168,late_probes=12,late_prefix=170,late_next=171,retry_work=2411,unchanged_refusals=1,sufficient_retry=1,following_tick=1,recurring_new=0 PASS
whole_field_setup,peak_bytes=32890,allocation_work=35840,plan_work=53774,metadata_work=28625,one_short_refusals=4,exact_retries=4,unwind=1,backing=stable,postclaim_unready=1 PASS
```

### Correction qualification and evidence

| Fresh correction scope | Result |
| --- | --- |
| Quick local CI | 3/3 CTests plus placement math/capacity controls |
| Clang 15, warm/viewport ON | 4/4 whole-field profiles: one/two workers × bulk/direct |
| GCC 11, same LLVM 15 CPU SDK | 2/2 whole-field profiles: one-worker bulk/two-worker direct |
| Clang 15, warm/viewport OFF | Prepared-owner library compiles; no minimal CTest run claimed |
| Current-source operator capture | Four exchanges/seven ticks; per-root state/focus/claim-peak audit passes |

These nine fresh CTests include the new full/late-free probes, constructor-budget
regression, existing three-round-trip motion/focus/peak controls, exact/one-short
resources, stale views, native backing stability and postclaim-unready behavior.
The runners finish with unchanged clean source/HEAD at the corrected pin. The
new constructor/live-slot probes do not allocate recurring C++ storage. The
original hosted run was left to finish; the corrected PR head receives its own
automatic hosted qualification when published.


Both regression controls fail on reviewed production code with only the new
test patch: `full catalogue accepted outer-only work without charging live
probes` and `reviewed setup allowance still omits address probes`. Both pass
after the four searches are metered. The red patch, build output and complete
red/green stdout/stderr are retained. A first GCC driver invocation exited before
creating its evidence directory and produced no diagnostic; it is not a passing
test result. The separate traced retry records the successful clean run. Its
cause is not inferred from the empty log.

The original forty-test/IR/capture qualification below remains evidence for its
initial pins. It is not counted again as correction coverage. Unchanged native
and pure source/build boundaries are verified, and the original archive is
freshly checked before reuse. The original three captures retain their original
work counters and source pins. The [current-source positive capture](../../tools/viewport/examples/whole-field-probe-correction-capture.jsonl)
records the corrected operator's work reporting and continued exchange/ticking.
The vendored Taichi reference `a94a9eb350c94dfa2eb586bee49c53e0ee886648`
is a tree; its SHA and runtime bitcode are unchanged.

Correction archive: `/tmp/hcp-430-b8185fe9-evidence.tar.gz`, SHA-256
`2f9b3c0454218ae90137ecbd12564b242a030f166d3ba4b60f0abd09db2550cb`. Fresh extraction verifies the root and every nested manifest.
The bundle retains clean source/toolchain/runtime/binary pins, exact commands,
red/green regression patch/output, setup/probe witnesses, corrected raw capture,
source equivalence and incremental Git bundle. The original archive is referenced
by its separately verified digest and fresh root/nested checks. The full bundles
remain machine-local; the bounded corrected capture and raw witness rows above
are directly reviewable in Git. The final handoff commit changes only Markdown
and that capture; executable equivalence is verified separately at submission.


The correction does not extend the supported owner beyond the initial declared
single-byte profile. The recorded crowded 65,537/65,536 placement-cell refusal
below is unchanged. General multibyte adaptation, focus handover, automatic LoD,
GPU/concurrency and allocation-policy changes remain separate. After re-review,
the assigned capability continuation is the fine confounding/next-level
structural trace alongside targets and motion.

## Initial implementation and evidence at 6a94343b

Contributor implementation for [#428](https://github.com/Human-Cognome-Project/human-cognome-project/issues/428),
following [maintainer direction 7141017357](https://github.com/Human-Cognome-Project/human-cognome-project/issues/428#issuecomment-7141017357).
The first explicit profile uses an inherited English membership focus which is
present at both exposures. The existing ParentValue profile and focal-retirement
refusal remain available. Focus handover is a separate operation.

Qualified implementation pin: `6a94343ba6b0acf8b75ab16482eba6ab56ed7f1b`,
tree `57dad9b9ec2d40d22c30970fc9da063051732770`, on provisionally accepted
[#429](https://github.com/Human-Cognome-Project/human-cognome-project/pull/429)
head `b2c516bf`. Main remains `98cdbe03`; #427 and #429 remain open at this
inspection. These dependencies and authoritative CI are separate from local
qualification. All merges remain maintainer-only.

Both native wrapper compilers use the matching Clang/LLVM 15.0.7 CPU SDK from
fork tree `a94a9eb350c94dfa2eb586bee49c53e0ee886648`. Runtime bitcode remains
`4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada`.

## Supported operation

The opt-in command exchanges every live logical occurrence in the current
prepared owner between English A aggregate and its one declared byte. It reuses
the same session, logical study, native Harness, model frame and completed tick.
The owner publishes the complete exposure/map after one native generation. It
does not loop committed single-occurrence exchanges, reconstruct recipe starts,
or combine distinct occurrences into one motion reduction.

```sh
source engine/build/engine-env.sh
engine/build/viewport/viewport_live_study --threads 2 --direct --whole-field \
  --commands tools/viewport/examples/whole-field-study.commands
```

Interactive commands include `lod parts`, `lod aggregate`, ordinary `add` and
`remove`, `tick`, `observe` and `list`. The original command without
`--whole-field` retains its original ParentValue-focus profile. A default owner
without a prepared alternate refuses LoD as unsupported.

The [positive raw capture](../../tools/viewport/examples/whole-field-study-capture.jsonl)
records the command sequence above. The [peak-refusal commands](../../tools/viewport/examples/whole-field-refusal.commands)
and [raw capture](../../tools/viewport/examples/whole-field-refusal-capture.jsonl)
exercise a third live occurrence, refusal before claims, removal and continued
exchange/ticking. The [identity commands](../../tools/viewport/examples/whole-field-identity.commands)
and [raw capture](../../tools/viewport/examples/whole-field-identity-capture.jsonl)
cover declared-template restoration and the crowded packing refusal described
below. Each JSONL record includes the ordinary completed snapshot, full live
claims and detached native velocities. These captures use fixture source pins.

This first owner subset has one physical body per logical occurrence at either
exposure. Aggregate identity is `study/entry`; the declared byte has the ordered
path `study/entry/0/0`. Initial logical entries are 2 and 10. Both representations
and their native bindings are prepared before readiness: root units 1 and primary
units 64, comparison definitions/topology, exact source masses, geometry and
particle/field addresses. The shared recipe has both role alternatives; an
explicit immutable logical view selects a uniform initial aggregate exposure.
Its normal value-copy semantics stay independent. Explicit exposure views share
the pinned immutable recipe and retain owner/study with a fresh logical view
generation. The setup rejects alternate sources with a different recipe pointee,
study or owner. Current execution exposure comes from the prepared owner report,
separate from the immutable source view.

Genuinely new occurrences still use native nearest-free placement around the
current focus with zero initial velocity. Existing occurrence template and
declared-destination checks apply at either exposure, including after instance
metadata reuse. Removing the last live cohort is unsupported. General multibyte
owner adaptation, arbitrary definition replacement, automatic exposure selection,
focus handover and confounding/structural interpretation remain separate.

## One native transaction

`PlacementExchangeGroup` partitions validated outgoing handles and canonically
ordered incoming ranks. Groups must be nonempty, contiguous, disjoint and cover
both complete sets. Exact incoming/outgoing source mass must agree separately
for every logical group; agreement of only the combined total is insufficient.
The normal updater also revalidates the exact retirement and participation set.

For each group the existing strict native reduction reads current physical
positions/velocities and exact source masses. Its centre and velocity are copied
natively into reserved request rows for that group's replacements. The host
supplies source/group identities and geometry, never replacement coordinates or
velocity. The existing certified search uses each group's own outgoing centre
while planning all prospective occupants, excluding all outgoing alternatives.
Earlier arrivals block later arrivals in canonical logical order. It uses the
same reviewed nearest-free objective and numerical guards.

The existing initializer scatters every incoming body once, using its own group
velocity. The native accounting kernel checks each group's mass and momentum
with the existing bounded tolerance. Source diff, contact/hierarchy preparation
and final checks precede the single native update. All incoming particle/field
claims exist before any outgoing retirement. Only after successful native
publication does the owner swap its prepared live catalogue, full native claims,
current field-source view and exposure. Ordinary refusal leaves completed
physical state, generation, claims and views unchanged. Unexpected postclaim or
owner-map publication failure marks the shared execution unready.

Membership comparison 102 keeps the same complete field claim at both exposures.
Aggregate comparisons 100/101 are replaced by fine comparisons 110/111/112;
the logical meaning of the inherited membership is unchanged. Focus centroid
values may change with the expressed geometry. Retaining focus identity is not
a new promise of exact geometric centre preservation. The existing D7 contract
allows an initialization offset when nearest-free packing requires it.

## Actual capacity and storage boundary

The explicitly selected profile reserves potential 128, particle records 4,
field records 6, participation 32, contacts 6 and batch 64. A whole-field exchange
of the two initial logical occurrences observes the following native map counts:

| Direction | Particles before / peak / after | Fields before / peak / after |
| --- | --- | --- |
| Aggregate → byte | 2 / 4 / 2 | 3 / 6 / 4 |
| Byte → aggregate | 2 / 4 / 2 | 4 / 6 / 3 |

The old-plus-incoming peak is measured by the updater's existing claim receipt,
not inferred from final population. Particle capacity 3 refuses required 4 even
though the final two bodies fit. Adding a third live logical occurrence is valid,
but its whole-field exchange refuses required 6 against 4. Refusal returns an
unsampled claim receipt and permits later ordinary edits/ticks. No outgoing
retirement finances incoming claims, and allocation policy has not been relaxed.

The native request array grows from `3*B` to `9*B` doubles, where
`B=max(Batch,BootstrapCommands,1)`. The six added rows cost `48*B` requested bytes
(3,072 at this profile's B=64). The host upload buffer remains `3*B`; native
target/motion rows are not host-uploaded. The additional extent is included in
`required_scratch`, also for other prepared placement profiles. Existing kernels
gain six total parameters; the binding bound changes from 95 to 101, with no new
kernel/context. Both default and grouped signatures are warmed before readiness.
All 19 placement modules retain strict arithmetic/output/publication auditing,
with a new negative omission control for grouped target/motion writes.

All owner maps, paths, template metadata, retained alternate plans, group/command
staging and coherent hierarchy scratch are charged to the same owner reservation
domain before readiness. Both plan traversals share one total work allowance.
Recurring edits perform prepared assignments/swaps without C++ new/new[].

At this x64 source both Clang 15 and GCC 11 measure private
`PreparedStudyOwner` at 424→448 bytes (the added domain-aware unique handle is
24 bytes), `PreparedLodWorkspace` at 936 bytes and `PlacementExchangeGroup` at
32 bytes. The six added frozen parameters reserve another 672 bytes using the
existing binding formula. The actual profile scratch reservation is 272,427,356
bytes, including the existing 256 MiB allowance above `required_scratch`.

The setup witness measures 32,890 peak/live owner-domain bytes, 35,840 cumulative
allocation-work bytes, 53,774 combined plan-work units and 28,541 metadata-work
units. The captured owner retains the same 32,890 bytes across exchanges and
edits. It retains two immutable plans and fixed scratch rather than replaying
setup or allocating an exchange plan during recurring operation. Caller-side
immutable recipe views, native/runtime allocations and compiler resources are
distinct from this owner-domain measurement.

These figures are allocator requests/bounds or occupied records, with their
separate meanings. They are not RSS, native executable/ORC bytes, full B, complete
owner peak or admission credit. No cache, retained data, backing pool or Git
history was cleared. Matching native/client rebuilds are required for changed
private layouts/signatures.

## Qualification and retained evidence

| Local scope | Result |
| --- | --- |
| Quick CI | 3/3 CTests plus placement math/capacity controls |
| Clang 15, warm/viewport ON | 17/17 focused CTests; ten native control/audit invocations |
| GCC 11, same CPU SDK | 11/11 focused CTests; five native control invocations |
| Clang 15, warm/viewport OFF | 4/4 minimal CTests; four native control/audit invocations |
| Standalone logical build | 5/5 CTests; no native target in its dependency graph |

These forty CTests qualify the stated changed CPU scope. Both wrapper compilers
run all four new public whole-field profiles (one/two workers × bulk/direct).
Clang runs all four new operator profiles; GCC additionally runs one-worker bulk
and two-worker direct. Native grouping and legacy field-exchange controls run
one-worker bulk/two-worker direct in both compilers; the minimal build exercises
the core without warm/viewport integration. Clang audits actual emitted placement
modules in default one-worker bulk and no-unroll two-worker direct; the minimal
build also audits default modules. This is not the complete hosted matrix.

All qualification runners preserve clean source/HEAD at their recorded pins.
Three raw captures are separately audited for same-study execution, per-root
motion/fresh claims, inherited membership identity, observed claim peaks and
unchanged completed observations on refusal/no-op. The positive sequence
completes four whole-field exchanges and seven ticks in one study. The third-live
occurrence refuses required six particle claims against four, then removal,
exchange and ticking continue. Expected refusal captures exit 1.


The two-occurrence witness deliberately has nonzero, distinct native velocities
before exchange. Three round trips verify each complete occurrence's velocity
bitwise, its own target on unblocked expansion, exact source/physical mass,
fresh claims, one unticked native generation and the same membership field claim.
It also adds/removes a fine occurrence, checks survivor bits and report currency,
and completes ticks after changes and ordinary refusals. Stale/foreign identities,
zero/one-short metadata work, zero placement work and final-fit/peak-overflow
refusals remain healthy and unchanged.

Direct grouped native controls add malformed/overlapping/partial partitions,
per-group mass mismatch despite an unchanged total, unchanged third-survivor bits,
native backing/compiler stability and postclaim-unready behavior. Exact and
one-short metadata bytes, allocation work, combined plan work and metadata work
unwind to zero before claims, followed by exact retries. Late owner-map failure
rejects borrowed views and future healthy operations. Existing single-occurrence
exchange, focus-retirement, generated placement, bootstrap and preflight controls
stay active. The independent logical suite checks explicit exposure view pinning,
selection isolation and replaced/cleared focus.

Initial clean runs at `baa254df` failed to compile the new audit because its
fixture reference was misspelled `h` rather than `f.h`. The separate correction
changes only that test line; failed logs are retained. The five-test standalone
suite passed at `baa254df` and is reused by exact unchanged logical source/test/
build proof. Earlier development failures include a mixed-const pointer list,
an invalid display exposure label and allocating test-input construction inside
the native no-new guard. The corrections retain strict display validation and
the full guarded native operation. No production predicate or tolerance was
weakened to obtain a pass.

The additional fine-exposure identity capture reproduces the earlier maintainer
sequence: remove declared entry 2, reuse its slot for entry 20 from template 10,
reject `add 2 10` unchanged, restore from template 2, accept a repeated no-op add,
remove entry 20 and continue ticking. Restoring near the focus gives a more
crowded compression target. That subsequent aggregate request reaches the
configured placement-cell bound: required 65,537 against 65,536. The completed
state and claims remain unchanged, and the next fine-exposure tick succeeds.
This is a recorded capacity limitation; the capture does not claim successful
compression of that crowded arrangement or relax the certified search.

Machine-local archive: `/tmp/hcp-whole-6a94343b-evidence.tar.gz`, SHA-256
`82202217e60b3164eb7b3f1b88a18058ce990f4a58f29d9d528bb56fef814612`. Fresh extraction verifies the root and every nested manifest.
It retains exact code/fork/runtime/toolchain/binary pins, commands/configuration,
clean and failed qualification, source equivalence and incremental Git bundle,
emitted LLVM modules/ABI manifests/negative controls, raw native/test/capture
evidence, ABI/setup probes and development failures. The five-test standalone
reuse has an exact source/test/build proof. The full local bundle is not remotely
published; the three bounded raw captures above are checked in for direct review.
The final documentation/capture commit leaves qualified executable source
unchanged, which is separately verified at handoff.


This is contributor CPU evidence for the stated subset. Hosted checks remain
authoritative, and GPU/concurrency execution is not inferred. Maintainer review
should reconcile dependencies in order #427 → #429 → this contribution before
integration. Next capability work is the fine confounding/next-level structural
trace alongside targets and motion; peripheral rollup and pattern/PoC acceptance
remain later work.
