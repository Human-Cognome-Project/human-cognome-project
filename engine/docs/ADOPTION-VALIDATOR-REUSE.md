# Adoption validator buffer reuse inspection

Status: **accepted and integrated in #381**, under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343) and the [refactoring plan](../../docs/engine-refactoring-plan.md). Tested source `244ef0b03da5db96693b26715035426e1a095090`, tree `dc9770b004b26dfd05859672a6531e2d2ee7cc21`, independently based on main `4bb937d8bd33d292da16dfdd6abdb36f065e3865`. Only executable tests change. Engine/fork/common/database/workflow implementations and build registration are unchanged. **Achieved runtime saving at this inspection source is zero.** The [separate runtime follow-up #382](https://github.com/Human-Cognome-Project/human-cognome-project/pull/382#issuecomment-7013865241) is provisionally accepted, pending its hosted native CPU gate; it is not included in this branch's runtime. Main `952e1651` integrates #381.

The earlier [adoption source-capture implementation](ADOPTION-SOURCE-CAPTURE.md) removed temporary copies of prepaid owner maps. It explicitly left reuse of validation buffers across selections separate. This inspection measures that remaining repeated construction and exercises existing private binding operations on an independent scratch object. It does not change the actual owner's constructor or substitute scratch facts for native admission inputs.

## Actual responsibility, lifetime and native overlap

`ExchangeOwner::ExchangeOwner` in `src/assembly/exchange_owner.cpp` retains one detached projection, trusted occurrence/field maps and membership catalogue. For each selected logical entry it collects and retains an exact exchange declaration, then constructs a local `ExchangeBindings check` to capture and immediately validate the current exposed alternative. The local is destroyed within that iteration. After all declarations, the owner constructs its spare projection and persistent coordinator/binding with their own prepared storage. The private adoption constructor in `exchange_bindings.cpp` synchronously reads the prepaid maps; it retains no map loan or duplicate map payload.

Each temporary reserves the same five buffers from the same Harness capacities:

| Buffer | Required purpose | Lifetime / scope |
|---|---|---|
| u64 masses | Exact source mass aligned with the complete active occurrence map | One capture's independent validation facts |
| Full outgoing handles | Declared outgoing alternative, including context/address/record/incarnation | Same capture |
| Edge facts | Actual row, both full endpoints, ordinal, float share bits and exact source count | Same capture |
| Outgoing-row packed mask | Identify precisely the outgoing native records | Fully reassigned from current source spans |
| Edge-row packed mask | Detect duplicate/missing/undeclared incident participation | Fully reassigned from current source spans |

Taichi/RecordMap and prepared Harness remain the execution and identity authorities. Native/current source headers, exact masses and participation rows supply validation inputs; these five host buffers are HCP provenance/revalidation evidence, not another physical model or a substitute native allocator. Their semantic checks cannot be removed merely because indexed native storage exists. The opportunity is to reuse already-supported scratch storage during one exclusive adoption call, not to replace captured facts with a live alias or introduce a new backend mechanism.

The basic private `ExchangeBindings(Harness&, domain, meter)` already reserves the buffers. Its existing `stage` invalidates the prior view and selects an immutable contract/direction/catalogue. `capture` clears facts, completely reassigns both masks, checks the entire current inventory and exact outgoing ledger, then charges and performs full validation before setting a valid current generation. Nothing in these operations requires a new buffer set for another selection within the same exclusive, unchanged source. Standalone value-taking constructors and the persistent operational binding have different retention/ownership obligations and remain outside this proposal.

## Actual current costs and isolated replay

The existing test-only allocation observer records real owner-adoption epochs. Each isolated fresh private constructor must match the corresponding actual constructor's ordered acquisition/release trace, request count, full retained facts and work. Thus the first two columns below are observed current runtime requests, not vector-capacity estimates or subtraction from an unrelated envelope.

The new test adapter constructs an independent scratch binding once through its existing basic constructor. It preserves the adoption constructor's checked map-capacity sums and inventory bounds, then invokes existing `stage`/`capture` with synchronous const-map arguments. Two passes visit all selected declarations in forward then reverse order. Complete fresh-reference snapshots compare masses, all handle fields, each edge's row/endpoints/ordinal/share bits/count, every mask bit, declaration/catalogue identity, generation and validation work. The mixed fixture changes outgoing counts and roles; the equal-sized fixture changes semantic identities despite equal body counts and mass.

