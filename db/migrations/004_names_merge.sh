#!/usr/bin/env bash
# 004_names_merge.sh — Merge hcp_names data into hcp_english
#
# Decision 002: yA namespace retired. Name-only tokens become AB.AB.CA
# entries with PoS = label. All English words get capitalized form variants.
#
# Usage: ./db/migrations/004_names_merge.sh
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
EXPORT_FILE="/tmp/hcp_names_export.csv"

psql_english() {
    psql -U "$DB_USER" -h "$DB_HOST" -d hcp_english -v ON_ERROR_STOP=1 "$@"
}

psql_names() {
    psql -U "$DB_USER" -h "$DB_HOST" -d hcp_names -v ON_ERROR_STOP=1 "$@"
}

psql_core() {
    psql -U "$DB_USER" -h "$DB_HOST" -d hcp_core -v ON_ERROR_STOP=1 "$@"
}

echo "=== Migration 004: Names Merge ==="
echo ""

# --- Pre-flight ---
echo "--- Pre-flight counts (hcp_english) ---"
psql_english -t -c "SELECT 'tokens:  ' || COUNT(*) FROM tokens;"
psql_english -t -c "SELECT 'entries: ' || COUNT(*) FROM entries;"
psql_english -t -c "SELECT 'forms:   ' || COUNT(*) FROM forms;"
echo ""

# --- Step 1: Capitalized form variants ---
echo "=== Step 1: Adding capitalized form variants ==="
psql_english -f "$SCRIPT_DIR/004a_capitalized_forms.sql"
echo ""

# --- Step 2: Overlap (no-op) ---
echo "=== Step 2: 8,225 overlapping names covered by Step 1. No action needed. ==="
echo ""

# --- Step 3: Export name-only tokens from hcp_names ---
echo "=== Step 3: Import name-only tokens ==="
echo "Exporting names from hcp_names..."
psql_names -c "\COPY (SELECT name FROM tokens ORDER BY name) TO '$EXPORT_FILE' CSV"
EXPORT_COUNT=$(wc -l < "$EXPORT_FILE")
echo "  Exported $EXPORT_COUNT names."
echo ""

echo "Importing into hcp_english..."
psql_english -f "$SCRIPT_DIR/004b_import_names.sql"
echo ""

# --- Post-flight ---
echo "--- Post-flight counts (hcp_english) ---"
psql_english -t -c "SELECT 'tokens:  ' || COUNT(*) FROM tokens;"
psql_english -t -c "SELECT 'entries: ' || COUNT(*) FROM entries;"
psql_english -t -c "SELECT 'forms:   ' || COUNT(*) FROM forms;"
psql_english -t -c "SELECT 'label entries: ' || COUNT(*) FROM entries WHERE pos_token = 'AB.AB.CA.DB.Ek';"
echo ""

# --- Update shard registry ---
echo "Updating shard_registry..."
psql_core -c "UPDATE shard_registry SET description = 'Name components — RETIRED (merged into hcp_english)' WHERE ns_prefix = 'yA';"
echo ""

# --- Cleanup ---
rm -f "$EXPORT_FILE"
psql_english -c "ANALYZE tokens; ANALYZE entries; ANALYZE forms;"
echo "=== Migration 004 complete ==="
