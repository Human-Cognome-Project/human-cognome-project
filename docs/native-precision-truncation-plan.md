# Native precision and positional significand truncation

Status: **proposed bounded plan for review, 2026-10-23**.
Owner: [#172](https://github.com/Human-Cognome-Project/human-cognome-project/issues/172).
Baseline: `main` at `c6b85de0d513dcc8d60fa38bdcfb8d1640c07e66` (#171).
The current [direction notes in PR #174](https://github.com/Human-Cognome-Project/human-cognome-project/blob/fc4b04d5bb5a491e21f51f14275ea9d12c5c6a10/docs/precision-exposure-and-concept-catalogue.md)
are an unmerged documentation dependency. #175 separately supplies the reviewed
current-geometry query; it is not part of this plan's baseline or deliverable.

Patrick's clarification in this discussion is the governing intent: use f64
calculation where its specificity benefits the result, reduce calculation width
for less critical work, and independently truncate terminal significand bits
to suppress insignificant positional variation. This plan proposes the precise
initial truncation behavior below; earlier notes leaving the rounding rule open
do not override this later request. Retained bit counts and per-LoD settings
remain calibration choices.

## First deliverable and release boundary

The first implementation PR supplies **native numerical primitives and f64
transport qualification**, with a source audit of the later live conversion.
It must demonstrate genuinely wider calculation reaching an f64 stored result
and configurable truncation through the actual C++/Taichi boundary. It changes
no running field/tick policy, recipe, database, placement rule or native activity
semantics. This is one prerequisite of #172, not completion of the issue.

Acceptance of this plan releases that first implementation slice. A subsequent
bounded integration PR must resolve the concrete field-state contracts below
before enabling the new policy in live ticks. Preserve the existing engine and
legacy-profile evidence; do not add a second solver, analyst, scheduler or an
unused whole-engine precision selector in the first slice.

## Independent numerical choices

| Choice | Meaning and proposed first support |
|---|---|
| Calculation format | Explicit f32 or f64 operands, constants, intermediates and reduction result. Later integration selects this by calculation purpose at setup, allowing higher precision for sensitive work and lower precision for less critical work. |
| Stored container | Explicit native f32 or f64, with matching host element type, ndarray dtype, ABI width, transfer/readback and byte accounting. |
| Retained fraction bits | Keep b of the container's encoded fraction bits: b in [0,23] for f32 or [0,52] for f64. The normal leading significand bit is implicit and not counted in b. |

The first slice implements a small policy/value boundary used by native test
kernels and the reusable quantizer. Future field selection must distinguish
combined force/reduction, dynamics, geometry/contact and reporting calculations;
which may safely use less precision is an empirical question. Do not infer a
fixed calculation width from conceptual LoD or a universal width-reduction ladder.
Secondary compression remains 1:1. No automatic per-particle profile selection
or precision change during a hot stream is introduced.

Required qualification includes f32→f32 and f64→f64, plus explicitly requested
mixed calculation/storage pairs. f32 calculation stored in f64 does not recover
lost arithmetic detail. f64→f32 is a declared narrowing conversion, never the
default step before significand trimming. The f64→f64 route must retain detail
that an intermediate f32 cast would destroy. Keeping fewer bits inside a native
container does not reduce its allocated/transfer bytes or imply faster arithmetic.

## Proposed positional quantization contract

For the selected stored container with m encoded fraction bits, define Q_b:

1. Combine the relevant effects at the selected calculation precision. Promote
   inputs **before** wider arithmetic; widening an already rounded sum is too late.
   Use correctly typed constants and strict math so compiler reassociation cannot
   silently change the declared calculation boundary.
2. Convert to the explicitly selected stored container, if different. Specify and
   test native round-to-nearest, ties-to-even for this conversion; reject a profile
   whose backend cannot honor it. Range/finite checks apply before publication.
3. Bit-cast the finite container value to its equal-width unsigned integer, clear
   its lowest m-b fraction bits, then bit-cast back. Preserve sign and exponent.
   Q_m is identity; Q_0 removes all encoded fraction bits. This is truncation
   toward zero in magnitude, not round-to-nearest at the trimming step.
4. Publish only that result. Return validity alongside the candidate value; NaN,
   infinity, unsupported formats/counts and nonfinite conversion results must not
   become publishable coordinates. No saturation, epsilon, dither or hysteresis
   is added implicitly.

Use the fork's existing `create_bit_cast`, typed casts and bitwise IR operations
before considering new runtime/compiler machinery. Mask construction is validated
at setup, including the no-trimming and maximum-trimming cases; avoid undefined
integer shifts or aliasing through C++ pointers. The reusable helper returns no
positional remainder. Integers, handles, flags and counters retain their own types.

For a finite normal value with unbiased exponent e, a truncation cell has spacing
`Delta = 2^(e-b)`, and `abs(x-Q_b(x)) < Delta`. Subnormals have fixed spacing
`2^(emin-b)`; their implicit leading bit is absent. Keep the selected container's
native exponent range and preserve signed zero. Subnormal masking can produce
signed zero. Detect backend flushing/unsupported behavior rather than claiming
IEEE boundary results from an incompatible execution mode.

For example, f32 with b=21 clears two terminal fraction bits while retaining a
four-byte container and the f32 exponent range. The same independent control can
clear two of f64's 52 fraction bits while retaining an eight-byte container.
Neither example is a selected production setting or a custom native “f30” format.

Truncation filters changes that remain in one cell; arbitrarily small changes
across a cell boundary can still alter the stored value. It introduces a bias
toward zero and spacing changes at exponent boundaries. Test and report these
properties; do not assert universal jitter elimination or add an unreviewed
stability controller to hide them. Quantize the combined positional result,
not each incoming field contribution independently.

## Source audit and downstream conversion obligations

The following are checked source facts at the baseline, not new implementation:

| Surface | Current behavior | Obligation before the live conversion |
|---|---|---|
| [PackedView](../engine/src/engine/packed_view.h) | Accepts float/int elements, four bytes each. | Add qualified double transport without changing integer widths or weakening lifetime, alignment, rank/shape, generation and overflow checks. |
| [Runtime parameters](../engine/src/engine/runtime.h), [transfers](../engine/src/engine/transfer.cpp) | Param already carries dtype; argument_float accepts double; transfers use actual ndarray element bytes. | Exercise existing f64/prepared paths first. Avoid duplicate scalar setters or transport infrastructure where current native functions suffice. |
| [Field kernels](../engine/src/field/field.cpp) | Body/group payload, parameters and most arithmetic are f32. Integration stores native-rounded next position and its accepted displacement. | Trace loads, literal types, products, force accumulation, brake/damping, integration and narrowing. Select precision per calculation purpose and identify every conversion. Preserve formulas. |
| [Centroid finalizer](../engine/src/field/centroid_math.h) | Widens final addition/division, then explicitly narrows centre and weighted rounding remainder to f32. | A wider stored profile must not retain that unconditional narrowing. Discarding positional remainder requires coherent reductions from accepted body positions. |
| [Activity](../engine/src/field/activity.cpp) | Three body delta components; six group delta/history components. Group pending offsets combine member movement with carried finalization remainder. | Separate physical displacement, positional rounding residue and previous-centre history by actual read/write sites. They cannot be cleared together. |
| [Prepared updater](../engine/src/field/prepared_update.cpp) | Uses the same finalizer and publishes repaired state/wake effects. | Future tick and structural repair must agree on precision, discarded detail, current membership and unchanged survivor state. |
| [Capacity](../engine/src/field/prepared_capacity.cpp), [storage](../engine/src/field/prepared_storage.cpp) | Numerical shapes/default element bytes and many scratch outputs are explicitly f32. | Inventory real typed host/backend payloads and bindings; derive each byte term. Do not double all budgets: integer maps, ledgers and flags do not become f64. |
| [Study values](../engine/src/assembly/study.h), host reports and the #175 query | Geometry/source and existing exposed numerical values include f32 boundaries. | List precision already lost at source separately from runtime loss. Future f64 readback, focal/contact arithmetic and reporting must not narrow silently. |

The first implementation retains a source-pinned table of the exact state owners,
read/write sites, widths and conversion points, including kernel constants,
reduction order and prepared scalar caches. It inventories frame/focal/universal
values, candidate/update outputs, reports and optional capture boundaries as well
as the primary body array. It records actual native capability checks: the fork
already advertises [data64](../engine/taichi/taichi/program/extension.cpp), but
advertisement does not prove device accuracy or throughput. No blanket
float-to-double substitution is authorized by this plan.

### State that must remain distinct

- **Discarded positional rounding detail:** the difference between candidate
  calculated position and the chosen published coordinate. It is not a future
  motion input, hidden compensation term or accumulated threshold countdown.
- **Physical velocity and force:** continue to obey the existing model. An
  unchanged quantized position is not permission to zero velocity or invent a
  zero-speed sleep rule. The future conversion must decide activity from coherent
  model state; the current Moving/continued predicates cannot simply be relabelled.
- **Accepted body displacement:** published new minus published old coordinates,
  in the declared calculation/frame convention. This is the displacement relevant
  to centroid membership accounting, rather than the discarded trial displacement.
- **Centroid mass, physical member contributions and previous published centres:**
  retain coherent inclusive mass/shares and next-phase wake comparisons. Current
  group_delta[0..2] is mixed-purpose state; group_delta[3..5] is previous-centre
  history. A blanket buffer clear is incorrect.

A concrete route for the integration review is a fresh, ordered reduction of
each affected group's current accepted member positions, including sleepers,
around a stable reference. It can avoid a carried positional remainder without
losing current membership/mass or reintroducing unpublished body detail. Measure
its cost and compare with a proven incremental alternative before selecting the
live implementation. The first slice records this choice as **unresolved** and
does not implement a new centroid algorithm or merely erase the existing carry.

Retain relative coordinates. Truncation spacing depends on magnitude, and an
exact native translation need not preserve a quantization grid. #170's D6 and
old bitwise residue-survival assertions must be reconciled for the new profile:
prove a supported frame transform or reject it before mutation. Do not claim
general rebase support from arithmetic cast tests. Existing wake masks and type
phases remain the only scheduler throughout the eventual conversion.

## First implementation sequence and acceptance

1. **Typed substrate and audit.** Extend only the necessary C++ packed scalar
   support for double, exercise existing dtype/scalar-cache/transfer facilities,
   and publish the source/state/conversion inventory. No live profile switch.
2. **Native quantizer and calculation probes.** Implement Q_b through the existing
   IR primitives. Run kernels producing combined results in explicit f32/f64,
   then store quantized candidates in their selected native arrays. Prepare and
   warm bindings before repeated dispatch; do not alter Program-wide default
   floating type or rely on implicit promotion.
3. **Evidence and integration handoff.** Retain CPU boundary results, typed array
   sizes, transfer bytes, measured native/C++ allocation observations and focused
   costs. Reconcile #172's remaining live-conversion requirements and #170's
   affected contracts in the follow-up proposal. Do not close either umbrella.

Required independent native tests, with expected values established outside the
new bit-mask implementation:

- Exact powers of two and immediate neighbors on both signs; values within a
  truncation cell and on either side of its boundary; all legal retained-bit
  counts, endpoints, signed zero, smallest/largest subnormals, smallest normals
  and largest finite values. Check sign/exponent preservation, idempotence and
  magnitude/error bounds against a separate arithmetic/known-bit-vector oracle.
- Reject invalid settings and nonfinite inputs/results. Check explicitly requested
  mixed-container conversion boundaries, overflow and gradual underflow behavior.
- Show useful f64 detail survives arithmetic, storage, prepared scalar arguments,
  BulkCopy and CPU direct borrowing: for example `(2^25 + 1) - 2^25` distinguishes
  f64 evaluation from f32 evaluation or a cast made after the rounded sum. Include
  an f64 stored-position increment below an f32 ULP, retained with sufficient b.
  Supply runtime ndarray/scalar inputs so constant folding cannot stand in for
  wider kernel arithmetic. Include one kernel with a sensitive f64 result and an
  independent f32 diagnostic to qualify selection by calculation purpose; record
  its types and error without assuming a performance benefit.
- Combine individually subthreshold contributions before final truncation: they
  can jointly cross a boundary. Include cancellation/order sensitivity and both
  positive and negative noise around interior and boundary positions. Characterize
  oscillation/bias; do not make a universal stability pass criterion.
- Use a small publication probe to repeat subthreshold positional trials: the
  discarded difference must not feed the next step. Keep an independent physical
  velocity value unchanged to demonstrate that positional discard and velocity
  policy are separate. This probe is not a substitute for future live tick tests.
- Verify double shape/alignment/overflow checks, exact dtype/byte counts, prepared
  bindings and stable owned/borrowed lifetime. Measure setup separately from first
  use and recurring dispatch/readback; no recurring backing/layout/compilation or
  observed C++ control allocation within the measured prepared probe boundaries.

Run the existing native CPU regressions because the packed boundary is shared.
Keep old profile evidence labelled; it does not validate the unbuilt live policy.
Retain source/backend/compiler pins, actual architecture, configuration, raw results
and precise allocation observation limits. No Python is used for these runtime,
test or audit tools.

CUDA qualification remains separately local under #145, after the intended core
is ready. A CPU-only build must refuse requested CUDA; a device lacking required
f64/bitwise capability must reject the requested profile rather than fall back.
Record actual device precision, subnormal behavior and f64/mixed-width compute,
storage and transfer costs when that gate is run. No GPU performance claim or new
immediate GPU-testing requirement follows from this planning PR.

## Remaining work after this slice

Enable a coherent precision-controlled field/tick/update/contact/focus/report path
only after the residue, centroid and activity conversion is reviewed and tested.
That integration must show sensitive work benefits from f64, test lower precision
where appropriate, apply truncation once at each declared publication boundary,
and prove discarded noise cannot return through another remainder. Calibration
then chooses useful retained bits and calculation classes at expressed LoDs.
Neither a whole-container reduction nor a fixed ladder is predetermined.

The #156 operational matrix, #170 transactional tick/admission and supported
focus/rebase/exchange operations retain their own acceptance. Exposure controls
(#115), concept.db (#173), automatic perspective selection and private-manager
behavior are separate. No store migration, fresh linguistic ingestion, model-law
replacement or automatic analyst is included.
