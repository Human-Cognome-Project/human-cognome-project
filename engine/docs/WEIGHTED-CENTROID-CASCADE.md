# Source-weighted centroid candidates

Status: **Segment B implementation for review**, under the accepted
[#183 accounting plan](../../docs/weighted-centroid-accounting-plan.md), following
[#184 source counts](SOURCE-COUNTS.md). This extends the bounded
[upward candidate operation](CENTROID-CASCADE.md); it changes no live field,
force, motion, wake, focus, topology or phase behavior. The full query is a
reference operation, not a required per-tick solver. Later live propagation is
governed by the [#181 settling clarification](../../docs/upward-centroid-integration-plan.md#resolution-cycle-clarification--2026-10-24).

## Explicit source selection

`Harness::bind_weighted_centroid_cascade` accepts a `WeightedCascadeSpec`.
Its node and child tables have the same postorder, context, conceptual-level and
publication rules as the original API. Each node's cohort span indexes
`CascadeContribution` rows: a full occurrence handle plus source generation,
ordered participation row and ordinal. Counts and weights are not query inputs.
The bind validates the row against that exact field/occurrence and the committed
source accounting retained by Segment A. Missing, zero or inconsistent metadata
returns `UnsupportedAccounting`; rounded legacy shares are never inverted to
guess counts.

Within a node, repeated paths to the same source row coalesce. Different source
rows and ordinals remain separate even when their endpoints coincide. First
appearances follow source-row order; children follow ascending node order.
Independent live copies retain independent full handles and contributions.
Caller tables remain stable during bind and must not alias borrowed inspection
tables. No caller pointers survive the bind.

For every selected occurrence, the bind accumulates integer coverage C from
its selected source counts against its structural denominator D:

```text
0 < C(node, occurrence) <= D
C(parent, occurrence) = sum C(child, occurrence)
```

Each addition checks remaining capacity before adding. The parent must account
for every child occurrence, with no extras or omissions. There is no overflow,
floating tolerance, inferred count, automatic rescaling or fallback to a flat
body reduction. Compatible overlaps work: A has a=1,b=1; B has b=1,c=1; their
parent has a=1,b=2,c=1, with denominators 1,2,1. Arbitrary overlapping whole-child
comparisons still reject when exact coverage does not balance. Unrelated
comparison contexts do not share a global participation cap.

The original `bind_centroid_cascade` API retains its share-one, unambiguous-row,
disjoint-child rules, including on a weighted-enabled reservation. It can still
operate with unknown source accounting. Its behavior is not silently widened.

## Native calculation and publication

Finest-node contributions use the current native body's position and physical
mass, including sleepers. The source denominator is structural provenance, not
a numerical mass override:

```text
s = f64(source count) / f64(source denominator)
w = f64(current physical mass) * s
M = sum(w)
c = a + sum(w * (f64(current position) - a)) / M
```

The implementation uses native u64 loads/casts, f64 division and ordered strict
reductions. It promotes before arithmetic. Resident body values remain f32;
widening cannot recover their lost detail. Conversion of integers above 2^53,
division, products and reduction have ordinary floating error. Tests encode
round-to-nearest-even integer-to-binary64 conversion independently with integer
bits, then check native ratios at both significand and unsigned 64-bit boundaries.
Long-double count/mass/position references separately bound ratio and reduction
error. Thirds demonstrably differ from the promoted legacy f32 share.

A parent consumes each child's **published** centre and untrimmed participating
mass. It does not apply the source share again, recover a hidden first moment,
or reintroduce a discarded positional tail. Same-LoD internal grouping uses
identity precision. Expressed boundaries use the requested experimental Q_b,
with all 0..52 retained fraction-bit counts supported. Toward-zero bit clearing
has boundary crossing and origin bias; production rounding and a per-LoD ladder
remain open. Existing [native FTZ limits](NATIVE-PRECISION.md#discovered-native-flush-to-zero-boundary)
are unchanged.

All inputs share the operative frame and current frame anchor. Conceptual source
unit conversion has already produced resident coordinates. Neither a comparison
edge nor internal grouping introduces another distance factor; secondary
compression and naming remain 1:1.

## Validity, resources and observations

The original candidate failure/lifetime contract also applies here: every bind
attempt invalidates the prior hierarchy and candidate; a rejected bind leaves
the hierarchy unbound. A failed query invalidates its candidate but retains the
bound hierarchy. Ordinary rejections preserve physical/source/control/activity/
history/anchor/Demand state and readiness. Unexpected backend/source-invariant
failure makes execution unready. Native source endpoints and legacy share are
checked against the pinned descriptors before each reduction. Source edits,
including count-only edits with an unchanged f32 share, require explicit rebind.
Generation, full handles and ordinals prevent following compacted/recycled rows.
Ticks invalidate old observations while a valid binding reads current values on
its next query.

In addition to the original N/H/R/L/work limits, reserve `CascadeCoverage` for
unique node/occurrence coverage entries C. Coverage cannot exceed normalized
selected references. Zero disables weighted binding and adds no weighted tables,
native arrays or extra kernel parameters. With it enabled, both APIs use the
same prepared kernels; whole binds install count/denominator descriptors 1/1.
Global `References` charges live source references plus N+R+H+C. The tables
describe candidates and do not claim physical records.
Raw contribution and child table lengths must also fit their respective limits
before alias normalization.

In addition to the original candidate payload, with
`r=max(Rcap,1)`, `c=max(Ccap,1)`, `n=max(Ncap,1)`:

| Prepared payload | Bytes on the tested ABI |
|---|---:|
| Host source row identities, parallel to cohort handles | 24r |
| Host/native pinned legacy shares | 2 ×4r |
| Host/native source count and denominator descriptors | 2 ×2r ×8 |
| Host exact coverage (handle/count/denominator) | 40c |
| Host checked child accumulation work | 8c |
| Host coverage offsets | 4(n+1) |

The added typed payload is **64r +48c +4(n+1)**. The caller's contribution
structure is 48 bytes; it is not retained as an additional owned copy. Source
metadata/commands from Segment A are unchanged. Two additional array arguments
bring the existing validation/reduction contexts from 9/13 to 10/14 arguments;
there are no additional kernels, layouts or activity sets. Binding reserve and
actual binding bytes are recorded separately in the manifest/evidence. Fixed
object/vector metadata, compiler/runtime storage, pool alignment and caller input
arrays are outside typed payload figures. Ordinary host vectors are not pinned.
Observed by-value sizes are CascadeBinding=136 and CascadeResult=272 bytes
(16-byte increases each), and BudgetDemand=128 bytes (an 8-byte increase).
CascadeNode remains 88 bytes; Segment A's update command/source tables are
unchanged. These fixed header sizes are reported separately from array payload.

Bind performs separately bounded host validation, alias/coverage comparison and
packing; its scans can be quadratic. It uploads full reserved control and source
descriptor buffers, then reads 64 counter bytes with one explicit synchronization.
No allocation, layout construction or compilation belongs to bind/query/reuse.
Preparation visits count logical comparisons/initialization/packing, not every
instruction; capacity-sized transfer bytes are reported separately.

Query work remains **2N +R +Rleaf +H** logical validation/reduction visits;
extra ratio arithmetic does not create additional logical visits. A successful
query reads 184 bytes with two explicit synchronizations and no numeric upload
or host body download. Explicit inspection additionally reads `7n ×8 +n ×4`
native value/validity bytes. Source identity and exact coverage inspection are
reserved host metadata; use only the successful result's logical prefixes.

## CPU evidence and reproduction

[Retained evidence](../tests/weighted_cascade/README.md) supplies the tested
source/backend/compiler pins, complete native regression, eight weighted profiles,
typed manifests, observed widths, timings and backing/allocation counters.
The modes cross both CPU source host paths, P=70/G=9 and P=4096/G=257, and
holes/final rows versus full P/G/E/R/C/N/H/L demands. Each performs 32 real
tick/query/inspection cycles. Nonmaximal profiles test every experimental Q_b,
direct/partial/skipped-level comparisons, compatible overlaps, repeated ordinals,
same-LoD grouping, and both-sign same-cell/crossing traces. Supplemental small
profiles check independent limits, source edits/recycling, native source faults,
nonfinite bodies, and actual assembly preparation of a/b/c plus an independent
copy and converted source units.

All **74 native CPU suites passed in 454.04 seconds** on runtime/test source
`58ccae54da69bede55b49e6e3bc8e05eb58f3f8e`. Measured resources, equal on both
CPU source host paths:

| P/G/E capacities | Scratch bytes including binding reserve | Actual binding bytes | Full bind upload bytes | Query work: 8 bodies / maximal |
|---|---:|---:|---:|---:|
| 70/9/560 | 260,484 | 9,416 | 18,252 | 124 /930 |
| 4096/257/32768 | 13,282,276 | 9,416 | 1,048,908 | 124 /53,268 |

The maximal 4,096-body bind uses 201,416,802 separately bounded preparation
visits and takes 154 ms in this run. Query summary transfer remains 184 bytes;
these are observations on this CPU build, not throughput guarantees.

```sh
cmake --build engine/build -j3
ctest --test-dir engine/build -R '^weighted_cascade_' --output-on-failure -j2
ctest --test-dir engine/build --output-on-failure -j2
```

Use the [matched native build](PREPARED-EXECUTION.md#reproduction). Linked C++
new/new[] and known native pool/list/cursor/layout/compiler observations do not
cover arbitrary direct malloc, page residency/faults or driver internals. This
is bounded CPU candidate evidence, not live ripple/force/settling acceptance or
GPU qualification. No database writes or Python runtime are introduced.
#172/#177/#156/#145 remain open.
