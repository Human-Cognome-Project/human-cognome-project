# Private prepared-request preflight

Implementation for review under #343, following [#415 decision 7106148574](https://github.com/Human-Cognome-Project/human-cognome-project/pull/415#issuecomment-7106148574).
`ActiveSession::submit_prepared` delegates its binding, layout, frame and scratch
checks to one private helper in the same translation unit. The accepted request,
refusal and publication behaviour is preserved.

## Source and reviewed dependency

Tested source `9c3f03a899cb7c3670cdc6ba3bb4fdbbd7526bc2`, tree
`e2d8cc44768719d773a29d7e23283e1bb9de5fdc`, changes only
`engine/src/assembly/active_session.cpp` against reviewed #415 `8101a1ed`.
The native fork subtree remains `a94a9eb350c94dfa2eb586bee49c53e0ee886648`.
#415 is provisionally source-accepted at `8101a1ed`, open with smoke passed and native hosted qualification pending; main is `d78c6d86`. This branch explicitly pins that reviewed dependency. All integration remains maintainer-only.
Later report/handoff changes are Markdown only.

## Responsibility and preserved order

`preflight_prepared_request` has internal linkage in the anonymous namespace of
[active_session.cpp](../src/assembly/active_session.cpp). It borrows the existing
request and recipe by const reference, and writes the existing result's refusal
fields. It has no independent owner, registry, exported API or native allocation.
The caller establishes its non-null source and authorization/capability/duplicate
preconditions before invoking it.

| Position | Existing checks and outcomes |
|---|---|
| Session, before helper | Missing source/resource domain returns the default result; wrong owner throws. Unsupported recipe/input/local/profile/backend returns `Unsupported`. Duplicate study returns the existing default result. |
| Helper: binding | Exact owner, study, generation, recipe selection/revision and nonempty focus; failure retains default `InvalidInput`. |
| Helper: layout | Recipe shape and the root/optional primary-step subset, null native pointers, parents, roles and units; failure sets `Unsupported`. |
| Helper: frame | Every anchor coordinate must equal zero, with the same floating comparison; failure sets `Unsupported`. |
| Helper: scratch | Existing `PreparedStorage::required_scratch(limits,false,profile)` calculation and `ScratchBytes` comparison; failure sets `CapacityExhausted` and the identical budget/demand/limit tuple. |
| Session, after success | The existing clock, staging map, Execution/tree/Harness creation, prepared-owner preparation, bootstrap and map-node transfer run unchanged. |

Every former early `return out` inside the moved block becomes `return false`;
the caller returns that same `out`. Successful preflight returns true and does
not set a success status prematurely. Exceptions from existing calculations are
not intercepted or converted. The request remains passed by value at the public
boundary, and its later internal layout-pointer writes are unchanged.

Source proof extracts the original block from `8101a1ed`, adapts only these return
statements and compares it exactly to the helper body. Removing the helper and
restoring the original block at its call reconstructs the entire original
translation unit byte for byte. This establishes the mechanical scope, check
order, status assignments and demand arithmetic; executable evidence below checks
the existing operation paths.

No header, CMake target, public signature, class layout, source pin lifetime,
prepared-owner/bootstrap logic, native compiler/JIT API or numerical kernel
changes. The helper stays within the existing native assembly target, adding no
dependency to standalone projection. No runtime saving, new resource gate,
retirement, admission credit or complete B is claimed. The prior
[refusal/JIT disposition](REFUSED-SUBMISSION-JIT-TRACE.md) retains its source and
separately measured native retention.

## Qualification and handoff

Clean-source quick CI, eight Clang CTests and six GCC 11 wrapper CTests passed
against the matching Clang/LLVM 15 CPU SDK. Four Clang active-submission cases
cover one/two workers × bulk/direct; GCC covers one-worker bulk/two-worker direct.
The existing active fixture checks wrong-owner/unsupported-pointer and scratch
refusal, late placement refusal, exact combined bootstrap, retry, legacy sibling
preservation, reports, retained source and ordinary teardown. No replacement test
implementation or weaker assertion is introduced.

Both compilers also run real prepared/legacy submission at one-worker bulk and
two-worker direct. Original compiler/cache/borrow/refusal/dispatch histories,
**132 phase samples and 24,024 individual CPU row-to-dylib observations** compare
exactly against the manifest-verified #415 evidence. Early scratch refusal still
creates no compiler/registration context; late refusal still retains the same
registered resources while roots retire. Successful retry and surviving dispatch
retain their original counts, identities and outputs through finalization.

Runtime bitcode remains
`4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada`.
These CPU results do not qualify GPU or concurrent use. The existing owner fixtures
require warm integration and viewport; a minimal profile cannot run them. Native
SDK code and all test sources are unchanged from the reviewed dependency.

Archive `/tmp/hcp-preflight-9c3f03a8-evidence.tar.gz`, SHA-256 `9851eca64d54527cf25967fc5cb5835546ede6fdf0c34c53595cc4dd202df281`,
retains commands, clean source/tree/submodules, matching configuration/build and
binary hashes, exact reconstruction proof, raw tests, development build evidence
and the verified #415 comparison archive. Fresh extraction verifies manifests
and matching reused raw logs. It remains machine-local evidence, not remotely
published raw data. The maintainer's #415 review inspected source/committed trace;
its local histories were contributor-reported.

Return this extraction for maintainer review. New authoritative GitHub CI and
parent #415 qualification/integration remain separate. Broader ownership changes,
complete native accounting, cleanup, admission credit and #324 remain separate;
#75/#111/#123 stay held. No further implementation scope is inferred from this pass.
