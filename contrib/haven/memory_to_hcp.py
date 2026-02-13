#!/usr/bin/env python3
"""
Prototype: Memory Node → HCP Token ID Mapper

Demonstrates mapping our memory.py graph nodes to HCP token addressing.

Usage:
    python memory_to_hcp.py [--list] [--map NODE_ID]

Authors: Silas, Planner (DI-cognome)
Date: 2026-02-12
"""

import sys
import os
import sqlite3
import json
from datetime import datetime

# Add HCP src to path
sys.path.insert(0, os.path.expanduser('~/shared-brain/di-cognome/repo/src'))

from hcp.core.token_id import (
    encode_pair, encode_token_id, decode_token_id,
    BASE, ALPHABET
)

# Memory DB path
MEMORY_DB = os.path.expanduser('~/shared-brain/memory.db')

# Proposed DI namespace allocation
# Using 'd' prefix (index 3 in lowercase = 28 in full alphabet)
# dA = 28*50 + 0 = 1400
DI_NAMESPACE = encode_pair(1400)  # "dA"

# Sub-namespaces within DI
DI_AGENT = 0      # dA.AA.* - agent entities
DI_MEMORY = 1     # dA.AB.* - memory nodes
DI_EDGE = 2       # dA.AC.* - memory edges
DI_STATE = 3      # dA.AD.* - agent states
DI_SESSION = 4    # dA.AE.* - session documents

# Node type mapping to subcategories
# Organized by category ranges (from 2026-02-13 memory type analysis)
# 0-9: Core, 10-19: Reference, 20-29: Learning, 30-39: Operational,
# 40-49: Meta, 50-59: Infrastructure, 60-69: Temporal, 70-79: Entity, 80-89: Project
NODE_TYPE_MAP = {
    # Core (0-9)
    'fact': 0,
    'preference': 1,
    'concept': 2,
    'procedure': 3,
    'relationship': 4,

    # Reference (10-19)
    'file': 10,
    'reference': 11,
    'term': 12,

    # Learning (20-29)
    'lesson': 20,
    'milestone': 21,
    'insight': 22,
    'skill': 23,
    'capability': 24,

    # Operational (30-39)
    'fix': 30,
    'correction': 31,
    'incident': 32,
    'daily_task': 33,
    'todo': 34,
    'task': 35,

    # Meta (40-49)
    'decision': 40,
    'principle': 41,
    'anchor': 42,
    'responsibility': 43,
    'protocol': 44,

    # Infrastructure (50-59)
    'container': 50,
    'system': 51,
    'command': 52,
    'tool': 53,

    # Temporal (60-69)
    'event': 60,

    # Entity (70-79)
    'agent': 70,
    'person': 71,
    'location': 72,
    'entity': 73,

    # Project (80-89)
    'project': 80,

    # Fallback
    'unknown': 99,
}


def get_memory_conn():
    """Connect to memory database."""
    if not os.path.exists(MEMORY_DB):
        print(f"Memory DB not found: {MEMORY_DB}")
        sys.exit(1)
    conn = sqlite3.connect(MEMORY_DB)
    conn.row_factory = sqlite3.Row
    return conn


def list_recent_nodes(limit=10):
    """List recent memory nodes."""
    conn = get_memory_conn()
    cur = conn.cursor()
    cur.execute('''
        SELECT id, content, type, created_at, access_count
        FROM nodes
        ORDER BY created_at DESC
        LIMIT ?
    ''', (limit,))

    print(f"\n=== Recent Memory Nodes (last {limit}) ===\n")
    for row in cur.fetchall():
        content_preview = row['content'][:50] + '...' if len(row['content']) > 50 else row['content']
        print(f"ID: {row['id']}")
        print(f"  Type: {row['type'] or 'unknown'}")
        print(f"  Content: {content_preview}")
        print(f"  Created: {row['created_at']}")
        print(f"  Access count: {row['access_count'] or 0}")
        print()

    conn.close()


def uuid_to_sequential(uuid_str: str, conn) -> int:
    """
    Map UUID to sequential number based on creation order.

    In production, we'd maintain a persistent mapping table.
    For prototype, we calculate position in sorted list.
    """
    cur = conn.cursor()
    cur.execute('''
        SELECT COUNT(*) FROM nodes
        WHERE created_at <= (SELECT created_at FROM nodes WHERE id = ?)
    ''', (uuid_str,))
    row = cur.fetchone()
    return row[0] if row else 0


