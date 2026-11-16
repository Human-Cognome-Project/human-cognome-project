# Origin definition from one complete provenance pin

**Integration preparation — 2026-11-16:** main `1337bbcc` integrates #353.
This branch preserves that ancestry and its historical comparison document,
retaining #354's reviewed observer/provenance tests in place of the old
duplicate-member witness, as instructed in review 5929111772. Production and
test files remain byte-identical to reviewed `eddd4f35` and tested `417aed19`;
the merge adds/reconciles only Markdown. #354 remains source-accepted, awaiting
hosted qualification and integration. Older dependency/status statements below
describe the original implementation source.

Contributor implementation under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343),
2026-11-16, awaiting review. Base: main
`5f45f5669da902072cfb312e43f2153b517f61d8`; public tested source:
`417aed199079050d40db67967e4cf06b06ce13a1`.

This implements the removal accepted in
[#353 review 5928222116](https://github.com/Human-Cognome-Project/human-cognome-project/pull/353#pullrequestreview-5928222116).
The [prior comparison](https://github.com/Human-Cognome-Project/human-cognome-project/blob/9bf2b956e6d9dc21d3bb239a5d31f0ebfa1495fd/engine/docs/ORIGIN-ASSOCIATION-COST.md)
is an inspection/evidence dependency. Its test-only branch remains unmerged;
the implementation starts directly from main, which already integrates #352.
No additional runtime dependency is imported from #353.

## Definition authority and lifetime

`assembly::Origin` no longer stores an independent `ObjectKey definition`
beside the same object in its provenance pin. It retains the exact numeric
occurrence identity and its existing optional shared `ToolRef` or local
`LocalRef`. `definition()` returns a const reference to the object already
owned by exactly one complete pin. Valid reads allocate no C++ storage.

The accessor refuses missing or simultaneous pins. Shared pins require the
shared namespace and a complete nonnil revision. Local pins require complete
nonnil instance scope, owner and version. Both require five complete valid
address elements. UUID checks also refuse moved-out values. This validates
held metadata; it performs no cache lookup, authorization or private-input
admission. Existing owner/session authorization remains authoritative.

All origin producers and readers use this representation together, including
shared and local `ActiveAssembly::origins()`, the live-intent reporting fixture,
viewport labels and snapshot serialization. All alternatives and repeated
occurrences remain in their original numeric order. There is no new registry,
cache, shared owner or pointer into an ActiveAssembly. Detached rows still own
their full shared revision or local owner/version after source and native
teardown. A returned definition reference borrows its Origin value and expires
when that value is destroyed or mutated.

The existing encoded definition in a prepared occurrence remains a transition
boundary. Snapshot serialization checks it against the canonical key from the
complete pin and refuses disagreement; it cannot silently select one key.
Every origin is validated, including hidden alternatives. The old four-member
Origin aggregate is no longer accepted by the C++ interface. A checked legacy
conversion exists only in the test reference and explicitly rejects duplicate
key disagreement; production has no legacy-key fallback.

Sources: [Origin interface](../src/assembly/active_workbench.h),
[accessor and producers](../src/assembly/active_workbench.cpp),
[snapshot/context readers](../../tools/viewport/native/snapshot.cpp).

## Controls

[Provenance controls](../tests/origin_provenance_test.cpp) cover both pin
families, missing/dual pins, incomplete or moved-out identities, private scope
masquerading as shared, malformed addresses, and legacy duplicate mismatch.
They compare complete pins, occurrence order and counts and verify detached
ownership after source teardown. The valid accessor returns the actual owned
pin member without allocation. Producer clear/destruction releases every
observed nested allocation and then the vector backing allocation.

[Snapshot controls](../../tools/viewport/native/snapshot_test.cpp) exercise
the existing native ActiveSession with repeated shared instances, aggregate
and parts exposure, hidden alternatives and a local value. They compare full
JSON with checked legacy provenance values and independently compare every
exposed definition and label against the old key/recipe lookup. They refuse
missing/dual pins in every row and disagreements between retained encoded
definitions and otherwise valid shared/local pins. A complete detached
snapshot remains identical after source/session/Runtime teardown. Existing
snapshot, native intent and checked-owner/observer controls remain exercised.

The full JSON comparison uses the current serializer with old-shaped reference
values, not a separately executed historical serializer. Independent key/label
expectations and existing physical-state checks constrain that comparison.

## Producer allocation comparison

Measurements bracket the actual changed `ActiveAssembly::origins()` call after
fixture preparation. The before control reconstructs the old owning row shape
and unchanged push/growth algorithm from identical seeds prepared outside the
counter window; it is test-only code in
[origin_reference.h](../tests/origin_reference.h). Its four-row shared result
exactly reproduces #353's measured actual old producer. An isolated Clang
build of main `5f45f566` with an added measurement target also executes the
actual historical producer for both fixtures. It confirms every before value
below independently of the reference reconstruction. Its production sources
are unchanged; the added target supplies only the fixture/counters.

Both local compiler builds observed:

| Fixture / producer | Rows / capacity | Allocation requests | Cumulative requested bytes | Retained requested bytes | Scoped peak requested bytes |
| --- | --- | --- | --- | --- | --- |
| Shared, old reference | 4 / 4 | 29 | 4,368 | 2,808 | 3,662 |
| Shared, actual current | 4 / 4 | 21 | 3,544 | 2,248 | 2,978 |
| Reduction | | 8 | 824 | 560 | 684 |
| Shared + local, old reference | 5 / 8 | 37 | 8,743 | 5,103 | 7,183 |
| Shared + local, actual current | 5 / 8 | 27 | 7,163 | 4,139 | 5,867 |
| Reduction | | 10 | 1,580 | 964 | 1,316 |

On this Linux x64 ABI, the row changes from 520 to 432 bytes. Removing the
88-byte in-vector key also removes its separately allocated contents. The
shared retained reduction is 352 bytes of vector element extent plus the
208 nested requested bytes attributed by #353. Capacity growth contributes
to cumulative requests and scoped peak; these categories must not be added
together as independent savings. The local vector has capacity eight for five
rows, so its retained element reduction also includes unused capacity.

Counters observe new/new[] requests and their releases on the measuring thread.
They do not measure arbitrary malloc, allocator rounding, RSS, native/device
storage, process peak, latency or complete B accounting. Source assemblies and
seeds exist before the window. No canonical identity interning is claimed.

Removing the duplicate also makes the existing retained-versus-consuming
observer comparison smaller:

| Compiler / observer path | Allocation requests | Requested bytes |
| --- | --- | --- |
| Clang 15 retained / consuming | 150 / 119 | 8,637 / 5,350 |
| GCC 11 retained / consuming | 156 / 125 | 9,117 / 5,830 |
| Within-build difference, both | 31 | 3,287 |

The historical #352 difference of 39/3,847 remains
correct at its pinned source in [PREPARED-ASSOCIATIONS.md](PREPARED-ASSOCIATIONS.md).
The 8/560 difference is the eliminated redundant deep copy of four origin keys;
it is not an additional independent saving to add to producer results.

## Qualification and next work

Personally executed at public `417aed19`, using the matched CPU-only LLVM 15
fork and exact recursive submodules recorded in the evidence:

- `bash engine/scripts/run_local_ci.sh quick /tmp/hcp-pin-authority-417aed19-quick`:
  PASS. Full native rebuild, actual CPU, three core CTests, 632 independent
  placement interval samples and placement capacity.
- `flock engine/build/.local-ci.lock bash /tmp/hcp-pin-authority-417aed19-focused.sh`:
  all 16 CTests passed in 523.37 seconds. Includes provenance (44 checks),
  builder, all four owner profiles, existing workbench, all four live-intent
  reporting-fixture profiles, snapshot (92 checks) and all four observer
  worker/path profiles. Observer transfer savings agree at ticks 0/1/2.
- `bash /tmp/hcp-pin-authority-417aed19-gcc.sh`: fresh GCC 11 Release wrapper
  on the matched LLVM fork. All five CTests passed in 164.22 seconds:
  provenance, workbench, one-worker bulk owner, snapshot and observer.
  All producer allocation/retained/peak values and within-build transfer
  savings match the Clang measurements.
- `bash /tmp/hcp-pin-authority-417aed19-before-complete.sh`: PASS. A fresh
  isolated Clang build executes the actual old producer from main `5f45f566`
  with the same shared/local fixture values and added allocation counters.
  Both rows in the before comparison match exactly and release fully.

The first historical export omitted `common/reservation_allocator.h` and failed
to compile. It remains preserved in the separate `...-before` evidence
directory; the corrected `...-before-complete` run uses a new export/build.
This export-preparation failure is not relabelled as a successful run.

The implementation checkout stayed clean at `417aed19` through every run,
with source-after checks, commands, configurations, logs, binary hashes and
verified per-file manifests. Historical production sources are exported from
the recorded base; its added measurement source/target are retained in that
export and manifest. The final contribution changes only five Markdown files
after the tested source. Raw evidence directories are
`/tmp/hcp-pin-authority-417aed19-{quick,focused,gcc,before,before-complete}`.
Archive `/tmp/hcp-pin-authority-417aed19.tar.gz` excludes build products;
SHA256 `ec7a487bfb65ca98703428d238ed1d3d43248e9c7c80a3ef7ed3bd73c44d9404`.
This is contributor-local evidence, not remotely inspected raw evidence.
The complete regression/compiler/database matrices, GPU, browser and native
Windows checks were not run locally for this slice.

GitHub CI is authoritative and review remains pending. Main's normal combined
native run and this contribution's hosted run are separate from these local
CPU results; no duplicate workflow dispatch is requested. #353 smoke/viewport
and all 12 native jobs in 43958409039 now report success; that does not integrate
the branch or independently inspect its local archive.

Native claims, candidate/committed state, allocation policy, numerical kernels,
JSON schema and private namespace boundaries are preserved. Local snapshot
coverage uses existing ActiveSession; local prepared admission is not added.
Retained encoded occurrence definitions remain checked rather than removed.
Broader identity/string accounting, direct indexed guards, mixed-revision
catalogue/schema adoption, public producer migration and #324 mathematics
remain separately scoped. Reconcile this removal with the pending #353 witness
when integrating; the old duplicate-member witness must remain historical or
be adapted to the explicit old-shaped reference, not restore the removed key.
