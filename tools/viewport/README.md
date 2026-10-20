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
- Every opened file is a static capture, labelled **STATIC**. A new valid file
  replaces the displayed generation and clears selection/filters (and ends any
  folder watch). Invalid or unsupported files show an error and leave the
  previous capture identified. A watched folder is the only way captures change
  without a new file choice; see below. No browser action controls
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

### Watch a running capture

**Proposed in this PR, not yet accepted.** A running native capture can write
each coherent capture, atomically, as `latest.json` in its output directory
every K ticks (K is the native tick frequency). Choose **Watch folder** and
select that directory to see each new capture as it appears, while keeping the
same spatial frame to watch whether a coherent pattern forms.

- **Browser.** Needs the File System Access API, so a Chromium-based desktop
  browser. The page asks for **read-only** access to the one folder you choose,
  for that tab only; Chromium refuses a few system folders, so pick a dedicated
  capture directory. Each check looks `latest.json` up by name again, so
  a replacement file written by rename is always read whole (a read that races
  a replacement is retried at the next check, and reported only after four consecutive failures). No server or
  network is involved and the page cannot write to the folder. Where the API
  is absent (for example Firefox), the control is disabled with an explanation;
  **Open snapshot** still works and no browser policy is changed.
  Observed locally: on a direct `file://` page in Chromium 145 (Playwright
  `chromium-1208`) `showDirectoryPicker` is a function and the page is a
  secure context. The browser test records this in its results (recorded, not
  asserted), because the pinned Playwright in CI uses `chromium-1194` and the
  API's presence depends on the build; the control follows whatever the
  browser reports. The retained evidence file below predates watch mode. The folder picker
  itself needs a person, so the test drives watching through a page-side
  source with the same `getFileHandle()`/`getFile()` shape. An origin-private
  directory handle was not usable from `file://` (`SecurityError`), so a real
  directory handle remains unverified by automation.
- **Update frequency.** **Check every (ms)** sets only how often the viewer
  looks, default 1000, minimum 100, remembered locally when the browser permits.
  It is independent of simulation tick rate and of K. A file whose modified time
  and size are unchanged is not read or parsed again, and a capture with the
  same session, study, generation and tick is not re-applied. Change detection
  uses the file's modification time and size, so on filesystems with coarse
  timestamps (for example FAT/exFAT or some NFS) two equal-length captures
  written within that granularity can show as one. **Pause** stops
  checking and applying (**Resume** catches up to the newest file) and
  **Stop watching** ends the watch, leaving the last capture as a static display.
- **Status.** The tag beside Capture reads STATIC for opened files, and LIVE,
  PAUSED, WAITING (no `latest.json` yet, still polling), ERROR or NO CHANGE
  while watching. NO CHANGE is only a display hint: no new capture for more than
  ten checks (at least ten seconds). It does not mean the engine has stopped;
  the native capture interval may exceed that threshold. The Live updates panel
  and footer show the applied tick, wall time of the last change and time since it.
- **Same study keeps your view.** The capture on display is the baseline. A
  capture with the same session and study updates positions, masses, metadata
  and the inspection panel in place, including when a watch is started or
  restarted (for example after **Open snapshot**) on that study. Camera position
  and target, level/exposure/search filters and the selection are kept; the
  selection is found again by occurrence ID and is cleared only if that ID is
  absent from the new capture or filtered out. A level filter whose level is
  absent from the new capture resets to "All captured levels". Only when
  nothing is displayed, or the capture has a different session or study, does
  it behave exactly like opening a new file: the generation is replaced,
  selection and filters are cleared and the camera frames it.
- **Colour is relative within each update.** Mass ranges and legends are
  computed per capture, as for any opened file, so a sphere's colour compares
  it only with the same level of that capture. Colours can shift between
  updates as the masses change; read the legend values, not the hue alone.
- **Bad or missing files.** If `latest.json` is malformed, unsupported or
  unreadable, the previous capture stays displayed and identified with the error,
  and watching continues; the next valid capture recovers automatically. A
  missing file means the run has not started and shows WAITING.

The viewer controls only its own looking. It does not start, stop or configure
the capture, and the native side does not need the page.

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
cmake --build engine/build --target viewport_snapshot_test viewport_options_test viewport_letter_capture -j 2
ctest --test-dir engine/build -R '^viewport_(snapshot|options)_test$' --output-on-failure
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
assembles a native study, captures tick 0 and the final tick (8 by default),
then releases its own pin.

```sh
engine/build/viewport/viewport_letter_capture /tmp/hcp-view-capture [options]
```

Options are parsed natively; an unknown option or invalid value prints the usage
to stderr and exits 2 before any database or engine work. With none, the
capture is the fixed 8-tick CPU run and file names above.

