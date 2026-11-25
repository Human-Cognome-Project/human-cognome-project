#!/usr/bin/env bash
# Run the existing live CLI under its retained publishing owner.
set -euo pipefail
out=${1:?evidence directory required}
shift
: "${HCP_WARM_TEST_CONNINFO:?}"
mkdir -p "$out"
step=freshness
pid=
writer=
finish() {
  local status=$?
  trap - EXIT
  if [[ -n $pid ]]; then
    if [[ -n $writer ]]; then exec {writer}>&-; fi
    wait "$pid" || true
  fi
  printf 'last_step=%s\nexit_code=%s\n' "$step" "$status" > "$out/result.txt"
  exit "$status"
}
trap finish EXIT
warm_sql() {
  PGOPTIONS='-c default_transaction_read_only=on' psql -X -qAt -v ON_ERROR_STOP=1 \
    --dbname="$HCP_WARM_TEST_CONNINFO" -c "$1"
}
state() {
  warm_sql 'SELECT (SELECT count(*) FROM warm_toolbox.root_binding),
    (SELECT count(*) FROM warm_toolbox.bundle_revision),
    (SELECT count(*) FROM warm_toolbox.bundle_lease)'
}
initial=$(state)
test "$initial" = '0|0|0'
step=offer
coproc STUDY { "$@" --retained --response-work-limit 1000000 2>"$out/live.stderr"; }
pid=$STUDY_PID
study_read=${STUDY[0]}
study_write=${STUDY[1]}
exec {reader}<&"$study_read" {writer}>&"$study_write"
exec {study_read}<&- {study_write}>&-
receive() {
  IFS= read -r -t 300 -u "$reader" event
  printf '%s\n' "$event" >> "$out/live.jsonl"
  [[ "$event" == *"${1:?expected event text}"* ]]
}
send() {
  printf '%s\n' "$*" >&"$writer"
}
json_text() {
  local key=$1
  printf '%s\n' "$event" | sed -nE "s/.*\"$key\":\"([^\"]+)\".*/\1/p"
}
receive '"command":"recipe_available"'
selection=$(json_text selection)
revision=$(json_text revision)
[[ "$selection" =~ ^[a-f0-9-]{36}$ && "$revision" =~ ^[a-f0-9-]{36}$ ]]
step=refusal
send open "$selection"
receive '"status":"invalid_input"'
send open "$selection" 00000000-0000-0000-0000-000000000000
receive '"status":"unavailable"'
test "$(state)" = '2|1|0'

step=acquisition
send open "$selection" "$revision"
receive '"status":"ok"'
receive '"command":"create"'
[[ "$event" == *'"actual_backend":"native_cpu"'* && "$event" == *'"completed_tick":0'* ]]
test "$(json_text selection)" = "$selection"
[[ "$event" == *"\"revision\":\"$revision\""* ]]
study=$(json_text study)
[[ "$study" =~ ^[a-f0-9-]{36}$ ]]
test "$(state)" = '2|1|1'

step=tick
send tick 1
receive '"command":"tick"'
[[ "$event" == *'"completed_tick":1'* ]]
test "$(json_text study)" = "$study"

step=release
send quit
receive '"command":"recipe_release"'
[[ "$event" == *'"acquisitions":1'* && "$event" == *'"releases":1'* ]]
exec {writer}>&-
wait "$pid"
pid=
test "$(state)" = '2|1|0'

step=populated-store-refusal
fingerprint() {
  pg_dump --data-only --no-owner --no-privileges --dbname="$HCP_WARM_TEST_CONNINFO" \
    | sed -E '/^\\(un)?restrict /d' | sha256sum
}
fingerprint > "$out/warm-before-retry.sha256"
if "$@" --retained < /dev/null >"$out/retry.stdout" 2>"$out/retry.stderr"; then
  exit 1
fi
test ! -s "$out/retry.stdout"
fingerprint > "$out/warm-after-retry.sha256"
cmp "$out/warm-before-retry.sha256" "$out/warm-after-retry.sha256"
step=complete
echo 'PASS: exact retained recipe pin, refusal before acquisition, native tick and released lease on one study'
