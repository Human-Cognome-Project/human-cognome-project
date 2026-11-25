#!/usr/bin/env bash
# Run the existing live CLI under its retained publishing owner.
set -euo pipefail
out=${1:?evidence directory required}
shift
: "${HCP_WARM_TEST_CONNINFO:?}"
exit_mode=${HCP_RETAINED_LIVE_EXIT:-quit}
case "$exit_mode" in
  quit|idle-stop|output-disconnect|batch-stop|offer-stop) ;;
  *) exit 2 ;;
esac
mkdir -p "$out"
step=freshness
pid=
writer=
reader=
finish() {
  local status=$?
  trap - EXIT
  if [[ -n $pid ]]; then
    if [[ -n $writer ]]; then exec {writer}>&-; fi
    if [[ -n $reader ]]; then exec {reader}<&-; fi
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
coproc STUDY { exec "$@" --retained --response-work-limit 1000000 2>"$out/live.stderr"; }
pid=$STUDY_PID
study_read=${STUDY[0]}
study_write=${STUDY[1]}
exec {reader}<&"$study_read" {writer}>&"$study_write"
exec {study_read}<&- {study_write}>&-
receive() {
  IFS= read -r -t "${2:-300}" -u "$reader" event
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
json_number() {
  local key=$1
  printf '%s\n' "$event" | sed -nE "s/.*\"$key\":([0-9]+)[,}].*/\1/p"
}
wait_for_exit() {
  local expected=$1 attempt status=0
  for ((attempt=0; attempt<300; ++attempt)); do
    if ! kill -0 "$pid" 2>/dev/null; then
      wait "$pid" || status=$?
      pid=
      test "$status" -eq "$expected"
      return
    fi
    sleep 0.1
  done
  echo 'live study remained running with command input open' >&2
  return 1
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
if [[ $exit_mode == offer-stop ]]; then
  step=offer-stop
  kill -TERM "$pid"
  wait_for_exit 0
  test "$(state)" = '2|1|0'
  exec {writer}>&- {reader}<&-
  step=complete
  echo 'PASS: first stop released the offer wait with input open and no acquired lease'
  exit 0
fi

step=acquisition
send open "$selection" "$revision"
receive '"status":"ok"'
receive '"command":"create"'
[[ "$event" == *'"actual_backend":"native_cpu"'* && "$event" == *'"completed_tick":0'* ]]
test "$(json_text selection)" = "$selection"
[[ "$event" == *"\"revision\":\"$revision\""* ]]
study=$(json_text study)
generation=$(json_number generation)
[[ "$study" =~ ^[a-f0-9-]{36}$ ]]
test "$(state)" = '2|1|1'

expected_exit=0
if [[ $exit_mode == quit ]]; then
  step=malformed-command
  send add banana
  receive '"command":"add"'
  [[ "$event" == *'"status":"invalid_input"'* && "$event" == *'"completed_tick":0'* ]]
  test "$(json_text study)" = "$study"
  test "$(json_number generation)" = "$generation"
  test "$(state)" = '2|1|1'
  expected_exit=1
fi

step=tick
send tick 1
receive '"command":"tick"'
[[ "$event" == *'"completed_tick":1'* ]]
test "$(json_text study)" = "$study"

step=$exit_mode
case "$exit_mode" in
  quit) send quit ;;
  idle-stop) kill -TERM "$pid" ;;
  output-disconnect)
    exec {reader}<&-
    reader=
    expected_exit=1
    ;;
  batch-stop)
    send tick 1000000
    sleep 0.5
    kill -TERM "$pid"
    receive '"command":"tick"' 30
    [[ "$event" == *'"status":"stopped"'* ]]
    completed=$(json_number ticks)
    [[ $completed =~ ^[0-9]+$ && $completed -gt 0 && $completed -lt 1000000 ]]
    test "$(json_number completed_tick)" -eq "$((completed+1))"
    test "$(json_text study)" = "$study"
    ;;
esac
if [[ $exit_mode == output-disconnect ]]; then
  wait_for_exit "$expected_exit"
  grep -Fxq 'live study output closed' "$out/live.stderr"
else
  receive '"command":"recipe_release"' 30
  [[ "$event" == *'"acquisitions":1'* && "$event" == *'"releases":1'* ]]
  wait_for_exit "$expected_exit"
fi
# Query from a separate session before closing the command pipe.
test "$(state)" = '2|1|0'
exec {writer}>&-
if [[ -n $reader ]]; then exec {reader}<&-; fi

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
printf 'PASS: retained recipe %s path exited with no lease and input open\n' "$exit_mode"
