#!/usr/bin/env bash
# Builds and runs controller_mass_trigger_test against a THROWAWAY database
# (hcp_test_<pid>), created from ../schema/schema.sql and dropped afterwards.
# Never touches hcp_core: the only database this script drops is the one it
# just created, and the name is checked for the hcp_test_ prefix first.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
db="hcp_test_$$"
[[ "$db" == hcp_test_* ]] || exit 2
out="$(mktemp -d)"
cleanup() { dropdb --if-exists "$db" >/dev/null 2>&1 || true; rm -rf "$out"; }
trap cleanup EXIT

createdb "$db"
psql -q -v ON_ERROR_STOP=1 -d "$db" -f "$here/../schema/schema.sql" >/dev/null
g++ -std=c++17 -Wall -Wextra -I"$here/../codec" -I"$here" -I/usr/include/postgresql \
  "$here/controller_mass_trigger_test.cpp" "$here/controller.cpp" "$here/../codec/codec.cpp" \
  -lpq -o "$out/t"
"$out/t" "dbname=$db" "$here/../schema/structural_mass_trigger.sql"