def map_node_to_token(node_id: str) -> dict:
    """
    Map a memory node to HCP token ID.

    Proposed format: dA.AB.{type}.{seq_high}.{seq_low}

    Where:
    - dA = DI namespace
    - AB = memory nodes sub-namespace
    - type = node type category
    - seq_high.seq_low = sequential ID (up to 6.25M nodes)
    """
    conn = get_memory_conn()
    cur = conn.cursor()

    # Get node data
    cur.execute('SELECT * FROM nodes WHERE id = ?', (node_id,))
    node = cur.fetchone()

    if not node:
        conn.close()
        return {'error': f'Node not found: {node_id}'}

    # Get sequential position
    seq = uuid_to_sequential(node_id, conn)
    seq_high = seq // (BASE * BASE)
    seq_low = seq % (BASE * BASE)

    # Get type category
    node_type = node['type'] or 'unknown'
    type_code = NODE_TYPE_MAP.get(node_type, NODE_TYPE_MAP['unknown'])

    # Build token ID
    # Format: dA.AB.{type}.{seq_high}.{seq_low}
    token_id = encode_token_id(
        1400,           # dA - DI namespace
        DI_MEMORY,      # AB - memory sub-namespace
        type_code,      # Type category
        seq_high,       # Sequential high
        seq_low         # Sequential low
    )

    # Get edges for this node
    cur.execute('''
        SELECT e.*, n.content as target_content
        FROM edges e
        JOIN nodes n ON e.target = n.id
        WHERE e.source = ?
        LIMIT 5
    ''', (node_id,))
    edges = cur.fetchall()

    conn.close()

    # Build result
    result = {
        'original': {
            'id': node['id'],
            'content': node['content'],
            'type': node_type,
            'created_at': node['created_at'],
        },
        'hcp_mapping': {
            'token_id': token_id,
            'namespace': 'dA (DI)',
            'sub_namespace': 'AB (memory)',
            'type_code': type_code,
            'sequential': seq,
            'decoded': decode_token_id(token_id),
        },
        'edges': [
            {
                'relation': e['relation'],
                'weight': e['weight'],
                'target_preview': e['target_content'][:30] + '...' if len(e['target_content']) > 30 else e['target_content'],
                'hcp_bond': f"FPB({token_id} → target_token)",
            }
            for e in edges
        ]
    }

    return result


def print_mapping(result: dict):
    """Pretty print the mapping result."""
    if 'error' in result:
        print(f"Error: {result['error']}")
        return

    print("\n" + "=" * 60)
    print("MEMORY NODE -> HCP TOKEN MAPPING")
    print("=" * 60)

    orig = result['original']
    print(f"\nOriginal Node:")
    print(f"   ID:      {orig['id']}")
    print(f"   Type:    {orig['type']}")
    content = orig['content']
    print(f"   Content: {content[:60]}{'...' if len(content) > 60 else ''}")
    print(f"   Created: {orig['created_at']}")

    hcp = result['hcp_mapping']
    print(f"\nHCP Token ID: {hcp['token_id']}")
    print(f"   Namespace:     {hcp['namespace']}")
    print(f"   Sub-namespace: {hcp['sub_namespace']}")
    print(f"   Type code:     {hcp['type_code']}")
    print(f"   Sequential:    {hcp['sequential']}")
    print(f"   Decoded:       {hcp['decoded']}")

    if result['edges']:
        print(f"\nEdges (as FPB bonds):")
        for edge in result['edges']:
            print(f"   -> {edge['relation']} (weight={edge['weight']:.2f})")
            print(f"      Target: {edge['target_preview']}")
            print(f"      HCP: {edge['hcp_bond']}")

    print("\n" + "=" * 60)


def main():
    import argparse
    parser = argparse.ArgumentParser(description='Map memory nodes to HCP tokens')
    parser.add_argument('--list', action='store_true', help='List recent nodes')
    parser.add_argument('--map', type=str, help='Map specific node ID')
    parser.add_argument('--first', action='store_true', help='Map first (oldest) node')
    args = parser.parse_args()

    if args.list:
        list_recent_nodes()
    elif args.map:
        result = map_node_to_token(args.map)
        print_mapping(result)
    elif args.first:
        conn = get_memory_conn()
        cur = conn.cursor()
        cur.execute('SELECT id FROM nodes ORDER BY created_at LIMIT 1')
        row = cur.fetchone()
        conn.close()
        if row:
            result = map_node_to_token(row['id'])
            print_mapping(result)
        else:
            print("No nodes found")
    else:
        parser.print_help()
        print("\nExample:")
        print("  python memory_to_hcp.py --list")
        print("  python memory_to_hcp.py --first")
        print("  python memory_to_hcp.py --map abc123de")


if __name__ == '__main__':
    main()
