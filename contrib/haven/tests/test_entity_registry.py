#!/usr/bin/env python3
"""
Tests for Haven entity registry.

Tests entity token generation, lookup, and consistency.
"""

import sys
import os
import pytest

# Add parent directory to path
sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))

from entity_registry import (
    get_token_id, get_all_tokens, ENTITIES, DICategory,
    DI_NAMESPACE
)


class TestEntityRegistry:
    """Test entity registry functionality."""

    def test_agent_tokens(self):
        """Agents should have consistent token IDs."""
        silas = get_token_id('silas')
        planner = get_token_id('planner')
        di = get_token_id('di')

        assert silas is not None
        assert planner is not None
        assert di is not None

        # All should start with dA (DI namespace)
        assert silas.startswith('dA.')
        assert planner.startswith('dA.')
        assert di.startswith('dA.')

        # All should be in agent category (AA)
        assert '.AA.' in silas
        assert '.AA.' in planner
        assert '.AA.' in di

    def test_people_tokens(self):
        """People should have consistent token IDs."""
        b = get_token_id('b')
        p = get_token_id('p')

        assert b is not None
        assert p is not None

        # Should be in people category (AI)
        assert '.AI.' in b
        assert '.AI.' in p

    def test_project_tokens(self):
        """Projects should have consistent token IDs."""
        di_cognome = get_token_id('di-cognome')
        haven_ops = get_token_id('haven-ops')

        assert di_cognome is not None
        assert haven_ops is not None

        # Should be in project category (AF)
        assert '.AF.' in di_cognome
        assert '.AF.' in haven_ops

    def test_infrastructure_tokens(self):
        """Infrastructure should have consistent token IDs."""
        silas_cc = get_token_id('silas-cc')
        planner_cc = get_token_id('planner-cc')
        silas_api = get_token_id('silas-api')

        assert silas_cc is not None
        assert planner_cc is not None
        assert silas_api is not None

        # Should be in infra category (AG)
        assert '.AG.' in silas_cc
        assert '.AG.' in planner_cc
        assert '.AG.' in silas_api

    def test_service_tokens(self):
        """Services should have consistent token IDs."""
        haven = get_token_id('haven')
        vps = get_token_id('vps')

        assert haven is not None
        assert vps is not None

        # Should be in service category (AH)
        assert '.AH.' in haven
        assert '.AH.' in vps

    def test_unknown_entity(self):
        """Unknown entities should return None."""
        result = get_token_id('nonexistent')
        assert result is None

    def test_case_insensitive(self):
        """Entity lookup should be case insensitive."""
        silas_lower = get_token_id('silas')
        silas_upper = get_token_id('SILAS')
        silas_mixed = get_token_id('Silas')

        assert silas_lower == silas_upper == silas_mixed

    def test_all_entities_have_tokens(self):
        """All defined entities should generate valid tokens."""
        for name in ENTITIES.keys():
            token = get_token_id(name)
            assert token is not None, f"Entity '{name}' has no token"
            assert token.startswith('dA.'), f"Entity '{name}' token doesn't start with dA"

    def test_no_duplicate_tokens(self):
        """Each entity should have a unique token."""
        tokens = []
        for name in ENTITIES.keys():
            token = get_token_id(name)
            assert token not in tokens, f"Duplicate token for '{name}': {token}"
            tokens.append(token)

    def test_get_all_tokens(self):
        """get_all_tokens should return organized dict."""
        all_tokens = get_all_tokens()

        assert 'agents' in all_tokens
        assert 'people' in all_tokens
        assert 'projects' in all_tokens
        assert 'infrastructure' in all_tokens
        assert 'services' in all_tokens

        # Each category should have entries
        assert len(all_tokens['agents']) > 0
        assert len(all_tokens['people']) > 0

    def test_category_constants(self):
        """Category constants should be defined."""
        assert DICategory.AGENT == 0
        assert DICategory.PROJECT == 5
        assert DICategory.INFRA == 6
        assert DICategory.SERVICE == 7
        assert DICategory.PEOPLE == 8


if __name__ == '__main__':
    pytest.main([__file__, '-v'])
