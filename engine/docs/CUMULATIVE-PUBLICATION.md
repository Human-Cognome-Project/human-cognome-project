# C2b-1 cumulative publication and activity

**Local motion revision, 2026-10-29:** [accepted-displacement motion](ACCEPTED-MOTION.md)
supersedes the untrimmed-velocity rule described below. The historical accepted
runs retain their original source pins; validation of the revision is separate.

## Maintainer acceptance — 2026-10-26

C2b-1 is accepted in #192 at reviewed head `67c7bdc`. Hosted
[native CPU run](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/40426660208/job/121934003147)
passed **88/88 suites in 1702.96 seconds**, followed by successful PostgreSQL
integration and CPU-only backend checks. Repository smoke and projection checks
also passed. Contributor evidence remains separately source-pinned.
The CI adjustment increases the job budget to 90 minutes and retains partial
CTest output; it changes no engine arithmetic or test pass criteria.

**C2b-2 is now released**, in its own test-only implementation PR under #191:
weighted multilevel centroid arithmetic using the accepted native handoff.
Production hierarchy installation, inherited response and the #156/#145 exits
remain open. Regional/density-based calculation-width selection remains an
optional future experiment, not part of this release. Keep numerical controls
minimal and general; retained precision is calibrated, not a fixed f60 rule.


