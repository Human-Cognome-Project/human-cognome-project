# First-stage data-object model

Status: **Native record/preparation slice built and tested, 2026-10-10.**
The engine corrections approved in #124/#125 have passed their focused checks.
Continue object modeling from this contract; no further general physics
qualification gate is imposed. Persistent insertion and DECLARE corrections
remain separate work.

## Records carried into the engine

[`assembly/study.h`](../engine/src/assembly/study.h) defines the native records.
They separate reusable objects from their live occurrences:

| Record | Required information | Meaning |
|---|---|---|
| Definition | Opaque identity, exact integer structural mass, supplied centroid/component spheres, ordered parent occurrences | Reusable object; geometry supplies the prepared enclosure, independently of mass. |
| Parent occurrence | Parent value identity, ordinal, exact positive count | Every ordered contribution, including repetitions; counts sum to the composite's structural mass. Atomic definitions can have no parents. |
| Live occurrence | Occurrence identity, definition identity, preassigned slot, study-node binding, position and velocity | Independent mutable state for each use of a definition. Position denotes the object centroid, in the bound node's spatial units. |
| Study-node binding | Native SNode, preceding parent binding, explicit scale role | Primary structure expresses the study; primary steps multiply by 64, secondary compression/naming inherit 1:1. |
| Exposure selection | Aggregate occurrence, disjoint declared part occurrences, aggregate/parts choice | Exactly one representation contributes; exact aggregate/part mass agrees. |
| Contact | Two selected occurrence identities | Explicit contact coverage; inactive occurrences and duplicate/self pairs are rejected. |

Definition and parent identities are opaque strings in this bounded API.
An eventual loader must supply resolved cold addresses rather than debug
notation. It must preserve the store/address context; this API does not decide
formal cross-database routing. Occurrence ids are study-local identities, slots
are numerical addresses, and SNode ids identify schema nodes. No lifetime
across general recomposition is implied. The caller keeps bound layouts alive.

Preparation generates one whole-mass sibling field per definition and one
parent field per parent value. Parent fields span ordinal positions. Each
ordered parent occurrence produces its own edge and numerical share
`exact_count / exact_mass`. The host retains the exact counts and ordinals;
hot f32 values are computational approximations. No extra grouping by ordinal
or internal application point is introduced.

## First existing objects

Use the retained [encoding-floor inventory](../data/postgres/snapshots/2026-09-28-encoding-floor/MANIFEST.md):
16 mass-1 hex atoms and 256 mass-2 couplets already exist. The bounded native
fixture exercises the following structural mapping:

| Definition | Ordered parent occurrences | Live uses in the fixture |
|---|---|---|
| `01` | `0` at 0, count 1; `1` at 1, count 1 | `01-a` |
| `10` | `1` at 0, count 1; `0` at 1, count 1 | `10-a` |
| `11` | `1` at 0, count 1; `1` at 1, count 1 | `11-a`, `11-b` |

These labels are fixture aliases for cold definitions, not new cold addresses.
Parent `0` contributes mass 2 and parent `1` mass 6 across the four instances;
the repeated `11` sibling field contributes mass 4. The two `11` instances
have independent positions, forces and slots. A deduplicated reverse child
link never replaces the two ordered parent rows.

The snapshot's couplet `token_parent.mass` cells are NULL. A later projection
must explicitly resolve the count of each known mass-1 parent; it must not
convert NULL to zero or assert those cells are already populated. The fixture
supplies counts of 1 from that known atomic structure without writing the store.
Other composed definitions need their own counted construction before staging.

The three provisional group labels have assigned mass but incomplete naming
structures. This slice does not turn them into constructed physical definitions.
The addressed mass-zero `0x` record also remains a cold record; an unused hot
slot is zero state without an active occurrence. Those meanings are distinct.
Whitespace gaps and any optional local separator require their separately
specified representation; free slots do not silently encode whitespace.

## Geometry and publication

Object definitions provide component layouts. The supported enclosure is
`max(distance(component centre, supplied centroid) + component radius)`.
The [executed fixture](../engine/tests/assembly_test.cpp) supplies two spheres
at x = -0.25 and +0.25, each radius 0.4, around centroid zero. Its enclosing
radius is 0.65 before the 64 conversion. These are test inputs, not selected
packing for stored hex objects or a resolution of the single/double/triple/
quadruple examples. Actual object geometry can be supplied without altering
the parent-response rule.

Prepared state converts positions, velocities and radii into one frame and
seeds centroids there. Mass and identities are unchanged. An unchanged study
reuses its prepared extent each tick. Publication/refresh recomputes from the
supplied target and requires the fixed array counts; it replaces target state
explicitly rather than automatically inheriting old moving state.

The native tests cover initial/refreshed first force, cross-position fields,
repeated instances, 1:1 compression, aggregate/part exclusion, contact and inert
free slots. [Results and settings](../engine/docs/OBJECT-SUPPORT-RESULTS.md)
are recorded. This establishes the engine-facing object record slice needed
for the next data-modeling work. Completing the naming/language definitions,
English store scope and formal cross-database links remains the intended
discussion before additive persistent insertion.
