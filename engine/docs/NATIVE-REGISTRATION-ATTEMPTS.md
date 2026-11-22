# Native CPU registration attempts

This bounded #343 implementation follows [maintainer review 7082960643](https://github.com/Human-Cognome-Project/human-cognome-project/pull/408#issuecomment-7082960643).
It records registration progress beside existing native CPU launch slots and
retains a lifetime-safe compiled-record identity. It adds no execution authority,
registered retirement, row reuse, ORC cleanup or admission credit.

## Source and dependency

Tested source: `635ee3eaad820e2a43d0d38526b761b1edd21955`, tree
`06a81c994f9f94adbed12d801486a29e108656b7`, fork subtree
`d412616af631a24773b27ff47006090c6756a41b`.
The parent is resolved #408 `4747d1134516eadc72cbe0c1665b91ac2022f2c8`, tree
`5a9ad101760226aa9d3a0e7cec34ef6932c8338d`. That tree exactly equals #409's
qualified correction head `95eb275d`; the maintainer incorporated both original
PR histories without rewriting them. Main `ff82d2cc` integrates qualified #407,
whose reviewed tree is `69dd8a62`. #408 remains open pending its hosted gate and
maintainer integration. A chat acceptance is a review signal, not merge evidence.

The original [registration-failure report](NATIVE-REGISTRATION-FAILURES.md),
[unregistered-removal report](NATIVE-UNREGISTERED-REMOVAL.md) and their original
pins remain unchanged. Their combined `cf26b62e` evidence is the comparison
baseline for the seven original registration histories, not a new production
baseline rerun. The new implementation has its own source and archive below.

## Native ownership and observation

`cpu::KernelLauncher::Context::Registration` lives inside the existing append-only
`contexts_` vector. It stores a record identity, confirmed module pointer, stage,
module-observation status and failure flag. One launcher counter records issued
slot IDs even when row allocation fails. There is no second registry or vector.

`CompiledKernelIdentity` shares the existing native borrow-state control block.
Copying it neither allocates state nor increments tracked client borrows. It has
no data accessor and retains no compiled data, Program, LLVM context or module.
Compiled-data destruction invalidates the same metadata. A copied identity can
then report `Absent` with a stable record token and null owner, even after Runtime
destruction. Holding metadata therefore cannot authorize access or reclamation.
Empty/unmanaged identities stay `Unavailable`; observation never creates provenance.

`ProgramImpl` delegates two const, non-lazy CPU queries to an existing launcher:
`cpu_registration_attempt_counts(entry_budget)` and
`cpu_registration_attempt(index)`. No existing launcher reports `Absent`;
unsupported launchers default to `Unavailable`. An individual query does constant
work. Counts scan at most the supplied budget; insufficient budget reports known
issued/stored sizes with zero visited/role counts. All calls require serialized,
quiescent live launcher/session ownership. Only the record identity itself may
outlive those owners. Owner/module addresses are opaque live-owner identifiers.

| Boundary | Retained observation |
|---|---|
| Existing handle issued, row allocation throws | Issued ID and failed gap remain; record metadata is explicitly missing, module absent. A later resize cannot invent that record identity. |
| Row allocated | Known record identity and stage are attached before cloning; subsequent exceptions mark the row failed. |
| Enter JIT factory | Stage becomes `CreatingModule` and module status becomes `Incomplete` before the call. Earlier dylib/IR side effects may occur before a wrapper can return or be stored. |
| Factory returns normally | Confirmed wrapper identity and `ModuleReturned` stage survive a later task-vector failure. |
| Native handle published | Stage becomes `Published`; existing registration and dispatch order are preserved. |
| Retry | Existing append-only ID issuance creates another row; failed rows and confirmed/uncertain associations remain. |
| Finalization | Compiled-data identities become expired while rows/modules remain with their existing owners. |

The integer-ID guard refuses exhaustion before overflowing the existing native
counter. That bound is inspected in source; it is not exercised by allocating
billions of rows. `Present` describes metadata coverage, never full resource
coverage. Missing row metadata, unavailable record identity or uncertain JIT
side effects leave counts `Incomplete`. Every `complete` flag remains false.
The JIT/ORC implementation is unchanged, so inner failures and resource-tracker
ownership are not claimed as solved by a returned-wrapper association.

## Focused executable evidence

`registration_attempt_test` uses the real CPU launcher, compiler, JIT/session and
dispatch. Test-only link wrappers and fixed C++ allocation observers inject:

- A row-resize allocation failure: ID 0 is issued without stored metadata; retry
  publishes ID 1. No JIT call occurs in the failed window.
- A task-vector allocation failure after the real module returns: ID 2 retains
  its confirmed module/record; retry shares the record and publishes a different
  module at ID 3.
