#!/usr/bin/env bash
# Explicitly local PostgreSQL: new retained warm fixture, read-only cold stores.
set -Eeuo pipefail
build=$(realpath "${1:?Supply engine build directory}")
evidence=${2:?Supply new evidence directory}
repo=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
core=${HCP_LOCAL_CORE_SERVICE:-hcp-core}
ling=${HCP_LOCAL_LING_SERVICE:-hcp-linguistics}
warm=${HCP_LOCAL_WARM_SERVICE:-hcp-warm}
role=${HCP_LOCAL_PG_ROLE:-hcp_local}
for identifier in "$core" "$ling" "$warm" "$role"; do
  [[ "$identifier" =~ ^[A-Za-z_][A-Za-z0-9_-]*$ ]] || exit 2
done
[[ "$role" =~ ^[a-z_][a-z0-9_]*$ ]] || exit 2
mkdir "$evidence"
db="hcp_ci_$(date -u +%Y%m%d%H%M%S)_${RANDOM}"
admin() {
  if test -n "${HCP_LOCAL_PG_ADMIN_SERVICE:-}"; then
    PGSERVICE="$HCP_LOCAL_PG_ADMIN_SERVICE" psql -X -v ON_ERROR_STOP=1 "$@"
  else
    # Only this explicit local mode uses the local OS postgres administrator.
    test "$(id -u)" = 0 || { echo 'Supply HCP_LOCAL_PG_ADMIN_SERVICE or run as root locally.'; return 2; }
    runuser -u postgres -- psql -X -v ON_ERROR_STOP=1 "$@"
  fi
}
fingerprint() {
  local service
  for service in "$core" "$ling"; do
    PGOPTIONS='-c default_transaction_read_only=on' pg_dump \
      --dbname="service=$service" --data-only --no-owner --no-privileges |
      sed -E '/^\\(un)?restrict /d' | sha256sum | cut -d ' ' -f 1
  done
}
fingerprint > "$evidence/cold-before.sha256"
finish() {
  local result=$?
  trap - EXIT
  fingerprint > "$evidence/cold-after.sha256" || result=1
  cmp "$evidence/cold-before.sha256" "$evidence/cold-after.sha256" || result=1
  printf 'database=%s\nexit_code=%s\n' "$db" "$result" > "$evidence/result.txt"
  exit "$result"
}
trap finish EXIT
printf '%s\n' "$db" > "$evidence/retained-database.txt"
admin -d postgres -c "CREATE DATABASE $db OWNER $role" > "$evidence/create.log" 2>&1
# Feed SQL on stdin so the OS administrator need not traverse a private checkout.
{ printf 'SET ROLE %s;\n' "$role"; cat "$repo/kernels/database/warm/schema.sql"; } |
  admin -d "$db" > "$evidence/schema.log" 2>&1
# shellcheck source=/dev/null
source "$build/engine-env.sh"
export HCP_CORE_TEST_CONNINFO="service=$core"
export HCP_LING_TEST_CONNINFO="service=$ling"
export HCP_WARM_TEST_CONNINFO="service=$warm dbname=$db"
"$build/warm_active_integration" > "$evidence/native-checks.txt" 2>&1
printf 'Retained new fixture: %s\n' "$db"