Snapshots and fixed observer storage are provisioned before the allocation ROI. Positive recapture is guarded by `no_new`; allocator attempts/releases, selected-domain live bytes and cumulative request charges stay fixed. No actual owner, current/candidate map, projection, native generation, numerical/packet/history state or compiled binding is mutated by the scratch comparison. Actual current/candidate backing and full source mappings are checked again afterward. Captured reference snapshots are diagnostic copies only and never supply an operational request.

| Fixture | Actual current temporary requests / cumulative requested bytes | Scratch requests / bytes / self peak | Candidate reduction in requests / cumulative bytes |
|---|---:|---:|---:|
| One selection | 5 / 43,280 | 5 / 43,280 / 43,280 | 0 / 0 |
| Four mixed selections | 20 / 173,120 | 5 / 43,280 / 43,280 | 15 / 129,840 |
| Three equal-sized alternatives | 15 / 129,840 | 5 / 43,280 / 43,280 | 10 / 86,560 |

Scratch allocation count remains five across both passes and healthy retries. Fresh temporaries already have nonoverlapping lifetimes; their cumulative bytes must not be called simultaneous retained storage. The scratch self peak equals one fresh capture-buffer set, not the sum across selections. The actual whole-owner retained/observed requested peaks remain 242,404 / 294,636 / 258,014 bytes at this source. Actual whole-construction cumulative requested bytes remain 292,961 / 484,240 / 397,901. No runtime owner, whole-process peak, allocator metadata, RSS, driver residency, transfer, timing or GPU saving is measured.

Logical work is accounted separately. Basic scratch construction costs **44,174 work units**, including the existing 376-byte stack-object setup fee, conservative buffer reservation work and packed-mask rounding charges. Complete per-selection stage/capture/validation work is unchanged and matches the fresh constructor after removing that repeated construction charge:

| Fixture | Fresh temporary constructor work total | Scratch forward-pass work | Scratch forward+reverse work | Candidate one-pass work reduction |
|---|---:|---:|---:|---:|
| One | 49,265 | 49,265 | 54,356 | 0 |
| Four mixed | 281,098 | 148,576 | 252,978 | 132,522 |
| Three equal | 163,971 | 75,623 | 107,072 | 88,348 |

The test requires `fresh = one_pass + (N-1)*base` and `two_passes = 2*one_pass-base`. These are existing bounded logical work units, not timings or FLOPs. The sizeof-based header charge is a type extent, not a measured compiler stack frame or stack saving. No full capture/validation scan is deleted or priced from empty owned-map headers. Actual whole-owner setup work remains 381,949 / 772,470 / 528,149. Candidate quantities are not an achieved whole-owner result.

## Failure, validity and cleanup controls

For every selection the same scratch object refuses zero and one-less stage/capture work, must advertise no valid prior selection, then succeeds at exact work with fresh-reference equality and no allocation. Selecting the opposite direction against the unchanged committed exposure also refuses, invalidates the prior view and permits exact healthy recapture. Both passes preserve all declaration-specific full facts and complete masks. The old actual-constructor zero/short/exact and independent oversized-field bound tests remain intact.

Expected exception diagnostics are outside the successful allocation ROI; the capture buffers remain tagged throughout and no diagnostic text is mistaken for a capture allocation. The scratch domain's actual charged bytes remain exactly the five requests. All five buffers release at destruction, then its strong and weak domain references release fully. The existing observer also verifies full owner/alias/constructor cleanup and unchanged native execution. These controls exercise existing scratch operations; they do not implement or qualify a new production failure/publication path.

## Reviewable next proposal

A subsequent runtime slice could retain one existing private `ExchangeBindings` local around the adoption declaration loop and reuse its prepared capture buffers for each retained declaration. Keep its scope inside that loop boundary and destroy it **before** spare-projection/coordinator preparation; do not add an owner member or share it across owners, generations, source revisions or operations. Use the current const source maps synchronously and retain each independently owned immutable declaration as before. No new wrapper, registry, source-view lifetime or public API is needed.

