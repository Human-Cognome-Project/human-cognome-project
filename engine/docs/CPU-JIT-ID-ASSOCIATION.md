# Native CPU JIT ID association and presence

Implementation for review under #343, following [#413 decision 7101856435](https://github.com/Human-Cognome-Project/human-cognome-project/pull/413#issuecomment-7101856435).
The existing CPU registration row now retains its issued native JIT ID. Presence
is queried through its live owner with the session's authoritative prepaid bound.
Cleanup, registered retirement, row reuse, admission credit and #324 remain separate.

## Source and integrated baseline

Tested source `2664a3bdc34a6d90a3a93ff7364841dae34495d3`, tree `5169bfe04cb898969ed943e18f0c41a94b79819d`, native fork subtree
`a94a9eb350c94dfa2eb586bee49c53e0ee886648`. Parent main is
`3c495db7a422e096766eb65fc1264cde0ce7ecbf`, whose November 23 handoff reconciles
qualified #410–#413 integrated parent-first through `daf6f74d`. Each merge has
its reviewed head's exact tree. No open contribution overlaps this work; held
#75/#111/#123 remain unchanged. All merge events remain maintainer-only.

The [attribution inspection](CPU-JIT-ATTRIBUTION.md) keeps its original
`b533ff93` evidence and 80→88-byte forecast. This implementation has separate
source-qualified evidence and actual native storage observations. Later handoff
changes are Markdown only.

## Publication and ownership

`Context::Registration::native_id` is one integer, initialized to `-1`, appended
to the existing CPU row. The existing attempt snapshot exposes that value. The
CPU launcher passes a synchronous reference through a new executor overload and
native session creation entry. No pointer/reference is retained by either owner;
no session/executor field, association registry or thread-local channel is added.

Under the existing CPU-session mutex, the checked issuance helper increments the
counter and writes the output before name formatting or LLVM creation. An
exception therefore leaves the row with either no issued ID or its reserved ID.
Existing failed-row/stage/record metadata remains intact. The old one-argument
factory remains for runtime/other callers; unsupported backends preserve old
creation with unavailable CPU identity. Matching test interposers now follow the
output-bearing overload so the accepted failure controls continue to intercept
actual CPU creation.

The real helper is checked at zero, its last supported ID and exhaustion. A
following injected exception preserves its published ID; exhausted output stays
unissued and the counter does not change. The integrated source ordering places
that publication before formatting. No formatting error is injected into fmt,
and billions of native attempts are not used to reach full-owner exhaustion.

No new lifetime owner is created. Compiled-record metadata can be safely observed
as expired after finalization/Runtime destruction, as before. Session/dylib
addresses remain opaque live-owner observations. The presence API takes the
current owner's row index, not a saved session address or escaped token. Queries
require the original live, serialized/quiescent Program/session; no backend query
is performed after Runtime destruction.

## Production query and states

`ProgramImpl::cpu_registration_jit_presence(index, entry_budget)` delegates only
to an already existing launcher. CPU lookup reads that row's ID and delegates to
the existing executor/session. It does not instantiate a launcher or session.
The session locks its own mutex, reads its counter, and uses the existing LLVM
name table. The caller supplies no issuance bound.

| Observation | Result | Native name lookup |
|---|---|---:|
| No launcher/session or no such CPU attempt | `status=Absent`, ID/dylib unavailable | 0 |
| Unsupported CPU observation | `status=Unavailable` | 0 |
| Existing attempt with no ID, including failed row storage | `status=Present`, `id_status=Absent`, ID `-1`, dylib unavailable | 0 |
| Nonnegative ID outside this session's issued range | `status=Absent`, `id_status=Absent` | 0 |
| Issued ID, insufficient budget | `status=Incomplete`, ID present, dylib unavailable, charge zero | 0 |
| Issued ID, no retained dylib | `status=Present`, ID present, dylib absent | 1 |
| Issued ID, retained dylib | `status=Present`, ID present, dylib present | 1 |

Every `complete` flag remains false. An absent dylib does not establish absence
of all backend effects or eligibility for reclamation. A retained dylib does not
establish wrapper publication, materialization, executable ownership or safe
independent unloading. Existing pre-return `module_status=Incomplete` is preserved.

