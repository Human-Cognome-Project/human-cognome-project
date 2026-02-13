#!/usr/bin/env bash
# load.sh — Load SQL dumps into local PostgreSQL
#
# Usage: ./db/load.sh [core|english|all]
# Default: all
#
# Environment:
#   HCP_DB_USER  — PostgreSQL user (default: hcp)
#   HCP_DB_HOST  — PostgreSQL host (default: localhost)
#   PGPASSWORD   — PostgreSQL password (set externally or defaults to hcp_dev)

set -euo pipefail

DB_USER="${HCP_DB_USER:-hcp}"
DB_HOST="${HCP_DB_HOST:-localhost}"
SCRIPT_DIR="$(dirname "$0")"
export PGPASSWORD="${PGPASSWORD:-hcp_dev}"

load_db() {
    local dbname="$1"
    local sqlfile="$2"
    if [ ! -f "$sqlfile" ]; then
        echo "Skip: $sqlfile not found"
        return
    fi
    echo "Loading $sqlfile into $dbname..."
    psql -U "$DB_USER" -h "$DB_HOST" -d "$dbname" -f "$sqlfile" -q
    echo "  Done."
}

TARGET="${1:-all}"

case "$TARGET" in
    core)
        load_db "hcp_core" "$SCRIPT_DIR/core.sql"
        ;;
    english)
        load_db "hcp_english" "$SCRIPT_DIR/english.sql"
        ;;
    all)
        load_db "hcp_core" "$SCRIPT_DIR/core.sql"
        load_db "hcp_english" "$SCRIPT_DIR/english.sql"
        ;;
    *)
        echo "Usage: $0 [core|english|all]"
        exit 1
        ;;
esac
