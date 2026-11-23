# Private prepared-source layout construction

Implementation for review under #343, following [#417 decision 7108110435](https://github.com/Human-Cognome-Project/human-cognome-project/pull/417#issuecomment-7108110435).
`make_prepared_source_layout` privately constructs the existing unregistered
root/optional-primary/i32-place graph and fills its validated binding pointers.
The session still registers that graph in the same candidate guard before
constructing its Harness. The supported shapes and ownership contract are unchanged.

## Source and reviewed dependency

Tested source `6844897534bf2621bfdcedc621c0c17e52ce0588`, tree
`989211918761f4160323d809db23b62c7b659dac`, changes only
`engine/src/assembly/active_session.cpp` against reviewed #417
`0d1fd0a287eeb1bd37c439b498fb8058af29b3fb`. Native fork subtree remains
`a94a9eb350c94dfa2eb586bee49c53e0ee886648`; test and CMake sources are unchanged.
At qualification start, main `fabf803f` integrates qualified #415 with its exact
reviewed tree. #416 passed both hosted gates but remained unmerged; source-accepted
#417 had smoke passed and native qualification running. This branch explicitly
pins #417's reviewed head. Current disposition is maintained in
[CURRENT-STATE](../../docs/onboarding/CURRENT-STATE.md); integration remains
maintainer-only. Later report/handoff changes are Markdown only.

## Extracted responsibility

The helper has internal linkage in the anonymous namespace of
[active_session.cpp](../src/assembly/active_session.cpp). It takes only
`detail::AdmissionBindings &`, under the existing successful-preflight precondition.
It takes no Program, Runtime, staging map, Harness or candidate owner.

The moved statements are unchanged: create the root, bind index zero, optionally
attach `dense(Axis(0),1)` and bind index one, then append the terminal `i32` place.
Roles, parents and units are still explicit values validated by preflight; the
helper does not infer model semantics or broaden the two supported layouts.
It returns the existing `unique_ptr<SNode>` ownership, with no new persistent
owner, wrapper, registry or allocation beyond the graph already constructed here.

The helper-local raw `parent` pointer disappears on return, but its graph does
not. Returning/moving `unique_ptr` transfers ownership without moving SNode
pointees; the pointers stored in the by-value request still identify that same
root/child graph. The caller's reusable template is not patched. Unique-pointer
return introduces no additional native registration or heap-copy operation.

`submit_prepared` still performs all source/resource, owner, capability, duplicate
and private preflight checks before staging. It creates the same candidate and
authorization, then receives the unregistered root. The existing
`e.layout.node = runtime_->program().add_snode_tree(std::move(root),false)` line
remains immediately afterward, followed by unchanged Harness construction,
prepared-owner preparation, bootstrap and map transfer.

On graph-construction failure, ordinary unique-pointer unwind releases the C++
graph before the staged candidate dies; its retirement handle is still null.
After successful Program registration, the returned handle is assigned to the
same existing guard. Drain/dependent/source ordering and unpublished-candidate
cleanup are unchanged. No new catch, retry, result conversion or native rollback
is added. Complete backend cleanup for failures inside materialization remains
outside the qualification, as in the accepted [inspection](PREPARED-SOURCE-LAYOUT.md).

There is no public signature, class layout, source pin, CMake dependency, compiler/
JIT interface, numerical kernel, resource gate or cleanup policy change. Legacy
layout construction is untouched. This is the approved private preparation split,
not a general assembly builder or an alternative lifecycle authority.

## Equivalence and qualification

Source reconstruction extracts the parent graph/binding block and compares it
exactly to the helper body. Removing the helper and restoring the block at the
call reconstructs the entire parent translation unit byte for byte. Registration,
Harness construction and all code outside the extraction are therefore unchanged
in source. This proof accompanies execution; it is not a new test-side builder.

Clean-source local quick CI, **eight Clang CTests and six GCC 11 wrapper CTests**
pass against the matching Clang/LLVM 15 CPU SDK. Clang active submission covers
one/two workers × bulk/direct; GCC uses one-worker bulk/two-worker direct. Both
compilers run real prepared/legacy refusal/retry/sibling controls on one-worker
bulk and two-worker direct. Existing root-only/primary source shapes, detached
report pointer checks, combined bootstrap, claims, source pin lifetime and ticks
remain exercised. No assertion or test implementation is replaced.

Comparison against the verified pre-helper #416 bundle preserves active-submission
results and the original compiler/cache/borrow/failure/dispatch histories exactly:
**132 phase samples and 24,024 individual row-to-dylib observations**. Native root
registrations and observed backing/context counts also match after excluding
compilation-duration fields. Retirement identities/capacities/multiplicities match
per test and phase. Independent final Execution order is compared separately;
all earlier ordered retirement records remain exact. Constructor-fault cleanup and following retry/
sibling dispatch retain their prior outcomes. These comparisons observe actual
owner paths, not a separately constructed candidate layout.

The first audit required identical final retirement order and failed. The
session's `std::map<Uuid,...>` clears independent studies in the order of their
random UUID keys, so final teardown sequences differ between runs. The comparison
now preserves the four final prepared / three final legacy retirement records
per test without imposing cross-Execution order. Existing executable checks still
require drain and dependency cleanup before source retirement. The initial failed
comparison, raw ordered logs and diagnosis are retained; no native test or source
change was needed for this audit correction.

The wrapper archive is rebuilt. Native SDK code is unchanged and runtime bitcode
stays `4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada`.
CPU evidence does not qualify GPU or concurrent use. The existing owner fixtures
require warm integration/viewport; no minimal-profile owner run is claimed.

Archive `/tmp/hcp-source-layout-68448975-evidence.tar.gz`, SHA-256 `ed756678c5951558f79c174763cdb256bdcec93b7d4f47167d4fe4a221596f9f`,
retains clean source/tree/submodules, commands, exact reconstruction proof,
configuration/build/binary pins, raw tests, development build evidence, native
lifetime comparisons and the manifest-verified #416 comparison bundle. Fresh
extraction verifies manifests and reused raw-log equality. This machine-local
path does not publish raw evidence remotely.

Return this helper extraction for maintainer review and authoritative GitHub CI.
Parent qualification/integration remains separately recorded in the current
handoff. Broader ownership changes, native rollback, complete accounting, cleanup,
admission credit and #324 remain separate; #75/#111/#123 remain held. No further
implementation scope follows from the local pass.
