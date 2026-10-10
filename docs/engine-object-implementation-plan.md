# Implementation plan: engine support for first-stage data objects

Status: **Implemented and verified on native CPU (2026-10-10).**
Execution evidence: [object-support results](../engine/docs/OBJECT-SUPPORT-RESULTS.md).
Continuation: [first-stage data-object model](first-stage-data-object-model.md).
Scope approved in [PR #124](https://github.com/Human-Cognome-Project/human-cognome-project/pull/124),
merged as `99fa4c03a804d533f7c8601adb1216dd9402c641`.
The [accepted proposal](engine-object-support-corrections.md) and its maintainer
review govern this plan. Tests verify the corrections; after they succeed,
proceed directly to completing first-stage data modeling.

## Execution order

Use a focused implementation branch from the merged main. Make the following
dependency-ordered commits, keeping runtime, payload callers and affected tests
buildable together. Publish one implementation PR with the completed checks.
Separate adversarial agents are optional; no local CI model-routing requirement
is imposed on this contributor.

### 1. Remove positional parent response

Files: `engine/src/field/field.{h,cpp}`, `engine/tests/field_test.cpp`.

- Reduce the edge float payload to proportional participation; remove
  `kOffX/Y/Z` from payload allocation, readers, staging helpers and assertions.
- Force uses the particle centre. Centroid accumulation uses the same centre
  weighted by participating mass. Whole-particle acceleration still uses the
  particle's total mass.
- Retain the existing inverse-square law and its coincidence/single-member
  gates. Keep distinct repeated contributions, their ordinals and exact object
  counts; convert participation to hot numerical values at staging, rather
  than treating a stored float share as archival structural mass.
- Replace `check_offset_pull` with unequal-mass and repeated-parent checks of
  both the published centroid and first force contribution. Migrate the
  CPU/CUDA agreement fixture that currently assigns an offset.

Exit: corrected field suite compiles and passes; no active reader or caller
still depends on the offset payload. This does not remove actual positions of
separately exposed component particles.

### 2. Prepare and carry object extents

Files: new native preparation helpers under `engine/src/assembly/`,
`engine/src/field/field.{h,cpp}`, `engine/CMakeLists.txt`, focused native tests.

- Add per-particle prepared radius to host/device staging and transfers.
  Update field-count-dependent readers and writers together. Mass and radius
  remain independent.
- Implement the accepted enclosure calculation for a supplied object centroid
  and supplied component spheres:
  `radius = max(distance(component centre, object centroid) + component radius)`.
  Use finite geometry and non-negative radii; reject malformed inputs before
  device transfer.
- Object definitions supply component layout. Record the actual layout used
  by first-stage fixtures. The enclosure calculation does not choose packing,
  a movable minimum-sphere centre or exact single/double/triple/quadruple sizes.
- Reuse prepared extents within an unchanged study. Recompute during preparation
  when component layout, composition or operative scale changes; do not add
  a recursive per-tick geometry walk.
- Check enclosure of supplied spheres, translation invariance, unequal radii
  and consistent position/radius conversion. Component preparation coordinates
  never become internal force-application offsets.

Exit: numerical arrays carry validated prepared radii, and the enclosure
helper passes the focused geometry checks for declared inputs.

### 3. Replace corona/hold regimes with ordinary body contact

Depends on steps 1–2. Files: `engine/src/field/field.{h,cpp}` and field tests.

- Retain explicit ledgered contact-pair coverage. Reduce pair data to endpoint
  indices, removing the defined flag, alignment float and now-unused float
  array. Update constructors/accessors and index-validation tests together.
- Remove corona and hold parameters and their kernel/launcher arguments.
  Retain a clearly named finite body-contact stiffness.
- At nonzero centre separation `d`, use contact distance `r_a + r_b` and
  penetration magnitude `stiffness * max(r_a + r_b - d, 0)`, applied equally
  and oppositely along the existing pair direction. Contact supplies no
  attraction above the contact distance; field participation supplies cohesion.
- Preserve and document the exact-coincidence behavior: the existing direction
  gate produces finite zero contact force at coincident centres. Do not claim
  it separates coincident centres or guarantees hard non-overlap.
- Replace `check_corona` and the defined-hold assertions with unequal-radius
  checks below, at and above the summed-radius boundary; verify absence of
  standoff/hold force above contact, equal/opposite force and finite coincidence.

Exit: contact behavior and payload contain no corona, alignment or special
shear-hold mechanism. No all-pairs collision pass is introduced.

### 4. Assemble and publish the first fixed study

Depends on steps 1–3. Files: native `engine/src/assembly/` preparation/staging
helpers, field publication entry points and native assembly tests.

- Keep explicit definition identity, occurrence identity, particle slot,
  parent value and ordinal in host preparation records. Repeated definitions
  instantiate distinct mutable state. Group parent-value participation across
  ordinal positions; do not key that field by `(parent value, ordinal)`.
- Keep the designated primary SNode study structure and its scale mapping
  explicit in preparation. Cache the resulting operative values for the hot
  arrays; raw SNode depth and schema id do not determine conceptual scale.
- Prepare a fixed initial study using factor 64 for an expressed primary
  transition. Secondary compression and naming equivalences inherit 1:1.
  Put positions, radii and centroids in one operative frame; keep structural
  mass and cold identity unchanged. This does not implement general zoom or
  claim equivalent trajectories across an unspecified time/unit conversion.
- Select aggregate or exposed-part participation once, with no duplicate
  mass/field contribution. Preserve exact repeated parent occurrences.
- Support inert mass-zero free slots if used by this preallocated study:
  no interaction references a free slot, state remains zero, and integration
  guards both particle mass and an all-free total-mass case before division.
  Handle empty optional arrays with valid storage/transfer sizes and unchanged
  logical loop counts.
- Publication sequence: synchronize prior work, validate and stage the target,
  upload, seed its centroids, then expose it as ready to tick. A failed
  publication stays non-runnable and reports its error. Do not infer atomic
  multi-array transfer or implement general recomposition rollback.
- Verify the first force evaluation after initial publication and a supported
  relationship refresh against the freshly seeded target centroids. Verify
  cross-position parent fields, independent repeated occurrences, inherited
  compression scale and finite inert free slots in the same native path.

Exit: the engine can accept the fixed-study object records required for this
stage without depending on #123's unresolved general recomposition machinery.
No database loader, analyst behavior or persistent data write is added.

### 5. Integrate, verify and publish the implementation

Files: `engine/CMakeLists.txt`, `.github/workflows/native-engine-cpu.yml`,
current engine documentation, and files above.

- Register the preparation/assembly regressions with CTest and the explicit
  native CI test filter, so the new tests actually execute in hosted checks.
- Keep existing integration/braking behavior unless a correction exposes a
  specific failure. Diagnose that failure and make the smallest justified
  correction; report any changed setting and its effect.
- Preserve total-mass accounting used by radial damping. Removing dormant
  universal-attraction code is optional cleanup, not a new force requirement.
- Update current payload, contact and assembly documentation. Preserve recovered
  historical records with their status labels. State that PR #75's deferred
  probe must migrate to the new payload when resumed; do not modify that PR.
- Record native results, first-stage layout, masses, radii, participation,
  scale, timestep, gravity and contact stiffness in the implementation PR.

Use the prepared cloud toolchain from the repository root:

```sh
. /workspace/.hcp-setup/env.sh
cmake -S engine -B engine/build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=/workspace/.hcp-setup/sysroot/usr/lib/llvm-15/bin/clang++ \
  -DENGINE_LLVM_CONFIG=/workspace/.hcp-setup/sysroot/usr/lib/llvm-15/bin/llvm-config \
  -DENGINE_TAICHI_CUDA=OFF
cmake --build engine/build -j 4
ctest --test-dir engine/build --output-on-failure
. ./engine/build/engine-env.sh
./engine/build/engine_devices --cpu-only
```

Reuse the matching Taichi build: these changes target HCP's wrapper/model,
not its compiler substrate. The existing smoke, field and >2^31 index tests
must still execute alongside the new focused suites. CUDA checks run where
available; unavailable hardware is reported without delaying the CPU milestone.
Do not suppress failed checks, invent a new force law or add activation work
to manufacture success. Repository-wide unrelated suites are not a new gate.

### 6. Handoff directly to first-stage data modeling

When the corrected native implementation checks pass, publish their evidence
and proceed to finishing the first-stage data-object definitions/modeling and
the separately scoped insertion work. Carry forward the actual prepared layout,
extent calculation, identity/occurrence records and parent relationships.
No additional general physics-validation project is inserted at this boundary.
DECLARE/#111 corrections and persistent insertion permissions retain their
separate scope; successful engine tests do not authorize an implicit data write.
