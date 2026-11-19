# One adoption-local validator

Status: **contributor runtime implementation awaiting review**, under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343). The maintainer [accepted #381 and authorized this implementation](https://github.com/Human-Cognome-Project/human-cognome-project/pull/381#issuecomment-7001610867), explicitly requiring validity invalidation before inventory refusal, the same setup meter for each selection, scratch destruction before spare/coordinator preparation, and separate cumulative/peak measurements. This slice is stacked on accepted, unmerged #381 at `616a29875417674894ffb11339966c74ae3daa17`, including current main `3e76bf1e`. Tested runtime source: `3d9424e478f3768b180ab70b6462f55f480c2f85`, tree `69aa8916dbfbee30cd80a814bc9b944540e2e4e2`. The [inspection](ADOPTION-VALIDATOR-REUSE.md) retains its original scratch-only evidence.

## Runtime boundary

`ExchangeOwner::ExchangeOwner` now constructs one existing `ExchangeBindings` local around the adoption declaration loop, rather than constructing one per selection. Its five buffers are reserved once. Each declaration still comes from the current explicit exposure, pinned source and membership catalogue, remains independently retained in `declarations_`, and receives complete source/handle/mass/participation/mask validation. The local's lexical scope ends before `SpareInspection` and therefore before spare projection or persistent coordinator preparation. No scratch member, registry or cross-owner/operation/generation sharing is introduced.

The new private `ExchangeBindings::capture_adoption` centralizes the former private constructor's checked capacity sums, independent row/field inventory bounds, explicit staging and complete capture/validation. Its **first operation sets `valid_ = false`**, before reading limits or evaluating checked sums. Thus an inventory refusal before staging cannot advertise the previous selection's capture as current. `stage` and `capture` retain their existing invalidation and full reset behavior. The fresh private constructor and the owner's reusable local use this same method; the test adapter delegates to it instead of duplicating its guards.

The method requires the existing shared `PreparationWork&`. Every selection pays unchanged complete stage/capture/validation work. Only repeated object/buffer construction charges disappear. No private unlimited allowance or validation shortcut is added. Standalone value-taking constructors, owned maps, operational binding preparation, source argument lifetimes, candidate/current map reservation, native execution and atomic publication are preserved. Map arguments are synchronous const loans and are never retained. Taichi/RecordMap/Harness remain the execution authorities; captured HCP provenance is validation evidence, not a second physical state.

## Actual before/after, including peak

The before binaries were built from the exact accepted parent `616a2987`, checked clean before/after and retained with their hashes and matching runtime. Both compilers run the same one-, four-mixed-, and three-equal-alternative fixtures on 1-worker bulk and 2-worker direct. After binaries measure the real modified production constructor. These are actual owner request/work observations, not candidate subtraction or a scratch-only result.

| Fixture | Actual whole construction requests before → after | Cumulative requested bytes before → after | Actual whole setup work before → after | Retained / requested peak before = after |
|---|---:|---:|---:|---:|
| One selection | 1,759 → 1,759 | 292,961 → 292,961 | 381,949 → 381,949 | 242,404 |
| Four mixed | 2,300 → 2,285 | 484,240 → 354,400 | 772,470 → 639,948 | 294,636 |
| Three equal-sized | 1,924 → 1,914 | 397,901 → 311,341 | 528,149 → 439,801 | 258,014 |

The actual production capture lifetime contains **five requests / 43,280 bytes** for each fixture, instead of 5N requests / N×43,280 bytes. Real cumulative reductions are **15 requests / 129,840 bytes** and **10 requests / 86,560 bytes** for four/three selections. The single-selection control is unchanged. Logical setup work drops by 132,522 / 88,348 units, exactly `(N-1)*44,174`; no per-selection capture or complete validation scan is removed.

Requested peak is measured independently. The production temporary's actual self peak remains 43,280 bytes. Whole requested peaks and retained bytes remain unchanged in these fixtures. Maximum whole-owner requested bytes observed while a temporary is live are also unchanged: 81,850 / 112,619 / 92,073 bytes for one/four/three selections. The observer requires all temporary backing released before the first spare/coordinator request. Current/candidate map census stays 94,360 bytes; independent candidate backing remains 47,180 bytes across 562 requests. Other released setup requests stay 7,277 / 16,484 / 10,047 bytes.

A standalone native C++ comparison tool reads the retained before/after logs, requires complete passing fixture sets, and checks actual request/byte/work deltas against the eliminated construction costs. It separately verifies unchanged retained/peak/map backing/other-setup fields and the measured temporary-overlap values. All **12 fixture/configuration comparisons** agree (Clang/GCC × 1-worker bulk/2-worker direct × three fixtures). The extra Clang host/worker probes reproduce the same after measurements.

These are calling-thread C++ allocator request and logical work measurements. The observer excludes its own reference storage and does not measure allocator metadata, complete stack frames, malloc/driver allocations, RSS, physical residency or throughput. The scope extension changes overlap and failure order even though the observed peaks above are equal; no general peak reduction or full combined accounting claim follows.

## Validity, lifetime and failure evidence

The real constructor's five ordered acquisitions/releases must match an isolated fresh private binding's capture-buffer trace. One production allocation lifetime spans all selections, and an explicit phase observation fails if any capture backing survives into spare/coordinator preparation. Full reference snapshots compare exact masses, context/incarnation/address/record for every outgoing handle, every captured row/endpoints/ordinal/share bits/count, every mask bit, declaration/catalogue identity, generation and validation work. Forward/reverse selection passes and exact retries allocate or release no capture backing.

The shared production method is exercised after a valid capture with zero and one-short work and wrong source exposure/direction. Refusal must invalidate that old capture; exact healthy recapture reproduces fresh facts. Two added controls pass oversized row and field inventories **before staging**, require the existing bounds error with zero stage/capture charge, then validate against the original healthy maps: that validation must return InvalidInput. This would fail if the old valid flag survived the early bounds error. Exact retry succeeds on the same buffers. Diagnostic exception allocations are outside the successful replay ROI.

Actual whole-owner tests preserve all underlying allocation-failure, byte/allocation-work/setup-work refusal boundaries, zero/one-short/exact retries, partial-construction unwind, strong/weak/bootstrap/domain release, caller-source lifetime and source/claim/role checks. The full single/mixed/equal owners retain native survivor/retirement, stale-map/source refusal, ordinary retry, postclaim unreadiness, atomic projection/mapping/report publication and following ticks. Standalone binding/commit/field lifecycle cases retain independent owned-map semantics. No failed execution is reset, repaired or resumed.

## Source-pinned validation

At the exact clean source, actual x64 CPU execution uses Clang/LLVM 15 and GCC 11 wrappers with the matching fork tree `8b31651be07d8bad0b2a6a261369e8690b9c3b53`, exact recursive submodule pins and GPU backends/qualification disabled. Warm/viewport support is enabled.

- Quick CI passes three native CTests, 632 independent placement arithmetic samples and the placement-capacity witness.
- Clang passes **13 native case invocations**: four worker/path cost probes; all three complete single/mixed/equal owner cases on 1-worker bulk and 2-worker direct; standalone bindings, atomic commit and field lifecycle on 1-worker bulk.
- GCC passes **8 native case invocations**: 1-worker bulk/2-worker direct cost probes; all three complete owners and the three standalone cases on 2-worker direct.

The after runs contain **27 actual production allocation/lifetime observations**, including 18 dedicated probe fixtures and nine complete-owner fixtures. All include work/role refusal and exact retry plus two pre-stage inventory controls each (**54 inventory-refusal checks**). Four exact before binary/configuration runs supply 12 fixture observations for the checked before/after comparison. All three final profile results exit zero with unchanged clean source before/after. The engine runtime substrate, native field code, fork, workflows, database and build registration remain byte-identical to the accepted parent. Subsequent documentation changes no tested code.

Development's cost and full mixed-owner probes passed; these unpinned logs are retained separately and excluded from qualification counts. No unsuccessful test/build was observed in this slice. Existing post-PASS native cache-lock warnings are retained without cache cleanup. The before runs are necessary measurements for this runtime change; no accepted full baseline was rerun merely to onboard.

Verified machine-local bundle: `/tmp/hcp-validator-runtime-3d9424e4-evidence.tar.gz`, SHA-256 `6be2ba3d787b92e34792b0d566f1ad21f360ebccfea59e3371a5b4e736876189`. It contains both baseline binaries, before/after source identities and clean checks, exact commands/configurations, source/native-boundary equivalence patches, raw case/cost/work/lifetime/failure logs, comparison source/binary/results, toolchain/fork/submodule/runtime/binary hashes, development logs and verified profile/bundle manifests. A published digest does not make machine-local raw evidence remotely available.

Reproduction on a matched native CPU build:

```sh
bash engine/scripts/run_local_ci.sh quick /absolute/new/quick-evidence
engine/build/live_hierarchy_test --threads 1 --case exchange_adoption_binding_cost
engine/build/live_hierarchy_test --threads 2 --direct --case exchange_adoption_binding_cost
engine/build/live_hierarchy_test --threads 2 --direct --case exchange_owner_multiple
```

The retained scripts include both exact before builds, all after probe/owner/standalone modes, the native comparison tool and configuration, source-clean checks, fork/submodule/toolchain/runtime identities and verified manifests. The existing hosted placement/exchange-owner suite executes the new production-lifetime and refusal controls. GitHub CI is authoritative. Full local regression, the no-unroll matrix, retained database requalification, GPU and performance remain outside this local evidence. No store, schema or persistent source is changed. All merge events remain with the maintainer.
