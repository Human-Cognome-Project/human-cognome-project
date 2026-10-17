#!/usr/bin/env bash
# Native client + matched runtime/libraries in an existing owned PG container.
# Retained cold stores are read-only; the only DB created is a new warm fixture.
set -euo pipefail
hcp_active_build=$(realpath "${1:?Pass engine build directory}")
hcp_active_container=${2:?Pass owned PostgreSQL container}
hcp_active_core=${3:?Pass retained core DB}
hcp_active_ling=${4:?Pass retained Linguistics DB}
for hcp_active_identifier in "$hcp_active_container" "$hcp_active_core" "$hcp_active_ling"; do
  [[ "$hcp_active_identifier" =~ ^[A-Za-z0-9_-]+$ ]] || exit 2
done
hcp_active_repo=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
hcp_active_db="hcp_warm_d_$(date -u +%Y%m%d%H%M%S)_${RANDOM}"
hcp_active_evidence="$hcp_active_build/acceptance-$hcp_active_db"
mkdir -p "$hcp_active_evidence/package/libs" "$hcp_active_evidence/package/runtime"
cp "$hcp_active_build/warm_active_integration" "$hcp_active_evidence/package/"
ldd "$hcp_active_build/warm_active_integration" > "$hcp_active_evidence/libraries.txt"
if rg -q 'not found' "$hcp_active_evidence/libraries.txt"; then
  cat "$hcp_active_evidence/libraries.txt"
  exit 2
fi
while IFS= read -r hcp_active_library; do
  cp -L "$hcp_active_library" "$hcp_active_evidence/package/libs/"
done < <(awk '/=> \// {print $3}' "$hcp_active_evidence/libraries.txt")
hcp_active_loader=$(awk '$1 ~ /\/ld[^/]*\.so/ {print $1}' "$hcp_active_evidence/libraries.txt")
test -n "$hcp_active_loader"
cp -L "$hcp_active_loader" "$hcp_active_evidence/package/libs/"
hcp_active_loader_name=$(basename "$hcp_active_loader")
cp -L "$hcp_active_build/taichi-runtime/"*.bc "$hcp_active_evidence/package/runtime/"
# Public executable dependencies must be readable by the container postgres UID.
chmod -R a+rX "$hcp_active_evidence/package"
hcp_active_fingerprint() {
  local hcp_active_source
  for hcp_active_source in "$hcp_active_core" "$hcp_active_ling"; do
    docker exec --user postgres "$hcp_active_container" pg_dump --data-only \
      --no-owner --no-privileges --dbname "$hcp_active_source" \
      | sed -E '/^\\(un)?restrict /d' | sha256sum | cut -d ' ' -f 1
  done
}
hcp_active_fingerprint > "$hcp_active_evidence/cold-before.sha256"
hcp_active_verify_sources() {
  local hcp_active_status=$?
  trap - EXIT
  hcp_active_fingerprint > "$hcp_active_evidence/cold-after.sha256"
  cmp "$hcp_active_evidence/cold-before.sha256" "$hcp_active_evidence/cold-after.sha256" || exit 1
  exit "$hcp_active_status"
}
trap hcp_active_verify_sources EXIT
docker exec --user postgres "$hcp_active_container" createdb "$hcp_active_db"
docker exec -i --user postgres "$hcp_active_container" psql -X -q -v ON_ERROR_STOP=1 \
  --dbname "$hcp_active_db" -f - < "$hcp_active_repo/kernels/database/warm/schema.sql"
docker cp "$hcp_active_evidence/package" "$hcp_active_container:/tmp/$hcp_active_db"
docker exec --user postgres \
  --env "HCP_WARM_TEST_CONNINFO=dbname=$hcp_active_db" \
  --env "HCP_CORE_TEST_CONNINFO=dbname=$hcp_active_core" \
  --env "HCP_LING_TEST_CONNINFO=dbname=$hcp_active_ling" \
  --env "LD_LIBRARY_PATH=/tmp/$hcp_active_db/libs" \
  --env "TI_LIB_DIR=/tmp/$hcp_active_db/runtime" \
  --env "XDG_CACHE_HOME=/tmp/$hcp_active_db-cache" \
  "$hcp_active_container" "/tmp/$hcp_active_db/libs/$hcp_active_loader_name" \
  --library-path "/tmp/$hcp_active_db/libs" "/tmp/$hcp_active_db/warm_active_integration" \
  | tee "$hcp_active_evidence/native-checks.txt"
printf 'Retained warm fixture: %s\nEvidence: %s\n' "$hcp_active_db" "$hcp_active_evidence"
