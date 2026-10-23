# Coordinate-free structural recipes

Segment 1 of the accepted [#170 plan](../../../docs/core-operation-readiness-plan.md),
implemented for review on 2026-10-23. This supplies versioned values, persistence,
projection and a structural input boundary. Engine admission, candidate ticks,
enduring owner refresh and the complete CPU/GPU handoff remain later segments.
The existing numerical engine and its v1 fixtures retain their behavior.

## Values and projection

| Surface | Retained v1 | Structural v2 |
|---|---|---|
| Occurrences | `Bundle::occurrences`, with original position/velocity | `Bundle::structural_occurrences`, with identity, definition and conceptual node only |
| Alternatives | Aggregate and every declared byte alternative | Same complete exact construction; inactive alternatives also have no starts |
| Focus | Supplied by legacy active input | Ordered `Bundle::focal_fields`, explicitly naming positive participating fields |
| Projection | `normalize_letters`, supplied `LetterRequest`/byte starts, `bind` → `StudyInput` | `normalize_structural_letters`, structural requests, `bind_structure` → `StructuralStudyInput` |
| Numerical slots | Legacy fixture slots | No slot assignments; projected legacy slot metadata is `-1` |
| Persistence | Original occurrence/selection/part/contact tables | Separate `_v2` tables and `bundle_focus_v2`; common immutable definitions, geometry, nodes and reciprocals |

The new request contains only entry ID, pinned letter reference and explicit
exposure. The normalizer derives every ordered byte alternative from the exact
source UTF-8 construction using the existing byte/nibble checks. It preserves
scoped definition/revision identity, NULL versus resolved source contributions,
repeated parents and distinct occurrences. It does not allocate cold addresses.

Intrinsic component spheres and their local centroid remain structural geometry.
They are not world placement or restart data. The bounded normalized frame has
Root and, when bytes are exposed, one primary conceptual step for aggregates.
V2 permits explicit secondary-compression/naming attachments at 1:1 without
changing that conceptual depth; deeper primary arrangements are rejected.
Native verification of actual SNodes and unit conversion retains its own boundary.

Focus must name available sibling-definition, parent-value or inherited membership
fields with active positive contributors. Reject missing, duplicate, inactive or
unknown selections. Ordered focal selection is persisted exactly. Membership
targets still deduplicate per occurrence; exposed bytes inherit their own letter's
groups. Shared-manager gating covers structural occurrences and focal objects
before preparation/lookup, as well as the existing definition/geometry surfaces.

`project_letters` supports both versions, preserving their separation:

- `bind` accepts only v1 and produces the original supplied-state fixture input.
- `bind_structure` accepts only v2. The structural input has no position, velocity,
  capacity or slot fields. Membership binding has the same explicit v2 route.
- Mixed v1/v2 vectors reject; the library does not silently discard supplied starts.
- `structural_from_v1(legacy, new_publication, focal_fields)` is the explicit
  conversion. It requires a distinct publication identity, copies structural
  content and discards starts in the returned v2 value. Original v1 values and
  persisted rows remain intact. Legacy starts never become implicit restart hints.

No v2-to-legacy adapter inserts zero starts. Operational owner submission and
engine initialization must consume the structural boundary in the later segments.
Optional owner-bound restart positions also remain separate future inputs.

## Schema and migration contract

The original [schema.sql](schema.sql) is unchanged and still installs v1 in a
new warm database. [Migration 002](migrations/002-structural-recipes.sql) is a
reviewable **psql** script, not automatic connection-time DDL. Only newly created,
isolated acceptance fixtures were migrated for this implementation. No retained
user warm store or cold database was migrated.

For an explicitly chosen warm connection, the migration command is:

```bash
psql -X -v ON_ERROR_STOP=1 --dbname="${HCP_WARM_CONNINFO:?Select the warm connection explicitly}" \
  -f kernels/database/warm/migrations/002-structural-recipes.sql
```

The complete upgrade is one transaction. It takes an exclusive bundle-header
table lock with a five-second lock timeout. Existing readers can finish; a busy
reader/writer can make the upgrade fail without changing the schema. Quiesce
writers and retry rather than treating a timeout as a completed upgrade. No
trigger, stored procedure, database reset or row conversion is introduced.

DDL adds v2 tables with PK/FK ownership and ordered focus, a version marker and
a version-qualified header key. It widens only the header's recipe-version check
from v1 to v1/v2. Original tables, NOT NULL numerical columns, starts, tool bodies,
publication heads and leases remain intact. The v2 occurrence table has no
numerical columns; its version-qualified FK cannot attach a structural row to a
v1 header. New tables retain the schema owner's access boundary with PUBLIC
privileges revoked; custom application roles require explicit owner grants.

Discovery is read-only: `Toolbox::schema_version()` returns 1 when the migration
marker is absent, 2 for the supported marker, and rejects unsupported metadata.
V2 preparation on v1 fails before publication. The reader dispatches by immutable
header version and rejects unknown versions. Older v1 readers reject a v2 head
without acquiring a lease and can continue reading their owned v1 pins.

Failure/disconnection before COMMIT rolls back the entire DDL and marker.
After success, rerunning 002 is a checked no-op under the same lock. Unsupported
marker versions fail explicitly. Publication remains a single transaction with
the existing exact-head/source/owner checks; two concurrent v2 writers cannot
publish different winners for one expected head. Superseded v1/v2 pins remain
readable under their original ownership rules.

## Reproduce compatibility checks

Build the warm targets and the pure projection tests using the existing native
instructions. The extended `run_container_acceptance.sh` runs the original warm
suite, then calls `run_structural_acceptance.sh` in a separate new fixture. The
latter can also run independently:

```bash
bash kernels/database/warm/run_structural_acceptance.sh /tmp/hcp-warm-build \
  YOUR_OWNED_CONTAINER YOUR_CORE_DB YOUR_LINGUISTICS_DB
```

It seeds real retained English A construction into an original-schema warm
fixture, holds a concurrent reader to check lock-timeout safety, injects a statement
failure immediately before migration COMMIT, verifies full rollback, retries the
upgrade and checks a repeated upgrade. It compares complete data-only fingerprints
of every original warm table before/after migration, including all original starts,
and complete cold fingerprints before/after the run. It retains the fixture,
original v1 pin and raw evidence. Database names are independent configuration.

Native checks cover v2 full readback, ordered focus, uint64 occurrence IDs,
geometry, exact construction, v1/v2 pinned coexistence, private-manager prelookup
rejection, late-failure rollback and competing v2 publishers. Pure projection
checks also cover inactive alternatives, repeated letters, exact multiscalar
byte paths, 1:1 role attachments and explicit legacy conversion.

`recipe_reader_probe.cpp` deliberately uses only the original v1 API. Compile it
against an archived `768e222` warm/codec source tree and its static libraries,
with that tree's **database include root**, to build a real historical reader.
Set `HCP_WARM_LEGACY_READER` to that executable when running the helper: it checks
the old owned pin and requires an explicit `unsupported recipe version` error
without a new lease on the v2 head. The normal probe, linked to the new library,
must read both. Do not mix old libraries with new C++ value headers; storage
compatibility does not promise cross-version C++ object-layout ABI compatibility.

[Acceptance evidence](structural-acceptance.json) records the native source,
commands, bounded results and retained fixture fingerprints. No GPU behavior,
pinning, natural packing or completed operational source-to-tick path is claimed.
