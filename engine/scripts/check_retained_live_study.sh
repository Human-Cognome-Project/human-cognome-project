#!/usr/bin/env bash
# Run the existing live CLI under its retained publishing owner.
set -euo pipefail
out=${1:?evidence directory required}
shift
: "${HCP_WARM_TEST_CONNINFO:?}"
exit_mode=${HCP_RETAINED_LIVE_EXIT:-quit}
case "$exit_mode" in
  quit|idle-stop|output-disconnect|batch-stop|offer-stop) ;;
  catalogue) set -- "$@" --letter 02.00.00.00.03 --letter 02.00.00.00.1W ;;
  batch-disconnect|blocked-write) : "${HCP_RETAINED_LIVE_PIPE_CHECK:?}" ;;
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
cpu_time() {
  local stat fields
  IFS= read -r stat < "/proc/$pid/stat"
  read -r -a fields <<< "${stat##*) }"
  printf '%s\n' "$((fields[11]+fields[12]))"
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
claim() {
  printf '%s\n' "$event" | sed -nE \
    's|.*(\{"address":[0-9]+,"context":[0-9]+,"id":"'"$study/$1"'","incarnation":[0-9]+,"record":[0-9]+),.*|\1|p'
}
if [[ $exit_mode == catalogue ]]; then
  step=catalogue-offer
  receive '"command":"catalogue_available"'
  offer_entry() {
    printf '%s\n' "$event" |
      sed -nE 's/.*\{"definition":"([^"]+)","entry":'"$1"',"revision":"([^"]+)"\}.*/\1 \2/p'
  }
  read -r a a_revision < <(offer_entry 1)
  read -r i i_revision < <(offer_entry 2)
  test "$a" != "$i"
  test "$(state)" = '2|1|1'
  send open "$a" 00000000-0000-0000-0000-000000000000
  receive '"status":"unavailable"'
  test "$(state)" = '2|1|1'
  send open "$a" "$a_revision"
  receive '"status":"ok"'

  step=catalogue-bootstrap
  receive '"command":"create"'
  study=$(json_text study)
  [[ "$study" =~ ^[a-f0-9-]{36}$ ]]
  test "$(state)" = '2|2|1'
  mass() {
    printf '%s\n' "$event" | tr ',' '\n' |
      sed -nE 's/^"exact_mass":"([0-9]+)"$/\1/p' | awk '{total+=$1} END {print total+0}'
  }
  responses() {
    printf '%s\n' "$event" |
      sed -nE 's/.*"response_trace":\{"capacity":"[^"]+","count":"([0-9]+)".*/\1/p'
  }
  observe_state() {
    test "$(json_text study)" = "$study"
    test "$(mass)" = "$1"
    rows=$(responses)
    [[ $rows =~ ^[1-9][0-9]*$ ]]
    printf '%s|%s|%s\n' "$step" "$1" "$rows" >> "$out/catalogue-transitions.txt"
  }
  observe_state 2
  original=$(claim 1)
  test -n "$original"
  test -z "$(claim 2)"
  initial_generation=$(json_number generation)

  step=catalogue-refusal
  send add 2 "$i" 00000000-0000-0000-0000-000000000000
  receive '"status":"invalid_input"'
  test "$(json_number generation)" = "$initial_generation"
  test "$(claim 1)" = "$original"
  test -z "$(claim 2)"

  step=catalogue-add
  send add 2 "$i" "$i_revision"
  receive '"status":"ok"'
  observe_state 6
  test "$(claim 1)" = "$original"
  test -n "$(claim 2)"
  send tick 1
  receive '"completed_tick":1'
  test "$(claim 1)" = "$original"

  step=catalogue-parts
  send lod parts
  receive '"status":"ok"'
  observe_state 6
  [[ "$event" == *"\"id\":\"$study/1/0/0\""*"\"id\":\"$study/2/0/0\""*"\"id\":\"$study/2/0/1\""* ]]
  test -n "$(claim 1/0/0)"
  test -n "$(claim 2/0/0)"
  test -n "$(claim 2/0/1)"

  step=catalogue-rollup
  send lod aggregate
  receive '"status":"ok"'
  observe_state 6
  rolled=$(claim 1)
  test -n "$rolled"
  test -n "$(claim 2)"

  step=catalogue-remove
  send remove 2
  receive '"status":"ok"'
  observe_state 2
  test "$(claim 1)" = "$rolled"
  test -z "$(claim 2)"
  send list
  receive '"command":"list"'
  [[ "$event" == *"\"definition\":\"$i\",\"entry\":2,\"id\":\"$study/2\",\"revision\":\"$i_revision\"}"* ]]
  step=catalogue-final-tick
  send tick 1
  receive '"completed_tick":2'
  observe_state 2
  test "$(claim 1)" = "$rolled"

  step=catalogue-reselect
  send add 3 "$i" "$i_revision"
  receive '"status":"ok"'
  observe_state 6
  test "$(claim 1)" = "$rolled"
  test -z "$(claim 2)"
  test -n "$(claim 3)"

  step=catalogue-release
  send quit
  receive '"command":"recipe_release"' 30
  [[ "$event" == *'"acquisitions":2'* && "$event" == *'"releases":2'* ]]
  wait_for_exit 1
  test "$(state)" = '2|2|0'
  exec {writer}>&- {reader}<&-
  step=complete
  echo 'PASS: retained catalogue edited and exchanged differently composed roots on one handle'
  exit 0
fi

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
  step=pin-edits
  read -r definition definition_revision < <(printf '%s\n' "$event" |
    sed -nE 's/.*"catalogue":\[\{"definition":"([^"]+)","entry":2,"id":"[^"]+","revision":"([^"]+)"\},.*/\1 \2/p')
  [[ $definition_revision =~ ^[a-f0-9-]{36}$ ]]
  first_claim=$(claim 2)
  second_claim=$(claim 10)
  test -n "$first_claim"
  test -n "$second_claim"

  refuse_pin() {
    send add 27 "$@"
    receive '"command":"add"'
    [[ "$event" == *'"status":"invalid_input"'* ]]
    test "$(json_number generation)" = "$generation"
    test "$(claim 2)" = "$first_claim"
    test "$(claim 10)" = "$second_claim"
    test -z "$(claim 27)"
  }
  refuse_pin "$definition" 00000000-0000-0000-0000-000000000000
  refuse_pin "00000000-0000-0000-0000-000000000001/${definition#*/}" "$definition_revision"

  send add 10 "$definition" "$definition_revision"
  receive '"command":"add"'
  [[ "$event" == *'"status":"no_change"'* ]]
  test "$(json_number generation)" = "$generation"

  step=malformed-command
  send add banana
  receive '"command":"add"'
  [[ "$event" == *'"status":"invalid_input"'* && "$event" == *'"completed_tick":0'* ]]
  test "$(json_text study)" = "$study"
  test "$(json_number generation)" = "$generation"
  test "$(state)" = '2|1|1'
  expected_exit=1

  step=add-by-pin
  send add 27 "$definition" "$definition_revision"
  receive '"command":"add"'
  [[ "$event" == *'"status":"ok"'* &&
     "$event" == *"\"definition\":\"$definition\",\"exact_mass\":\"2\",\"exposure\":\"aggregate\",\"id\":\"$study/27\""* ]]
  test "$(json_text study)" = "$study"
  test "$(claim 2)" = "$first_claim"
  test "$(claim 10)" = "$second_claim"
  added_claim=$(claim 27)
  test -n "$added_claim"
fi

step=tick
send tick 1
receive '"command":"tick"'
[[ "$event" == *'"completed_tick":1'* ]]
test "$(json_text study)" = "$study"
if [[ $exit_mode == quit ]]; then
  test "$(claim 27)" = "$added_claim"
  step=remove-by-entry
  send remove 27
  receive '"command":"remove"'
  [[ "$event" == *'"status":"ok"'* ]]
  test "$(claim 2)" = "$first_claim"
  test "$(claim 10)" = "$second_claim"
  test -z "$(claim 27)"

  send list
  receive '"command":"list"'
  [[ "$event" == *"\"catalogue\":[{\"definition\":\"$definition\",\"entry\":2,\"id\":\"$study/2\",\"revision\":\"$definition_revision\"}"* ]]
  send tick 1
  receive '"command":"tick"'
  [[ "$event" == *'"completed_tick":2'* ]]
  test "$(json_text study)" = "$study"
  test "$(state)" = '2|1|1'

  step=explicit-clone-pin
  send add 28 10
  receive '"command":"add"'
  [[ "$event" == *'"status":"ok"'* ]]
  clone_claim=$(claim 28)
  test -n "$clone_claim"
  clone_generation=$(json_number generation)

  send add 28 "$definition" "$definition_revision"
  receive '"command":"add"'
  [[ "$event" == *'"status":"no_change"'* ]]
  test "$(json_number generation)" = "$clone_generation"
  test "$(claim 28)" = "$clone_claim"

  send add 28 2
  receive '"command":"add"'
  [[ "$event" == *'"status":"invalid_input"'* ]]
  test "$(json_number generation)" = "$clone_generation"
  test "$(claim 28)" = "$clone_claim"

  send tick 1
  receive '"command":"tick"'
  [[ "$event" == *'"completed_tick":3'* ]]
  test "$(claim 28)" = "$clone_claim"
  send remove 28
  receive '"command":"remove"'
  [[ "$event" == *'"status":"ok"'* ]]
  test -z "$(claim 28)"

  step=restore-explicit-clone-by-pin
  send add 28 "$definition" "$definition_revision"
  receive '"command":"add"'
  [[ "$event" == *'"status":"ok"'* ]]
  restored_claim=$(claim 28)
  test -n "$restored_claim"
  test "$restored_claim" != "$clone_claim"
  restore_generation=$(json_number generation)

  send add 28 10
  receive '"command":"add"'
  [[ "$event" == *'"status":"no_change"'* ]]
  test "$(json_number generation)" = "$restore_generation"
  test "$(claim 28)" = "$restored_claim"
  test "$(claim 2)" = "$first_claim"
  test "$(claim 10)" = "$second_claim"
  test "$(json_text study)" = "$study"
  test "$(state)" = '2|1|1'
fi

step=$exit_mode
case "$exit_mode" in
  quit) send quit ;;
  idle-stop) kill -TERM "$pid" ;;
  output-disconnect)
    exec {reader}<&-
    reader=
    expected_exit=1
    ;;
  batch-disconnect)
    before=$(cpu_time)
    send tick 1000000
    for ((attempt=0; attempt<100; ++attempt)); do
      read -r capacity queued < <("$HCP_RETAINED_LIVE_PIPE_CHECK" <&"$writer")
      active=$(cpu_time)
      if [[ $queued == 0 && $active -gt $before ]]; then break; fi
      sleep 0.1
    done
    [[ $queued == 0 && $active -gt $before ]]
    printf 'batch_command_pending_bytes=%s\nnative_cpu_before=%s\nnative_cpu_active=%s\n' \
      "$queued" "$before" "$active" > "$out/shutdown-witness.txt"
    exec {reader}<&-
    reader=
    expected_exit=1
    ;;
  blocked-write)
    "$HCP_RETAINED_LIVE_PIPE_CHECK" shrink <&"$reader" > "$out/output-pipe-initial.txt"
    send observe
    for ((attempt=0; attempt<100; ++attempt)); do
      read -r capacity queued < <("$HCP_RETAINED_LIVE_PIPE_CHECK" <&"$reader")
      if [[ $queued == "$capacity" ]]; then break; fi
      sleep 0.1
    done
    [[ $capacity -gt 0 && $queued == "$capacity" ]]
    printf 'output_pipe_capacity=%s\nqueued_bytes_before_signal=%s\n' \
      "$capacity" "$queued" > "$out/shutdown-witness.txt"
    kill -TERM "$pid"
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
if [[ $exit_mode == output-disconnect || $exit_mode == batch-disconnect ]]; then
  wait_for_exit "$expected_exit"
  grep -Fxq 'live study output closed' "$out/live.stderr"
elif [[ $exit_mode == blocked-write ]]; then
  wait_for_exit 0
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
