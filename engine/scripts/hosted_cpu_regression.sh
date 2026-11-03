#!/usr/bin/env bash
# Round-robin indexes cover every configured test once, including new tests.
set -Eeuo pipefail
shard=${1:?Shard number 1..4 required}
out=${2:?Evidence directory required}
[[ "$shard" =~ ^[1-4]$ ]] || exit 2
mkdir -p "$out"
ctest --test-dir engine/build --show-only=json-v1 > "$out/all-tests.json"
jq -r '.tests[].name' "$out/all-tests.json" > "$out/all-tests.txt"
jq -r '.tests[].name' engine/build/ci-provenance/tests.json > "$out/build-tests.txt"
cmp "$out/all-tests.txt" "$out/build-tests.txt"
total=$(jq '.tests | length' "$out/all-tests.json")
test "$total" -ge 4
ctest --test-dir engine/build -I "$shard,$total,4" --show-only=json-v1 > "$out/selected-tests.json"
jq -r --argjson shard "$shard" '.tests | to_entries[] | select(.key % 4 == $shard - 1) | .value.name' \
  "$out/all-tests.json" > "$out/expected.txt"
jq -r '.tests[].name' "$out/selected-tests.json" > "$out/selected.txt"
cmp "$out/expected.txt" "$out/selected.txt"
test -s "$out/selected.txt"
# Explicit -j 1 preserves the CPU budgets of the original serial suite.
ctest --test-dir engine/build -j 1 -I "$shard,$total,4" --no-tests=error \
  --output-on-failure --output-log "$out/ctest.log" --output-junit "$out/results.xml"
