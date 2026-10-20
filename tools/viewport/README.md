# Offline study viewport

This optional human observation tool makes the native study's data shape
accessible and supports visual pattern recognition and intuitive spatial or
quantitative judgments. Inspect a pattern by navigating around it and selecting
its objects. The page shows captured model evidence; interpretation stays with
the person using it. [Accepted purpose and plan](../../docs/viewport-purpose-and-plan.md).

## Open a capture

Open [index.html](index.html) in a WebGL2-capable desktop browser and choose
**Open snapshot**. Select [examples/letter-study.json](examples/letter-study.json)
or an exported snapshot. All browser code and dependencies are already bundled
locally. Viewing needs no server, npm installation, database access or network.
On a machine whose browser disallows local pages, use a browser/environment
permitted to open the page; this tool does not change browser policies.

- Drag to orbit, right-drag to pan, scroll to zoom. **Frame all** fits the
  filter-matched objects; **Frame selection** fits one selected object.
  **Study focus** centres the camera on the capture-time focal centroid.
  Camera actions never change the study's selected focal fields or LoD.
- Click a sphere or use the searchable object list to inspect an occurrence,
  scoped definition, local position, actual radius, both masses and its explicit
  conceptual/exposure binding. The list also reaches occluded and zero-radius
  objects. Capture metadata has its own scroll area.
- Mass colours use a linear minimum-to-maximum spectrum independently per
  captured conceptual level. Filtering preserves the original ranges and
  legends. Equal masses use the midpoint colour. Unavailable mass is grey and
  excluded from the range; zero mass is a known value.
- Sphere radius comes directly from the model. Missing and zero radii receive
  no substitute sphere and have distinct visible undrawn counts. Colours are
  unlit so the legend and sphere colours have the same mapping. Selection adds
  a wire indicator at the same reported radius.
- Counts distinguish total, filter-matched and drawable objects, missing
  extent and zero radius. Camera clipping/occlusion is separate. There is no
  subsampling or imposed particle-count cap; capacity has only been tested at
  the counts in the evidence below.
- Every file is a static capture. A new valid file replaces the displayed
  generation and clears selection/filters. Invalid or unsupported files show an
  error and leave the previous capture identified. No browser action controls
  the engine, alters its inputs or looks up labels externally.

The two examples contain the same **16 exposed occurrences** at publication
and after eight native CPU ticks: repeated English and French A and Dutch
multi-scalar letter constructions, in aggregate and byte exposures. Other
recipe alternatives and inert capacity slots are not drawn. Every exposed
occurrence remains represented regardless of native activity. The test geometry
is explicitly supplied: centred spheres with node-local radius 0.25 for Dutch
aggregates, 0.125 otherwise. The native preparation performs its existing unit
conversion; the viewer uses the resulting radii. The example establishes real
report integration, not a claim about natural language coherence or settling.

## Snapshot contract

[snapshot.schema.json](snapshot.schema.json) is JSON Schema 2020-12, version 1.
It records one session/study/generation/tick, phase, separate double reference,
capture-time focal fields/target, explicit conceptual bindings, particles and
supplied source metadata. Exact IDs, tick and uint64 structural masses remain
strings. Numerical mass is the reported f32 value used for colouring; exact
structural mass is separately available for inspection. Optional supplied
display labels are text, including any UTF-8 and escaped control characters.
Radius and numerical mass can be explicitly `null` for unavailable UI data;
the native exporter produces finite, known values for its exposed particles.

The browser also checks unique occurrence IDs/slots, study consistency, binding
ancestry and levels, uint64 limits, tick/phase consistency and the focal target.
Particle position/radius/mass values must preserve native f32 representation;
unsupported numerical values are refused instead of silently narrowing them.
Semantic checks complement the machine-readable schema.

Positions stay reference-local throughout rendering. The double reference is
metadata and is never added to low-precision render positions. The camera target
is the participating-mass weighted centroid of selected fields **in this same
report**, including at tick zero. With no selected fields the named target is
the engine's legacy origin, expressed as negative reference. Last-tick
`demand.focal_origin` is not exported as current focus. Explicit primary roles
advance conceptual level; compression/naming roles inherit it at 1:1. No raw
SNode depth or unit-factor logarithm determines level.

`native/snapshot.h` exposes a value writer and an owner-checked capture entry
point. The trusted synchronous caller binds `CaptureContext` to its submitted
assembly and exact execution handle, owns the successful completed-tick count,
and calls between native operations. Refresh/restart requires a new context
and count. No concurrent snapshot acquisition or automatic engine polling is
implemented. The writer reads current downloaded scalar positions, not the
publication-time positions retained in prepared metadata. Private/local report
data stays with its active owner; no report is published through the manager.

## Build and reproduce native capture

Use the repository's [native toolchain/runtime setup](../../engine/README.md).
The exporter is **off by default**. PostgreSQL >=16 development libraries are
needed only for the retained-data capture client.

