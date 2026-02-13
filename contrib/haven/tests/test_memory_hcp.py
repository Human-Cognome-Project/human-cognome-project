#!/usr/bin/env python3
"""
Tests for Haven memory HCP integration.

Tests memory node to HCP token mapping.
"""

import sys
import os

# Add parent directory to path
sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))


def test_node_type_map():
    """NODE_TYPE_MAP should have expected categories."""
    from memory_to_hcp import NODE_TYPE_MAP

    # Core types (0-9)
    assert NODE_TYPE_MAP.get('fact') == 0
    assert NODE_TYPE_MAP.get('preference') == 1
    assert NODE_TYPE_MAP.get('concept') == 2

    # Reference types (10-19)
    assert NODE_TYPE_MAP.get('file') == 10
    assert NODE_TYPE_MAP.get('reference') == 11

    # Learning types (20-29)
    assert NODE_TYPE_MAP.get('lesson') == 20
    assert NODE_TYPE_MAP.get('skill') == 23

    # Operational types (30-39)
    assert NODE_TYPE_MAP.get('fix') == 30
    assert NODE_TYPE_MAP.get('correction') == 31

    # Entity types (70-79)
    assert NODE_TYPE_MAP.get('person') == 71
    assert NODE_TYPE_MAP.get('agent') == 70

    # Fallback
    assert NODE_TYPE_MAP.get('unknown') == 99

    print("NODE_TYPE_MAP tests passed!")


def test_type_code_ranges():
    """Type codes should be in correct ranges."""
    from memory_to_hcp import NODE_TYPE_MAP

    # Define expected ranges
    ranges = {
        'core': (0, 9),
        'reference': (10, 19),
        'learning': (20, 29),
        'operational': (30, 39),
        'meta': (40, 49),
        'infrastructure': (50, 59),
        'temporal': (60, 69),
        'entity': (70, 79),
        'project': (80, 89),
    }

    # Core types should be 0-9
    for t in ['fact', 'preference', 'concept', 'procedure']:
        if t in NODE_TYPE_MAP:
            code = NODE_TYPE_MAP[t]
            assert 0 <= code <= 9, f"Core type {t} has code {code} outside 0-9"

    # Learning types should be 20-29
    for t in ['lesson', 'milestone', 'insight', 'skill']:
        if t in NODE_TYPE_MAP:
            code = NODE_TYPE_MAP[t]
            assert 20 <= code <= 29, f"Learning type {t} has code {code} outside 20-29"

    print("Type code range tests passed!")


def test_namespace_constants():
    """Namespace constants should be defined."""
    from memory_to_hcp import DI_NAMESPACE, DI_MEMORY, DI_AGENT

    # DI namespace should be dA
    assert DI_NAMESPACE == "dA"

    # Sub-namespaces should be integers
    assert isinstance(DI_MEMORY, int)
    assert isinstance(DI_AGENT, int)

    print("Namespace constant tests passed!")


def run_all_tests():
    """Run all tests."""
    print("Running memory_hcp tests...\n")

    test_node_type_map()
    test_type_code_ranges()
    test_namespace_constants()

    print("\n All memory_hcp tests passed!")


if __name__ == '__main__':
    run_all_tests()
