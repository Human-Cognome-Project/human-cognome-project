#!/usr/bin/env bash
# Actual retained acquisition -> native tick -> release; NEW v2 warm DB only.
set -Eeuo pipefail
evidence=${1:?Supply a new evidence directory}
verifier=${2:?Supply the native readback checker}; shift 2
test "$#" -gt 0 # Native executable, optionally preceded by its matching loader.
: "${HCP_CORE_TEST_CONNINFO:?}" "${HCP_LING_TEST_CONNINFO:?}" "${HCP_WARM_TEST_CONNINFO:?}"
mkdir "$evidence"
step=freshness
finish() {
  local code=$?
  trap - EXIT
  printf 'exit_code=%s\nlast_step=%s\nscope=actual retained prepared acquisition/native tick/lease release\n' \
    "$code" "$step" > "$evidence/result.txt"
  exit "$code"
}
trap finish EXIT
read_sql() {
  PGOPTIONS='-c default_transaction_read_only=on' psql --dbname="$1" -X -qAt -v ON_ERROR_STOP=1
}
read_sql "$HCP_WARM_TEST_CONNINFO" > "$evidence/warm-before.txt" <<'SQL'
SELECT (SELECT version FROM warm_toolbox.recipe_schema),
 (SELECT count(*) FROM warm_toolbox.root_binding),
 (SELECT count(*) FROM warm_toolbox.tool_revision),
 (SELECT count(*) FROM warm_toolbox.bundle_revision),
 (SELECT count(*) FROM warm_toolbox.bundle_lease);
SQL
test "$(cat "$evidence/warm-before.txt")" = '2|0|0|0|0'
for source_conn in "$HCP_CORE_TEST_CONNINFO" "$HCP_LING_TEST_CONNINFO"; do
  read_sql "$source_conn" <<'SQL'
SHOW default_transaction_read_only;
SQL
done > "$evidence/cold-read-only.txt"
test "$(cat "$evidence/cold-read-only.txt")" = $'on\non'
step=actual-producer
printf '%q ' "$@" "$evidence/capture" --study prepared-retained-english-pair --backend cpu \
  --ticks 1 --cpu-threads 1 --intent-work-limit 1000000 > "$evidence/command.txt"
printf '\n' >> "$evidence/command.txt"
"$@" "$evidence/capture" --study prepared-retained-english-pair --backend cpu \
  --ticks 1 --cpu-threads 1 --intent-work-limit 1000000 > "$evidence/capture.stdout" 2> "$evidence/capture.stderr"
step=publication-state
read_sql "$HCP_WARM_TEST_CONNINFO" > "$evidence/warm-after.json" <<'SQL'
SET search_path=warm_toolbox,pg_catalog;
SELECT json_build_object('schema',(SELECT version FROM recipe_schema),
 'roots',(SELECT count(*) FROM root_binding),'leases',(SELECT count(*) FROM bundle_lease),
 'bundles',(SELECT json_agg(json_build_object('selection',b.selection,'revision',b.revision,
   'version',b.recipe_version,'head',h.revision)) FROM bundle_revision b JOIN bundle_head h USING(selection)),
 'tools',(SELECT json_agg(json_build_object('object',scope::text||'/'||array_to_string(address,'.'),
   'revision',revision,'epoch',epoch,'mass',mass::text) ORDER BY address) FROM tool_revision),
 'definitions',(SELECT json_agg(json_build_object('selection',selection,'bundle_revision',bundle_revision,
   'object',scope::text||'/'||array_to_string(address,'.'),'revision',revision) ORDER BY address) FROM bundle_definition),
 'memberships',(SELECT json_agg(json_build_object('object',scope::text||'/'||array_to_string(address,'.'),
   'revision',revision,'group',group_scope::text||'/'||array_to_string(group_address,'.'),
   'group_revision',group_revision) ORDER BY address,group_address) FROM tool_member_of),
 'parents',(SELECT json_agg(json_build_object('object',scope::text||'/'||array_to_string(address,'.'),
   'parent',parent_scope::text||'/'||array_to_string(parent_address,'.'),'ordinal',ordinal,
   'stored_mass',stored_mass::text,'contribution',contribution::text) ORDER BY address,ordinal) FROM tool_parent));
SQL
step='native-readback-check'
native=("$@")
native[${#native[@]}-1]=$verifier
"${native[@]}" "$evidence" > "$evidence/checker.stdout" 2> "$evidence/checker.stderr"
step=complete
cat "$evidence/checker.stdout"
