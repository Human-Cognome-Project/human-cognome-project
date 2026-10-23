#!/usr/bin/env bash
# Create an isolated fixture. Never migrate a retained user warm/cold store.
set -euo pipefail
hcp_v2_build=$(realpath "${1:?Pass native warm build directory}")
hcp_v2_container=${2:?Pass owned PostgreSQL container}
hcp_v2_core=${3:?Pass retained core DB}
hcp_v2_ling=${4:?Pass retained Linguistics DB}
for hcp_v2_identifier in "$hcp_v2_container" "$hcp_v2_core" "$hcp_v2_ling"; do
  [[ "$hcp_v2_identifier" =~ ^[A-Za-z0-9_-]+$ ]] || exit 2
done
hcp_v2_source=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
hcp_v2_db="hcp_warm_v2_$(date -u +%Y%m%d%H%M%S)_${RANDOM}"
hcp_v2_evidence="$hcp_v2_build/acceptance-$hcp_v2_db"
mkdir -p "$hcp_v2_evidence"
hcp_v2_psql() {
  docker exec -i --user postgres "$hcp_v2_container" psql -X -v ON_ERROR_STOP=1 --dbname "$hcp_v2_db" "$@"
}
hcp_v2_migrate() {
  hcp_v2_psql -f - < "$hcp_v2_source/migrations/002-structural-recipes.sql"
}
hcp_v2_fingerprint_cold() {
  local hcp_v2_target
  for hcp_v2_target in "$hcp_v2_core" "$hcp_v2_ling"; do
    docker exec --user postgres "$hcp_v2_container" pg_dump --data-only --no-owner --no-privileges \
      --dbname "$hcp_v2_target" | sed -E '/^\\(un)?restrict /d' | sha256sum
  done
}
hcp_v2_fingerprint_cold > "$hcp_v2_evidence/cold-before.sha256"
docker exec --user postgres "$hcp_v2_container" createdb "$hcp_v2_db"
hcp_v2_psql -f - < "$hcp_v2_source/schema.sql" > "$hcp_v2_evidence/schema-v1.txt"
docker cp "$hcp_v2_build/structural_postgres_test" "$hcp_v2_container:/tmp/$hcp_v2_db-test"
hcp_v2_test() {
  docker exec --user postgres --env "HCP_WARM_TEST_CONNINFO=dbname=$hcp_v2_db" \
    --env "HCP_CORE_TEST_CONNINFO=dbname=$hcp_v2_core" \
    --env "HCP_LING_TEST_CONNINFO=dbname=$hcp_v2_ling" \
    "$hcp_v2_container" "/tmp/$hcp_v2_db-test" "$@"
}
hcp_v2_test seed | tee "$hcp_v2_evidence/pre-migration.txt"
# Capture the original table set, so new empty tables/marker metadata cannot
# conceal a change to any retained v1 row in the migration fingerprint.
mapfile -t hcp_v2_old_tables < <(hcp_v2_psql -At -c \
  "SELECT schemaname||'.'||tablename FROM pg_tables WHERE schemaname='warm_toolbox' ORDER BY tablename")
hcp_v2_dump_args=()
for hcp_v2_table in "${hcp_v2_old_tables[@]}"; do
  hcp_v2_dump_args+=(--table "$hcp_v2_table")
done
hcp_v2_fingerprint_legacy() {
  docker exec --user postgres "$hcp_v2_container" pg_dump --data-only --no-owner --no-privileges \
    --dbname "$hcp_v2_db" "${hcp_v2_dump_args[@]}" | sed -E '/^\\(un)?restrict /d' | sha256sum
}
hcp_v2_fingerprint_legacy > "$hcp_v2_evidence/legacy-before.sha256"