| Option | Meaning |
|---|---|
| `--backend cpu\|cuda` | Engine backend (default `cpu`). `cuda` needs a matched Taichi build with CUDA, its driver and a visible device. The engine can silently fall back to the CPU when it finds no driver or device, so the client checks the Program's actual architecture right after the session starts and, before any warm write, exits non-zero on a mismatch or a startup error. `native-evidence.json` records the verified backend (`native_cpu` or `native_cuda`), not the request. |
| `--ticks N` | Total completed ticks, N >= 1 (default 8). |
| `--dt X` | Tick length, finite and > 0 (default 0.1). It is the engine parameter (a 32-bit float). The configured value is recorded in `native-evidence.json` as `dt`; the snapshot's existing `source.parameters.dt` carries the engine float. |
| `--cpu-threads N` | CPU worker threads, N >= 1 (default 1). Applies to the `cpu` backend. |
| `--every K` | Also capture every K completed ticks, K >= 1. Absent: tick 0 and the final tick only. |
| `--pace-ms M` | Minimum wall-clock milliseconds between successive captures, M >= 0 (default 0). The client sleeps before a capture if needed so a small study is watchable. The simulation is unchanged: same ticks and `dt`. |

`--ticks` stays finite but may be large. Tick 0 is written as
`letter-study-tick0.json` and the final tick as `letter-study.json`. **With
`--every`**, every capture, including those two, is also written to
`latest.json`, so a watcher can follow one fixed path; without it no
`latest.json` is written and the output files are exactly the fixed set. With
`--every`, the evidence's `export_ms` includes that extra `latest.json` write.
Intermediate `--every` captures are not kept as separate files; each is
replaced by the next. Each file is one coherent capture (one study, generation
and tick) taken between ticks. Files are written to a hidden, uniquely named
temporary in the same directory, closed, then renamed over the target, so a
reader sees the previous or the new complete file and never a partial one. The
rename gives reader atomicity, not crash durability (no `fsync`). One writer per
output directory is the supported use: a concurrent second writer leaves the
final content undefined, though readers still never see a torn file.

`SIGINT` or `SIGTERM` only sets a flag. The warm acquire commits its lease
before it returns, so both signals are blocked from before the acquire until the
pin's release guard exists and the handlers are installed, then unblocked: a
signal that arrived in that window is delivered to the graceful handler. (The
engine's threads are started with the signals blocked and so never receive
them.) A signal during the later engine setup is likewise noted: setup finishes,
the tick-0 capture is written as the final capture and the run exits 0 with the
pin released. A signal before the acquire (including a hung database connection,
and the engine start-up), or a second signal of the same kind, takes the default
action and ends the process immediately without releasing the pin or writing
evidence; so does `SIGKILL`. These forced terminations are a documented
limitation: a lease left that way stays in the warm database. A launcher that
starts the client with `SIGINT` ignored (for example `&` from a non-interactive
shell) makes a setup-phase `SIGINT` a no-op; `SIGTERM` is unaffected. In the
loop, the client finishes the current tick (or ends a pacing sleep early), writes
a final capture with the actual completed tick count to `letter-study.json` (and
`latest.json` with `--every`), releases its warm pin, writes `native-evidence.json`
and exits 0. The pin is also released if the run fails after acquiring it.
The Docker helper copies its output only when the client exits and does not
forward signals; use a finite `--ticks` there.

`native-evidence.json` records the backend verified on the constructed engine
(`native_cpu` or `native_cuda`), `dt`, `cpu_threads`, `every` (null when absent),
`pace_ms`, `requested_ticks`, completed `ticks`, `stopped_by_signal` (the run
ended before the requested ticks) and `capture_count`. To stay bounded in long
runs, `captures` holds only the first and last captures (one entry when there
was only one), each with tick, particles, bytes and report/export milliseconds.
A CUDA run is functional execution only: it is not a GPU performance or pinning
qualification (#145).

For an existing owned Linux PostgreSQL Docker container, this helper packages
the matched executable/runtime and creates a new warm fixture itself:

```sh
bash tools/viewport/run-capture.sh engine/build OWNED_CONTAINER RETAINED_CORE_DB RETAINED_LING_DB /tmp/hcp-view-capture [client options]
```

Any arguments after the output directory pass through to the client. The
container path is CPU-only unless that container itself has GPU access and a
CUDA-enabled build is packaged; otherwise `--backend cuda` fails as above.

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
Watch-mode checks (44 additional, 117 in all) feed the retained tick 0 and 8
captures (115 under `HCP_VIEWPORT_IN_MEMORY=1`, which skips the direct-file and
local-storage checks), and clearly labelled mutated UI fixtures, through a page-side
folder source: same-study camera/filter/selection preservation, updated values
and per-capture colours, invalid/unsupported/missing files with recovery,
unchanged files not re-read, interval and its minimum, pause/resume, stop,
different-study replacement, opening a file during a watch, and the
unsupported-API fallback. They show viewer behaviour, not native evidence.

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
time scrubbing, dynamic LoD and engine controls remain outside this tool.
Engine operation and tests do not require the page or the optional target.

Three.js 0.180.0/OrbitControls and Ajv 8.17.1 are bundled by esbuild 0.25.10 from
the locked npm inputs. Their MIT licenses are retained in `vendor/` alongside
the generated bundle's license notices. C++ uses the vendored
[nlohmann/json v3.12.0 single header](https://github.com/nlohmann/json/releases/tag/v3.12.0),
SHA256 `aaf127c04cb31c406e5b04a63f1ae89369fccde6d8fa7cdda1ed4f32dfc5de63`,
with its MIT license. The fork's JSON code is unchanged. Browser dependencies
are isolated to this optional tool and do not link into the physics engine.
