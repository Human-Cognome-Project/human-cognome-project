# Current-state upward centroid candidates

Status: **accepted in #182, 2026-10-24**, following the accepted
[#180 plan](../../docs/upward-centroid-integration-plan.md) under #172/#177.
This is the candidate-only component. Live field replacement, forces, wakes,
motion, transactional ticks and downward response remain outstanding.

## Resolution-cycle boundary

The [#181 settling clarification](../../docs/upward-centroid-integration-plan.md#resolution-cycle-clarification--2026-10-24)
governs later live integration. This explicit complete candidate evaluation is
not a mandatory per-tick hierarchy solve. Trial propagation across ordinary
native ticks and judge coherent settling and perturbation recovery; these
candidate tests do not qualify that live cycle.

## Native operation

[Harness](../src/field/field.h) exposes `bind_centroid_cascade`,
`query_centroid_cascade` and `inspect_centroid_cascade`. The
[input/result types](../src/field/cascade.h) and
[implementation](../src/field/centroid_cascade.cpp) use the existing exclusively
owned, synchronized prepared-execution boundary. There is no second awake set,
new scheduler, per-level SNode tree, semantic matcher or database access.

Preparation supplies a fixed explicit hierarchy; it is input scope, not an
analyst selection policy. Each node identifies an exact native field, a nonzero
comparison identity, a nonnegative conceptual level, a cohort of full occurrence
handles, child indices and an output profile. Higher conceptual levels are
coarser. Indices are postorder, children precede parents, each nonroot has exactly
one parent, and the final node is the unique expressed root. One, two or many
expressed levels are supported within reservation; skipped levels create no
synthetic comparison or quantization stage.

Within a node, the first appearances of cohort handles must follow the source
participation ledger's row order; first appearances of children must be in
ascending node-index order. Identical requested handles or child indices within
that node coalesce. Caller arrays remain stable through bind and must not alias
the operation's borrowed inspection arrays. Bind copies normalized input into
owned reserved tables; it does not retain caller pointers.

Each selected occurrence/field pair must have exactly one source participation
row with share one. Distinct source ordinals remain distinct: repeated source
rows reject as unsupported participation rather than being silently deduplicated.
Children must have disjoint physical occurrence cohorts whose union exactly
matches their parent's cohort. Distinct live handles for repeated definitions
remain independent contributions. These whole-body/disjoint restrictions belong
to this first component, **not the general membership model**.

The separate [Segment B weighted interface](WEIGHTED-CENTROID-CASCADE.md), accepted
in #185 following #184, adds exact source-row accounting for compatible fractional
overlaps. The original API and its restrictions above remain intact.

Nodes sharing a comparison identity must have the same field and conceptual
level, with at most one expressed publication. Internal nodes stay within their
parent's context and use full precision. Expressed children have distinct contexts
and strictly finer conceptual levels. Dependency rounds include temporary
internal grouping; they are not inferred conceptual LoDs.

## Calculation and publication

Every evaluation validates current native presence, source endpoints/share and
finite positive mass/finite coordinates, including sleeping bodies. It then
reduces finest-to-root in prepared dependency rounds. Nodes in a round may run
in parallel; each node uses canonical ordered inputs with strict math:

```text
M = sum(m_i)
c = a + sum(m_i * (x_i - a)) / M
```

The reference `a` is the first contributor in the same operative frame. Finest
loads are promoted from resident f32 to f64 before subtraction, multiplication
and addition. Child inputs are their **published** f64 centres and untrimmed f64
cohort masses. Internal grouping has an identity output mask. Expressed nodes
apply one requested experimental `Q_b` boundary, using the same checked
`Precision::fraction_mask()` as the standalone native primitive. Both calculation
and candidate containers are f64; other profiles reject. Legal b is 0 through 52.
There is no residual storage or re-entry of a discarded child tail.

The experimental mask clears low encoded fraction bits toward zero. It can keep
small variation within an interval from changing the stored position, but still
has boundary crossing and origin bias. Production rounding and a per-LoD ladder
remain open. Resident f32 precision is not recovered by widening. The fork's
existing FTZ arithmetic mode also remains unchanged; see
[native precision limits](NATIVE-PRECISION.md#discovered-native-flush-to-zero-boundary).

All candidates retain the current double frame anchor. Source-unit conversion
has already produced resident coordinates. Neither hierarchy edges nor internal
grouping multiply distance; compression/naming remains 1:1. Tests use actual
ScaleBinding preparation to map source positions 1 and 1 at units 1 and 64 to
resident positions 1 and 64, with mean 32.5 and no second factor. This qualifies
conversion once, not the later conceptual force-distance policy.

Temporary association can change floating-point rounding. Tests compare each
node against an independent long-double weighted reference using the same
declared published child boundaries and a count/magnitude error bound. They do
not assert arbitrary regrouping is bitwise identical to a flat untrimmed mean.

## Validity and failure

Any bind attempt invalidates the previous candidate and hierarchy. A rejected
bind leaves the hierarchy unbound; retry explicitly with valid input. A failed
query invalidates its previous candidate but retains the bound hierarchy.
Ordinary input, arithmetic or budget rejection changes no live particles,
centroids, focus, source topology/ordinals, activity/history or Demand, and keeps
execution ready. Unexpected backend, source-invariant or borrowed-binding failure
marks the Harness unready under existing policy. There is no recovery/restore
bank or healthy partial physical publication.

An empty prepared study returns `Empty`, root -1, mass 0 and no physical centre.
Successful results carry native context, global source generation, completed
tick observation, bind serial and query serial. These are provenance, not activity
epochs. Ticks, source edits, rebinds and superseding/failed queries invalidate old
observations. A source edit requires explicit rebind even when it affects another
field. A bound hierarchy survives ordinary ticks and reads current native values
on its next query. Inspection is a synchronous borrowed view, valid only until
the next synchronized operation or destruction; use the result's logical prefixes.

## Resources and work

Reserve `CascadeNodes`, `CascadeLinks`, `CascadeReferences`, `CascadeLevels` and
`CascadeWork` along with existing capacities and `ScratchBytes`. Zero nodes
disables this component and adds no cascade payload or kernel contexts. Owned
references additionally charge existing `References` by normalized N+R+H on top
of live source references. This does not claim new physical records.

The typed manifest records actual widths and capacities. With physical sentinels
`n=max(Ncap,1)`, `r=max(Rcap,1)`, `h=max(Hcap,1)`:

| Storage | Payload |
|---|---:|
| Host node headers | n × sizeof(CascadeNode), observed 88 bytes each |
| Host cohort handles | r × sizeof(RecordHandle), 24 bytes each |
| Host/native node controls | 2 ×8n ×4 |
| Host/native reference controls | 2 ×3r ×4 |
| Host/native children and execution order | 2 ×(h+n) ×4 |
| Host/native precision masks | 2 ×n ×8 |
| Host parent/rank work | 2 ×n ×4 |
| Host round offsets/cursors | 2 ×(Lcap+1) ×4 |
| Host/native trial XYZ, published XYZ, mass | 2 ×7n ×8 |
| Host/native node validity | 2 ×n ×4 |
| Host/native counters and root summary | 2 ×(8 ×8 +7 ×8) |

Candidate arrays use native ndarrays on both host paths. CpuDirect borrows the
existing source vectors; BulkCopy reads existing native source ndarrays. Host
vectors are ordinary memory, not pinned. Two prepared kernel contexts add
22 arguments; binding reservation and actual measured bindings are separate
from typed payload. Fixed objects/vector metadata, compiler/runtime internals
and native pool alignment are outside payload figures.

Bind validates headers, memberships, aliases and coverage in reserved host
tables, then uploads full reserved control/reference/child/order/mask buffers
and performs native source validation. Successful nonempty bind reads 64 bytes
of counters and synchronizes once. Host preparation can be O(R×E) for ledger
lookup and quadratic for deduplication/coverage. `preparation_visits` charges
logical validation comparisons and packing/initialization work; it is not an
instruction or memory-operation counter. Capacity-sized transfers are reported
separately. Preparation is measured independently from ordinary query work.

For normalized N nodes, R cohort references, H links and Rleaf finest references,
query preflight requires **2N +R +Rleaf +H** logical visits. It dispatches one
validation kernel and one reduction launch per dependency round. A successful
query has two explicit synchronizations and 184 readback bytes (two 64-byte
counter reads and a 56-byte root summary), without host body downloads or numeric
uploads. `awake_reference_visits` counts references, not unique awake bodies.
Explicit inspection additionally downloads `7n ×8 +n ×4` bytes.

## CPU evidence and reproduction

[Retained evidence](../tests/cascade/README.md) pins the tested source, backend,
compiler, artifacts, profiles and raw results. Eight modes cover both CPU host
paths, reservations P=70/G=9 and P=4096/G=257, holes/final physical rows, and
maximal P/G/E/cohort/link/node/round demands. Each repeats 32 real tick/query
cycles. Independent tests cover every f64 retained-bit count, both-sign wobble
and crossing, cancellation/unequal masses, internal grouping, skipped levels,
repeated definitions, alias/source-ordinal distinctions and independent limits.

All **62 native CPU suites passed in 427.34 seconds**. The two reservations have
the following measured logical resources (the same on both source host paths):

| P/G/E capacities | Scratch bytes, including binding reserve | Actual binding bytes | Query work with eight bodies / maximal bodies |
|---|---:|---:|---:|
| 70/9/280 | 124,900 | 9,304 | 52 /300 |
| 4096/257/16384 | 5,409,604 | 9,304 | 52 /16,404 |

The maximal 4,096-body bind performs 234,903,642 preparation visits; observed
binding time is 178–193 ms in this run. That cost belongs to separately requested
hierarchy preparation. A normal query still reads 184 bytes with two explicit
synchronizations; full inspection is a separate readback. These measurements
are bounded CPU observations, not hardware throughput guarantees.

Complete before/after snapshots check physical arrays, source/control/native
activity/history, anchor and Demand. BulkCopy staging is deliberately poisoned
to distinguish backend reads. A test-only nonfinite source fault checks rejection
without reduction/publication. Measured first-use/reuse boundaries observe zero
linked C++ new/new[] calls and stable known native backing/layout/compiler
counters. This excludes arbitrary direct malloc, page residency/faults and driver
internals; it is not complete process/device allocation accounting.

```sh
cmake --build engine/build -j3
ctest --test-dir engine/build -R '^cascade_' --output-on-failure -j2
ctest --test-dir engine/build --output-on-failure -j2
```

Use the [matched native build](PREPARED-EXECUTION.md#reproduction). No PostgreSQL
code/data changes or Python runtime are involved. Actual GPU qualification
remains #145 after the integrated CPU operation exit. #172/#177/#156 remain open;
this candidate component does not satisfy live precision/state/force/phase or
#170 motion/contact publication acceptance.
