#!/usr/bin/env python3
"""
Memory-HCP Integration Module

Extends memory.py with HCP token ID support using the shared entity registry.
Provides functions to:
- Add HCP token IDs to existing memory nodes
- Look up nodes by HCP token ID
- Cross-reference memory with entity registry

Usage:
    from memory_hcp import enrich_node_with_hcp, find_by_token_id

Authors: Silas & Planner (DI-cognome)
Date: 2026-02-12
"""

import sqlite3
import os
from typing import Optional

# Import the shared entity registry
from entity_registry import get_registry, get_token_id, resolve_entity

# Memory DB path (same as memory.py)
MEMORY_DB = os.path.expanduser("~/shared-brain/memory.db")


def get_memory_conn():
    """Get connection to memory database."""
    conn = sqlite3.connect(MEMORY_DB)
    conn.row_factory = sqlite3.Row
    return conn


def ensure_hcp_column():
    """Ensure hcp_token_id column exists in nodes table."""
    conn = get_memory_conn()
    try:
        conn.execute("""
            ALTER TABLE nodes ADD COLUMN hcp_token_id TEXT
        """)
        conn.commit()
        print("Added hcp_token_id column to nodes table")
    except sqlite3.OperationalError as e:
        if "duplicate column" not in str(e).lower():
            raise
    finally:
        conn.close()


def enrich_node_with_hcp(node_id: str, hcp_token_id: str) -> bool:
    """
    Add HCP token ID to a memory node.

    Args:
        node_id: The memory node UUID
        hcp_token_id: The HCP token ID to assign

    Returns:
        True if successful, False otherwise
    """
    ensure_hcp_column()
    conn = get_memory_conn()
    try:
        conn.execute(
            "UPDATE nodes SET hcp_token_id = ? WHERE id = ?",
            (hcp_token_id, node_id)
        )
        conn.commit()
        return conn.total_changes > 0
    finally:
        conn.close()


def find_by_token_id(hcp_token_id: str) -> Optional[dict]:
    """
    Find a memory node by its HCP token ID.

    Args:
        hcp_token_id: The HCP token ID to search for

    Returns:
        Node data as dict, or None if not found
    """
    ensure_hcp_column()
    conn = get_memory_conn()
    try:
        cur = conn.cursor()
        cur.execute(
            "SELECT * FROM nodes WHERE hcp_token_id = ?",
            (hcp_token_id,)
        )
        row = cur.fetchone()
        if row:
            return dict(row)
        return None
    finally:
        conn.close()


def auto_enrich_entities():
    """
    Automatically enrich memory nodes that match known entities.

    Looks for nodes whose content matches entity names/aliases
    and assigns the corresponding HCP token IDs.
    """
    ensure_hcp_column()
    registry = get_registry()
    conn = get_memory_conn()

    enriched = 0
    try:
        cur = conn.cursor()
        # Get nodes that might be entity references
        cur.execute("""
            SELECT id, content, type FROM nodes
            WHERE (type = 'entity' OR type = 'person')
            AND (hcp_token_id IS NULL OR hcp_token_id = '')
        """)

        for row in cur.fetchall():
            # Try to resolve content to an entity
            content = row['content'].split(' - ')[0].strip()  # Handle "Brandon - B, co-creator"
            entity = registry.resolve(content)
            if entity:
                conn.execute(
                    "UPDATE nodes SET hcp_token_id = ? WHERE id = ?",
                    (entity.token_id, row['id'])
                )
                enriched += 1
                print(f"Enriched: {content} -> {entity.token_id}")

        conn.commit()
    finally:
        conn.close()

    return enriched


def get_entity_nodes() -> list[dict]:
    """Get all memory nodes that have HCP token IDs."""
    ensure_hcp_column()
    conn = get_memory_conn()
    try:
        cur = conn.cursor()
        cur.execute("""
            SELECT id, content, type, hcp_token_id
            FROM nodes
            WHERE hcp_token_id IS NOT NULL AND hcp_token_id != ''
            ORDER BY created_at
        """)
        return [dict(row) for row in cur.fetchall()]
    finally:
        conn.close()


