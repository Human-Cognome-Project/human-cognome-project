#!/usr/bin/env bash
# HCP Migration 007: PBM & Entity Infrastructure
#
# Creates the PBM and entity database infrastructure:
#   Phase 1: Namespace allocations (hcp_core)
#   Phase 2: PBM structural markers — 91 tokens (hcp_core)
#   Phase 3: Entity classification tokens — 60 tokens (hcp_core)
#   Phase 4: Create hcp_en_pbm database + 8 tables
#   Phase 5: Create hcp_fic_entities database + 7 tables
#   Phase 6: Create hcp_nf_entities database + 7 tables (same schema)
#   Phase 7: Register shards (hcp_core)
#
# Usage: bash 007_pbm_entity_infrastructure.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PSQL="psql -h localhost -U hcp -v ON_ERROR_STOP=1"

echo "=== Migration 007: PBM & Entity Infrastructure ==="
echo ""

# Phase 1: Namespace allocations
echo "--- Phase 1: Namespace allocations ---"
PGPASSWORD=hcp_dev $PSQL -d hcp_core -f "$SCRIPT_DIR/007a_namespace_allocations.sql"
echo "Phase 1 complete."
echo ""

# Phase 2: PBM structural markers
echo "--- Phase 2: PBM structural markers (91 tokens) ---"
PGPASSWORD=hcp_dev $PSQL -d hcp_core -f "$SCRIPT_DIR/007b_pbm_structural_markers.sql"
echo "Phase 2 complete."
echo ""

# Phase 3: Entity classification tokens
echo "--- Phase 3: Entity classification tokens ---"
PGPASSWORD=hcp_dev $PSQL -d hcp_core -f "$SCRIPT_DIR/007c_entity_classification_tokens.sql"
echo "Phase 3 complete."
echo ""

# Phase 4: Create hcp_en_pbm database
echo "--- Phase 4: Create hcp_en_pbm database ---"
if PGPASSWORD=hcp_dev $PSQL -d postgres -tAc "SELECT 1 FROM pg_database WHERE datname = 'hcp_en_pbm'" | grep -q 1; then
    echo "Database hcp_en_pbm already exists — skipping creation."
else
    PGPASSWORD=hcp_dev $PSQL -d postgres -c "CREATE DATABASE hcp_en_pbm OWNER hcp;"
    echo "Database hcp_en_pbm created."
fi
PGPASSWORD=hcp_dev $PSQL -d hcp_en_pbm -f "$SCRIPT_DIR/000_helpers.sql"
PGPASSWORD=hcp_dev $PSQL -d hcp_en_pbm -f "$SCRIPT_DIR/007d_create_pbm_database.sql"
echo "Phase 4 complete."
echo ""

# Phase 5: Create hcp_fic_entities database
echo "--- Phase 5: Create hcp_fic_entities database ---"
if PGPASSWORD=hcp_dev $PSQL -d postgres -tAc "SELECT 1 FROM pg_database WHERE datname = 'hcp_fic_entities'" | grep -q 1; then
    echo "Database hcp_fic_entities already exists — skipping creation."
else
    PGPASSWORD=hcp_dev $PSQL -d postgres -c "CREATE DATABASE hcp_fic_entities OWNER hcp;"
    echo "Database hcp_fic_entities created."
fi
PGPASSWORD=hcp_dev $PSQL -d hcp_fic_entities -f "$SCRIPT_DIR/000_helpers.sql"
PGPASSWORD=hcp_dev $PSQL -d hcp_fic_entities -f "$SCRIPT_DIR/007e_create_entity_database.sql"
echo "Phase 5 complete."
echo ""

# Phase 6: Create hcp_nf_entities database
echo "--- Phase 6: Create hcp_nf_entities database ---"
if PGPASSWORD=hcp_dev $PSQL -d postgres -tAc "SELECT 1 FROM pg_database WHERE datname = 'hcp_nf_entities'" | grep -q 1; then
    echo "Database hcp_nf_entities already exists — skipping creation."
else
    PGPASSWORD=hcp_dev $PSQL -d postgres -c "CREATE DATABASE hcp_nf_entities OWNER hcp;"
    echo "Database hcp_nf_entities created."
fi
PGPASSWORD=hcp_dev $PSQL -d hcp_nf_entities -f "$SCRIPT_DIR/000_helpers.sql"
PGPASSWORD=hcp_dev $PSQL -d hcp_nf_entities -f "$SCRIPT_DIR/007e_create_entity_database.sql"
echo "Phase 6 complete."
echo ""

# Phase 7: Register shards
echo "--- Phase 7: Register shards ---"
PGPASSWORD=hcp_dev $PSQL -d hcp_core -f "$SCRIPT_DIR/007f_shard_registry.sql"
echo "Phase 7 complete."
echo ""

# Summary
echo "=== Migration 007 Complete ==="
echo ""
echo "Core tokens added:"
PGPASSWORD=hcp_dev $PSQL -d hcp_core -c "
    SELECT p2, COUNT(*) as tokens
    FROM tokens
    WHERE ns = 'AA' AND p2 IN ('AE', 'AF')
    GROUP BY p2
    ORDER BY p2;
"
echo ""
echo "Databases created:"
PGPASSWORD=hcp_dev $PSQL -d postgres -c "
    SELECT datname FROM pg_database
    WHERE datname IN ('hcp_en_pbm', 'hcp_fic_entities', 'hcp_nf_entities')
    ORDER BY datname;
"
echo ""
echo "Shard registry:"
PGPASSWORD=hcp_dev $PSQL -d hcp_core -c "
    SELECT ns_prefix, shard_db, active
    FROM shard_registry
    ORDER BY ns_prefix;
"
