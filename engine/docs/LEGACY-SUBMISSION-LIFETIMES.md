# Legacy submission source-layout lifetime

Status: **provisionally accepted at `bed13aa7` in [#401 review 7064542786](https://github.com/Human-Cognome-Project/human-cognome-project/pull/401#issuecomment-7064542786)**, under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343). The maintainer authorizes the bounded existing-Execution owner/drain correction from this head while remaining hosted qualification finishes. #400 native/smoke passed; #401 passed smoke, compiler and database checks with regression pending at the runtime handoff. Main `d13e6e3c` integrates qualified #399. The original source-pinned inspection and zero-saving measurements below remain unchanged; the [runtime correction](SESSION-SOURCE-TREE-OWNERSHIP.md) records after evidence separately. Every merge remains maintainer work.

Canonical tested source `a0ec440bb9a27a8e04fd8acba1851faf85e923bb`, tree `d0214e7075d60fc6582b922c1f655d9d7af32447`, adds one native inspection executable and four Linux GNU/Clang CTests. Production, fork, numerical kernels, public operations and achieved runtime saving are unchanged. The witness intentionally records the existing retention boundary for a separately reviewed correction.

## Actual owner and failure boundary

Legacy [`ActiveSession::submit`](../src/assembly/active_session.cpp) builds the v1 root/optional primary-step source layout, validates the bound input, and calls `Program::add_snode_tree`. It discards the returned handle. The legacy Execution constructor stores only an ActiveAssembly value; its Runtime and layout members stay null. Consequently its destructor never calls native destruction for that source tree.

[`FixedStudy`](../src/assembly/study.cpp) constructs its full field Harness before staging supplied state and parameters. [`Harness::publish`](../src/field/field.cpp) rejects negative/nonfinite parameters after that constructor has completed. On this real exception path, the failed study/Harness unwinds and #400's Activity owner retires its registered root. The source tree remains Program-owned without an Execution retirement obligation. No visible execution is published; a corrected retry creates another source tree rather than reusing the retained one.

Normal teardown has the same gap: ActiveSession synchronizes and clears all Executions before destroying Runtime, but each successful legacy source tree is still live at that point. This differs from the prepared entry, which retains its source-tree handle and destroys its prepared owner/Harness before retiring the source tree. The v1 supplied-start/fixed-count semantics remain distinct from prepared C3 and operational Add.

## Native witness and observed costs

[`legacy_submission_inspection`](../tests/legacy_submission_inspection.cpp) calls the real public session API using a warmed aggregate sibling and a root-plus-primary-step candidate. Linker-only observations capture real Runtime construction/destruction, native registration/destruction and compilation results. There is no alternate Runtime, production inspection API, private access, injected failure, cache clearing or new lifecycle authority.

Each run performs unavailable-input and duplicate-study early refusals; owner/generation rejection; negative and nonfinite parameter failures of the same candidate; corrected retry; bitwise preservation of sibling particle/group/determined reports and completed count; and following ticks on both executions. Early refusals and completed observations provision no new context. A copied report remains usable after session teardown.

The wrapper captures allocation records by value while each tree is live, distinguishes source layouts from Activity's two pointer/bitmasked sets, and later queries copied tokens through the independently live Device. Its Runtime-destructor entry observes the boundary after every Execution is gone and before native finalization. It performs no query through a destroyed Runtime/device or freed tree pointer.

| Completed boundary | Live source roots / requested bytes | Live Activity roots / requested bytes | Observed compile calls / unique compiled objects | Native tree slots / free IDs |
| --- | ---: | ---: | ---: | ---: |
| Warm sibling | 1 / 4,096 | 1 / 4,096 | 13 / 13 | 2 / 0 |
| Negative-parameter refusal | 2 / 8,192 | 1 / 4,096 | 26 / 26 | 4 / 1 |
| Nonfinite-parameter refusal | 3 / 12,288 | 1 / 4,096 | 39 / 39 | 5 / 1 |
| Corrected retry and following ticks | 4 / 16,384 | 2 / 8,192 | 52 / 52 | 6 / 0 |
| Executions gone, before Runtime finalization | 4 / 16,384 | 0 / 0 | 52 / 52 | 6 / 2 |

Each late failure retains one actual 4,096-byte source-root request and retires one Activity root. The two failed attempts therefore leave 8,192 extra requested root bytes during the healthy retry. All four source tokens, including the two successful studies, remain live immediately before Runtime finalization. All four Activity tokens are retired. Released Activity tree IDs are reused for later source layouts while the old allocation tokens remain retired; this is native descriptor-slot reuse, not proof that a retained source allocation was reclaimed. Issued SNode IDs reach 39 even though tree slots reach only six.

Program's own Kernel vector is empty throughout; 52 observed compiled-data objects remain readable through their existing compilation-manager ownership until Program teardown. Each failed Harness contributes thirteen more observed objects. These counts do not estimate compiled bytes, JIT/module closure or a safe eviction mechanism.

The raw samples separately retain runtime cumulative requests and process-wide pool requested/reserved observations. Root requests, pool observations and compiled objects are overlapping or differently scoped quantities and are not summed. Root retirement would not imply complete pool/list or compiler release. This inspection does not enumerate a disjoint borrowed-resource ledger, enforce complete B, or quantify RSS. Runtime saving is **zero**.

## Smallest proposed follow-up

Retain the returned source-tree handle in the existing Execution on both legacy and prepared submission paths. Reuse the accepted `SNodeTreeOwner`/`Drain` mechanism instead of adding a registry or lifecycle framework: initialize the Program pointer before registration, capture the returned handle immediately, drain while dependencies still exist, destroy FixedStudy/prepared-owner/Harness/native contexts, then retire the source tree while Runtime/Program is alive. An owner-first/drain-last Execution layout can cover normal destruction and member unwind. Replace the current manual teardown body with that ordered member destruction, or explicitly drain before any manual resets; keeping those resets ahead of a member drain would put the barrier too late.

Merely setting the legacy `layout` pointer is insufficient: today's Execution destructor retires that pointer in its body, whereas the legacy `study` member otherwise dies afterwards. The follow-up must make the dependency order explicit and preserve prepared owner/Harness ordering, native authority and descriptor/backing distinctions. Native registration's own exception handling remains separate from the retirement obligation taken after a successful return.

Update this witness in that runtime change to require failed and successful source tokens retired at their intended boundary. Add a partial-constructor failure before Harness publication, preserve both layout forms, warmed sibling reports/ticks, real parameter failures and corrected retry, and qualify the affected legacy/prepared paths on both compiler profiles. Measure actual root/host/pool changes against this source; do not relabel these before measurements as a saving. Compiler/JIT eviction, wider adoption/revision policy, full B, GPU and #324 mathematics stay separate. This document proposes the correction; it does not implement or approve it.

## Contributor local qualification

All qualification uses the clean canonical source/tree above, the exact fork tree `8b31651be07d8bad0b2a6a261369e8690b9c3b53`, matching pinned submodules and runtime bitcode SHA-256 `4a57d249a308b5a4523e788025515e7adbccce7565ecafe2871cc34094ca7ada`. Actual execution is LLVM/x64 CPU, Release, warm/viewport enabled, CUDA qualification disabled.

- `bash engine/scripts/run_local_ci.sh quick /tmp/hcp-legacy-layout-a0ec440b-quick`: exit zero; matching fork/wrapper build, backend check, three CTests, 632 independent placement-math samples and placement-capacity case pass.
- `bash /tmp/hcp-legacy-layout-checks.sh a0ec440bb9a27a8e04fd8acba1851faf85e923bb clang`: all four one/two-worker bulk/direct CTests pass with Clang/LLVM 15.
- The same command with `gcc`: one-worker bulk and two-worker direct pass with GCC 11 against the same Clang-built native substrate. This is the recorded GCC subset, not all four modes or hosted parity.
- `perl /tmp/hcp-legacy-layout-audit.pl /tmp/hcp-legacy-layout-a0ec440b`: six runs, 48 captured roots, 66 phase samples and twelve real parameter exceptions; structural/root/compilation counters agree across all six profiles. Each run observes four live source tokens and four retired Activity tokens before finalization. Separate raw pool counts remain path/worker dependent.

Verified machine-local archive: `/tmp/hcp-legacy-layout-a0ec440b-evidence.tar.gz`, SHA-256 `ac8fea42290e1a895a4d3c8b8db66bb82ad30ebe4cfad5c63809d41bdfe83c59`. It retains all three final profile directories, exact commands/configurations/toolchains/submodules, source/tree and clean before/after checks, unchanged-runtime and accepted-baseline equivalence patches, binary/runtime hashes, complete native observations, cross-profile audit, development build/run logs and archive scripts. Fresh extraction verifies the root and all three profile manifests. The hash identifies contributor-retained local evidence; it does not publish the raw bundle.

The initial development compile used an increment for a UUID generation and failed; its log is retained, the check was corrected to a different UUID, and the final clean qualification was run independently. Native shutdown's existing cache-lock warnings are retained. No cache, retained store, credential or Git history was cleared. Accepted broader native/compiler suites were not repeated for this test/build-only inspection; GPU, database execution, complete B and numerical #324 qualification are not claimed. Hosted CI remains authoritative and pending for this contribution; #400's remaining native gate and both integrations remain separate maintainer work.
