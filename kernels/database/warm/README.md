# Native warm toolbox — Stage A

Implements the storage slice of the [accepted #137 contract](../../../docs/warm-toolbox-implementation-plan.md)
and [#140](https://github.com/Human-Cognome-Project/human-cognome-project/issues/140).
Accepted and merged in #141 at `e7acdea`. This module supplies native C++/libpq
storage and the shared value interface for supplied normalized recipes.
[Production projection/normalization (B)](../../../engine/docs/LETTER-PROJECTION.md)
is accepted/merged in #143. C is reviewed with nine green checks in #146,
accepted and merged at `43f418a`. Design #147 is accepted/merged; #148 explicitly
releases bounded [D integration](../../../engine/docs/ACTIVE-WORKBENCH.md).

**Ownership correction (Patrick, 2026-10-16):** the cache manager handles shared
tools only. Analyst-private/local inputs enter active hot assembly through a
separate boundary and never use manager source lookup, warm publication,
acquisition, refresh or reporting. Exclude private DB codes/handles from the
manager's request/source scope; reject attempts before lookup, without content
inspection/filtering or private-content/code logging. Generic personal-storage
operations/tests below are built capabilities, not authority for a manager
private-data pipeline. Temporary ownership and retained fixtures are unchanged.
`shared_manager.h` adds the manager-facing shared-scope gate. The embedding
owner provides a dedicated shared-only store and only public resolver credentials;
it must not delegate generic `Toolbox` access to request clients. Publication
tickets are minted by this public path, and fresh clients can recover them.
The separate active adapter has no manager/storage handle. Public recipe callers
remain responsible for supplying explicitly public inputs: the gate checks
authority before lookup, not whether a caller secretly derived a shared-key
placement from private information. See the [design](../../../docs/pre-d-hot-workbench-design.md).

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
studies. In the generic scoped-storage capability tests, personal/temporary
reciprocals stay in that isolated warm projection; this does not authorize
manager private-data integration. No API writes
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

### Without Docker

`run_container_acceptance.sh`, `engine/scripts/run_warm_active_acceptance.sh` and
`tools/viewport/run-capture.sh` all assume the cold stores live in a local Docker
container (`docker exec`). For a PostgreSQL server outside Docker (remote or
host-installed), run the same sequence by hand from a local client. Database
names and connection details are local configuration (a libpq `service=NAME`
from `~/.pg_service.conf`, password in `~/.pgpass`); configuration scripts are
not yet formalized, so this is manual. See the
[restore notes](../../../data/postgres/snapshots/2026-10-13-letter-seed-validation/MANIFEST.md#restoring-onto-an-existing-server)
for mounting the retained pair.

Run the steps in a shell with `set -euo pipefail`, as the repository scripts do;
otherwise a failed `pg_dump` hashes empty output and the before/after comparison
passes vacuously. Put the steps in a script, or use `set -o pipefail` alone in an
interactive shell (`set -e` closes an interactive session on the first failure).

1. Fingerprint both cold databases (complete data-only dumps, excluding only
   pg_dump's per-session restriction nonce) and keep the two hashes:

   ```bash
   pg_dump --data-only --no-owner --no-privileges --dbname="service=NAME dbname=CORE_DB" \
     | sed -E '/^\\(un)?restrict /d' | sha256sum
   # repeat for the Linguistics database
   ```

   A hash of `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`
   (SHA-256 of empty input) means the dump produced nothing and the check is
   invalid. A newer `pg_dump` client against an older 16.x server is fine.
2. Create a new, uniquely named warm database and apply the schema. The
   connecting role needs `CREATEDB` for this. The 2026-10-20 run used a role with
   `CREATEDB` (and superuser), so the minimum privileges needed on the cold
   databases were not separately tested; read-only access is not claimed to be
   sufficient.

   ```bash
   createdb --maintenance-db="service=NAME" hcp_warm_a_$(date -u +%Y%m%d%H%M%S)   # note the name
   psql -X -v ON_ERROR_STOP=1 --dbname="service=NAME dbname=WARM_DB" -f kernels/database/warm/schema.sql
   ```
3. Run the native client with the three connection settings. libpq conninfo
   accepts `service=NAME`, so these work:

   ```bash
   HCP_WARM_TEST_CONNINFO="service=NAME dbname=WARM_DB" \
   HCP_CORE_TEST_CONNINFO="service=NAME dbname=CORE_DB" \
   HCP_LING_TEST_CONNINFO="service=NAME dbname=LING_DB" \
     /tmp/hcp-warm-build/warm_postgres_test
   ```

   The engine's `warm_active_integration` client (built with
   `-DHCP_WITH_WARM_INTEGRATION=ON`) reads the same three variables.
4. Re-fingerprint both cold databases as in step 1 and compare with the
   earlier hashes; any difference fails acceptance.
