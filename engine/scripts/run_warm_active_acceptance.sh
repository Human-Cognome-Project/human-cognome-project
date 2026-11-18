#!/usr/bin/env bash
# Native client + matched runtime/libraries in an existing owned PG container.
# Retained cold stores are read-only; the only DB created is a new warm fixture.
set -euo pipefail
hcp_active_build=$(realpath "${1:?Pass engine build directory}")
hcp_active_container=${2:?Pass owned PostgreSQL container}
hcp_active_core=${3:?Pass retained core DB}
hcp_active_ling=${4:?Pass retained Linguistics DB}
hcp_active_mode=${5:-legacy}
hcp_active_program=warm_active_integration
hcp_active_program_path="$hcp_active_build/$hcp_active_program"
case "$hcp_active_mode" in
  legacy) ;;
  prepared-membership-refusal|prepared-retained-positive)
    hcp_active_program=viewport_letter_capture
    hcp_active_program_path="$hcp_active_build/viewport/$hcp_active_program" ;;
  *) echo 'Unknown acceptance mode' >&2; exit 2 ;;
esac
for hcp_active_identifier in "$hcp_active_container" "$hcp_active_core" "$hcp_active_ling"; do
  [[ "$hcp_active_identifier" =~ ^[A-Za-z0-9_-]+$ ]] || exit 2
done
hcp_active_repo=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
hcp_active_db="hcp_warm_d_$(date -u +%Y%m%d%H%M%S)_${RANDOM}"
hcp_active_evidence="$hcp_active_build/acceptance-$hcp_active_db"
mkdir -p "$hcp_active_evidence/package/libs" "$hcp_active_evidence/package/runtime"
cp "$hcp_active_program_path" "$hcp_active_evidence/package/"
ldd "$hcp_active_evidence/package/$hcp_active_program" > "$hcp_active_evidence/libraries.txt"
git -C "$hcp_active_repo" rev-parse HEAD > "$hcp_active_evidence/source-pin.txt"
if test "$hcp_active_mode" != legacy; then
  if test "$hcp_active_mode" = prepared-membership-refusal; then
    cp "$hcp_active_repo/engine/scripts/check_retained_prepared_capture.sh" "$hcp_active_evidence/package/"
  else
    cp "$hcp_active_repo/engine/scripts/check_retained_positive_capture.sh" "$hcp_active_evidence/package/"
    cp "$hcp_active_build/viewport/viewport_retained_capture_check" "$hcp_active_evidence/package/"
    ldd "$hcp_active_evidence/package/viewport_retained_capture_check" >> "$hcp_active_evidence/libraries.txt"
  fi
  hcp_active_snapshots="$hcp_active_repo/data/postgres/snapshots/2026-10-13-letter-seed-validation"
  printf '%s  %s\n' \
    f8019624a617a84410c0fd927b4947ed87955b48280016af8fb40b35eb8ac121 hcp_core.sql.gz \
    9784ed93d361de61bfb169d13a6e2fd9d2a58eb4dd1ac889b9dc6e0d09e32296 hcp_linguistics.sql.gz \
    > "$hcp_active_evidence/source-hashes.txt"
  (cd "$hcp_active_snapshots" && sha256sum --check "$hcp_active_evidence/source-hashes.txt") \
    > "$hcp_active_evidence/source-hashes-check.txt"
fi
# Fail closed on missing libraries or an unsuccessful check; use the portable
# awk already required below rather than an optional ripgrep installation.
if ! awk '/not found/ { missing = 1 } END { exit (missing ? 1 : 0) }' "$hcp_active_evidence/libraries.txt"; then
  cat "$hcp_active_evidence/libraries.txt"
  exit 2
fi
while IFS= read -r hcp_active_library; do
  cp -L "$hcp_active_library" "$hcp_active_evidence/package/libs/"
done < <(awk '/=> \// {print $3}' "$hcp_active_evidence/libraries.txt")
hcp_active_loader=$(awk '$1 ~ /\/ld[^/]*\.so/ {print $1; exit}' "$hcp_active_evidence/libraries.txt")
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
if test "$hcp_active_mode" != legacy; then
  docker exec -i --user postgres "$hcp_active_container" psql -X -q -v ON_ERROR_STOP=1 \
    --dbname "$hcp_active_db" -f - < "$hcp_active_repo/kernels/database/warm/migrations/002-structural-recipes.sql"
fi
docker cp "$hcp_active_evidence/package" "$hcp_active_container:/tmp/$hcp_active_db"
hcp_active_command=("/tmp/$hcp_active_db/libs/$hcp_active_loader_name"
  --library-path "/tmp/$hcp_active_db/libs" "/tmp/$hcp_active_db/$hcp_active_program")
# Only this native command uses the staged loader/library path. The surrounding
# container shell and PostgreSQL utilities must use their own runtime libraries.
if test "$hcp_active_mode" = prepared-membership-refusal; then
  hcp_active_command=(bash "/tmp/$hcp_active_db/check_retained_prepared_capture.sh"
    "/tmp/$hcp_active_db-probe" "${hcp_active_command[@]}")
elif test "$hcp_active_mode" = prepared-retained-positive; then
  hcp_active_command=(bash "/tmp/$hcp_active_db/check_retained_positive_capture.sh"
    "/tmp/$hcp_active_db-probe" "/tmp/$hcp_active_db/viewport_retained_capture_check" "${hcp_active_command[@]}")
fi
set +e
docker exec --user postgres \
  --env "HCP_WARM_TEST_CONNINFO=dbname=$hcp_active_db" \
  --env "HCP_CORE_TEST_CONNINFO=dbname=$hcp_active_core" \
  --env "HCP_LING_TEST_CONNINFO=dbname=$hcp_active_ling" \
  --env "LD_LIBRARY_PATH=" \
  --env "TI_LIB_DIR=/tmp/$hcp_active_db/runtime" \
  --env "XDG_CACHE_HOME=/tmp/$hcp_active_db-cache" \
  "$hcp_active_container" "${hcp_active_command[@]}" \
  | tee "$hcp_active_evidence/native-checks.txt"
hcp_active_result=$?
set -e
if test "$hcp_active_mode" != legacy; then
  docker cp "$hcp_active_container:/tmp/$hcp_active_db-probe" "$hcp_active_evidence/retained-probe" || hcp_active_result=1
fi
test "$hcp_active_result" = 0
printf 'Retained warm fixture: %s\nEvidence: %s\n' "$hcp_active_db" "$hcp_active_evidence"
