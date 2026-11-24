#!/usr/bin/env bash
# Compare both sides of renames. Unknown paths always select the full workflow.
set -euo pipefail
base=$1
head=$2
paths=$(mktemp)
trap 'rm -f "$paths"' EXIT
git diff --no-renames --name-only -z "$base" "$head" > "$paths"
if [[ ! -s "$paths" ]]; then
  echo full
  exit 0
fi
while IFS= read -r -d '' path; do
  case "$path" in
    README.md|AGENTS.md|CONTRIBUTING.md|ROADMAP.md|docs/*.md|engine/README.md|engine/ARCHITECTURE.md|engine/docs/*.md|tools/viewport/README.md) ;;
    *) echo full; exit 0 ;;
  esac
done < "$paths"
echo prose
