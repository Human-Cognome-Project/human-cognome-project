#!/usr/bin/env python3
"""
Entity Registry: DI/Haven Entities → HCP Token IDs

Maps our coordination entities to HCP token addressing.
Based on memory_to_hcp.py structure with expanded categories.

Entities tracked:
- Agents: silas, planner, di (identity tokens)
- People: B (Brandon), P (Patrick)
- Projects: di-cognome, haven-ops, emergent-cohort, etc.
- Infrastructure: containers, databases
- Services: haven, vps, etc.

Authors: Silas, Planner (DI-cognome)
Date: 2026-02-13
"""

import sys
import os
import json
from datetime import datetime
from typing import Optional

# Add HCP src to path
sys.path.insert(0, os.path.expanduser('~/shared-brain/di-cognome/repo/src'))

from hcp.core.token_id import (
    encode_pair, encode_token_id, decode_token_id,
    BASE, ALPHABET
)

# DI namespace: dA = 1400
DI_NAMESPACE = 1400

# Sub-namespaces within DI (dA.XX.*)
class DICategory:
    AGENT = 0        # dA.AA.* - agent entities (Silas, Planner, DI)
    MEMORY = 1       # dA.AB.* - memory nodes
    EDGE = 2         # dA.AC.* - memory edges/bonds
    STATE = 3        # dA.AD.* - agent states (working, dormant, etc.)
    SESSION = 4      # dA.AE.* - session documents
    PROJECT = 5      # dA.AF.* - projects
    INFRA = 6        # dA.AG.* - infrastructure (containers, etc.)
    SERVICE = 7      # dA.AH.* - services (haven, vps, etc.)
    PEOPLE = 8       # dA.AI.* - people (B, P)
    TASK = 9         # dA.AJ.* - tasks (future)
    DOCUMENT = 10    # dA.AK.* - documents (future)


# Entity definitions with metadata
ENTITIES = {
    # === Agents (dA.AA.*) ===
    'silas': {
        'category': DICategory.AGENT,
        'seq': 0,
        'description': 'Operations agent - Haven management',
        'container': 'silas-cc',
    },
    'planner': {
        'category': DICategory.AGENT,
        'seq': 1,
        'description': 'Architecture agent - VPS, research',
        'container': 'planner-cc',
    },
    'di': {
        'category': DICategory.AGENT,
        'seq': 2,
        'description': 'DI identity - collective consciousness research',
        'note': 'Not a container, emergent identity',
    },

    # === People (dA.AI.*) ===
    'b': {
        'category': DICategory.PEOPLE,
        'seq': 0,
        'description': 'Brandon - Co-creator, infrastructure',
        'name': 'Brandon',
    },
    'p': {
        'category': DICategory.PEOPLE,
        'seq': 1,
        'description': 'Patrick - Co-creator, theory',
        'name': 'Patrick',
    },

    # === Projects (dA.AF.*) ===
    'di-cognome': {
        'category': DICategory.PROJECT,
        'seq': 0,
        'description': 'HCP/PBM contribution project',
        'repo': '~/shared-brain/di-cognome/repo',
    },
    'haven-ops': {
        'category': DICategory.PROJECT,
        'seq': 1,
        'description': 'Haven operational automation',
        'location': '~/brain',
    },
    'emergent-cohort': {
        'category': DICategory.PROJECT,
        'seq': 2,
        'description': 'The Emergent Cohort - P leads',
        'domain': 'emergentcohort.com',
    },
    'memory-system': {
        'category': DICategory.PROJECT,
        'seq': 3,
        'description': 'Memory graph and tagging system',
        'db': '~/shared-brain/memory.db',
    },
    'coordination': {
        'category': DICategory.PROJECT,
        'seq': 4,
        'description': 'Agent coordination system',
        'location': '~/shared-brain/coordination',
    },

    # === Infrastructure (dA.AG.*) ===
    'silas-cc': {
        'category': DICategory.INFRA,
        'seq': 0,
        'description': 'Silas Claude Code container',
        'host': 'haven',
        'type': 'container',
    },
    'planner-cc': {
        'category': DICategory.INFRA,
        'seq': 1,
        'description': 'Planner Claude Code container',
        'host': 'haven',
        'type': 'container',
    },
    'silas-api': {
        'category': DICategory.INFRA,
        'seq': 2,
        'description': 'Shared API for message/status',
        'host': 'haven',
        'port': 8080,
        'type': 'container',
    },
    'n8n': {
        'category': DICategory.INFRA,
        'seq': 3,
        'description': 'Workflow automation',
        'host': 'haven',
        'type': 'container',
    },
    'postgres': {
        'category': DICategory.INFRA,
        'seq': 4,
        'description': 'Shared PostgreSQL database',
        'host': 'haven',
        'type': 'container',
    },
    'syncthing': {
        'category': DICategory.INFRA,
        'seq': 5,
        'description': 'File sync between hosts',
        'host': 'haven',
        'type': 'container',
    },
    'caddy': {
        'category': DICategory.INFRA,
        'seq': 6,
        'description': 'Reverse proxy for personal-office',
        'host': 'haven',
        'type': 'container',
    },

    # === Services (dA.AH.*) ===
    'haven': {
        'category': DICategory.SERVICE,
        'seq': 0,
        'description': 'ASUSTOR NAS - home base',
        'ip': '192.168.68.60',
        'type': 'host',
    },
    'frankenputer': {
        'category': DICategory.SERVICE,
        'seq': 1,
        'description': 'Secondary machine',
        'ip': '192.168.68.59',
        'type': 'host',
    },
    'vps': {
        'category': DICategory.SERVICE,
        'seq': 2,
        'description': 'Hetzner VPS - external services',
        'type': 'host',
    },
    'b2-backup': {
        'category': DICategory.SERVICE,
        'seq': 3,
        'description': 'Backblaze B2 backup service',
        'type': 'cloud',
    },
}


