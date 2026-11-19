#!/usr/bin/env bash
# Transport regression only: real shell/loader/native /bin/true, fixture Docker
# and PostgreSQL boundaries. No database, retained source or tick qualification.
set -Eeuo pipefail
scripts=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT
mkdir -p "$scratch/bin" "$scratch/build/viewport" "$scratch/build/taichi-runtime" "$scratch/container"
cp /bin/true "$scratch/build/warm_active_integration"
cp /bin/true "$scratch/build/viewport/viewport_letter_capture"
cp /bin/true "$scratch/build/viewport/viewport_retained_capture_check"
printf 'transport fixture\n' > "$scratch/build/taichi-runtime/runtime_x64.bc"
ldd /bin/true > "$scratch/real-libraries.txt"
export HCP_TRANSPORT_TEST_ROOT="$scratch"
export HCP_TRANSPORT_REAL_SHA256SUM
HCP_TRANSPORT_REAL_SHA256SUM=$(command -v sha256sum)
cat > "$scratch/bin/ldd" <<'SH'
#!/usr/bin/env bash
cat "$HCP_TRANSPORT_TEST_ROOT/real-libraries.txt"
if test "${HCP_TRANSPORT_MISSING:-0}" = 1; then echo 'libmissing.so => not found'; fi
SH
# Source-payload acquisition is outside this transport fixture. Real data hashes
# are checked by the actual database job. Fingerprinting still uses real SHA256.
cat > "$scratch/bin/sha256sum" <<'SH'
#!/usr/bin/env bash
if test "${1:-}" = --check; then echo 'fixture source precondition'; exit 0; fi
exec "$HCP_TRANSPORT_REAL_SHA256SUM" "$@"
SH
cat > "$scratch/bin/rg" <<'SH'
#!/usr/bin/env bash
echo 'rg unavailable in this fixture' >&2
exit 127
SH
cat > "$scratch/bin/psql" <<'SH'
#!/usr/bin/env bash
if test -n "${LD_LIBRARY_PATH:-}"; then
  echo 'polluted utility runtime' >> "$HCP_TRANSPORT_TEST_ROOT/utility-runtime.txt"
  exit 88
fi
echo 'clean utility runtime' >> "$HCP_TRANSPORT_TEST_ROOT/utility-runtime.txt"
# Deliberate SQL boundary stop after the real probe shell starts. The wrapper
# must preserve its failed probe evidence. No fixture source shape is simulated.
exit 42
SH
cat > "$scratch/bin/docker" <<'SH'
#!/usr/bin/env bash
set -Eeuo pipefail
root=$HCP_TRANSPORT_TEST_ROOT
printf '%s\n' "$*" >> "$root/docker-calls.txt"
map_path() {
  case "$1" in
    /tmp/hcp_warm_d_*) printf '%s/container/%s' "$root" "${1#/tmp/}" ;;
    *) printf '%s' "$1" ;;
  esac
}
operation=$1; shift
case "$operation" in
cp)
  from=$1; to=$2
  case "$from" in fixture:*) from=$(map_path "${from#fixture:}");; esac
  case "$to" in fixture:*) to=$(map_path "${to#fixture:}");; esac
  cp -a "$from" "$to"
  ;;
exec)
  environment=()
  while test "$#" -gt 0; do
    case "$1" in
      -i) shift ;;
      --user) shift 2 ;;
      --env) environment+=("${2%%=*}=$(map_path "${2#*=}")"); shift 2 ;;
      *) break ;;
    esac
  done
  test "$1" = fixture; shift
  case "$1" in
    pg_dump) echo 'unchanged transport fixture source' ;;
    createdb) : ;;
    psql) cat > /dev/null ;;
    *)
      command=()
      for argument in "$@"; do command+=("$(map_path "$argument")"); done
      env "${environment[@]}" "${command[@]}"
      ;;
  esac
  ;;
*) exit 2 ;;
esac
SH
chmod +x "$scratch/bin/"*
export PATH="$scratch/bin:$PATH"
# Missing-library failure must precede all database/container work, including
# when rg is absent. ldd deliberately returns zero with an unresolved dependency.
set +e
HCP_TRANSPORT_MISSING=1 bash "$scripts/run_warm_active_acceptance.sh" "$scratch/build" fixture core ling > "$scratch/missing.log" 2>&1
code=$?
set -e
test "$code" = 2
test ! -e "$scratch/docker-calls.txt"
grep -Fq 'libmissing.so => not found' "$scratch/missing.log"
# Legacy transport executes a real native program through its staged loader.
bash "$scripts/run_warm_active_acceptance.sh" "$scratch/build" fixture core ling > "$scratch/legacy.log" 2>&1
# Prepared mode executes the real probe shell and reaches a container-side psql
# with an unpolluted environment. Its controlled SQL failure must be retained.
for mode in prepared-membership-refusal prepared-retained-positive; do
  set +e
  bash "$scripts/run_warm_active_acceptance.sh" "$scratch/build" fixture core ling "$mode" > "$scratch/$mode.log" 2>&1
  code=$?
  set -e
  test "$code" != 0
done
test "$(cat "$scratch/utility-runtime.txt")" = $'clean utility runtime\nclean utility runtime'
results=("$scratch/build/"acceptance-*/retained-probe/result.txt)
test "${#results[@]}" = 2
for result in "${results[@]}"; do
  grep -Fxq 'last_step=freshness' "$result"
  grep -Fxq 'exit_code=42' "$result"
done
echo 'PASS: missing-library refusal without rg, native loader transport, clean shell/utility runtime and failed probe evidence'
