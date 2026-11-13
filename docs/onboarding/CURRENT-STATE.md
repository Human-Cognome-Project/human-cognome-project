# Current implementation and work

**Intent domain follow-up (awaiting review, 2026-11-13):** follows #326 review
5895998111 from main bf363f92. Reference comparisons now reject nonfinite operands
and assert angled coupling intent. Exact-coincidence alternatives, negative-contact
normalizer failure, and changing local/inherited intent are independently compared.
Recommend explicit unresolved coincidence for bounded diagnostics; no production law
or brake is selected. Native diagnostics, wake and many-tick dt/Q evidence remain next.
See the [intent proposal](../../engine/docs/INTENT-DERIVATION.md). Storage work and
Taichi-reuse follow-up remain separate.

**Maintainer acceptance — #326, 2026-11-13:** comparison proposal and independent algebra reference accepted in review 5895998111 and merged at `f08ad43a476a1c09abd925b6fab884db682bfec8`. Hosted smoke and all 12 native jobs passed; the new standalone reference is outside that native filter. Reviewer independently compiled and ran it with GCC 14.2.0/C++17 and strict warnings: PASS. This accepts the analysis, not a production intent/braking law. #324 remains open: resolve exact coincidence/domain/contact choices and develop bounded native diagnostics and trajectory/activity fixtures before selecting the correction. Coefficient weighting remains a candidate. Next reference revision should reject nonfinite comparisons explicitly and check the documented angled coupling target. Storage work #322/#323/#325 and the later Taichi-reuse audit remain separate.