Preserve both checked map-capacity sums, independent inventory bounds, full current-source/typed identity checks, all masks/facts, explicit direction and validity invalidation. If constructor setup is factored into a private reuse method, both the fresh constructor and owner loop must invoke the same bounded behavior with the **existing shared setup meter**. The scratch probe's per-attempt meters do not authorize a private unlimited production allowance. Only eliminated repeated object/buffer construction may lose work/request charges; every selection still pays complete stage/capture/validation work. Standalone owned-map semantics and the persistent coordinator/binding remain unchanged.

Initialization before the first declaration and holding buffers during later declaration collection changes temporary overlap and failure order. The replay starts from already-retained declarations and does not qualify that new interleaving or a whole-owner peak reduction. Runtime acceptance must measure actual before/after owner totals and overlap, preserve byte/allocation/setup-work refusal and exact retry, constructor unwind and strong/weak/domain cleanup, and retain the existing native owner/exchange/mapping/survivor/publication/failure/tick cases. Preserve the single-selection control, mixed and equal-sized alternatives, source-pin/claim/direction/generation refusal and independent standalone lifetimes. Review must authorize runtime implementation first.

## Source-pinned local evidence

At the exact clean source, native x64 CPU execution uses Clang/LLVM 15 and GCC 11 wrappers with the matched Clang-built fork tree `8b31651be07d8bad0b2a6a261369e8690b9c3b53` and recursive submodule pins. Warm/viewport support is enabled; GPU backends/qualification are disabled.

- Quick CI passes its three native CTests, 632 independent placement arithmetic samples and placement-capacity witness.
- Clang passes all four `exchange_adoption_binding_cost` modes (1/2 workers × bulk/direct) and the complete existing `exchange_owner_multiple` case on 2-worker direct: **five case invocations**.
- GCC passes the dedicated probe on 1-worker bulk and 2-worker direct: **two case invocations**.

The dedicated probes supply 18 fixture observations; the complete mixed-owner control includes one additional comparison, for **19 observed fixtures**. All compilers/modes agree on the request/work tables. Each N-selection fixture compares 2N forward/reverse captures and 3N exact healthy recaptures after 3N refused work/direction attempts. Fresh-reference constructors still match actual adoption traces and retain their independent work/bound/retry checks. The full mixed-owner case then covers its ordinary native owner/exchange/lifetime/refusal/publication/following-tick behavior. All three profile result files exit zero with unchanged clean source before/after. Documentation follows separately and changes no tested code.

The initial unpinned development probe passed all three fixtures; its build/test logs are retained separately and are not qualification counts. No unsuccessful build/test was observed in this slice. Existing post-PASS native cache-lock warnings are preserved, with no cache cleanup. No accepted baseline was rerun merely to onboard.

Verified machine-local bundle: `/tmp/hcp-validator-reuse-244ef0b0-evidence.tar.gz`, SHA-256 `82bf40eebf3a6f63fcc317e1942a645e1ceee8e5361e0178aaa1e71f6531e119`. It includes exact commands/configuration, source and unchanged-runtime patches, source-clean checks, native/toolchain/fork/submodule identities, binary/runtime hashes, actual constructor/replay/work/lifetime logs, development records and verified profile/bundle manifests. Publishing a digest does not make these local raw artifacts remotely available.

Reproduction on the matching native build:

```sh
bash engine/scripts/run_local_ci.sh quick /absolute/new/quick-evidence
engine/build/live_hierarchy_test --threads 1 --case exchange_adoption_binding_cost
engine/build/live_hierarchy_test --threads 2 --direct --case exchange_adoption_binding_cost
engine/build/live_hierarchy_test --threads 2 --direct --case exchange_owner_multiple
```

The retained command script includes all four Clang worker/path probes and exact GCC configuration/two mode commands. Existing registered placement/exchange-owner cases call the instrumented adoption-cost test, so hosted native regression exercises the new inspection without an extra runtime branch or build registration. GitHub CI remains authoritative. No full local regression, no-unroll matrix, database or GPU qualification is claimed. All merges remain with the maintainer.
