# Native intent packet diagnostic experiment

Status: bounded test implementation for [#327 review 5896078321](https://github.com/Human-Cognome-Project/human-cognome-project/pull/327#pullrequestreview-5896078321),
not a production intent or brake. Continues the [comparison proposal](INTENT-DERIVATION.md).
Coefficient weighting/contact exclusion are authorized only for this experiment.
The exact-coincidence model law and production activity/braking are unchanged.

## Boundary and native ownership

[intent_native_test.cpp](../tests/intent_native_test.cpp) builds strict f64 IR through
Taichi's existing IRBuilder and engine::CompiledKernel. Runtime/Program own the native
execution; two existing Ndarray wrappers own the input/output allocations. A prepared
launch context is reused twice. There is no custom executor, activity set, registry,
field storage or production Harness modification. The test's independent expected
values are literals/algebra from #326/#327, not outputs of the native calculation.

This first native step consumes **supplied contribution packets**, not actual live
hierarchy publications. It does not establish native source enumeration, alias dedup,
sleeping-body invalidation, publication validity, fixed-backing admission or trajectories.
Those integrations remain necessary; a passing packet probe cannot substitute for them.

Each case has exactly two packets. A packet contains displacement XYZ, coupling
G*participating_mass*field_mass, and separately supplied contact-force XYZ. Three
additional values give recipient XYZ in the common operative frame. Zero-coupling
packets contribute no attraction but their entire input representation is validated.
Negative coupling, nonfinite or subnormal input is unsupported. Signed zero is zero.
Native normal-or-zero validation reuses field::detail::supported_real.

Both packets are visited regardless of prior invalidity or coincidence. Sanitized zero
values prevent invalid arithmetic from becoming authoritative; they are placeholders,
not epsilons or a coincidence law. Positive finite noncoincident packets contribute
alpha=k/|d|^3, gated field force alpha*d and coefficient sum. Eligible exact-coincident
packets contribute zero gated physical force and record coincidence without dividing.
Contacts are accumulated separately and never enter the candidate normalizer.

## Status and output validity

Status precedence is explicit: Unsupported(3) > UnresolvedCoincidence(2) > Supported(1)
> Absent(0). Mixed invalid/coincident packet fixtures run in both orders, including
infinite coincident coupling, NaN and negative coupling. No early return at coincidence.

Outputs have 17 columns:

| Columns | Meaning |
|---|---|
| 0 | Combined status |
| 1 | Field/coordinate arithmetic available |
| 2 | Contact arithmetic available |
| 3 | Coincident-source count, trustworthy only when field arithmetic is available |
| 4–6 | Gated attraction force, zero placeholder when unavailable |
| 7–9 | Contact force, zero placeholder when unavailable |
| 10–12 | Recipient-relative intent displacement, only usable for Supported |
| 13–15 | Recipient plus displacement in the same common frame, only usable for Supported |
| 16 | Fixed packet visit bound (2), not a measured instruction counter |

Unsupported contact can coexist with available field force and vice versa. Unresolved
coincidence can retain available gated force from separated packets, but **no target is
usable**. Absent intent and valid cancellation at zero displacement differ by status.
A zero placeholder never establishes valid zero intent. There is no implicit sleep/wake
use of any result.

The kernel checks normal-or-zero results, nonzero products/quotients flushed to zero,
zero sums not explained by exact cancellation, positive norm/cube denominators and
accumulation overflow. It refuses intermediate range loss; it does not rescale or prove
all mathematically finite inputs are supported. Squared-distance overflow/underflow and
coefficient underflow fixtures exercise this conservative boundary. No epsilon restores
rejected inputs. This is scoped native domain evidence, not a verified interval/error
bound for all binary64 inputs or another backend's floating-point behavior.

## Frames, motion and repeated observations

For an inherited packet, the supplied displacement is parent-centroid minus child-
centroid, already converted to the common operative frame. The virtual target is
recipient plus displacement. The fixture holds displacement +2 while moving recipient
10 to 11: relative target stays +2, common target moves 12 to 13. A translated recipient
(138,-64,0) yields target (140,-64,0). These verify packet arithmetic, not an operational
rebase or hierarchy provenance path. Actual graph extraction must preserve the existing
source identity/canonical path rules and publication semantics.

Two equal-force packets at distances 1 and 2 yield force +1 but different intent. The
probe observes the difference; it does not wake a sleeping Harness recipient. There is
no dt, quantizer, brake, motion integrator or automatic stability classifier here.

## Storage and work disclosure

Case count is bounded by 64 and packets per case by 2. Fixed shapes are 17 by N for each
native array. Their combined logical payload is 34*N*sizeof(double), with another two
host transfer vectors and the case table. The test reports their actual host capacities
and native prepared-context requested bytes. All buffers/context are created before the
repeated diagnostic launches. No per-packet container allocation or graph discovery is
emitted into the kernel; arithmetic work is bounded by two packets per case and native
range traversal. Native compilation/registration, wrapper objects, allocator rounding,
Program and runtime storage are separate overhead; the printed payload is not full B
or a physical-residency bound. No general admission interface is added by this fixture.

Before moving the probe into a running owner, charge all persistent/scratch/readback and
validity/baseline storage through existing owner preparation, and prepay traversal and
comparison work. This test adds no hidden production state requiring a parallel ledger.

## Qualification and next work

CMake registers intent_native_1 and intent_native_2 in the engine's existing test list.
The hosted regression script shards that complete configured list, so these new native
cases are included automatically. The earlier standalone long-double reference remains
separate. Local results must identify actual CPU backend and source pin; GPU behavior is
not inferred. Next steps remain live-owner extraction/accounting, canonical/rebase and
wake/invalidation fixtures, then many-tick dt/Q comparisons before any production brake.

### Executed CPU evidence — 2026-11-13

Tested source `e6c4a5e1694cc07f730c8a20291543bbf5d1a12b`; tracked Taichi tree
`ef9795b3e135de6edb4c3f5c8d409d6487f41385`. Native static library and diagnostic
build passed. intent_native_1 and intent_native_2 both passed (29 cases each), with
two prepared launches per test; explicit direct runs also passed. CPU was asserted
from the actual runtime. No GPU computation, native Windows or database qualification.

Additional controls cover XY and Z components, lone coincidence, invalid recipient
coordinates, normalizer/contact accumulation overflow and a contact sum underflow.
Both worker counts report 58 fixed packet visits, 7,888 bytes native logical payload,
12,240 bytes host table/transfer capacities and 208 prepared-context bytes. Those are
this fixture/ABI's observations, not full-runtime residency or a performance claim.

The first source `71cb9f84` built, but both tests failed before launch with
`prepared array type/rank differs from signature`: the test prepared an external
context and then launched a different, unbound reusable context. The correction binds,
prepares and reuses the same native context. Initial logs/source pin are preserved in
`initial-failed-attempt`; they are not relabelled as passing evidence. Final captured
build/direct-run logs have no warnings/errors. The production context API was unchanged.

Contributor-local archive `native-intent-e6c4a5e1.tar.gz` retains source/config/compiler/
binary pins, commands, full test/direct results, configured test listing and failed logs.
Manifest SHA256 `fa282f69b0739cd4ec1e1742125cbbc3b36d0182a016452cabdc8cefcb034914`;
archive SHA256 `da5c23313665c530c412d5305d4e4ea7b53b11020f9317a6dee1721f5fde3f90`.
Raw local evidence is not independently reviewed remotely. Final publication after the
tested pin changes only Markdown. No unrelated parent qualification was rerun.
