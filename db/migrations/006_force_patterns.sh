#!/usr/bin/env bash
# 006_force_patterns.sh — Force pattern infrastructure
#
# Phase 1 (006a): Universal tokens in hcp_core (force types, relationships, LoD, principles)
# Phase 2: SKIPPED — existing p3 namespaces ARE structural categories (Q1 answer)
# Phase 3 (006b): Sub-cat pattern tokens + slot table + token_sub_cat junction in hcp_english
# Phase 4 (006c): Initial verb classification data in hcp_english
# Phase 5 (006d): Rule tables + token dimension columns in hcp_english
#
# Usage: ./db/migrations/006_force_patterns.sh [phase]
#   No argument = run all available phases
#   phase = 1|3|4|5 to run a specific phase
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

# --- Phase 2: SKIPPED ---
# Q1 answer: existing p3 namespaces (CA=N, CB=V, CC=A, etc.) ARE the
# structural categories. No new category tokens needed.

# --- Phase 3: Sub-cat patterns + slots + junction table ---
if [[ "$PHASE" == "all" || "$PHASE" == "3" ]]; then
    echo "=== Phase 3: Sub-cat patterns + slots + token_sub_cat (hcp_english) ==="
    psql_english -f "$SCRIPT_DIR/006b_sub_cat_patterns.sql"
    echo ""
    echo "Phase 3 complete."
    echo ""
fi

# --- Phase 4: Initial verb classification data ---
if [[ "$PHASE" == "all" || "$PHASE" == "4" ]]; then
    echo "=== Phase 4: Initial verb classification (hcp_english) ==="
    psql_english -f "$SCRIPT_DIR/006c_verb_classification.sql"
    echo ""
    echo "Phase 4 complete."
    echo ""
fi

# --- Phase 5: Rule tables + token dimensions ---
if [[ "$PHASE" == "all" || "$PHASE" == "5" ]]; then
    echo "=== Phase 5: Rule tables + token dimensions (hcp_english) ==="
    psql_english -f "$SCRIPT_DIR/006d_rules_and_dimensions.sql"
    echo ""
    echo "Phase 5 complete."
    echo ""
fi

echo "=== Migration 006 status ==="
echo "  Phase 1 (universal tokens):     DONE (006a)"
echo "  Phase 2 (categories):           SKIPPED — p3 namespaces are categories"
echo "  Phase 3 (patterns+slots+junct): $([ -f "$SCRIPT_DIR/006b_sub_cat_patterns.sql" ] && echo 'READY' || echo 'TODO')"
echo "  Phase 4 (verb classification):  $([ -f "$SCRIPT_DIR/006c_verb_classification.sql" ] && echo 'READY' || echo 'TODO')"
echo "  Phase 5 (rules+dimensions):     $([ -f "$SCRIPT_DIR/006d_rules_and_dimensions.sql" ] && echo 'READY' || echo 'TODO')"
echo ""
echo "=== Migration 006 done ==="