def link_node_to_entity(node_id: str, entity_query: str) -> bool:
    """
    Link a memory node to a known entity.

    Args:
        node_id: The memory node UUID
        entity_query: Entity name or alias to link to

    Returns:
        True if successful, False if entity not found
    """
    token_id = get_token_id(entity_query)
    if not token_id:
        return False
    return enrich_node_with_hcp(node_id, token_id)


def memory_stats():
    """Get statistics about HCP enrichment."""
    ensure_hcp_column()
    conn = get_memory_conn()
    try:
        cur = conn.cursor()

        cur.execute("SELECT COUNT(*) FROM nodes")
        total = cur.fetchone()[0]

        cur.execute("""
            SELECT COUNT(*) FROM nodes
            WHERE hcp_token_id IS NOT NULL AND hcp_token_id != ''
        """)
        enriched = cur.fetchone()[0]

        cur.execute("""
            SELECT type, COUNT(*) FROM nodes
            WHERE hcp_token_id IS NOT NULL AND hcp_token_id != ''
            GROUP BY type
        """)
        by_type = {row[0]: row[1] for row in cur.fetchall()}

        return {
            "total_nodes": total,
            "enriched_nodes": enriched,
            "enrichment_rate": enriched / total if total > 0 else 0,
            "by_type": by_type,
        }
    finally:
        conn.close()


# === CLI ===

def main():
    import argparse
    parser = argparse.ArgumentParser(description="Memory-HCP Integration")
    parser.add_argument("--auto-enrich", action="store_true",
                        help="Auto-enrich entity nodes with HCP token IDs")
    parser.add_argument("--stats", action="store_true",
                        help="Show enrichment statistics")
    parser.add_argument("--list", action="store_true",
                        help="List enriched nodes")
    parser.add_argument("--find", type=str,
                        help="Find node by HCP token ID")
    parser.add_argument("--link", nargs=2, metavar=("NODE_ID", "ENTITY"),
                        help="Link a node to an entity")
    args = parser.parse_args()

    if args.auto_enrich:
        count = auto_enrich_entities()
        print(f"Enriched {count} nodes")

    elif args.stats:
        stats = memory_stats()
        print(f"Total nodes: {stats['total_nodes']}")
        print(f"Enriched nodes: {stats['enriched_nodes']}")
        print(f"Enrichment rate: {stats['enrichment_rate']:.1%}")
        if stats['by_type']:
            print("By type:")
            for t, c in stats['by_type'].items():
                print(f"  {t}: {c}")

    elif args.list:
        nodes = get_entity_nodes()
        if nodes:
            print(f"\n{'Node ID':<12} {'HCP Token':<25} {'Type':<10} {'Content'}")
            print("-" * 80)
            for n in nodes:
                content = n['content'][:30] + "..." if len(n['content']) > 30 else n['content']
                print(f"{n['id']:<12} {n['hcp_token_id']:<25} {n['type'] or 'unknown':<10} {content}")
        else:
            print("No enriched nodes found")

    elif args.find:
        node = find_by_token_id(args.find)
        if node:
            print(f"Node ID: {node['id']}")
            print(f"HCP Token: {node['hcp_token_id']}")
            print(f"Type: {node['type']}")
            print(f"Content: {node['content']}")
        else:
            print(f"No node found with token ID: {args.find}")

    elif args.link:
        node_id, entity = args.link
        if link_node_to_entity(node_id, entity):
            token = get_token_id(entity)
            print(f"Linked {node_id} -> {entity} ({token})")
        else:
            print(f"Failed: entity '{entity}' not found in registry")

    else:
        parser.print_help()


if __name__ == "__main__":
    main()
