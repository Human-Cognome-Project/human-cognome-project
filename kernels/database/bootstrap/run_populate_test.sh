#!/usr/bin/env bash
# Builds and runs encoding_populate_test against a THROWAWAY database
# (hcp_test_<pid>), created from ../schema/schema.sql (trigger included) and
# dropped afterwards. Never touches hcp_core: the only database this script
# drops is the one it just created, and the name is checked for the hcp_test_
# prefix first. Also compile-checks the populate_encoding driver.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
db="hcp_test_$$"
[[ "$db" == hcp_test_* ]] || exit 2
out="$(mktemp -d)"
cleanup() { dropdb --if-exists "$db" >/dev/null 2>&1 || true; rm -rf "$out"; }
trap cleanup EXIT

flags=(-std=c++17 -Wall -Wextra -I"$here/../codec" -I"$here/../controller" -I"$here" -I/usr/include/postgresql)
libs=("$here/encoding_populate.cpp" "$here/../controller/controller.cpp" "$here/../codec/codec.cpp" -lpq)

createdb "$db"
psql -q -v ON_ERROR_STOP=1 -d "$db" -f "$here/../schema/schema.sql" >/dev/null
g++ "${flags[@]}" "$here/encoding_populate_test.cpp" "${libs[@]}" -o "$out/t"
"$out/t" "dbname=$db"

# The driver must build too (it is run against a real floor by the coordinator).
g++ "${flags[@]}" "$here/populate_encoding.cpp" "${libs[@]}" -o "$out/driver"
echo "ok   populate_encoding driver builds"
