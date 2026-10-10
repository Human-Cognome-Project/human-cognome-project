# Native engine object-support results

Date: **2026-10-10**. Implements the accepted corrections and execution plan
in [#124](https://github.com/Human-Cognome-Project/human-cognome-project/pull/124)
and [#125](https://github.com/Human-Cognome-Project/human-cognome-project/pull/125),
from main `5d42a43fb87928c89f7c23eb060b7e324a550a9c`.

## Changes exercised

- Parent force and centroid reduction use the particle centre and proportional
  participating mass. Repeated ordered parents remain separate contributions.
- Particle payload carries independent prepared radius; supplied centroid and
  component spheres determine enclosure. Positions, velocity and radius use
  the same operative conversion.
- Explicit contact pairs contain only endpoint indices. Finite penetration
  resistance uses summed radii, with no corona, alignment or defined-bond hold.
- Native host preparation retains definition/occurrence identities, exact
  counts, ordinals and explicit SNode scale bindings. Whole-mass sibling fields
  and cross-position parent-value fields are prepared separately.
- Fixed-study publication seeds centroids and determiner state before tick;
  equal-count refreshes reseed the supplied target. Failed publication leaves
  ticks disabled. Aggregate/part selection excludes duplicate representation.
- Empty/all-free studies use valid transfers and guarded mass divisions.
  The dormant universal-attraction kernel is removed; radial damping's total
  mass is still counted once per particle. No brake or damping retuning was
  needed.

## Executed native checks

Local environment: Debian 13, Clang/LLVM 15.0.6, CMake 3.31.6, Ninja 1.12.1;
Release CPU-only wrapper linked to the unchanged matching Taichi build.
The runtime reports x64, five host launch threads, 33.3 GiB host memory and
no CUDA. The assembly fixture explicitly uses two launch threads, SNode
capacity 4096 and tree capacity 128, with environment overrides disabled.
The retained field fixture uses SNode capacity 2^20 and tree capacity 2^16.

After the runtime changes and new staging regressions, CTest passed **5/5**
at 19:11 UTC in **7.69 s**:

| Suite | Result | Time | Evidence |
|---|---|---|---|
| `engine_smoke_test` | PASS | 0.86 s | Native compile/launch/transfers, independent trees and sparse traversal. |
| `field_test` | PASS | 5.79 s | Unequal/repeated participating masses, whole-mass acceleration, contact boundaries, coincidence, calibration, brake, radial damping and staging rejection. |
| `preparation_test` | PASS | 0.01 s | Supplied enclosure, translation invariance, unequal radii, conversion, exact identity/count retention and malformed-input rejection. |
| `assembly_test` | PASS | 0.89 s | Registered study tree, initial/refreshed first force, repeated instances, exposure selection, free slots and non-runnable failed publication. |
| `index_cap_test` | PASS | 0.14 s | Native round-trip at flattened offset 2,179,999,999 (>2^31), with low cell unchanged. |

`engine_devices --cpu-only` also passed. CUDA execution and CPU/CUDA agreement
checks were unavailable on this host; native tests keep their available-CUDA
paths. The explicit hosted CPU CI regex includes all five suites.

Reproduction uses the checked-in
[native CPU workflow](../../.github/workflows/native-engine-cpu.yml).
The prepared cloud toolchain commands are recorded in the
[implementation plan](../../docs/engine-object-implementation-plan.md).
No compiler-substrate source change or Taichi rebuild was required.

## Fixture inputs and observations

The integrated study supplies definitions `01`, `10`, `11` of exact mass 2.
Each couplet has two parent occurrences of exact count 1, hence hot share 0.5.
`11` appears twice with independent state. Every selected occurrence has one
whole-mass sibling participation; six explicit contact pairs cover the four
live particles. Two additional slots are unused and stay zero.

All definitions supply centroid zero and component spheres at
`(-0.25,0,0)` and `(0.25,0,0)`, radius 0.4 each. The enclosure is 0.65.
These are declared test inputs, not approved hex packing. An expressed primary
step supplies spatial units 64; nested compression and naming equivalence keep
those units. The prepared f32 body radius rounds outward from 41.6.

| Occurrence | Slot | Supplied position | Operative initial position |
|---|---|---|---|
| `01-a` | 0 | (-3,0,0) | (-192,0,0) |
| `10-a` | 1 | (3,0,0) | (192,0,0) |
| `11-a` | 2 | (-1,3,0) | (-64,192,0) |
| `11-b` | 3 | (1,-3,0) | (64,-192,0) |

Initial velocities are zero. Parameters: `dt=1`,
`gravity=0.0771063443835106`, `contact_stiffness=40`, `reach_strength=1`.
Initial inclusive parent masses are 2 for `0` and 6 for `1`; the repeated
`11` sibling mass is 4. Independently calculated first forces agree within
1e-7 after both initial publication and a relationship refresh that replaces
`10`'s parents with two `0` occurrences and sets its position to (5,-1,0)
before conversion. The old centroid is not reused.

After **401 ticks**, the four-object fixture remained finite: final farthest
distance 202.379486, minimum centre separation 230.750992, maximum displacement
from prepared position 0.006307. This shows bounded interaction over that
declared horizon, not settled equilibrium or general scale/focus invariance.
The fixture stays above contact; contact onset is tested separately.

The isolated contact fixture uses masses 1 and 3, radii 0.25 and 0.75,
`contact_stiffness=4`, `dt=0.001`, no field edges. At centre separations
0.5, 1.0, 1.5 and 0, force on A along x is respectively -2, 0, 0 and 0,
with equal/opposite force on B. Coincident centres remain coincident and
finite. Penetration resistance is not a hard non-overlap guarantee.

The unequal/repeated response fixture uses masses 6 and 2 at x=0 and 8,
two shares of 0.25 on the first particle and one share of 1 on the second.
Its seeded centroid is x=3.2, mass 5; first force on the first particle is
`gravity*15/3.2^2`, absorbed by its total mass 6 at `dt=0.001`.
The unchanged unconstrained calibration meets at time 0.998 with `dt=0.002`;
the retained 64-particle ring at `dt=1`, 400 ticks remains finite with observed
farthest excursion 1520.875122. That historical ring is still an unsettled
fixture, not a model convergence gate.

## Compatibility and continuation

This is a source/payload change: particle fields now include radius, edge
floats contain only share, and bond/defined/alignment payloads are replaced by
contact endpoints. Callers must rebuild and restage with the current enums;
raw old payload layouts are not accepted as the new layout. No persisted
engine-state format or database schema is migrated here.

PR #75's deferred monitoring prototype must migrate its payload readers when
resumed after selective control functions exist. It is not a prerequisite and
was not modified. #123's general recomposition and packing examples remain
outside this fixed-study implementation. #111/DECLARE and persistent data
insertion are separate workstreams; no database was changed.

The passing engine-facing records are carried into the
[first-stage data-object model](../../docs/first-stage-data-object-model.md).
Continue that object/naming/language work directly rather than adding another
broad physics qualification phase.
