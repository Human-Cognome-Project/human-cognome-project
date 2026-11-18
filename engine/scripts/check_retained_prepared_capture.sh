#!/usr/bin/env bash
# Current retained-source negative witness. A successful probe verifies refusal,
# never claims native publication/tick. Caller supplies a NEW v2 qualification DB.
set -Eeuo pipefail
evidence=${1:?Supply a new evidence directory}; shift
test "$#" -gt 0 # Remaining arguments are the native command/loader prefix.
: "${HCP_CORE_TEST_CONNINFO:?}" "${HCP_LING_TEST_CONNINFO:?}" "${HCP_WARM_TEST_CONNINFO:?}"
mkdir "$evidence"
step=freshness
finish() {
  local code=$?
  trap - EXIT
  printf 'exit_code=%s\nlast_step=%s\nscope=retained-source membership refusal; no completed native tick qualification\n' \
    "$code" "$step" > "$evidence/result.txt"
  exit "$code"
}
trap finish EXIT
read_sql() {
  # Connection values are never echoed or included in retained command logs.
  PGOPTIONS='-c default_transaction_read_only=on' psql --dbname="$1" -X -At -v ON_ERROR_STOP=1
}
warm_state() {
  read_sql "$HCP_WARM_TEST_CONNINFO" <<'SQL'
SELECT (SELECT version FROM warm_toolbox.recipe_schema),
 (SELECT count(*) FROM warm_toolbox.root_binding),
 (SELECT count(*) FROM warm_toolbox.tool_revision),
 (SELECT count(*) FROM warm_toolbox.bundle_revision),
 (SELECT count(*) FROM warm_toolbox.bundle_lease);
SQL
}
warm_state > "$evidence/warm-before.txt"
test "$(cat "$evidence/warm-before.txt")" = '2|0|0|0|0'
for source_conn in "$HCP_CORE_TEST_CONNINFO" "$HCP_LING_TEST_CONNINFO"; do
  read_sql "$source_conn" <<'SQL'
SHOW default_transaction_read_only;
SQL
done > "$evidence/cold-read-only.txt"
test "$(cat "$evidence/cold-read-only.txt")" = $'on\non'
step=retained-record
read_sql "$HCP_LING_TEST_CONNINFO" > "$evidence/retained-english.json" <<'SQL'
SELECT json_build_object('object','02.00.00.00.03','notation',t.notation,'mass',t.mass::text,
 'parents',(SELECT json_agg(json_build_object('object',array_to_string(p.parent_token_id,'.'),
   'ordinal',p.ordinal,'stored_mass',p.mass::text) ORDER BY p.ordinal)
   FROM public.token_parent p WHERE p.token_id=t.token_id),
 'memberships',(SELECT json_agg(json_build_object('object',array_to_string(m.group_token_id,'.'),
   'notation',g.notation,'mass',g.mass::text) ORDER BY m.group_token_id)
   FROM public.member_of m JOIN public.token g ON g.token_id=m.group_token_id WHERE m.token_id=t.token_id))
FROM public.token t WHERE t.token_id=ARRAY['02','00','00','00','03'];
SQL
# Independently identify the precise source mismatch. Keep the full record above
# even when another source shape fails this bounded expected-refusal witness.
read_sql "$HCP_LING_TEST_CONNINFO" > "$evidence/source-shape.txt" <<'SQL'
SELECT count(*)=1 AND bool_and(t.notation='A' AND t.mass=2)
 AND (SELECT count(*)=1 AND bool_and(p.parent_token_id=ARRAY['00','00','00','00','23']
   AND p.ordinal=0 AND p.mass IS NULL) FROM public.token_parent p
   WHERE p.token_id=ARRAY['02','00','00','00','03'])
 AND (SELECT array_agg(array_to_string(m.group_token_id,'.') ORDER BY m.group_token_id)=
   ARRAY['02.00.00.00.00','02.00.00.00.01'] AND bool_and(g.mass IS NULL)
   FROM public.member_of m JOIN public.token g ON g.token_id=m.group_token_id
   WHERE m.token_id=ARRAY['02','00','00','00','03'])
FROM public.token t WHERE t.token_id=ARRAY['02','00','00','00','03'];
SQL
test "$(cat "$evidence/source-shape.txt")" = t
step=actual-producer
printf '%q ' "$@" "$evidence/capture" --study prepared-english-pair --backend cpu --ticks 1 \
  --cpu-threads 1 --intent-work-limit 1000000 > "$evidence/command.txt"
printf '\n' >> "$evidence/command.txt"
set +e
"$@" "$evidence/capture" --study prepared-english-pair --backend cpu --ticks 1 \
  --cpu-threads 1 --intent-work-limit 1000000 > "$evidence/capture.stdout" 2> "$evidence/capture.stderr"
code=$?
set -e
printf '%s\n' "$code" > "$evidence/producer-exit-code.txt"
test "$code" = 1
grep -Fxq 'prepared English pair: membership comparison choices are unsupported' "$evidence/capture.stderr"
test -d "$evidence/capture"
test -z "$(find "$evidence/capture" -type f -name '*.json' -print)"
step=publication-state
warm_state > "$evidence/warm-after.txt"
# The real client configured 00/02 but stopped before publishing source pins or
# acquiring a lease. Do not label the absence of a lease as a completed release.
test "$(cat "$evidence/warm-after.txt")" = '2|2|0|0|0'
read_sql "$HCP_WARM_TEST_CONNINFO" > "$evidence/qualification.json" <<'SQL'
SELECT json_build_object('profile','prepared-english-pair',
 'outcome','verified_unsupported_memberships','producer_exit',1,
 'retained_tick_qualified',false,'pinned_recipe',null,'actual_claims',json_build_array(),
 'comparisons',json_build_array(),'native_generation',null,'completed_tick',null,
 'warm_publications',(SELECT count(*) FROM warm_toolbox.bundle_revision),
 'warm_leases',(SELECT count(*) FROM warm_toolbox.bundle_lease),
 'lease_release','not_applicable_no_acquisition');
SQL
step=complete
echo 'Verified retained English membership refusal; no recipe pin, native claims or completed tick.'
