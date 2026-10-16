#!/usr/bin/env bash
# Explicit existing owned cluster + retained sources; CREATE a new warm DB only.
set -euo pipefail
hcp_warm_build=$(realpath "${1:?Pass native warm build directory}")
hcp_warm_container=${2:?Pass the owned PostgreSQL container name}
hcp_warm_core=${3:?Pass retained core database name}
hcp_warm_ling=${4:?Pass retained linguistics database name}
for hcp_warm_identifier in "$hcp_warm_container" "$hcp_warm_core" "$hcp_warm_ling"; do
  [[ "$hcp_warm_identifier" =~ ^[A-Za-z0-9_-]+$ ]] || exit 2
done
hcp_warm_source=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
hcp_warm_database="hcp_warm_a_$(date -u +%Y%m%d%H%M%S)_${RANDOM}"
hcp_warm_evidence="$hcp_warm_build/acceptance-$hcp_warm_database"
mkdir -p "$hcp_warm_evidence"

hcp_warm_fingerprint() {
  local hcp_warm_db
  for hcp_warm_db in "$hcp_warm_core" "$hcp_warm_ling"; do
    # pg_dump 16.15's restriction nonce is session metadata, not source data.
    # Hash the complete data-only dump, including every table/COPY row.
    docker exec --user postgres "$hcp_warm_container" pg_dump --data-only \
      --no-owner --no-privileges --dbname "$hcp_warm_db" \
      | sed -E '/^\\(un)?restrict /d' | sha256sum | cut -d ' ' -f 1
  done
}
hcp_warm_fingerprint > "$hcp_warm_evidence/cold-before.sha256"
docker exec --user postgres "$hcp_warm_container" psql -X -v ON_ERROR_STOP=1 \
  --dbname postgres -c "CREATE DATABASE $hcp_warm_database"
docker exec -i --user postgres "$hcp_warm_container" psql -X -q -v ON_ERROR_STOP=1 \
  --dbname "$hcp_warm_database" -f - < "$hcp_warm_source/schema.sql"
docker cp "$hcp_warm_build/warm_postgres_test" "$hcp_warm_container:/tmp/$hcp_warm_database-test"
docker exec --user postgres \
  --env "HCP_WARM_TEST_CONNINFO=dbname=$hcp_warm_database" \
  --env "HCP_CORE_TEST_CONNINFO=dbname=$hcp_warm_core" \
  --env "HCP_LING_TEST_CONNINFO=dbname=$hcp_warm_ling" \
  "$hcp_warm_container" "/tmp/$hcp_warm_database-test" \
  | tee "$hcp_warm_evidence/native-checks.txt"
hcp_warm_fingerprint > "$hcp_warm_evidence/cold-after.sha256"
cmp "$hcp_warm_evidence/cold-before.sha256" "$hcp_warm_evidence/cold-after.sha256"
printf 'Complete cold data fingerprints unchanged. Retained warm fixture: %s\nEvidence: %s\n' \
  "$hcp_warm_database" "$hcp_warm_evidence"