Status: **accepted in #192**, under accepted
[#191](../../docs/cumulative-publication-integration-plan.md), following #190.
This corrects the existing single-level live f64 trial profile. Default legacy
arithmetic and its activity rules remain unchanged. The subsequent C2b-2 weighted
multilevel probe is now [implemented for review](WEIGHTED-NATIVE-HANDOFF.md)
in its own test-only PR.

## Publication and physical state

An active trial body still accumulates every applicable direct field/contact
force before one physical integration, using the existing brake and radial
damping. It publishes its complete positional result through the configured Q
once. Movement activity now follows `published_next != published_old`, rather
than an untrimmed displacement attempt or a velocity-only change.

Physical velocity, mass, accepted displacement and necessary history remain
separate from the discarded positional tail. A quiet publication consumes work
even when its physical velocity is nonzero or changed during that evaluation.
Later integration waits for another valid input/control notification. Inactivity
under this policy is not zero velocity or an exact force-equilibrium claim.
There is no new epsilon, speed threshold, velocity reset or cross-tick positional
accumulator. Attempt counts remain diagnostic and do not select work.

The integration visit also writes final cached speed from its post-brake,
post-radial-damping velocity **before** consuming the activity bit. This matters
for velocity-changing steps whose Q position is unchanged. The ordinary trial
tick omits the now-redundant determiner traversal; its prepared kernel remains
available for initialization. Legacy still uses its original determiner phase.

Dirty trial fields retain C1's fresh, ordered full-member f64 reduction,
including sleepers and exact source count/denominator ratios. The complete mean
is quantized once and compared with the previous published centre to set
`Changed`. Individual normalized movements are not independently filtered.
Untrimmed participating mass and previous-centre history are preserved; transient
anchor work is zero after publication. Current fine positions that survived
their own publication are valid inputs, not recovered discarded tails.

## Work consumption and readers

The existing one particle/one centroid NativeSets and ordered type phases remain.
There are no concurrent same-type notifiers during body integration: force and
contact calculation finish before integration, and contact propagation/centroid
publication happen after all body integration. Thus late body consumption is
safe in this bounded path. It is not the consume-at-entry protocol required by
C2a's concurrent centroid-to-centroid probe.

A body consumes only its completed evaluation. Later contact/centroid notifications
remain pending for its following calculation, including when that body was quiet
in the current phase. Parameter/control changes, count/source repair, contact
edits and derived focus/radial-origin changes still invalidate applicable inputs.
Mass/count changes cannot be suppressed by an unchanged positional Q result.
The public `invalidate()` operation still withdraws execution readiness; it is
not a wake-only API. A supported control change can invalidate input work while
retaining readiness. Unsupported prepared focus replacement stays unsupported;
the tests change the derived focus through supported coherent source edits.

The [accepted reader/writer audit](../../docs/cumulative-publication-integration-plan.md#all-current-readers-and-writers)
remains the implementation boundary. Force/contact read completed published
state, integration owns velocity/motion/final-speed writes, field reduction and
publication follow body writes, and field wake follows complete current/history
publication. Seed/repair, geometry, optional candidates and inspection run at
ordered exclusive boundaries. No production numerical selector, second slot,
hierarchy dependency or new activity surface is installed.

Ordinary validation/capacity rejection preserves committed state/readiness.
Backend/numerical failure after mutation still marks execution unready. Finite
normal-or-zero/FTZ restrictions and experimental toward-zero Q are unchanged;
production rounding and precision calibration remain open.

## CPU fixtures and interpretation

[Source-pinned evidence](../tests/cumulative_publication/README.md) retains
**88/88 CPU suites passing in 892.49 seconds** on runtime/test source
`8db7289398b2523519adb3c77ccf7cee852efa3c`, separately from later hosted acceptance.
It includes manifests, raw outputs, source/binary hashes and before/after backing
observations. No browser or GPU run is claimed.

[cumulative_publication_test.cpp](../tests/cumulative_publication_test.cpp) adds
four CPU modes: one/two native worker configurations crossed with BulkCopy and
CpuDirect. Every mode checks actual CPU selection and runs 22 bounded cases:

- Combined field/contact forces individually publish no outward movement at Q_4,
  while their sum crosses the body threshold. Independent long-double references
  check force, one integration, brake, speed and both-sign publication. Separate
  opposing-force controls cancel before Q; full-bit publication is a control.
- Forward/reverse contact indexes span holes and the final body row. A quiet
  recipient is notified after integration; its following calculation must read
  the changed contact geometry. Quiet velocity-changing contact work stores
  both endpoint velocities and final speeds, then consumes its bits.
- A three-axis radial fixture checks the final post-damping speed before sleep.
  Thirty-two later quiet ticks preserve position/velocity/speed without body
  evaluations, attempts, force/contact work or centroid reductions.
- Equal and unequal exact weights, repeated ordinals, cancellation and sleeping
  members exercise combined centroid publication. Physical mass is distinct
  from structural denominators and universal mass counts each body once.
- Repeated fresh reductions of fixed accepted fine positions discard the same
  subthreshold field result without accumulating a tail. This explicit all-loaded
  reference is a test control, not an added production scheduler.
- Thirty-two count-edit/reuse cycles with an unchanged f32 share and Q position
  wake/evaluate the affected members. Another 32 cycles change derived focal
  position and wake the otherwise unrelated radial recipient. Survivor state,
  previous-centre history and prepared backing remain intact.

Toward-zero Q is asymmetric at cell boundaries: an inward contribution can cross
a cell when an equally sized outward contribution does not. Cancellation is
therefore checked on the combined result; it does not assume both opposing
isolated steps would remain in the old cell. Numerical comparison bounds are
independent error checks and never sleep/activity thresholds.

The eight existing numerical-profile modes retain their full typed manifests,
source/update/query/geometry/reuse checks and prior regression coverage. Their
Q_4/Q_0 watch fixture now records one attempt, no publications, two initial body
evaluations and no later work over its 32-tick window, with preserved velocity.
The historical [C1 evidence](../tests/numerical_profile/README.md) continues to
describe its own source; it is not overwritten with revised results.

## Resources and reproduction

No new numerical array, per-capacity flag, native activity set or recurring context
is added. Integration binds the existing speed array as one additional trial
argument. Scratch/binding reservation includes that argument; actual retained
binding bytes and per-profile manifests are reported with the new evidence.
The ordinary trial tick removes one determiner dispatch without asserting a
throughput guarantee. Tick/inspection transfer shapes remain unchanged.

Setup, first population and reuse are observed separately. The guarded boundaries
count linked C++ new/new[] including workers; native requested/list/pool/cursor/
layout/compiler observations must remain unchanged across measured reuse. These
checks do not cover arbitrary malloc, page faults, residency or drivers. Test
fixtures and inspection copies are accounted separately from authoritative
execution state; host vectors are unpinned.

Use the matched native build and run:

```sh
cmake --build engine/build -j3
ctest --test-dir engine/build -R '^(cumulative_publication_|numerical_profile_)' --output-on-failure -j2
ctest --test-dir engine/build --output-on-failure -j2
```

This single-level change does not install live hierarchy, inherited response,
analyst/cache-manager behavior or database data. C2b-2 follows only after C2b-1
acceptance; production comparison ownership, publication/history/repair lifetimes
and coupled multilevel settling remain later review boundaries under #177/#172.
The #156 CPU operation exit and #145 GPU qualification remain open.
