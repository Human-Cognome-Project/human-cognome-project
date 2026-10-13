# Initial letter population writer

The implementation follows [PR #132's accepted plan](linguistic-bootstrap-population-plan.md).
It supplies database records for the current letter test objects and the prepared
punctuation inventory. It has no runtime selection or assembly functions.
Existing user stores remain unmodified; applying this implementation to them
requires its review and explicitly selected targets.

`tools/linguistics/populate.py` consumes the v2 manifest, accepted map and retained
[address ledger](../data/linguistics/letter-seed-ledger.json). The first allocation
has 2,707 components and 65 groups in root `02`. Existing ledger identities stay
fixed; allocation appends new entries after existing slots. The writer handles
the reviewed initial batch and its identical reruns, not arbitrary incremental
edits to populated stores. Groups carry NULL mass and no construction parents.

## Schema and mass

Create Linguistics with [the reviewed passive schema](../kernels/database/schema/schema.sql),
not the populated core's trigger definitions. The writer checks column types,
nullability, address collation, primary indexes, validated foreign keys, rules
and user triggers before writing. It accepts the exact retained core mass trigger
or the passive schema's absence of that trigger, and preserves whichever is present.
Linguistics must have no user triggers. Triggers are never disabled.

Inside the two store transactions, it drops exactly these destination keys:

```sql
-- Core
ALTER TABLE public.token_child DROP CONSTRAINT token_child_child_token_id_fkey;
-- Linguistics
ALTER TABLE public.token_parent DROP CONSTRAINT token_parent_parent_token_id_fkey;
```

Owner-token foreign keys and all membership foreign keys remain. The bootstrap
validates cross-store destinations instead of creating copies or placeholders.
Each component's explicitly written whole mass sums verified actual core masses,
counting every ordered occurrence. Parent-row mass cells remain NULL. Exact
notation, scalar order, case and combining sequences are preserved.

## Access and recovery

Both databases must remain offline for every other mutation path. `--exclusive-bootstrap`
records the operator's explicit commitment to this restriction. The writer rejects
other writable login roles, other connected clients, unsupported transaction
configuration and unrelated prepared work. A core advisory lock excludes another
bootstrap invocation; exclusive table locks protect validation and staged writes.
The same bootstrap owner can bypass this procedure, so that owner's connection
must not be shared with the unchanged native controller or other writers after
population. This is an operational restriction, not a new runtime enforcement service.

The private journal uses atomic replacement plus file and directory fsync. It
retains input/ledger hashes, batch identity, database/cluster identities, prepared
transaction IDs, schema/data baselines and the commit/abort decision. No passwords
or connection strings are retained. Preserve the journal, original inputs and
ledger until the batch and all its prepared transactions have completed.

Restart the **same command with the same inputs, journal and targets** after an
interruption. Recovery happens before ordinary conflict checks. Without a durable
commit decision, it rolls back that batch's prepared work, checks restoration of
both data and schema, then retries the same batch. After the commit decision it
completes both stores; it never rolls back one side. Missing prepared IDs may mean
the corresponding commit already finished, so recovery proves completion by
exact read-back. It leaves unrelated prepared transactions untouched. Completion
is reported only after both stores pass read-back. An unchanged rerun adds zero rows.

## Reproduction

Generate the candidate and compare its allocation to the retained ledger:

```bash
python tools/linguistics/import_inventory.py preview \
  --profile tools/linguistics/bootstrap-locales.json \
  --out sources/data/linguistics/population-input
python tools/linguistics/populate.py allocate \
  --manifest sources/data/linguistics/population-input/manifest.json \
  --ledger sources/data/linguistics/population-input/ledger.json
cmp sources/data/linguistics/population-input/ledger.json data/linguistics/letter-seed-ledger.json
```

For isolated acceptance, supply the current October 3 full UTF-8 gzip export:

```bash
bash tools/linguistics/start_validation_postgres.sh sources/data/linguistics/postgres-tools
python tools/linguistics/verify_population.py \
  --psql "$PWD/sources/data/linguistics/postgres-tools/psql-prepared" \
  --unsupported-psql "$PWD/sources/data/linguistics/postgres-tools/psql-unsupported" \
  --snapshot /path/to/current/hcp_core.sql.gz \
  --manifest sources/data/linguistics/population-input/manifest.json \
  --work sources/data/linguistics/population-checks \
  --out sources/data/linguistics/population-checks/acceptance.json --full-restore
```

The harness verifies the export's accepted SHA256, creates uniquely named empty
databases and preserves them. Most failure cases use a closed subset of actual
core character definitions; unrelated encoding-group memberships are omitted
from those fixtures. `--full-restore` separately restores the complete snapshot
into a new empty fixture, retaining its original trigger and memberships. No
database is reset, dropped or truncated. The helper creates private containers
and volumes; it does not remove existing infrastructure.

After implementation review and target selection, the load command is:

```bash
python tools/linguistics/populate.py load \
  --manifest sources/data/linguistics/population-input/manifest.json \
  --ledger data/linguistics/letter-seed-ledger.json \
  --core-db SELECTED_CORE --linguistics-db SELECTED_LINGUISTICS \
  --journal /durable/private/path/letter-batch.json \
  --out sources/data/linguistics/population-result.json --exclusive-bootstrap
```

Names are explicit PostgreSQL database names. Standard PostgreSQL environment
configuration selects the host/user; optional `--psql` selects a socket-only
wrapper for the owned test cluster. The writer does not create target databases,
restore snapshots, configure the server or grant itself access. Source generation
and validation are authorized parts of this bootstrap; these Python helpers are
not reusable runtime database or WAL components.
