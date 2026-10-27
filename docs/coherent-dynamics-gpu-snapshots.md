# Coherent dynamics and GPU snapshot continuation

Status: maintainer direction, 2026-10-27, following Patrick's clarification.
This supersedes universal moving-fixture quiescence requirements in #194 and
older guidance. It does not retroactively turn #195's recorded failing run into
a passing run or accept unreviewed runtime behavior.

## Model requirement

Exact resolution is an asymptotic ideal, not a required endpoint for every study.
Shared nibble/endpoint structure does not establish enough differentiated mass
or constraints to crystallize. Do not assume these particular fixtures have an
equilibrium, or assert that their continued motion proves semantic ambiguity.
The engine must coherently derive field effects from current positions and
validated participation, propagate them, and integrate each physical body once.

Settling is an observation unless a fixture independently establishes that it
should settle. Preserve physical velocity, cumulative destination publication,
mass/count accounting and pending inputs. No forced stillness, arbitrary sleep
epsilon, extra damping or new scheduler is authorized by this clarification.

## Revision required in #195

Keep the original source pins, 92/96 hosted result and all failing traces intact.
The new test contract must be visibly versioned separately from those results.

- Keep arithmetic, single-integration, complete-packet/history, no-lost-input,
  finite/domain, identity, rejection and prepared-resource checks as hard failures.
- Keep held-input convergence and reference agreement as hard requirements.
- Exercise moving two/three-level, overlap, permuted-order and letter/byte cases
  for declared finite observation windows and all 32 perturbation/reuse rounds.
  Record settled versus still evolving independently of invariant pass/fail.
  Reaching 4096 ticks alone is not an invariant failure. Do not skip later fixtures
  or reuse checks merely because an earlier fixture remains active.
- Preserve per-tick force/integration and acquired-publication reference checks.
  An instantaneous full hierarchy solve is not the reference for a lagged live
  tick. Use its actually acquired packets; test held-input resolution separately.
- The current post-limit frozen continuation changes hierarchy publications and
  pending work. Do not simply remove its throw and resume the same moving study
  as if nothing happened. Use a separately reconstructed diagnostic state or
  run that diagnostic after the continuous observation/recovery sequence, with
  its intervention explicit. Do not introduce routine global reseeding.
- Use quiet-mask controls with justified expected quiet behavior to detect lost
  wakeups. Continue collision/late-notification controls and state preservation.
  Do not impose arbitrary spatial boundedness on all studies either: report
  excursions, travel, reversals and growth; fail violated declared numerical or
  resource domains and independently justified fixture bounds.
- Preserve Q8/Q6 as a labelled coarse stress case, not a production default.
  Add focused publication diagnostics for both signs, toward/away from zero,
  exponent/cell boundaries and cancellation: compare calculated displacement,
  published displacement and absolute publication error in local ULPs.
  A large ratio to an almost-zero attempted step alone is not a universal error
  criterion. Do not change Q or the physical laws merely to obtain quiet masks.
  Production calibration remains open; expose selected precision honestly.
- Run the revised four CPU modes and full regression. PostgreSQL integration and
  actual CPU backend verification skipped by the failed hosted job still need
  fresh successful evidence. Contributor and hosted results remain separate.

This is a request to revise executable acceptance and evidence, not permission
to merge with known unexplained arithmetic, lifetime or state-integrity defects.
C3-2 structural refresh still follows separate C3-1 acceptance.

## Path to Patrick's GPU snapshots

The immediate deliverable is a reproducible native run of the current hierarchy
with coherent rendered snapshots so Patrick can inspect the patterns. A bounded
fixed study is sufficient for this observation milestone. Completion of all
#156 admission/owner operations is not a prerequisite for that bounded trial;
those operations and full #145 qualification remain open.

Current concrete gaps at #195's runtime pin:

1. `LiveHierarchy::install` explicitly accepts only actual LLVM/x64. Do not
   remove that check and claim CUDA qualification. Qualify the actual CUDA
   publication/selector memory ordering, packet lifetimes, native activity and
   f64 numerical path on the supported local GPU. Preserve explicit rejection
   until the backend contract is supported. A bounded backend adaptation needs
   review; prefer native facilities and no additional scheduler.
2. The existing viewport capture client constructs legacy ActiveSession/FixedStudy
   execution. Its `--backend cuda` option does not install the C3 hierarchy.
   Provide a bounded native C++ trial capture adapter that reserves and installs
   the accepted hierarchy and exports that execution's synchronized state.
   Reuse snapshot validation/export and atomic latest.json publication.
3. The existing viewer renders exposed bodies, not hierarchy field/edge overlays.
   First deliver faithful body snapshots; make any field overlay a separately
   identified observation feature if needed. Do not render centroids as extra
   physical mass or invent spatial offsets.

Capture identity must retain occurrence/source mapping, local frame/reference,
generation and completed tick, actual backend, numerical profile and per-node
precision where the global metadata cannot express overrides. Export sleepers
as well as active bodies, actual radii/masses and supplied labels. Capture must
not mutate dynamics, drop pending work, force settling or narrow f64 silently.
Borrowed inspection arrays must be copied at the synchronized observation point.
Unsupported capture metadata must fail explicitly, not be mislabelled.

Provide a finite reproducible CLI run, configurable capture cadence, source and
backend pins, first/final captures, atomic latest.json and a matching native
evidence record. Test CPU export/round-trip/viewer consistency before the local
GPU run. Reuse the existing offline viewer and folder watch. Actual CUDA must be
checked from the running Program, with fallback treated as failure for a CUDA
request. Report transfers/readback and capture cost separately from engine work.

Patrick's machine supplies actual local GPU evidence. Cloud CPU results cannot
substitute for it. Full GPU throughput/pinning claims are not needed to inspect
a correctly qualified bounded run. No persistent database mutation, new data
population or operational insertion is implied by this milestone.