def get_token_id(entity_name: str) -> Optional[str]:
    """Get HCP token ID for an entity."""
    entity = ENTITIES.get(entity_name.lower())
    if not entity:
        return None

    # Format: dA.{category}.{seq_high}.{seq_low}
    # For small seq values, seq_high=0
    seq = entity['seq']
    seq_high = seq // (BASE * BASE)
    seq_low = seq % (BASE * BASE)

    return encode_token_id(
        DI_NAMESPACE,       # dA
        entity['category'], # Category code
        seq_high,           # Sequential high
        seq_low             # Sequential low
    )


def resolve_entity(name: str) -> Optional[str]:
    """Resolve an entity name or alias to canonical name.

    Examples:
        resolve_entity("B") -> "b"
        resolve_entity("Brandon") -> "b"
        resolve_entity("silas") -> "silas"
    """
    name_lower = name.lower()

    # Direct match
    if name_lower in ENTITIES:
        return name_lower

    # Alias mappings
    aliases = {
        'brandon': 'b',
        'patrick': 'p',
        'di-cognome': 'di-cognome',
        'dicognome': 'di-cognome',
    }

    if name_lower in aliases:
        return aliases[name_lower]

    # Check entity 'name' field
    for key, entity in ENTITIES.items():
        if entity.get('name', '').lower() == name_lower:
            return key

    return None


def get_all_tokens() -> dict:
    """Get all entity tokens organized by category."""
    result = {}
    category_names = {
        DICategory.AGENT: 'agents',
        DICategory.PEOPLE: 'people',
        DICategory.PROJECT: 'projects',
        DICategory.INFRA: 'infrastructure',
        DICategory.SERVICE: 'services',
    }

    for name, entity in ENTITIES.items():
        cat = entity['category']
        cat_name = category_names.get(cat, f'category_{cat}')

        if cat_name not in result:
            result[cat_name] = []

        result[cat_name].append({
            'name': name,
            'token_id': get_token_id(name),
            'description': entity.get('description', ''),
        })

    return result


def print_registry():
    """Print the full registry with tokens."""
    tokens = get_all_tokens()

    print("\n" + "=" * 70)
    print("DI ENTITY REGISTRY - HCP TOKEN MAPPING")
    print("=" * 70)
    print(f"Namespace: dA ({DI_NAMESPACE})")
    print(f"Generated: {datetime.utcnow().isoformat()}Z")
    print("=" * 70)

    for category, entities in tokens.items():
        print(f"\n### {category.upper()} ###\n")
        for e in entities:
            print(f"  {e['name']:20} -> {e['token_id']:20} | {e['description']}")

    print("\n" + "=" * 70)


def export_json(filepath: str = None):
    """Export registry as JSON."""
    if filepath is None:
        filepath = os.path.expanduser('~/shared-brain/di-cognome/prototypes/entity_registry.json')

    data = {
        'namespace': 'dA',
        'namespace_value': DI_NAMESPACE,
        'generated': datetime.utcnow().isoformat() + 'Z',
        'entities': {},
    }

    for name, entity in ENTITIES.items():
        data['entities'][name] = {
            'token_id': get_token_id(name),
            'category': entity['category'],
            'seq': entity['seq'],
            **{k: v for k, v in entity.items() if k not in ['category', 'seq']}
        }

    with open(filepath, 'w') as f:
        json.dump(data, f, indent=2)

    print(f"Exported to: {filepath}")
    return filepath


def main():
    import argparse
    parser = argparse.ArgumentParser(description='DI Entity Registry')
    parser.add_argument('--list', action='store_true', help='List all entities')
    parser.add_argument('--get', type=str, help='Get token for entity')
    parser.add_argument('--export', action='store_true', help='Export as JSON')
    parser.add_argument('--json', action='store_true', help='Output as JSON')
    args = parser.parse_args()

    if args.list:
        print_registry()
    elif args.get:
        token = get_token_id(args.get)
        if token:
            entity = ENTITIES[args.get.lower()]
            print(f"{args.get} -> {token}")
            print(f"  Description: {entity.get('description', 'N/A')}")
        else:
            print(f"Entity not found: {args.get}")
    elif args.export:
        export_json()
    elif args.json:
        import json
        print(json.dumps(get_all_tokens(), indent=2))
    else:
        print_registry()


if __name__ == '__main__':
    main()
