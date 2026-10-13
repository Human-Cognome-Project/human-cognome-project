#!/usr/bin/env bash
# Private PostgreSQL fixtures for verify_population.py; no listener or DB reset.
set -euo pipefail
hcp_validation_work=${1:?Pass an ignored working directory for psql wrappers}
mkdir -p "$hcp_validation_work"
hcp_validation_image=postgres@sha256:ca0bd484cb98bf4b24eb1010e73fb3fcbd6714d240fbc1a10eea5b7dbecb641d
hcp_validation_suffix="${GITHUB_RUN_ID:-local}-${RANDOM}-${RANDOM}"
docker image inspect "$hcp_validation_image" >/dev/null 2>&1 || docker pull "$hcp_validation_image"
for hcp_validation_mode in prepared unsupported; do
  hcp_validation_limit=16
  if [ "$hcp_validation_mode" = unsupported ]; then hcp_validation_limit=0; fi
  hcp_validation_container="hcp-letter-${hcp_validation_suffix}-${hcp_validation_mode}"
  docker run -d --name "$hcp_validation_container" --network none --user postgres \
    --volume "$hcp_validation_container:/var/lib/postgresql/data" \
    --env "HCP_PREPARED_LIMIT=$hcp_validation_limit" \
    --entrypoint bash "$hcp_validation_image" -c \
    'set -e; if [ ! -s "$PGDATA/PG_VERSION" ]; then initdb -D "$PGDATA" --encoding=UTF8 --locale=C.UTF-8 --auth-local=peer --auth-host=scram-sha-256 >/tmp/hcp-initdb.log; fi; exec postgres -D "$PGDATA" -c listen_addresses="" -c max_prepared_transactions="$HCP_PREPARED_LIMIT"' \
    >/dev/null
  hcp_validation_ready=false
  for hcp_validation_attempt in {1..30}; do
    if docker exec --user postgres "$hcp_validation_container" pg_isready --dbname postgres >/dev/null 2>&1; then
      hcp_validation_ready=true
      break
    fi
    sleep 1
  done
  test "$hcp_validation_ready" = true
  hcp_validation_wrapper="$hcp_validation_work/psql-$hcp_validation_mode"
  printf '#!/usr/bin/env bash\nset -euo pipefail\nexec docker exec -i --user postgres %s psql "$@"\n' \
    "$hcp_validation_container" > "$hcp_validation_wrapper"
  chmod 700 "$hcp_validation_wrapper"
done
printf 'Private PostgreSQL 16 fixtures ready; wrappers in %s\n' "$hcp_validation_work"