**Intent correction analysis (#324, comparison accepted in #326, 2026-11-13):** accepted scope
6859051253 authorizes the [comparison proposal](../../engine/docs/INTENT-DERIVATION.md)
and independent algebra fixtures before selecting a kernel correction. The proposal
compares three target normalizations and constructs equal-force/different-intent under
the leading coefficient-weighted candidate. Exact source coincidence remains unresolved;
no braking law or production dynamics changed. Native trajectory/activity/quantization
qualification remains pending. #325 storage work and the later Taichi-reuse audit stay
separate; preserve main's composition orientation at647c6484.

**Maintainer integration — 2026-11-13:** #321 is accepted and integrated by this merge, preserving reviewed `8d77ff2e81d0e72f23ff7d3b73a5ee90064cb055`. Native run 43446605556 passed all 12 jobs and smoke 43446605870 passed. Personally inspected regression logs show 119 passes (30/30/30/29), with database addressed_reads=24/private_reads=0. Main is qualified through #321. #322–#323 have bounded source acceptance and passing smoke; native qualification remains pending. #323 review 5894203678 accepts fixed-work SNodeTree/SNode host inspection with strings/gradient/nested coverage incomplete. Next: remaining nested ownership and mixed ABI/provenance/string coverage before trusted lifetime/required-family aggregation, canonical/domain deduplication and prepaid work. Compiler exclusions remain conditional; full B and combined admission remain unaccepted. Held #75/#111/#123 remain unchanged. Earlier status paragraphs below are historical where they conflict.

**Follow-up direction — 2026-11-13:** Finish the current work, then review earlier implementation for functions already supplied by Taichi. Reuse existing optimized native compilation, dispatch, ownership and thread facilities. That later review should identify concrete duplication or unnecessary integration paths; do not divert the current bounded work into speculative engine replacement or optimization. Ownership clarity and avoiding duplicate work matter to performance; measured speedups remain separate evidence.

**Maintainer integration — 2026-11-13:** #319–#320 are accepted and integrated by this merge, preserving reviewed `4c7b7beeda7d3e39fe8ab07ed019f7d3b3e2047b`. Native runs 43439246063 and 43442561844 passed all 12 jobs each; both smoke workflows passed. Personally inspected #320 regression logs show 119 passes (30/30/30/29), with database addressed_reads=24/private_reads=0. Main is qualified through #320. #321–#322 have bounded source acceptance and passing smoke; native qualification remains pending. #322 review 5893720324 accepts Ndarray host object/shape-capacity inspection, separate from backing identity and with debug/shared-type coverage incomplete. Next: remaining nested owners and mixed ABI/provenance/string coverage before trusted required-family/lifetime aggregation, canonical/domain deduplication and prepaid work. Reuse Taichi compilation, dispatch and thread machinery. Compiler exclusions remain conditional; full B and combined admission remain unaccepted. Held #75/#111/#123 remain unchanged. Earlier status paragraphs below are historical where they conflict.

**Field/accessor map requests (awaiting review, 2026-11-13):** follows #320
review5892748763 and integrates qualified main dc7705eb through #318. Existing native
field-expression/accessor maps now reuse shared request counters; Program forwards the
bank's same domain report. All five direct map reports govern map availability, while
pointee/full coverage remains false. Tree retirement removes accessor rows but retains
field rows/kernel objects; no new cleanup policy. Next: nested ownership and mixed ABI/
provenance/string coverage before trusted aggregation. Compiler exclusions remain
conditional. #319–#320 await integration; held #75/#111/#123 unchanged.

**Maintainer integration — 2026-11-13:** #318 is accepted and integrated by this merge, preserving reviewed `7b1242be177283c39b9b5ea38a4590b34804f79d`. Native run 43432471080 passed all 12 jobs and smoke 43432471126 passed. Personally inspected regression logs show 119 passes (30/30/30/29), with database addressed_reads=24/private_reads=0. Main is qualified through #318. #319–#320 have bounded source acceptance and passing smoke; native qualification remains pending. #320 review 5892748763 accepts native free-ID deque request inspection and its scoped allocation/failure controls, retaining native LIFO behavior. Next: existing field-expression/accessor maps, then nested ownership and mixed ABI/provenance/string coverage before trusted aggregation. Reuse Taichi compilation, dispatch and thread machinery. Compiler exclusions remain conditional; full B and combined admission remain unaccepted. Held #75/#111/#123 remain unchanged. Earlier status paragraphs below are historical where they conflict.

**Free-ID deque requests (awaiting review, 2026-11-13):** follows #319
review5892365537 and integrates qualified main5733255c through #317. The native free-ID
stack retains std::deque/LIFO behavior while its shared allocator exposes element and
internal pointer-map requests. Fixed metadata inspection does not copy/pop the stack.
Container availability is explicit; field/accessor maps, pointees and full coverage remain
incomplete. Next: those maps, then nested ownership and mixed ABI/string coverage before
trusted aggregation. Compiler exclusions remain conditional. #318–#319 remain source
accepted pending integration; held #75/#111/#123 unchanged.

**Maintainer integration — 2026-11-13:** #316–#317 are accepted and integrated by this merge, preserving reviewed `31cd23abb0f07ab23520ac8c09a7025f7417566e`. Native runs 43427530304 and 43429268947 passed all 12 jobs each; both smoke workflows passed. Personally inspected #317 regression logs show 119 passes (30/30/30/29), with database addressed_reads=24/private_reads=0. Main is qualified through #317. #318–#319 have bounded source acceptance and passing smoke; native qualification remains pending. #319 review 5892365537 accepts request observation for three existing Program registries, explicitly including added counter/control-block costs and excluding pointees/external key strings from those map reports. Next: existing field/accessor maps and free-ID container, then nested ownership and mixed ABI/provenance/string coverage before trusted aggregation. Reuse Taichi compilation, dispatch and thread machinery. Compiler exclusions remain conditional; full B and combined admission remain unaccepted. Held #75/#111/#123 remain unchanged. Earlier status paragraphs below are historical where they conflict.

**Maintainer integration — 2026-11-13:** #315 is accepted and integrated by this merge, preserving reviewed `891f6141b3c849de3a5ae5f587649d7bcc4e1c22`. Native run 43422882717 passed all 12 jobs and smoke 43422882727 passed. Personally inspected regression logs show 119 passes (30/30/30/29), with database addressed_reads=24/private_reads=0. Main is qualified through #315. #316–#318 have bounded source acceptance and passing smoke; native qualification remains pending. #318 review 5891877028 accepts native Program object/vector inspection with map/free-ID/pointee coverage explicitly incomplete. Next: map/free-ID request observation at existing native owners, then nested ownership and mixed ABI/provenance/string coverage before trusted aggregation. Compiler exclusions remain conditional; full B and combined admission remain unaccepted. Reuse Taichi compilation, dispatch and thread machinery. Held #75/#111/#123 remain unchanged. Earlier pending/status paragraphs below are historical where they conflict.



**Program registry requests (awaiting review, 2026-11-13):** follows #318
review5891877028 and integrates qualified main ed9c153d through #315. Program function,
ndarray and argpack maps now reuse native shared request counters; existing owner
inspection reports nodes/buckets and counter/control-block requests. Retained empty
buckets remain observable. Pointees/external strings are separate; field/accessor maps
and the free-ID deque remain incomplete. No parallel registry or execution mechanism.
Next: those remaining containers, nested ownership and mixed ABI/string coverage before
trusted aggregation. Compiler exclusion remains conditional; full B is not established.
#316–#318 remain source accepted pending integration; held #75/#111/#123 unchanged.

**Maintainer integration — 2026-11-13:** #315 is accepted and integrated by this merge, preserving reviewed `891f6141b3c849de3a5ae5f587649d7bcc4e1c22`. Native run 43422882717 passed all 12 jobs and smoke 43422882727 passed. Personally inspected regression logs show 119 passes (30/30/30/29), with database addressed_reads=24/private_reads=0. Main is qualified through #315. #316–#318 have bounded source acceptance and passing smoke; native qualification remains pending. #318 review 5891877028 accepts native Program object/vector inspection with map/free-ID/pointee coverage explicitly incomplete. Next: map/free-ID request observation at existing native owners, then nested ownership and mixed ABI/provenance/string coverage before trusted aggregation. Compiler exclusions remain conditional; full B and combined admission remain unaccepted. Reuse Taichi compilation, dispatch and thread machinery. Held #75/#111/#123 remain unchanged. Earlier pending/status paragraphs below are historical where they conflict.

**Maintainer integration — 2026-11-13:** #313–#314 are accepted and integrated by this merge, preserving reviewed `f850a38f71cad0b1f4c4cc951a477a3b7e62ff94`. Native runs 43417067391 and 43419927538 passed all 12 jobs each; both smoke runs passed. Personally inspected #314 regression logs show 119 passes (30/30/30/29), with database addressed_reads=24/private_reads=0. Main is qualified through #314. #315–#316 have bounded source acceptance and passing smoke; native qualification remains pending. #316 observes the existing Taichi thread-pool object and handle-vector capacity without changing execution machinery. Remaining work: native Program ownership visibility and concrete string/compiler/JIT scope decisions, then trusted lifetime/required-family aggregation with canonical/domain deduplication and prepaid work. Full B and combined admission remain unaccepted; held #75/#111/#123 are unchanged. Earlier pending/status paragraphs below are historical where they conflict.


**Program lifecycle vectors (awaiting review, 2026-11-13):** follows #317
review5891422350 at31cd23ab. Native Program inspection reports its object and existing
kernel/function/tree/texture vector capacity with per-family overflow/availability.
Retained tree slots are distinct from retired backing and recycled slots. No lazy
creation or native execution changes. Map/free-ID/pointee coverage remains unavailable,
full B false. Required mixed metadata/string-contract direction is accepted; compiler
exclusion remains conditional. Next: map/free-ID request observation and remaining
nested ownership, then qualified string/ABI coverage before trusted aggregation.
Main remains qualified through #314; #315–#317 are source accepted pending integration.

**Program/compiler scope audit (awaiting review, 2026-11-13):** follows #316
review5891228994 after integrating qualified main e8b503d7 through #314. The new
[Program accounting audit](../../engine/docs/PROGRAM-ACCOUNTING-SCOPE.md) traces retained Program containers, separately HCP-owned
kernels, compiler provenance/ABI metadata and copied launcher parameters. Serialized
cache-file sizes are not memory evidence. It proposes a narrow compiler-payload exclusion
and a disjoint string-storage contract; neither is accepted or implemented here.
Next bounded code: read-only Program lifecycle object/vector inspection, then map/free-ID
requests and nested ownership, preserving native compilation/dispatch. Full B/combined
admission remain open. #315–#316 remain pending native integration; held PRs unchanged.

**Maintainer integration — 2026-11-13:** #313–#314 are accepted and integrated by this merge, preserving reviewed `f850a38f71cad0b1f4c4cc951a477a3b7e62ff94`. Native runs 43417067391 and 43419927538 passed all 12 jobs each; both smoke runs passed. Personally inspected #314 regression logs show 119 passes (30/30/30/29), with database addressed_reads=24/private_reads=0. Main is qualified through #314. #315–#316 have bounded source acceptance and passing smoke; native qualification remains pending. #316 observes the existing Taichi thread-pool object and handle-vector capacity without changing execution machinery. Remaining work: native Program ownership visibility and concrete string/compiler/JIT scope decisions, then trusted lifetime/required-family aggregation with canonical/domain deduplication and prepaid work. Full B and combined admission remain unaccepted; held #75/#111/#123 are unchanged. Earlier pending/status paragraphs below are historical where they conflict.

**Existing thread-pool storage (awaiting review, 2026-11-13):** follows #315
review5890749492 and integrates current main d262bc4d through qualified #312.
The common runtime_bookkeeping report observes Taichi's already-owned thread pool:
object bytes, constructed handle count and checked handle-vector capacity. No worker
creation, scheduling, dispatch or compiler behavior changes. Full coverage stays false;
OS stacks/hidden thread resources are unmeasured. Next: remaining Program ownership and
concrete string/compiler/JIT scope decisions, then trusted required-family/lifetime
aggregation, canonical/domain deduplication and prepaid work before C+B+H+D.
#313–#315 remain pending integration; held #75/#111/#123 are unchanged.

**Maintainer handoff — 2026-11-13:** #312 is accepted and merged at `176d964a4e1084faa6d470a94bf41a04aad42e57`, preserving reviewed `9d459141c854e923e5eefd6bac776713cd5bb08d`. Native run 43415723155 passed all 12 jobs; smoke 43415723848 passed. Personally inspected regression logs show 119 passes (30/30/30/29), and database evidence reports addressed_reads=24/private_reads=0. Main is qualified through #312. #313–#315 have bounded source acceptance and passing smoke; their native CI remains pending. #315 review 5890749492 accepts inspection of the existing launcher registry, with incomplete coverage and separate potentially-inline name capacity. Contributor-local raw archives have not been independently inspected. Held #75/#111/#123 remain unchanged. Earlier status paragraphs below are historical where they conflict.

**Current direction:** reuse Taichi's existing optimized compilation, registration, dispatch and thread machinery. Inspect those facilities before adding any remaining Program/thread ownership integration. Ownership visibility does not authorize a parallel scheduler, compiler or thread-specific execution mechanism. New machinery requires a demonstrated missing function. Resolve string accounting and compiler/JIT scope before trusted required-family/lifetime aggregation, canonical/domain deduplication and prepaid work for C+B+H+D. Full B and combined admission are not yet accepted.




**Shared launcher registry capacity (awaiting review, 2026-11-13):** follows #314
review5890456367 atf850a38f. Common native inspection reports existing CPU launcher/
context/task/parameter/path/shape capacities, with parameter work checked before traversal.
Absent inspection never creates a launcher. Name capacity is a separate conservative
charge that can overlap inline parameter storage; actual-request/full coverage remains
incomplete. Taichi compilation/registration/dispatch is unchanged. Next: Program/thread
ownership and concrete scope/string-policy decisions, then trusted Harness aggregation
and prepaid work. GPU inspection is deferred, not a detected GPU failure.

**Runtime-global requests (awaiting review, 2026-11-13):** follows accepted #313
implementation direction/review5890147678. Native/common reservation queries now expose
a ten-entry runtime-global catalog, including the existing ambient metadata once.
CPU canonical pool bridges preserve shared backing identity; unsupported GPU/preallocation
inspection remains unavailable. No allocation layout or owner-field changes. #312/#313
remain pending integration; main is qualified through #311. Next: shared launch registry,
Program/thread coverage or reviewed scope decisions, then trusted Harness lifetime/
required-family/canonical-domain deduplication and prepaid work before C+B+H+D.

**Runtime coverage policy audit (awaiting review, 2026-11-13):** follows #312
review5890043642 at9d459141. [Coverage matrix](../../engine/docs/RUNTIME-COVERAGE.md)
separates required-family blockers, metric exclusions and proposed compiler/JIT scope.
No exclusion or admission behavior is implemented. Next bounded code work: native
runtime-global object/table/temporary/random/result request exports, then shared launch
registry and remaining ownership coverage or reviewed scope decisions. Trusted Harness
lifetime/deduplication/prepaid work still precede C+B+H+D. #312 remains provisional;
main is qualified through #311. No new runtime/GPU qualification.

**Runtime bookkeeping tables (awaiting review, 2026-11-13):** starts from main
c5645a28 after qualified #309–#311 integration. Common ProgramImpl inspection now
reports executor/root-manager objects and actual retained storage for three allocation
tables, including counter ownership. Descriptor entries can outlive native backing;
counts describe bookkeeping, not live payload. Constant-time reads preserve explicit
unsupported/empty states; full runtime metadata remains incomplete. Next: remaining
shared surfaces/scoped exclusions, then trusted Harness lifetime/coverage and canonical/
domain deduplication with prepaid work before C+B+H+D. No full B or GPU/Windows claim.

**Maintainer integration and morning handoff — 2026-11-13:** #310–#311 are accepted and integrated by the merge containing this record, preserving reviewed `ec463bdfddbb17d5f78b5cca6fd395e92e565846`. #310 native 43332423596 passed all 12 jobs and smoke 43332423712 passed; #311 native 43337937696 passed all 12 jobs and smoke 43337938428 passed. Personally inspected #311's 119 passing regressions (30/30/30/29), including expanded prepared_backing_test; database job 131603715402 reports addressed_reads=24/private_reads=0. Main is qualified through #311. No active contribution remains awaiting review/integration; only deliberately held #75/#111/#123 remain open. Historical failures remain preserved; contributor-local archives were not independently inspected. No unchanged CI rerun was requested. Earlier pending/next labels below are historical where they conflict.

**Next bounded task for the contributing agent:** start from current main, retaining the qualified #311 implementation. Complete the remaining native runtime/shared metadata surfaces and explicitly enumerate any remaining coverage exclusions. Then construct the trusted lifetime-valid required-family Harness snapshot, deduplicating canonical backing and shared allocator domains with prepaid enumeration/lookup before checked C+B+H+D admission. Keep one charge per shared domain under domain-wide quiescence; map payload is a subset of map requests, and key buffers remain separate. Preserve retained-versus-peak distinctions, native authority and independent work gates. Additional setup cost is acceptable for long-running focal models. Full B and combined admission remain unfinished; GPU/native-Windows execution qualification is not implied. Projection-copy audit stays closed; #75/#111/#123 remain held.

**Maintainer integration — 2026-11-12:** #309 pool container metadata is accepted and integrated by the merge containing this record, preserving reviewed `6b3180b542e1628b13284923e6f98a0f3e3e9f1b`. Native 43328682126 passed all 12 jobs; smoke 43328681937 passed. Personally inspected 119 passing regressions (30/30/30/29), including native_pool_test; database job 131574683251 reports addressed_reads=24/private_reads=0. Main already includes qualified #302–#308. #310 budgeted prepared metadata and #311 shared map allocation domains remain provisionally source-accepted, with smoke passed and native qualification active. Local archives were not independently inspected; historical failures remain preserved. Older pending/next labels below are historical where they conflict.

**Next bounded task:** remaining runtime/shared metadata on reviewed #311, then trusted lifetime-valid required-family Harness coverage and canonical backing/domain deduplication with prepaid enumeration/lookup before checked C+B+H+D admission. Shared allocator domains require quiescence across aliases and one charge per domain. Preserve retained-versus-peak distinctions, native authority and independent work gates; setup cost is acceptable for long-running focal models. No full B or GPU qualification; projection-copy audit stays closed and #75/#111/#123 remain held.

**Prepared map requests (awaiting review, 2026-11-12):** continues reviewed #310
75025c10/review5883920961. Four binding maps now report actual retained node/bucket
allocation requests and their stable shared counter/control-block requests. Map value
payload remains a subset; key buffers are separate. Moves/allocator copies can share
a domain, so domain lifetime/deduplication is explicit; ordinary container copies get
independent domains. Existing inspection preflight remains. Main cee73d53 is already
included through #308; #309/#310 remain provisional pending hosted integration. Next:
remaining runtime/shared metadata, trusted Harness coverage/lifetime/deduplication and
prepaid work before combined C+B+H+D. No full B or GPU/Windows-execution claim.

**Prepared host metadata (awaiting review, 2026-11-12):** continues reviewed #309
6b3180b5/review5883627408. Reports prepared scalar capacity and binding-map key
capacities/value payload via a separate entry-budgeted inspection. Refusal precedes
traversal; exact retry is allocation-free. Node/bucket request storage remains
unmeasured, so full host metadata/B coverage remains incomplete. Main cee73d53's
qualified #308 integration is included through normal merge992d54a0; #309 remains
provisional pending native qualification. Next: remaining prepared/runtime/shared
metadata, trusted Harness lifetime/required-family coverage/deduplication and prepaid
work before combined C+B+H+D. No GPU/Windows execution claim; held items unchanged.

**Maintainer integration — 2026-11-12:** #308 rounded host-pool virtual extents are accepted and integrated by the merge containing this record, preserving reviewed `10b3ed7f6f0401148716311cfe0181e0d75362d9`. Native 43321294234 passed all 12 jobs; smoke 43321294253 passed. Personally inspected 119 passing regressions (30/30/30/29), including native_pool_test, allocation_record_test, prepared_backing_test and reservation_request_test; database job 131549865939 reports addressed_reads=24/private_reads=0. Main already includes qualified #302–#307. #309 pool container metadata is provisionally source-accepted at `6b3180b5`; smoke 43328681937 passed and native 43328682126 remains active. Local archives were not independently inspected; historical failures remain preserved. Older status/next labels below are historical where they conflict.

**Next bounded task:** remaining prepared/runtime/shared metadata on reviewed #309, then trusted lifetime-valid required-family Harness coverage and canonical backing deduplication with prepaid lookup/enumeration before checked C+B+H+D admission. Retained requested storage is distinct from transient peak, heap rounding and physical residency. Preserve native authority and separate work gates; setup cost remains acceptable for long-running focal models. No full B or GPU qualification; projection-copy audit stays closed and #75/#111/#123 remain held.

**Pool container metadata (awaiting review, 2026-11-12):** continues #308
review5883275400 at10b3ed7f. Native map allocator requests and retained allocator
object/vector storage are now reported separately; value/incremental payload remains
a subset, never an extra charge. Actual node allocation requests replace guessed
node overhead. Empty-map storage and reset-copy counter lifetime are explicit; global/
heap overhead and full metadata coverage remain incomplete. Main e62924fd integrates
qualified #306–#307; normal merge488a17bd preserves maintainer handoff and history.
#308 remains provisional with native qualification pending. Next: remaining metadata,
trusted Harness lifetime/coverage/deduplication/prepaid work, then checked C+B+H+D.
No full B or GPU claim; held #75/#111/#123 remain unchanged.

**Maintainer integration — 2026-11-12:** #306–#307 are accepted and integrated by the merge containing this record, preserving reviewed `88aa2cd0ce6360b2cc2d9d1f446294bb50203309`. #306 native 43305748693/smoke 43305748766 passed; #307 native 43310337611 passed all 12 jobs and smoke 43310337168 passed. Personally inspected #307's 119 passing regressions (30/30/30/29), including native_pool_test, allocation_record_test, prepared_backing_test and reservation_request_test; database job 131512684418 reports addressed_reads=24/private_reads=0. Main already includes qualified #302–#305; the original #302 initialization failure remains preserved. #308 rounded virtual mapping extents are provisionally source-accepted at `10b3ed7f` in review 5883275400; smoke passed, native qualification remains active. Contributor-local archives were not independently inspected. Older pending/next labels below are historical where they conflict.

**Next bounded task:** remaining prepared/runtime/pool metadata on reviewed #308, then lifetime-valid required-family Harness coverage and canonical backing deduplication with prepaid lookup/enumeration work before checked C+B+H+D admission. Rounded mapping extents are not physical residency; shared chunks and contained requests are not disjoint charges. Preserve native authority and separate work gates; setup cost is acceptable for long-running focal models. No full B or GPU qualification is claimed. Projection-copy audit stays closed; #75/#111/#123 remain held.

**Rounded host-pool extents (awaiting review, 2026-11-12):** continues #307
review5882509096 at88aa2cd0. Native setup captures OS page size; checked rounding is
passed to the OS and retained separately from raw request sizes. Live pool records
report virtual rounded extents; request/view bounds remain unchanged, so padding does
not enlarge valid views. Added metadata is included once by existing sizeof reports.
Main a139c21b integrates qualified #302–#305; normal merge ba723dd0 reconciles the
handoff without discarding prior contribution history. #306/#307 native and smoke
runs now pass; maintainer integration remains pending. Next: remaining metadata, Harness lifetime/
coverage/deduplication/work, then checked C+B+H+D. Linux CPU scope; no full B/GPU claim.

**Maintainer integration — 2026-11-12:** #302–#305 are accepted and integrated by the merge containing this record, preserving reviewed `44c890b78c973be21a5b2e705ac79c334da3ae4e`. Corrected #304 native 43299289153 and smoke 43299289118 passed; #305 native 43302942903 passed all 12 jobs and smoke 43302942605 passed. Personally inspected 119 passing regression tests (30/30/30/29), including allocation_record_test, owner_backing_test, prepared_backing_test and reservation_request_test; database job 131487117070 reports addressed_reads=24/private_reads=0. The original #302 initialization failure remains preserved, not reclassified as green. Contributor-local archives were not independently inspected. #306 prepared canonical import adoption and #307 host-pool identity remain provisionally source-accepted, with native qualification pending; their smoke runs passed. This supersedes older pending/status labels below.

**Next bounded task:** authoritative rounded extents and remaining prepared/runtime/pool metadata on the reviewed #307 stack, followed by trusted Harness required-family coverage, lifetime validity, canonical backing deduplication and prepaid lookup/enumeration work before checked C+B+H+D admission. Shared raw chunks and contained requests are not disjoint charges. Preserve independent work gates and native authority; setup cost is acceptable for long-running focal models. No complete B or GPU qualification is claimed. Projection-copy audit stays closed; #75/#111/#123 remain held.

**Host-pool backing identity (awaiting review, 2026-11-12):** continues #306
review5881942652 at52696f12. Native raw allocation identities survive chunk-index
movement and reject stale tokens after release/reset/reuse. Common Device/Program
queries resolve proven live owner/request ranges; raw imports remain unresolved.
Raw request sizes are exported, rounded OS extents remain unavailable. Shared pool
metadata is reported once by pool identity with explicit exclusions, not per owner.
Lookup work is logarithmic and remains to be metered with Harness enumeration.
Main e11525a0 is already an ancestor; #302–#306 integration awaits hosted qualification.
Next: rounded extents/remaining metadata, trusted Harness coverage/deduplication/work,
then combined C+B+H+D. No full B or GPU claim; held #75/#111/#123 remain unchanged.

**Prepared owned-import adoption (awaiting review, 2026-11-12):** continues #305
review5881732238 at44c890b7. Separate canonical backing queries preserve captured
registration identity, validate bound byte extent against the registration, and
accept only live explicitly proven owned backing for array coverage. Raw imports
remain unresolved; full host metadata remains incomplete. No new retained metadata.
Summary traversal remains input-sized work requiring future metering. Main e11525a0
is already in this stack; #302–#305 remain unmerged pending corrected parent/hosted
qualification. Next: underlying pool identity/rounded extents and remaining metadata,
then trusted Harness lifetime/deduplication/work accounting and combined C+B+H+D.
No full B or GPU claim; held #75/#111/#123 remain unchanged.

**Explicit owned-import backing (awaiting review, 2026-11-12):** continues #304
review5881459672 at d40daac0. Common Device captures same-device Owned provenance at
import creation and resolves canonical live backing with bounded indexed queries.
Raw imports remain unresolved; registrations, backing ownership and lifetime remain
distinct. Embedded metadata is included once in existing sizeof-based row capacity.
Main e11525a0 is already in this stack. #304's source blocker is corrected, but
#302–#304 integration still awaits corrected combined hosted qualification. Next:
underlying pool identity, remaining prepared/runtime metadata and binding adoption,
then a lifetime-valid work-metered Harness snapshot and combined C+B+H+D admission.
No full B or GPU claim; held #75/#111/#123 and projection-copy closure are unchanged.

**CTest qualification correction (awaiting review, 2026-11-12):** #304 review
5881347735 blocks integration on missing runtime environment for three tests.
#302 native run43287680849/job131433550369 failed owner_backing_test during
initialization; its assertions were not reached. Earlier local runs inherited
TI_LIB_DIR and did not detect the configuration omission. Corrected combined head
e0cc9e72 adds owner_backing_test, prepared_backing_test and reservation_request_test
to the existing explicit runtime environment/900-second timeout group. All three
pass with TI_LIB_DIR unset in the parent shell, as does the combined nine-test
backing/reservation subset. Original failures and earlier evidence remain preserved;
#302 is not represented as green. The fix is carried at the combined #304 head,
not by rerunning unchanged parents. Await corrected hosted qualification/review
before integration; pool/alias/metadata work remains next.

**Native reservation exports (awaiting review, 2026-11-12):** continues #303 review
5880964608. Common ProgramImpl exposes bounded CPU node/list/ambient request views,
with Present/Absent/Invalid/Unavailable states and explicit unresolved pool identity/
rounded extents. Ambient request metadata is reported once per runtime; enumeration
remains caller-metered work, not a hidden scan. Main e11525a0 (#301 integrated) is
reconciled by normal merge b86533e5; #302/#303 remain provisional and unmerged.
Next: pool/import alias resolution, remaining metadata and a trusted lifetime-valid,
work-metered Harness snapshot before combined C+B+H+D. No full B or GPU claim.

**Prepared CPU backing exports (awaiting review, 2026-11-12):** continues #302
review 5880581671. Common launch-context queries expose actual host buffers and
captured array identities, with separate family coverage and explicit incomplete
host metadata. Imports/external/raw bindings cannot certify complete owned backing.
Records are embedded in existing binding rows and included by existing sizeof-based
capacity accounting. Main 0e82ebe0 (#300 integrated) is reconciled by normal merge
f0e1e4b8; #301/#302 remain provisionally accepted and unmerged. Next: node/list/ambient
exports, remaining metadata and alias resolution, trusted Harness lifetime/coverage,
then combined admission. No full B or GPU qualification is claimed.

**Array/root owner exports (awaiting review, 2026-11-12):** continues #301 review
5880096613 with captured Ndarray identities and native tree-root exports through
common ProgramImpl. Released/reset/foreign records remain explicit; imports are
unresolved registrations, not certified backing. Incremental root-record metadata
is reported once per runtime. Prepared invocation and node/list coverage remain
unavailable; no complete B ledger or combined admission is claimed. Main b2eb86c7
(#299 integrated) is reconciled by normal merge ca24735f; #300/#301 remain unmerged.
Next: prepared invocation and node/list exports, alias resolution and trusted
Harness coverage/lifetime, then combined admission. The projection-copy audit stays closed.

Read when selecting work, reviewing acceptance or asking what is built. For the next bounded task, use the [continuation handoff](../core-local-study-continuation-plan.md#next-agent-task--2026-11-05). You do not need the conceptual introduction first.

## Current state

**Maintainer integration — 2026-11-12:** #301 common/CPU allocation records are accepted and integrated by the merge containing this record, preserving reviewed `63a46aab6db261d2b8ecf189d06999d87f2f2dd2`. Native 43282494612 passed all 12 jobs and smoke 43282494900 passed. Personally inspected 116 passing regression tests (29/29/29/29), including allocation_record_test, and database job 131415289289 with addressed_reads=24/private_reads=0. #302 array/root exports remain provisionally accepted at `aa48a2e4` (review 5880581671); #303 prepared CPU exports at `bb0dfd28` are provisionally accepted in [review 5880964608](https://github.com/Human-Cognome-Project/human-cognome-project/pull/303#pullrequestreview-5880964608). Both remain unmerged with native qualification active; their smoke checks passed. Contributor-local artifacts were not independently inspected. This does not establish full B, combined admission or GPU qualification.

**Next bounded task:** native node/list/ambient request exports on reviewed #303, then remaining prepared metadata and imported-to-owner alias resolution. Preserve explicit required-family coverage and account retained metadata once at its actual owner. The trusted Harness snapshot must establish lifetime/backing validity, deduplicate actual backing rather than registrations, and meter input-sized enumeration/validation before checked C+B+H+D enforcement. Retain separate work gates and native authority; remaining operation/control integration follows. Projection-copy audit stays closed and additional setup cost is acceptable for long-running focal models. #75/#111/#123 remain deliberately held. Earlier pending/next labels below are historical where they conflict.

**Maintainer integration — 2026-11-12:** #300's borrowed-resource audit is accepted and integrated by the merge containing this record, preserving `e540a8712c176c06d2f5729da2aa6602d67ec4cb`. Native 43276619607 passed all 12 jobs; smoke 43276619735 passed. Personally inspected 115 passing regression tests (29/29/29/28) and database job 131388302021 with addressed_reads=24/private_reads=0. Main includes #299 owner bootstrap admission and now the [borrowed-resource audit](../../engine/docs/BORROWED-RESOURCES.md). #301 CPU record interface remains provisionally accepted at `63a46aab` (review 5880096613), and #302 array/root exports at `aa48a2e4` in [review 5880581671](https://github.com/Human-Cognome-Project/human-cognome-project/pull/302#pullrequestreview-5880581671). Both remain unmerged with native qualification running; their smoke checks passed. Local archives remain contributor-reported, not independently inspected. Neither a complete B ledger nor combined admission is built.

**Next bounded task:** prepared-invocation backing exports and node/list/ambient reservations on reviewed #302. Preserve explicit required-family coverage, owner lifetime and request/rounded extent distinctions. Resolve imported registrations to actual backing owners before deduplication; account runtime metadata once. Missing coverage or unresolved ownership keeps the complete ledger unavailable. Then assemble the trusted lifetime-valid Harness snapshot, enforce checked C+B+H+D with separate work gates and complete remaining operation/control integration. The projection-copy audit stays closed; additional setup cost is acceptable for long-running focal models. #75/#111/#123 remain deliberately held. Earlier pending and next-task labels below are historical where they conflict.

**Maintainer integration — 2026-11-12:** #299 is accepted and integrated by the merge containing this record, preserving reviewed head `5771326e4992889c3b9c16b8b98ee752ae4d0d1a`. Native 43274126767 passed all 12 jobs; smoke 43274126747 and warm 43274126853 passed. Personally inspected regression logs total 115 passing tests (29/29/29/28); database job 131379495998 reports addressed_reads=24/private_reads=0. #298 is already merged at `319f892a`. #300 audit is source-accepted at `e540a871` under review 5879635043 and #301 common/CPU allocation interface is provisionally accepted at `63a46aab` under [review 5880096613](https://github.com/Human-Cognome-Project/human-cognome-project/pull/301#pullrequestreview-5880096613); both remain unmerged with native qualification active. Local archives remain contributor-reported, not independently inspected. Main now includes owner bootstrap admission, not the pending native record interface or complete B ledger.

**Next bounded task:** actual native owner backing exports on reviewed #301, including array/tree-root, node/list and prepared-launch families. Resolve imported registrations to actual owners before deduplication; record identity alone is not backing identity. Preserve explicit coverage and unavailable status, lifetime validity and retained metadata accounting. Then assemble the trusted Harness snapshot, enforce checked disjoint C+B+H+D admission and complete remaining operation/control integration. HCP model code remains backend-independent. Initial-projection copy audit stays closed; additional setup cost is acceptable for long-running focal models. #75/#111/#123 remain deliberately held. Earlier pending and next labels below are historical where they conflict.

**Previous disposition (historical):**

**Maintainer disposition — 2026-11-12:** #298 is accepted and merged at `319f892a1aa35e29597e89e461c0512eba233e33`. Native 43268613665 passed all 12 jobs; smoke 43268613606 and warm 43268613622 passed. Personally inspected regression logs: 115 passing tests (29/29/29/28); database job 131360157711 reports addressed_reads=24/private_reads=0. #299 owner integration is provisionally source-accepted at `5771326e` (review 5879484896), but native qualification remains active and it is unmerged. #300's five-file documentation audit is accepted at `e540a871` in [review 5879635043](https://github.com/Human-Cognome-Project/human-cognome-project/pull/300#pullrequestreview-5879635043); smoke passed, native is active, and it remains unmerged. Local probe archives are contributor-reported, not independently inspected.

**Next bounded task:** implement the common native allocation-record contract and CPU implementation/tests authorized by #300: original request versus rounded extent, live/retired/unknown status, explicit import ownership and validity across device lifetime/reset/reuse. Unsupported coverage is unavailable, never zero; HCP model code consumes one interface. Then export actual owner backing (including tree/node/list and prepared-launch families), assemble the trusted Harness ledger and enforce checked combined C+B+H+D admission. Resolve aliases and pool/suballocation overlap at the owning boundary; include new ledger storage in the policy and keep backing validity distinct from publication generation. Do not query dead device pointers to test expiry. This is scoped accounted-byte work, not a process-wide profiler. Remaining operation/control integration follows. The projection-copy audit stays closed; additional setup cost is acceptable for long-running focal models. #75/#111/#123 remain deliberately held. Earlier pending and next-task labels below are historical where they conflict.

**Maintainer integration — 2026-11-12:** #295–#297 are accepted and integrated by the merge containing this record, preserving original commits through `272e160d56201fd1f93353c5ba22efd88f0ef69d`. #297 review [5878815959](https://github.com/Human-Cognome-Project/human-cognome-project/pull/297#pullrequestreview-5878815959) found no blocking source defect. Native 43252923816 passed all 12 jobs; smoke 43252923333 passed. Personally inspected regression logs total 115 passing tests (29/29/29/28); database job 131305209894 reports addressed_reads=24/private_reads=0. #295 native 43245334785/smoke 43245334673 and #296 native 43249326537/smoke 43249326530 also passed. Local archives remain contributor-reported, not independently inspected. This accepts binding setup, the accounting inventory and checked initial-map work prepayment, not complete owner-resource, GPU or performance qualification.

**Next bounded task:** track actual domain bootstrap request bytes with an outer tracker whose lifetime safely covers domain/control-block destruction, and establish a trusted disjoint borrowed-resource ledger. Then enforce combined accounted-byte admission using those disjoint components and selected live requests; do not sum overlapping snapshot, retained, peak or cumulative-work counters. Preserve checked arithmetic, pre-allocation refusal, reclamation, native state and exact retry controls. Remaining operation/control integration follows. The projection copy audit is closed. Additional setup work remains acceptable for long-running focal models; reducing setup totals is not an acceptance gate. #75/#111/#123 remain deliberately held/deferred.

**Native allocation records (awaiting review, 2026-11-12):** #300 review 5879635043
accepts a CPU-first common native contract. This contribution adds explicit record
status/ownership and separate original-request/extent fields, with device-instance
and reset-epoch value tokens checked only through a known-live owner. CPU supplies
the records and retained metadata; unsupported backends report Unavailable. Tests
cover alias/import/retirement/reset/address reuse and registration-failure retry.
See [native record contract](../../engine/docs/BORROWED-RESOURCES.md#cpu-allocation-records-awaiting-review-2026-11-12). Main `9025c301` (#298 integrated) is reconciled by normal merge `2a919e22`;
#299/#300 remain reviewed but unmerged. Next owner backing exports, trusted Harness
coverage/lifetime, then combined admission; no complete B/GPU acceptance is claimed.
**Borrowed backing audit (historical contribution, 2026-11-12):** #299 review 5879484896
accepts owner bootstrap provisionally and authorizes the trusted native B ledger.
The source audit identifies missing common allocation extent/lifecycle queries,
private SNode/node-pool and prepared-launch backing, and incomplete per-harness
coverage. A focused CPU probe confirms alias overcount, cross-device numeric-ID
collision and retained size metadata after release. See [borrowed resources](../../engine/docs/BORROWED-RESOURCES.md) for source pointers,
explicit exclusions, validity requirements and the bounded native-hook sequence.
No B ledger or combined gate is implemented by this audit. Next implement native
request/lifecycle observation and owner backing exports before assembling trusted B;
then enforce combined admission. #298/#299 remain open with native CI active.
**Owner bootstrap integration (historical contribution, 2026-11-12):** continues from
#298 review 5879152987 on `386be0a0`; #298 remains open while hosted qualification
finishes. Owner admission now gates actual outer tracker/domain control-block
requests and reports them separately from selected owner requests. Both bootstrap
boundaries precede owner construction/native mutation. Later constructor refusals
and allocation exceptions release bootstrap storage while preserving prepaid work;
selected weak references retain the tracker through final release. See [adoption accounting](../../engine/docs/ADOPTION-ACCOUNTING.md#owner-bootstrap-integration-awaiting-review).
Next: the trusted disjoint borrowed-resource ledger, then combined accounted-byte
admission and remaining operation/control integration. Both builds and all 47 scoped CPU checks passed without warnings; this new
integration evidence is recorded separately from the accepted helper evidence.
**Bootstrap tracker prerequisite (historical contribution, 2026-11-12):** a new shared
native helper accounts for its own outer allocation and the actual rebound
reservation-domain/control-block request, with lifetime through the final weak
release. It keeps bootstrap and selected requests disjoint. See
[shared support](../../common/README.md#domain-bootstrap-tracker-awaiting-review) for the contract and standalone refusal/unwind/lifetime coverage.
This contribution starts from integrated main `f20acbb8`; owner adoption still
uses its existing factory. Next wire the helper into owner admission with native
unchanged-state/retry evidence, establish the trusted borrowed-resource ledger,
then enforce combined accounted bytes. No complete owner-resource claim is made.
**Historical handoff records below:** earlier pending, awaiting-review and next-task statements are source-pinned history, superseded by the disposition above.

**Maintainer integration — 2026-11-12:** reviewed #290–#294 are integrated by the merge containing this record, preserving original commits through `32fe9928cdbed11b5092724d81f3bfbff07f0b9d`. #290's retry 43225419105 and smoke passed. #291/#292 native/smoke passed. Corrected #294 native 43240639938 passed all 12 jobs and smoke 43240639817 passed. Personally inspected #294 logs: 115 regression tests (29/29/29/28), and database job 131262194888 reports addressed_reads=24/private_reads=0. #293's original GCC 11 cleanup failure and #290's original unavailable-log failure remain historical; corrected/retried evidence does not relabel them.

#295 remains provisionally source-accepted at `c118e6ef` under [review 5877025531](https://github.com/Human-Cognome-Project/human-cognome-project/pull/295#pullrequestreview-5877025531); its native qualification is still running. #296's documentation inventory at `c43de241` is source-accepted in [review 5877302279](https://github.com/Human-Cognome-Project/human-cognome-project/pull/296#pullrequestreview-5877302279); its smoke passed and native run is active. Neither is integrated here. This does not establish complete owner-resource, GPU or performance acceptance.

**Next bounded task:** implement the reviewed work-component correction in #296: preserve the input byte census, replace only the two initial map work components with checked fresh-map demand, and prepay the projection copy once. Expose the breakdown and define the bounded fixed-construction policy explicitly. Extend existing zero/one-short/exact and overflow/refusal/reclamation/native-state/retry controls. Then establish domain bootstrap tracking and a trusted disjoint borrowed-resource ledger before enforcing combined accounted-byte admission. Do not sum overlapping snapshot, retained, peak and cumulative-work counters. Remaining operation/control integration follows.

Additional setup work is acceptable where it benefits long-running focal models; lower setup totals are not an acceptance gate. The initial-projection copy audit is closed. #75/#111/#123 remain held/deferred. Earlier dated pending, integrated and “next” labels below are historical and superseded by this record where they conflict.


**Map copy-work contribution (awaiting review, 2026-11-12):** implements #296
review 5877302279's checked component replacement. Projection work is retained
once; fresh occurrence/field map work is prepaid separately and exposed in the
result. The byte census and selected request/allocation-work limits are unchanged.
Fixed units explicitly cover bounded header/scalar/control initialization. Empty,
unequal-input, overflow and pre-allocation refusal/retry controls accompany the
change. Both builds, 47 scoped CPU checks and six standalone compiler CTests passed
without warnings; see admission for source-pinned evidence. Main through #294 is
included by normal merge `9acdd5fb`. Next: domain bootstrap tracking and a trusted
disjoint borrowed ledger before combined admission; operation integration follows.

**Binding setup contribution (awaiting review, 2026-11-12):** continues from #294
review 5876572990 at `32fe9928`, including current main `7bf08034` by normal merge
`cdcf28e0`. The shared owner setup meter now covers binding/coordinator buffer
reserves, map construction/reservation, staging and adoption capture under a
Bindings phase. Later operational staging/capture/matching allowances remain
separate; standalone unrestricted wrappers stay explicit. Expanded map/owner
refusal controls passed with both builds and all 47 scoped CPU checks, without
warnings. Source-pinned qualification is recorded in admission.
Next after review: complete the remaining setup/resource inventory and combined
caller/owner/borrowed accounting, then remaining operation/control integration.
This is not complete owner-resource acceptance; projection copy cleanup stays closed.

**Accounting inventory contribution (awaiting review, 2026-11-12):**
[The source audit](../../engine/docs/ADOPTION-ACCOUNTING.md) maps all adoption operations after #295 and
resolves the initial-map distinction: input prepayment covers one logical row copy,
while the metered map constructor additionally counts reservation and nil-slot
initialization. Selected allocations are already gated. The proposed next change
replaces only map work components, retaining projection prepayment once and leaving
the input byte census distinct. Fixed-construction policy, domain bootstrap tracking
and a trusted disjoint borrowed-resource ledger must be explicit before a combined
limit; the inventory specifies that finite implementation boundary. Runtime code is
unchanged and accepted native baselines are not rerun for this documentation slice.

**Historical records below:** earlier pending, integrated and “next” statements are source-pinned history and are superseded by this disposition where they conflict. #75, #111 and #123 remain deliberately held/deferred. CPU acceptance does not qualify the full GPU path.

**Maintainer acceptance — 2026-11-12:** #285 and #286 are integrated into main at `087b6aff14d454c9ebafcda28c9e9243f596e4d2`, reviewed tip `49643f2a22bbb0076584e01d990a32ff7fe0acda`. Exact-head #286 native 43130677310 and smoke 43130677322 passed; #285 native 43121173964 and smoke 43121173792 also passed. All 12 #286 native jobs succeeded. Personally inspected regression logs total 115 passing tests (29/29/29/28); database job 130894721312 reports addressed_reads=24/private_reads=0. At that integration the active review queue was clear; #75, #111 and #123 remain deliberately held. Earlier provisional/pending labels below are historical and superseded by this acceptance. No GPU or performance qualification is implied.

**Active declaration setup contribution (2026-11-12):** #293 `b0f2f289` is
provisionally accepted by review 5875958258. Main remains `a4de248e`, integrated
through #289; #290–#293 are unmerged. Declaration collection and reverse-copy work
now share the owner setup meter under a distinct Declarations phase. Selection,
occurrence/tool searches, payload comparisons/copies, source/leaf accumulation,
relocation and declaration publication are admitted before work. Standalone paths
remain unrestricted. This continuation also corrects GCC 11 partial-allocation
cleanup failures exposed by #293 hosted projection-values job 131239777540; the
original failed run is preserved. Standalone GCC 11/13 checks now supplement the
native CPU qualification. Both builds, 47 scoped CPU checks and six standalone
compiler CTests passed; one post-PASS cache-lock warning is preserved. See admission
for source-pinned qualification.

Next: ExchangeBindings constructor reservation/copies, staging and capture under
the shared setup allowance, replacing owner-path UINT64_MAX gaps while preserving
separate later operational capture/matching budgets. Combined accounting and
operation/control integration follow. Projection cleanup stays closed; this is
not the complete owner setup gate. Hosted qualification/review remain required.

**Active tool-index contribution (2026-11-11):** continues from source-accepted
#285 `7105c22e` under review 5866915489, with accepted main `c2c168b3` included by
normal merge `9f88ce31`. Temporary normalization/projection tool-index keys borrow
stable source objects and retain value ordering/validation. A focused prior-source
probe records 22→0 UUID-sized requests and validates independent lookup keys;
source-death and duplicate-order controls remain active. Both builds and 47 scoped
CPU checks passed; one post-PASS cache warning is preserved. Detailed qualification
is in the admission record; hosted qualification/review remain pending. Next:
initial occurrence indexes/publication setup, then membership callable/recursion,
declaration/binding budgets and combined resource/operation integration.


**#285 qualification update (2026-11-11):** Patrick has accepted the contribution;
exact-head native 43121173964 and smoke 43121173792 now both report success.
The PR remains open at inspection; no merge is inferred. Earlier pending labels
are historical. This does not qualify the new tool-index continuation.

**Maintainer integration — 2026-11-11:** Reviewed #275–#277 and #279–#284 are integrated into main at `5beeb2cacf28c75cbecef385a0de1e877154bbc0`, preserving original commits through reviewed tip `09bafe48869ddf239e6e245d766c0c0e41a43364`. #279's fresh native 43115507493, viewport 43115507480 and smoke 43115507510 succeeded; #280–#284 completed their applicable hosted checks, including #284 native 43116769390 and smoke 43116769371. Original failed and stuck queued attempts remain historical, not relabeled. Earlier pending/integration labels below are superseded by this record. #285 is provisionally source-accepted at `7105c22e866af7c3cf5c1b2e452a8bf8bef643f8` (review 5866915489), targets main and awaits native qualification; its smoke passed. Next: initial tool/occurrence indexes and publication/setup costs, then membership callable/recursion, declaration/binding budgets, combined resource accounting and operation integration. No blanket GPU or performance acceptance.


**Active rendering-copy contribution (2026-11-11):** #283 `38e81a95` has no blocking
source finding and green exact-head native/smoke per review 5866277807. This
continuation removes owning occurrence-key temporaries used only for initial
projection text rendering; public validation and independent output remain intact.
Both builds and 47 scoped CPU checks passed; a post-PASS cache warning is retained.
Detailed evidence is in the admission record; hosted qualification/review remain
pending. Initial projection indexes/provenance/equivalence, membership callable/
recursion and declaration/binding setup budgets precede combined resources and
remaining-operation integration.

**Active provenance/equivalence contribution (2026-11-11):** #284 `09bafe48` is
provisionally accepted with no blocker by review 5866703067. This continuation
borrows stable source ToolRefs in temporary projection lookup values, constructing
independent published identities at consumption. Source-death tests compare full
pinned references; a source-to-source probe records 45 fewer UUID-sized requests.
Both builds and all 47 scoped CPU checks passed without warnings; detailed evidence
is in the admission record. Hosted qualification/review remain pending. Initial tool/occurrence indexes and publication setup, membership callable/
recursion and declaration/binding setup budgets precede combined resource/operation
integration. #279 remains unmerged; its fresh native matrix is running while
smoke/viewport are green. #284 smoke 43116769371 is green and native 43116769390
was running at maintainer review.

**#279 queue remediation (2026-11-11):** old native/viewport/smoke attempts remained
queued, yet cancel/force-cancel returned 409 and rerun 403. Close/reopen triggered
fresh native 43115507493, viewport 43115507480 and smoke 43115507510 at unchanged
reviewed `5f1ca0c3`; old attempts remain preserved. Smoke and viewport passed at inspection;
native had passed build/projection-values and was running its matrix. #280–#283 green checks do not replace #279's
combined-head qualification. The stack remains unmerged pending maintainer action.

**Provisionally accepted #282 (2026-11-11):** review 5865626048 accepts the adoption
argument-copy slice at `29b82129`. Its 47 scoped CPU checks passed locally; the
initial obsolete-assertion failure is preserved. Maintainer inspection reports
smoke 43104595184 green, native 43104595076 running; #280/#281 native still running
and #279 latest-head workflows queued. These are not integration acceptance.

**Provisionally accepted #281 (2026-11-11):** review 5865407113 accepts the
nil/containment source provisionally at `15db4b81`, with no blocking findings.
Its 47 scoped CPU checks and two prior-source negative controls passed locally.
At inspection, smoke 43101300624 and warm 43101300677 succeeded; native
43101300728 was queued. These states do not imply integration or qualify the
new adoption-copy head.

**Provisionally accepted #280 (2026-11-11):** allocation-free ObjectKey ordering on
provisional #279 head `5f1ca0c3`, authorized by updated-head review 5865153858.
Both-address validation and the rendered-key ordering oracle are preserved.
All 46 scoped local CPU checks passed; a post-PASS cache warning remains recorded.
Awaiting review and hosted qualification. Next: nil/containment validation and
temporary identity copies, then traversal/setup budgets and combined resources.
#279 is the main-targeted integration PR for the pending text stack; dependent PRs
remain unmerged until its hosted qualification and maintainer disposition.

**Maintainer reconciliation — 2026-11-11:** #278 is accepted and merged at `3cadbff286b0c4553808595f624f8a98add36682`, reviewed head `07f2d66763631afe8d62438351bfd7ece1f149c8`. Native 43087248989, smoke 43087248327 and viewport 43087248550 passed. Inspected native and viewport installation logs confirm the mirror-list rewrite and archive.ubuntu.com traffic. The earlier pending labels are historical. #279 identity/reference input text is source-reviewed and provisionally accepted at `11a4813759c1e384fa6b077499c9f06998c66f58` (review 5864859891); smoke/viewport are green and native matrix qualification is pending. #275–#277 and #279 remain unmerged. Current main at 6067c641 is brought into this stack by a normal merge; the combined head a75e3408 passed 45 scoped local CPU checks plus mirror/CI contract checks. Updated-head hosted qualification and maintainer integration remain pending. Next bounded work: remaining identity/comparison-formatting and membership traversal/callable audit, then complete setup, combined accounting and operation/control integration.


**Queue reconciliation — 2026-11-11:** Reviewed #268–#274 are integrated into main at `b05db31c2e591f8fbcd5c14dc7ddff14ef46b2e9`, reviewed tip `ce3642ee463beb76948a2497c9aeb50172c3267d`. Every head in that stack has successful native and smoke workflows; #272 warm also passed. #274 native 43030713058 passed all 12 jobs; inspected regression logs total 115 tests and database reports addressed_reads=24/private_reads=0. Earlier provisional/pending labels below are historical and superseded by this acceptance.

**Earlier review queue (preserved history):** #275 parent-value text, #276 contact/membership/focus text and #277 selection text are source-reviewed and provisionally accepted, but remain unmerged. Inspected native jobs 130564239034 (#275), 130638929459 (#276 retry) and 130648619590 (#277) failed during toolchain installation because azure.archive.ubuntu.com could not resolve, before product compilation. #276 viewport retry job 130638930090 was cancelled during dependency installation with the same DNS failures. Preserve these attempts; complete successful hosted qualification before integration. The next coding slice is definition/occurrence identities and references, per #277 review 5863228338. Other identities/comparison formatting, complete setup, combined accounting and operation/control integration remain open. #75, #111 and #123 remain deliberately held.

**Earlier infrastructure follow-up (now accepted in #278):** normalize the affected native/viewport runner APT sources to the primary Ubuntu archive, preserving authentication, pinned toolchain and gates. Local mirror fixtures, ShellCheck and orchestration contracts passed; hosted qualification remains required. See [hosted CI](../../engine/docs/HOSTED-CI.md#ubuntu-archive-resolution-repair-2026-11-11-awaiting-review). Definition/occurrence identity text remains the next coding slice.

C2b-1 is accepted in #192 under the [#191 plan](../../docs/cumulative-publication-integration-plan.md). The [C2b-2 weighted native probe](../../engine/docs/WEIGHTED-NATIVE-HANDOFF.md) is accepted in #193 with [source-pinned evidence](../../engine/tests/weighted_handoff/README.md). It qualifies held-input numerical propagation, not production hierarchy or inherited response.

The [production hierarchy/response plan](../../docs/production-hierarchy-response-plan.md)
is accepted in #194. It specifies comparison and force ownership, production
reader/history lifetimes, combined integration and coupled recovery. Its C3-1
fixed-tree implementation is accepted in #195; C3-2 coherent refresh is accepted
in #198. B1 contact-safe ticks are accepted in #201; private admission/bootstrap and atomic exchange are accepted through #210/#219.

The [accepted C3-1 implementation](../../engine/docs/LIVE-HIERARCHY.md) now supplies
that opt-in fixed-tree CPU path. Its executable acceptance now follows the
[coherent-dynamics direction](../../docs/coherent-dynamics-gpu-snapshots.md); see the separately pinned
[revised observations and original investigation](../../engine/tests/live_hierarchy/README.md).
The active-capture adapter is accepted. #196 supplies scoped local GTX 1070
evidence; full integrated CUDA qualification remains open.

This file is the maintained status entry point. Code and executable evidence determine
what is built; accepted decisions govern intended behavior. A conflict between
them and this summary is drift to investigate, not permission to silently pick
one. Check [open PRs](https://github.com/Human-Cognome-Project/human-cognome-project/pulls)
and latest maintainer dispositions before implementing. Update this section when
a merge or review changes Built/Held/Active status; other entry points link here.
Keep this table and the immediate task current rather than adding another milestone
row for every PR. Put detailed run/commit evidence in the owning acceptance record.
When the next task changes, reconcile this status and the continuation task
in the same documentation change. Preserve provisional versus merged acceptance.

| State | Current scope and authoritative evidence |
|---|---|
| Accepted data and preparation | Retained [letter/core snapshots](../../data/postgres/snapshots/2026-10-13-letter-seed-validation/MANIFEST.md), native [warm tools](../../kernels/database/warm/README.md), coordinate-free [v2 recipes](../../kernels/database/warm/STRUCTURAL-RECIPES.md), [projection](../../engine/docs/LETTER-PROJECTION.md) and prepared native backing/updates. Preserve existing stores; population and operational ownership are separate. |
| Accepted CPU numerical path | Cumulative publication, [accepted motion](../../engine/docs/ACCEPTED-MOTION.md), [installed hierarchy](../../engine/docs/LIVE-HIERARCHY.md), coherent [refresh](../../engine/docs/LIVE-REFRESH.md) and [contact-safe ticks](../../engine/docs/CONTACT-SAFE-TICKS.md), accepted through #198/#201. Scope-specific GPU evidence does not qualify the complete path. |
| Accepted private operations | Certified placement (#203), ordinary admission/bootstrap (#210), live focus (#211), exact full/reduced f64 rebase (#212/#213), and source-bound mass/momentum/placement accounting with [atomic exchange](../../engine/docs/ATOMIC-EXCHANGE.md) through #219. These are bounded private primitives; an enduring operational owner/public Add API is not accepted. |
| Latest accepted engine evidence | Integrated through corrected #294, reviewed head `32fe9928`; all 12 native jobs and smoke passed. Inspected 115 regression tests and addressed_reads=24/private_reads=0. |
| Accepted exchange owner | Scopes through #297 integrated: binding setup, accounting inventory and checked initial-map work prepayment. Combined admission remains open; projection-copy audit stays closed. |
| Next implementation | Node/list/ambient exports on reviewed #303, remaining metadata/alias resolution, trusted Harness ledger and combined admission. |
| Factoring follow-up | #230 completes the first mechanical kernel extraction under [#229](https://github.com/Human-Cognome-Project/human-cognome-project/issues/229). Keep #229 open; further separation follows the resource/lifetime inventory. Layered onboarding is authorized on 2026-11-08; see the [migration plan](ONBOARDING-PLAN.md). |
| Accepted tooling | [Local evidence runner](../../engine/docs/LOCAL-CI.md) (#204), [parallel hosted CI](../../engine/docs/HOSTED-CI.md) (#206), and optional [offline viewport](../../tools/viewport/README.md). #220's AJV 8.18.0 fix is merged: hosted 15 model tests, reproducible bundle and 125 browser checks passed. Observation is not engine control or proof of natural clustering. |
| Outstanding qualification | #156 owns integrated operation acceptance; #145 owns actual GPU, transfer/pinning and resource qualification. Scoped [GTX 1070 evidence](../../engine/docs/CUDA-LOCAL-VALIDATION.md) is not blanket GPU acceptance. Mixed-precision repair, complete operation/hardware matrix and [Docker release](../../docs/onboarding/AGENT-START.md#intended-local-study-and-release) remain separately scoped. |
| Held/deferred | #111 and #123 remain held; draft #75 remains deferred. They do not block separately accepted core work. Analyst functions are conceptually designed within the overall architecture; their implementation structure and calibration await engine results on real data structures. Dynamic LoD selection and process-failure hosting remain unbuilt. |

Detailed milestone pins and their historical limits are preserved in the
[2026-11-05 acceptance snapshot](../../docs/history/agent-acceptance-2026-11-05.md)
and [earlier history](../../docs/history/acceptance-through-201.md).
Do not copy historical “next” instructions back into this table.

<a id="active-and-deferred-work--2026-10-19"></a>
<a id="active-and-deferred-work--2026-10-20"></a>

## Active and deferred work

**Immediate work:** node/list/ambient exports under #303 review 5880964608, remaining metadata/alias resolution, then trusted borrowed accounting. Preserve refusal/retry, reclamation and native authority.

- Continue #156 through the accepted #170 stages. #162's bounded foundation and
  #139's original A–D sequence are complete; their closures do not close #156/#145. Held #123 must not
  defer required preservation of existing live state during insertion.
- Use retained letters as current test objects. Broader linguistic coverage and
  word-level seeding require their own accepted data contracts; do not infer
  production allocations or new writes from an onboarding summary.
- #134 tracks exact range-compressed reverse child navigation; it is not built
  and does not change the accepted row-based seed writer. Common words retain
  complete cold references even when traversal value is low.
- #138 tracks endpoint subclassification and digit/operator placement separately.
- Dynamic LoD selection, slopes/transitions, initial test granularities and
  private/temp root subdivisions await discussion. Do not invent policies.


