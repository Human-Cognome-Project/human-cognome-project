#!/bin/bash
# Master migration script: Convert all HCP databases to five-column token_id structure
# Run this to migrate all databases in the correct order

set -e  # Exit on error

echo "========================================="
echo "HCP Five-Column Token ID Migration"
echo "========================================="
echo ""

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}Step 1: Creating helper functions in all databases${NC}"
sudo -u postgres psql -d hcp_core -f migrate_to_five_column_tokens.sql
sudo -u postgres psql -d hcp_english -f migrate_to_five_column_tokens.sql
sudo -u postgres psql -d hcp_names -f migrate_to_five_column_tokens.sql
sudo -u postgres psql -d hcp_en_pbm -f migrate_to_five_column_tokens.sql
echo -e "${GREEN}✓ Helper functions created in all databases${NC}"
echo ""

echo -e "${YELLOW}Step 2: Migrating hcp_core${NC}"
sudo -u postgres psql -d hcp_core -f migrate_core_to_five_column.sql
echo -e "${GREEN}✓ hcp_core migrated${NC}"
echo ""

echo -e "${YELLOW}Step 3: Migrating hcp_english${NC}"
sudo -u postgres psql -d hcp_english -f migrate_english_to_five_column.sql
echo -e "${GREEN}✓ hcp_english migrated${NC}"
echo ""

echo -e "${YELLOW}Step 4: Migrating hcp_names${NC}"
sudo -u postgres psql -d hcp_names -f migrate_names_to_five_column.sql
echo -e "${GREEN}✓ hcp_names migrated${NC}"
echo ""

echo -e "${YELLOW}Step 5: Migrating hcp_en_pbm${NC}"
sudo -u postgres psql -d hcp_en_pbm -f migrate_pbm_to_five_column.sql
echo -e "${GREEN}✓ hcp_en_pbm migrated${NC}"
echo ""

echo "========================================="
echo -e "${GREEN}All migrations complete!${NC}"
echo "========================================="
echo ""
echo "Next steps:"
echo "  1. Review compression gains with: sudo -u postgres psql -d hcp_en_pbm -c '\\dt+'"
echo "  2. Test queries on five-column structure"
echo "  3. Update Python code to use five-column token IDs"
echo "  4. Analyze and compress all fields (atomization, metadata, etc.)"
echo ""