- An exception before the real factory and an exception after its real return
  but before the CPU caller receives it: IDs 4/5 remain explicitly uncertain.
  The latter really retains a session wrapper; retry publishes ID 6 separately.
- A real unmanaged LLVM clone: ID 7 publishes with unavailable record identity,
  without manufacturing provenance.

The resulting census is eight issued/stored slots, seven recorded attempts,
one missing record, one unknown record, four failed attempts, four publications,
five confirmed wrappers and two uncertain module observations. The actual native
session contains six kernel wrappers plus its initial runtime wrapper. The
unreturned sixth kernel wrapper is deliberately not reported as a confirmed link.

The test checks zero-allocation queries/copy/move of metadata identity, unchanged
client-borrow counts, non-lazy fresh/unsupported/bounded queries, unchanged
survivor handles and prepared/dynamic output. All HCP definitions and arrays die
before explicit finalization. Identity tokens remain valid as expired metadata
across finalization and Runtime destruction; the selected control releases only
when the last escaped identity is dropped.

The existing registration fixture additionally checks native associations at
its original 16 phases. Across seven Clang/GCC/minimal histories, all original
112 snapshots and 14 fault windows match the accepted `cf26b62e` output exactly.
Its deletion witnesses still establish data-at-finalize and modules-after-finalize
ordering. New metadata assertions and storage lines are separate observations.

## Measured ownership cost

The same one-worker prepared registration history was instrumented before and
after on Clang 15 and GCC 11 wrappers using the matching Clang/LLVM 15 CPU SDK.
Before production is resolved `4747d113` / fork `71e58621`; the temporary
measurement-only fixture patch, commands, binary hashes and logs are retained.
After production is the tested source above. Both compiler profiles agree:

| Quantity | Before | After | Interpretation |
|---|---:|---:|---|
| CPU launcher object | 48 bytes | 56 bytes | +8 inline bytes for issued-ID count |
| One slot extent | 48 bytes | 80 bytes | +32 inline bytes per vector element |
| Two-slot capacity | 96 bytes | 160 bytes | +64 requested capacity bytes |
| Six live slots, capacity eight | 384 bytes | 640 bytes | +256 requested capacity bytes |
| Four selected record controls before finalize | 192 bytes | 192 bytes | Existing four 48-byte requests; no new control allocation |
| Those controls after finalize | 0 bytes | 192 bytes | Longer metadata lifetime, ending with rows/caller identities |
| Native base / concrete LLVM record | 40 / 168 bytes | 40 / 168 bytes | Unchanged by this slice |
| HCP wrapper / client guard | 48 / 16 bytes | 48 / 16 bytes | Unchanged by this slice |

A copied identity occupies 16 inline bytes. Attempt/count query results occupy 56/96 bytes and are caller objects, not another persistent registry.
Row metadata is already included in vector allocation; do not add its inline
extent again. The 48-byte controls are shared with existing borrow metadata and
must also be counted once. Extra references can extend their lifetime further,
as the escaped-identity test demonstrates. No whole-owner peak, allocation-growth
transient, executable/ORC bytes, allocator overhead, malloc coverage, RSS,
complete B, admission charge or net saving follows from this scoped measurement.

The CPU row/launcher layout and public native virtual interface require a
matching SDK/component/wrapper rebuild. Runtime bitcode remains pinned at
`4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada`;
serialized/AOT formats and numerical kernels are unchanged.

## Qualification and continuation

Local CPU checks at the clean tested source: quick CI; 23 Clang CTests;
17 GCC 11 wrapper CTests; and six standalone warm/viewport-OFF CTests.
The focused subset includes registration attempts/failures, unregistered removal,
borrow moves/unwind/expiration and architecture provenance, loaded/native owner
counts, reclamation/sharing, prepared launch/backing, owner backing and real
legacy/prepared submission histories at one-worker bulk/two-worker direct.
This qualifies the stated CPU scope, not hosted acceptance or GPU/concurrency.

Local evidence: `/tmp/hcp-attempts-635ee3ea-evidence.tar.gz`, SHA-256
`f8f396ee253419cc08a5d0782f2ed9b1ba79ad74c4e7f1176d1264a583217b8c`. It contains source/command/toolchain/build/binary pins, raw logs,
comparison audit, before cost probe and its binaries, development logs, and the
previous combined evidence archive with its manifest. The archive is local,
not published with GitHub source; hosted CI remains authoritative.

Return this implementation and measured cost for maintainer review. Reconcile
its dependency's and its own hosted gates without duplicate dispatch. Next work
must follow that review; registered retirement requires separately justified
ORC ownership/cleanup and failure safety. Row reuse, admission credit, complete B,
GPU/concurrent use and #324 remain separate. Held #75/#111/#123 are unchanged.
All merge events belong to the maintainer.
