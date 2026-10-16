# Native warm toolbox — Stage A

Implements the storage slice of the [accepted #137 contract](../../../docs/warm-toolbox-implementation-plan.md)
and [#140](https://github.com/Human-Cognome-Project/human-cognome-project/issues/140).
Accepted and merged in #141 at `e7acdea`. This module supplies native C++/libpq
storage and the shared value interface for supplied normalized recipes.
[Production projection/normalization (B)](../../../engine/docs/LETTER-PROJECTION.md)
is accepted and merged in #143 at `6ccd5a3`.
[Membership field generation (C)](../../../engine/docs/MEMBERSHIP-FIELDS.md)
is implemented for review under #144; full engine materialization (D) remains
queued in #139.

The library has no engine, Taichi or Python dependency. `values.h` defines scoped
object/tool/bundle references, exact ordered construction and membership records,
geometry and initial occurrences/selections/contacts. A bundle does not contain
live GPU slots or mutable engine state. The accepted version-1 layout is Root for
letter-only exposure, or Root plus its primary letter child when bytes are exposed.
Stage A checks that a supplied recipe follows that contract; it does not generate
the recipe, geometry, placement or an SNode layout. The shared `warm_values`
CMake target (`values.cmake`) contains only values/codec and is reused by B
without linking libpq or introducing an engine dependency into storage.

## Storage and operations

Apply `schema.sql` once to a **new separate PostgreSQL 16 database**. It creates
only the `warm_toolbox` schema, revokes public access, and fails if that schema
already exists. It contains PK/FK tables, no triggers or stored procedures and
no reverse-search indexes. Addresses remain paired Base62 `text[] COLLATE "C"`.
Never apply this schema to either retained cold database.

- `Administrator::configure` installs a complete trusted root directory in one
  transaction. Shared `00` and `02` must carry the same validated pair epoch;
  their individual snapshot hashes stay separate. Changing source/hash requires
  a new epoch; changing configuration requires a new policy version. Existing
  bindings cannot be removed or reassigned to another ownership kind.
- Personal `y*` and temporary `z*` are reserved first-symbol families. A concrete
  full-couplet binding, owning instance and permitted actors are required for
  writes; the reservation grants no address or delegation. `01` is unassigned.
  Test-only `y0`/`z0` bindings allocate no production subdivisions.
- `Toolbox` receives its trusted actor/instance/role at connection construction.
  Requests cannot choose those credentials. `prepare` validates the supplied
  complete recipe and captures binding versions in a coherent read transaction.
  The embedding owner creates these contexts and retains SQL credentials;
  this is not an analyst authentication or network service.
- `prepare(recipe, existing_roots)` explicitly reuses published `ToolRef`s.
  Omit those roots' bodies from the supplied recipe: the store reads their exact
  immutable dependency closure, preserves each original author and checks read
  scope. Any overlapping supplied dependency must match the pinned revision/body.
  Publication does not claim these addresses or retarget their heads. A different
  same-instance writer can publish a new bundle or authorized new tool referencing
  them; supplying a temporary body for creation/retry still requires its creator.
- `publish` rechecks binding/policy/epoch snapshots under shared row locks,
  serializes object preparation in sorted object order, reuses identical tools,
  then compares/locks the selection head. All tools, relationships, recipe rows,
  projected reciprocals and epoch return paths commit with that head in **one
  transaction**. A stale head is `Conflict`; errors roll back all new rows.
- `object_slot` is the one storage detail added to the plan's table list: a direct
  per-object publication lock plus a permanent temporary creator/revision claim.
  It prevents two preparation recipe keys from minting different bodies at the
  same temporary address. Identical retries belong to the same creator; a changed
  temporary construction needs a new explicitly authorized address.
- `acquire` recovers the full immutable recipe through the selection head, checks
  current source epochs/instance ownership, locks bindings before the head, and
  records an actor-owned consumer lease. A concurrent head change is `Conflict`
  for retry. A superseded source epoch is `Stale`. Published definition references
  are pinned consistently, and stored child/member return lists are verified
  against those exact definitions during read-back.
- `read_pinned` permits that consumer to finish an older revision even after a
  head/epoch change. `release` is owned and idempotent. All superseded revisions
  and their dependencies remain stored; no automatic expiry or deletion exists.
- `Administrator::affected` follows `(binding, old epoch)` return paths directly
  to dependent bundles. No full Linguistics scan or live-source WAL is needed.

Shared tools may reference only shared tools. Instance tools may reference shared
tools or the same instance. Bundles always have a local owner, including all-shared
studies. Personal/temporary reciprocals stay in the warm projection; no API writes
them into a shared source or promotes a private tool to a shared root. A temporary
writer can reuse cached shared dependencies but cannot create source-backed tools.

Canonical comparisons preserve source NULL parent-row mass separately from its
verified positive contribution, and NULL group mass separately from zero. Repeated
ordered parent contributions remain separate; projected child navigation pairs
deduplicate. Values exceeding signed 64-bit mass storage, malformed identities,
missing/repeated recipe parts, inconsistent dependency revisions, non-finite
initial values and embedded NUL text are rejected. Geometry receives structural
and finite-value checks here; engine enclosure/numerical conversion and hot-ready
validation remain D's boundary.
Finite real values, including subnormals and the smallest representable positive
double, survive publication and fresh reads. Coordinates, velocities, centroids
and radii share locale-independent serialization/parsing; invalid text, actual
overflow/underflow beyond the representable range, NaN and infinity are rejected.

## Read-only cold boundary

`ColdReader::read` follows one addressed token's attributes, ordered parents and
upward memberships in a repeatable read-only transaction. Server startup options
force read-only mode even when conninfo requests writable defaults. There is no
cold mutation method or generic SQL method on that public interface.

The trusted configuration owner binds source handles to actual immutable source
installations and attests their previously validated export hashes/pair epoch.
The store checks that attestation's consistency and stability; it does not infer
snapshot validity from a UUID or inspect arbitrary deployment files. Preparation
clients supply records obtained from those bindings. Live mutable sources and
automatic source refresh remain outside this slice.

## Build and reproduce acceptance

Requires C++17, CMake, libpq development headers/library (16+) and PostgreSQL 16.
Build outside the checkout:

```bash
cmake -S kernels/database/warm -B /tmp/hcp-warm-build -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/hcp-warm-build -j 2
```

The native acceptance client requires an explicitly selected **new** warm schema
and the retained populated #136 pair. It refuses a warm fixture with published
bundles; no routine test resets, drops or truncates any database/schema. Against
an already running owned PostgreSQL container containing that pair:

```bash
bash kernels/database/warm/run_container_acceptance.sh /tmp/hcp-warm-build \
  YOUR_OWNED_CONTAINER YOUR_RETAINED_CORE_DB YOUR_RETAINED_LINGUISTICS_DB
```

The helper creates a uniquely named warm database, applies the schema and runs
the native client over the container's local PostgreSQL socket. It hashes complete
data-only dumps of both cold stores before and after, excluding only pg_dump's
per-session restriction nonce. A difference fails acceptance. Evidence and the
fixture name are printed; databases and evidence remain for inspection.
Without Docker, create the new database/schema explicitly and run
`warm_postgres_test` with `HCP_WARM_TEST_CONNINFO`, `HCP_CORE_TEST_CONNINFO` and
`HCP_LING_TEST_CONNINFO` set through your connection configuration.

[acceptance.json](acceptance.json) records **90 passing checks** on PostgreSQL
16.15 against 27 retained source definitions, including real Dutch repeated
nibble construction and source NULLs. Cases cover two native writers with one
same-head winner/conflict, independent publications, late contact failure rollback,
fresh-client recovery of two English A uses with different exposures and complete
initial inputs, paired epoch adoption/staleness, retained owned pins, normalized
fixture validation, uint64 entry IDs, and positive scoped private/temporary use.
The review regressions cover a manager reusing another writer's temporary tool
without address creation rights, an authorized second temporary writer composing
a new tool from it, mixed-author dependency closure and protected original claims.
Very small finite values round-trip through every persisted real field and pinned
reads, including the smallest positive subnormal; invalid numeric text is rejected.
The cold data hashes match before/after. Engine/GPU execution is not part of A.

The [hosted workflow](../../../.github/workflows/warm-toolbox.yml) builds native
code, verifies the exact paired export hashes, restores only into new cold
fixtures and runs the same acceptance helper. The published exports are unchanged.
