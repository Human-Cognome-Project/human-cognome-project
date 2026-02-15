#!/usr/bin/env bash
# 005_decompose_refs.sh — Decompose all token ID references
#
# Phase 1: Single token_id columns (entries, forms, relations, core metadata)
# Phase 2: Junction tables for array columns (relation_tags through form_tags)
# Phase 3: sense_glosses junction table (~8M+ rows, heaviest operation)
#
# Usage: ./db/migrations/005_decompose_refs.sh
#
# Environment:
#   HCP_DB_USER  — PostgreSQL user (default: hcp)
#   HCP_DB_HOST  — PostgreSQL host (default: localhost)
#   PGPASSWORD   — PostgreSQL password (default: hcp_dev)

set -euo pipefail

DB_USER="${HCP_DB_USER:-hcp}"
DB_HOST="${HCP_DB_HOST:-localhost}"
export PGPASSWORD="${PGPASSWORD:-hcp_dev}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

psql_english() {
    psql -U "$DB_USER" -h "$DB_HOST" -d hcp_english -v ON_ERROR_STOP=1 "$@"
}

psql_core() {
    psql -U "$DB_USER" -h "$DB_HOST" -d hcp_core -v ON_ERROR_STOP=1 "$@"
}

echo "=== Migration 005: Decompose All Token ID References ==="
echo ""

# --- Phase 1: Single columns ---
echo "=== Phase 1: Single token_id columns (hcp_english) ==="
psql_english -f "$SCRIPT_DIR/005a_decompose_single_refs.sql"
echo ""

echo "=== Phase 1b: Core metadata (hcp_core) ==="
psql_core -f "$SCRIPT_DIR/005b_decompose_core_metadata.sql"
echo ""

echo "Phase 1 complete."
echo ""

# --- Phase 2: Junction tables (small + medium) ---
echo "=== Phase 2: Junction tables (relation_tags → form_tags) ==="
psql_english -f "$SCRIPT_DIR/005c_junction_tables.sql"
echo ""

echo "Phase 2 complete."
echo ""

# --- Phase 3: sense_glosses (the big one) ---
echo "=== Phase 3: sense_glosses (~8M+ rows) ==="
psql_english -f "$SCRIPT_DIR/005d_junction_sense_glosses.sql"
echo ""

echo "Phase 3 complete."
echo ""

# --- Final summary ---
echo "=== Final table summary ==="
psql_english -t -c "
SELECT table_name, n_live_tup AS row_count
FROM pg_stat_user_tables
ORDER BY table_name;"

echo ""
echo "=== Migration 005 complete ==="
