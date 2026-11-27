#!/usr/bin/env bash
# Run the existing live CLI under its retained publishing owner.
set -euo pipefail
out=${1:?evidence directory required}
shift
: "${HCP_WARM_TEST_CONNINFO:?}"
exit_mode=${HCP_RETAINED_LIVE_EXIT:-quit}
root_prefixes=()
case "$exit_mode" in
  quit|idle-stop|output-disconnect|batch-stop|offer-stop) ;;
  catalogue) set -- "$@" --letter 02.00.00.00.03 --letter 02.00.00.00.1W ;;
  catalogue-single) set -- "$@" --letter 02.00.00.00.03 ;;
  catalogue-shared|catalogue-shared-direct)
    if [[ $exit_mode == catalogue-shared-direct ]]; then set -- "$@" --threads 2 --direct; fi
    set -- "$@" --letter 02.00.00.00.03 --letter 02.00.00.00.04
    exit_mode=catalogue-shared ;;
  catalogue-cross|catalogue-cross-direct)
    if [[ $exit_mode == catalogue-cross-direct ]]; then set -- "$@" --threads 2 --direct; fi
    set -- "$@" --letter 02.00.00.00.03 --letter 02.01.00.00.03 --focus-byte 00.00.00.00.23
    exit_mode=catalogue-cross ;;
  catalogue-restart)
    : "${HCP_RETAINED_RESTART_CONNINFO:?}"
    set -- "$@" --letter 02.00.00.00.03 --letter 02.01.00.00.03 --focus-byte 00.00.00.00.23 ;;
  catalogue-four-a|catalogue-nine-a|catalogue-nine-a-direct|catalogue-three-b)
    suffix=03
    focus_byte=00.00.00.00.23
    case "$exit_mode" in
      catalogue-four-a) root_prefixes=(02.00.00 02.00.10 02.00.20 02.01.00) ;;
      catalogue-three-b)
        root_prefixes=(02.00.00 02.00.10 02.01.00)
        suffix=04
        focus_byte=00.00.00.00.24 ;;
      *) root_prefixes=(02.00.00 02.00.10 02.00.20 02.01.00 02.01.10 02.01.20 02.01.30 02.02.40 02.20.00) ;;
    esac
    if [[ $exit_mode == catalogue-nine-a-direct ]]; then
      set -- "$@" --threads 2 --direct
      exit_mode=catalogue-nine-a
    fi
    set -- "$@" --focus-byte "$focus_byte"
    for prefix in "${root_prefixes[@]}"; do set -- "$@" --letter "$prefix.00.$suffix"; done ;;
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
start_study() {
  local error_log=$1 study_read study_write
  shift
  coproc STUDY { exec "$@" --retained --response-work-limit 1000000 2>"$error_log"; }
  pid=$STUDY_PID
  study_read=${STUDY[0]}
  study_write=${STUDY[1]}
  exec {reader}<&"$study_read" {writer}>&"$study_write"
  exec {study_read}<&- {study_write}>&-
}
start_study "$out/live.stderr" "$@"
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
if [[ $exit_mode == catalogue* ]]; then
  step=catalogue-offer
  receive '"command":"catalogue_available"'
  catalogue_offer=$event
  offer_entry() {
    printf '%s\n' "$catalogue_offer" |
      sed -nE 's/.*\{"definition":"([^"]+)","entry":'"$1"',"revision":"([^"]+)"\}.*/\1 \2/p'
  }
  read -r a a_revision < <(offer_entry 1)
  if [[ $exit_mode != catalogue-single ]]; then
    read -r second second_revision < <(offer_entry 2)
    test "$a" != "$second"
  else
    test -z "$(offer_entry 2)"
  fi
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
  release_catalogue() {
    step=catalogue-release
    send quit
    receive '"command":"recipe_release"' 30
    [[ "$event" == *'"acquisitions":2'* && "$event" == *'"releases":2'* ]]
    wait_for_exit "${1:-1}"
    test "$(state)" = '2|2|0'
    exec {writer}>&- {reader}<&-
    step=complete
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

  if [[ $exit_mode == catalogue-restart ]]; then
    positions() {
      printf '%s\n' "$event" | tr '{' '\n' | sed -nE \
        's/^"aggregate":"([^"]+)","binding":([0-9]+),"definition":"([^"]+)","exact_mass":"([0-9]+)","exposure":"([^"]+)","id":"([^"]+)"[^}]*"position":(\[[^]]+\]).*/\1|\2|\3|\4|\5|\6|\7/p'
    }
    step=restart-save
    send add 2 "$second" "$second_revision"
    receive '"status":"ok"'
    send save "$out/seed.restart.json"
    receive '"status":"ok"'
    send tick 1
    receive '"completed_tick":1,'
    send save "$out/tick1.restart.json"
    receive '"status":"ok"'
    test "$(json_number saved_tick)" = 1
    send tick 127
    receive '"completed_tick":128,'
    send dt 0.25
    receive '"status":"ok"'
    send save "$out/tick128.restart.json"
    receive '"status":"ok"'
    test "$(json_number saved_tick)" = 128
    observe_state 4
    saved_positions=$(positions)
    test "$(printf '%s\n' "$saved_positions" | wc -l)" -eq 2
    send lod parts
    receive '"status":"ok"'
    send save "$out/parts.restart.json"
    receive '"status":"ok"'
    parts_positions=$(positions)
    test "$(printf '%s\n' "$parts_positions" | wc -l)" -eq 2
    release_catalogue 0

    step=restart-mount
    export HCP_WARM_TEST_CONNINFO=$HCP_RETAINED_RESTART_CONNINFO
    test "$(state)" = '0|0|0'
    start_study "$out/receiver.stderr" "$@" --load "$out/tick128.restart.json" --threads 2 --direct
    receive '"command":"catalogue_available"'
    test "$event" = "$catalogue_offer"
    receive '"command":"create"'
    [[ "$event" == *'"actual_backend":"native_cpu"'* && "$event" == *'"host_path":"direct"'* ]]
    test "$(json_number saved_tick)" = 128
    [[ "$event" == *'"dt":0.25,'* ]]
    test "$(json_number completed_tick)" = 0
    test "$(positions)" = "$saved_positions"
    [[ "$event" == *'"focal_fields":[{"kind":"parent_value","mass":4.0,'* ]]
    test "$(printf '%s\n' "$event" | tr '{' '\n' | sed -nE 's/.*"velocity":\[0\.0,0\.0,0\.0\].*/zero/p' | wc -l)" -eq 2
    observe_state 4
    send tick 1
    receive '"completed_tick":1,'

    step=restart-refusal
    prior_positions=$(positions)
    prior_generation=$(json_number generation)
    prior_claim1=$(claim 1)
    prior_claim2=$(claim 2)
    sed -E 's/^  "dt": [^,]+,/  "dt": 0,/' "$out/tick128.restart.json" > "$out/zero-dt.restart.json"
    send load "$out/zero-dt.restart.json"
    receive '"status":"invalid_input"'
    [[ "$event" == *'"dt":0.25,'* ]]
    test "$(json_number completed_tick)" = 1
    test "$(positions)" = "$prior_positions"
    sed '/"position": \[/ { n; s/.*/        1e100,/; }' "$out/tick128.restart.json" > "$out/invalid.restart.json"
    send load "$out/invalid.restart.json"
    receive '"status":"invalid_numerics"'
    test "$(json_number completed_tick)" = 1
    test "$(json_number generation)" = "$prior_generation"
    test "$(positions)" = "$prior_positions"
    test "$(claim 1)" = "$prior_claim1"
    test "$(claim 2)" = "$prior_claim2"
    send tick 1
    receive '"completed_tick":2,'

    step=restart-parts
    send load "$out/parts.restart.json"
    receive '"status":"ok"'
    test "$(json_text exposure)" = parts
    test "$(json_number completed_tick)" = 0
    test "$(positions)" = "$parts_positions"
    observe_state 4
    send tick 1
    receive '"completed_tick":1,'
    step=live-timestep
    # Compare the same two seeded bodies after one native step at each dt.
    send load "$out/seed.restart.json"
    receive '"status":"ok"'
    [[ "$event" == *'"dt":1.0,'* ]]
    send tick 1
    receive '"completed_tick":1,'
    local_step_positions=$(positions)
    send load "$out/seed.restart.json"
    receive '"status":"ok"'
    send dt 0.25
    receive '"status":"ok"'
    send tick 1
    receive '"completed_tick":1,'
    test "$(positions)" != "$local_step_positions"

    # Older position-only files predate the timestep field and used dt=1.
    sed -e '/^  "dt": /d' -e 's/^  "version": 2$/  "version": 1/' \
      "$out/seed.restart.json" > "$out/legacy.restart.json"
    send load "$out/legacy.restart.json"
    receive '"status":"ok"'
    [[ "$event" == *'"dt":1.0,'* ]]

    step=live-clock
    pause_clock() {
      send pause
      while true; do
        receive '"status":"ok"'
        if [[ "$event" == *'"command":"pause"'* ]]; then break; fi
        [[ "$event" == *'"automatic":true'* ]]
      done
      [[ "$event" == *'"running":false'* ]]
    }
    send run 50
    receive '"command":"run"'
    receive '"automatic":true'
    pause_clock
    held=$(json_number completed_tick)
    sleep 0.15
    send observe
    receive '"command":"observe"'
    test "$(json_number completed_tick)" = "$held"
    send tick 1
    receive '"automatic":false'
    test "$(json_number completed_tick)" -eq "$((held+1))"
    send run
    receive '"command":"run"'
    receive '"automatic":true'
    pause_clock
    release_catalogue 1
    echo 'PASS: independent CPU receiver restored saved positions, refused an invalid hint and ticked again'
    exit 0
  fi

  field_row() {
    local address=${1//./\\.}
    printf '%s\n' "$event" | tr '{' '\n' |
      sed -nE "s/^\"comparison\":\"([0-9]+)\",\"kind\":$2,\"object\":\"[^\"]*$address\",\"record\":([0-9]+)\}.*/\1 \2/p"
  }
  field_slot() {
    local identity
    identity=$(field_row "$1" "$2")
    test -n "$identity"
    printf '%s\n' "$event" | sed -nE "s/.*\"comparison\":\"${identity%% *}\",\"slot\":([0-9]+)\}.*/\1/p"
  }
  response_links() {
    printf '%s\n' "$event" | sed 's/},{"body":/}\n{"body":/g' | sed -nE \
      's/.*"record":([0-9]+)\},"body_mass":[^,]+,"child":([0-9]+),[^}]*"denominator":"([0-9]+)","extraction_valid":true,"kind":"([^"]+)","numerator":"([0-9]+)","parent":([0-9]+),.*/\1 \2 \6 \4 \5 \3/p'
  }
  check_recipient() {
    local body slot foreign=
    body=$(claim "$1"); body=${body##*:}
    slot=$(field_slot "$2" 2)
    test -n "$body"
    test -n "$slot"
    if [[ $# == 3 && -n $(field_row "$3" 2) ]]; then foreign=$(field_slot "$3" 2); fi
    response_links | awk -v body="$body" -v slot="$slot" -v foreign="$foreign" '
      $1==body && $3==slot { own=1 }
      $1==body && foreign!="" && $3==foreign { other=1 }
      END { exit !(own && !other) }'
  }

  if ((${#root_prefixes[@]})); then
    root_count=${#root_prefixes[@]}
    focus_claim=$(field_row "$focus_byte" 1)
    test -n "$focus_claim"
    declare -a live_claims
    live_claims[1]=$original
    step=multilingual-add
    for ((entry=2; entry<=root_count; ++entry)); do
      read -r definition revision < <(offer_entry "$entry")
      send add "$entry" "$definition" "$revision"
      receive '"status":"ok"'
      observe_state "$((2*entry))"
      live_claims[entry]=$(claim "$entry")
      test -n "${live_claims[entry]}"
    done

    positions() {
      printf '%s\n' "$event" | tr '{' '\n' |
        sed -nE 's/.*"exact_mass":"2".*"position":(\[[-+0-9.eE,]+\]).*/\1/p'
    }
    checkpoint() {
      local count=$1 completed=$2 entry prefix samples
      observe_state "$((2*count))"
      test "$(json_number completed_tick)" = "$completed"
      test "$(field_row "$focus_byte" 1)" = "$focus_claim"
      for ((entry=1; entry<=count; ++entry)); do
        test "$(claim "$entry")" = "${live_claims[entry]}"
        prefix=${root_prefixes[entry-1]}
        check_recipient "$entry" "$prefix.00.00"
        check_recipient "$entry" "$prefix.00.01"
      done
      samples=$(positions)
      test "$(printf '%s\n' "$samples" | wc -l)" -eq "$count"
      printf '%s|%s|%s\n' "$step" "$completed" "${samples//$'\n'/|}" >> "$out/trajectories.txt"
    }

    if [[ $exit_mode == catalogue-four-a ]]; then
      step=multilingual-partial-batch
      send tick 4 48
      receive '"status":"capacity_exhausted"'
      geometry_refusal='"failure":{"budget":"geometry-work","capacity":48,"required":84}'
      [[ "$event" == *"$geometry_refusal"* ]]
      test "$(json_number requested_ticks)" = 4
      test "$(json_number ticks)" = 1
      checkpoint 4 1
      generation=$(json_number generation)
      unchanged_positions=$(positions)
      step=multilingual-refused-retry
      send tick 1 48
      receive '"status":"capacity_exhausted"'
      [[ "$event" == *"$geometry_refusal"* ]]
      test "$(json_number ticks)" = 0
      test "$(json_number generation)" = "$generation"
      test "$(positions)" = "$unchanged_positions"
      checkpoint 4 1
      step=multilingual-recover
      send remove 4
      receive '"status":"ok"'
      test -z "$(claim 4)$(field_row "${root_prefixes[3]}.00.00" 2)"
      send tick 32 48
      receive '"status":"ok"'
      checkpoint 3 33
      release_catalogue
      echo 'PASS: budget-limited tick batch reported partial progress and recovered after removal'
      exit 0
    fi

    step=multilingual-first-tick
    send tick 1
    receive '"status":"ok"'
    checkpoint "$root_count" 1
    if [[ $exit_mode == catalogue-three-b ]]; then
      release_catalogue 0
      echo 'PASS: three retained B definitions ticked with their own language and alphabet recipients'
      exit 0
    fi

    step=multilingual-sustained
    send tick 255
    receive '"status":"ok"'
    checkpoint 9 256
    step=multilingual-after-remove
    send remove 9
    receive '"status":"ok"'
    test -z "$(claim 9)$(field_row "${root_prefixes[8]}.00.00" 2)"
    send tick 256
    receive '"status":"ok"'
    checkpoint 8 512
    step=multilingual-after-restore
    send add 9
    receive '"status":"ok"'
    restored=$(claim 9)
    test -n "$restored"
    test "$restored" != "${live_claims[9]}"
    live_claims[9]=$restored
    send tick 256
    receive '"status":"ok"'
    checkpoint 9 768
    release_catalogue 0
    echo 'PASS: nine-A study ticked, accepted removal and restoration, and reached tick 768'
    exit 0
  fi

  if [[ $exit_mode == catalogue-cross ]]; then
    check_shared_byte() {
      local suffix=$1 english_target=$2 french_target=$3 kind=$4 english french shared_slot en_slot fr_slot links
      english=$(claim "1$suffix"); english=${english##*:}
      french=$(claim "2$suffix"); french=${french##*:}
      shared_slot=$(field_slot "$focus_byte" 1)
      en_slot=$(field_slot "$english_target" "$kind")
      fr_slot=$(field_slot "$french_target" "$kind")
      links=$(response_links)
      [[ "$links" == *"$english $shared_slot $en_slot scoped_inherited 2 2"* ]]
      [[ "$links" == *"$french $shared_slot $fr_slot scoped_inherited 2 2"* ]]
      check_recipient "1$suffix" "$english_group" "$french_group"
      check_recipient "1$suffix" "$english_alphabet" "$french_alphabet"
      check_recipient "2$suffix" "$french_group" "$english_group"
      check_recipient "2$suffix" "$french_alphabet" "$english_alphabet"
      test "$(field_row "$focus_byte" 1)" = "$focus_claim"
    }
    focus_byte=00.00.00.00.23
    english_group=02.00.00.00.00
    english_alphabet=02.00.00.00.01
    french_group=02.01.00.00.00
    french_alphabet=02.01.00.00.01
    [[ "$event" == *'"focus":{"kind":"parent","object":'*"$focus_byte"* ]]
    focus_claim=$(field_row "$focus_byte" 1)
    test -n "$focus_claim"
    check_recipient 1 "$english_group" "$french_group"
    test -z "$(field_row "$french_alphabet" 2)"

    step=cross-refusal
    send add 2 "$second" 00000000-0000-0000-0000-000000000000
    receive '"status":"invalid_input"'
    test "$(claim 1)" = "$original"
    test -z "$(claim 2)"

    step=cross-add
    send add 2 "$second" "$second_revision"
    receive '"status":"ok"'
    observe_state 4
    test "$(claim 1)" = "$original"
    check_shared_byte '' 02.00.00.00.03 02.01.00.00.03 0
    send tick 1
    receive '"completed_tick":1'
    check_shared_byte '' 02.00.00.00.03 02.01.00.00.03 0

    step=cross-parts
    send lod parts
    receive '"status":"ok"'
    observe_state 4
    [[ "$event" == *"\"id\":\"$study/1/0/0\""*"\"id\":\"$study/2/0/0\""* ]]
    check_shared_byte /0/0 "$english_alphabet" "$french_alphabet" 2
    french_part=$(claim 2/0/0)

    step=cross-remove-english
    send remove 1
    receive '"status":"ok"'
    observe_state 2
    test -z "$(claim 1/0/0)"
    test -z "$(field_row "$english_group" 2)$(field_row "$english_alphabet" 2)"
    test "$(claim 2/0/0)" = "$french_part"
    check_recipient 2/0/0 "$french_group" "$english_group"
    send tick 1
    receive '"completed_tick":2'
    test "$(claim 2/0/0)" = "$french_part"

    step=cross-restore-english
    send add 1
    receive '"status":"ok"'
    observe_state 4
    check_shared_byte /0/0 "$english_alphabet" "$french_alphabet" 2
    test "$(claim 2/0/0)" = "$french_part"

    step=cross-rollup
    send lod aggregate
    receive '"status":"ok"'
    observe_state 4
    check_shared_byte '' 02.00.00.00.03 02.01.00.00.03 0
    english_rollup=$(claim 1)

    step=cross-remove-french
    send remove 2
    receive '"status":"ok"'
    observe_state 2
    test -z "$(claim 2)"
    test -z "$(field_row "$french_group" 2)$(field_row "$french_alphabet" 2)"
    check_recipient 1 "$english_group" "$french_group"
    send tick 1
    receive '"completed_tick":3'
    test "$(claim 1)" = "$english_rollup"
    test "$(field_row "$focus_byte" 1)" = "$focus_claim"
    release_catalogue
    echo 'PASS: English and French A share their byte and keep their own membership recipients'
    exit 0
  fi

  if [[ $exit_mode == catalogue-shared ]]; then
    scoped_rows() {
      printf '%s\n' "$event" | sed 's/},{"body":/}\n{"body":/g' | sed -nE \
        's/.*"record":([0-9]+)\},"body_mass":[^,]+,"child":([0-9]+),[^}]*"denominator":"2","extraction_valid":true,"kind":"scoped_inherited","numerator":"1","parent":([0-9]+),"production_eligible":(true|false),.*/\1 \2 \3 \4/p'
    }
    shared_nibble() {
      local a_record b_record r1 r2 child1 child2 parent1 parent2
      local -a scoped
      readarray -t scoped < <(scoped_rows)
      test "${#scoped[@]}" -eq 2
      a_record=$(claim 1/0/0); a_record=${a_record##*:}
      b_record=$(claim 2/0/0); b_record=${b_record##*:}
      read -r r1 child1 parent1 _ <<< "${scoped[0]}"
      read -r r2 child2 parent2 _ <<< "${scoped[1]}"
      [[ "$r1 $r2" == "$a_record $b_record" || "$r1 $r2" == "$b_record $a_record" ]]
      test "$child1" = "$child2"
      test "$parent1" != "$parent2"
    }

    step=shared-refusal
    send add 2 "$second" 00000000-0000-0000-0000-000000000000
    receive '"status":"invalid_input"'
    test "$(claim 1)" = "$original"
    test -z "$(claim 2)"

    step=shared-add
    send add 2 "$second" "$second_revision"
    receive '"status":"ok"'
    observe_state 4
    test "$(claim 1)" = "$original"
    test -n "$(claim 2)"
    send tick 1
    receive '"completed_tick":1'

    step=shared-parts
    send lod parts
    receive '"status":"ok"'
    observe_state 4
    [[ "$event" == *"\"id\":\"$study/1/0/0\""*"\"id\":\"$study/2/0/0\""* ]]
    shared_nibble
    a_part=$(claim 1/0/0)
    b_part=$(claim 2/0/0)
    send tick 1
    receive '"completed_tick":2'
    shared_nibble

    step=shared-clone
    send add 27 "$a" "$a_revision"
    receive '"status":"ok"'
    observe_state 6
    test -n "$(claim 27/0/0)"
    test "$(scoped_rows | wc -l)" -eq 3
    send tick 1
    receive '"completed_tick":3'
    test "$(scoped_rows | awk '$4=="true"{++eligible} END{print eligible+0}')" -eq 2
    send remove 27
    receive '"status":"ok"'
    shared_nibble
    test "$(claim 1/0/0)" = "$a_part"
    test "$(claim 2/0/0)" = "$b_part"

    step=shared-remove-in-parts
    send remove 2
    receive '"status":"ok"'
    observe_state 2
    test "$(claim 1/0/0)" = "$a_part"
    test -z "$(claim 2/0/0)"
    test -z "$(scoped_rows)"

    step=shared-readd-in-parts
    send add 2 "$second" "$second_revision"
    receive '"status":"ok"'
    observe_state 4
    shared_nibble
    test "$(claim 1/0/0)" = "$a_part"
    test "$(claim 2/0/0)" != "$b_part"

    step=shared-rollup
    send lod aggregate
    receive '"status":"ok"'
    observe_state 4
    rolled=$(claim 1)
    test -n "$rolled"
    test -n "$(claim 2)"

    step=shared-remove
    send remove 2
    receive '"status":"ok"'
    observe_state 2
    test -z "$(claim 2)"
    step=shared-final-tick
    send tick 1
    receive '"completed_tick":4'
    observe_state 2
    test "$(claim 1)" = "$rolled"
    release_catalogue
    echo 'PASS: retained A and B preserve one shared nibble with ordered per-byte contributions'
    exit 0
  fi

  step=catalogue-declared-retry
  send add 1
  receive '"status":"no_change"'
  test "$(claim 1)" = "$original"

  step=catalogue-clone-retry
  send add 27 "$a" "$a_revision"
  receive '"status":"ok"'
  clone=$(claim 27)
  test -n "$clone"
  clone_generation=$(json_number generation)
  send add 27
  receive '"status":"no_change"'
  test "$(claim 27)" = "$clone"
  test "$(json_number generation)" = "$clone_generation"

  step=catalogue-clone-conflict
  if [[ $exit_mode == catalogue ]]; then
    send add 27 2
    receive '"status":"invalid_input"'
    test "$(claim 27)" = "$clone"
    test "$(json_number generation)" = "$clone_generation"
    send add 27 "$second" "$second_revision"
  else
    send add 27 "$a" 00000000-0000-0000-0000-000000000000
  fi
  receive '"status":"invalid_input"'
  test "$(claim 27)" = "$clone"
  test "$(json_number generation)" = "$clone_generation"

  step=catalogue-clone-restore
  send remove 27
  receive '"status":"ok"'
  test -z "$(claim 27)"
  send add 27
  receive '"status":"ok"'
  restored=$(claim 27)
  test -n "$restored"
  test "$restored" != "$clone"
  test "$(mass)" = 4
  send add 27 1
  receive '"status":"no_change"'
  test "$(claim 27)" = "$restored"

  step=catalogue-unknown-refusal
  restored_generation=$(json_number generation)
  send add 28
  receive '"status":"invalid_input"'
  test "$(json_number generation)" = "$restored_generation"
  test "$(claim 27)" = "$restored"
  test -z "$(claim 28)"
  send remove 27
  receive '"status":"ok"'
  test "$(claim 1)" = "$original"
  test -z "$(claim 27)"
  if [[ $exit_mode == catalogue-single ]]; then
    step=catalogue-single-tick
    send tick 1
    receive '"completed_tick":1'
    observe_state 2
    test "$(claim 1)" = "$original"
    release_catalogue
    echo 'PASS: single-root retained catalogue retries, restores and requires a selector for new entries'
    exit 0
  fi

  initial_generation=$(json_number generation)

  step=catalogue-refusal
  send add 2 "$second" 00000000-0000-0000-0000-000000000000
  receive '"status":"invalid_input"'
  test "$(json_number generation)" = "$initial_generation"
  test "$(claim 1)" = "$original"
  test -z "$(claim 2)"

  step=catalogue-add
  send add 2 "$second" "$second_revision"
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
  [[ "$event" == *"\"definition\":\"$second\",\"entry\":2,\"id\":\"$study/2\",\"revision\":\"$second_revision\"}"* ]]

  step=catalogue-declared-restore
  send add 2
  receive '"status":"ok"'
  test "$(mass)" = 6
  test "$(claim 1)" = "$rolled"
  test -n "$(claim 2)"
  send remove 2
  receive '"status":"ok"'
  test -z "$(claim 2)"

  step=catalogue-final-tick
  send tick 1
  receive '"completed_tick":2'
  observe_state 2
  test "$(claim 1)" = "$rolled"

  step=catalogue-reselect
  send add 3 "$second" "$second_revision"
  receive '"status":"ok"'
  observe_state 6
  test "$(claim 1)" = "$rolled"
  test -z "$(claim 2)"
  test -n "$(claim 3)"

  release_catalogue
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