# A reader pins the old schema. Upgrade must time out without partial DDL;
# the reader finishes normally, then the operator can retry.
hcp_v2_reader="hcp_v2_reader_$RANDOM"
docker exec --user postgres --env "PGAPPNAME=$hcp_v2_reader" "$hcp_v2_container" \
  psql -X -v ON_ERROR_STOP=1 --dbname "$hcp_v2_db" \
  -c 'BEGIN; SELECT count(*) FROM warm_toolbox.bundle_revision; SELECT pg_sleep(10); COMMIT;' \
  > "$hcp_v2_evidence/concurrent-reader.txt" &
hcp_v2_reader_pid=$!
hcp_v2_locked=false
for hcp_v2_attempt in {1..50}; do
  if [ "$(hcp_v2_psql -At -c "SELECT count(*) FROM pg_locks l JOIN pg_stat_activity a USING(pid) WHERE a.application_name='$hcp_v2_reader' AND l.relation='warm_toolbox.bundle_revision'::regclass AND l.granted")" != 0 ]; then
    hcp_v2_locked=true
    break
  fi
  sleep 0.1
done
test "$hcp_v2_locked" = true
if hcp_v2_migrate > "$hcp_v2_evidence/reader-timeout.txt" 2>&1; then
  echo 'Migration unexpectedly passed a pinned reader' >&2
  exit 1
fi
wait "$hcp_v2_reader_pid"
grep -q 'lock timeout' "$hcp_v2_evidence/reader-timeout.txt"

# Inject a statement failure after all DDL, before commit. A real connection
# interruption has the same transaction rollback requirement.
if sed '/^COMMIT;$/i SELECT 1/0;' "$hcp_v2_source/migrations/002-structural-recipes.sql" \
  | hcp_v2_psql -f - > "$hcp_v2_evidence/interrupted-migration.txt" 2>&1; then
  echo 'Injected migration failure unexpectedly succeeded' >&2
  exit 1
fi
test "$(hcp_v2_psql -At -c "SELECT to_regclass('warm_toolbox.recipe_schema') IS NULL AND to_regclass('warm_toolbox.bundle_occurrence_v2') IS NULL")" = t
hcp_v2_fingerprint_legacy > "$hcp_v2_evidence/legacy-after-rollback.sha256"
cmp "$hcp_v2_evidence/legacy-before.sha256" "$hcp_v2_evidence/legacy-after-rollback.sha256"
hcp_v2_migrate > "$hcp_v2_evidence/migration.txt"
hcp_v2_migrate > "$hcp_v2_evidence/migration-retry.txt"
hcp_v2_fingerprint_legacy > "$hcp_v2_evidence/legacy-after.sha256"
cmp "$hcp_v2_evidence/legacy-before.sha256" "$hcp_v2_evidence/legacy-after.sha256"
hcp_v2_test | tee "$hcp_v2_evidence/post-migration.txt"
docker cp "$hcp_v2_build/recipe_reader_probe" "$hcp_v2_container:/tmp/$hcp_v2_db-reader"
docker exec --user postgres --env "HCP_WARM_TEST_CONNINFO=dbname=$hcp_v2_db" \
  "$hcp_v2_container" "/tmp/$hcp_v2_db-reader" read-v2 | tee "$hcp_v2_evidence/current-reader.txt"
if [ -n "${HCP_WARM_LEGACY_READER:-}" ]; then
  docker cp "$HCP_WARM_LEGACY_READER" "$hcp_v2_container:/tmp/$hcp_v2_db-legacy-reader"
  docker exec --user postgres --env "HCP_WARM_TEST_CONNINFO=dbname=$hcp_v2_db" \
    "$hcp_v2_container" "/tmp/$hcp_v2_db-legacy-reader" reject-v2 \
    | tee "$hcp_v2_evidence/legacy-reader.txt"
fi
hcp_v2_fingerprint_cold > "$hcp_v2_evidence/cold-after.sha256"
cmp "$hcp_v2_evidence/cold-before.sha256" "$hcp_v2_evidence/cold-after.sha256"
printf 'Migration rollback/retry/readers and complete cold/v1 fingerprints passed.\nRetained fixture: %s\nEvidence: %s\n' "$hcp_v2_db" "$hcp_v2_evidence"
