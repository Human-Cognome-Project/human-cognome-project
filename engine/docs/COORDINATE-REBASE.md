# Private exact coordinate rebasing

This is the D6 continuation of provisionally accepted #211's explicit focus
selection. `Harness::rebase_current` changes the operative coordinate frame in
the existing prepared execution. It does not change focal selection or initialize
objects. The boundary is private; operational owner/browser wiring remains later.

For requested translation `d`, each retained coordinate becomes `x - d` and the
double frame reference becomes `anchor + d`. The installed wide f64 profile
supports exact translations at each declared retained precision, from 0 through
52 fraction bits. Legacy f32 remains Unsupported. The original full-precision
increment is provisionally accepted in #212; reduced-significand support is its
following reviewable increment. Neither qualifies rounded transforms or GPU execution.

## Exactness and numerical range

The native validation kernel checks every present body position, current field
centre, previous field centre, both hierarchy publication slots, hierarchy reader
history, focal origin and fallback origin. A strict error-free TwoDiff residual
must be zero for every subtraction. The host double reference addition uses the
corresponding TwoSum residual. Round-trip equality alone is insufficient: it can
miss a small translation lost against a large coordinate.

Exact f64 subtraction is necessary but not sufficient for reduced profiles. The
candidate's bit pattern must also survive its destination's fraction mask
unchanged. Body positions use the body's declared retained bits. Installed fields,
their previous centres and both hierarchy/history slots use the current native
owner-node mask; a field without an owner uses the field profile. These masks are
read through fixed native bindings, including independently configured hierarchy
precision. Focal controls and the frame reference remain full-precision f64.
The operation rejects discarded bits as InvalidNumerics; it never applies rounding
or clears bits during publication to force an otherwise invalid transform to fit.

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
that a body actually moves. The dyadic continuation compares body/motion and
unaffected state exactly, while checking each poll's packet/history lifetime.
Independent concurrent traversals may publish the parent from either complete
old or new child packets. This fixture accepts only its two exact parent values,
requires pending work for the lagging value, and requires exact catch-up on the
next native poll. Forced parent-first and child-first orders demonstrate distinct
valid histories in translated/untranslated frames. Their old packets are retained;
they are not normalized or erased to manufacture equality. Negative controls
reject lost pending work, an intermediate coordinate and incorrect mass. These
are the existing hierarchy semantics, not a rebase-specific relaxation or epsilon.
Reduced cases independently exercise body, field and
hierarchy precision at 8 bits, and a zero-fraction-bit profile. They accept an
exact shift of 16 and inverse, reject an exact-f64 shift of `2^-10` whose reduced
destination would lose bits, retain observations/history on rejection and continue
ticking. Separate old-history witnesses make only a previous centre, backup slot
or reader history fail that precision condition. The case is also included in the
placement/admission suite.

The emitted compiler audit checks the two new kernels' actual frozen bindings,
writer ownership, strict arithmetic and publication guard. Unsafe missing-output,
contracted-arithmetic and removed-publication-guard controls must fail; legal
repeated writes remain accepted. Source-pinned results and untested profiles are
recorded in [admission evidence](../tests/admission/README.md).

The combined placement test runner reserves 2,048 native SNode IDs because its
many independent fixtures share a Program whose issued IDs are monotonic. This
test-only reservation does not enlarge production model capacity.

### Hosted continuation correction

Review [5775793038](https://github.com/Human-Cognome-Project/human-cognome-project/pull/213#pullrequestreview-5775793038)
found that #213's hosted combined two-worker placement runs failed the old
cross-execution equality assertion. Local instrumented combined runs at the
#213 source and accepted #212 baseline did not reproduce the natural timing
failure. They are not evidence that the hosted failure was absent.

The old assertion nevertheless exceeds the accepted [publication contract](LIVE-HIERARCHY.md#publication-history-and-recipient-checks):
parents need not acquire a simultaneous sibling snapshot. Deterministic targeted
polls reproduce the invalid assumption on the accepted #212 engine as well as
the corrected branch: parent-first retains `1.0625` with work pending, while
child-first publishes `1.125`. Both reach exactly `1.125` on the next poll, with
old packet/history lifetime intact. During the original moving tick, the exact
permitted parent values are `1` and `1.0625`; the old/new distinction affects only
the parent's current coordinate, selected packet word and pending bit. Other
words and old history still compare exactly across the two executions.

Production kernels, precision gates, arithmetic and scheduling are unchanged.
The no-tick rebase tests still require exact translation of all current/old
surfaces and exact inverse restoration. This correction addresses the demonstrated
test-contract defect; confirmation against the original hosted configuration
remains necessary before maintainer integration. Source-pinned diagnostic and
corrected results are in the admission evidence record.
