# Private exact coordinate rebasing

This is the D6 continuation of provisionally accepted #211's explicit focus
selection. `Harness::rebase_current` changes the operative coordinate frame in
the existing prepared execution. It does not change focal selection or initialize
objects. The boundary is private; operational owner/browser wiring remains later.

For requested translation `d`, each retained coordinate becomes `x - d` and the
double frame reference becomes `anchor + d`. Only the installed wide profile with
52 retained fraction bits for bodies, fields and every hierarchy node is supported
in this increment. Legacy f32 and reduced-significand transforms return Unsupported
before mutation. This does not complete those D6 profiles or qualify GPU execution.

## Exactness and numerical range

The native validation kernel checks every present body position, current field
centre, previous field centre, both hierarchy publication slots, hierarchy reader
history, focal origin and fallback origin. A strict error-free TwoDiff residual
must be zero for every subtraction. The host double reference addition uses the
corresponding TwoSum residual. Round-trip equality alone is insufficient: it can
miss a small translation lost against a large coordinate.

The runtime enables flush-to-zero, including on CPU. For a nonzero translated
axis, every nonzero input coordinate and translation must have magnitude at least
`2^-969`. Their least represented bit is then at least `2^-1021`, so a nonzero
exactness residual cannot silently flush into zero. Outputs must be finite normal
values or zero. Within a nonzero transform, zero-shift axes still validate finite
normal-or-zero coordinates; an all-zero request is NoChange without a native pass.
This conservative gate rejects some mathematically exact small transforms; it
does not change global floating-point mode or emulate arbitrary precision.

Within the supported range an accepted translation has zero coordinate error:
all translated stored values and the double reference are exactly representable.
The transform preserves the real physical arrangement and relative displacements.
Later arithmetic retains its ordinary profile semantics; this is not a claim of
bitwise cross-backend trajectories or unlimited translation range.

## Transaction and preservation

Validation and publication are separately compiled native kernels with fixed,
warmed bindings. Validation writes only a reserved status scalar. Publication
writes coordinate columns only, behind its enable guard. The same strict
subtraction is used in both kernels under exclusive execution ownership, with no
intervening numerical mutation. This avoids compiler conversion of a combined
check/commit kernel into stores of unchanged live values during preflight.

Traversal scans the bounded physical particle/field capacities and checks native
presence; unused records are not translated. Hierarchy owners map each present
field to its retained packets. This is a structural operation, not a tick scheduler
or a scan of potential capacity. CascadeWork charges 64 times `(P + G + 1)` plus
four times installed hierarchy references, covering both coordinate passes,
metadata validation and source-generation retagging. No new record/reference
claims or contact unions are built. Both kernel contexts are reserved at setup.

Velocities, forces, radii, masses, displacement vectors, transient reduction
vectors, selected fields, presence/activity bits, selectors, counters, source
ledgers and contact indexes are retained. Previous centres translate; they are
not displacement vectors. Dirty history is not consumed, recomputed or reseeded.
The radial-relative arrangement is unchanged and causes no new wake.

Ordinary stale/unsupported/numerical/work rejection leaves the old execution and
observation tokens healthy. An all-zero translation is NoChange. Successful
publication updates the frame reference and fallback control together, retags
hierarchy source references, advances one generation without advancing a tick,
and invalidates old geometry observations. The last completed tick report is not
relabelled as a new tick. Unexpected failure after coordinate writes begins makes
the execution unready; rollback is not claimed.

## Qualification

`live_hierarchy_test --case coordinate_rebase` exercises 70/128 capacities, holes,
sleepers, retained explicit contacts, dirty hierarchy history and current native
values. It checks a `2^27` translation and inverse, unchanged motion/activity,
both packet slots and reader history, exact frame/focal translation, typed
rejections, prior observation validity, fixed backing, no observed recurring C++
allocation and postpublication unready failures. A translated-frame tick is
compared with a separate unrebased execution at positive dt, with an assertion
that a body actually moves. Reduced profiles are explicitly
rejected. The case is also included in the placement/admission suite.

The emitted compiler audit checks the two new kernels' actual frozen bindings,
writer ownership, strict arithmetic and publication guard. Unsafe missing-output,
contracted-arithmetic and removed-publication-guard controls must fail; legal
repeated writes remain accepted. Source-pinned results and untested profiles are
recorded in [admission evidence](../tests/admission/README.md).

The combined placement test runner reserves 2,048 native SNode IDs because its
many independent fixtures share a Program whose issued IDs are monotonic. This
test-only reservation does not enlarge production model capacity.
