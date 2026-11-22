# Serialized removal of unregistered native compiled data

Status: **implementation awaiting review**, under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343). Implements [#406 decision 7081114329](https://github.com/Human-Cognome-Project/human-cognome-project/pull/406#issuecomment-7081114329) from provisionally accepted `33da9a0f`. Main `92e4ceaf` integrates #405 with reviewed `06e71404`'s exact tree. #406 smoke passed; authoritative native CPU qualification remains pending at this handoff. This contribution is stacked on #406; every merge remains maintainer work.

Tested source `505ab435c0bfaf7018517f50b8678512c443927f`, tree `921e8cccf2bfc3090e1d06082583cc025860872c`, fork tree `71e58621eac5373cbfcd71a0db29b469a85093b7`. Later report/handoff files are Markdown only. Matching native/wrapper rebuild is required: native compiled-data C++ layout changes; runtime bitcode, serialized compiled-data/AOT formats and numerical formulas are unchanged.

## Native operation and caller contract

[`KernelCompilationManager::remove_unregistered_kernel`](../taichi/taichi/compilation_manager/kernel_compilation_manager.cpp) selects the **current record for an existing key** in the existing native owner. It takes an entry budget, inspects both pending and disk metadata maps, then reports a status and visited-entry count. Inspection never creates provenance, loads a disk object, compiles, registers, dumps or purges. The count API's `complete=false` remains truthful: a complete bounded map scan is not a complete resource census.

The caller must hold live, exclusive, quiescent Program ownership, keep its LLVM context alive, and serialize removal against cache mutation, borrow acquire/release and registration. The operation provides no lock, drain, concurrent lifetime guarantee or independently owning context. Existing external Invocation/derived-reference lifetime obligations still apply. The budget limits inspected entries; it is not a bound on key bytes, LLVM destruction work or complete resource size. It is an explicit operation, with no automatic call added to submission, refusal, retry or teardown.

| Status | Boundary, before mutation |
| --- | --- |
| `Incomplete` | The budget cannot cover both existing maps. No entry is scanned and zero visits are reported. |
| `NotFound` | The current key has no pending record and no disk metadata row. |
| `DiskBacked` | Any matching disk metadata row, including a shadowed key, or a pending `MemAndDiskCache` record. No metadata row or `updated_data_` pointer is erased. |
| `Unsupported` | The actual record is not LLVM/x64, or this substrate lacks LLVM support. Both architecture and concrete LLVM type are checked. |
| `UnknownBorrowers` | Missing object/provenance, another owner, or any sticky raw-client exposure. |
| `RegistrationStarted` | Any present launch handle or sticky CPU registration attempt, including a failed partial registration without a handle. |
| `LiveBorrows` | One or more tracked guards still own borrower coverage. |
| `Removed` | Exactly one pending `MemCache` LLVM/x64 object has positive owner provenance, no unknown/raw clients, no registration attempt/handle and zero tracked borrowers. |

All ordinary refusals precede the sole mutation: erase that pending entry through its existing unique ownership while the Program/context remains alive. Its ordinary destructor invalidates borrow metadata. No launch row, JIT module, ORC resource, disk metadata or disk file is retired. A repeated call sees absence. Selection uses the current key, not a saved pointer or incarnation; a later request may allocate a fresh object at a reused address.

CPU registration now sets `CompiledKernelData::mark_registration_started()` **before** handle allocation, row resize, cloning or JIT creation. Handle absence alone cannot prove that those side effects never began. The marker is sticky and nonallocating. Clone/deserialization creates a fresh unregistered object; it does not copy a source launch history. This slice leaves partial rows/modules to their existing owners and shutdown path.

## Executable refusal, removal and retry

[`unregistered_removal_test`](../tests/unregistered_removal_test.cpp) runs actual HCP i32-store kernels on LLVM/x64 CPU with one/two workers and prepared/dynamic dispatch. Two warmed survivors keep their exact data handles, resolved task symbols and independent output bindings. Two candidate definitions share one never-registered record. Two/one live guards refuse; after both guards release, a short budget refuses and exact work removes the record. Delete observations confirm destruction of that exact object and its metadata, without dereferencing either afterward.

The pending-object count changes 3→2 while row and JIT counts remain equal. Repeated removal reports absence. Retry acquires a fresh unregistered lifetime, compiles through the now-empty key, registers a new append-only row and dispatches. Existing handles/tasks and actual following survivor dispatch remain correct. Pointer inequality is not used as a fresh-lifetime proof.

Controls also exercise sticky raw exposure through a same-key CUDA-named request returning real x64 data; zero-borrow registered refusal; and an injected JIT-creation failure after native row creation. That partial record has no handle, has the marker, retains its row, and refuses after all guards release. The fixture does not attempt registered recovery or retire the partial row.

A verified LLVM fixture injects allocation failure after cache insertion but before metadata creation. Removal refuses the resulting unknown record without manufacturing coverage. Explicit tracked reacquisition establishes coverage; live-borrow refusal and successful removal after release follow. A disposable disk fixture separately checks unloaded metadata, an explicitly loaded object, pending disk-cache data and a positively tracked pending shadow of a disk key. Counts and owned file bytes remain unchanged on refusal. Metadata-only foreign architecture/type fixtures require no GPU code generation or driver execution. Only files created by these fixture instances are removed.

## Measured effects and limits

All seven qualified cases release **83 observed C++ requests / 7,010 requested bytes** during the candidate removal call, with zero new C++ requests and pending objects 3→2. The fixed-storage observer captures current-thread C++ new/new[] during construction, then freezes acquisitions before removal. It observes selected released requests, including the native object/IR metadata, cache node/key and borrow control; it does not observe arbitrary malloc, LLVM arenas, other threads, executable allocations or physical memory. A separate successful removal covers recovery from unknown coverage.

| Host object | Corrected #406 source | This source |
| --- | ---: | ---: |
| HCP `CompiledKernel` | 48 bytes | 48 bytes |
| Native `CompiledKernelData` base | 32 bytes | 40 bytes |
| Concrete LLVM compiled record | 160 bytes | 168 bytes |
| Guard | 16 bytes | 16 bytes |
| First borrow/control allocation request | 48 bytes | 48 bytes |

The base/concrete eight-byte deltas overlap; they are one inline increase per LLVM record, not sixteen. No new metadata allocation or persistent manager table is added. Existing guard-only moves/observation/release remain allocation-free; raw/tracked API hit observations retain their before costs. Empty wrapper slots retain their original guard cost. No complete owner peak, executable-byte recovery, RSS reduction, B charge, admission credit or resource policy is established.

## Source-pinned qualification

Actual backend: LLVM/x64 CPU, Release, Clang/LLVM 15.0.7 native substrate, CUDA OFF. Wrappers use Clang 15 and GCC 11. Exact recursive submodules and runtime bitcode SHA-256 `4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada` are preserved.

- `HCP_LOCAL_BUILD_JOBS=4 bash engine/scripts/run_local_ci.sh quick /tmp/hcp-removal-505ab435-quick`: matching build/backend, three CTests, 632 placement-math samples and capacity check pass.
- `bash /tmp/hcp-removal-checks.sh clang`: nineteen CTests pass: all four legacy modes, prepared one-worker bulk/two-worker direct, all four sharing modes, all four new removal modes, plus guard, provenance, loaded-count, prepared-launch and owner-backing controls.
- The same command with `gcc`: thirteen CTests pass, including one-worker bulk/two-worker direct legacy/prepared, one-worker prepared/two-worker dynamic sharing/removal and those five controls.
- The same command with `minimal`: warm/viewport OFF, guard, provenance, standalone prepared sharing and one-worker prepared removal CTests pass.
- `perl /tmp/hcp-removal-audit.pl /tmp/hcp-removal-505ab435`: seventeen before lifetime histories, 260 native-count snapshots, 243 borrow snapshots, three guard profiles, six architecture assertions and seven removal cases reconcile. Guard/control extent, raw/tracked hit costs (three requests/263 bytes), source/native counts, move/unwind and surviving dispatch remain preserved; only native inline extent rises eight bytes.

Independent focused wrapper builds may run concurrently after the one matching substrate rebuild; CTests are serial within each profile. Clean-source/tree checks bracket every profile. HCP runtime, assembly/field/numerical code, prepared origin/ABI checks, row indexing, JIT/ORC ownership and workflows are unchanged. The only CPU registration change is the marker before existing side effects. Guard observations continue to report `reclamation_supported=false`: that observation alone cannot certify the new manager operation's full eligibility contract.

Verified machine-local archive `/tmp/hcp-removal-505ab435-evidence.tar.gz`, SHA-256 `addb90e77d65216b862dc60ffb1476cd5ade45112b53ca869e088e4c4e730df6`, retains raw logs, commands/configurations, source/tree/submodules, binary/runtime hashes, source equivalence, development failure/source/correction and all audit observations. It embeds verified #406 provenance evidence (`d2b3f3dc8373e31313e15c680158145f52e970a291f4871b4b7299adbde6ea30`). Reused before working logs match their manifest-verified archived copies. Root and all four profile manifests verify after fresh extraction. This hash identifies retained contributor evidence without publishing the machine-local bundle.

Development initially armed the unknown-record allocation fault before LLVM verification, so it threw before insertion and the fixture refused to claim coverage. Its failed log and exact source are retained. A separate ordinary commit moves injection after full LLVM verification; production is unchanged by that correction. The passing development run is supplemental to clean canonical qualification. Existing shutdown cache-lock warnings remain in raw logs; no persistent-store/cache reset, manual dump or purge is used.

Return this implementation and measured effects for maintainer review. Hosted GitHub CI is authoritative. Automatic policy, loaded/registered retirement, launch invalidation, row reuse, ORC dependency/error handling, concurrent acquisition/dispatch safety, GPU qualification, complete B/admission and #324 remain separately scoped.
