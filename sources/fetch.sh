#!/usr/bin/env bash
# fetch.sh — Download external data sources for HCP
#
# Downloads go into sources/data/ (gitignored).
# Run from the repository root: ./sources/fetch.sh
#
# Currently a placeholder. Data sources will be added as they are
# identified and their formats stabilize.

set -euo pipefail

DATA_DIR="$(dirname "$0")/data"
mkdir -p "$DATA_DIR"

echo "HCP data fetch script"
echo "Target directory: $DATA_DIR"
echo ""
echo "No data sources configured yet."
echo "See sources/README.md for planned sources."
