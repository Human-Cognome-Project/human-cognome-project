#!/usr/bin/env bash
# 006_force_patterns.sh — Force pattern infrastructure
#
# Phase 1 (006a): Universal tokens in hcp_core (force types, relationships, LoD, principles)
# Phase 2 (006b): English structural categories in hcp_english (BLOCKED — awaiting ling Q1 answer)
# Phase 3 (006c): Sub-cat patterns + slot table in hcp_english
# Phase 4 (006d): token_sub_cat junction + rule tables in hcp_english
# Phase 5 (006e): Token dimension columns on hcp_english tokens
#
# Usage: ./db/migrations/006_force_patterns.sh [phase]
#   No argument = run all available phases
#   phase = 1|2|3|4|5 to run a specific phase
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

PHASE="${1:-all}"

psql_core() {
    psql -U "$DB_USER" -h "$DB_HOST" -d hcp_core -v ON_ERROR_STOP=1 "$@"
}

psql_english() {
    psql -U "$DB_USER" -h "$DB_HOST" -d hcp_english -v ON_ERROR_STOP=1 "$@"
}

echo "=== Migration 006: Force Pattern Infrastructure ==="
echo ""

# --- Phase 1: Universal tokens in hcp_core ---
if [[ "$PHASE" == "all" || "$PHASE" == "1" ]]; then
    echo "=== Phase 1: Universal force infrastructure tokens (hcp_core) ==="
    psql_core -f "$SCRIPT_DIR/006a_force_infrastructure_tokens.sql"
    echo ""
    echo "Phase 1 complete."
    echo ""
fi

# --- Phase 2: English structural categories ---
if [[ "$PHASE" == "all" || "$PHASE" == "2" ]]; then
    if [[ -f "$SCRIPT_DIR/006b_english_categories.sql" ]]; then
        echo "=== Phase 2: English structural categories (hcp_english) ==="
        psql_english -f "$SCRIPT_DIR/006b_english_categories.sql"
        echo ""
        echo "Phase 2 complete."
        echo ""
    else
        echo "=== Phase 2: SKIPPED — 006b_english_categories.sql not yet written ==="
        echo "  (Blocked on linguistics specialist answer to Q1: PoS ↔ structural categories)"
        echo ""
    fi
fi

# --- Phase 3: Sub-cat patterns + slot table ---
if [[ "$PHASE" == "all" || "$PHASE" == "3" ]]; then
    if [[ -f "$SCRIPT_DIR/006c_sub_cat_patterns.sql" ]]; then
        echo "=== Phase 3: Sub-cat patterns + slot table (hcp_english) ==="
        psql_english -f "$SCRIPT_DIR/006c_sub_cat_patterns.sql"
        echo ""
        echo "Phase 3 complete."
        echo ""
    else
        echo "=== Phase 3: SKIPPED — 006c_sub_cat_patterns.sql not yet written ==="
        echo ""
    fi
fi

# --- Phase 4: token_sub_cat junction + rule tables ---
if [[ "$PHASE" == "all" || "$PHASE" == "4" ]]; then
    if [[ -f "$SCRIPT_DIR/006d_rules_and_mapping.sql" ]]; then
        echo "=== Phase 4: token_sub_cat junction + rule tables (hcp_english) ==="
        psql_english -f "$SCRIPT_DIR/006d_rules_and_mapping.sql"
        echo ""
        echo "Phase 4 complete."
        echo ""
    else
        echo "=== Phase 4: SKIPPED — 006d_rules_and_mapping.sql not yet written ==="
        echo ""
    fi
fi

# --- Phase 5: Token dimension columns ---
if [[ "$PHASE" == "all" || "$PHASE" == "5" ]]; then
    if [[ -f "$SCRIPT_DIR/006e_token_dimensions.sql" ]]; then
        echo "=== Phase 5: Token dimension columns (hcp_english) ==="
        psql_english -f "$SCRIPT_DIR/006e_token_dimensions.sql"
        echo ""
        echo "Phase 5 complete."
        echo ""
    else
        echo "=== Phase 5: SKIPPED — 006e_token_dimensions.sql not yet written ==="
        echo ""
    fi
fi

echo "=== Migration 006 status ==="
echo "  Phase 1 (universal tokens): $([ -f "$SCRIPT_DIR/006a_force_infrastructure_tokens.sql" ] && echo 'READY' || echo 'TODO')"
echo "  Phase 2 (categories):       $([ -f "$SCRIPT_DIR/006b_english_categories.sql" ] && echo 'READY' || echo 'BLOCKED on Q1')"
echo "  Phase 3 (sub-cat patterns):  $([ -f "$SCRIPT_DIR/006c_sub_cat_patterns.sql" ] && echo 'READY' || echo 'TODO')"
echo "  Phase 4 (mapping + rules):   $([ -f "$SCRIPT_DIR/006d_rules_and_mapping.sql" ] && echo 'READY' || echo 'TODO')"
echo "  Phase 5 (dimensions):        $([ -f "$SCRIPT_DIR/006e_token_dimensions.sql" ] && echo 'READY' || echo 'TODO')"
echo ""
echo "=== Migration 006 done ==="