The authoritative counter bounds the linear native name-table scan, including
initial-runtime IDs and consumed holes. Full lookup prepays that count before
fixed-buffer decimal formatting or lookup. A platform could create independent
dylibs and invalidate the bound; such a session returns unavailable. The current
CPU owner has no platform and creates at most one dylib per issued ID. This is a
conservative entry bound, not a measured vector-comparison count. After a consumed
name with no dylib, it may exceed retained dylib count; short work still refuses.

## Executable evidence and actual cost

The real owner API replaces #413's test-side presence model. Five cases run per
history: successful creation, no ID at factory refusal, issued ID before real
creation, retained empty dylib before generator installation, and exception after
real factory return. Retry uses a fresh CPU row and its own native ID; both the
failed association and its record identity remain unchanged. In the no-ID case,
retry first issues ID 4; after an issued failure, retry issues ID 5.

Seven Clang/GCC/minimal histories exercise 35 creation/shutdown windows and 154
production-presence phases, including 21 no-ID observations. Test wrappers count
zero native getter calls for short/unissued/absent-row controls, one for a fully
prepaid issued query and none for a no-ID query. All query phases observe zero
C++ new calls and no added client borrows. Fresh queries do not instantiate a
launcher; unsupported launchers keep unavailable status. A row-allocation gap
receives no fabricated ID. Record metadata expires while native presence remains
queryable through the live session after compiled-data finalization.

Survivor handles, task addresses, prepared/dynamic range outputs, existing thread
service, actual retry and ordinary native shutdown remain checked. Original
registration/attempt histories retain their logical counts and failure/dispatch
outcomes; storage/snapshot extent changes are separately reconciled.

| Storage | Integrated #413 | This implementation |
|---|---:|---:|
| Actual CPU slot extent | 80 bytes | 88 bytes |
| Actual vector capacity eight | 640 bytes | 704 bytes |
| CPU launcher object | 56 bytes | 56 bytes |
| Attempt snapshot, caller object | 56 bytes | 64 bytes |
| Presence snapshot, caller object | Not implemented | 56 bytes |
| Native base/concrete LLVM compiled data | 40/168 bytes | 40/168 bytes |
| HCP wrapper / client guard / record control request | 48/16/48 bytes | 48/16/48 bytes |

The extra integer occupies four value bytes plus padding, eight per vector
capacity entry. No new control allocation or production metadata reference is
introduced. Row storage is already included in vector allocation; do not add
its inline extent again. Retained failed dylibs/IR/wrappers are not removed.
These scoped quantities do not establish whole-owner peak, executable/ORC bytes,
RSS, complete B, admission credit or saving.

The launcher/session virtual interfaces, CPU row and attempt snapshot change:
rebuild matching SDK, component archives and native clients. Runtime bitcode stays
`4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada`;
serialized/AOT formats, numerical kernels and HCP production under `engine/src`
are unchanged. CPU evidence does not qualify GPU or concurrent use.

## Qualification and handoff

Clean-source local CI: quick, 24 Clang CTests, 18 GCC 11 wrapper CTests and seven
warm/viewport-OFF CTests passed against the rebuilt matching Clang/LLVM 15 CPU
SDK. The changed native interface/layout additionally requalifies owner/prepared
backing and real legacy/prepared submission at one-worker bulk/two-worker direct.
The original registration histories/112 snapshots, loan/architecture, removal,
loaded-count, prepared-launch and sharing controls remain source-qualified.

An initial quick pass at `be7f16fc` precedes a one-line fixture summary correction:
two creation retries and one no-ID refusal retry are now reported separately.
The native fork is identical. Final quick and all matrices use `2664a3bd`; the
initial pass and native-production equivalence proof remain in the bundle.

Archive `/tmp/hcp-id-association-2664a3bd-evidence.tar.gz`, SHA-256
`33371efe5bae896ad486e6f7d5f6623a972369ef501092d766f171af96e6f28b`, retains commands, source/tree/submodules, native/wrapper builds,
raw tests, toolchain/configuration, binary/runtime hashes, before/after audit,
development evidence and the manifest-verified integrated #413 comparison bundle.
It is local evidence, not published raw data. GitHub CI remains authoritative.
Return this implementation and measured cost for review before broader resource
work. Cleanup, registered retirement, admission credit, full accounting, GPU/
concurrency and #324 remain separate; #75/#111/#123 stay held.