```sh
cmake -S engine -B engine/build -DHCP_WITH_VIEWPORT=ON -DHCP_WITH_WARM_INTEGRATION=ON
cmake --build engine/build --target viewport_snapshot_test viewport_letter_capture -j 2
ctest --test-dir engine/build -R '^viewport_snapshot_test$' --output-on-failure
. engine/build/engine-env.sh
```

The capture client takes one output directory and connection settings through
`HCP_WARM_TEST_CONNINFO`, `HCP_CORE_TEST_CONNINFO` and
`HCP_LING_TEST_CONNINFO`. Supply a **fresh empty warm DB with
[schema.sql](../../kernels/database/warm/schema.sql) installed** and the retained,
validated [paired cold sources](../../data/postgres/snapshots/2026-10-13-letter-seed-validation/MANIFEST.md).
The embedding owner attests those source export hashes; the client does not
infer provenance from a DB name. Its `ColdReader` connections enforce read-only
sessions. It publishes a supplied public recipe, acquires an owned warm pin,
assembles a native study, captures ticks 0/8, then releases its own pin.

```sh
engine/build/viewport/viewport_letter_capture /tmp/hcp-view-capture
```

For an existing owned Linux PostgreSQL Docker container, this helper packages
the matched executable/runtime and creates a new warm fixture itself:

```sh
bash tools/viewport/run-capture.sh engine/build OWNED_CONTAINER RETAINED_CORE_DB RETAINED_LING_DB /tmp/hcp-view-capture
```

The helper retains that warm fixture, verifies complete cold data-only hashes
before/after even on failure, and writes captures plus evidence to the output
directory. It does not reset/drop/truncate a retained store. Run the helper
with the build's shared libraries on the loader path, as with the existing
warm acceptance helpers. New captures have fresh UUIDs; repeated generation
instructions reproduce the bounded study rather than identical file bytes.

## Rebuild and validate the browser

Node 24 and the locked dependencies are build/test tools only:

```sh
cd tools/viewport
npm ci --ignore-scripts --no-audit --no-fund
npm run build
npm test
npm run test:browser
```

The browser test uses `HCP_CHROMIUM`, system `/usr/bin/chromium`, or Playwright's
pinned browser, in that order. Install the latter with
`npx playwright install --with-deps chromium` if needed. It opens the file URL
in an offline context and selects the real native JSON through the file input.
Optional positional arguments select another capture and results directory.

Managed cloud Chromium here blocks `file://` navigation. The executed cloud
check used `HCP_VIEWPORT_IN_MEMORY=1 npm run test:browser`: it supplied the exact
retained HTML/CSS/bundle as an in-memory page and exercised the same file picker
with networking disabled. **Direct-file navigation is unverified in this cloud environment**;
the default browser test and the separate viewport CI workflow exercise it
where allowed. No browser policy was changed.

The browser checks real instance positions/radii/colours, stable legends,
filter counts, camera invariance, object-list and sphere selection, generation
replacement, malformed/unsupported files and synthetic unavailable/zero/large
integer/escaped-label cases. Synthetic cases establish UI behavior only.

## Recorded evidence and dependencies

- [Native export measurements](evidence/native-evidence.json): 16 particles,
  tick 8 snapshot 9,579 bytes; report download 0.048 ms and export plus file
  write 0.445 ms in this small run. Tick-zero capture is recorded separately.
- [Browser evidence](evidence/browser-evidence.json) and
  [capture screenshot](evidence/letter-study.png): offline, real native capture,
  software WebGL2 in Chromium 154; 73 assertions, no network requests or page
  exceptions. Parse/validation, scene-build and render-submission costs are
  recorded separately. Browser render submission is not GPU execution time.
- [Before](evidence/cold-before.sha256) / [after](evidence/cold-after.sha256)
  complete cold-store hashes match. The retained warm fixture and native test
  status are recorded in [acceptance.json](evidence/acceptance.json).

These are single-run observations on the cloud CPU, not performance guarantees.
Native CPU integration and software browser WebGL are distinct from unexecuted
Taichi GPU execution/pinning work under #145. Fields/edges, activity overlays,
refresh/timeline, dynamic LoD and engine controls remain outside this tool.
Engine operation and tests do not require the page or the optional target.

Three.js 0.180.0/OrbitControls and Ajv 8.17.1 are bundled by esbuild 0.25.10 from
the locked npm inputs. Their MIT licenses are retained in `vendor/` alongside
the generated bundle's license notices. C++ uses the vendored
[nlohmann/json v3.12.0 single header](https://github.com/nlohmann/json/releases/tag/v3.12.0),
SHA256 `aaf127c04cb31c406e5b04a63f1ae89369fccde6d8fa7cdda1ed4f32dfc5de63`,
with its MIT license. The fork's JSON code is unchanged. Browser dependencies
are isolated to this optional tool and do not link into the physics engine.
