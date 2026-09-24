#!/usr/bin/env bash
# HCP Token ID Decomposition — Master Migration Script
#
# Transforms all token tables from monolithic TEXT token_ids to
# decomposed CHAR(2) columns with generated TEXT PKs.
#
# Prerequisites:
#   - PostgreSQL running with hcp_core, hcp_english, hcp_names databases
#   - Databases loaded from current dumps (db/core.sql, db/english.sql, db/names.sql)
#   - User with CREATE/DROP privileges on all three databases
#
# Usage:
#   ./db/migrations/run.sh
#
# Environment:
#   HCP_DB_USER  — PostgreSQL user (default: hcp)
#   HCP_DB_HOST  — PostgreSQL host (default: localhost)
#   PGPASSWORD   — PostgreSQL password (set externally if needed)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DB_USER="${HCP_DB_USER:-hcp}"
DB_HOST="${HCP_DB_HOST:-localhost}"
PSQL="psql -U $DB_USER -h $DB_HOST -v ON_ERROR_STOP=1"

echo "============================================"
echo "HCP Token ID Decomposition Migration"
echo "============================================"
echo ""
echo "User: $DB_USER | Host: $DB_HOST"
echo ""

# Step 1: Install helper functions in all databases
echo "--- Installing helper functions ---"
for db in hcp_core hcp_english hcp_names; do
    echo "  $db..."
    $PSQL -d "$db" -f "$SCRIPT_DIR/000_helpers.sql" -q
done
echo "  Done."
echo ""

# Step 2: Migrate hcp_core (includes shard_registry creation)
echo "--- Migrating hcp_core ---"
$PSQL -d hcp_core -f "$SCRIPT_DIR/001_core.sql"
echo ""

# Step 3: Migrate hcp_english
echo "--- Migrating hcp_english ---"
$PSQL -d hcp_english -f "$SCRIPT_DIR/002_english.sql"
echo ""

# Step 4: Migrate hcp_names
echo "--- Migrating hcp_names ---"
$PSQL -d hcp_names -f "$SCRIPT_DIR/003_names.sql"
echo ""

echo "============================================"
echo "All migrations complete."
echo "============================================"
echo ""
echo "Verify:"
echo "  $PSQL -d hcp_core    -c \"SELECT ns, p2, p3, p4, p5, name FROM tokens LIMIT 5;\""
echo "  $PSQL -d hcp_english -c \"SELECT ns, p2, p3, p4, p5, name FROM tokens WHERE ns='AB' LIMIT 5;\""
echo "  $PSQL -d hcp_names   -c \"SELECT ns, p2, p3, name FROM tokens LIMIT 5;\""
echo "  $PSQL -d hcp_core    -c \"SELECT * FROM shard_registry;\""
