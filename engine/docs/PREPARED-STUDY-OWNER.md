# Private prepared study bootstrap owner

The subsequent [private instance-binding proposal](PRIVATE-INSTANCE-BINDING.md)
compares this owner with ExchangeOwner and defines the source/lifetime/publication
requirements for sharing associations. It is a contract proposal; the existing
owner implementation and qualified scope recorded below are unchanged.

Contributor slice under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343),
2026-11-16, awaiting review. Starts from main `422a32e1`, explicitly dependent on
source-accepted [#348](https://github.com/Human-Cognome-Project/human-cognome-project/pull/348)
at `9221599ac2bd7f697e329cc72d0d698d52724f8c`. That PR's native run 43875676396
now reports success; #348 remains unmerged at inspection. Documentation-only
main `f34afd62` is reconciled after qualification, preserving its
[coherence/controller and native-instance inspection direction](../../docs/engine-refactoring-plan.md#agent-focus-coherence-frames-and-analyst-control).
Tested public implementation: `a97b604932f2d5f4463e67dab3e9a37af5a35bb4`;
Taichi tree `8b31651be07d8bad0b2a6a261369e8690b9c3b53`, exact recursive pins.

## Scope and control trace

[`detail::PreparedStudyOwner`](../src/assembly/prepared_study_owner.h) joins the
accepted [structural plan builder](STRUCTURAL-ADMISSION-PLAN.md) to existing
combined native bootstrap. It is an exclusive logical owner on a **borrowed**
prepared Harness. Runtime outlives Harness; Harness outlives this owner and
borrowed views. It owns an immutable source pin, plan, pointer-free reporting
scales and reserved actual-handle tables aligned with the plan's typed keys.
It creates no Runtime, native allocator, physical storage, placement solver,
instance hierarchy or scheduler. Native ownership remains in Harness/RecordMap/
NodeManager; the HCP bridge binds pinned identity to committed native claims.

The preparing caller supplies a pinned `shared_ptr<const ActiveAssembly>`,
logical session identity, explicit `AdmissionBindings`, an empty prepared Harness
and an existing reservation domain. Caller/controller choices remain source,
exposure, focus and interventions. No Analyst policy or comparison topology is
selected here. A concrete trace, exercised by the tests, is:

1. `PreparedStudyOwner::prepare` verifies the empty target, invokes existing
   source/binding validation, retains the source pin and reserves metadata before
   any native claim. Plan and metadata work have separate allowances. Preparation
   publishes no model; source/maps cannot be read as current yet.
2. `bootstrap` authorizes owner/session/study/logical-generation identity, checks
   native context/generation and invokes private `Harness::stage_current_admission`
   with bootstrap/commit enabled. Native placement, exact source, hierarchy,
   focus, contacts and actual claims remain one transaction. No fixture admission
   or host-supplied starts enter this path.
3. After native commit, fixed-size assignments resolve each sealed command through
   `PreparedStorage::resolved`. Full context/incarnation/address/record/header
   checks precede the Published phase. No key copies, callbacks, resizing or
   ordinary resource refusal remain in this publication step.
4. `tick` delegates to native contact-safe `tick_prepared`. A successful
   contact-changing tick advances native generation; the owner follows that
   committed result while retaining occurrence/field handles. Logical source
   generation and completed tick remain distinct identities.
5. `viewport::observe(PreparedStudyOwner, owner, handle, request)` authorizes the
   published owner, builds detached identity/scale reporting metadata and reuses
   existing `viewport::bind/observe(Harness)` and snapshot serialization. Physical
   values and optional combined intent come from native completed state. Report
   copying is requested observation work, outside commit/publication; legacy
   staging members in this wide mapping carry no physical-state authority.
   No SNode pointer is exported.

Implementations: [owner](../src/assembly/prepared_study_owner.cpp),
[completed-state adapter](../../tools/viewport/native/prepared_owner_state.cpp).
The actual ActiveSession/letter CLI still uses FixedStudy and reports its own C3
request unavailable. This private route does not silently migrate it.

## Refusal, lifetime and resource limits

Ordinary preclaim work/capacity/input refusal retains the healthy empty Harness,
logical pin and handle for retry. Failed preparation unwinds selected requests
and exposes no owner. Two pending views cannot both publish: a stale native
generation refuses without changing the current owner. Repeated bootstrap is
NoChange only while full native handles remain current. Wrong owner/logical
handle rejects before payload access; current checks include context, generation
and full committed handles.

Backend/invariant failures after claims, including before owner-map visibility,
mark Harness and owner unready. Partial maps remain unavailable. There is no
healthy rollback claim or generic reset. Later placement scratch reuse does not
change copied handles. Owner teardown releases its source pin and metadata
without retiring borrowed native state; resource teardown stays with Harness/Runtime.

Existing reservation/work helpers bound selected plan/owner construction,
metadata arrays and one successful bootstrap map publication. Caller snapshot/
shared-control-block/domain construction and borrowed native backing remain
separate surfaces. Ready/current-map checks and detached observation retain
their bounded present-count/reporting scope; no complete combined operation or
C+B+H+D accounting envelope is claimed. C++ new/new[] counters are distinct from
malloc, physical residency and the native backing/compilation census. See
[borrowed-resource limits](BORROWED-RESOURCES.md).

The subset remains V2/f64, explicit complete supported comparison tree, zero
root frame, explicit nonempty focus and available shared inputs. Local overlays,
general live edits/exposure exchange, restart/rebase and capacity expansion are
not supplied here. The current prepared model remains a comparison surface for
the nested-instance/accounting audit, not acceptance of a parallel flat
constituent/lifecycle framework as necessary.

## Integration findings and qualification

The [retained-entry inspection](RETAINED-C3-HANDOFF.md) (contributor evidence,
awaiting review) demonstrates this existing bounded bridge alongside legacy
ActiveSession execution. Bootstrap publishes initial C3 through combined native
admission, not the standalone C3-1 installer. Source ownership and actual claim
mapping are already built here; the retained capture adapter still needs an
explicit prepared request/context lifetime handoff. Its fixture reproduction
does not qualify current retained database pins or the full retained study.

Native field declaration extent is `Fields`; particle declarations use
`Potential`. Development exposed that #348's validator checked field addresses
against Potential. Native rejection stayed ready and preserved state. Separate
correction `5cea1446c542e80360df73e3b509a02833da22e4` enforces Fields and adds a
named-bound control. No native bound or assertion was weakened.

Earlier owner `6f80a40a` passed its original quiet-tick qualification. Final source
inspection identified missing following of native generation changes caused by
contact publication. Corrected `a97b6049` adds moving-body controls: positive
gravity produces a contact-changing tick, retains current handles and supports
observation at tick 2. The earlier pass does not qualify this added behavior.

Personally executed at public `a97b6049`, Linux CPU, matched LLVM/Clang 15 fork:

- `bash engine/scripts/run_local_ci.sh quick /tmp/hcp-owner-quick-a97b6049`:
  PASS, all native targets rebuilt; actual CPU, one worker, bulk. Three CTests,
  632 independent placement-math samples and placement_capacity passed.
- `/tmp/hcp-owner-native-a97b6049.sh`: 11 CTests passed, 500.75 seconds. Owner
  and observer cover workers 1/2 × bulk/direct. Includes the corrected builder
  and existing 165 workbench/64 viewport checks.
- Owner checks: 197 in one-worker bulk, 187 in each other profile. Covers exact
  allocation/work refusal/unwind, actual claims/field inventory, zero-motion D1
  anchor, unchanged-state rejection/retry, native backing/compilation reuse,
  allocation-free bootstrap/no-op/ticks, scratch independence, stale pending
  owner, positive-motion contact generation, source retention, borrower teardown
  and postclaim/unpublished-map failures.
- Each observer profile passes 34 controls at ticks 0/1/2: backend, occurrence/
  full handles, native/logical generations, pointer-free scales, serialization,
  read-only optional intent and budget refusal.
- `/tmp/hcp-owner-gcc-a97b6049.sh`: fresh GCC 11 native-wrapper build passed
  builder (4,455 controls), bulk owner and observer; three CTests, 147.56 seconds.
  Separate engine-independent GCC build passed four standalone projection tests.

Raw evidence: `/tmp/hcp-owner-{quick,native,gcc,record}-a97b6049`, with exact
commands/scripts, toolchain/configuration, source/submodule/binary hashes and
logs. Archive `/tmp/hcp-prepared-owner-a97b6049.tar.gz`, SHA256
`b0f1ac1726d0e9e6a74e91dcc84c5e37901831d7a1bfb716040690adcd482c16`.
Earlier quiet-tick archive `/tmp/hcp-prepared-owner-6f80a40a.tar.gz`, SHA256
`0782c5bc85b58d3f68fb66320be749b5e982e5e0b8c9112ee8ca4f55c1f18917`.
These are machine-local contributor artifacts, not remotely available or
independently inspected execution. Post-PASS cache-lock warnings are retained.
Full local regression/compiler matrix, database, browser and GPU execution were
not run. GitHub CI remains authoritative; disposition is recorded in the PR.

## Next inspection

Follow the [updated #343 inspection brief](../../docs/engine-refactoring-plan.md#inspection-clarification-nested-assemblies-and-live-instances)
before further instance/lifecycle wrappers or public producer expansion. Trace
controller requests through controls/execution/completed observation and compare
nested reusable assemblies, two live instances, contextual attachment, rollup
and release/accounting against exact Taichi mechanisms. State native authority,
HCP semantics, redundant state and missing support with source-pinned witnesses.
This slice does not complete that audit, enforce the intended coherence frame/
mathematical degrees of freedom, or relabel combined intent as #324 structural
intent. No new mathematical or Analyst policy is selected.
