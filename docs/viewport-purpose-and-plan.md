# Human viewport: purpose and implementation plan

Status: **accepted plan in #154, 2026-10-20; first implementation under review**. This document records Patrick's
purpose clarification and proposes the first implementation of
[#151](https://github.com/Human-Cognome-Project/human-cognome-project/issues/151).
The optional [viewport and exporter](../tools/viewport/README.md) now implement
this first slice, with bounded CPU/browser evidence and direct-file testing
limits recorded there. This document retains the accepted plan below.
The native basis is accepted #149/#150/#153.

## Purpose

The viewport serves two related purposes:

1. **Make the data shape directly accessible.** People should be able to see
   the relationships expressed by the model, connect them to identifiable
   objects and gain a concrete basis for confidence that the correlations
   have observable structure.
2. **Put human visual processing to work.** Human vision is highly developed
   for pattern recognition and intuitive judgments of spatial and quantitative
   relationships. Clustering, separation, relative spacing, symmetry, density
   and outliers can prompt useful observations and hypotheses before a numerical
   summary makes them apparent.

These purposes make observation, exploration and reasoning central to the
design. Troubleshooting is one application. A person should be able to notice
a pattern, navigate around it and inspect the objects and numbers behind it.
The view supplies faithful evidence for that judgment; linguistic coherence
remains a human interpretation rather than a score assigned by the page.

Geometry, colour and camera behavior therefore need consistent meanings.
Changing the camera helps distinguish projected overlap from spatial proximity.
Stable colour ranges support comparisons while filtering. Exact identities and
numerical values connect a visual impression back to the source report.
A single capture shows an arrangement at a stated tick; observation of motion
or settling across time requires additional captures or a later time view.

The tool remains optional, lightweight and removable. Engine operation,
reporting and acceptance remain usable with the viewport absent.

## Existing native basis and capture gaps

The current [active workbench](../engine/docs/ACTIVE-WORKBENCH.md) already
provides an owner-checked `ActiveSession::report(handle, owner)`.
Its [report types](../engine/src/assembly/active_workbench.h) and
[prepared-study types](../engine/src/assembly/study.h) provide:

- Session/study/generation identity, scoped occurrence provenance and exact
  structural mass.
- Current particle positions, numerical masses and actual model radii in the
  downloaded scalar arrays. Prepared positions describe publication-time
  state and must not substitute for current positions.
- Occurrence-to-slot and study-node bindings, explicit scale roles/parents and
  operative unit conversions. Reports clear native SNode pointers.
- A separate double frame reference, local focal origin, typed field
  identities and prepared participation.

The report has no completed-tick counter, standalone exposure labels or display
labels. The proposed capture caller supplies these from its own synchronous
execution loop and the exact submitted recipe. Tick zero means the successfully
published and seeded state; increment only after a successful tick. Capture
metadata is bound to the same execution handle and generation, and is replaced
on refresh/restart. Capture occurs between native operations under that caller's
control. No new scheduler, activity state or reporting overlay is needed.

### Focal origin at capture time

Keep the capture's geometry and last-tick diagnostics distinct.
`ActiveReport::demand.focal_origin` is the origin used by the last tick's
calculation; it is not guaranteed to be the focal centroid after that tick's
particle movement. Demand is cleared on publication, so its zero value at
tick zero must not be treated as a measured focal origin.

For the snapshot's camera target, derive the ordinary participating-mass
weighted centroid of the selected focal groups from the same captured
`groups` arrays and `prepared.focal_groups`. Use their current centroid
positions and masses, not publication-time particle positions or last-tick
demand. With no focal selection, preserve the engine's explicit legacy origin
in local coordinates (negative frame reference), or document a separately
named display framing target. A display target is not a changed study focus.

If last-tick integration origin is exported, label it separately with its
phase; omit it or mark it unavailable before a successful tick. Include
tick-zero and moving-focal-field cases in exporter checks. These are derived
snapshot metadata, not a new engine control or a change to centroid rules.

## First implementation boundary

Use this one-way flow:

```text
owner-checked native report -> C++ exporter -> JSON snapshot -> static browser page
```

Keep implementation files under `tools/viewport/`, with an opt-in native CMake
target linking the existing workbench. The proposed browser dependency is a
pinned Three.js bundle with OrbitControls, built locally into a classic script
and retained with its license and reproducible build instructions. Opening the
page and selecting a local file must work offline without a server, package
installation or CDN access. No application framework is required.

The page receives the chosen file and supplied labels. It has no database
credentials, manager access, live Taichi access or engine-control channel.
Private/local reports stay with their active owner; the exporter does not
publish through the shared manager. A repository example uses public retained
letter data and explicitly supplied test geometry. It excludes private inputs
and connection configuration.

## Proposed snapshot v1

One file contains one coherent study/generation/tick. The implementation PR
will include a machine-readable schema and document the following contract.

| Section | Proposed contents and source |
|---|---|
| Capture | Schema version; session, study and generation strings; completed tick as a decimal string; capture phase; optional export time and supplied study label. |
| Frame | Local coordinate convention and operative units; double frame reference and local focal origin kept separately; existing focal-field keys where present. |
| Scale bindings | Binding ID, parent, explicit role, operative units and a resolved conceptual-level ID. Only explicit primary steps introduce a conceptual level; compression and naming-equivalent bindings inherit it. |
| Particles | Exact occurrence and scoped definition identity, slot, current local position, actual radius, numerical mass, exact structural mass as a decimal string, scale binding, explicit aggregate/parts exposure context and optional supplied labels. |

Export particle records by the prepared exposed occurrence list, reading their
current scalar values by slot. Inert capacity slots and hidden alternatives
remain absent. Sleeping exposed occurrences remain present. Repeated uses of
one definition retain distinct occurrence IDs. Exposure metadata joins the
submitted selection to each occurrence; it is not guessed from a label or mass.
Conceptual levels follow the explicit role/parent chain, never raw storage
depth or a logarithm of the unit multiplier.

Preserve round-trip f32/f64 values and correctly escaped UTF-8 strings. Retain
exact integers beyond JavaScript's safe numeric range as decimal strings.
Render in the reported local frame, using the focal origin as a camera target;
do not add the large double reference into low-precision render positions.
Units are model units, with their supplied conversions, rather than an invented
physical unit such as metres.

The first slice omits field graphics, participation edges and activity/movement
overlays. Existing field-kind distinctions remain intact in any exported focal
metadata. Later field records must keep centroids separate from particles and
retain explicit participation rather than expanding reverse child listings.

## Visual and interaction choices

- Draw spheres at their reported radii, preserving their relative extent and
  spacing. Use restrained shading and a visible mass legend. A missing radius
  is explicitly unavailable and contributes to a visible undrawn count; a
  legitimate zero radius remains zero and is listed as such. Neither receives
  an invented sphere. Negative or non-finite values are invalid.
- Colour by the reported numerical mass within each conceptual level. Show
  exact structural mass separately in inspection so numerical rounding is
  explicit. Use a sequential spectrum with a linear minimum-to-maximum mapping
  computed from the whole captured level, before filtering. Equal masses use
  the spectrum midpoint; unavailable mass uses a distinct neutral colour and
  is excluded from the range. Each level has its own labelled legend. The first
  slice offers no automatic rescaling.
- Provide orbit, pan, zoom, frame-all and frame-selection. These change only
  the display camera. Click inspection shows occurrence/definition identity,
  position, radius, both mass representations, conceptual binding, exposure
  context and supplied labels. A searchable particle list also supports
  selecting obscured or zero-radius objects.
- Filter by captured conceptual level, exposure context and supplied
  identity/label text. Display total, filter-matched and drawn/undrawn counts
  with reasons. Camera visibility is separate from filtering. No silent
  subsampling or undisclosed display cap is proposed. Instancing and ordinary
  camera culling can reduce draw cost without changing particle data.
- Validate a newly chosen file before replacing the displayed capture as a
  whole, clearing prior selection, filters and renderer resources. A failed
  load displays an error and leaves the previous capture clearly identified.
  Show schema/study/generation/tick and identify the view as a static capture.

## Implementation sequence and acceptance

1. **Contract and native export.** Implement the versioned schema, checked
   capture metadata and C++ writer behind the optional target. Verify array
   shape/slot mapping, current-state values, exact identity/mass strings,
   role-based LoD, exposure and numerical round trips. Reject inconsistent
   metadata and unsupported/non-finite data before producing a snapshot.
   Use a JSON writer with proper escaping and precision; the fork's current
   [JSON printer](../engine/taichi/taichi/common/json.cpp) uses default stream
   precision and unescaped strings, so direct reuse does not meet this contract.
   Keep any serialization dependency local
   to the optional tool rather than changing the Taichi fork.
2. **Bounded real letter capture.** Reuse the accepted warm-to-active path to
   create a small mixed-exposure study from retained language endpoints,
   including repeated occurrences and multi-byte construction. Record source
   provenance, supplied geometry, parameters and successful tick count. Read
   retained cold sources without mutation and use a fresh warm fixture if
   needed; retain existing stores. Supply one public snapshot and exact
   reproduction instructions, with the actual exported particle count.
3. **Static observation page.** Implement file validation, instanced spheres,
   camera controls, stable per-level mass colours, legends, inspection and
   filters. Verify direct file opening and offline operation in Chromium.
4. **Integration evidence and review.** Display the real native capture in the
   browser and compare positions, radii, identities and counts to its report.
   Exercise the same contract with synthetic edge cases for equal/unavailable
   mass, unavailable/zero radius, escaping, large exact integers, a large frame
   reference, empty data, malformed data and generation replacement. Check
   that camera/filter actions leave captured model values unchanged and
   legends keep their original ranges. Measure report download separately
   from export time/file size and browser parse/build/render cost; state
   hardware/browser and tested counts. Run affected native tests and confirm
   the existing engine builds/tests with the optional target disabled. Submit
   the implementation in its own focused PR after this plan is reviewed.

The evidence will distinguish native CPU execution from browser WebGL
rendering and from unexecuted Taichi GPU work. The small real capture establishes
report integration; it establishes no million/billion-particle browser capacity.
Visual observations can inform the next data/model discussion without adding
a new requirement to reprove the model mechanics.

## Accepted decisions and deferred work

The snapshot boundary, caller-owned tick/exposure metadata, local bundled
Three.js dependency, numerical-mass colour mapping and bounded real-letter
demonstration are accepted for this first slice. Capture-origin phase handling
below is part of that contract. The two human purposes above are Patrick's stated intent.

Edges/hulls, centroid markers, activity overlays, time scrubbing and
binary transport remain follow-ups. A later centroid marker needs a distinct
symbol without invented physical extent. Any later activity display must state
its observed phase and keep selection, motion and settledness distinct.
Refresh must use coherent captures independently of simulation tick rate.

**Live refresh accepted and merged in #159 on 2026-10-21.** The native capture client
would write each coherent capture (snapshot v1, unchanged) atomically as
`latest.json` every K ticks, K being the native tick frequency. The browser
would watch that folder read-only through the File System Access API at its own
configurable polling interval, independent of K and of simulation tick rate.
Captures within one session and study update in place and keep the camera,
filters and selection by occurrence ID, so the person's spatial frame stays
stable while the model changes, including when watching starts on the study
already displayed; nothing displayed, or a different session/study, replaces
the display as a new file does. Mass colour ranges stay per capture. It adds no
engine control or network, and invalid or missing captures keep the previous one
displayed. See the [watch behaviour](../tools/viewport/README.md#watch-a-running-capture).

Dynamic LoD selection/slopes, analyst behavior, general recomposition, physics
changes, deferred #75 and held #123 are outside this plan. GPU execution and
pinning remain under #145. Warm stays the tool/part box, hot the owner-controlled
assembly workbench, and GPU memory the execution surface; this optional viewport
adds no new required architecture layer.
