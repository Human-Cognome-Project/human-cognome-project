#!/usr/bin/env bash
# load.sh — Load SQL dumps into local PostgreSQL
#
# Usage: ./db/load.sh [core|english|var|fic_pbm|fic_entities|nf_entities|all]
# Default: all
#
# Databases:
#   core          — hcp_core: structural tokens, byte codes, PBM markers
#   english       — hcp_english: English vocabulary (~280K words, labels, affixes)
#   var           — hcp_var: var tokens (unresolved words)
#   fic_pbm       — hcp_fic_pbm: fiction PBM bond data
#   fic_entities  — hcp_fic_entities: fiction entity data
#   nf_entities   — hcp_nf_entities: non-fiction entity data
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
    local gzfile="${sqlfile}.gz"

    # Prefer gzipped + verify checksum if available
    if [ -f "$gzfile" ]; then
        if [ -f "${gzfile}.sha256" ]; then
            echo "Verifying checksum for $gzfile..."
            sha256sum -c "${gzfile}.sha256" --quiet || { echo "CHECKSUM FAILED: $gzfile"; exit 1; }
        fi
        echo "Loading $gzfile into $dbname..."
        createdb -U "$DB_USER" -h "$DB_HOST" "$dbname" 2>/dev/null || true
        gunzip -c "$gzfile" | psql -U "$DB_USER" -h "$DB_HOST" -d "$dbname" -q
        echo "  Done."
    elif [ -f "$sqlfile" ]; then
        echo "Loading $sqlfile into $dbname..."
        createdb -U "$DB_USER" -h "$DB_HOST" "$dbname" 2>/dev/null || true
        psql -U "$DB_USER" -h "$DB_HOST" -d "$dbname" -f "$sqlfile" -q
        echo "  Done."
    else
        echo "Skip: neither $gzfile nor $sqlfile found"
    fi
}

TARGET="${1:-all}"

case "$TARGET" in
    core)
        load_db "hcp_core" "$SCRIPT_DIR/core.sql"
        ;;
    english)
        load_db "hcp_english" "$SCRIPT_DIR/english.sql"
        ;;
    var)
        load_db "hcp_var" "$SCRIPT_DIR/var.sql"
        ;;
    fic_pbm)
        load_db "hcp_fic_pbm" "$SCRIPT_DIR/hcp_fic_pbm.sql"
        ;;
    fic_entities)
        load_db "hcp_fic_entities" "$SCRIPT_DIR/fic_entities.sql"
        ;;
    nf_entities)
        load_db "hcp_nf_entities" "$SCRIPT_DIR/hcp_nf_entities.sql"
        ;;
    all)
        load_db "hcp_core" "$SCRIPT_DIR/core.sql"
        load_db "hcp_english" "$SCRIPT_DIR/english.sql"
        load_db "hcp_var" "$SCRIPT_DIR/var.sql"
        load_db "hcp_fic_pbm" "$SCRIPT_DIR/hcp_fic_pbm.sql"
        load_db "hcp_fic_entities" "$SCRIPT_DIR/fic_entities.sql"
        load_db "hcp_nf_entities" "$SCRIPT_DIR/hcp_nf_entities.sql"
        ;;
    *)
        echo "Usage: $0 [core|english|var|fic_pbm|fic_entities|nf_entities|all]"
        exit 1
        ;;
esac
