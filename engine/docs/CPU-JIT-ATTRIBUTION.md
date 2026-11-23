# Fresh-ID attribution at the native CPU JIT boundary

November 23 disposition: qualified #410–#413 are integrated through `daf6f74d`;
main `3c495db7` records that baseline. [#413 decision 7101856435](https://github.com/Human-Cognome-Project/human-cognome-project/pull/413#issuecomment-7101856435)
authorizes the separate [native ID association implementation](CPU-JIT-ID-ASSOCIATION.md)
at tested `2664a3bd`, now awaiting implementation review and authoritative GitHub
CI. Its actual 88-byte row is separate from this inspection's original 80-byte
baseline and 88-byte forecast. The original source, observations and historical
handoff below are preserved.

Inspection/proposal under #343, following [#412 review 7084634921](https://github.com/Human-Cognome-Project/human-cognome-project/pull/412#issuecomment-7084634921).
The fresh native ID can distinguish a consumed name with no session-retained
dylib from one with a retained dylib, provided the native owner supplies issuance
and lifetime provenance. The current CPU caller API does not expose that ID on
failure. This contribution measures a minimal association proposal; production
association, cleanup, retirement and admission behavior remain unimplemented.

## Source and confirmed state

Tested source `b533ff939ee18e94a4a7e2b7f4f2a042cce182b8`, tree
`8ba082c1ca2048fd286b8a1243f471c99e889841`, fork subtree
`9c9e42288b502447a39f4c3c0c8205ebeb45837e`. Parent #412 is
`371701762c9c0dbd819f67552a04b52f8de770dc`. Only the existing CPU JIT fixture,
a new test-only probe/layout header and its CMake link-wrapper registration
change. All native SDK/HCP production is exactly #412; later handoff files are
Markdown only.

Main `6441a740a23b34b8504b719345894177db6da952` integrates #408 with resolved
`4747d113`'s exact tree `5a9ad101`. #410 targets main; #410 → #411 → #412 remain
open for native qualification and maintainer integration. #412 is provisionally
accepted at `37170176`; #411's decision is now [recorded at 7084632898](https://github.com/Human-Cognome-Project/human-cognome-project/pull/411#issuecomment-7084632898).
#410 has both hosted gates passed; #411/#412 smoke passed and native gates remain
pending at inspection. A chat
acceptance is review notation; these refs/dispositions were checked directly.
All merge events remain maintainer-only.

The original [naming implementation](CPU-JIT-NAME-ISSUANCE.md),
[JIT creation inspection](CPU-JIT-CREATION-OWNERSHIP.md) and
[CPU attempt metadata](NATIVE-REGISTRATION-ATTEMPTS.md) retain their original
source/evidence and costs. This is separate attribution evidence.

## What existing native state establishes

The fixture captures the actual decimal ID at the existing
`ExecutionSession::createJITDylib` call boundary. This occurs before the
pre-creation refusal, so ID 4 is witnessed even when the real call is skipped.
The existing LLVM `getJITDylibByName` then reports current session retention:

| Original CPU attempt 3 / native ID 4 | Native wrapper count before retry | Dylib presence before/after retry and after finalization |
|---|---:|---|
| Failure before real creation | 4 | Absent throughout; ID 4 was consumed |
| Failure after creation, before generator installation | 4 | Same empty dylib remains present throughout |
| Exception after real factory return | 5 | Same dylib remains present; wrapper and lookup exposure are also retained |

All three failed CPU entries still report `CreatingModule`, no confirmed module
and `module_status=Incomplete`. Neither the CPU snapshot nor equal wrapper counts
separate the first two cases. Native presence lookup with known ID does. Retry
uses CPU slot 4/native ID 5, retains the same compiled-record identity and adds a
different dylib; it does not replace ID 4. These are distinct ID domains: the
runtime module and CPU row failures prevent deriving a native ID from a CPU slot.

A null native lookup alone cannot establish issuance: future name 5 is absent
before retry just as consumed name 4 is absent in the pre-creation case. The
session's existing issuance counter can distinguish issued range from future
IDs, but it is private and no failure output currently reaches the CPU caller.
The witness's captured-call bound is valid only for these exercised paths. A
formatting failure before the intercepted call would not expose its issued ID
there. Production must publish the ID before formatting rather than reconstruct
it from captured names, module count or the CPU slot.

Dylib presence is not an IR, generator, tracker, emitted-code, lookup-exposure or
complete-resource census. Absence describes the session's name table at the
observation time, not all transient work or resources outside that table.
Interior LLVM errors remain unexercised. Even a known retained dylib is not a
proved independent cleanup unit; shared runtime/service/dependency and stale
function-pointer exclusions remain.

## Lookup work and lifetime

LLVM 15.0.7's [execution-session implementation](https://github.com/llvm/llvm-project/blob/llvmorg-15.0.7/llvm/lib/ExecutionEngine/Orc/Core.cpp)
searches the dylib vector under its session lock. A query by one name therefore
has input-sized work. It does not materialize a function or transfer ownership.

The test-only proposal model prepays the issued-ID count as a conservative upper
bound on that search. In the current CPU owner, its private creation path is the
only dylib creator, names are unique, and no platform is installed. Consequently
retained dylibs cannot exceed issued IDs. The fixture checks the no-platform
condition and captured creation count. A production query would read the real
counter under `mut_`; the fixture-bound argument is not an existing trusted API.
Other creation paths/platforms would invalidate this bound and need new coverage.

Insufficient budget returns incomplete before formatting or native lookup.
Unissued IDs and unavailable owners likewise perform no lookup. A test link
wrapper counts the actual native getter calls: zero for those controls, exactly
one for a fully prepaid presence query. Decimal formatting uses a fixed buffer.
All 119 probe phases observe zero C++ new calls and add no tracked client borrows.
This demonstrates the model and existing getter, not a new production query API
or a measured number of vector comparisons.

The bound is conservative: after a no-dylib failure and successful retry, six
IDs have been issued but five dylibs remain. A budget of five refuses the model
query. Wrapper count alone cannot safely prepay lookup when an unwrapped dylib
also exists, as the post-creation failure demonstrates.

Probes run only under the original live, quiescent Program/session. They repeat
after compiled-data finalization while the backend remains live. The record's
existing invalidatable metadata is safely expired after Runtime destruction;
no backend query is performed then. A raw session address is not an epoch or
lifetime guard and can be reused. It must not validate an escaped token against
a later Runtime. A production association should be read through the current
owner's row, with the existing live-owner preconditions; only record metadata
has the demonstrated post-Runtime observation guarantee.

## Compiled storage models

The test header contains explicit layout surrogates using the native member
types. The baseline 80-byte slot is first reconciled with the real native
one-slot capacity export. Candidate sizes are compiled on both wrapper profiles;
they are forecasts, not allocated production associations:

| Candidate row representation | Slot extent | At actual capacity eight | Increment over native 640 bytes |
|---|---:|---:|---:|
| Current native row | 80 | 640 | 0 |
| Add one `int` native ID, with unavailable sentinel | 88 | 704 | 64 |
| Add ID, dylib pointer and status with member reordering | 96 | 768 | 128 |
| Append that richer link without reordering existing fields | 104 | 832 | 192 |

The minimum ID adds four value bytes plus padding, eight bytes per capacity
entry. It reuses the already owned compiled-record identity and derives session
scope through the existing launcher/executor binding. It needs no second registry,
new session field or separate lifetime/control allocation for owner-local reads.
A richer pointer/status link can avoid some lookup but still cannot resolve all
failures before a dylib pointer returns. All new layout/API costs require matching
native/client rebuilds if implemented.

The fixture's two 40-byte inline tokens hold extra references to existing record
metadata; they add no client borrow or new control block, but can extend that
control's lifetime until the trace dies. The 40-byte presence result is a caller
object. Those observer costs are not proposed native row storage. Do not add
nested row extents to vector capacity a second time. No measured production
saving, whole-owner peak, IR/ORC/executable byte count, RSS or complete B follows.

## Smallest production proposal for review

1. Add only an unavailable-initialized native creation ID to the existing CPU
   registration row. Forward a synchronous, non-retained output slot through the
   existing native executor/session boundary. CPU creation writes the issued ID
   before formatting. Initial-runtime and unsupported-backend paths remain
   explicit; no session-to-launcher dependency or thread-local execution channel
   is introduced. Exact interface spelling remains for implementation review.
2. Expose an owner-local, non-lazy CPU presence query. Use the existing counter
   as issuance provenance and prepaid lookup bound, and the existing LLVM name
   table for retention. Query only a row belonging to the current live owner;
   copied addresses/record metadata do not authorize later backend access.
3. Keep no-ID, unissued, insufficient-work, issued-without-retained-dylib and
   retained-dylib observations distinct. Keep full-resource completeness false
   and the existing pre-return module status incomplete. Add no cleanup, retirement,
   row reuse or admission credit.

Acceptance would verify actual row cost, source ordering of pre-format ID publication,
all refusal/unknown states and work/allocation controls, presence through
failure/retry/finalization, owner-lifetime exclusions and unchanged successful
module/dispatch/shutdown behavior. This proposal is not implemented here.

## Qualification and evidence

At the clean tested source, quick local CI, 18 Clang CTests, 12 GCC 11 wrapper
CTests and seven warm/viewport-OFF CTests pass against the unchanged matching
Clang/LLVM 15 CPU SDK. Seven naming histories preserve all 28 creation/shutdown
windows, fourteen inner retries, seven outer retries and seven boundary checks.
All original registration/attempt/naming output matches #412 evidence exactly.
The new evidence contains 119 presence probe phases (21 consumed-no-dylib and
49 post-finalization phases), seven compiled layout comparisons, budget/owner/
unissued controls and unchanged survivor dispatch. Production and fork `9c9e4228`
are unchanged; runtime bitcode remains `4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada`.
No database, GPU, full compiler matrix or concurrent-use qualification is claimed.

Archive `/tmp/hcp-attribution-b533ff93-evidence.tar.gz`, SHA-256 `660bd5afcc00598d667fbe3fad666f6071b6b8444073fa9d72e32007ebc1d9a2`,
retains source/submodules, commands/toolchains, build/test and native-call counts,
binary/runtime hashes, unchanged-production proof, before/after audit, development
compile failure/repair and the manifest-verified #412 comparison archive. Raw
archive evidence remains machine-local; GitHub CI is authoritative. This branch
stays on reviewed #412 until maintainer integration. Return the attribution and
cost proposal for review; held #75/#111/#123 and separate #324 remain unchanged.
