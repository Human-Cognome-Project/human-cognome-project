#!/usr/bin/env bash
# Package the native client into an existing owned PostgreSQL container.
# Creates and retains a fresh warm fixture. Cold sources are never reset.
# Arguments after the output directory pass through to the capture client.
set -euo pipefail
hcp_view_build=$(realpath "${1:?Pass engine build directory}")
hcp_view_container=${2:?Pass owned PostgreSQL container}
hcp_view_core=${3:?Pass retained core DB}
hcp_view_ling=${4:?Pass retained linguistics DB}
mkdir -p "${5:?Pass output directory}"
hcp_view_output=$(realpath "$5")
for hcp_view_id in "$hcp_view_container" "$hcp_view_core" "$hcp_view_ling"; do
  [[ "$hcp_view_id" =~ ^[A-Za-z0-9_-]+$ ]] || exit 2
done
hcp_view_repo=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
hcp_view_db="hcp_viewport_$(date -u +%Y%m%d%H%M%S)_${RANDOM}"
hcp_view_package="$hcp_view_build/$hcp_view_db"
mkdir -p "$hcp_view_package/libs" "$hcp_view_package/runtime"
cp "$hcp_view_build/viewport/viewport_letter_capture" "$hcp_view_package/"
ldd "$hcp_view_package/viewport_letter_capture" > "$hcp_view_package/libraries.txt"
if rg -q 'not found' "$hcp_view_package/libraries.txt"; then exit 2; fi
while IFS= read -r hcp_view_library; do
  cp -L "$hcp_view_library" "$hcp_view_package/libs/"
done < <(awk '/=> \// {print $3}' "$hcp_view_package/libraries.txt")
hcp_view_loader=$(awk '$1 ~ /\/ld[^/]*\.so/ {print $1}' "$hcp_view_package/libraries.txt")
test -n "$hcp_view_loader"
cp -L "$hcp_view_loader" "$hcp_view_package/libs/"
hcp_view_loader_name=$(basename "$hcp_view_loader")
cp -L "$hcp_view_build/taichi-runtime/"*.bc "$hcp_view_package/runtime/"
chmod -R a+rX "$hcp_view_package"
hcp_view_fingerprint() {
  local hcp_view_source
  for hcp_view_source in "$hcp_view_core" "$hcp_view_ling"; do
    docker exec --user postgres "$hcp_view_container" pg_dump --data-only \
      --no-owner --no-privileges --dbname "$hcp_view_source" \
      | sed -E '/^\\(un)?restrict /d' | sha256sum | cut -d ' ' -f 1
  done
}
hcp_view_fingerprint > "$hcp_view_output/cold-before.sha256"
hcp_view_verify() {
  local hcp_view_status=$?
  trap - EXIT
  hcp_view_fingerprint > "$hcp_view_output/cold-after.sha256"
  cmp "$hcp_view_output/cold-before.sha256" "$hcp_view_output/cold-after.sha256" || exit 1
  exit "$hcp_view_status"
}
trap hcp_view_verify EXIT
docker exec --user postgres "$hcp_view_container" createdb "$hcp_view_db"
docker exec -i --user postgres "$hcp_view_container" psql -X -q -v ON_ERROR_STOP=1 \
  --dbname "$hcp_view_db" -f - < "$hcp_view_repo/kernels/database/warm/schema.sql"
docker cp "$hcp_view_package" "$hcp_view_container:/tmp/$hcp_view_db"
docker exec --user postgres "$hcp_view_container" mkdir "/tmp/$hcp_view_db-output"
docker exec --user postgres \
  --env "HCP_WARM_TEST_CONNINFO=dbname=$hcp_view_db" \
  --env "HCP_CORE_TEST_CONNINFO=dbname=$hcp_view_core" \
  --env "HCP_LING_TEST_CONNINFO=dbname=$hcp_view_ling" \
  --env "TI_LIB_DIR=/tmp/$hcp_view_db/runtime" \
  --env "XDG_CACHE_HOME=/tmp/$hcp_view_db-cache" \
  "$hcp_view_container" "/tmp/$hcp_view_db/libs/$hcp_view_loader_name" \
  --library-path "/tmp/$hcp_view_db/libs" "/tmp/$hcp_view_db/viewport_letter_capture" \
  "/tmp/$hcp_view_db-output" "${@:6}" | tee "$hcp_view_output/native-checks.txt"
docker cp "$hcp_view_container:/tmp/$hcp_view_db-output/." "$hcp_view_output/"
printf '%s\n' "$hcp_view_db" > "$hcp_view_output/warm-fixture.txt"
printf 'Retained warm fixture: %s\nCapture output: %s\n' "$hcp_view_db" "$hcp_view_output"
