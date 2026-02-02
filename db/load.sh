#!/usr/bin/env bash
# load.sh — Load SQL dumps into local PostgreSQL
#
# Usage: ./db/load.sh [core|english|all]
# Default: all

set -euo pipefail

DB_USER="hcp"
DB_HOST="localhost"
SCRIPT_DIR="$(dirname "$0")"

load_db() {
    local dbname="$1"
    local sqlfile="$2"
    if [ ! -f "$sqlfile" ]; then
        echo "Skip: $sqlfile not found"
        return
    fi
    echo "Loading $sqlfile into $dbname..."
    PGPASSWORD="hcp_dev" psql -U "$DB_USER" -h "$DB_HOST" -d "$dbname" -f "$sqlfile" -q
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
